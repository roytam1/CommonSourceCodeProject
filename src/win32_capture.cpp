/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.27 -

	[ win32 capture ]
*/

#include <malloc.h>
#include "emu.h"
#include "vm/vm.h"

LPSTR EMU::MyAtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	lpa[0] = '\0';
	WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}
#define MyW2T(lpw) (((_lpw = lpw) == NULL) ? NULL : (_convert = (lstrlenW(_lpw) + 1) * 2, MyAtlW2AHelper((LPSTR)_alloca(_convert), _lpw, _convert)))

void EMU::initialize_capture()
{
	HRESULT hr;
	capture_devs = 0;
	capture_connected = -1;
	
	// initialize com
	CoInitialize(NULL);
	
	// create the system device enum
	ICreateDevEnum *pDevEnum = NULL;
	if(FAILED(hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&pDevEnum)))
		return;
	// create the video input device enu,
	IEnumMoniker *pClassEnum = NULL;
	if(FAILED(hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0)))
		return;
	// no capture devices ?
	if(pClassEnum == NULL)
		return;
	// enumerate the capture devices
	ULONG cFetched;
	IMoniker *pMoniker = NULL;
	while(hr = pClassEnum->Next(1, &pMoniker, &cFetched), hr == S_OK) {
		IPropertyBag *pBag;
		if(!FAILED(hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag))) {
			VARIANT var;
			var.vt = VT_BSTR;
			if((hr = pBag->Read(L"FriendlyName", &var, NULL)) == NOERROR) {
				LPCWSTR _lpw = NULL;
				int _convert = 0;
				_tcscpy(capture_dev_name[capture_devs++], MyW2T(var.bstrVal));
				SysFreeString(var.bstrVal);
				pMoniker->AddRef();
			}
			pBag->Release();
		}
		pMoniker->Release();
		if(capture_devs >= CAPTURE_MAX)
			break;
	}
	pClassEnum->Release();
	pDevEnum->Release();
}

void EMU::release_capture()
{
	if(capture_connected != -1)
		disconnect_capture_device();
	
	// release com
	CoUninitialize();
}

void EMU::connect_capture_device(int index, bool pin)
{
	if(capture_connected == index && !pin)
		return;
	if(capture_connected != -1)
		disconnect_capture_device();
	
	// create the filter graph
	HRESULT hr;
	if(FAILED(hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraph)))
		return;
	// create the system device emum
	ICreateDevEnum *pDevEnum = NULL;
	if(FAILED(hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **)&pDevEnum)))
		return;
	// create the video input device enu,
	IEnumMoniker *pClassEnum = NULL;
	if(FAILED(hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0)))
		return;
	// no capture devices ?
	if(pClassEnum == NULL)
		return;
	
	// connect te capture device
	ULONG cFetched;
	IMoniker *pMoniker = NULL;
	for(int i = 0; i < index; i++) {
		hr = pClassEnum->Next(1, &pMoniker, &cFetched);
		pMoniker->Release();
	}
	if((hr = pClassEnum->Next(1, &pMoniker, &cFetched)) == S_OK) {
		// bind the moniker to the filter object
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pSrc);
		pMoniker->Release();
	}
	pClassEnum->Release();
	pDevEnum->Release();
	
	// verify connection
	if(FAILED(hr))
		return;
	// add the capture filter
	if(FAILED(hr = pGraph->AddFilter(pSrc, L"Video Capture")))
		return;
	// create the capture graph builder
	if(FAILED(hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pBuilder)))
		return;
	if(FAILED(hr = pBuilder->SetFiltergraph(pGraph)))
		return;
	// create the pin filter first to show the crossbar property
	IAMStreamConfig *pSC;
	hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pSrc, IID_IAMStreamConfig, (void **)&pSC);
	if(hr != NOERROR)
		hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrc, IID_IAMStreamConfig, (void **)&pSC);
	if(hr == NOERROR) {
		ISpecifyPropertyPages *pSpec;
		CAUUID cauuid;
		hr = pSC->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
		if(hr == S_OK) {
			hr = pSpec->GetPages(&cauuid);
			if(pin) {
				hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)&pSC, cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
				CoTaskMemFree(cauuid.pElems);
			}
			pSpec->Release();
		}
		pSC->Release();
	}
	// create the sample grabber
	if(FAILED(hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pF)))
		return;
	if(FAILED(hr = pF->QueryInterface(IID_ISampleGrabber, (void **)&pSGrab)))
		return;
	// set the media type
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;// MEDIASUBTYPE_UYVY;
	mt.formattype = FORMAT_VideoInfo;
	if(FAILED(hr = pSGrab->SetMediaType(&mt)))
		return;
	// add to the filter graph
	if(FAILED(hr = pGraph->AddFilter(pF, L"Grabber")))
		return;
	// connect the sample grabber
	// [pSrc](o) -> (i)[pF](o) -> [VideoRender]
	//        ªA   ªB     ªC
	IPin *pSrcOut = get_pin(pSrc, PINDIR_OUTPUT);	// A
	IPin *pSGrabIn = get_pin(pF, PINDIR_INPUT);	// B
	IPin *pSGrabOut = get_pin(pF, PINDIR_OUTPUT);	// C
	if(FAILED(hr = pGraph->Connect(pSrcOut, pSGrabIn)))
		return;
