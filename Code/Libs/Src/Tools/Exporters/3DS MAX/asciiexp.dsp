# Microsoft Developer Studio Project File - Name="asciiexp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=asciiexp - Win32 Hybrid_DAMESH
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "asciiexp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "asciiexp.mak" CFG="asciiexp - Win32 Hybrid_DAMESH"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "asciiexp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Hybrid" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Hybrid25" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Release25" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Hybrid_DAMESH" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asciiexp - Win32 Release_DAMESH" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/Exporters/MAX", BFLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "asciiexp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /I "..\..\..\include" /I "..\common" /I "c:\3dsmax6\maxsdk\include" /I "c:\3dsmax6\maxsdk\cssdk\include" /D "NDEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /pdb:none /map /debug /machine:I386 /out:"C:\3dsmax6\plugins\3dbexp.DLE" /libpath:"c:\3dsmax6\maxsdk\lib" /fixed:no
# SUBTRACT LINK32 /profile /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /I "..\common" /I "c:\3dsmax6\maxsdk\include" /I "c:\3dsmax6\maxsdk\cssdk\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /map /debug /machine:I386 /out:"C:\3dsmax6\plugins\3dbexp.DLE" /libpath:"C:\3dsmax6\maxsdk\lib"
# SUBTRACT LINK32 /profile /incremental:no /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Hybrid"
# PROP BASE Intermediate_Dir ".\Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Hybrid"
# PROP Intermediate_Dir ".\Hybrid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MDd /GR /GX /Zi /Od /I "Z:\CQ2\Code\Libs\Src\Include" /I "C:\3dsmax6\maxsdk\include" /I "..\common" /I "C:\3dsmax6\maxsdk\cssdk\include" /D "_DEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /FAs /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 MSVCRTD.LIB MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"C:\3dsmax6\plugins\3dbexp.DLE" /libpath:"C:\3dsmax6\maxsdk\lib"
# SUBTRACT LINK32 /verbose /profile /map /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid25"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Hybrid25"
# PROP BASE Intermediate_Dir "Hybrid25"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Hybrid25"
# PROP Intermediate_Dir ".\Hybrid25"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MDd /W3 /GR /GX /Zi /Od /I "d:\libs\include" /I "f:\3dsmax2.5\maxsdk\include" /I "h:\mikes\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "MAX2" /FAs /Fr /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /MDd /W3 /GR /GX /Zi /Od /I "d:\libs\include" /I "f:\3dsmax2.5\maxsdk\include" /I "..\common" /D "_DEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /FAs /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB MSVCRTD.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib /force /out:"f:\3dsmax2.5\plugins\3dbexp25.DLE" /libpath:"f:\3dsmax2.5\maxsdk\lib"
# SUBTRACT BASE LINK32 /verbose /profile /map
# ADD LINK32 MSVCRTD.LIB UTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"f:\3dsmax2.5\plugins\3dbexp25.DLE" /libpath:"f:\3dsmax2.5\maxsdk\lib"
# SUBTRACT LINK32 /verbose /profile /pdb:none /map /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release25"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release25"
# PROP BASE Intermediate_Dir "Release25"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release25"
# PROP Intermediate_Dir ".\Release25"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /Zd /O2 /I "..\..\..\include" /I "f:\3dsmax2.5\maxsdk\include" /I "h:\mikes\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MAX2" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MD /W3 /GX /Zd /O2 /I "..\..\..\include" /I "f:\3dsmax2.5\maxsdk\include" /I "..\common" /D "NDEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /pdb:none /machine:I386 /force /out:"f:\3dsmax2.5\plugins\3dbexp25.DLE" /libpath:"f:\3dsmax2.5\maxsdk\lib" /fixed:no
# SUBTRACT BASE LINK32 /profile /map /debug
# ADD LINK32 UTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /pdb:none /machine:I386 /out:"f:\3dsmax2.5\plugins\3dbexp25.DLE" /libpath:"f:\3dsmax2.5\maxsdk\lib" /fixed:no
# SUBTRACT LINK32 /profile /map /debug /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid_DAMESH"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "asciiexp___Win32_Hybrid_DAMESH"
# PROP BASE Intermediate_Dir "asciiexp___Win32_Hybrid_DAMESH"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "asciiexp___Win32_Hybrid_DAMESH"
# PROP Intermediate_Dir "asciiexp___Win32_Hybrid_DAMESH"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MDd /W3 /GR /GX /Zi /Od /I "d:\libs\include" /I "f:\3dsmax3_1\maxsdk\include" /I "h:\mikes\common" /D "_DEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /FAs /Fr /YX /FD /c
# ADD CPP /nologo /G6 /MDd /w /W0 /GR /GX /Zi /Od /I "Z:\CQ2\Code\Libs\Src\Include" /I "C:\3dsmax6\maxsdk\include" /I "..\common" /I "C:\3dsmax6\maxsdk\cssdk\include" /D "_DEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /D USE_DA_MESH=1 /FAs /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d USE_DA_MESH=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 MSVCRTD.LIB MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"f:\3dsmax3_1\plugins\3dbexp.DLE" /libpath:"f:\3dsmax3_1\maxsdk\lib"
# SUBTRACT BASE LINK32 /verbose /profile /map /force
# ADD LINK32 MSVCRTD.LIB MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"C:\3dsmax6\plugins\3dbexp_new.DLE" /libpath:"C:\3dsmax6\maxsdk\lib"
# SUBTRACT LINK32 /verbose /profile /map /force

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release_DAMESH"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "asciiexp___Win32_Release_DAMESH0"
# PROP BASE Intermediate_Dir "asciiexp___Win32_Release_DAMESH0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "asciiexp___Win32_Release_DAMESH0"
# PROP Intermediate_Dir "asciiexp___Win32_Release_DAMESH0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /I "..\..\..\include" /I "f:\3dsmax3_1\maxsdk\include" /I "h:\mikes\common" /D "NDEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /I "..\..\..\include" /I "f:\3dsmax3_1\maxsdk\include" /I "..\common" /D "NDEBUG" /D "MAX2" /D "WIN32" /D "_WINDOWS" /D "EXPORT_3DB" /D USE_DA_MESH=1 /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /pdb:none /machine:I386 /out:"f:\3dsmax3_1\plugins\3dbexp.DLE" /libpath:"f:\3dsmax3_1\maxsdk\lib" /fixed:no
# SUBTRACT BASE LINK32 /profile /map /debug /force
# ADD LINK32 MAXUTIL.LIB EDMODEL.LIB CORE.LIB BMM.LIB MESH.LIB MNMATH.LIB GEOM.LIB KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB ODBC32.LIB ODBCCP32.LIB COMCTL32.LIB /nologo /base:"0X02C60000" /subsystem:windows /dll /pdb:none /machine:I386 /out:"f:\3dsmax3_1\plugins\3dbexp_new.DLE" /libpath:"f:\3dsmax3_1\maxsdk\lib" /fixed:no
# SUBTRACT LINK32 /profile /map /debug /force

