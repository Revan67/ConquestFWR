# Microsoft Developer Studio Project File - Name="Trim" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Trim - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Trim.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Trim.mak" CFG="Trim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Trim - Win32 Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Trim - Win32 DEMO Final" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Trim - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Trim - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Trim - Win32 DEMO Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/Src", DFDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Trim - Win32 Final"

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
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D "BUILD_TRIM" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Final\Globals.lib ddraw.lib strmiids.lib shlwapi.lib rpul.lib version.lib dinput.lib amstrmid.lib imm32.lib dsetup.lib DXGuid.lib MathLib.lib DACOM.lib COMHeap.lib winmm.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Final"

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
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D "BUILD_TRIM" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /D "_DEMO_" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 DemoFinal\Globals.lib ddraw.lib strmiids.lib shlwapi.lib rpul.lib version.lib dinput.lib amstrmid.lib imm32.lib dsetup.lib DXGuid.lib MathLib.lib DACOM.lib COMHeap.lib winmm.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "Trim - Win32 Release"

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
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Oa /Ow /Og /Oi /Oy- /Gf /I "include" /I "..\DInclude" /D "BUILD_TRIM" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 Release\Globals.lib ddraw.lib strmiids.lib shlwapi.lib rpul.lib version.lib dinput.lib amstrmid.lib imm32.lib dsetup.lib DXGuid.lib MathLib.lib DACOM.lib COMHeap.lib winmm.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Trim - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "TrimDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "TrimDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D "BUILD_TRIM" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Debug\Globals.lib ddraw.lib strmiids.lib dsetup.lib shlwapi.lib rpul.lib version.lib dinput.lib amstrmid.lib imm32.lib DXGuid.lib MathLib.lib DACOM.lib COMHeap.lib winmm.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DemoDebug"
# PROP BASE Intermediate_Dir "TrimDemoDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DemoDebug"
# PROP Intermediate_Dir "TrimDemoDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D "BUILD_TRIM" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DEMO_" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 DemoDebug\Globals.lib ddraw.lib strmiids.lib dsetup.lib shlwapi.lib rpul.lib version.lib dinput.lib amstrmid.lib imm32.lib DXGuid.lib MathLib.lib DACOM.lib COMHeap.lib winmm.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386

!ENDIF 

# Begin Target

# Name "Trim - Win32 Final"
# Name "Trim - Win32 DEMO Final"
# Name "Trim - Win32 Release"
# Name "Trim - Win32 Debug"
# Name "Trim - Win32 DEMO Debug"
# Begin Group "Data Definition Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DInclude\DAdmiralBar.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DAnimate.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\Data.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBaseData.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DButton.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DCamera.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DCombobox.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DConfirm.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DDiplomacyButton.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DDropDown.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEdit.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEndGame.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DExplosion.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFog.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFonts.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DHotButton.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DHotButtonText.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DHotStatic.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DIcon.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\Digoptions.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DInstance.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DLight.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DListBox.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DLoadSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMenu1.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMenu_Credits.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMenuBriefing.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMovieScreen.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMusic.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DNebula.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DNewPlayer.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPause.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DProgressStatic.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DQueueControl.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DScrollBar.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DShipSilButton.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSilhouette.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSlider.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSounds.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DStatic.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DStringPack.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSysmap.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTabControl.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DToolbar.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTypes.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\GameTypes.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\Sfxid.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\BaseHotRect.h
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\CQGame.h
# End Source File
# Begin Source File

SOURCE=.\CQImage.h
# End Source File
# Begin Source File

SOURCE=.\Include\CQTrace.h
# End Source File
# Begin Source File

SOURCE=.\Cursor.h
# End Source File
# Begin Source File

SOURCE=.\DrawAgent.h
# End Source File
# Begin Source File

SOURCE=.\DSStream.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSysKitSaveLoad.h
# End Source File
# Begin Source File

SOURCE=.\EventPriority.h
# End Source File
# Begin Source File

SOURCE=.\Frame.h
# End Source File
# Begin Source File

SOURCE=.\GenData.h
# End Source File
# Begin Source File

SOURCE=.\Include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\GridVector.h
# End Source File
# Begin Source File

SOURCE=.\GRPackets.h
# End Source File
# Begin Source File

SOURCE=.\Hintbox.h
# End Source File
# Begin Source File

SOURCE=.\Hotkeys.h
# End Source File
# Begin Source File

SOURCE=.\HPEnum.h
# End Source File
# Begin Source File

SOURCE=.\IActiveButton.h
# End Source File
# Begin Source File

SOURCE=.\IAnimate.h
# End Source File
# Begin Source File

SOURCE=.\IBackground.h
# End Source File
# Begin Source File

SOURCE=.\IBriefing.h
# End Source File
# Begin Source File

SOURCE=.\IBuildWindow.h
# End Source File
# Begin Source File

SOURCE=.\IButton2.h
# End Source File
# Begin Source File

SOURCE=.\IDiplomacyButton.h
# End Source File
# Begin Source File

SOURCE=.\IEdit2.h
# End Source File
# Begin Source File

