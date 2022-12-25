# Microsoft Developer Studio Project File - Name="Globals" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Globals - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Globals.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Globals.mak" CFG="Globals - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Globals - Win32 Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Globals - Win32 DEMO Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Globals - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Globals - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Globals - Win32 DEMO Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/Src", DFDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Globals - Win32 Final"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Final"
# PROP BASE Intermediate_Dir "Final"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Final"
# PROP Intermediate_Dir "Final"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /O2 /I "include" /I "..\DInclude" /D "BUILD_GLOBALS" /D "BUILD_DLL" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /HEAP:100,0
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Precompiling Data definitions
PreLink_Cmds=cl /P /EP /nologo /I..\dinclude /D_ADB ..\dinclude\data.h	rc /l 0x409 /fo Final\Globals.res /dNDEBUG Globals.rc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Final"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DemoFinal"
# PROP BASE Intermediate_Dir "DemoFinal"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DemoFinal"
# PROP Intermediate_Dir "DemoFinal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /O2 /I "include" /I "..\DInclude" /D "BUILD_GLOBALS" /D "BUILD_DLL" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /D "_DEMO_" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /HEAP:100,0
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Precompiling Data definitions
PreLink_Cmds=cl /P /EP /nologo /I..\dinclude /D_ADB ..\dinclude\data.h	rc /l 0x409 /fo DemoFinal\Globals.res /dNDEBUG /d_DEMO_ Globals.rc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Globals - Win32 Release"

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
# ADD CPP /nologo /G6 /W4 /O2 /I "include" /I "..\DInclude" /D "BUILD_GLOBALS" /D "BUILD_DLL" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /HEAP:100,0
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Precompiling Data definitions
PreLink_Cmds=cl /P /EP /nologo /I..\dinclude /D_ADB ..\dinclude\data.h	rc /l 0x409 /fo Release\Globals.res /dNDEBUG Globals.rc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Globals - Win32 Debug"

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
# ADD CPP /nologo /G6 /ML /W4 /Od /I "include" /I "..\DInclude" /D "BUILD_DLL" /D "BUILD_GLOBALS" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /machine:I386 /HEAP:100,0
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Precompiling Data definitions
PreLink_Cmds=cl /P /EP /nologo /I..\dinclude /D_ADB ..\dinclude\data.h	rc /l 0x409 /fo Debug\Globals.res /d_DEBUG Globals.rc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DemoDebug"
# PROP BASE Intermediate_Dir "DemoDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DemoDebug"
# PROP Intermediate_Dir "DemoDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Od /I "include" /I "..\DInclude" /D "BUILD_DLL" /D "BUILD_GLOBALS" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DEMO_" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /machine:I386 /HEAP:100,0
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Precompiling Data definitions
PreLink_Cmds=cl /P /EP /nologo /I..\dinclude /D_ADB ..\dinclude\data.h	rc /l 0x409 /fo DemoDebug\Globals.res /d_DEBUG /d_DEMO_ Globals.rc
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Globals - Win32 Final"
# Name "Globals - Win32 DEMO Final"
# Name "Globals - Win32 Release"
# Name "Globals - Win32 Debug"
# Name "Globals - Win32 DEMO Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "*.ico, *.cur, *.fnt"
# Begin Group "Monochrome"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Res\Bw\Arrow_l.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\attack.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\ban.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Bleftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Brightscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Btmscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\build.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\capture.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\cur00003.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\cur00004.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\cursor_a.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\default.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\escort.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\H_arrow.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Harvest.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\intermediate.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\jump.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Leftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\mimic.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Minidefault.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Minimove.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\MiniRally.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Move.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\MoveAdmiral.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\patrol.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Rally.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Ram.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Repair.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Resupply.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Rightscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Sell.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\specattk.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Tleftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Topscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bw\Trightscroll.cur
# End Source File
# End Group
# Begin Source File

SOURCE=.\Res\Arrow_l.cur
# End Source File
# Begin Source File

SOURCE=.\Res\attack.cur
# End Source File
# Begin Source File

SOURCE=.\Res\ban.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Bleftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Brightscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Btmscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\build.cur
# End Source File
# Begin Source File

SOURCE=.\Res\capture.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cur00002.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cur00003.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cur00004.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\Res\cursor_a.cur
# End Source File
# Begin Source File

