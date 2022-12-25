# Microsoft Developer Studio Project File - Name="Conquest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Conquest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Conquest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Conquest.mak" CFG="Conquest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Conquest - Win32 Final" (based on "Win32 (x86) Application")
!MESSAGE "Conquest - Win32 DEMO Final" (based on "Win32 (x86) Application")
!MESSAGE "Conquest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Conquest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Conquest - Win32 DEMO Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Conquest/Src", DFDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Conquest - Win32 Final"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Final"
# PROP BASE Intermediate_Dir "Final"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Final"
# PROP Intermediate_Dir "Final"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /Yu"pch.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 Final\Mission.lib Final\Globals.lib Final\Trim.lib Final\ZBatcher.lib DXGuid.lib MathLib.lib COMCTL32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3dx.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none /incremental:yes /map /debug

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Final"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DemoFinal"
# PROP BASE Intermediate_Dir "DemoFinal"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DemoFinal"
# PROP Intermediate_Dir "DemoFinal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Ox /Ot /Oa /Ow /Og /Oi /Gf /I "include" /I "..\DInclude" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FINAL_RELEASE" /D "_DEMO_" /Yu"pch.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 DemoFinal\Mission.lib DemoFinal\Globals.lib DemoFinal\Trim.lib DemoFinal\ZBatcher.lib DXGuid.lib MathLib.lib COMCTL32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3dx.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none /incremental:yes /map /debug

!ELSEIF  "$(CFG)" == "Conquest - Win32 Release"

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
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Oa /Ow /Og /Oi /Oy- /Gf /I "include" /I "..\DInclude" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"pch.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 Release\Mission.lib Release\Globals.lib Release\Trim.lib Release\ZBatcher.lib DXGuid.lib MathLib.lib COMCTL32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3dx.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "Conquest - Win32 Debug"

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
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Debug\Mission.lib Debug\Globals.lib Debug\Trim.lib Debug\ZBatcher.lib DXGuid.lib MathLib.lib COMCTL32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3dx.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DemoDebug"
# PROP BASE Intermediate_Dir "DemoDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DemoDebug"
# PROP Intermediate_Dir "DemoDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /ML /W4 /Zi /Od /Gf /I "include" /I "..\DInclude" /D DA_ERROR_LEVEL=7 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DEMO_" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 DemoDebug\Mission.lib DemoDebug\Globals.lib DemoDebug\Trim.lib DemoDebug\ZBatcher.lib DXGuid.lib MathLib.lib COMCTL32.lib DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3dx.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Conquest - Win32 Final"
# Name "Conquest - Win32 DEMO Final"
# Name "Conquest - Win32 Release"
# Name "Conquest - Win32 Debug"
# Name "Conquest - Win32 DEMO Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "cur;ico;bmp"
# Begin Source File

SOURCE=.\Conquest.rc

!IF  "$(CFG)" == "Conquest - Win32 Final"

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Final"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEMO_"

!ELSEIF  "$(CFG)" == "Conquest - Win32 Release"

!ELSEIF  "$(CFG)" == "Conquest - Win32 Debug"

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Debug"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEMO_"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Res\Icon1.ico
# End Source File
# Begin Source File

SOURCE=.\resource2.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\Aegis.cpp
# End Source File
# Begin Source File

SOURCE=.\AirDefense.cpp
# End Source File
# Begin Source File

SOURCE=.\anim2d.cpp
# End Source File
# Begin Source File

SOURCE=.\AnimObj.cpp
# End Source File
# Begin Source File

SOURCE=.\AnmBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\AntiMatter.cpp
# End Source File
# Begin Source File

SOURCE=.\arc.cpp
# End Source File
# Begin Source File

SOURCE=.\AreaEffectBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\ArtileryLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\AutoCannon.cpp
# End Source File
# Begin Source File

SOURCE=.\BarrageLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseObject.cpp
# End Source File
# Begin Source File

SOURCE=.\beam.cpp
# End Source File
# Begin Source File

SOURCE=.\blackhole.cpp
# End Source File
# Begin Source File

SOURCE=.\blast.cpp
# End Source File
# Begin Source File

SOURCE=.\Blinkers.cpp
# End Source File
# Begin Source File

