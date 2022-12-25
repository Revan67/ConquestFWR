# Microsoft Developer Studio Project File - Name="ROS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ROS - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ROS.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ROS.mak" CFG="ROS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ROS - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ROS - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/ED System/ROS DLL", GULAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ROS - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /Od /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /Od /D "BUILD_ROS_DLL" /D "AUDIO_SUPPORT" /D "SUPPORT_OLD_SCE_FILE_FORMAT" /D "NDEBUG" /D "MOTION_PROPERTIES" /D "WIN32" /D "_WINDOWS" /D "CODE_MSG" /D DA_ERROR_LEVEL=1 /Yu"pch.h" /FD /Zm200 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 version.lib wavlib.lib dacom.lib mathlib.lib deform.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib iinurbs.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"LIBC" /nodefaultlib:"LIBCD"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy Release ROS.DLL to client folder
PostBuild_Cmds=Copy Release\ROS.dll ..
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ROS - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "BUILD_ROS_DLL" /D "AUDIO_SUPPORT" /D "SUPPORT_OLD_SCE_FILE_FORMAT" /D "_DEBUG" /D "MOTION_PROPERTIES" /D "WIN32" /D "_WINDOWS" /D "CODE_MSG" /D DA_ERROR_LEVEL=7 /FR /Yu"PCH.h" /FD /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib wavlib.lib dacom.lib mathlib.lib deform.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib iinurbs.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy Debug ROS.dll to client folder
PostBuild_Cmds=Copy Debug\ROS.dll ..
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ROS - Win32 Release"
# Name "ROS - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AAudibleSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ACamera.cpp
# End Source File
# Begin Source File

SOURCE=.\ACameraDynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\ACameraState.cpp
# End Source File
# Begin Source File

SOURCE=.\ACompoundSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ACompoundSceneEntityState.cpp
# End Source File
# Begin Source File

SOURCE=.\AController.cpp
# End Source File
# Begin Source File

SOURCE=.\Actor.cpp
# End Source File
# Begin Source File

SOURCE=.\ADynamicCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\ADynamicSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ADynamicSpotLight.cpp
# End Source File
# Begin Source File

SOURCE=.\ADynamicSpotLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\ADynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\ALight.cpp
# End Source File
# Begin Source File

SOURCE=.\ALightState.cpp
# End Source File
# Begin Source File

SOURCE=.\AMarker.cpp
# End Source File
# Begin Source File

SOURCE=.\AmbientLight.cpp
# End Source File
# Begin Source File

SOURCE=.\AngularVelocity.cpp
# End Source File
# Begin Source File

SOURCE=.\AOperation.cpp
# End Source File
# Begin Source File

SOURCE=.\APhysicalState.cpp
# End Source File
# Begin Source File

SOURCE=.\ARole.cpp
# End Source File
# Begin Source File

SOURCE=.\Article.cpp
# End Source File
# Begin Source File

SOURCE=.\ASceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ASpotLight.cpp
# End Source File
# Begin Source File

SOURCE=.\ASpotLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\AStateVariable.cpp
# End Source File
# Begin Source File

SOURCE=.\AStaticSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\AStaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\AView.cpp
# End Source File
# Begin Source File

SOURCE=.\Camera.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraDynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraState.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\char.cpp
# End Source File
# Begin Source File

SOURCE=.\CodeMsg.cpp

!IF  "$(CFG)" == "ROS - Win32 Release"

!ELSEIF  "$(CFG)" == "ROS - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Color.cpp
# End Source File
# Begin Source File

SOURCE=.\CompoundEntityStaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\CompoundSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstAudioStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstCameraStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstDynamicsStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstLightStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstMotionStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstSceneEntityStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstSpotLightStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstStaticsStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\DAAmbientLight.cpp
# End Source File
# Begin Source File

SOURCE=.\DAAmbientLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\DABaseCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\DACamera.cpp
# End Source File
# Begin Source File

SOURCE=.\DACameraDynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\DACompoundObject.cpp
# End Source File
# Begin Source File

SOURCE=.\DADeformableObject.cpp
# End Source File
# Begin Source File

SOURCE=.\DADynamicSpotLight.cpp
# End Source File
# Begin Source File

SOURCE=.\DADynamicSpotLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\DAHardPoints.cpp
# End Source File
# Begin Source File

SOURCE=.\DARenderPipeline.cpp
# End Source File
# Begin Source File

SOURCE=.\DeformableEntityStaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\DeformableSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicSceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicSceneEntityState.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicSpotLight.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicSpotLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicsStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\FlaggedLocation.cpp
# End Source File
# Begin Source File