SOURCE=.\Res\default.cur
# End Source File
# Begin Source File

SOURCE=.\Res\default.fnt
# End Source File
# Begin Source File

SOURCE=.\Res\Escort.cur
# End Source File
# Begin Source File

SOURCE=.\Res\H_arrow.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Harvest.cur
# End Source File
# Begin Source File

SOURCE=.\Res\intermediate.cur
# End Source File
# Begin Source File

SOURCE=.\Res\jump.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Leftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\mimic.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Minidefault.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Minimove.cur
# End Source File
# Begin Source File

SOURCE=.\Res\MiniRally.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Move.cur
# End Source File
# Begin Source File

SOURCE=.\Res\MoveAdmiral.cur
# End Source File
# Begin Source File

SOURCE=.\Res\patrol.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Rally.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Ram.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Repair.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Resupply.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Rightscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Sell.cur
# End Source File
# Begin Source File

SOURCE=.\Res\specattk.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Tleftscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Topscroll.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Trightscroll.cur
# End Source File
# End Group
# Begin Source File

SOURCE=.\Conquest.ini
# End Source File
# Begin Source File

SOURCE=.\Data.cpp

!IF  "$(CFG)" == "Globals - Win32 Final"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\Data.cpp

"data.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /P /EP /nologo /I..\dinclude /D_ADB data.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Final"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\Data.cpp

"data.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /P /EP /nologo /I..\dinclude /D_ADB data.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\Data.cpp

"data.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /P /EP /nologo /I..\dinclude /D_ADB data.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\Data.cpp

"data.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /P /EP /nologo /I..\dinclude /D_ADB data.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\Data.cpp

"data.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /P /EP /nologo /I..\dinclude /D_ADB data.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\data.i
# End Source File
# Begin Source File

SOURCE=.\DBHotkeys.h
# End Source File
# Begin Source File

SOURCE=.\DBHotkeys.utf
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\Include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Globals.rc

!IF  "$(CFG)" == "Globals - Win32 Final"

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Final"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEMO_"

!ELSEIF  "$(CFG)" == "Globals - Win32 Release"

!ELSEIF  "$(CFG)" == "Globals - Win32 Debug"

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Debug"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEMO_"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Hotkeys.h
# End Source File
# Begin Source File

SOURCE=.\Hotkeys.utf
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\RuseKeys.h
# End Source File
# Begin Source File

SOURCE=.\RuseKeys.utf
# End Source File
# Begin Source File

SOURCE=.\Sfxdata.dat
# End Source File
# Begin Source File

SOURCE=.\sfxdata.xmf

!IF  "$(CFG)" == "Globals - Win32 Final"

# PROP Ignore_Default_Tool 1
USERDEP__SFXDA="..\DInclude\sfxid.h"	
# Begin Custom Build
InputPath=.\sfxdata.xmf
InputName=sfxdata

"$(InputName).dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	xmiff -i ..\DInclude $(InputPath) $(InputName).dat

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Final"

# PROP Ignore_Default_Tool 1
USERDEP__SFXDA="..\DInclude\sfxid.h"	
# Begin Custom Build
InputPath=.\sfxdata.xmf
InputName=sfxdata

"$(InputName).dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	xmiff -i ..\DInclude $(InputPath) $(InputName).dat

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__SFXDA="..\DInclude\sfxid.h"	
# Begin Custom Build
InputPath=.\sfxdata.xmf
InputName=sfxdata

"$(InputName).dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	xmiff -i ..\DInclude $(InputPath) $(InputName).dat

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__SFXDA="..\DInclude\sfxid.h"	
# Begin Custom Build
InputPath=.\sfxdata.xmf
InputName=sfxdata

"$(InputName).dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	xmiff -i ..\DInclude $(InputPath) $(InputName).dat

# End Custom Build

!ELSEIF  "$(CFG)" == "Globals - Win32 DEMO Debug"

# PROP Ignore_Default_Tool 1
USERDEP__SFXDA="..\DInclude\sfxid.h"	
# Begin Custom Build
InputPath=.\sfxdata.xmf
InputName=sfxdata

"$(InputName).dat" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	xmiff -i ..\DInclude $(InputPath) $(InputName).dat

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