!ENDIF 

# Begin Target

# Name "asciiexp - Win32 Release"
# Name "asciiexp - Win32 Debug"
# Name "asciiexp - Win32 Hybrid"
# Name "asciiexp - Win32 Hybrid25"
# Name "asciiexp - Win32 Release25"
# Name "asciiexp - Win32 Hybrid_DAMESH"
# Name "asciiexp - Win32 Release_DAMESH"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\Common\3db.cpp
# End Source File
# Begin Source File

SOURCE=.\asciiexp.cpp

!IF  "$(CFG)" == "asciiexp - Win32 Release"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Debug"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid25"

# ADD BASE CPP /W3
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release25"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid_DAMESH"

# ADD BASE CPP /W3
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release_DAMESH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\asciiexp.def
# End Source File
# Begin Source File

SOURCE=..\Common\CMESH.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\CMP.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\CYLINDER.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\DA_MESH.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\eigen.cpp
# End Source File
# Begin Source File

SOURCE=.\export.cpp

!IF  "$(CFG)" == "asciiexp - Win32 Release"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Debug"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid25"

# ADD BASE CPP /W3
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release25"

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Hybrid_DAMESH"

# ADD BASE CPP /W3
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "asciiexp - Win32 Release_DAMESH"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\extents.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\imagefilter.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\matrix4.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\minbox3.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\minsphr.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\MISC.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\mtl_txt.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\RayPolygon.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\rgbutils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\SORTQ.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\UTF.CPP
# End Source File
# Begin Source File

SOURCE=..\Common\volint.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\Common\3DB.H
# End Source File
# Begin Source File

SOURCE=.\asciiexp.h
# End Source File
# Begin Source File

