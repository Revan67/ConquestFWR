# Microsoft Developer Studio Project File - Name="CmpndView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=CmpndView - Win32 MSHeap Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CmpndView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CmpndView.mak" CFG="CmpndView - Win32 MSHeap Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CmpndView - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CmpndView - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "CmpndView - Win32 MSHeap Release" (based on "Win32 (x86) Application")
!MESSAGE "CmpndView - Win32 MSHeap Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/ObjView", BLGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CmpndView - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CmpndView___Win32_Release"
# PROP BASE Intermediate_Dir "CmpndView___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "." /D "NDEBUG" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "WIN32" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "." /I "../Common" /D "DA_HEAP" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /D "DA_HEAP_ENABLED" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ComHeap.lib MathLib.lib DACOM.lib EngOps.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /out:"ReleasePIPE\ObjView.exe" /fixed:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 rpul.lib ComHeap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"Release\ObjView.exe" /pdbtype:con /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "CmpndView - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CmpndView___Win32_Debug"
# PROP BASE Intermediate_Dir "CmpndView___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /ML /W3 /Gm /GX /ZI /Od /I "." /D "_DEBUG" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "WIN32" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /FR /YX /FD /c
# ADD CPP /nologo /ML /W3 /Gm /GX /Zi /Od /I "." /I "../Common" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /D "DA_HEAP_ENABLED" /D "DA_HEAP_PRINT_ENABLED" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ComHeap.lib MathLib.lib DACOM.lib EngOps.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /out:"DebugPIPE\ObjView.exe"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 rpul.lib ComHeap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"Debug\ObjView.exe" /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "CmpndView - Win32 MSHeap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CmpndView___Win32_MSHeap_Release"
# PROP BASE Intermediate_Dir "CmpndView___Win32_MSHeap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSHeap_Release"
# PROP Intermediate_Dir "MSHeap_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "." /I "../Common" /D "NDEBUG" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "WIN32" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /D "DA_HEAP" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /I "." /I "../Common" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /D "DA_HEAP" /D "WIN32" /D "NDEBUG" /D DA_ERROR_LEVEL=3 /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 rpul.lib ComHeap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Release\ObjView.exe" /fixed:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 rpul.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Release\ObjView.exe" /fixed:no
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "CmpndView - Win32 MSHeap Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CmpndView___Win32_MSHeap_Debug"
# PROP BASE Intermediate_Dir "CmpndView___Win32_MSHeap_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSHeap_Debug"
# PROP Intermediate_Dir "MSHeap_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /ML /W3 /Gm /GX /ZI /Od /I "." /I "../Common" /D "_DEBUG" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "WIN32" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MD /W3 /Gm /GX /Zi /Od /I "." /I "../Common" /D "NWO" /D "HARDPOINTS" /D "DRAWEXTENTS" /D "LIGHT" /D "_WINDOWS" /D "ANIMATION" /D "EXPLOSIONS" /D "DRAW_LIGHT" /D "LEGO" /D "WIN32" /D "_DEBUG" /D DA_ERROR_LEVEL=8 /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 rpul.lib ComHeap.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug\ObjView.exe"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 rpul.lib MathLib.lib DACOM.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"Debug\ObjView.exe" /pdbtype:con

!ENDIF 

# Begin Target

# Name "CmpndView - Win32 Release"
# Name "CmpndView - Win32 Debug"
# Name "CmpndView - Win32 MSHeap Release"
# Name "CmpndView - Win32 MSHeap Debug"
# Begin Group "Source files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Animation.cpp
# End Source File
# Begin Source File

SOURCE=.\cmpndview.cpp
# End Source File
# Begin Source File

SOURCE=.\Err.cpp
# End Source File
# Begin Source File

SOURCE=.\Extent.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\GetProfileInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Hardpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\Joint.cpp
# End Source File
# Begin Source File

SOURCE=.\Lights.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\Shapes.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\States.cpp
# End Source File
# Begin Source File

SOURCE=.\WinStuff.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\CmpndView.h
# End Source File
# Begin Source File

SOURCE=.\Lights.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ObjView.rc
# End Source File
# End Group
# End Target
# End Project
