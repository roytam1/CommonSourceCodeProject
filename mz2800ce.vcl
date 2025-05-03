<html>
<body>
<pre>
<h1>ビルドのログ</h1>
<h3>
--------------------構成 : mz2800ce - Win32 (WCE ARMV4I) GDI_DSOUND--------------------
</h3>
<h3>コマンド ライン</h3>
Creating command line "rc.exe /l 0x411 /fo"GDI_DSOUND/mz2800.res" /i "src\res" /d UNDER_CE=400 /d _WIN32_WCE=400 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WCE_PLATFORM_STANDARDSDK" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /d "ARMV4I" /r "D:\Develop\Common\source\source\src\res\mz2800.rc"" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP16F.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob2 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_MZ2800" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\emu.cpp"
"D:\Develop\Common\source\source\src\win32_input.cpp"
"D:\Develop\Common\source\source\src\win32_screen.cpp"
"D:\Develop\Common\source\source\src\win32_sound.cpp"
"D:\Develop\Common\source\source\src\win32_timer.cpp"
"D:\Develop\Common\source\source\src\vm\fmgen\file.cpp"
"D:\Develop\Common\source\source\src\vm\fmgen\fmgen.cpp"
"D:\Develop\Common\source\source\src\vm\fmgen\fmtimer.cpp"
"D:\Develop\Common\source\source\src\vm\fmgen\opna.cpp"
"D:\Develop\Common\source\source\src\vm\fmgen\psg.cpp"
"D:\Develop\Common\source\source\src\vm\disk.cpp"
"D:\Develop\Common\source\source\src\vm\event.cpp"
"D:\Develop\Common\source\source\src\vm\i8253.cpp"
"D:\Develop\Common\source\source\src\vm\i8255.cpp"
"D:\Develop\Common\source\source\src\vm\i8259.cpp"
"D:\Develop\Common\source\source\src\vm\io8.cpp"
"D:\Develop\Common\source\source\src\vm\mb8877.cpp"
"D:\Develop\Common\source\source\src\vm\pcm1bit.cpp"
"D:\Develop\Common\source\source\src\vm\rp5c15.cpp"
"D:\Develop\Common\source\source\src\vm\upd71071.cpp"
"D:\Develop\Common\source\source\src\vm\ym2203.cpp"
"D:\Develop\Common\source\source\src\vm\z80pio.cpp"
"D:\Develop\Common\source\source\src\vm\z80sio.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\calendar.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\crtc.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\floppy.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\joystick.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\keyboard.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\memory.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\mouse.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\mz2800.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\reset.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\sysport.cpp"
"D:\Develop\Common\source\source\src\vm\mz2800\timer.cpp"
"D:\Develop\Common\source\source\src\config.cpp"
"D:\Develop\Common\source\source\src\fileio.cpp"
"D:\Develop\Common\source\source\src\winmain.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP16F.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP170.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob0 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_MZ2800" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\vm\x86.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP170.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP171.tmp" を作成し、次の内容を記録します
[
commctrl.lib coredll.lib WinCE\dsound.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"GDI_DSOUND/mz2800ce.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"GDI_DSOUND/mz2800ce.exe" /subsystem:windowsce,4.00 /MACHINE:THUMB 
.\GDI_DSOUND\emu.obj
.\GDI_DSOUND\win32_input.obj
.\GDI_DSOUND\win32_screen.obj
.\GDI_DSOUND\win32_sound.obj
.\GDI_DSOUND\win32_timer.obj
.\GDI_DSOUND\file.obj
.\GDI_DSOUND\fmgen.obj
.\GDI_DSOUND\fmtimer.obj
.\GDI_DSOUND\opna.obj
.\GDI_DSOUND\psg.obj
.\GDI_DSOUND\disk.obj
.\GDI_DSOUND\event.obj
.\GDI_DSOUND\i8253.obj
.\GDI_DSOUND\i8255.obj
.\GDI_DSOUND\i8259.obj
.\GDI_DSOUND\io8.obj
.\GDI_DSOUND\mb8877.obj
.\GDI_DSOUND\pcm1bit.obj
.\GDI_DSOUND\rp5c15.obj
.\GDI_DSOUND\upd71071.obj
.\GDI_DSOUND\x86.obj
.\GDI_DSOUND\ym2203.obj
.\GDI_DSOUND\z80pio.obj
.\GDI_DSOUND\z80sio.obj
.\GDI_DSOUND\calendar.obj
.\GDI_DSOUND\crtc.obj
.\GDI_DSOUND\floppy.obj
.\GDI_DSOUND\joystick.obj
.\GDI_DSOUND\keyboard.obj
.\GDI_DSOUND\memory.obj
.\GDI_DSOUND\mouse.obj
.\GDI_DSOUND\mz2800.obj
.\GDI_DSOUND\reset.obj
.\GDI_DSOUND\sysport.obj
.\GDI_DSOUND\timer.obj
.\GDI_DSOUND\config.obj
.\GDI_DSOUND\fileio.obj
.\GDI_DSOUND\winmain.obj
.\GDI_DSOUND\mz2800.res
]
コマンド ライン "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP171.tmp" の作成中
<h3>アウトプット ウィンドウ</h3>
リソースをコンパイル中...
コンパイル中...
emu.cpp
win32_input.cpp
win32_screen.cpp
win32_sound.cpp
win32_timer.cpp
file.cpp
fmgen.cpp
fmtimer.cpp
opna.cpp
psg.cpp
disk.cpp
event.cpp
i8253.cpp
i8255.cpp
i8259.cpp
io8.cpp
mb8877.cpp
pcm1bit.cpp
rp5c15.cpp
upd71071.cpp
Generating Code...
Compiling...
ym2203.cpp
z80pio.cpp
z80sio.cpp
calendar.cpp
crtc.cpp
floppy.cpp
joystick.cpp
keyboard.cpp
memory.cpp
mouse.cpp
mz2800.cpp
reset.cpp
sysport.cpp
timer.cpp
config.cpp
fileio.cpp
winmain.cpp
Generating Code...
コンパイル中...
x86.cpp
リンク中...





<h3>結果</h3>
mz2800ce.exe - エラー 0、警告 0
</pre>
</body>
</html>