SOURCE=.\ASCIITOK.H
# End Source File
# Begin Source File

SOURCE=..\Common\CHANNEL.H
# End Source File
# Begin Source File

SOURCE=..\Common\CMESH.H
# End Source File
# Begin Source File

SOURCE=..\Common\CMP.H
# End Source File
# Begin Source File

SOURCE=..\Common\CYLINDER.H
# End Source File
# Begin Source File

SOURCE=..\Common\da_mesh.h
# End Source File
# Begin Source File

SOURCE=..\Common\eigen.h
# End Source File
# Begin Source File

SOURCE=..\Common\EXTENTS.H
# End Source File
# Begin Source File

SOURCE=..\Common\imagefilter.h
# End Source File
# Begin Source File

SOURCE=.\lod.h
# End Source File
# Begin Source File

SOURCE=..\Common\minsphr.h
# End Source File
# Begin Source File

SOURCE=..\Common\misc.h
# End Source File
# Begin Source File

SOURCE=..\Common\mtl_txt.h
# End Source File
# Begin Source File

SOURCE=..\Common\names.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\Common\rgbutils.h
# End Source File
# Begin Source File

SOURCE=..\Common\SGI_UTF.H
# End Source File
# Begin Source File

SOURCE=..\Common\VOLINT.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\asciiexp.rc
# End Source File
# End Group
# Begin Group "Extern Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\acolor.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\actiontable.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\animtbl.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\appio.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\assert1.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\baseinterface.h
# End Source File
# Begin Source File

SOURCE=C:\DX90SDK\Include\basetsd.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\cssdk\include\BIPEXP.H
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\bitarray.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\bitmap.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\bmmlib.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\box2.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\box3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\buildnumber.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\buildver.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\captypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\ChannelEventTypes.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\channels.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\cmdmode.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\color.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\CONT_LOD.H
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\coreexp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\custcont.h
# End Source File
# Begin Source File

SOURCE=..\Common\daprop.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\dbgprint.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\decomp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\DefaultActions.H
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\dpoint3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\euler.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\evuser.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\excllist.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\export.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\FaceProp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gbuf.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gencam.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\genhier.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\genlight.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\genshape.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\geom.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\geomlib.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gfloat.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gfx.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gfxlib.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\gutil.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\hitdata.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\hold.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\icolorman.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ifnpub.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ILayerProperties.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\impapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\impexp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\impexpintf.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\imtl.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\inode.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\INodeGIProperties.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\INodeLayerProperties.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\interpik.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\interval.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ioapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\iparamb.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\iparamb2.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\iparamm.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ipipelineclient.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ipoint2.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ipoint3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\iRenderPresets.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\istdplug.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\iTargetedIO.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\itreevw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\libver.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\linklist.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\lockid.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\log.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\matrix2.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\matrix3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\matrix4.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\max.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\maxapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\maxcom.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\maxtess.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\maxtypes.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\maxversion.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mesh.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\meshlib.h
# End Source File
# Begin Source File

SOURCE=..\Common\minbox3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mnbigmat.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mncommon.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mnmath.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mnmesh.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\MNNormalSpec.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\modstack.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mouseman.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\mtl.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\namesel.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\nametab.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\notetrck.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\object.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\objmode.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\palutil.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\paramtype.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\patch.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\patchlib.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\patchobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\persistanim.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\PersistChannel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\PERSISTCOMPOUND.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\PERSISTHARDPOINT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\PersistMisc.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\cssdk\include\Phyexp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\plugapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\plugin.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\point2.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\point3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\point4.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\polyobj.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\polyshp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ptrvec.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\quat.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\random.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\ref.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\render.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\renderelements.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\rtclick.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\sbmtlapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\sceneapi.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\sfx.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\shape.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\shphier.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\shpsels.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\snap.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\soundobj.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\spline3d.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\stack.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\stack3.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\stdmat.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\strbasic.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\strclass.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\surf_api.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\svcore.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\systemutilities.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\tab.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\templt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\TextureCoord.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\trig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\trimeshlod.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\triobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\TYPEDEFS.H
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\udmia64.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\units.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\utilexp.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\utilintf.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\utillib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\vector4.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\vedge.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\winutil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\xform.h
# End Source File
# Begin Source File

SOURCE=C:\3dsmax6\maxsdk\include\xtcobject.h
# End Source File
# End Group
# End Target
# End Project