SOURCE=.\FlaggedOrientation.cpp
# End Source File
# Begin Source File

SOURCE=.\Force.cpp
# End Source File
# Begin Source File

SOURCE=.\GLUtils.cpp

!IF  "$(CFG)" == "ROS - Win32 Release"

!ELSEIF  "$(CFG)" == "ROS - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Hardpoints.cpp
# End Source File
# Begin Source File

SOURCE=.\IKState.cpp
# End Source File
# Begin Source File

SOURCE=.\IStreamWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\LightState.cpp
# End Source File
# Begin Source File

SOURCE=.\LightStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\LinearVelocity.cpp
# End Source File
# Begin Source File

SOURCE=.\Livecamera.cpp
# End Source File
# Begin Source File

SOURCE=.\LiveCameraDynamicsState.cpp
# End Source File
# Begin Source File

SOURCE=.\LiveCameraRole.cpp
# End Source File
# Begin Source File

SOURCE=.\LiveCameraState.cpp
# End Source File
# Begin Source File

SOURCE=.\Location.cpp
# End Source File
# Begin Source File

SOURCE=.\LocationKeyPointMarker.cpp
# End Source File
# Begin Source File

SOURCE=.\LocationKeyPointStaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\LocationRole.cpp
# End Source File
# Begin Source File

SOURCE=.\Marker.cpp
# End Source File
# Begin Source File

SOURCE=.\MatrixUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\ModuleVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\MotionState.cpp
# End Source File
# Begin Source File

SOURCE=.\MotionStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\Observer.cpp
# End Source File
# Begin Source File

SOURCE=.\OperationStack.cpp
# End Source File
# Begin Source File

SOURCE=.\Orientation.cpp
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyPointMarker.cpp
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyPointStaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\OrientationRole.cpp
# End Source File
# Begin Source File

SOURCE=.\ParentRole.cpp
# End Source File
# Begin Source File

SOURCE=.\ParentState.cpp
# End Source File
# Begin Source File

SOURCE=.\PCH.cpp

!IF  "$(CFG)" == "ROS - Win32 Release"

# ADD CPP /Yc"pch.h"

!ELSEIF  "$(CFG)" == "ROS - Win32 Debug"

# ADD CPP /Yc"PCH.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Position.cpp
# End Source File
# Begin Source File

SOURCE=.\PositionMarker.cpp
# End Source File
# Begin Source File

SOURCE=.\RayMeshCollision.cpp
# End Source File
# Begin Source File

SOURCE=.\Role.cpp
# End Source File
# Begin Source File

SOURCE=.\ROS.cpp
# End Source File
# Begin Source File

SOURCE=.\ROS.rc
# End Source File
# Begin Source File

SOURCE=.\rosmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\ROSSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\Scene.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneController.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneEntityList.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneEntityState.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneEntityStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneModel.cpp
# End Source File
# Begin Source File

SOURCE=.\SingularStateVariable.cpp
# End Source File
# Begin Source File

SOURCE=.\spline.cpp
# End Source File
# Begin Source File

SOURCE=.\SpotLightState.cpp
# End Source File
# Begin Source File

SOURCE=.\SpotLightStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticsState.cpp
# End Source File
# Begin Source File

SOURCE=.\StaticsStateAccessor.cpp
# End Source File
# Begin Source File

SOURCE=.\StringList.cpp
# End Source File
# Begin Source File

SOURCE=.\StringType.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtils.cpp

!IF  "$(CFG)" == "ROS - Win32 Release"

!ELSEIF  "$(CFG)" == "ROS - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\timetag.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeType.cpp
# End Source File
# Begin Source File

SOURCE=.\Torque.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AAudibleSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\ACamera.h
# End Source File
# Begin Source File

SOURCE=.\ACameraDynamicsState.h
# End Source File
# Begin Source File

SOURCE=.\ACameraState.h
# End Source File
# Begin Source File

SOURCE=.\ACompoundSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\ACompoundSceneEntityState.h
# End Source File
# Begin Source File

SOURCE=.\AController.h
# End Source File
# Begin Source File

SOURCE=.\Actor.h
# End Source File
# Begin Source File

SOURCE=.\ADynamicCamera.h
# End Source File
# Begin Source File

SOURCE=.\ADynamicSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\ADynamicSpotLight.h
# End Source File
# Begin Source File

SOURCE=.\ADynamicSpotLightState.h
# End Source File
# Begin Source File

SOURCE=.\ADynamicsState.h
# End Source File
# Begin Source File

SOURCE=.\AEventListener.h
# End Source File
# Begin Source File

