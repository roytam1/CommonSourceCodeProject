echo off
set path=%path%;"C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE"
mkdir build

devenv.com fmr30.vcproj /Rebuild Release_d3d9
mkdir build\fmr30
copy Release_d3d9\fmr30.exe build\fmr30\.

devenv.com fmr50.vcproj /Rebuild Release_d3d9
mkdir build\fmr50
copy Release_d3d9\fmr50.exe build\fmr50\.

devenv.com fmr60.vcproj /Rebuild Release_d3d9
mkdir build\fmr60
copy Release_d3d9\fmr60.exe build\fmr60\.

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

devenv.com mz700.vcproj /Rebuild Release_d3d9
mkdir build\mz700
copy Release_d3d9\mz700.exe build\mz700\.

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

devenv.com pc98ha.vcproj /Rebuild Release_d3d9
mkdir build\pc98ha
copy Release_d3d9\pc98ha.exe build\pc98ha\.

devenv.com pc98lt.vcproj /Rebuild Release_d3d9
mkdir build\pc98lt
copy Release_d3d9\pc98lt.exe build\pc98lt\.

devenv.com pc100.vcproj /Rebuild Release_d3d9
mkdir build\pc100
copy Release_d3d9\pc100.exe build\pc100\.

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

devenv.com tk80bs.vcproj /Rebuild Release_d3d9
mkdir build\tk80bs_lv1
mkdir build\tk80bs_lv2
copy Release_d3d9\tk80bs.exe build\tk80bs_lv1\.
copy Release_d3d9\tk80bs.exe build\tk80bs_lv2\.

devenv.com x07.vcproj /Rebuild Release_d3d9
mkdir build\x07
copy Release_d3d9\x07.exe build\x07\.

devenv.com x1twin.vcproj /Rebuild Release_d3d9
mkdir build\x1twin
copy Release_d3d9\x1twin.exe build\x1twin\.

pause
echo on
