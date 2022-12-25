SET FROM_PATH=%1
SET EXPLICITDLL_PATH=..\..\ExplicitDLL\
SET IMPLICITDLL_PATH=..\..\ImplicitDLL\

@ REM for local builds of libs

attrib %EXPLICITDLL_PATH%*.* -r
attrib %IMPLICITDLL_PATH%*.* -r

@ rem EXPLICITDLL_PATH

xcopy ..\Anim\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Anim\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Channel\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Channel\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\COMHeap\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\COMHeap\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\DAHOTKEY\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\DAHOTKEY\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\VIEWER\DOCUVIEW\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\VIEWER\DOCUVIEW\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\VIEWER\HexView\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\VIEWER\HexView\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Viewer\ViewUTF\exe\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Viewer\ViewUTF\exe\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\VIEWER\VfxView\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\VIEWER\VfxView\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\DEFCOMP\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\DEFCOMP\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\DOSFILE\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\DOSFILE\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\EngComps\EngineCameras\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\EngComps\EngineCameras\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\EngComps\EngineLights\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\EngComps\EngineLights\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\EngComps\Hardpoint\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\EngComps\Hardpoint\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\EngComps\RenderMgr\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\EngComps\RenderMgr\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Engine\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Engine\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\LightManager\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\LightManager\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\RendComp\Optics\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\RendComp\Optics\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\RendComp\PolyMesh\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\RendComp\PolyMesh\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\RendComp\PolyMesh\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\RendComp\PolyMesh\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\RenderBatcher\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\RenderBatcher\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\RenderPipeline\DirectDraw\D3D\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\RenderPipeline\DirectDraw\D3D\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Shading\LightManager\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Shading\LightManager\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Shading\TextureLibrary\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Shading\TextureLibrary\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Streamer\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Streamer\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Streamer\dsstreamer\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Streamer\dsstreamer\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Viewer\VfxView\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Viewer\VfxView\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\Viewer\DOCUVIEW\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\Viewer\DOCUVIEW\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\VertexBufferManager\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\VertexBufferManager\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

xcopy ..\x86Math\%FROM_PATH%\*.dll %EXPLICITDLL_PATH% /Y
xcopy ..\x86Math\%FROM_PATH%\*.pdb %EXPLICITDLL_PATH% /Y

@rem IMPLICITDLL_PATH

xcopy ..\DACOM\%FROM_PATH%\*.dll %IMPLICITDLL_PATH% /Y
xcopy ..\DACOM\%FROM_PATH%\*.pdb %IMPLICITDLL_PATH% /Y

xcopy ..\Sockets\%FROM_PATH%\*.dll %IMPLICITDLL_PATH% /Y
xcopy ..\Sockets\%FROM_PATH%\*.pdb %IMPLICITDLL_PATH% /Y

del %IMPLICITDLL_PATH%vc60.pdb
del %IMPLICITDLL_PATH%vc60.pdb

pause