SOURCE=.\IGameProgress.h
# End Source File
# Begin Source File

SOURCE=.\IHotButton.h
# End Source File
# Begin Source File

SOURCE=.\IHotStatic.h
# End Source File
# Begin Source File

SOURCE=.\IIcon.h
# End Source File
# Begin Source File

SOURCE=.\IImageReader.h
# End Source File
# Begin Source File

SOURCE=.\ILineManager.h
# End Source File
# Begin Source File

SOURCE=.\IListBox.h
# End Source File
# Begin Source File

SOURCE=.\IMultiHotButton.h
# End Source File
# Begin Source File

SOURCE=.\InProgressAnim.h
# End Source File
# Begin Source File

SOURCE=.\IProgressStatic.h
# End Source File
# Begin Source File

SOURCE=.\IQueueControl.h
# End Source File
# Begin Source File

SOURCE=.\IResource.h
# End Source File
# Begin Source File

SOURCE=.\IScrollBar.h
# End Source File
# Begin Source File

SOURCE=.\IShapeLoader.h
# End Source File
# Begin Source File

SOURCE=.\IShipSilButton.h
# End Source File
# Begin Source File

SOURCE=.\ISlider.h
# End Source File
# Begin Source File

SOURCE=.\IStatic.h
# End Source File
# Begin Source File

SOURCE=.\ISubtitle.h
# End Source File
# Begin Source File

SOURCE=.\ITabControl.h
# End Source File
# Begin Source File

SOURCE=.\ITeletype.h
# End Source File
# Begin Source File

SOURCE=.\IToolbar.h
# End Source File
# Begin Source File

SOURCE=.\LFParser.h
# End Source File
# Begin Source File

SOURCE=.\Menu.h
# End Source File
# Begin Source File

SOURCE=.\MScroll.h
# End Source File
# Begin Source File

SOURCE=.\MusicManager.h
# End Source File
# Begin Source File

SOURCE=.\NetBuffer.h
# End Source File
# Begin Source File

SOURCE=.\NetConnectBuffers.h
# End Source File
# Begin Source File

SOURCE=.\NetFileTransfer.h
# End Source File
# Begin Source File

SOURCE=.\NetPacket.h
# End Source File
# Begin Source File

SOURCE=.\Objwatch.h
# End Source File
# Begin Source File

SOURCE=.\RandomNum.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\SFX.h
# End Source File
# Begin Source File

SOURCE=.\SoundManager.h
# End Source File
# Begin Source File

SOURCE=.\StatusBar.h
# End Source File
# Begin Source File

SOURCE=.\StringData.h
# End Source File
# Begin Source File

SOURCE=.\TCPoint.h
# End Source File
# Begin Source File

SOURCE=.\TDocClient.h
# End Source File
# Begin Source File

SOURCE=.\TManager.h
# End Source File
# Begin Source File

SOURCE=.\TResClient.h
# End Source File
# Begin Source File

SOURCE=.\TResource.h
# End Source File
# Begin Source File

SOURCE=.\UserDefaults.h
# End Source File
# Begin Source File

SOURCE=.\VideoSurface.h
# End Source File
# Begin Source File

SOURCE=.\VoxCompress.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\Animate.cpp
# End Source File
# Begin Source File

SOURCE=.\BmpRead.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildButton.cpp
# End Source File
# Begin Source File

SOURCE=.\Button2.cpp
# End Source File
# Begin Source File

SOURCE=.\Camera.cpp
# End Source File
# Begin Source File

SOURCE=.\Combobox.cpp
# End Source File
# Begin Source File

SOURCE=.\CQImage.cpp
# End Source File
# Begin Source File

SOURCE=.\cqpipeline.cpp
# End Source File
# Begin Source File

SOURCE=.\Cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\DiplomacyButton.cpp
# End Source File
# Begin Source File

SOURCE=.\DrawAgent.cpp
# End Source File
# Begin Source File

SOURCE=.\DrawAgent16.cpp
# End Source File
# Begin Source File

SOURCE=.\Dropdown.cpp
# End Source File
# Begin Source File

SOURCE=.\DSStream.cpp
# End Source File
# Begin Source File

SOURCE=.\DumpView.cpp
# End Source File
# Begin Source File

SOURCE=.\Edit2.cpp
# End Source File
# Begin Source File

SOURCE=.\EulaWin.cpp
# End Source File
# Begin Source File

SOURCE=.\GameProgress.cpp
# End Source File
# Begin Source File

SOURCE=.\GenData.cpp
# End Source File
# Begin Source File

SOURCE=.\GridVector.cpp
# End Source File
# Begin Source File

SOURCE=.\Hintbox.cpp
# End Source File
# Begin Source File

SOURCE=.\HKManager.cpp
# End Source File
# Begin Source File

SOURCE=.\HotButton.cpp
# End Source File
# Begin Source File

SOURCE=.\HotStatic.cpp
# End Source File
# Begin Source File

SOURCE=.\Icon.cpp
# End Source File
# Begin Source File

SOURCE=.\IniConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\InProgressAnim.cpp
# End Source File
# Begin Source File

