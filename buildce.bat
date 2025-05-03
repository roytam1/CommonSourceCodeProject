echo off
set path=%path%;"C:\Program Files\Microsoft eMbedded C++ 4.0\Common\EVC\Bin"
mkdir build

evc m5ce.vcp /MAKE "m5ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc m5ce.vcp /MAKE "m5ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc m5ce.vcp /MAKE "m5ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc m5ce.vcp /MAKE "m5ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\m5
copy GDI_WAVEOUT\m5ce.exe build\m5\.

evc multi8ce.vcp /MAKE "multi8ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc multi8ce.vcp /MAKE "multi8ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc multi8ce.vcp /MAKE "multi8ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc multi8ce.vcp /MAKE "multi8ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\multi8
copy GDI_WAVEOUT\multi8ce.exe build\multi8\.

evc mz2500ce.vcp /MAKE "mz2500ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc mz2500ce.vcp /MAKE "mz2500ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc mz2500ce.vcp /MAKE "mz2500ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc mz2500ce.vcp /MAKE "mz2500ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz2500
copy GDI_WAVEOUT\mz2500ce.exe build\mz2500\.

evc pasopiace.vcp /MAKE "pasopiace - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopiace.vcp /MAKE "pasopiace - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopiace.vcp /MAKE "pasopiace - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopiace.vcp /MAKE "pasopiace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pasopia_oa
mkdir build\pasopia_t
copy GDI_WAVEOUT\pasopiace.exe build\pasopia_oa\.
copy GDI_WAVEOUT\pasopiace.exe build\pasopia_t\.

evc pasopia7ce.vcp /MAKE "pasopia7ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7ce.vcp /MAKE "pasopia7ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7ce.vcp /MAKE "pasopia7ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7ce.vcp /MAKE "pasopia7ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7lcdce.vcp /MAKE "pasopia7lcdce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7lcdce.vcp /MAKE "pasopia7lcdce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7lcdce.vcp /MAKE "pasopia7lcdce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7lcdce.vcp /MAKE "pasopia7lcdce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pasopia7
copy GDI_WAVEOUT\pasopia7ce.exe build\pasopia7\.
copy GDI_WAVEOUT\pasopia7lcdce.exe build\pasopia7\.

evc rx78ce.vcp /MAKE "rx78ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc rx78ce.vcp /MAKE "rx78ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc rx78ce.vcp /MAKE "rx78ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc rx78ce.vcp /MAKE "rx78ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\rx78
copy GDI_WAVEOUT\rx78ce.exe build\rx78\.

evc pv2000ce.vcp /MAKE "pv2000ce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pv2000ce.vcp /MAKE "pv2000ce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pv2000ce.vcp /MAKE "pv2000ce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc pv2000ce.vcp /MAKE "pv2000ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pv2000
copy GDI_WAVEOUT\pv2000ce.exe build\pv2000\.

evc scvce.vcp /MAKE "scvce - Win32 (WCE ARMV4I) GAPI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc scvce.vcp /MAKE "scvce - Win32 (WCE ARMV4I) GAPI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc scvce.vcp /MAKE "scvce - Win32 (WCE ARMV4I) GDI_DSOUND" /REBUILD /CECONFIG="STANDARDSDK"
evc scvce.vcp /MAKE "scvce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\scv
copy GDI_WAVEOUT\scvce.exe build\scv\.

pause
del *.vcl
echo on
