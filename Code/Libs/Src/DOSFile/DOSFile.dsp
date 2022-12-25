# Microsoft Developer Studio Project File - Name="DOSFile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DOSFile - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DOSFile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DOSFile.mak" CFG="DOSFile - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DOSFile - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DOSFile - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DOSFile - Win32 Release Thread Safe" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DOSFile - Win32 Debug Thread Safe" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DOSFile - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DOSFile - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/DOSFile", GBCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DOSFile_"
# PROP BASE Intermediate_Dir "DOSFile_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /W3 /Zi /O2 /D "DA_HEAP_ENABLED" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DOSFile0"
# PROP BASE Intermediate_Dir "DOSFile0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /ML /W3 /Zi /Od /Gf /D "DA_HEAP_ENABLED" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DOSFile___Win32_Release_Thread_Safe"
# PROP BASE Intermediate_Dir "DOSFile___Win32_Release_Thread_Safe"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMT"
# PROP Intermediate_Dir "ReleaseMT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=1 /FAs /YX /FD /c
# ADD CPP /nologo /G5 /MD /W3 /O2 /D "DA_MULTI_THREADED" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386
# ADD LINK32 DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /out:"ReleaseMT/DOSFileMT.dll" /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DOSFile___Win32_Debug_Thread_Safe"
# PROP BASE Intermediate_Dir "DOSFile___Win32_Debug_Thread_Safe"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugMT"
# PROP Intermediate_Dir "DebugMT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /Zi /Od /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=8 /FR /YX /FD /c
# ADD CPP /nologo /G5 /MDd /W3 /Zi /Od /Gf /D "DA_MULTI_THREADED" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /debug /machine:I386
# ADD LINK32 DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /out:"DebugMT/DOSFileMT.dll" /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DOSFile___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "DOSFile___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /O2 /D "DA_HEAP_ENABLED" /D "NDEBUG" /D DA_ERROR_LEVEL=1 /D "WIN32" /D "_WINDOWS" /FAs /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /O2 /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "DA_MULTI_THREADED" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386
# ADD LINK32 DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DOSFile___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "DOSFile___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /Zi /Od /Gf /D "DA_HEAP_ENABLED" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /Gf /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "DA_MULTI_THREADED" /D "WIN32" /D "_DEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /debug /machine:I386
# ADD LINK32 DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:con

!ENDIF 

# Begin Target

# Name "DOSFile - Win32 Release"
# Name "DOSFile - Win32 Debug"
# Name "DOSFile - Win32 Release Thread Safe"
# Name "DOSFile - Win32 Debug Thread Safe"
# Name "DOSFile - Win32 MSHeap Release"
# Name "DOSFile - Win32 MSHeap Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp; c"
# Begin Source File

SOURCE=.\BaseUTF.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Dosfile.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\IFF.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MemFile.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SearchPath.cpp
# End Source File
# Begin Source File

SOURCE=.\UTF.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UTFDMan.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UTFShare.cpp

!IF  "$(CFG)" == "DOSFile - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Release Thread Safe"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 Debug Thread Safe"

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Release"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DOSFile - Win32 MSHeap Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\BaseUTF.h
# End Source File
# Begin Source File

SOURCE=.\DosFileVer.h
# End Source File
# Begin Source File

SOURCE=.\UTFDMan.h
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

SOURCE=..\..\Include\IUTFWriter.h
# End Source File
# Begin Source File

SOURCE=..\libver.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\MemFile.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\SearchPath.h
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
