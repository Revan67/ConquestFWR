#ifndef TMANAGER_H
#define TMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TManager.h                                 //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/TManager.h 4     4/28/00 1:41a Jasony $
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "Pixel.h"

#define IID_ITManager MAKE_IID("ITManager",1)

namespace DA
{
	enum FILETYPE
	{
		UNKTYPE,
		BMP=1,
		TGA,
		VFX
	};

}

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ITManager : public IDAComponent
{
	struct InitInfo
	{
		struct IRenderPipeline * PIPE;
	};

	virtual U32 __stdcall CreateTextureFromMemory (void *pMemory, DA::FILETYPE type, const PixelFormat &format, const char *name) = 0;

	virtual U32 __stdcall CreateTextureFromFile (const char *fileName, IComponentFactory *parentFile, DA::FILETYPE type, const PixelFormat &format) = 0;

	virtual U32 __stdcall AddTextureRef (U32 textureID) = 0;		// returns new ref count

	virtual U32 __stdcall ReleaseTextureRef (U32 textureID) = 0;	// returns new ref count

	virtual void __stdcall Flush (void) = 0;

	virtual U32 __stdcall GetFirstTexture() = 0;

	virtual U32 __stdcall GetNextTexture(U32 textureID) = 0;

	virtual U32 __stdcall GetPrevTexture(U32 textureID) = 0;

	virtual void Initialize(InitInfo & info) = 0;

	//
	// methods for optimizing drawAgent usage
	//
	
	// use a texture from special drawAgent cache, or create a new texture
	virtual U32 __stdcall CreateDrawAgentTexture (U32 resolution, bool bAlpha) = 0;

	virtual void __stdcall ReleaseDrawAgentTexture (U32 textureID) = 0;
};


//---------------------------------------------------------------------------//
//-----------------------------End TManager.h--------------------------------//
//---------------------------------------------------------------------------//
#endif