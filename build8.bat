echo off
set path=%path%;"C:\Program Files\Microsoft Visual Studio 8\Common7\IDE"
mkdir build

devenv.com m5.vcproj /Rebuild Release
mkdir build\m5
copy release\m5.exe build\m5\.

devenv.com multi8.vcproj /Rebuild Release
mkdir build\multi8
copy release\multi8.exe build\multi8\.

devenv.com mz2500.vcproj /Rebuild Release
mkdir build\mz2500
copy release\mz2500.exe build\mz2500\.

devenv.com pasopia.vcproj /Rebuild Release
mkdir build\pasopia_t
mkdir build\pasopia_oa
copy release\pasopia.exe build\pasopia_t\.
copy release\pasopia.exe build\pasopia_oa\.

devenv.com pasopia7.vcproj /Rebuild Release
devenv.com pasopia7lcd.vcproj /Rebuild Release
mkdir build\pasopia7
copy release\pasopia7.exe build\pasopia7\.
copy release\pasopia7lcd.exe build\pasopia7\.

devenv.com pv1000.vcproj /Rebuild Release
mkdir build\pv1000
copy release\pv1000.exe build\pv1000\.

devenv.com pv2000.vcproj /Rebuild Release
mkdir build\pv2000
copy release\pv2000.exe build\pv2000\.

devenv.com rx78.vcproj /Rebuild Release
mkdir build\rx78
copy release\rx78.exe build\rx78\.

devenv.com scv.vcproj /Rebuild Release
mkdir build\scv
copy release\scv.exe build\scv\.

pause
echo on
