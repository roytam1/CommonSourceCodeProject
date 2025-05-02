echo off
rem set path=%path%;"C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE"
set path=%path%;"C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE"
mkdir build

devenv.com babbage2nd.vcproj /Rebuild Release
mkdir build\babbage2nd
copy Release\babbage2nd.exe build\babbage2nd\.

devenv.com familybasic.vcproj /Rebuild Release
mkdir build\familybasic_v2
copy Release\familybasic.exe build\familybasic_v2\.
mkdir build\familybasic_v3
copy Release\familybasic.exe build\familybasic_v3\.

devenv.com fm16pi.vcproj /Rebuild Release
mkdir build\fm16pi
copy Release\fm16pi.exe build\fm16pi\.

devenv.com fmr30.vcproj /Rebuild Release
mkdir build\fmr30
copy Release\fmr30.exe build\fmr30\.

devenv.com fmr50.vcproj /Rebuild Release
mkdir build\fmr50
copy Release\fmr50.exe build\fmr50\.

devenv.com fmr60.vcproj /Rebuild Release
mkdir build\fmr60
copy Release\fmr60.exe build\fmr60\.

devenv.com fp1100.vcproj /Rebuild Release
mkdir build\fp1100
copy Release\fp1100.exe build\fp1100\.

devenv.com hc40.vcproj /Rebuild Release
mkdir build\hc40
copy Release\hc40.exe build\hc40\.

devenv.com hc80.vcproj /Rebuild Release
mkdir build\hc80
copy Release\hc80.exe build\hc80\.

devenv.com m5.vcproj /Rebuild Release
mkdir build\m5
copy Release\m5.exe build\m5\.

devenv.com map1010.vcproj /Rebuild Release
mkdir build\map1010
copy Release\map1010.exe build\map1010\.

devenv.com multi8.vcproj /Rebuild Release
mkdir build\multi8
copy Release\multi8.exe build\multi8\.

devenv.com mycomz80a.vcproj /Rebuild Release
mkdir build\mycomz80a
copy Release\mycomz80a.exe build\mycomz80a\.

devenv.com mz80k.vcproj /Rebuild Release
mkdir build\mz80k
copy Release\mz80k.exe build\mz80k\.

devenv.com mz700.vcproj /Rebuild Release
mkdir build\mz700
copy Release\mz700.exe build\mz700\.

devenv.com mz1200.vcproj /Rebuild Release
mkdir build\mz1200
copy Release\mz1200.exe build\mz1200\.

devenv.com mz1500.vcproj /Rebuild Release
mkdir build\mz1500
copy Release\mz1500.exe build\mz1500\.

devenv.com mz2500.vcproj /Rebuild Release
mkdir build\mz2500
copy Release\mz2500.exe build\mz2500\.

devenv.com mz2800.vcproj /Rebuild Release
mkdir build\mz2800
copy Release\mz2800.exe build\mz2800\.

devenv.com mz3500.vcproj /Rebuild Release
mkdir build\mz3500
copy Release\mz3500.exe build\mz3500\.

devenv.com mz5500.vcproj /Rebuild Release
mkdir build\mz5500
copy Release\mz5500.exe build\mz5500\.

devenv.com mz6500.vcproj /Rebuild Release
mkdir build\mz6500
copy Release\mz6500.exe build\mz6500\.

devenv.com pasopia.vcproj /Rebuild Release
mkdir build\pasopia_t
copy Release\pasopia.exe build\pasopia_t\.
mkdir build\pasopia_oa
copy Release\pasopia.exe build\pasopia_oa\.

devenv.com pasopia7.vcproj /Rebuild Release
devenv.com pasopia7lcd.vcproj /Rebuild Release
mkdir build\pasopia7
copy Release\pasopia7.exe build\pasopia7\.
copy Release\pasopia7lcd.exe build\pasopia7\.

devenv.com pc9801.vcproj /Rebuild Release
mkdir build\pc9801
copy Release\pc9801.exe build\pc9801\.

devenv.com pc9801e.vcproj /Rebuild Release
mkdir build\pc9801e
copy Release\pc9801e.exe build\pc9801e\.

devenv.com pc98ha.vcproj /Rebuild Release
mkdir build\pc98ha
copy Release\pc98ha.exe build\pc98ha\.

devenv.com pc98lt.vcproj /Rebuild Release
mkdir build\pc98lt
copy Release\pc98lt.exe build\pc98lt\.

devenv.com pc100.vcproj /Rebuild Release
mkdir build\pc100
copy Release\pc100.exe build\pc100\.

devenv.com pc8201.vcproj /Rebuild Release
mkdir build\pc8201
copy Release\pc8201.exe build\pc8201\.

devenv.com pc8201a.vcproj /Rebuild Release
mkdir build\pc8201a
copy Release\pc8201a.exe build\pc8201a\.

devenv.com phc20.vcproj /Rebuild Release
mkdir build\phc20
copy Release\phc20.exe build\phc20\.

devenv.com phc25.vcproj /Rebuild Release
mkdir build\phc25
copy Release\phc25.exe build\phc25\.

devenv.com pv1000.vcproj /Rebuild Release
mkdir build\pv1000
copy Release\pv1000.exe build\pv1000\.

devenv.com pv2000.vcproj /Rebuild Release
mkdir build\pv2000
copy Release\pv2000.exe build\pv2000\.

devenv.com pyuta.vcproj /Rebuild Release
mkdir build\pyuta
mkdir build\pyuta_jr
copy Release\pyuta.exe build\pyuta\.
copy Release\pyuta.exe build\pyuta_jr\.

devenv.com qc10.vcproj /Rebuild Release
devenv.com qc10cms.vcproj /Rebuild Release
mkdir build\qc10
copy Release\qc10.exe build\qc10\.
copy Release\qc10cms.exe build\qc10\.

devenv.com rx78.vcproj /Rebuild Release
mkdir build\rx78
copy Release\rx78.exe build\rx78\.

devenv.com sc3000.vcproj /Rebuild Release
mkdir build\sc3000
copy Release\sc3000.exe build\sc3000\.

devenv.com scv.vcproj /Rebuild Release
mkdir build\scv
copy Release\scv.exe build\scv\.

devenv.com tk80bs.vcproj /Rebuild Release
mkdir build\tk80bs_lv1
copy Release\tk80bs.exe build\tk80bs_lv1\.
mkdir build\tk80bs_lv2
copy Release\tk80bs.exe build\tk80bs_lv2\.

devenv.com x07.vcproj /Rebuild Release
mkdir build\x07
copy Release\x07.exe build\x07\.

devenv.com x1twin.vcproj /Rebuild Release
mkdir build\x1twin
copy Release\x1twin.exe build\x1twin\.

devenv.com ys6464a.vcproj /Rebuild Release
mkdir build\ys6464a
copy Release\ys6464a.exe build\ys6464a\.

pause
echo on
