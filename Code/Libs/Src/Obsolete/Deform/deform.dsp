# Microsoft Developer Studio Project File - Name="deform" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=deform - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "deform.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "deform.mak" CFG="deform - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "deform - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "deform - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/deform", LNOAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "deform - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "\Develop\Projects\Libs\Include" /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D USE_NWO=1 /D DA_ERROR_LEVEL=1 /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "deform - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /ML /W3 /GX /Zi /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D USE_NWO=1 /D DA_ERROR_LEVEL=8 /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "deform - Win32 Release"
# Name "deform - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\AnimTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\channeltypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\cmesh.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\collision.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Cont_lod.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\deform.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\display.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\edgeprop.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Engine.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\EventIterator.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\extent.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\facegroup.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\FaceProp.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\geom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IAnim.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\ICamera.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\ichannel.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IDispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IDumpText.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IHardPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\ITXMLib.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\lightman.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\material.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Mesh.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\model.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\physics.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\pixel.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\PrimitiveBuilder.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\PrimitiveBuilder_inl.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\rendpipeline.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\RPUL.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\RPUL_Misc.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Stddat.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\StopWatch.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\StopWatch_inl.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\Include\System.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\Text.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\uvchannel.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Rpul\VisualSnooper.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\xform.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\def_patch.cpp
# End Source File
# Begin Source File

SOURCE=.\defdyn.cpp
# End Source File
# Begin Source File

SOURCE=.\defmesh.cpp
# End Source File
# Begin Source File

SOURCE=.\defmesh.h
# End Source File
# Begin Source File

SOURCE=.\deform.cpp
# End Source File
# Begin Source File

SOURCE=.\deformIK.cpp
# End Source File
# Begin Source File

SOURCE=.\eng.h
# End Source File
# Begin Source File

SOURCE=.\eulerangles.cpp
# End Source File
# Begin Source File

SOURCE=.\EulerAngles.h
# End Source File
# Begin Source File

SOURCE=.\ikinfo.h
# End Source File
# Begin Source File

SOURCE=.\mat.h
# End Source File
# Begin Source File

SOURCE=.\svdcmp.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\warning.h
# End Source File
# End Target
# End Project
