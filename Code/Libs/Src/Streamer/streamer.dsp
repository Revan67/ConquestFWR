# Microsoft Developer Studio Project File - Name="streamer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=streamer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "streamer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "streamer.mak" CFG="streamer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "streamer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "streamer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "streamer - Win32 Develop" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/streamer", TKUAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "streamer - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Zi /O2 /D DA_ERROR_LEVEL=2 /D "_WINDOWS" /D DIRECTSOUND_VERSION=0x0600 /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG" /d BUILD_LEVEL=\"Release\"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 winspool.lib oleaut32.lib DACOM.lib COMHeap.lib msacm32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib ole32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "streamer - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W3 /Zi /Od /Gf /I "." /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D DIRECTSOUND_VERSION=0x0600 /D "WIN32" /D "_DEBUG" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG" /d BUILD_LEVEL=\"Debug\"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib msacm32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib ole32.lib dxguid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "streamer - Win32 Develop"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "streamer"
# PROP BASE Intermediate_Dir "streamer"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Develop"
# PROP Intermediate_Dir "Develop"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=2 /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /W3 /O2 /D "NDEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /D DIRECTSOUND_VERSION=0x0600 /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG" /d BUILD_LEVEL=\"Develop\"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winspool.lib oleaut32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib ole32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386
# ADD LINK32 msacm32.lib winspool.lib oleaut32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib ole32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386

!ENDIF 

# Begin Target

# Name "streamer - Win32 Release"
# Name "streamer - Win32 Debug"
# Name "streamer - Win32 Develop"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Streamer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Eax.h
# End Source File
# End Group
# Begin Group "Shared Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\FDump.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Streamer.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\TSmartPointer.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Typedefs.h
# End Source File
# End Group
# End Target
# End Project
