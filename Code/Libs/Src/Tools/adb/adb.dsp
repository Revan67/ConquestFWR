# Microsoft Developer Studio Project File - Name="adb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=adb - Win32 EndUser
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "adb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "adb.mak" CFG="adb - Win32 EndUser"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "adb - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "adb - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "adb - Win32 EndUser" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/Database", MYFAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "adb - Win32 Release"

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
# ADD CPP /nologo /G5 /W3 /GX /O1 /I "..\include" /I "..\..\..\..\Shared\Dinclude" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FAs /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 DACOM.lib COMHEAP.lib winmm.lib COMCTL32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386 /out:"..\..\..\..\App\Src\Adb.exe"
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "adb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MDd /W3 /GX /Zi /Od /I "..\include" /I "..\..\..\..\Shared\Dinclude" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib DACOM.lib COMDLG32.LIB SHELL32.LIB ODBC32.LIB /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"..\..\..\BIN\ADB.EXE" /pdbtype:sept

!ELSEIF  "$(CFG)" == "adb - Win32 EndUser"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "adb___Win32_EndUser0"
# PROP BASE Intermediate_Dir "adb___Win32_EndUser0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "adb___Win32_EndUser0"
# PROP Intermediate_Dir "adb___Win32_EndUser0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MDd /W3 /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"pch.h" /FD /c
# ADD CPP /nologo /G5 /MDd /W3 /GX /Zi /Od /I "..\include" /I "..\..\..\..\Shared\Dinclude" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_END_USER" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib DACOM.lib COMDLG32.LIB SHELL32.LIB ODBC32.LIB /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib DACOM.lib COMDLG32.LIB SHELL32.LIB ODBC32.LIB /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"..\..\..\..\App\Src\Adb.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "adb - Win32 Release"
# Name "adb - Win32 Debug"
# Name "adb - Win32 EndUser"
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\..\DXSDK\Include\basetsd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\DACOM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Document.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\EventSys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\FDump.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\HKEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IDispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IDocClient.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\IStringSet.h
# End Source File
# Begin Source File

SOURCE=.\IStructEnumerator.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\MemFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\STDDAT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TSmartPointer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TYPEDEFS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\ViewCnst.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Viewer.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ADBFileImport.cpp
# End Source File
# Begin Source File

SOURCE=.\database.CPP
# End Source File
# Begin Source File

SOURCE=.\database.RC
# End Source File
# Begin Source File

SOURCE=.\dbexport.cpp
# End Source File
# Begin Source File

SOURCE=.\dbexport.h
# End Source File
# Begin Source File

SOURCE=.\DbImport.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\dbimport.h
# End Source File
# Begin Source File

SOURCE=.\dbkeys.H
# End Source File
# Begin Source File

SOURCE=.\dbstuff.cpp
# End Source File
# Begin Source File

SOURCE=.\dbstuff.h
# End Source File
# Begin Source File

SOURCE=.\dbsymbols.cpp
# End Source File
# Begin Source File

SOURCE=.\dbsymbols.h
# End Source File
# Begin Source File

SOURCE=.\dbtreeview.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\dbtreeview.h
# End Source File
# Begin Source File

SOURCE=.\devil.bmp
# End Source File
# Begin Source File

SOURCE=.\export_mdb.cpp
# End Source File
# Begin Source File

SOURCE=.\export_mdb.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\HIBIGBOY.wav
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\MicrosoftDatabaseExport.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjClass.h
# End Source File
# Begin Source File

SOURCE=.\PathExport.cpp
# End Source File
# Begin Source File

SOURCE=.\Pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\Pch.h
# End Source File
# Begin Source File

SOURCE=.\Printheap.cpp
# End Source File
# Begin Source File

SOURCE=.\RawData.cpp
# End Source File
# Begin Source File

SOURCE=.\RawData.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\..\Viewer\Docuview\Symtable.h
# End Source File
# Begin Source File

SOURCE=.\TDocClient.h
# End Source File
# Begin Source File

SOURCE=.\Userdefaults.CPP
# End Source File
# Begin Source File

SOURCE=.\Userdefaults.H
# End Source File
# End Target
# End Project
