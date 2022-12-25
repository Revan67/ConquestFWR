# Microsoft Developer Studio Project File - Name="SCRIPT03T" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SCRIPT03T - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SCRIPT03T.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SCRIPT03T.mak" CFG="SCRIPT03T - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SCRIPT03T - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SCRIPT03T - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/App/Src/Scripts/SCRIPT03T", LPBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SCRIPT03T - Win32 Release"

# ADD CPP /MD /GX /D "_WINDLL" /D "_AFXDLL"
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_AFXDLL"
BSC32=bscmake.exe
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /machine:IX86

!ELSEIF  "$(CFG)" == "SCRIPT03T - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCRIPT03T_EXPORTS" /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /ML /W4 /ZI /Od /I "..\..\include" /I "..\include" /I "..\..\..\Dinclude" /I "..\..\..\src" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BUILD_SCRIPTS" /FR /YX"windows.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "NDEBUG" /D "_DEBUG" /mktyplib203 /win32 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dacom.lib ..\..\debug\mission.lib ..\..\debug\trim.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"..\SCRIPT03.pdb" /debug /machine:I386 /out:"..\SCRIPT03.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "SCRIPT03T - Win32 Release"
# Name "SCRIPT03T - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DataDef.cpp

!IF  "$(CFG)" == "SCRIPT03T - Win32 Release"

DEP_CPP_DATAD=\
	".\DataDef.h"\
	
NODEP_CPP_DATAD=\
	".\ypedefs.h"\
	

!ELSEIF  "$(CFG)" == "SCRIPT03T - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__DATAD="DataDef.h"	
# Begin Custom Build - Preprocessing Data Definitions
InputPath=.\DataDef.cpp
InputName=DataDef

"$(InputName).i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /c /nologo /I..\include /I..\..\include /D_ADB /D__hexview=mutable /D__readonly=const $(InputPath) 
	cl /P /EP /nologo /I..\include /I..\..\include /D_ADB $(InputPath) 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Helper\helper.cpp

!IF  "$(CFG)" == "SCRIPT03T - Win32 Release"

!ELSEIF  "$(CFG)" == "SCRIPT03T - Win32 Debug"

# PROP Intermediate_Dir "Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SCRIPT03T.cpp

!IF  "$(CFG)" == "SCRIPT03T - Win32 Release"

!ELSEIF  "$(CFG)" == "SCRIPT03T - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SCRIPT03T.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DataDef.h
# End Source File
# Begin Source File

SOURCE=..\Include\DLLScriptMain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\DInclude\DMBaseData.h
# End Source File
# Begin Source File

SOURCE=..\Helper\helper.h
# End Source File
# Begin Source File

SOURCE=..\Include\Local.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\MovieCameraFlags.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\MPartRef.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\MScript.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\Include\ScriptDef.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\DataDef.i
# End Source File
# End Group
# End Target
# End Project
