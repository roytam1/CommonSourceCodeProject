# Microsoft Developer Studio Project File - Name="mz2500" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=mz2500 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mz2500.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mz2500.mak" CFG="mz2500 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mz2500 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mz2500 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mz2500 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_MZ2500" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib dsound.lib imm32.lib wsock32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "mz2500 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_MZ2500" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib dsound.lib imm32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mz2500 - Win32 Release"
# Name "mz2500 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "EMU Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\src\emu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_input.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_media.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_screen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_socket.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_sound.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32_timer.cpp
# End Source File
# End Group
# Begin Group "VM Common Source Files"

# PROP Default_Filter "cpp"
# Begin Group "fmgen Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\src\vm\fmgen\file.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\fmgen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\fmtimer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\opna.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\psg.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\vm\disk.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\event.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\i8253.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\i8255.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\io8.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mb8877.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\rp5c15.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\w3100a.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\ym2203.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\z80.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\z80pio.cpp
# End Source File
# End Group
# Begin Group "VM Driver Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\src\vm\mz2500\calendar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\cassette.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\crtc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\emm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\extrom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\floppy.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\joystick.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\kanji.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\keyboard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\mz2500.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\romfile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\sasi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\z80pic.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\fileio.cpp
# End Source File
# Begin Source File

SOURCE=.\src\winmain.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "EMU Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\src\emu.h
# End Source File
# End Group
# Begin Group "VM Common Header Files"

# PROP Default_Filter "h"
# Begin Group "fmgen Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vm\fmgen\diag.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\file.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\fmgen.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\fmgeninl.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\fmtimer.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\headers.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\misc.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\opna.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\psg.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fmgen\types.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\vm\device.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\disk.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\event.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\fifo.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\i8253.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\i8255.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\io8.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mb8877.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\rp5c15.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\vm.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\w3100a.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\ym2203.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\z80.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\z80pio.h
# End Source File
# End Group
# Begin Group "VM Driver Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\src\vm\mz2500\calendar.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\cassette.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\crtc.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\emm.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\extrom.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\floppy.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\joystick.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\kanji.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\keyboard.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\memory.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\mz2500.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\romfile.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\sasi.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\timer.h
# End Source File
# Begin Source File

SOURCE=.\src\vm\mz2500\z80pic.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\common.h
# End Source File
# Begin Source File

SOURCE=.\src\config.h
# End Source File
# Begin Source File

SOURCE=.\src\fileio.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\src\res\MZ2500.ico
# End Source File
# Begin Source File

SOURCE=.\src\res\mz2500.rc
# End Source File
# Begin Source File

SOURCE=.\src\res\resource.h
# End Source File
# End Group
# End Target
# End Project
