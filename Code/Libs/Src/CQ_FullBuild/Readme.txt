This Full Build process should build both the Release and Debug versions of the Conquest 1.6 libraries to their correct locations.

Right now the correct locations are:

1. for Dynamically loaded DLLs -> Z:\CQ2\Code\Libs\ExplicitDLL

2. linked in DLLs -> Z:\CQ2\Code\Libs\ImplicitDLL

3. statically linked DLLs -> Z:\CQ2\Code\Libs\Static

Static Libs:
	COMHeap
	DACOM
	MathLib
	RPUL

(maybe could get rid of this...)
Implicit Projects:
	DACOM (build in static)
	proto (x86math.dll)
	blade (not rebuild, already present)

Explicit Projects:
	Anim
	Channel
	D3DRenderPipe
	DataViewer
	DosFile
	Engine
	Hardpoint
	Hotkey
	LightManager
	Optics
	PolyMesh
	RenderManager
	streamer
	TextureLibrary
	VertexBufferManager

