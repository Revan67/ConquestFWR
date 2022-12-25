# Microsoft Developer Studio Project File - Name="setup" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=SETUP - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "setup.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "setup.mak" CFG="SETUP - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "setup - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "setup - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""$/Hotsetup/Asheron's Call/Flat""
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "setup - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "." /I "..\generic" /I "..\hotsetup\include" /I "c:\tools\PoorlyNamed\CDALibs_Common\include" /I "c:\tools\PoorlyNamed\CDALibs_Common\interfaces" /I "c:\tools\PoorlyNamed\CDALibs_Common\components" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "STRICT" /D "SINGLECD" /Fr /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "." /i "..\hotsetup\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 hotsetup.lib dxguid.lib rpul.lib dacom.lib ddraw.lib winmm.lib version.lib lz32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dsound.lib /subsystem:windows /map /machine:I386 /nodefaultlib:"libcmt" /libpath:"..\hotsetup\libs" /OPT:REF /VERBOSE:LIB
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "setup - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /I "..\generic" /I "..\hotsetup\include" /I "c:\tools\PoorlyNamed\CDALibs_Common\include" /I "c:\tools\PoorlyNamed\CDALibs_Common\interfaces" /I "c:\tools\PoorlyNamed\CDALibs_Common\components" /D "_DEBUG" /D "SINGLECD" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "STRICT" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "." /i "..\hotsetup\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 hotsetup.lib dxguid.lib rpul.lib dacom.lib ddraw.lib winmm.lib version.lib lz32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dsound.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /nodefaultlib:"libcmtd" /nodefaultlib:"libc" /pdbtype:sept /libpath:"..\hotsetup\libs"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "setup - Win32 Release"
# Name "setup - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Generic\chklst.cpp
# End Source File
# Begin Source File

SOURCE=..\generic\DACOMProvider.cpp
# End Source File
# Begin Source File

SOURCE=..\Generic\gauge.cpp
# End Source File
# Begin Source File

SOURCE=.\Setup.cpp
# End Source File
# Begin Source File

SOURCE=.\Setup_DeviceSelector.cpp
# End Source File
# Begin Source File

SOURCE=.\Setup_DeviceWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\Setupdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\generic\SymbolTable.cpp
# End Source File
# Begin Source File

SOURCE=..\Generic\widclass.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\app_configuration.h
# End Source File
# Begin Source File

SOURCE=.\appspecific.h
# End Source File
# Begin Source File

SOURCE=.\deviceselection.h
# End Source File
# Begin Source File

SOURCE=..\Generic\Gauge.h
# End Source File
# Begin Source File

SOURCE=..\generic\IDeviceConfigurator.h
# End Source File
# Begin Source File

SOURCE=.\resc1.h
# End Source File
# Begin Source File

SOURCE=.\setup.h
# End Source File
# Begin Source File

SOURCE=.\setupdlg.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=..\Generic\widclass.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\appbase.rc

!IF  "$(CFG)" == "setup - Win32 Release"

!ELSEIF  "$(CFG)" == "setup - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cdicon.ico
# End Source File
# Begin Source File

SOURCE=.\dummy.bmp
# End Source File
# Begin Source File

SOURCE=".\icon-install.ico"
# End Source File
# Begin Source File

SOURCE=".\icon-uninstall.ico"
# End Source File
# Begin Source File

SOURCE=.\icon.ico
# End Source File
# End Group
# Begin Group "Setup Specific"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Blob.PBF
# End Source File
# Begin Source File

SOURCE=.\version.ini
# End Source File
# End Group
# End Target
# End Project
