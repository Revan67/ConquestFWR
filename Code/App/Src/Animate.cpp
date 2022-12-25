//--------------------------------------------------------------------------//
//                                                                          //
//                               Aniamte.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Animate.cpp 37    4/25/01 11:34a Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include <DAnimate.h>

#include "IAnimate.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "DrawAgent.h"
#include "IShapeLoader.h"
#include "TManager.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#define TIMER_FOR_30FPS		33

//--------------------------------------------------------------------------//
//
struct ANIMATETYPE
{
	PGENTYPE pArchetype;
	COMPTR<IDrawAgent> shapes[GTASHP_MAX_SHAPES];
	U16 width;
	U16 height;
	U16 nCells;
	bool bRepeatAnimation;

	ANIMATETYPE (void)
	{
	}

	~ANIMATETYPE (void)
	{
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Animate : BaseHotRect, IAnimate
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Animate)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IAnimate)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	ANIMATETYPE * pAnimateType;

	U32 nCells;
	U32 ticksToNextCell;
	U32 timer, fullTimer;
	U32 currentCell;

	const U32 * pIndexArray;
	U32   nElements;
	U32   currentIndex;
	U32   textureID;
	COMPTR<IDrawAgent> frameAgent;
	U32	  fuzzIndex;

	bool bPaused;
	bool bLooping;
	bool bSeenAllCells;

	//
	// class methods
	//

	Animate (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		bLooping = true;
	}

	virtual ~Animate (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IAnimate methods  */

	virtual void InitAnimate (const ANIMATE_DATA & data, BaseHotRect * parent, const U32 * indexArray, U32 nElements); 

	virtual void SetVisible (bool bVisible);

	virtual void PauseAnim (bool bPause);

	virtual void DeferredDestruction (void);

	virtual void SetAnimationData (const U32 * indexArray, U32 numElements);

	virtual void SetLoopingAnimation (bool _bLooping)
	{
		bLooping = _bLooping;
	}

	virtual bool HasAnimationCompleted (void)
	{
		return bSeenAllCells;
	}

	virtual void SetAnimationFrameRate (U32 timePerCell)
	{
		ticksToNextCell = timePerCell;
	}

	/* Static methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (ANIMATETYPE * _pStaticType);

	void draw (void);

	void update (U32 dt);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Animate::~Animate (void)
{
	if (textureID)
	{
		if (TMANAGER)
		{
			TMANAGER->ReleaseTextureRef(textureID);
		}
	}
	frameAgent.free();
	if (GENDATA)
	{
		GENDATA->Release(pAnimateType->pArchetype);
	}
}
//--------------------------------------------------------------------------//
//
void Animate::InitAnimate (const ANIMATE_DATA & data, BaseHotRect * _parent, const U32 * indexArray, U32 _nElements)
{
	if ((parent == NULL) && (_parent != NULL))
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());

		if (parent == FULLSCREEN)
		{
			parent->SetCallbackPriority(this, EVENT_PRIORITY_TALKINGHEAD);
		}
		else
		{
			parent->SetCallbackPriority(this, 0x40000000);
		}
	}

	if (parent)
	{
		screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
		screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
		screenRect.bottom = screenRect.top + pAnimateType->height - 1;
		screenRect.right = screenRect.left + pAnimateType->width - 1;
	}
	else
	{
		screenRect.left = IDEAL2REALX(data.xOrigin);
		screenRect.top = IDEAL2REALY(data.yOrigin);
		screenRect.bottom = screenRect.top + pAnimateType->height - 1;
		screenRect.right = screenRect.left + pAnimateType->width - 1;
	}

	ticksToNextCell = data.dwTimer/nCells;

	pIndexArray = indexArray;
	if (pIndexArray && _nElements)
	{
		nElements = _nElements;
		currentIndex = 0;
		currentCell = pIndexArray[currentIndex];
	}

	// create a shape loader and use it to create draw agents until it fails
	COMPTR<IDAComponent> pBase;
	COMPTR<IShapeLoader> loader;


	// make the bullshit texture
	if (data.bFuzzEffect && CQFLAGS.b3DEnabled) 
	{
		GENDATA->CreateInstance("VFXShape!!TalkingHeadBoarder", pBase);
		pBase->QueryInterface("IShapeLoader", loader);
		loader->CreateDrawAgent(0, frameAgent);
		// only do fuzz texture effect if frame locking is disabled
		if (CQFLAGS.bFrameLockEnabled == false)
		{
			textureID = TMANAGER->CreateTextureFromFile("videoFX.tga", TEXTURESDIR,DA::TGA, PF_4CC_DAA4);
			fuzzIndex = rand()%16;
		}
	}
	else
	{
		textureID = 0;
	}
}
//--------------------------------------------------------------------------//
//
void Animate::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
void Animate::PauseAnim (bool bPause)
{
	bPaused=bPause;
}
//--------------------------------------------------------------------------//
//
void Animate::SetAnimationData (const U32 * indexArray, U32 numElements)
{
	pIndexArray = indexArray;
	if (pIndexArray && numElements)
	{
		nElements = numElements;
		currentIndex = 0;
		currentCell = pIndexArray[currentIndex];
	}
}
//--------------------------------------------------------------------------//
//
void Animate::DeferredDestruction (void)
{
#ifdef _DEBUG
	IAnimate * anim = static_cast<IAnimate *>(this);
	U32 ref;
	anim->AddRef();
	ref = anim->Release();
	CQASSERT(ref == 1);
#endif
	pIndexArray = NULL;
	if (parent)
		parent->PostMessage(CQE_DELETE_HOTRECT, static_cast<BaseHotRect *>(this));
	else
		getBase()->Release();
}
//--------------------------------------------------------------------------//
//
GENRESULT Animate::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case CQE_UPDATE:
		update((bPaused)? 0 : (U32(param) >> 10));
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Animate::update (U32 dt)
{
	timer += dt;
	fullTimer += dt;

	if (pIndexArray && bPaused == false)
	{
		// play back a recorded set of frames
		// assume this is at 30 frames/second
		if (timer > TIMER_FOR_30FPS)
		{
			currentIndex = fullTimer/TIMER_FOR_30FPS;

			if (currentIndex >= nElements)
			{
				if (bLooping)
				{
					currentIndex = 0;
				}
				else
				{
					currentIndex = nElements-1;
				}
			}

			// make sure the current cell is set and is valid
			currentCell = pIndexArray[currentIndex];
			if (currentCell >= nCells)
			{
				currentCell = 0;
			}
			timer = 0;
		}
	}
	else
	{
		if (bPaused == false)
		{
			// simply loop through the animation
			if (timer >= ticksToNextCell)
			{
				currentCell++;
				if (currentCell >= nCells)
				{
					bSeenAllCells = true;

					if (bLooping)
					{
						currentCell = 0;
					}
					else
					{
						currentCell = nCells-1;
					}
				}
				timer = 0;
			}
		}
		else
		{
			// make sure that the current cell is between 0 and nCells-1
			if (currentCell >= nCells || currentCell < 0)
			{
				currentCell = 0;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Animate::draw (void)
{
	// go through the animation cells
	if (pAnimateType->shapes[currentCell])
	{
		pAnimateType->shapes[currentCell]->Draw(0, screenRect.left, screenRect.top);
		
		// if we can, draw the stupid video effect
		if (textureID && CQFLAGS.bFrameLockEnabled == false && CQFLAGS.b3DEnabled)
		{
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND, D3DBLEND_ONE); 
			BATCH->set_render_state(D3DRS_DESTBLEND, D3DBLEND_ONE);

		  	SetupDiffuseBlend(textureID, TRUE);
			PB.Color3ub(255,255,255);

			if (!bPaused)
			{
				fuzzIndex = (fuzzIndex+1)%16;
			}

			SINGLE xf = (fuzzIndex%4) * 0.25f;
			SINGLE yf = (fuzzIndex/4) * 0.25f; 

			PB.Begin(PB_QUADS);
			PB.TexCoord2f(xf, yf);					PB.Vertex3f(screenRect.left,	screenRect.top, 0);
			PB.TexCoord2f(xf + 0.25f, yf);			PB.Vertex3f(screenRect.right,	screenRect.top, 0);
			PB.TexCoord2f(xf + 0.25f, yf + 0.25f);	PB.Vertex3f(screenRect.right,	screenRect.bottom, 0);
			PB.TexCoord2f(xf, yf + 0.25f);			PB.Vertex3f(screenRect.left,	screenRect.bottom, 0);
			PB.End();
		}

		if(frameAgent)
			frameAgent->Draw(0,screenRect.left-IDEAL2REALX(5),screenRect.top-IDEAL2REALY(8));
	}
}
//--------------------------------------------------------------------------//
//
void Animate::init (ANIMATETYPE * _pAnimateType)
{
	pAnimateType = _pAnimateType;
	nCells = pAnimateType->nCells;
}

//--------------------------------------------------------------------------//
//-----------------------Animate Factory class------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE AnimateFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(AnimateFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	AnimateFactory (void) { }

	~AnimateFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ICQFactory methods */

	virtual HANDLE CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *data);

	virtual BOOL32 DestroyArchetype (HANDLE hArchetype);

	virtual GENRESULT CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance);

	/* AnimateFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
AnimateFactory::~AnimateFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void AnimateFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE AnimateFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_ANIMATE)
	{
		GT_ANIMATE * data = (GT_ANIMATE *) _data;
		ANIMATETYPE * result = new ANIMATETYPE;

		result->pArchetype = pArchetype;

		// create a shape loader and use it to create draw agents until it fails
		COMPTR<IDAComponent> pBase;
		COMPTR<IShapeLoader> loader;

		GENDATA->CreateInstance(data->vfxType, pBase);
		pBase->QueryInterface("IShapeLoader", loader);

		U32 i;
		U32 nCells = 0;
		
		for (i = 0; i < GTASHP_MAX_SHAPES; i++)
		{
			if (loader->CreateDrawAgent(i, result->shapes[i]) == GR_OK)
			{
				nCells++;
			}
			else
			{
				break;
			}
		}

		// get the width and height
		result->shapes[0]->GetDimensions(result->width, result->height);
		result->nCells = nCells;
		
		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 AnimateFactory::DestroyArchetype (HANDLE hArchetype)
{
	ANIMATETYPE * type = (ANIMATETYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT AnimateFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	ANIMATETYPE * type = (ANIMATETYPE *) hArchetype;
	Animate * result = new DAComponent<Animate>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _AnimateFactory : GlobalComponent
{
	AnimateFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<AnimateFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _AnimateFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Animate.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//