<html>
<body>
<pre>
<h1>ビルドのログ</h1>
<h3>
--------------------構成 : multi8ce - Win32 (WCE ARMV4I) GDI_DSOUND--------------------
</h3>
<h3>コマンド ライン</h3>
Creating command line "rc.exe /l 0x411 /fo"GDI_DSOUND/multi8.res" /i "src\res" /d UNDER_CE=400 /d _WIN32_WCE=400 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WCE_PLATFORM_STANDARDSDK" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /d "ARMV4I" /r "D:\Develop\Common\source\source\src\res\multi8.rc"" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15D.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob2 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_MULTI8" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
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
"D:\Develop\Common\source\source\src\vm\hd46505.cpp"
"D:\Develop\Common\source\source\src\vm\i8251.cpp"
"D:\Develop\Common\source\source\src\vm\i8253.cpp"
"D:\Develop\Common\source\source\src\vm\i8255.cpp"
"D:\Develop\Common\source\source\src\vm\i8259.cpp"
"D:\Develop\Common\source\source\src\vm\io8.cpp"
"D:\Develop\Common\source\source\src\vm\upd765a.cpp"
"D:\Develop\Common\source\source\src\vm\ym2203.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\cmt.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\display.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\floppy.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\kanji.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\keyboard.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\memory.cpp"
"D:\Develop\Common\source\source\src\vm\multi8\multi8.cpp"
"D:\Develop\Common\source\source\src\config.cpp"
"D:\Develop\Common\source\source\src\fileio.cpp"
"D:\Develop\Common\source\source\src\winmain.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15D.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15E.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob0 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_MULTI8" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\vm\z80.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15E.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15F.tmp" を作成し、次の内容を記録します
[
commctrl.lib coredll.lib WinCE\dsound.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"GDI_DSOUND/multi8ce.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"GDI_DSOUND/multi8ce.exe" /subsystem:windowsce,4.00 /MACHINE:THUMB 
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
.\GDI_DSOUND\hd46505.obj
.\GDI_DSOUND\i8251.obj
.\GDI_DSOUND\i8253.obj
.\GDI_DSOUND\i8255.obj
.\GDI_DSOUND\i8259.obj
.\GDI_DSOUND\io8.obj
.\GDI_DSOUND\upd765a.obj
.\GDI_DSOUND\ym2203.obj
.\GDI_DSOUND\z80.obj
.\GDI_DSOUND\cmt.obj
.\GDI_DSOUND\display.obj
.\GDI_DSOUND\floppy.obj
.\GDI_DSOUND\kanji.obj
.\GDI_DSOUND\keyboard.obj
.\GDI_DSOUND\memory.obj
.\GDI_DSOUND\multi8.obj
.\GDI_DSOUND\config.obj
.\GDI_DSOUND\fileio.obj
.\GDI_DSOUND\winmain.obj
.\GDI_DSOUND\multi8.res
]
コマンド ライン "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP15F.tmp" の作成中
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
hd46505.cpp
i8251.cpp
i8253.cpp
i8255.cpp
i8259.cpp
io8.cpp
upd765a.cpp
ym2203.cpp
Generating Code...
Compiling...
cmt.cpp
display.cpp
floppy.cpp
kanji.cpp
keyboard.cpp
memory.cpp
multi8.cpp
config.cpp
fileio.cpp
winmain.cpp
Generating Code...
コンパイル中...
z80.cpp
リンク中...





<h3>結果</h3>
multi8ce.exe - エラー 0、警告 0
</pre>
</body>
</html>
