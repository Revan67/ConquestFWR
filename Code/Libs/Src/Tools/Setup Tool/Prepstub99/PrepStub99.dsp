# Microsoft Developer Studio Project File - Name="PrepStub99" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PrepStub99 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PrepStub99.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PrepStub99.mak" CFG="PrepStub99 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PrepStub99 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PrepStub99 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Hotsetup/Prepstub3/PrepStub99""
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PrepStub99 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib comctl32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "PrepStub99 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "PrepStub99 - Win32 Release"
# Name "PrepStub99 - Win32 Debug"
# Begin Group "Source Files (v3)"

# PROP Default_Filter ""
# Begin Group "Base Classes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CFileHist.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileList.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileRule.cpp
# End Source File
# Begin Source File

SOURCE=.\CFrameWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\CListView.cpp
# End Source File
# Begin Source File

SOURCE=.\CPickList.cpp
# End Source File
# Begin Source File

SOURCE=.\CProgressDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\CSplashWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\CSplitterWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\CStringList.cpp
# End Source File
# Begin Source File

SOURCE=.\CTabView.cpp
# End Source File
# Begin Source File

SOURCE=.\CTreeView.cpp
# End Source File
# End Group
# Begin Group "Compiler"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Binary.cpp
# End Source File
# Begin Source File

SOURCE=.\CMPSTATE.CPP
# End Source File
# Begin Source File

SOURCE=.\command.cpp
# End Source File
# Begin Source File

SOURCE=.\diskinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MYASSERT.CPP
# End Source File
# Begin Source File

SOURCE=.\setupdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\textdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\TextWrite.cpp
# End Source File
# Begin Source File

SOURCE=.\UTIL.CPP
# End Source File
# Begin Source File

SOURCE=.\VERUTIL.CPP
# End Source File
# End Group
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CCheckBoxListView.cpp
# End Source File
# Begin Source File

SOURCE=.\CCommandListView.cpp
# End Source File
# Begin Source File

SOURCE=.\CPrepDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\EditCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\PrepStub99.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\RegExMatch.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Group "Compiler Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CMPSTATE.H
# End Source File
# Begin Source File

SOURCE=.\Command.h
# End Source File
# Begin Source File

SOURCE=.\diskinfo.h
# End Source File
# Begin Source File

SOURCE=.\HotSetupRC.h
# End Source File
# Begin Source File

SOURCE=.\MYASSERT.H
# End Source File
# Begin Source File

SOURCE=.\Prepstub.h
# End Source File
# Begin Source File

SOURCE=.\script.h
# End Source File
# Begin Source File

SOURCE=.\setupdoc.h
# End Source File
# Begin Source File

SOURCE=.\textdoc.h
# End Source File
# Begin Source File

SOURCE=.\UTIL.H
# End Source File
# Begin Source File

SOURCE=.\VERUTIL.H
# End Source File
# End Group
# Begin Group "Base Class Header FIles"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CFileHist.hpp
# End Source File
# Begin Source File

SOURCE=.\CFileList.hpp
# End Source File
# Begin Source File

SOURCE=.\CFileRule.hpp
# End Source File
# Begin Source File

SOURCE=.\CFrameWnd.hpp
# End Source File
# Begin Source File

SOURCE=.\CListView.hpp
# End Source File
# Begin Source File

SOURCE=.\CPickList.hpp
# End Source File
# Begin Source File

SOURCE=.\CProgressDlg.hpp
# End Source File
# Begin Source File

SOURCE=.\CSplashWnd.hpp
# End Source File
# Begin Source File

SOURCE=.\CSplitterWnd.hpp
# End Source File
# Begin Source File

SOURCE=.\CStringList.hpp
# End Source File
# Begin Source File

SOURCE=.\CTabView.hpp
# End Source File
# Begin Source File

SOURCE=.\CTreeView.hpp
# End Source File
# Begin Source File

SOURCE=.\MULTIMON.H
# End Source File
# End Group
# Begin Group "Main Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CCheckBoxListView.hpp
# End Source File
# Begin Source File

SOURCE=.\CCommandListView.hpp
# End Source File
# Begin Source File

SOURCE=.\CPrepDoc.hpp
# End Source File
# Begin Source File

SOURCE=.\EditCommand.hpp
# End Source File
# Begin Source File

SOURCE=.\PrepStub99.hpp
# End Source File
# Begin Source File

SOURCE=.\ProcessBuild.hpp
# End Source File
# Begin Source File

SOURCE=.\RegExMatch.hpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StateInfo.h
# End Source File
# End Group
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CHECKBOXES.bmp
# End Source File
# Begin Source File

SOURCE=.\folder.ico
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\inject.ico
# End Source File
# Begin Source File

SOURCE=.\notebook.ico
# End Source File
# Begin Source File

SOURCE=.\OS_ICONS.bmp
# End Source File
# Begin Source File

SOURCE=.\OS_ICONS_MASK.bmp
# End Source File
# Begin Source File

SOURCE=.\PrepStub99.ico
# End Source File
# Begin Source File

SOURCE=.\PrepStub99.rc
# End Source File
# Begin Source File

SOURCE=.\Splash.bmp
# End Source File
# Begin Source File

SOURCE=.\Splith.cur
# End Source File
# Begin Source File

SOURCE=.\Splitv.cur
# End Source File
# Begin Source File

SOURCE=.\strings.ico
# End Source File
# Begin Source File

SOURCE=.\verify.ico
# End Source File
# End Group
# End Target
# End Project
