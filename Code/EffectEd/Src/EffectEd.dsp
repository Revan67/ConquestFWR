# Microsoft Developer Studio Project File - Name="EffectEd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EffectEd - Win32 DebugInternal
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EffectEd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EffectEd.mak" CFG="EffectEd - Win32 DebugInternal"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EffectEd - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EffectEd - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "EffectEd - Win32 DebugInternal" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/EffectEd/Src", VRDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EffectEd - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Comctl32.lib DACOM.lib COMHeap.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "EffectEd - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /ML /W3 /Gm /GX /ZI /Od /I "..\..\shared\dinclude\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Comctl32.lib DACOM.lib COMHeap.lib MathLib.lib d3dx9dt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "EffectEd - Win32 DebugInternal"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "EffectEd___Win32_DebugInternal"
# PROP BASE Intermediate_Dir "EffectEd___Win32_DebugInternal"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EffectEd___Win32_DebugInternal"
# PROP Intermediate_Dir "EffectEd___Win32_DebugInternal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /ML /W3 /Gm /GX /ZI /Od /I "..\..\shared\dinclude\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /ML /W3 /Gm /GX /ZI /Od /I "..\..\shared\dinclude\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_INTERNAL_PATHS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Comctl32.lib DACOM.lib COMHeap.lib MathLib.lib d3dx9dt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Comctl32.lib DACOM.lib COMHeap.lib MathLib.lib d3dx9dt.lib /nologo /subsystem:windows /pdb:"DebugInternal/EffectEd.pdb" /debug /machine:I386 /out:"DebugInternal/EffectEd.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "EffectEd - Win32 Release"
# Name "EffectEd - Win32 Debug"
# Name "EffectEd - Win32 DebugInternal"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ArchList.cpp
# End Source File
# Begin Source File

SOURCE=.\Camera.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectAction.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionAnim.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionAttachTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionDetachTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionGameEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionHideTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionParticle.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionSound.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectActionSwitch.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectEd.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectFile.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectParam.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoArea.cpp
# End Source File
# Begin Source File

SOURCE=.\ParticleView.cpp
# End Source File
# Begin Source File

SOURCE=.\PreviewWin.cpp
# End Source File
# Begin Source File

SOURCE=.\Search.asm

!IF  "$(CFG)" == "EffectEd - Win32 Release"

!ELSEIF  "$(CFG)" == "EffectEd - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "EffectEd - Win32 DebugInternal"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\EffectEd___Win32_DebugInternal
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Startup.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SuperTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\TargetAnim.cpp
# End Source File
# Begin Source File

SOURCE=.\TargetCue.cpp
# End Source File
# Begin Source File

SOURCE=.\TargetHp.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeBar.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ArchList.h
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\IEffectAction.h
# End Source File
# Begin Source File

SOURCE=.\IEffectEvent.h
# End Source File
# Begin Source File

SOURCE=.\IEffectFile.h
# End Source File
# Begin Source File

SOURCE=.\IEffectParam.h
# End Source File
# Begin Source File

SOURCE=.\IEffectTarget.h
# End Source File
# Begin Source File

SOURCE=.\InfoArea.h
# End Source File
# Begin Source File

SOURCE=.\ITargetAnim.h
# End Source File
# Begin Source File

SOURCE=.\ITargetCue.h
# End Source File
# Begin Source File

SOURCE=.\ITargetHp.h
# End Source File
# Begin Source File

SOURCE=.\ParticleView.h
# End Source File
# Begin Source File

SOURCE=.\PreviewWin.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TimeBar.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Script1.rc
# End Source File
# Begin Source File

SOURCE=.\Startup.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
