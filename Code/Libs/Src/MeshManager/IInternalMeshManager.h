#ifndef IINTERNALMESHMANAGER_H
#define IINTERNALMESHMANAGER_H

//--------------------------------------------------------------------------//
//                                                                          //
//                       IInternalMeshManager.H                             //
//                                                                          //
//               COPYRIGHT (C) 2004 By Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//

struct IInternalMeshManager
{
	virtual struct IEngine * GetEngine() = 0;

	virtual struct IAnimation * GetAnim() = 0;

	virtual struct IRenderer * GetRenderer() = 0;

	virtual struct IRenderPipeline * GetPipe() = 0;

	virtual struct ICamera * GetCamera() = 0;

	virtual struct IHardpoint * GetHardpoint() = 0;

	virtual struct IMaterialManager * GetMatManager() = 0;

	virtual void ReleaseArch(struct IMeshArchetype * arch) = 0;
};

#endif