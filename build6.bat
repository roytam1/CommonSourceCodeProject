echo off
mkdir build

msdev m5.dsp /MAKE "m5 - Win32 Release" /REBUILD
mkdir build\m5
copy release\m5.exe build\m5\.

msdev multi8.dsp /MAKE "multi8 - Win32 Release" /REBUILD
mkdir build\multi8
copy release\multi8.exe build\multi8\.

msdev mz2500.dsp /MAKE "mz2500 - Win32 Release" /REBUILD
mkdir build\mz2500
copy release\mz2500.exe build\mz2500\.

msdev pasopia.dsp /MAKE "pasopia - Win32 Release" /REBUILD
mkdir build\pasopia_t
mkdir build\pasopia_oa
copy release\pasopia.exe build\pasopia_t\.
copy release\pasopia.exe build\pasopia_oa\.

msdev pasopia7.dsp /MAKE "pasopia7 - Win32 Release" /REBUILD
msdev pasopia7lcd.dsp /MAKE "pasopia7lcd - Win32 Release" /REBUILD
mkdir build\pasopia7
copy release\pasopia7.exe build\pasopia7\.
copy release\pasopia7lcd.exe build\pasopia7\.

msdev pv1000.dsp /MAKE "pv1000 - Win32 Release" /REBUILD
mkdir build\pv1000
copy release\pv1000.exe build\pv1000\.

msdev pv2000.dsp /MAKE "pv2000 - Win32 Release" /REBUILD
mkdir build\pv2000
copy release\pv2000.exe build\pv2000\.

msdev rx78.dsp /MAKE "rx78 - Win32 Release" /REBUILD
mkdir build\rx78
copy release\rx78.exe build\rx78\.

msdev scv.dsp /MAKE "scv - Win32 Release" /REBUILD
mkdir build\scv
copy release\scv.exe build\scv\.

pause
del *.plg
echo on
