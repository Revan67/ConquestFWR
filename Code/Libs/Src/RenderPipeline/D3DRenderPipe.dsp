# Microsoft Developer Studio Project File - Name="D3DRenderPipe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=D3DRenderPipe - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "D3DRenderPipe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "D3DRenderPipe.mak" CFG="D3DRenderPipe - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "D3DRenderPipe - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "D3DRenderPipe - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "D3DRenderPipe - Win32 MSHeap Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "D3DRenderPipe - Win32 MSHeap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/RenderPipeline", URVAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "D3DRenderPipe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\DirectDraw\D3D\Release"
# PROP Intermediate_Dir ".\DirectDraw\D3D\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Zi /Ot /Oa /Ow /Oi /Oy /Ob2 /I "." /I ".\.." /I ".\..\.." /D RP_RD_DEBUG=0 /D "_WINDOWS" /D DA_ERROR_LEVEL=1 /D "D3D_OVERLOADS" /D "DA_HEAP_ENABLED" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX"windows.h" /FD /c
# SUBTRACT CPP /Ox /Og /Os /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib comheap.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib d3dx.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy .\DirectDraw\D3D\Release\*.dll ..\..\explicitdll	copy .\DirectDraw\D3D\Release\*.pdb ..\..\explicitdll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "newRende"
# PROP BASE Intermediate_Dir "newRende"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\DirectDraw\D3D\Debug"
# PROP Intermediate_Dir ".\DirectDraw\D3D\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W3 /GX /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /D RP_RD_DEBUG=1 /D "_WINDOWS" /D "D3D_OVERLOADS" /D DA_ERROR_LEVEL=8 /D "DA_HEAP_ENABLED" /D "WIN32" /D "_DEBUG" /Fr /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib comheap.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib dxguid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy .\DirectDraw\D3D\Debug\*.dll ..\..\ExplicitDLL	copy .\DirectDraw\D3D\Debug\*.pdb ..\..\explicitdll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "D3DRenderPipe___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "D3DRenderPipe___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\DirectDraw\D3D\MSHeap_Debug"
# PROP Intermediate_Dir ".\DirectDraw\D3D\MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /ML /W3 /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /D "_DEBUG" /D "_WINDOWS" /D "D3D_OVERLOADS" /D DA_ERROR_LEVEL=8 /D "WIN32" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Zi /Od /Gf /I "." /I ".\.." /I ".\..\.." /D DA_ERROR_LEVEL=8 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "_DEBUG" /FR /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /i "..\\" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib comheap.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /nodefaultlib:"LIBC"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "D3DRenderPipe___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "D3DRenderPipe___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\DirectDraw\D3D\MSHeap_Release"
# PROP Intermediate_Dir ".\DirectDraw\D3D\MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Zi /Ot /Oa /Ow /Oi /Oy /Ob2 /I "." /I ".\.." /I ".\..\.." /D "NDEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=1 /D "WIN32" /D "D3D_OVERLOADS" /D "DA_HEAP_ENABLED" /FR /YX"windows.h" /FD /c
# SUBTRACT BASE CPP /Ox /Og /Os
# ADD CPP /nologo /G6 /MD /Zi /Ot /Oa /Ow /Oi /Oy /Ob2 /I "." /I ".\.." /I ".\..\.." /D DA_ERROR_LEVEL=1 /D "_WINDOWS" /D "D3D_OVERLOADS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FR /YX"windows.h" /FD /c
# SUBTRACT CPP /Ox /Og /Os
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /i "..\\" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib comheap.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 vfw32.lib amstrmid.lib quartz.lib strmbase.lib kernel32.lib shell32.lib oleaut32.lib uuid.lib gdi32.lib mathlib.lib dacom.lib ddraw.lib rpul.lib advapi32.lib ole32.lib user32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBC"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "D3DRenderPipe - Win32 Release"
# Name "D3DRenderPipe - Win32 Debug"
# Name "D3DRenderPipe - Win32 MSHeap Debug"
# Name "D3DRenderPipe - Win32 MSHeap Release"
# Begin Group "External Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FVF.h
# End Source File
# End Group
# Begin Group "Local Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DirectDraw\DirectDraw.h
# End Source File
# Begin Source File

SOURCE=.\DirectDraw\DirectDrawTexture.h
# End Source File
# Begin Source File

SOURCE=.\RenderDebugger.h
# End Source File
# Begin Source File

SOURCE=.\StateCache.h
# End Source File
# Begin Source File

SOURCE=.\StateInfo.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\DirectDraw\D3d\Direct3D_pipe.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectDraw\DirectDraw.cpp

!IF  "$(CFG)" == "D3DRenderPipe - Win32 Release"

# ADD CPP /FAs /FR

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 Debug"

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 MSHeap Debug"

!ELSEIF  "$(CFG)" == "D3DRenderPipe - Win32 MSHeap Release"

# ADD BASE CPP /FAs /FR
# ADD CPP /FAs /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DirectDraw\DirectDrawTexture.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectDraw\DirectShow.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderDebugger.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectDraw\VideoForWindows.cpp
# End Source File
# End Target
# End Project
