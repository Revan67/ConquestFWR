# Microsoft Developer Studio Project File - Name="_CQ_FullBuild" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=_CQ_FullBuild - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_CQ_FullBuild.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_CQ_FullBuild.mak" CFG="_CQ_FullBuild - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_CQ_FullBuild - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "_CQ_FullBuild - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "_CQ_FullBuild"
# PROP Scc_LocalPath "."
MTL=midl.exe

!IF  "$(CFG)" == "_CQ_FullBuild - Win32 Release"

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
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying dlls
PostBuild_Cmds=copy_release_dlls.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "_CQ_FullBuild - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "_CQ_FullBuild___Win32_Debug"
# PROP BASE Intermediate_Dir "_CQ_FullBuild___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "_CQ_FullBuild___Win32_Debug"
# PROP Intermediate_Dir "_CQ_FullBuild___Win32_Debug"
# PROP Target_Dir ""
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy_debug_dlls.bat
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "_CQ_FullBuild - Win32 Release"
# Name "_CQ_FullBuild - Win32 Debug"
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
