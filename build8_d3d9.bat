echo off
set path=%path%;"C:\Program Files\Microsoft Visual Studio 8\Common7\IDE"
mkdir build

devenv.com hc40.vcproj /Rebuild Release_d3d9
mkdir build\hc40
copy Release_d3d9\hc40.exe build\hc40\.

devenv.com hc80.vcproj /Rebuild Release_d3d9
mkdir build\hc80
copy Release_d3d9\hc80.exe build\hc80\.

devenv.com m5.vcproj /Rebuild Release_d3d9
mkdir build\m5
copy Release_d3d9\m5.exe build\m5\.

devenv.com multi8.vcproj /Rebuild Release_d3d9
mkdir build\multi8
copy Release_d3d9\multi8.exe build\multi8\.

devenv.com mz2500.vcproj /Rebuild Release_d3d9
mkdir build\mz2500
copy Release_d3d9\mz2500.exe build\mz2500\.

devenv.com mz2800.vcproj /Rebuild Release_d3d9
mkdir build\mz2800
copy Release_d3d9\mz2800.exe build\mz2800\.

devenv.com mz5500.vcproj /Rebuild Release_d3d9
mkdir build\mz5500
copy Release_d3d9\mz5500.exe build\mz5500\.

devenv.com pasopia.vcproj /Rebuild Release_d3d9
mkdir build\pasopia_t
mkdir build\pasopia_oa
copy Release_d3d9\pasopia.exe build\pasopia_t\.
copy Release_d3d9\pasopia.exe build\pasopia_oa\.

devenv.com pasopia7.vcproj /Rebuild Release_d3d9
devenv.com pasopia7lcd.vcproj /Rebuild Release_d3d9
mkdir build\pasopia7
copy Release_d3d9\pasopia7.exe build\pasopia7\.
copy Release_d3d9\pasopia7lcd.exe build\pasopia7\.

devenv.com pv1000.vcproj /Rebuild Release_d3d9
mkdir build\pv1000
copy Release_d3d9\pv1000.exe build\pv1000\.

devenv.com pv2000.vcproj /Rebuild Release_d3d9
mkdir build\pv2000
copy Release_d3d9\pv2000.exe build\pv2000\.

devenv.com pyuta.vcproj /Rebuild Release_d3d9
mkdir build\pyuta
copy Release_d3d9\pyuta.exe build\pyuta\.

devenv.com qc10.vcproj /Rebuild Release_d3d9
devenv.com qc10cms.vcproj /Rebuild Release_d3d9
mkdir build\qc10
copy Release_d3d9\qc10.exe build\qc10\.
copy Release_d3d9\qc10cms.exe build\qc10\.

devenv.com rx78.vcproj /Rebuild Release_d3d9
mkdir build\rx78
copy Release_d3d9\rx78.exe build\rx78\.

devenv.com scv.vcproj /Rebuild Release_d3d9
mkdir build\scv
copy Release_d3d9\scv.exe build\scv\.

devenv.com x07.vcproj /Rebuild Release_d3d9
mkdir build\x07
copy Release_d3d9\x07.exe build\x07\.

pause
echo on
