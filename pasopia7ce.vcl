<html>
<body>
<pre>
<h1>ビルドのログ</h1>
<h3>
--------------------構成 : pasopia7ce - Win32 (WCE ARMV4I) GDI_DSOUND--------------------
</h3>
<h3>コマンド ライン</h3>
Creating command line "rc.exe /l 0x411 /fo"GDI_DSOUND/pasopia7.res" /i "src\res" /d UNDER_CE=400 /d _WIN32_WCE=400 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WCE_PLATFORM_STANDARDSDK" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /d "ARMV4I" /r "D:\Develop\Common\source\source\src\res\pasopia7.rc"" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP187.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob2 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_PASOPIA7" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\emu.cpp"
"D:\Develop\Common\source\source\src\win32_input.cpp"
"D:\Develop\Common\source\source\src\win32_screen.cpp"
"D:\Develop\Common\source\source\src\win32_sound.cpp"
"D:\Develop\Common\source\source\src\win32_timer.cpp"
"D:\Develop\Common\source\source\src\vm\beep.cpp"
"D:\Develop\Common\source\source\src\vm\datarec.cpp"
"D:\Develop\Common\source\source\src\vm\disk.cpp"
"D:\Develop\Common\source\source\src\vm\event.cpp"
"D:\Develop\Common\source\source\src\vm\hd46505.cpp"
"D:\Develop\Common\source\source\src\vm\i8255.cpp"
"D:\Develop\Common\source\source\src\vm\sn76489an.cpp"
"D:\Develop\Common\source\source\src\vm\upd765a.cpp"
"D:\Develop\Common\source\source\src\vm\z80ctc.cpp"
"D:\Develop\Common\source\source\src\vm\z80pio.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\display.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\floppy.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\io8.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\iotrap.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\joypac2.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\kanjipac2.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\keyboard.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\memory.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\pac2.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\pasopia7.cpp"
"D:\Develop\Common\source\source\src\vm\pasopia7\rampac2.cpp"
"D:\Develop\Common\source\source\src\config.cpp"
"D:\Develop\Common\source\source\src\fileio.cpp"
"D:\Develop\Common\source\source\src\winmain.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP187.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP188.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob0 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_PASOPIA7" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\vm\z80.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP188.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP189.tmp" を作成し、次の内容を記録します
[
commctrl.lib coredll.lib WinCE\dsound.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"GDI_DSOUND/pasopia7ce.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"GDI_DSOUND/pasopia7ce.exe" /subsystem:windowsce,4.00 /MACHINE:THUMB 
.\GDI_DSOUND\emu.obj
.\GDI_DSOUND\win32_input.obj
.\GDI_DSOUND\win32_screen.obj
.\GDI_DSOUND\win32_sound.obj
.\GDI_DSOUND\win32_timer.obj
.\GDI_DSOUND\beep.obj
.\GDI_DSOUND\datarec.obj
.\GDI_DSOUND\disk.obj
.\GDI_DSOUND\event.obj
.\GDI_DSOUND\hd46505.obj
.\GDI_DSOUND\i8255.obj
.\GDI_DSOUND\sn76489an.obj
.\GDI_DSOUND\upd765a.obj
.\GDI_DSOUND\z80.obj
.\GDI_DSOUND\z80ctc.obj
.\GDI_DSOUND\z80pio.obj
.\GDI_DSOUND\display.obj
.\GDI_DSOUND\floppy.obj
.\GDI_DSOUND\io8.obj
.\GDI_DSOUND\iotrap.obj
.\GDI_DSOUND\joypac2.obj
.\GDI_DSOUND\kanjipac2.obj
.\GDI_DSOUND\keyboard.obj
.\GDI_DSOUND\memory.obj
.\GDI_DSOUND\pac2.obj
.\GDI_DSOUND\pasopia7.obj
.\GDI_DSOUND\rampac2.obj
.\GDI_DSOUND\config.obj
.\GDI_DSOUND\fileio.obj
.\GDI_DSOUND\winmain.obj
.\GDI_DSOUND\pasopia7.res
]
コマンド ライン "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP189.tmp" の作成中
<h3>アウトプット ウィンドウ</h3>
リソースをコンパイル中...
コンパイル中...
emu.cpp
win32_input.cpp
win32_screen.cpp
win32_sound.cpp
win32_timer.cpp
beep.cpp
datarec.cpp
disk.cpp
event.cpp
hd46505.cpp
i8255.cpp
sn76489an.cpp
upd765a.cpp
z80ctc.cpp
z80pio.cpp
display.cpp
floppy.cpp
io8.cpp
iotrap.cpp
joypac2.cpp
Generating Code...
Compiling...
kanjipac2.cpp
keyboard.cpp
memory.cpp
pac2.cpp
pasopia7.cpp
rampac2.cpp
config.cpp
fileio.cpp
winmain.cpp
Generating Code...
コンパイル中...
z80.cpp
リンク中...





<h3>結果</h3>
pasopia7ce.exe - エラー 0、警告 0
</pre>
</body>
</html>
