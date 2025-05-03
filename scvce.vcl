<html>
<body>
<pre>
<h1>ビルドのログ</h1>
<h3>
--------------------構成 : scvce - Win32 (WCE ARMV4I) GDI_DSOUND--------------------
</h3>
<h3>コマンド ライン</h3>
Creating command line "rc.exe /l 0x411 /fo"GDI_DSOUND/scv.res" /i "src\res" /d UNDER_CE=400 /d _WIN32_WCE=400 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WCE_PLATFORM_STANDARDSDK" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /d "ARMV4I" /r "D:\Develop\Common\source\source\src\res\scv.rc"" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1BD.tmp" を作成し、次の内容を記録します
[
/nologo /W3 /Oxt /Ob2 /I ".\WinCE" /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_SCV" /FR"GDI_DSOUND/" /Fo"GDI_DSOUND/" /QRarch4T /QRinterwork-return /MC /c 
"D:\Develop\Common\source\source\src\emu.cpp"
"D:\Develop\Common\source\source\src\win32_input.cpp"
"D:\Develop\Common\source\source\src\win32_screen.cpp"
"D:\Develop\Common\source\source\src\win32_sound.cpp"
"D:\Develop\Common\source\source\src\win32_timer.cpp"
"D:\Develop\Common\source\source\src\vm\event.cpp"
"D:\Develop\Common\source\source\src\vm\upd7801.cpp"
"D:\Develop\Common\source\source\src\vm\scv\io.cpp"
"D:\Develop\Common\source\source\src\vm\scv\memory.cpp"
"D:\Develop\Common\source\source\src\vm\scv\scv.cpp"
"D:\Develop\Common\source\source\src\vm\scv\sound.cpp"
"D:\Develop\Common\source\source\src\vm\scv\vdp.cpp"
"D:\Develop\Common\source\source\src\config.cpp"
"D:\Develop\Common\source\source\src\fileio.cpp"
"D:\Develop\Common\source\source\src\winmain.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1BD.tmp" 
一時ファイル "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1BE.tmp" を作成し、次の内容を記録します
[
commctrl.lib coredll.lib WinCE\dsound.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"GDI_DSOUND/scvce.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"GDI_DSOUND/scvce.exe" /subsystem:windowsce,4.00 /MACHINE:THUMB 
.\GDI_DSOUND\emu.obj
.\GDI_DSOUND\win32_input.obj
.\GDI_DSOUND\win32_screen.obj
.\GDI_DSOUND\win32_sound.obj
.\GDI_DSOUND\win32_timer.obj
.\GDI_DSOUND\event.obj
.\GDI_DSOUND\upd7801.obj
.\GDI_DSOUND\io.obj
.\GDI_DSOUND\memory.obj
.\GDI_DSOUND\scv.obj
.\GDI_DSOUND\sound.obj
.\GDI_DSOUND\vdp.obj
.\GDI_DSOUND\config.obj
.\GDI_DSOUND\fileio.obj
.\GDI_DSOUND\winmain.obj
.\GDI_DSOUND\scv.res
]
コマンド ライン "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1BE.tmp" の作成中
<h3>アウトプット ウィンドウ</h3>
リソースをコンパイル中...
コンパイル中...
emu.cpp
win32_input.cpp
win32_screen.cpp
win32_sound.cpp
win32_timer.cpp
event.cpp
upd7801.cpp
io.cpp
memory.cpp
scv.cpp
sound.cpp
vdp.cpp
config.cpp
fileio.cpp
winmain.cpp
Generating Code...
リンク中...
upd7801.obj : warning LNK1166: cannot adjust code at offset=0x00001000, rva=0x00003A24





<h3>結果</h3>
scvce.exe - エラー 0、警告 1
</pre>
</body>
</html>
