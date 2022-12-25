# Microsoft Developer Studio Project File - Name="Physics" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Physics - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Physics.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Physics.mak" CFG="Physics - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Physics - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Physics - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Src/EngComps/Physics", ECEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Physics - Win32 Release"

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
# ADD CPP /nologo /G5 /W3 /Zi /O2 /I "..\..\include" /D "DA_HEAP_ENABLED" /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /FAs /YX"windows.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 MathLib.lib dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Physics - Win32 Debug"

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
# ADD CPP /nologo /G5 /ML /W3 /Zi /Od /I "..\..\include" /D "DA_HEAP_ENABLED" /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\\" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 mathlib.lib winmm.lib dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Physics - Win32 Release"
# Name "Physics - Win32 Debug"
# Begin Group "Souce Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\abm.cpp
# End Source File
# Begin Source File

SOURCE=.\algebra.cpp
# End Source File
# Begin Source File

SOURCE=.\collide.cpp
# End Source File
# Begin Source File

SOURCE=.\instance.cpp
# End Source File
# Begin Source File

SOURCE=.\internal.cpp
# End Source File
# Begin Source File

SOURCE=.\kintree.cpp
# End Source File
# Begin Source File

SOURCE=.\pcontact.cpp
# End Source File
# Begin Source File

SOURCE=.\physics.cpp
# End Source File
# Begin Source File

SOURCE=.\pythag.cpp
# End Source File
# Begin Source File

SOURCE=.\rbody.cpp
# End Source File
# Begin Source File

SOURCE=.\rest.cpp
# End Source File
# Begin Source File

SOURCE=.\rk.cpp
# End Source File
# Begin Source File

SOURCE=.\sect.cpp
# End Source File
# Begin Source File

SOURCE=.\spatial.cpp
# End Source File
# Begin Source File

SOURCE=.\svdcmp.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\abm.h
# End Source File
# Begin Source File

SOURCE=.\algebra.h
# End Source File
# Begin Source File

SOURCE=.\arch.h
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.h
# End Source File
# Begin Source File

SOURCE=.\fileutil.h
# End Source File
# Begin Source File

SOURCE=.\friction.h
# End Source File
# Begin Source File

SOURCE=.\instance.h
# End Source File
# Begin Source File

SOURCE=.\joint.h
# End Source File
# Begin Source File

SOURCE=.\kintree.h
# End Source File
# Begin Source File

SOURCE=.\Llist.h
# End Source File
# Begin Source File

SOURCE=.\nrutil.h
# End Source File
# Begin Source File

SOURCE=.\ode.h
# End Source File
# Begin Source File

SOURCE=.\pcontact.h
# End Source File
# Begin Source File

SOURCE=.\rbody.h
# End Source File
# Begin Source File

SOURCE=.\rigid.h
# End Source File
# Begin Source File

SOURCE=.\rk.h
# End Source File
# Begin Source File

SOURCE=.\spatial.h
# End Source File
# Begin Source File

SOURCE=.\stack.h
# End Source File
# Begin Source File

SOURCE=.\vbox.h
# End Source File
# Begin Source File

SOURCE=.\vmesh.h
# End Source File
# End Group
# Begin Group "Shared Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\cmesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\collision.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\connect.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Dacom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\engcomp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\extent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\force.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\geom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\physics.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Stddat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\SysConsumerDesc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\Typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\xform.h
# End Source File
# End Group
# End Target
# End Project
