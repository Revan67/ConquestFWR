# Microsoft Developer Studio Project File - Name="audio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=audio - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "audio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "audio.mak" CFG="audio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "audio - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "audio - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/Audio", MQJAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "audio - Win32 Release"

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
# ADD CPP /nologo /G6 /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 mss32.lib dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "audio - Win32 Debug"

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
# ADD CPP /nologo /G6 /ML /W3 /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mss32.lib dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept /libpath:"c:\libs\static"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "audio - Win32 Release"
# Name "audio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp *.c"
# Begin Source File

SOURCE=.\Audio.rc
# End Source File
# Begin Source File

SOURCE=.\audiofile.cpp
# End Source File
# Begin Source File

SOURCE=.\audiomgr.CPP
# End Source File
# Begin Source File

SOURCE=.\audiosample.cpp
# End Source File
# Begin Source File

SOURCE=.\Waudmgrsfil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\audiofile.h
# End Source File
# Begin Source File

SOURCE=.\audiosample.h
# End Source File
# Begin Source File

SOURCE=.\audiover.h
# End Source File
# Begin Source File

SOURCE=.\Mssw.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Waudmgrsfil.h
# End Source File
# End Group
# Begin Group "Shared Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Include\audiomgr.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IDumpText.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Stddat.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\System.h
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
# Begin Source File

SOURCE=..\..\Include\vector.h
# End Source File
# End Group
# End Target
# End Project
