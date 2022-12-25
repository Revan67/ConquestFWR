# Microsoft Developer Studio Project File - Name="VfxView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=VfxView - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VfxView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VfxView.mak" CFG="VfxView - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VfxView - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VfxView - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/Viewer/VfxView", LPEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VfxView - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "VfxView_"
# PROP BASE Intermediate_Dir "VfxView_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Zi /O2 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "VfxView - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "VfxView0"
# PROP BASE Intermediate_Dir "VfxView0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W3 /Zi /Od /Gf /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "VfxView - Win32 Release"
# Name "VfxView - Win32 Debug"
# Begin Source File

SOURCE=.\VfxShapeViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\VfxShapeViewer.h
# End Source File
# Begin Source File

SOURCE=.\Winvfx16.asm

!IF  "$(CFG)" == "VfxView - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\Winvfx16.asm

"$(IntDir)\WinVFX16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /Fo$(IntDir)\WinVFX16.obj  $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "VfxView - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\Winvfx16.asm

"$(IntDir)\WinVFX16.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /Fo$(IntDir)\WinVFX16.obj  $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