SOURCE=.\BuffLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildShip.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildSupPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\CloakEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\CloakLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\cloud.cpp
# End Source File
# Begin Source File

SOURCE=.\CommTrack.cpp

!IF  "$(CFG)" == "Conquest - Win32 Final"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Final"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Conquest - Win32 Release"

!ELSEIF  "$(CFG)" == "Conquest - Win32 Debug"

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Conquest.cpp
# End Source File
# Begin Source File

SOURCE=.\CQExtent.cpp
# End Source File
# Begin Source File

SOURCE=.\Damage.cpp
# End Source File
# Begin Source File

SOURCE=.\debris.cpp
# End Source File
# Begin Source File

SOURCE=.\Destabilizer.cpp
# End Source File
# Begin Source File

SOURCE=.\DumbReconProbe.cpp
# End Source File
# Begin Source File

SOURCE=.\Dust.cpp
# End Source File
# Begin Source File

SOURCE=.\EngineTrail.cpp
# End Source File
# Begin Source File

SOURCE=.\explosion.cpp
# End Source File
# Begin Source File

SOURCE=.\Fabricator.cpp
# End Source File
# Begin Source File

SOURCE=.\FancyLaunch.cpp
# End Source File
# Begin Source File

SOURCE=.\Fighter.cpp
# End Source File
# Begin Source File

SOURCE=.\FighterWing.cpp
# End Source File
# Begin Source File

SOURCE=.\FireBall.cpp
# End Source File
# Begin Source File

SOURCE=.\FlagShip.cpp
# End Source File
# Begin Source File

SOURCE=.\FogOfWar.cpp
# End Source File
# Begin Source File

SOURCE=.\GattlingBeam.cpp
# End Source File
# Begin Source File

SOURCE=.\GeneralPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupObj.cpp
# End Source File
# Begin Source File

SOURCE=.\Gunboat.cpp
# End Source File
# Begin Source File

SOURCE=.\GunPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\HarvestShip.cpp
# End Source File
# Begin Source File

SOURCE=.\jumpgate.cpp
# End Source File
# Begin Source File

SOURCE=.\JumpLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\JumpPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\KamikazeWing.cpp
# End Source File
# Begin Source File

SOURCE=.\light.cpp
# End Source File
# Begin Source File

SOURCE=.\Macrohelp.cpp
# End Source File
# Begin Source File

SOURCE=.\MantisBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\MassDisruptor.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshExplode.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshObj.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshRender.cpp
# End Source File
# Begin Source File

SOURCE=.\Mimic.cpp
# End Source File
# Begin Source File

SOURCE=.\MineField.cpp
# End Source File
# Begin Source File

SOURCE=.\MineLayer.cpp
# End Source File
# Begin Source File

SOURCE=.\MorphMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\MovieCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiCloakLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\Nugget.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjComm.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjControl.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectGenerator.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjGen.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjList.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjMap.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjMapIterator.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjMove.cpp
# End Source File
# Begin Source File

SOURCE=.\Overdrive.cpp
# End Source File
# Begin Source File

SOURCE=.\ParticleObj.cpp
# End Source File
# Begin Source File

SOURCE=.\Pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\PingLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\PlanetKillerBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\Planetoid.cpp
# End Source File
# Begin Source File

SOURCE=.\PlasmaBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerBomb.cpp
# End Source File
# Begin Source File

SOURCE=.\Projectile.cpp
# End Source File
# Begin Source File

SOURCE=.\RandomGen.cpp
# End Source File
# Begin Source File

SOURCE=.\ReconLaunch.cpp
# End Source File
# Begin Source File

SOURCE=.\ReconProbe.cpp
# End Source File
# Begin Source File

SOURCE=.\RefinePlat.cpp
# End Source File
# Begin Source File

SOURCE=.\RepairPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\RepellentCloud.cpp
# End Source File
# Begin Source File

SOURCE=.\Repulsor.cpp
# End Source File
# Begin Source File

SOURCE=.\RepulsorWave.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptObject.cpp
# End Source File
# Begin Source File

SOURCE=.\Search.asm

!IF  "$(CFG)" == "Conquest - Win32 Final"

