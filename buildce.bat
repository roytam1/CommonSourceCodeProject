echo off
set path=%path%;"C:\Program Files\Microsoft eMbedded C++ 4.0\Common\EVC\Bin"
mkdir build

evc fmr30ce.vcp /MAKE "fmr30ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\fmr30
copy GDI_WAVEOUT\fmr30ce.exe build\fmr30\.

evc fmr50ce.vcp /MAKE "fmr50ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\fmr50
copy GDI_WAVEOUT\fmr50ce.exe build\fmr50\.

evc fmr60ce.vcp /MAKE "fmr60ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\fmr60
copy GDI_WAVEOUT\fmr60ce.exe build\fmr60\.

evc fmrcardce.vcp /MAKE "fmrcardce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\fmrcard
copy GDI_WAVEOUT\fmrcardce.exe build\fmrcard\.

evc hc40ce.vcp /MAKE "hc40ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\hc40
copy GDI_WAVEOUT\hc40ce.exe build\hc40\.

evc hc80ce.vcp /MAKE "hc80ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\hc80
copy GDI_WAVEOUT\hc80ce.exe build\hc80\.

evc m5ce.vcp /MAKE "m5ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\m5
copy GDI_WAVEOUT\m5ce.exe build\m5\.

evc multi8ce.vcp /MAKE "multi8ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\multi8
copy GDI_WAVEOUT\multi8ce.exe build\multi8\.

evc mycomz80ace.vcp /MAKE "mycomz80ace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mycomz80a
copy GDI_WAVEOUT\mycomz80ace.exe build\mycomz80a\.

evc mz700ce.vcp /MAKE "mz700ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz700
copy GDI_WAVEOUT\mz700ce.exe build\mz700\.

evc mz2500ce.vcp /MAKE "mz2500ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz2500
copy GDI_WAVEOUT\mz2500ce.exe build\mz2500\.

evc mz2800ce.vcp /MAKE "mz2800ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz2800
copy GDI_WAVEOUT\mz2800ce.exe build\mz2800\.

evc mz5500ce.vcp /MAKE "mz5500ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz5500
copy GDI_WAVEOUT\mz5500ce.exe build\mz5500\.

evc mz6500ce.vcp /MAKE "mz6500ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\mz6500
copy GDI_WAVEOUT\mz6500ce.exe build\mz6500\.

evc pasopiace.vcp /MAKE "pasopiace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pasopia_oa
mkdir build\pasopia_t
copy GDI_WAVEOUT\pasopiace.exe build\pasopia_oa\.
copy GDI_WAVEOUT\pasopiace.exe build\pasopia_t\.

evc pasopia7ce.vcp /MAKE "pasopia7ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc pasopia7lcdce.vcp /MAKE "pasopia7lcdce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pasopia7
copy GDI_WAVEOUT\pasopia7ce.exe build\pasopia7\.
copy GDI_WAVEOUT\pasopia7lcdce.exe build\pasopia7\.

evc rx78ce.vcp /MAKE "rx78ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\rx78
copy GDI_WAVEOUT\rx78ce.exe build\rx78\.

evc pc98hace.vcp /MAKE "pc98hace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pc98ha
copy GDI_WAVEOUT\pc98hace.exe build\pc98ha\.

evc pc98ltce.vcp /MAKE "pc98ltce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pc98lt
copy GDI_WAVEOUT\pc98ltce.exe build\pc98lt\.

evc pc8201ce.vcp /MAKE "pc8201ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pc8201
copy GDI_WAVEOUT\pc8201ce.exe build\pc8201\.

evc pc8201ace.vcp /MAKE "pc8201ace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pc8201a
copy GDI_WAVEOUT\pc8201ace.exe build\pc8201a\.

evc pv2000ce.vcp /MAKE "pv2000ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pv2000
copy GDI_WAVEOUT\pv2000ce.exe build\pv2000\.

evc pyutace.vcp /MAKE "pyutace - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\pyuta
copy GDI_WAVEOUT\pyutace.exe build\pyuta\.

evc qc10ce.vcp /MAKE "qc10ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
evc qc10cmsce.vcp /MAKE "qc10cmsce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\qc10
copy GDI_WAVEOUT\qc10ce.exe build\qc10\.
copy GDI_WAVEOUT\qc10cmsce.exe build\qc10\.

evc scvce.vcp /MAKE "scvce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\scv
copy GDI_WAVEOUT\scvce.exe build\scv\.

evc tk80bsce.vcp /MAKE "tk80bsce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\tk80bs_lv1
mkdir build\tk80bs_lv2
copy GDI_WAVEOUT\tk80bsce.exe build\tk80bs_lv1\.
copy GDI_WAVEOUT\tk80bsce.exe build\tk80bs_lv2\.

evc x07ce.vcp /MAKE "x07ce - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\x07
copy GDI_WAVEOUT\x07ce.exe build\x07\.

evc x1twince.vcp /MAKE "x1twince - Win32 (WCE ARMV4I) GDI_WAVEOUT" /REBUILD /CECONFIG="STANDARDSDK"
mkdir build\x1twin
copy GDI_WAVEOUT\x1twince.exe build\x1twin\.

pause
del *.vcl
echo on