//	if(FAILED(hr = pGraph->Render(pSGrabOut)))
//		return;
	// set the grabber mode
	if(FAILED(hr = pSGrab->SetBufferSamples(TRUE)))
		return;
	if(FAILED(hr = pSGrab->SetOneShot(FALSE)))
		return;
	// start capture
	if(FAILED(hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl)))
		return;
	if(FAILED(hr = pMediaControl->Run()))
		return;
	
	// get the capture size
	pSGrab->GetConnectedMediaType(&mt); 
	VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)mt.pbFormat;
	capture_width = pVideoHeader->bmiHeader.biWidth;
	capture_height = pVideoHeader->bmiHeader.biHeight;
	capture_src_height = (int)(SCREEN_HEIGHT * capture_width / SCREEN_WIDTH + 0.5);	// keep aspect ratio
	capture_src_y = (capture_height - capture_src_height) >> 1;
	// get the buffer size
	do {
		pSGrab->GetCurrentBuffer(&capture_bufsize, NULL);
	} while(capture_bufsize <= 0);
	capture_bufsize = capture_width * capture_height * 3;	// GetCurrentBuffer sometimes returns the wrong size
	
	// create DIBSection
	HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
	lpCapBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpCapDIB = (LPBITMAPINFO)lpCapBuf;
	lpCapDIB->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpCapDIB->bmiHeader.biWidth = capture_width;
	lpCapDIB->bmiHeader.biHeight = capture_height;
	lpCapDIB->bmiHeader.biPlanes = 1;
	lpCapDIB->bmiHeader.biBitCount = 24;
	lpCapDIB->bmiHeader.biCompression = BI_RGB;
	lpCapDIB->bmiHeader.biSizeImage = 0;
	lpCapDIB->bmiHeader.biXPelsPerMeter = 0;
	lpCapDIB->bmiHeader.biYPelsPerMeter = 0;
	lpCapDIB->bmiHeader.biClrUsed = 0;
	lpCapDIB->bmiHeader.biClrImportant = 0;
	hCapBMP = CreateDIBSection(hdc, lpCapDIB, DIB_RGB_COLORS, (PVOID*)&lpCapBMP, NULL, 0);
	hdcCapDIB = CreateCompatibleDC(hdc);
	SelectObject(hdcCapDIB, hCapBMP);
	
	capture_connected = index;
}

void EMU::disconnect_capture_device()
{
	if(capture_connected != -1) {
		// release DIBSection
		DeleteDC(hdcCapDIB);
		DeleteObject(hCapBMP);
		GlobalFree(lpCapBuf);
		
		// release dshow
		if(pF) pF->Release();
		if(pSGrab) pSGrab->Release();
		if(pSrc) pSrc->Release();
		if(pMediaControl) pMediaControl->Release();
		if(pBuilder) pBuilder->Release();
		if(pGraph) pGraph->Release();
		
		pF = NULL;
		pSGrab = NULL;
		pSrc = NULL;
		pMediaControl = NULL;
		pBuilder = NULL;
		pGraph = NULL;
	}
	capture_connected = -1;
}

