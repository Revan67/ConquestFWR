# Microsoft Developer Studio Project File - Name="EngOps" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=EngOps - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EngOps.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EngOps.mak" CFG="EngOps - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EngOps - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "EngOps - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/EngOps", VKLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EngOps - Win32 Release"

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
# ADD CPP /nologo /G6 /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=1 /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "EngOps - Win32 Debug"

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
# ADD CPP /nologo /G6 /ML /W3 /Z7 /Od /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D DA_ERROR_LEVEL=8 /FD /c
# SUBTRACT CPP /YX
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

# Name "EngOps - Win32 Release"
# Name "EngOps - Win32 Debug"
# Begin Group "Source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\CntObjs.cpp
# End Source File
# Begin Source File

SOURCE=.\explode.cpp
# End Source File
# Begin Source File

SOURCE=.\FindChild.cpp
# End Source File
# Begin Source File

SOURCE=.\FindHP.cpp
# End Source File
# Begin Source File

SOURCE=.\FindJoint.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\..\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=.\ArchHolder.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\cmesh.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\collision.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Engine.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\EngOps.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\explode.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\extent.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\geom.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\IHardPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\matrix.h
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

SOURCE=..\..\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\rbdesc.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\TSmartPointer.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\Vfx.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\xform.h
# End Source File
# End Group
# End Target
# End Project
