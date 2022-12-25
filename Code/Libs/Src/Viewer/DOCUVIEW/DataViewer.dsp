# Microsoft Developer Studio Project File - Name="DataViewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DataViewer - Win32 OptimizeForSpeed
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DataViewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DataViewer.mak" CFG="DataViewer - Win32 OptimizeForSpeed"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DataViewer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DataViewer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DataViewer - Win32 OptimizeForSpeed" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/DataViewer", UVCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DataViewer - Win32 Release"

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
# ADD CPP /nologo /G5 /W4 /Zi /O2 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=1 /YX /FD -QaxW /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 COMHeap.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /pdb:"../../../ExplicitDLL/DocuView.pdb" /map /debug /machine:I386 /out:"Release\DocuView.dll" /pdbtype:con
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\..\copy_release_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DataViewer - Win32 Debug"

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
# ADD CPP /nologo /G5 /ML /W4 /Zi /Od /Gf /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 COMHeap.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"../../../ExplicitDLL/DocuView.pdb" /map /debug /machine:I386 /out:"Debug\DocuView.dll" /pdbtype:con /SECTION:.text,REW
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\..\copy_debug_explicit.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DataViewer - Win32 OptimizeForSpeed"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DataViewer___Win32_OptimizeForSpeed"
# PROP BASE Intermediate_Dir "DataViewer___Win32_OptimizeForSpeed"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DataViewer___Win32_OptimizeForSpeed"
# PROP Intermediate_Dir "DataViewer___Win32_OptimizeForSpeed"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W4 /Zi /Od /Gf /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /Fr /YX /FD /c
# ADD CPP /nologo /G5 /ML /W4 /Zi /O2 /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 COMHeap.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"../../../ExplicitDLL/DocuView.pdb" /map /debug /machine:I386 /out:"../../../ExplicitDLL/DocuView.dll" /pdbtype:con /SECTION:.text,REW
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 COMHeap.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"../../../ExplicitDLL/DocuView.pdb" /map /debug /machine:I386 /out:"../../../ExplicitDLL/DocuView.dll" /pdbtype:con /SECTION:.text,REW
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "DataViewer - Win32 Release"
# Name "DataViewer - Win32 Debug"
# Name "DataViewer - Win32 OptimizeForSpeed"
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\DataView.h
# End Source File
# Begin Source File

SOURCE=.\Ferror.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\IStringSet.h
# End Source File
# Begin Source File

SOURCE=.\IStructEnumerator.h
# End Source File
# Begin Source File

SOURCE=.\Lxtables.h
# End Source File
# Begin Source File

SOURCE=.\Parserrs.h
# End Source File
# Begin Source File

SOURCE=.\Prttoken.h
# End Source File
# Begin Source File

SOURCE=.\Symtable.h
# End Source File
# Begin Source File

SOURCE=.\Tokendef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\ViewCnst.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Viewer.h
# End Source File
# End Group
# Begin Group "Source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\DataParser.cpp
# End Source File
# Begin Source File

SOURCE=.\DataView.cpp

!IF  "$(CFG)" == "DataViewer - Win32 Release"

# ADD CPP /Ob1 /FAs

!ELSEIF  "$(CFG)" == "DataViewer - Win32 Debug"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DataViewer - Win32 OptimizeForSpeed"

# ADD BASE CPP /FAs
# ADD CPP /FAs

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Document.cpp

!IF  "$(CFG)" == "DataViewer - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DataViewer - Win32 Debug"

!ELSEIF  "$(CFG)" == "DataViewer - Win32 OptimizeForSpeed"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\FERROR.cpp
# End Source File
# Begin Source File

SOURCE=.\FILESTRM.cpp
# End Source File
# Begin Source File

SOURCE=.\LXTABLES.cpp
# End Source File
# Begin Source File

SOURCE=.\NUMBER.cpp
# End Source File
# Begin Source File

SOURCE=.\PARS1.cpp
# End Source File
# Begin Source File

SOURCE=.\PRTTOKEN.cpp
# End Source File
# Begin Source File

SOURCE=.\SCANNER.cpp
# End Source File
# Begin Source File

SOURCE=.\StringSet.cpp
# End Source File
# Begin Source File

SOURCE=.\SYMTABLE.cpp
# End Source File
# Begin Source File

SOURCE=.\treeview.cpp
# End Source File
# Begin Source File

SOURCE=.\treeview.h
# End Source File
# Begin Source File

SOURCE=.\ViewCnst.cpp

!IF  "$(CFG)" == "DataViewer - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "DataViewer - Win32 Debug"

!ELSEIF  "$(CFG)" == "DataViewer - Win32 OptimizeForSpeed"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\DataView.rc
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# End Target
# End Project