# Begin Custom Build
IntDir=.\Final
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Final"

# Begin Custom Build
IntDir=.\DemoFinal
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Conquest - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Conquest - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Conquest - Win32 DEMO Debug"

# Begin Custom Build
IntDir=.\DemoDebug
InputPath=.\Search.asm
InputName=Search

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /c /Cx /coff /nologo /Fo$(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Sector.cpp
# End Source File
# Begin Source File

SOURCE=.\SellPlat.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\SolarianBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\SpaceShip.cpp
# End Source File
# Begin Source File

SOURCE=.\SpaceWave.cpp
# End Source File
# Begin Source File

SOURCE=.\SpecialBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\SpiderDrone.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StacticInit.cpp
# End Source File
# Begin Source File

SOURCE=.\StatisBolt.cpp
# End Source File
# Begin Source File

SOURCE=.\Streak.cpp
# End Source File
# Begin Source File

SOURCE=.\SuperTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\SupplyShip.cpp
# End Source File
# Begin Source File

SOURCE=.\Swapper.cpp
# End Source File
# Begin Source File

SOURCE=.\Synthesis.cpp
# End Source File
# Begin Source File

SOURCE=.\SysMap.cpp
# End Source File
# Begin Source File

SOURCE=.\TalorianEffect.cpp
# End Source File
# Begin Source File

SOURCE=.\TalorianLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\TerrainMap.cpp
# End Source File
# Begin Source File

SOURCE=.\TerranBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\TerranDrone.cpp
# End Source File
# Begin Source File

SOURCE=.\Testbed.cpp
# End Source File
# Begin Source File

SOURCE=.\Tractor.cpp
# End Source File
# Begin Source File

SOURCE=.\TractorWaveLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\Trail.cpp
# End Source File
# Begin Source File

SOURCE=.\Trigger.cpp
# End Source File
# Begin Source File

SOURCE=.\TroopPod.cpp
# End Source File
# Begin Source File

SOURCE=.\Troopship.cpp
# End Source File
# Begin Source File

SOURCE=.\Turret.cpp
# End Source File
# Begin Source File

SOURCE=.\UIAnim.cpp
# End Source File
# Begin Source File

SOURCE=.\UnbornMeshList.cpp
# End Source File
# Begin Source File

SOURCE=.\VLaunch.cpp
# End Source File
# Begin Source File

SOURCE=.\Waypoint.cpp
# End Source File
# Begin Source File

SOURCE=.\WormholeBlast.cpp
# End Source File
# Begin Source File

SOURCE=.\WormholeLauncher.cpp
# End Source File
# Begin Source File

SOURCE=.\Zealot.cpp
# End Source File
# End Group
# Begin Group "Data Definition Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DInclude\DAirDefense.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DArtileryLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\Data.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBarrageLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBaseData.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBlackHole.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBuffLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBuildObj.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBuildPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBuildSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DBuildSupPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DCamera.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DCloakLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEffect.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEffectOpts.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DEngineTrail.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DExplosion.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DExtension.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFabricator.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFabSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFancyLaunch.h
# End Source File
# Begin Source File

SOURCE=..\dinclude\DField.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFighter.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFighterWing.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFlagship.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DFog.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DGeneralPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DGroup.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DGunPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DInstance.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DJumpGate.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DJumpLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DJumpPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DJumpSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DLight.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMBaseData.h
# End Source File
# Begin Source File

SOURCE=..\dinclude\DMinefield.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMissionEnum.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMovieCamera.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMTechNode.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMultiCloakLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DMusic.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DNebula.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DNugget.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DObjectGenerator.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DObjNames.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPingLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPlanet.h
# End Source File
# Begin Source File

SOURCE=..\dinclude\DPlatform.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPlatSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DPlayerBomb.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DQuickSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DReconLaunch.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DRefinePlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DRepairPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DRepairSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DResearch.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DScriptObject.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSector.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSellPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DShipLaunch.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DShipSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSounds.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSpaceship.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSpecial.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSpecialData.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DStarfield.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSupplyPlat.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSupplyShip.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSupplyShipSave.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DSysmap.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTalorianLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTractorWaveLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTrail.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTrigger.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTroopship.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTurret.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DTypes.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DUnitSounds.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DVLaunch.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DWaypoint.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DWeapon.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\DWormholeLauncher.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\GameTypes.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\ObjClass.h
# End Source File
# Begin Source File

SOURCE=..\DInclude\Sfxid.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\anim2d.h
# End Source File
# Begin Source File

SOURCE=.\AntiMatter.h
# End Source File
# Begin Source File

SOURCE=.\ArchHolder.h
# End Source File
# Begin Source File

SOURCE=.\BBMesh.h
# End Source File
# Begin Source File

SOURCE=.\CloakLauncher.h
# End Source File
# Begin Source File

SOURCE=.\CommTrack.h
# End Source File
# Begin Source File

SOURCE=.\corners.h
# End Source File
# Begin Source File

SOURCE=.\CQExtent.h
# End Source File
# Begin Source File

SOURCE=.\CQLight.h
# End Source File
# Begin Source File

SOURCE=.\Damage.h
# End Source File
# Begin Source File

SOURCE=.\FancyLaunch.h
# End Source File
# Begin Source File

SOURCE=.\Field.h
# End Source File
# Begin Source File

SOURCE=.\FighterWing.h
# End Source File
# Begin Source File

SOURCE=.\FogOfWar.h
# End Source File
# Begin Source File

SOURCE=.\Include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Hotkeys.h
# End Source File
# Begin Source File

SOURCE=.\IAdmiral.h
# End Source File
# Begin Source File

SOURCE=.\IAttack.h
# End Source File
# Begin Source File

SOURCE=.\IBlast.h
# End Source File
# Begin Source File

SOURCE=.\IBlinkers.h
# End Source File
# Begin Source File

SOURCE=.\IBuild.h
# End Source File
# Begin Source File

SOURCE=.\IBuildShip.h
# End Source File
# Begin Source File

SOURCE=.\ICloak.h
# End Source File
# Begin Source File

SOURCE=.\IDust.h
# End Source File
# Begin Source File

SOURCE=.\IEngineTrail.h
# End Source File
# Begin Source File

SOURCE=.\IExplosion.h
# End Source File
# Begin Source File

SOURCE=.\ifabricator.h
# End Source File
# Begin Source File

SOURCE=.\IFighter.h
# End Source File
# Begin Source File

SOURCE=.\IGotoPos.h
# End Source File
# Begin Source File

SOURCE=.\IGroup.h
# End Source File
# Begin Source File

SOURCE=.\IHarvest.h
# End Source File
# Begin Source File

SOURCE=.\IJumpGate.h
# End Source File
# Begin Source File

SOURCE=.\IJumpPlat.h
# End Source File
# Begin Source File

SOURCE=.\ILauncher.h
# End Source File
# Begin Source File

SOURCE=.\IMineField.h
# End Source File
# Begin Source File

SOURCE=.\IMineLayer.h
# End Source File
# Begin Source File

SOURCE=.\IMissionActor.h
# End Source File
# Begin Source File

SOURCE=.\IMorphMesh.h
# End Source File
# Begin Source File

SOURCE=.\IMovieCamera.h
# End Source File
# Begin Source File

SOURCE=.\INugget.h
# End Source File
# Begin Source File

SOURCE=.\IObject.h
# End Source File
# Begin Source File

SOURCE=.\IObjectGenerator.h
# End Source File
# Begin Source File

SOURCE=.\IPlanet.h
# End Source File
# Begin Source File

SOURCE=.\IRecon.h
# End Source File
# Begin Source File

SOURCE=.\IRecoverShip.h
# End Source File
# Begin Source File

SOURCE=.\IRepairee.h
# End Source File
# Begin Source File

SOURCE=.\IRepairPlatform.h
# End Source File
# Begin Source File

SOURCE=.\IScriptObject.h
# End Source File
# Begin Source File

SOURCE=.\IShipDamage.h
# End Source File
# Begin Source File

SOURCE=.\IShipMove.h
# End Source File
# Begin Source File

SOURCE=.\ISpaceWave.h
# End Source File
# Begin Source File

SOURCE=.\ISupplier.h
# End Source File
# Begin Source File

SOURCE=.\ITalorianEffect.h
# End Source File
# Begin Source File

SOURCE=.\ITrigger.h
# End Source File
# Begin Source File

SOURCE=.\ITroopship.h
# End Source File
# Begin Source File

SOURCE=.\IUnbornMeshList.h
# End Source File
# Begin Source File

SOURCE=.\IUpgrade.h
# End Source File
# Begin Source File

SOURCE=.\IVertexBuffer.h
# End Source File
# Begin Source File

SOURCE=.\IWeapon.h
# End Source File
# Begin Source File

SOURCE=.\IWormholeBlast.h
# End Source File
# Begin Source File

SOURCE=.\MeshExplode.h
# End Source File
# Begin Source File

SOURCE=.\MeshRender.h
# End Source File
# Begin Source File

SOURCE=.\MineField.h
# End Source File
# Begin Source File

SOURCE=.\MPart.h
# End Source File
# Begin Source File

SOURCE=.\MultiCloakLauncher.h
# End Source File
# Begin Source File

SOURCE=.\MyVertex.h
# End Source File
# Begin Source File

SOURCE=.\NetVector.h
# End Source File
# Begin Source File

SOURCE=.\ObjList.h
# End Source File
# Begin Source File

SOURCE=.\ObjMap.h
# End Source File
# Begin Source File

SOURCE=.\ObjMapIterator.h
# End Source File
# Begin Source File

SOURCE=.\PingLauncher.h
# End Source File
# Begin Source File

SOURCE=.\RandomGen.h
# End Source File
# Begin Source File

SOURCE=.\RangeFinder.h
# End Source File
# Begin Source File

SOURCE=.\ReconLaunch.h
# End Source File
# Begin Source File

SOURCE=.\Sector.h
# End Source File
# Begin Source File

SOURCE=.\SimpleMesh.h
# End Source File
# Begin Source File

SOURCE=.\Startup.h
# End Source File
# Begin Source File

SOURCE=.\SuperTrans.h
# End Source File
# Begin Source File

SOURCE=.\SysMap.h
# End Source File
# Begin Source File

SOURCE=.\TerrainMap.h
# End Source File
# Begin Source File

SOURCE=.\TFabricator.h
# End Source File
# Begin Source File

SOURCE=.\THashList.h
# End Source File
# Begin Source File

SOURCE=.\TObjBuild.h
# End Source File
# Begin Source File

SOURCE=.\TObjCloak.h
# End Source File
# Begin Source File

SOURCE=.\TObjControl.h
# End Source File
# Begin Source File

SOURCE=.\TObjDamage.h
# End Source File
# Begin Source File

SOURCE=.\TObject.h
# End Source File
# Begin Source File

SOURCE=.\TObjExtension.h
# End Source File
# Begin Source File

SOURCE=.\TObjExtent.h
# End Source File
# Begin Source File

SOURCE=.\TObjFControl.h
# End Source File
# Begin Source File

SOURCE=.\TObjFrame.h
# End Source File
# Begin Source File

SOURCE=.\TObjGlow.h
# End Source File
# Begin Source File

SOURCE=.\TObjMission.h
# End Source File
# Begin Source File

SOURCE=.\TObjMove.h
# End Source File
# Begin Source File

SOURCE=.\TObjPhys.h
# End Source File
# Begin Source File

SOURCE=.\TObjRender.h
# End Source File
# Begin Source File

SOURCE=.\TObjRepair.h
# End Source File
# Begin Source File

SOURCE=.\TObjSelect.h
# End Source File
# Begin Source File

SOURCE=.\TObjTeam.h
# End Source File
# Begin Source File

SOURCE=.\TObjTrans.h
# End Source File
# Begin Source File

SOURCE=.\TObjWarp.h
# End Source File
# Begin Source File

SOURCE=.\TPlatform.h
# End Source File
# Begin Source File

SOURCE=.\Include\TriggerFlags.h
# End Source File
# Begin Source File

SOURCE=.\TSpaceShip.h
# End Source File
# Begin Source File

SOURCE=.\VLaunch.h
# End Source File
# End Group
# End Target
# End Project