SOURCE=.\AEventSource.h
# End Source File
# Begin Source File

SOURCE=.\ALight.h
# End Source File
# Begin Source File

SOURCE=.\ALightState.h
# End Source File
# Begin Source File

SOURCE=.\AMarker.h
# End Source File
# Begin Source File

SOURCE=.\AmbientLight.h
# End Source File
# Begin Source File

SOURCE=.\AngularVelocity.h
# End Source File
# Begin Source File

SOURCE=.\AOperation.h
# End Source File
# Begin Source File

SOURCE=.\APhysicalState.h
# End Source File
# Begin Source File

SOURCE=.\ARole.h
# End Source File
# Begin Source File

SOURCE=.\Article.h
# End Source File
# Begin Source File

SOURCE=.\ASceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\ASceneEntityEventListener.h
# End Source File
# Begin Source File

SOURCE=.\ASceneEntityEventSource.h
# End Source File
# Begin Source File

SOURCE=.\ASpotLight.h
# End Source File
# Begin Source File

SOURCE=.\ASpotLightState.h
# End Source File
# Begin Source File

SOURCE=.\AStateVariable.h
# End Source File
# Begin Source File

SOURCE=.\AStaticSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\AStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\AudioRole.h
# End Source File
# Begin Source File

SOURCE=.\AudioState.h
# End Source File
# Begin Source File

SOURCE=.\AudioStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\AView.h
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\CameraDynamicsState.h
# End Source File
# Begin Source File

SOURCE=.\CameraRole.h
# End Source File
# Begin Source File

SOURCE=.\CameraState.h
# End Source File
# Begin Source File

SOURCE=.\CameraStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\Char.h
# End Source File
# Begin Source File

SOURCE=.\CodeMsg.h
# End Source File
# Begin Source File

SOURCE=.\Color.h
# End Source File
# Begin Source File

SOURCE=.\CompoundEntityStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\CompoundSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\CompoundStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\CompoundStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\ConstAudioStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstCameraStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstCompoundStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstDynamicsStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstLightStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstMotionStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstSceneEntityStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstSpotLightStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\ConstStaticsStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\DAAmbientLight.h
# End Source File
# Begin Source File

SOURCE=.\DAAmbientLightState.h
# End Source File
# Begin Source File

SOURCE=.\DABaseCamera.h
# End Source File
# Begin Source File

SOURCE=.\DACamera.h
# End Source File
# Begin Source File

SOURCE=.\DACameraDynamicsState.h
# End Source File
# Begin Source File

SOURCE=.\DACompoundObject.h
# End Source File
# Begin Source File

SOURCE=.\DADeformableObject.h
# End Source File
# Begin Source File

SOURCE=.\DADynamicSpotLight.h
# End Source File
# Begin Source File

SOURCE=.\DADynamicSpotLightState.h
# End Source File
# Begin Source File

SOURCE=.\DAMatrixUtil.h
# End Source File
# Begin Source File

SOURCE=.\DARenderPipeline.h
# End Source File
# Begin Source File

SOURCE=.\DeformableEntityStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\DeformableSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\drivermgr.h
# End Source File
# Begin Source File

SOURCE=.\DynamicSceneEntity.h
# End Source File
# Begin Source File

SOURCE=.\DynamicSceneEntityState.h
# End Source File
# Begin Source File

SOURCE=.\DynamicSpotLight.h
# End Source File
# Begin Source File

SOURCE=.\DynamicSpotLightState.h
# End Source File
# Begin Source File

SOURCE=.\DynamicsRole.h
# End Source File
# Begin Source File

SOURCE=.\dynamicsstate.h
# End Source File
# Begin Source File

SOURCE=.\DynamicsStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\EntityDescription.h
# End Source File
# Begin Source File

SOURCE=.\Environment.h
# End Source File
# Begin Source File

SOURCE=.\EventListenerTracker.h
# End Source File
# Begin Source File

SOURCE=.\EventSourceTracker.h
# End Source File
# Begin Source File

SOURCE=.\FlaggedLocation.h
# End Source File
# Begin Source File

SOURCE=.\FlaggedOrientation.h
# End Source File
# Begin Source File

SOURCE=.\Force.h
# End Source File
# Begin Source File

SOURCE=.\glu.h
# End Source File
# Begin Source File

SOURCE=.\gluP.h
# End Source File
# Begin Source File

SOURCE=.\GLUtils.h
# End Source File
# Begin Source File

SOURCE=.\HardPoints.h
# End Source File
# Begin Source File

SOURCE=.\IKState.h
# End Source File
# Begin Source File

