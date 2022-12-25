# Microsoft Developer Studio Project File - Name="RenderMgr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RenderMgr - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RenderMgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RenderMgr.mak" CFG="RenderMgr - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RenderMgr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RenderMgr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RenderMgr - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RenderMgr - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/EngComps/RenderMgr", PCGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RenderMgr - Win32 Release"

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
# ADD CPP /nologo /G6 /W3 /Zi /O2 /I "..\..\include" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "DA_HEAP_ENABLED" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "RenderMgr - Win32 Debug"

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
# ADD CPP /nologo /G6 /ML /W3 /Zi /Od /I "..\..\include" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "DA_HEAP_ENABLED" /D "WIN32" /D "_DEBUG" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "RenderMgr - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RenderMgr___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "RenderMgr___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /Zi /O2 /I "..\..\include" /D "NDEBUG" /D DA_ERROR_LEVEL=1 /D "WIN32" /D "_WINDOWS" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /O2 /I "..\..\include" /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\..\\" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 DACOM.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386

!ELSEIF  "$(CFG)" == "RenderMgr - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "RenderMgr___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "RenderMgr___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /ML /W3 /Zi /Od /I "..\..\include" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /I "..\..\include" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\..\\" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 DACOM.lib COMHeap.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# ADD LINK32 DACOM.lib MathLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:con

!ENDIF 

# Begin Target

# Name "RenderMgr - Win32 Release"
# Name "RenderMgr - Win32 Debug"
# Name "RenderMgr - Win32 MSHeap Release"
# Name "RenderMgr - Win32 MSHeap Debug"
# Begin Group "Source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\chull.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderMgr.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\chull.h
# End Source File
# Begin Source File

SOURCE=.\RenderArchetype.h
# End Source File
# End Group
# Begin Group "Shared Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Basecam.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\display.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\engcomp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\FDump.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\ICamera.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IDispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IDumpText.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IRenderComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\model.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\pblist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\polybatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Stddat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\SysConsumerDesc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TDispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TSmartPointer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Vfx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\xform.h
# End Source File
# End Group
# End Target
# End Project