void EMU::show_capture_device_filter()
{
	if(capture_connected != -1) {
		ISpecifyPropertyPages *pSpec;
		CAUUID cauuid;
		HRESULT hr = pSrc->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
		if(hr == S_OK) {
			hr = pSpec->GetPages(&cauuid);
			hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)&pSrc, cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
			CoTaskMemFree(cauuid.pElems);
			pSpec->Release();
		}
	}
}

void EMU::show_capture_device_pin()
{
	if(capture_connected != -1)
		connect_capture_device(capture_connected, true);
}

void EMU::show_capture_device_source()
{
	if(capture_connected != -1) {
		IAMCrossbar *pCrs;
		HRESULT hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pSrc, IID_IAMCrossbar, (void **)&pCrs);
		if(hr != NOERROR)
			hr = pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrc, IID_IAMCrossbar, (void **)&pCrs);
		if(hr != NOERROR)
			return;
		
		ISpecifyPropertyPages *pSpec;
		CAUUID cauuid;
		hr = pCrs->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
		if(hr == S_OK) {
			hr = pSpec->GetPages(&cauuid);
			hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)&pCrs, cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
			CoTaskMemFree(cauuid.pElems);
			pSpec->Release();
			
			// get the capture size
			AM_MEDIA_TYPE mt;
			pSGrab->GetConnectedMediaType(&mt); 
			VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)mt.pbFormat;
			capture_width = pVideoHeader->bmiHeader.biWidth;
			capture_height = pVideoHeader->bmiHeader.biHeight;
			capture_src_height = (int)(SCREEN_HEIGHT * capture_width / SCREEN_WIDTH + 0.5);	// keep aspect ratio
			capture_src_y = (capture_height - capture_src_height) >> 1;
			// get the buffer size
			do {
				pSGrab->GetCurrentBuffer(&capture_bufsize, NULL);
			} while(capture_bufsize <= 0);
			capture_bufsize = capture_width * capture_height * 3;	// GetCurrentBuffer sometimes returns the wrong size
			
			// release and re-create DIBSection
			DeleteDC(hdcCapDIB);
			DeleteObject(hCapBMP);
			GlobalFree(lpCapBuf);
			
			HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
			lpCapBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
			lpCapDIB = (LPBITMAPINFO)lpCapBuf;
			lpCapDIB->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			lpCapDIB->bmiHeader.biWidth = capture_width;
			lpCapDIB->bmiHeader.biHeight = capture_height;
			lpCapDIB->bmiHeader.biPlanes = 1;
			lpCapDIB->bmiHeader.biBitCount = 24;
			lpCapDIB->bmiHeader.biCompression = BI_RGB;
			lpCapDIB->bmiHeader.biSizeImage = 0;
			lpCapDIB->bmiHeader.biXPelsPerMeter = 0;
			lpCapDIB->bmiHeader.biYPelsPerMeter = 0;
			lpCapDIB->bmiHeader.biClrUsed = 0;
			lpCapDIB->bmiHeader.biClrImportant = 0;
			hCapBMP = CreateDIBSection(hdc, lpCapDIB, DIB_RGB_COLORS, (PVOID*)&lpCapBMP, NULL, 0);
			hdcCapDIB = CreateCompatibleDC(hdc);
			SelectObject(hdcCapDIB, hCapBMP);
		}
		pCrs->Release();
	}
}

IPin* EMU::get_pin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	BOOL bFound = FALSE;
	IEnumPins *pEnum;
	IPin *pPin;
	
	pFilter->EnumPins(&pEnum);
	while(pEnum->Next(1, &pPin, 0) == S_OK) {
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);
		if(bFound = (PinDir == PinDirThis))
			break;
		pPin->Release();
	}
	pEnum->Release();
	return (bFound ? pPin : 0);
}

bool EMU::get_capture_device_buffer()
{
	if(capture_connected != -1) {
		pSGrab->GetCurrentBuffer(&capture_bufsize, (long *)lpCapBMP);
		StretchBlt(hdcDIB, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcCapDIB, 0, capture_src_y, capture_width, capture_src_height, SRCCOPY);
		return true;
	}
	return false;
}

void EMU::set_capture_device_channel(int ch)
{
	IAMTVTuner *pTuner;
	HRESULT hr = pBuilder->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pSrc, IID_IAMTVTuner, (void **)&pTuner);
	if(hr == S_OK) {
		pTuner->put_Channel(ch, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
		pTuner->Release();
	}
}

