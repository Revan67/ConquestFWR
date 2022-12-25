# Microsoft Developer Studio Project File - Name="VertexBufferManager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=VertexBufferManager - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VertexBufferManager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VertexBufferManager.mak" CFG="VertexBufferManager - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VertexBufferManager - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VertexBufferManager - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VertexBufferManager - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VertexBufferManager - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/RenderPipeline", URVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VertexBufferManager - Win32 Release"

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
# ADD CPP /nologo /G6 /W4 /Zi /O2 /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D "DA_HEAP_ENABLED" /D DA_ERROR_LEVEL=3 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comheap.lib winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "VertexBufferManager - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "newRende"
# PROP BASE Intermediate_Dir "newRende"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W3 /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D "DA_HEAP_ENABLED" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "_DEBUG" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 comheap.lib winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /nodefaultlib:"libcmtd" /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "VertexBufferManager - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "VertexBufferManager___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "VertexBufferManager___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Zi /O2 /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D "DA_HEAP_ENABLED" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /D "_WINDOWS" /D "WIN32" /D "D3D_OVERLOADS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MD /W4 /Zi /O2 /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D DA_ERROR_LEVEL=3 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "NDEBUG" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comheap.lib winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /map

!ELSEIF  "$(CFG)" == "VertexBufferManager - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "VertexBufferManager___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "VertexBufferManager___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /ML /W3 /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D "DA_HEAP_ENABLED" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "WIN32" /D "D3D_OVERLOADS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /I "..\..\include" /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "_DEBUG" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comheap.lib winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /nodefaultlib:"libcmtd"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib:"libcmtd"
# SUBTRACT LINK32 /pdb:none /map

!ENDIF 

# Begin Target

# Name "VertexBufferManager - Win32 Release"
# Name "VertexBufferManager - Win32 Debug"
# Name "VertexBufferManager - Win32 MSHeap Release"
# Name "VertexBufferManager - Win32 MSHeap Debug"
# Begin Source File

SOURCE=.\CopyVertexBufferDesc.cpp
# End Source File
# Begin Source File

SOURCE=.\VertexBufferManager.cpp
# End Source File
# End Target
# End Project
