# Microsoft Developer Studio Project File - Name="proto" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=proto - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "x86math.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "x86math.mak" CFG="proto - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "proto - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "proto - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "proto - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "proto - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/x86math", VPCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "proto - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /W3 /Zi /O2 /I "." /I "../include" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "BUILD_MATHLIB" /D "DA_HEAP_ENABLED" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "proto - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /ML /W3 /Zi /Od /I "." /I "../include" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "BUILD_MATHLIB" /D "DA_HEAP_ENABLED" /D "WIN32" /D "_DEBUG" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "proto - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "proto___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "proto___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Zp4 /ML /W3 /Zi /Od /I "." /I "../include" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /D "BUILD_MATHLIB" /D "DA_HEAP_ENABLED" /FR /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /I "." /I "../include" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "BUILD_MATHLIB" /D "WIN32" /D "_DEBUG" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT BASE LINK32 /map
# ADD LINK32 dacom.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "proto - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "proto___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "proto___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Zp4 /W3 /Zi /O2 /I "." /I "../include" /D "NDEBUG" /D DA_ERROR_LEVEL=1 /D "WIN32" /D "_WINDOWS" /D "BUILD_MATHLIB" /D "DA_HEAP_ENABLED" /FAs /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /O2 /I "." /I "../include" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "BUILD_MATHLIB" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FAs /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\\" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 dacom.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes /map

!ENDIF 

# Begin Target

# Name "proto - Win32 Release"
# Name "proto - Win32 Debug"
# Name "proto - Win32 MSHeap Debug"
# Name "proto - Win32 MSHeap Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\x86math.cpp
# End Source File
# End Group
# Begin Group "Shared Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\xform.h
# End Source File
# End Group
# End Target
# End Project