SOURCE=.\InfoRole.h
# End Source File
# Begin Source File

SOURCE=.\IntersectInfo.h
# End Source File
# Begin Source File

SOURCE=.\IStreamWiz.h
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\LightRole.h
# End Source File
# Begin Source File

SOURCE=.\LightState.h
# End Source File
# Begin Source File

SOURCE=.\LightStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\LinearVelocity.h
# End Source File
# Begin Source File

SOURCE=.\Links.h
# End Source File
# Begin Source File

SOURCE=.\LiveCamera.h
# End Source File
# Begin Source File

SOURCE=.\LiveCameraDynamicsState.h
# End Source File
# Begin Source File

SOURCE=.\LiveCameraRole.h
# End Source File
# Begin Source File

SOURCE=.\LiveCameraState.h
# End Source File
# Begin Source File

SOURCE=.\Location.h
# End Source File
# Begin Source File

SOURCE=.\LocationKeyPointMarker.h
# End Source File
# Begin Source File

SOURCE=.\LocationKeyPointStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\LocationMemento.h
# End Source File
# Begin Source File

SOURCE=.\LocationRole.h
# End Source File
# Begin Source File

SOURCE=.\ManualPtr.h
# End Source File
# Begin Source File

SOURCE=.\Marker.h
# End Source File
# Begin Source File

SOURCE=.\Matrix4x4.h
# End Source File
# Begin Source File

SOURCE=.\MatrixUtil.h
# End Source File
# Begin Source File

SOURCE=.\Misc.h
# End Source File
# Begin Source File

SOURCE=.\modelns.h
# End Source File
# Begin Source File

SOURCE=.\ModuleVersion.h
# End Source File
# Begin Source File

SOURCE=.\MotionRole.h
# End Source File
# Begin Source File

SOURCE=.\MotionState.h
# End Source File
# Begin Source File

SOURCE=.\MotionStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\Observer.h
# End Source File
# Begin Source File

SOURCE=.\OperationStack.h
# End Source File
# Begin Source File

SOURCE=.\Orientation.h
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyPointMarker.h
# End Source File
# Begin Source File

SOURCE=.\OrientationKeyPointStaticsState.h
# End Source File
# Begin Source File

SOURCE=.\OrientationMemento.h
# End Source File
# Begin Source File

SOURCE=.\OrientationRole.h
# End Source File
# Begin Source File

SOURCE=.\OStreamWiz.h
# End Source File
# Begin Source File

SOURCE=.\PCH.h
# End Source File
# Begin Source File

SOURCE=.\Position.h
# End Source File
# Begin Source File

SOURCE=.\PositionMarker.h
# End Source File
# Begin Source File

SOURCE=.\RayMeshCollision.h
# End Source File
# Begin Source File

SOURCE=.\Remapper.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Role.h
# End Source File
# Begin Source File

SOURCE=.\ROSDLL.h
# End Source File
# Begin Source File

SOURCE=.\rosmodel.h
# End Source File
# Begin Source File

SOURCE=.\ROSSystem.h
# End Source File
# Begin Source File

SOURCE=.\Scene.h
# End Source File
# Begin Source File

SOURCE=.\SceneController.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityEvent.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityList.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityRemapper.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityState.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntityStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\SceneEvent.h
# End Source File
# Begin Source File

SOURCE=.\SceneModel.h
# End Source File
# Begin Source File

SOURCE=.\SceneState.h
# End Source File
# Begin Source File

SOURCE=.\SequenceGenerator.h
# End Source File
# Begin Source File

SOURCE=.\SingularStateVariable.h
# End Source File
# Begin Source File

SOURCE=.\spline.h
# End Source File
# Begin Source File

SOURCE=.\SpotLightRole.h
# End Source File
# Begin Source File

SOURCE=.\SpotLightState.h
# End Source File
# Begin Source File

SOURCE=.\SpotLightStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\StateRole.h
# End Source File
# Begin Source File

SOURCE=.\StaticsState.h
# End Source File
# Begin Source File

SOURCE=.\StaticsStateAccessor.h
# End Source File
# Begin Source File

SOURCE=.\StreamWiz.h
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

SOURCE=.\timetag.h
# End Source File
# Begin Source File

SOURCE=.\TimeType.h
# End Source File
# Begin Source File

SOURCE=.\Torque.h
# End Source File
# Begin Source File

SOURCE=.\TransformUtil.h
# End Source File
# Begin Source File

SOURCE=.\TrigUtil.h
# End Source File
# Begin Source File

SOURCE=.\Update.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# End Group
# End Target
# End Project
