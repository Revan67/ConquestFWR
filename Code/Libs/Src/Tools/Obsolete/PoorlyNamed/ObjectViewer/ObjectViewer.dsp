# Microsoft Developer Studio Project File - Name="ObjectViewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ObjectViewer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ObjectViewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ObjectViewer.mak" CFG="ObjectViewer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ObjectViewer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ObjectViewer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ObjectViewer - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I "..\CDALibs_Common\Include" /I "..\CDALibs_Common\Interfaces" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 daguid.lib RPUL.lib dacom.lib mathlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "ObjectViewer - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I ".\\" /I "..\CDALibs_Common\Include" /I "..\CDALibs_Common\Interfaces" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /c
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
# ADD LINK32 winmm.lib amstrmid.lib dxguid.lib rpul.lib dacom.lib mathlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /nodefaultlib:"libc"

!ENDIF 

# Begin Target

# Name "ObjectViewer - Win32 Release"
# Name "ObjectViewer - Win32 Debug"
# Begin Group "External Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IDACOMEngineInstance.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IDecorator.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IGeoTransformable.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\ILightSource.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\ILowLevelCamera.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\INamedProperty.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IObjectDatabase.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IPersistable.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IPersistableOptions.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IPhysicalCharacteristics.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IRenderable.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\ISceneCamera.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\ISimulatable.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Interfaces\IWindow.h
# End Source File
# End Group
# Begin Group "External Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CDALibs_Common\Src\DACOMProvider.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_3DWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_BasicSceneDatabase.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_Camera.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_Decorator.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_GammaTestWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_Light.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_Renderable.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_TextureLibraryTestWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_TextureTestWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Components\DALibs_TextureWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Src\SymbolTable.cpp
# End Source File
# End Group
# Begin Group "External Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CDALibs_Common\Include\CommonControls.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\CommonDialog.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\DACOMProvider.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\DPF.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\FrameTracker.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\MessageCrackers.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\SymbolTable.h
# End Source File
# Begin Source File

SOURCE=..\CDALibs_Common\Include\TCollection.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\DACOM.ini
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\ObjectViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectViewer.rc
# End Source File
# End Target
# End Project
