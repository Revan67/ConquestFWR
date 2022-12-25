# Microsoft Developer Studio Project File - Name="lualib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lualib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lualib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lualib.mak" CFG="lualib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lualib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lualib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Libs/Dev/Src/lualib", ASCAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lualib - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "lua\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "lualib - Win32 Debug"

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
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "lua\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
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

# Name "lualib - Win32 Release"
# Name "lualib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.c;*.cpp"
# Begin Source File

SOURCE=.\Lua\Src\Lapi.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lauxlib.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lbuffer.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lbuiltin.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ldo.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lfunc.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lgc.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Llex.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lmem.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lobject.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lparser.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lstate.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lstring.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ltable.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ltm.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lundump.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lvm.c
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lzio.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\Lua\Src\Lapi.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lbuiltin.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ldo.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lfunc.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lgc.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Llex.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lmem.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lobject.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lopcodes.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lparser.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lstate.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lstring.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ltable.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Ltm.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lundump.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lvm.h
# End Source File
# Begin Source File

SOURCE=.\Lua\Src\Lzio.h
# End Source File
# End Group
# End Target
# End Project
