# Microsoft Developer Studio Project File - Name="TriMesh" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TriMesh - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TriMesh.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TriMesh.mak" CFG="TriMesh - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TriMesh - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TriMesh - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TriMesh - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TriMesh - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/RendComp/TriMesh", VCGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TriMesh - Win32 Release"

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
# ADD CPP /nologo /G5 /W3 /Zi /O2 /I "..\..\include" /D "DA_HEAP_ENABLED" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /D "WIN32" /D "_WINDOWS" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ddraw.lib comheap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "TriMesh - Win32 Debug"

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
# ADD CPP /nologo /G5 /ML /W3 /Zi /Od /I "..\..\include" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /D "DA_HEAP_ENABLED" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 comheap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "TriMesh - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TriMesh___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "TriMesh___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /Zi /O2 /I "..\..\include" /D "NDEBUG" /D DA_ERROR_LEVEL=1 /D "WIN32" /D "_WINDOWS" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /O2 /I "..\..\include" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /D "WIN32" /D "_WINDOWS" /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib comheap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes /nodefaultlib
# ADD LINK32 ddraw.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "TriMesh - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TriMesh___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "TriMesh___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /Zi /Od /I "..\..\include" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /I "..\..\include" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "_WINDOWS" /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comheap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "TriMesh - Win32 Release"
# Name "TriMesh - Win32 Debug"
# Name "TriMesh - Win32 MSHeap Release"
# Name "TriMesh - Win32 MSHeap Debug"
# Begin Source File

SOURCE=.\TriMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\TriMeshArchetype.cpp
# End Source File
# Begin Source File

SOURCE=.\TriMeshArchetype.h
# End Source File
# Begin Source File

SOURCE=.\TriMeshFaceGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\TriMeshFaceGroup.h
# End Source File
# Begin Source File

SOURCE=.\TriMeshInstance.cpp
# End Source File
# Begin Source File

SOURCE=.\TriMeshInstance.h
# End Source File
# Begin Source File

SOURCE=.\VertexBufferDescUtil.h
# End Source File
# End Target
# End Project
