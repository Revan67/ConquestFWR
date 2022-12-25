# Microsoft Developer Studio Project File - Name="Editor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Editor - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Editor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Editor.mak" CFG="Editor - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Editor - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Editor - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Editor"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Editor - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\Libs\Inclue" /I "..\App\Src\Include" /I "..\App\Src" /I "..\App\DInclude" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Editor - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Libs\Inclue" /I "..\App\Src\Include" /I "..\App\Src" /I "..\App\DInclude" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "EDITOR" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBC" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Editor - Win32 Release"
# Name "Editor - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Sidebars"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AssetBar.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityBar.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\OutputBar.cpp
# End Source File
# Begin Source File

SOURCE=.\scbarcf.cpp
# End Source File
# Begin Source File

SOURCE=.\scbarg.cpp
# End Source File
# Begin Source File

SOURCE=.\SectorBar.cpp
# End Source File
# Begin Source File

SOURCE=.\SideBar.cpp
# End Source File
# Begin Source File

SOURCE=.\sizecbar.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemBar.cpp
# End Source File
# End Group
# Begin Group "DACOM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Cleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\cqpipeline.cpp
# End Source File
# Begin Source File

SOURCE=.\DataList.cpp
# End Source File
# Begin Source File

SOURCE=.\DataList.h
# End Source File
# Begin Source File

SOURCE=.\Editor_DACOM.cpp
# End Source File
# Begin Source File

SOURCE=.\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\App\Src\Search.asm

!IF  "$(CFG)" == "Editor - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=..\App\Src\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Editor - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=..\App\Src\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\App\Src\SuperTrans.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\SysContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\Trace.cpp
# End Source File
# End Group
# Begin Group "Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Camera.cpp
# End Source File
# Begin Source File

SOURCE=.\Campaign.cpp
# End Source File
# Begin Source File

SOURCE=.\Campaign.h
# End Source File
# Begin Source File

SOURCE=.\Hotkey.cpp
# End Source File
# Begin Source File

SOURCE=.\MainLoop.cpp
# End Source File
# Begin Source File

SOURCE=.\Object.cpp
# End Source File
# Begin Source File

SOURCE=.\Object.h
# End Source File
# Begin Source File

SOURCE=.\Scenario.cpp
# End Source File
# Begin Source File

SOURCE=.\Scenario.h
# End Source File
# Begin Source File

SOURCE=.\Sector.cpp
# End Source File
# Begin Source File

SOURCE=.\System.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemStructs.h
# End Source File
# End Group
# Begin Group "Editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ExportImport.cpp
# End Source File
# Begin Source File

SOURCE=.\ExportImport.h
# End Source File
# Begin Source File

SOURCE=.\Mode.h
# End Source File
# Begin Source File

SOURCE=.\ModeCampaign.cpp
# End Source File
# Begin Source File

SOURCE=.\ModeScenario.cpp
# End Source File
# Begin Source File

SOURCE=.\ModeSector.cpp
# End Source File
# Begin Source File

SOURCE=.\ModeStart.cpp
# End Source File
# Begin Source File

SOURCE=.\ModeSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\SaveLoad.h
# End Source File
# Begin Source File

SOURCE=.\undo.h
# End Source File
# End Group
# Begin Group "XML"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tinyxml\tinystr.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxml\tinystr.h
# End Source File
# Begin Source File

SOURCE=.\tinyxml\tinyxml.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxml\tinyxml.h
# End Source File
# Begin Source File

SOURCE=.\tinyxml\tinyxmlerror.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxml\tinyxmlparser.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Editor.rc
# End Source File
# Begin Source File

SOURCE=.\EditorDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorView.cpp
# End Source File
# Begin Source File

SOURCE=.\NewCampaignDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Libs\Include\3dmath.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\AnimTypes.h
# End Source File
# Begin Source File

SOURCE=.\AssetBar.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\BASECAM.H
# End Source File
# Begin Source File

SOURCE=C:\DX90SDK\Include\basetsd.h
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\CQImage.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\Include\CQTrace.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\da_d3dtypes.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\da_heap_utility.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\DACOM.H
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\DAVariant.h
# End Source File
# Begin Source File

SOURCE=..\App\DInclude\DCamera.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\Document.h
# End Source File
# Begin Source File

SOURCE=..\App\DInclude\DSector.h
# End Source File
# Begin Source File

SOURCE=..\App\DInclude\DTypes.h
# End Source File
# Begin Source File

SOURCE=.\Editor.h
# End Source File
# Begin Source File

SOURCE=.\EditorDoc.h
# End Source File
# Begin Source File

SOURCE=.\EditorView.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\ENGINE.H
# End Source File
# Begin Source File

SOURCE=.\EntityBar.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\EventIterator.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\EventSys.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\FDump.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\Filesys.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\FVF.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\gamesys.H
# End Source File
# Begin Source File

SOURCE=..\App\Src\GridVector.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\HeapObj.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\HKEvent.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IAnim.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\ICamera.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\ichannel.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IConnection.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IDispatch.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IDocClient.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IHardPoint.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IProfileParser.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IRenderPrimitive.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\IResource.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\ITextureLibrary.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\IVertexBufferManager.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\JointInfo.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\lightman.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\matrix4.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\MemFile.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\MyVertex.h
# End Source File
# Begin Source File

SOURCE=.\NewCampaignDlg.h
# End Source File
# Begin Source File

SOURCE=.\OutputBar.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\Include\pch.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\PersistMath.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\pixel.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\RPUL\PrimitiveBuilder.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\quat.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\renderer.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\RenderProp.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\rendpipeline.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\Results.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\RPVertex1.h
# End Source File
# Begin Source File

SOURCE=.\scbarg.h
# End Source File
# Begin Source File

SOURCE=.\SectorBar.h
# End Source File
# Begin Source File

SOURCE=.\SideBar.h
# End Source File
# Begin Source File

SOURCE=.\sizecbar.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\Startup.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\Streamer.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\SuperTrans.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\SYSTEM.H
# End Source File
# Begin Source File

SOURCE=.\SystemBar.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\TComponent.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\TempStr.h
# End Source File
# Begin Source File

SOURCE=.\TRect.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\TSmartPointer.h
# End Source File
# Begin Source File

SOURCE=..\App\DInclude\TYPEDEFS.H
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\TYPEDEFS.H
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\vector.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\vector4.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\VertexBufferDesc.h
# End Source File
# Begin Source File

SOURCE=..\App\Src\Include\VFX.H
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\View2d.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\ViewCnst.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\Viewer.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\WindowManager.h
# End Source File
# Begin Source File

SOURCE=..\Libs\Include\xform.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\CLOUD.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Win95\CLSDFOLD.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\EARTH.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Editor.ico
# End Source File
# Begin Source File

SOURCE=.\res\Editor.rc2
# End Source File
# Begin Source File

SOURCE=.\res\EditorDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\EXCLEM.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\FACE01.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\FACE02.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\FACE03.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Misc\FACE05.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\FIRE.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Office\FOLDER03.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\LITENING.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON01.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON02.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON03.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON04.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON05.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON06.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON07.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\MOON08.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\RAIN.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Industry\ROCKET.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\SNOW.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\SUN.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Graphics\Icons\Elements\WATER.ICO
# End Source File
# End Group
# Begin Group "Libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Libs\Static\DACOM.lib
# End Source File
# Begin Source File

SOURCE=..\Libs\Static\MathLib.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\globals.h
# End Source File
# End Target
# End Project
