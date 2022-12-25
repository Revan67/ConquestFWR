# Microsoft Developer Studio Project File - Name="phyedit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=phyedit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "phyedit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "phyedit.mak" CFG="phyedit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "phyedit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "phyedit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/Physics Editor", IIAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "phyedit - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /Zi /O2 /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comctl32.lib dacom.lib mathlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "phyedit - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib dacom.lib mathlib.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /nodefaultlib:"libc" /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "phyedit - Win32 Release"
# Name "phyedit - Win32 Debug"
# Begin Group "MinimalGeometry"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\chull3d.cpp
# End Source File
# Begin Source File

SOURCE=.\chull3d.h
# End Source File
# Begin Source File

SOURCE=.\eigen.cpp
# End Source File
# Begin Source File

SOURCE=.\eigen.h
# End Source File
# Begin Source File

SOURCE=.\minbox.cpp
# End Source File
# Begin Source File

SOURCE=.\mingeom.h
# End Source File
# Begin Source File

SOURCE=.\minsphere.cpp
# End Source File
# Begin Source File

SOURCE=.\mintube.cpp
# End Source File
# End Group
# Begin Group "Extent Editing"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\box_edit.cpp
# End Source File
# Begin Source File

SOURCE=.\convex_edit.cpp
# End Source File
# Begin Source File

SOURCE=.\editmode.h
# End Source File
# Begin Source File

SOURCE=.\sphere_edit.cpp
# End Source File
# Begin Source File

SOURCE=.\tube_edit.cpp
# End Source File
# End Group
# Begin Group "Support"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fileutil.cpp
# End Source File
# Begin Source File

SOURCE=.\fileutil.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# End Group
# Begin Group "Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cmesh.cpp
# End Source File
# Begin Source File

SOURCE=.\dup.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ctest.h
# End Source File
# Begin Source File

SOURCE=.\cvisualize.h
# End Source File
# Begin Source File

SOURCE=.\extutil.cpp
# End Source File
# Begin Source File

SOURCE=.\extutil.h
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\OpenGL.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenGL.h
# End Source File
# Begin Source File

SOURCE=.\phyedit.cpp
# End Source File
# Begin Source File

SOURCE=.\phyedit.h
# End Source File
# Begin Source File

SOURCE=.\phyedit.rc
# End Source File
# Begin Source File

SOURCE=.\primitive.cpp
# End Source File
# Begin Source File

SOURCE=.\primitive.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\submesh.cpp
# End Source File
# Begin Source File

SOURCE=.\submesh.h
# End Source File
# Begin Source File

SOURCE=.\treebar.cpp
# End Source File
# Begin Source File

SOURCE=.\treebar.h
# End Source File
# End Target
# End Project
