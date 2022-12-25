# Microsoft Developer Studio Project File - Name="ED" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ED - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Ed.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ed.mak" CFG="ED - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ED - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ED - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/ED System", MAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ED - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Od /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Od /I "ROS DLL" /D "_AFXDLL" /D "NDEBUG" /D "MOTION_PROPERTIES" /D "WIN32" /D "_WINDOWS" /D "CODE_MSG" /D DA_ERROR_LEVEL=1 /Yu"pch.h" /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib ROS.lib Wavlib.lib DSound.lib mathlib.lib DACOM.lib deform.lib vfw32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"LIBCD" /nodefaultlib:"LIBC" /libpath:"\Tools\ED System\ROS DLL\Release"

!ELSEIF  "$(CFG)" == "ED - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "." /I "ROS DLL" /D "_AFXDLL" /D "_DEBUG" /D "MOTION_PROPERTIES" /D "WIN32" /D "_WINDOWS" /D "CODE_MSG" /D DA_ERROR_LEVEL=7 /Fr /Yu"PCH.h" /FD /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib ROS.lib Wavlib.lib DSound.lib mathlib.lib DACOM.lib deform.lib vfw32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBC" /nodefaultlib:"LIBCD" /pdbtype:sept /libpath:"\Tools\ED System\ROS DLL\Debug"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "ED - Win32 Release"
# Name "ED - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AmbientLightCustomControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimatedSceneEntityControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimationSettingsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimationToolsController.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimationToolsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraCustomControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\CodeMsg.cpp

!IF  "$(CFG)" == "ED - Win32 Release"

!ELSEIF  "$(CFG)" == "ED - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\controller.cpp
# End Source File
# Begin Source File

SOURCE=.\DAAudioObject.cpp
# End Source File
# Begin Source File

SOURCE=.\DAInstanceTree.cpp
# End Source File
# Begin Source File

SOURCE=.\DBExtension.cpp
# End Source File
# Begin Source File

SOURCE=.\DeformableKeyPropertiesUI.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectionalLightCustomControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgbars.cpp
# End Source File
# Begin Source File

SOURCE=.\ED.cpp
# End Source File
# Begin Source File

SOURCE=.\ED.rc
# End Source File
# Begin Source File

SOURCE=.\EdTimelineDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\edutil.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityBrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityTree.cpp
# End Source File
# Begin Source File

SOURCE=.\GLSceneViewUI.cpp
# End Source File
# Begin Source File

SOURCE=.\GLUtils.cpp

!IF  "$(CFG)" == "ED - Win32 Release"

!ELSEIF  "$(CFG)" == "ED - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\IdleNotification.cpp
# End Source File
# Begin Source File

SOURCE=.\InterpolationKeyProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyPropertiesUI.cpp
# End Source File
# Begin Source File

SOURCE=.\MissingControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\ModuleVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\PCH.cpp

!IF  "$(CFG)" == "ED - Win32 Release"

# ADD CPP /Yc"pch.h"

!ELSEIF  "$(CFG)" == "ED - Win32 Debug"

# ADD CPP /Yc"PCH.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PointLightCustomControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneEntityControls.cpp
# End Source File
# Begin Source File

SOURCE=.\ScenePaletteController.cpp
# End Source File
# Begin Source File

SOURCE=.\ScenePaletteUI.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneView.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectDBEntityDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectEntityPart.cpp
# End Source File
# Begin Source File

SOURCE=.\SpotLightCustomControlsUI.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticSceneEntitySoundListener.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticSceneEntitySoundSource.cpp
# End Source File
# Begin Source File

SOURCE=.\StringType.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtils.cpp

!IF  "$(CFG)" == "ED - Win32 Release"

!ELSEIF  "$(CFG)" == "ED - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TonyTest.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackController.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackViewUI.cpp
# End Source File
# Begin Source File

SOURCE=.\UserPreferences.cpp
# End Source File
# Begin Source File

SOURCE=.\View.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AmbientLightCustomControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\AnimatedSceneEntityControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\AnimationSettingsUI.h
# End Source File
# Begin Source File

SOURCE=.\AnimationToolsController.h
# End Source File
# Begin Source File

SOURCE=.\AnimationToolsUI.h
# End Source File
# Begin Source File

SOURCE=.\ASoundListener.h
# End Source File
# Begin Source File

SOURCE=".\ROS DLL\ASpotLight.h"
# End Source File
# Begin Source File

SOURCE=".\Ros dll\Callback.hpp"
# End Source File
# Begin Source File

