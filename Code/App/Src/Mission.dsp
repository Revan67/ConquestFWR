# Microsoft Developer Studio Project File - Name="Mission" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Mission - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mission.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mission.mak" CFG="Mission - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mission - Win32 Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mission - Win32 DEMO Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mission - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mission - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mission - Win32 DEMO Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/Src", KHOAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Mission - Win32 Final"

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
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D "BUILD_MISSION" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Final\Globals.lib Final\Trim.lib DXGuid.lib MathLib.lib dacom.lib rpul.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "Mission - Win32 DEMO Final"

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
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D "BUILD_MISSION" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /D "_DEMO_" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 DemoFinal\Globals.lib DemoFinal\Trim.lib DXGuid.lib MathLib.lib dacom.lib rpul.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "Mission - Win32 Release"

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
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Oa /Ow /Og /Oi /Oy- /Gf /I "include" /I "..\DInclude" /D "BUILD_MISSION" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Release\Globals.lib Release\Trim.lib DXGuid.lib MathLib.lib dacom.lib rpul.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Mission - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MissionDebug"
# PROP BASE Intermediate_Dir "MissionDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "MissionDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D "BUILD_MISSION" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Debug\Globals.lib Debug\Trim.lib DXGuid.lib MathLib.lib dacom.lib rpul.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Mission - Win32 DEMO Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MissionDemoDebug"
# PROP BASE Intermediate_Dir "MissionDemoDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DemoDebug"
# PROP Intermediate_Dir "MissionDemoDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D "BUILD_MISSION" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DEMO_" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 DemoDebug\Globals.lib DemoDebug\Trim.lib DXGuid.lib MathLib.lib dacom.lib rpul.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Mission - Win32 Final"
# Name "Mission - Win32 DEMO Final"
# Name "Mission - Win32 Release"
# Name "Mission - Win32 Debug"
# Name "Mission - Win32 DEMO Debug"
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\CommPacket.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMenuObjectives.h
# End Source File
# Begin Source File

SOURCE=.\Include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\IBanker.h
# End Source File
# Begin Source File

SOURCE=.\ISPlayerAI.h
# End Source File
# Begin Source File

SOURCE=.\MapGen.h
# End Source File
# Begin Source File

SOURCE=.\Include\MGlobals.h
# End Source File
# Begin Source File

SOURCE=.\Mission.h
# End Source File
# Begin Source File

SOURCE=.\Include\MPartRef.h
# End Source File
# Begin Source File

SOURCE=.\Include\MScript.h
# End Source File
# Begin Source File

SOURCE=.\NetVector.h
# End Source File
# Begin Source File

SOURCE=.\ObjSet.h
# End Source File
# Begin Source File

SOURCE=.\OpAgent.h
# End Source File
# Begin Source File

SOURCE=.\RuseKeys.h
# End Source File
# Begin Source File

SOURCE=.\RuseMap.h
# End Source File
# Begin Source File

SOURCE=.\ScrollingText.h
# End Source File
# Begin Source File

SOURCE=.\SPlayerAI.h
# End Source File
# Begin Source File

SOURCE=.\UnitComm.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ".cpp"
# Begin Group "StrategicAI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MCorvetteRush.cpp
# End Source File
# Begin Source File

SOURCE=.\MDeny.cpp
# End Source File
# Begin Source File

SOURCE=.\MDreadnoughts.cpp
# End Source File
# Begin Source File

SOURCE=.\MForgers.cpp
# End Source File
# Begin Source File

SOURCE=.\MFortress.cpp
# End Source File
# Begin Source File

SOURCE=.\MForwardBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\MFrigateRush.cpp
# End Source File
# Begin Source File

SOURCE=.\MStandardMantisAI.cpp
# End Source File
# Begin Source File

SOURCE=.\MStandardSolarianAI.cpp
# End Source File
# Begin Source File

SOURCE=.\MSwarm.cpp
# End Source File
# Begin Source File

SOURCE=.\SPlayerAI.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Banker.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_BuildInd.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_Fabricator.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_FindButtons.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_Fleet.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_GenPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_Ind.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_MulMix.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_None.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_ResBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_Research.cpp
# End Source File
# Begin Source File

SOURCE=.\CMenu_WarPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\Macrohelp.cpp
# End Source File
# Begin Source File

SOURCE=.\MapGen.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_AdmiralBar.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Chat.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Diplomacy.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_EndGame.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_netunloading.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Objectives.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_PlayerChat.cpp
# End Source File
# Begin Source File

SOURCE=.\MGlobals.cpp
# End Source File
# Begin Source File

SOURCE=.\Mission.cpp
# End Source File
# Begin Source File

SOURCE=.\MScript.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjMapIterator.cpp
# End Source File
# Begin Source File

SOURCE=.\OpAgent.cpp
# End Source File
# Begin Source File

SOURCE=.\Pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\PlayerMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ruse.cpp
# End Source File
# Begin Source File

SOURCE=.\RuseMap.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollingText.cpp
# End Source File
# Begin Source File

SOURCE=.\Search.asm

!IF  "$(CFG)" == "Mission - Win32 Final"

# Begin Custom Build
IntDir=.\Final
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Mission - Win32 DEMO Final"

# Begin Custom Build
IntDir=.\DemoFinal
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Mission - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Mission - Win32 Debug"

# Begin Custom Build
IntDir=.\MissionDebug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Mission - Win32 DEMO Debug"

# Begin Custom Build
IntDir=.\MissionDemoDebug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SuperTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\UnitComm.cpp
# End Source File
# End Group
# Begin Group "Data Definition Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DInclude\DCQGame.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEndGame.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DGlobalData.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMapGen.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPlayerMenu.h
# End Source File
# End Group
# End Target
# End Project