SOURCE=.\LFParser.cpp
# End Source File
# Begin Source File

SOURCE=.\LineManager.cpp
# End Source File
# Begin Source File

SOURCE=.\lines.cpp
# End Source File
# Begin Source File

SOURCE=.\Listbox.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadFont.cpp
# End Source File
# Begin Source File

SOURCE=.\LogFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Macrohelp.cpp
# End Source File
# Begin Source File

SOURCE=.\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu1.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Briefing.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_campaign.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Confirm.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Credits.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_final.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_help.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_igoptions.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_LoadSave.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_map.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_MapSelect.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_mission.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_mshell.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_netconn.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_netloading.cpp

!IF  "$(CFG)" == "Trim - Win32 Final"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Final"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "Trim - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "Trim - Win32 Debug"

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Menu_netsess.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_netsess2.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_newplayer.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_options.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Pause.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_SlideShow.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_slots.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_SPGame.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_SysKitSaveLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_Toolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\Menu_zone.cpp
# End Source File
# Begin Source File

SOURCE=.\Modal.cpp
# End Source File
# Begin Source File

SOURCE=.\MovieScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\MScroll.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiHotButton.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiLineFont.cpp
# End Source File
# Begin Source File

SOURCE=.\MusicManager.cpp
# End Source File
# Begin Source File

SOURCE=.\NetBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\NetConnect.cpp
# End Source File
# Begin Source File

SOURCE=.\NetConnectBuffers.cpp
# End Source File
# Begin Source File

SOURCE=.\NetFileTransfer.cpp
# End Source File
# Begin Source File

SOURCE=.\NetPacket.cpp
# End Source File
# Begin Source File

SOURCE=.\Objwatch.cpp
# End Source File
# Begin Source File

SOURCE=.\Pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\PosTool.cpp
# End Source File
# Begin Source File

SOURCE=.\PrintHeap.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressStatic.cpp
# End Source File
# Begin Source File

SOURCE=.\QueueControl.cpp
# End Source File
# Begin Source File

SOURCE=.\ResearchButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollBar.cpp
# End Source File
# Begin Source File

SOURCE=.\Search.asm

!IF  "$(CFG)" == "Trim - Win32 Final"

# Begin Custom Build
IntDir=.\Final
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Final"

# Begin Custom Build
IntDir=.\DemoFinal
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 Debug"

# Begin Custom Build
IntDir=.\TrimDebug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Debug"

# Begin Custom Build
IntDir=.\TrimDemoDebug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SFX.cpp
# End Source File
# Begin Source File

SOURCE=.\ShapeLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\ShipSilButton.cpp
# End Source File
# Begin Source File

SOURCE=.\Slider.cpp
# End Source File
# Begin Source File

SOURCE=.\SoundManager.cpp
# End Source File
# Begin Source File

SOURCE=.\SpaceEnv.cpp
# End Source File
# Begin Source File

SOURCE=.\Static.cpp
# End Source File
# Begin Source File

SOURCE=.\StatusBar.cpp
# End Source File
# Begin Source File

SOURCE=.\Streamer.cpp
# End Source File
# Begin Source File

SOURCE=.\StringData.cpp
# End Source File
# Begin Source File

SOURCE=.\Subtitle.cpp
# End Source File
# Begin Source File

SOURCE=.\SuperTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\SysContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\System.cpp
# End Source File
# Begin Source File

SOURCE=.\TabButton.cpp
# End Source File
# Begin Source File

SOURCE=.\TabControl.cpp
# End Source File
# Begin Source File

SOURCE=.\Teletype.cpp
# End Source File
# Begin Source File

SOURCE=.\TestScript.cpp
# End Source File
# Begin Source File

SOURCE=.\Tgaread.cpp
# End Source File
# Begin Source File

SOURCE=.\TManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Trim.cpp
# End Source File
# Begin Source File

SOURCE=.\Trim.def
# End Source File
# Begin Source File

SOURCE=.\UserDefaults.cpp
# End Source File
# Begin Source File

SOURCE=.\VertexBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\VfxRead.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoSurface.cpp
# End Source File
# Begin Source File

SOURCE=.\VidStream.cpp
# End Source File
# Begin Source File

SOURCE=.\VoxCompress.cpp
# End Source File
# Begin Source File

SOURCE=.\WindowManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Winvfx16.asm

!IF  "$(CFG)" == "Trim - Win32 Final"

# Begin Custom Build
IntDir=.\Final
InputPath=.\Winvfx16.asm
InputName=Winvfx16

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Final"

# Begin Custom Build
IntDir=.\DemoFinal
InputPath=.\Winvfx16.asm
InputName=Winvfx16

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\Winvfx16.asm
InputName=Winvfx16

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 Debug"

# Begin Custom Build
IntDir=.\TrimDebug
InputPath=.\Winvfx16.asm
InputName=Winvfx16

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Trim - Win32 DEMO Debug"

# Begin Custom Build
IntDir=.\TrimDemoDebug
InputPath=.\Winvfx16.asm
InputName=Winvfx16

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
