<html>
<body>
<pre>
<h1>ビルドのログ</h1>
<h3>
--------------------構成 : qc10cmsce - Win32 (WCE ARMV4I) GDI_DSOUND--------------------
</h3>
<h3>コマンド ライン</h3>
Creating command line "rc.exe /l 0x411 /fo"GDI_DSOUND/qc10.res" /i "src\res" /d UNDER_CE=400 /d _WIN32_WCE=400 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WCE_PLATFORM_STANDARDSDK" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /d "ARMV4I" /r "D:\Develop\Common\source\source\src\res\qc10.rc"" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B7.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob2 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_QC10" /D "_COLOR_MONITOR" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\emu.cpp"
"D:\Develop\Common\source\source\src\win32_input.cpp"
"D:\Develop\Common\source\source\src\win32_screen.cpp"
"D:\Develop\Common\source\source\src\win32_sound.cpp"
"D:\Develop\Common\source\source\src\win32_timer.cpp"
"D:\Develop\Common\source\source\src\vm\beep.cpp"
"D:\Develop\Common\source\source\src\vm\disk.cpp"
"D:\Develop\Common\source\source\src\vm\event.cpp"
"D:\Develop\Common\source\source\src\vm\hd146818p.cpp"
"D:\Develop\Common\source\source\src\vm\i8237.cpp"
"D:\Develop\Common\source\source\src\vm\i8253.cpp"
"D:\Develop\Common\source\source\src\vm\i8255.cpp"
"D:\Develop\Common\source\source\src\vm\i8259.cpp"
"D:\Develop\Common\source\source\src\vm\io8.cpp"
"D:\Develop\Common\source\source\src\vm\upd7220.cpp"
"D:\Develop\Common\source\source\src\vm\upd765a.cpp"
"D:\Develop\Common\source\source\src\vm\z80sio.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\display.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\floppy.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\keyboard.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\memory.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\mfont.cpp"
"D:\Develop\Common\source\source\src\vm\qc10\qc10.cpp"
"D:\Develop\Common\source\source\src\config.cpp"
"D:\Develop\Common\source\source\src\fileio.cpp"
"D:\Develop\Common\source\source\src\winmain.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B7.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B8.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob0 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_QC10" /D "_COLOR_MONITOR" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\vm\z80.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B8.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B9.tmp" を作成し、次の内容を記録します
[
commctrl.lib coredll.lib WinCE\dsound.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"GDI_DSOUND/qc10cmsce.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"GDI_DSOUND/qc10cmsce.exe" /subsystem:windowsce,4.00 /MACHINE:THUMB 
.\GDI_DSOUND\emu.obj
.\GDI_DSOUND\win32_input.obj
.\GDI_DSOUND\win32_screen.obj
.\GDI_DSOUND\win32_sound.obj
.\GDI_DSOUND\win32_timer.obj
.\GDI_DSOUND\beep.obj
.\GDI_DSOUND\disk.obj
.\GDI_DSOUND\event.obj
.\GDI_DSOUND\hd146818p.obj
.\GDI_DSOUND\i8237.obj
.\GDI_DSOUND\i8253.obj
.\GDI_DSOUND\i8255.obj
.\GDI_DSOUND\i8259.obj
.\GDI_DSOUND\io8.obj
.\GDI_DSOUND\upd7220.obj
.\GDI_DSOUND\upd765a.obj
.\GDI_DSOUND\z80.obj
.\GDI_DSOUND\z80sio.obj
.\GDI_DSOUND\display.obj
.\GDI_DSOUND\floppy.obj
.\GDI_DSOUND\keyboard.obj
.\GDI_DSOUND\memory.obj
.\GDI_DSOUND\mfont.obj
.\GDI_DSOUND\qc10.obj
.\GDI_DSOUND\config.obj
.\GDI_DSOUND\fileio.obj
.\GDI_DSOUND\winmain.obj
.\GDI_DSOUND\qc10.res
]
コマンド ライン "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1B9.tmp" の作成中
<h3>アウトプット ウィンドウ</h3>
リソースをコンパイル中...
コンパイル中...
emu.cpp
win32_input.cpp
win32_screen.cpp
win32_sound.cpp
win32_timer.cpp
beep.cpp
disk.cpp
event.cpp
hd146818p.cpp
i8237.cpp
i8253.cpp
i8255.cpp
i8259.cpp
io8.cpp
upd7220.cpp
upd765a.cpp
z80sio.cpp
display.cpp
floppy.cpp
keyboard.cpp
Generating Code...
Compiling...
memory.cpp
mfont.cpp
qc10.cpp
config.cpp
fileio.cpp
winmain.cpp
Generating Code...
コンパイル中...
z80.cpp
リンク中...





<h3>結果</h3>
qc10cmsce.exe - エラー 0、警告 0
</pre>
</body>
</html>