SOURCE=.\CameraCustomControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\CodeMsg.h
# End Source File
# Begin Source File

SOURCE=.\Controller.h
# End Source File
# Begin Source File

SOURCE=.\DAAudioObject.h
# End Source File
# Begin Source File

SOURCE=.\DAInstanceTree.h
# End Source File
# Begin Source File

SOURCE=.\DBExtension.h
# End Source File
# Begin Source File

SOURCE=.\DeformableKeyPropertiesUI.h
# End Source File
# Begin Source File

SOURCE=.\DirectionalLightCustomControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\dlgbars.h
# End Source File
# Begin Source File

SOURCE=.\ED.h
# End Source File
# Begin Source File

SOURCE=.\EdTimelineDlg.h
# End Source File
# Begin Source File

SOURCE=.\edutil.h
# End Source File
# Begin Source File

SOURCE=.\EntityBrowser.h
# End Source File
# Begin Source File

SOURCE=.\EntityDB.h
# End Source File
# Begin Source File

SOURCE=.\EntityTree.h
# End Source File
# Begin Source File

SOURCE=.\FloatEditUtil.h
# End Source File
# Begin Source File

SOURCE=.\GLSceneViewUI.h
# End Source File
# Begin Source File

SOURCE=.\GLUtils.h
# End Source File
# Begin Source File

SOURCE=.\IdleNotification.h
# End Source File
# Begin Source File

SOURCE=.\InterpolationKeyProperties.h
# End Source File
# Begin Source File

SOURCE=".\Ros dll\IntersectInfo.h"
# End Source File
# Begin Source File

SOURCE=.\KeyPointOperations.h
# End Source File
# Begin Source File

SOURCE=.\KeyPropertiesUI.h
# End Source File
# Begin Source File

SOURCE=.\MarkerData.h
# End Source File
# Begin Source File

SOURCE=.\MissingControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\ModuleVersion.h
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyProperties.h
# End Source File
# Begin Source File

SOURCE=".\Ros dll\OrientationMemento.h"
# End Source File
# Begin Source File

SOURCE=.\PCH.h
# End Source File
# Begin Source File

SOURCE=.\PointLightCustomControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityControls.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityFormFactory.h
# End Source File
# Begin Source File

SOURCE=.\ScenePaletteController.h
# End Source File
# Begin Source File

SOURCE=.\ScenePaletteUI.h
# End Source File
# Begin Source File

SOURCE=.\SceneView.h
# End Source File
# Begin Source File

SOURCE=.\SelectDBEntityDialog.h
# End Source File
# Begin Source File

SOURCE=.\SelectEntityPart.h
# End Source File
# Begin Source File

SOURCE=.\SpotLightCustomControlsUI.h
# End Source File
# Begin Source File

SOURCE=.\StaticEntityOperations.h
# End Source File
# Begin Source File

SOURCE=.\StaticSceneEntitySoundListener.h
# End Source File
# Begin Source File

SOURCE=.\StaticSceneEntitySoundSource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Stringlist.h
# End Source File
# Begin Source File

SOURCE=.\StringType.h
# End Source File
# Begin Source File

SOURCE=.\StringUtils.h
# End Source File
# Begin Source File

SOURCE=.\Timer.h
# End Source File
# Begin Source File

SOURCE=.\TonyTest.h
# End Source File
# Begin Source File

SOURCE=.\TrackController.h
# End Source File
# Begin Source File

SOURCE=.\TrackData.h
# End Source File
# Begin Source File

SOURCE=.\TrackViewUI.h
# End Source File
# Begin Source File

SOURCE=".\Ros dll\TransformUtil.h"
# End Source File
# Begin Source File

SOURCE=".\Ros dll\Type.h"
# End Source File
# Begin Source File

SOURCE=.\UserPreferences.h
# End Source File
# Begin Source File

SOURCE=.\View.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Res\closed.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ED.ico
# End Source File
# Begin Source File

SOURCE=.\res\ED.rc2
# End Source File
# Begin Source File

SOURCE=.\Res\IKImages.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\open.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
# Section ED : {95C42E40-4273-11D2-85B3-0000F4A24553}
# 	2:21:DefaultSinkHeaderFile:timeline.h
# 	2:16:DefaultSinkClass:CTimeline
# End Section
# Section ED : {E0E7BC6F-4286-11D2-85B3-0000F4A24553}
# 	2:5:Class:CTimeline
# 	2:10:HeaderFile:timeline.h
# 	2:8:ImplFile:timeline.cpp
# End Section
