//--------------------------------------------------------------------------//
//                                                                          //
//                               QueueControl.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/QueueControl.cpp 6     7/31/00 2:56p Rmarr $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IQueueControl.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DQueueControl.h>
#include "DrawAgent.h"
#include "IShapeLoader.h"
#include "DHotButtonText.h"
#include "IActiveButton.h"

//#include "VideoSurface.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#define BEGIN_MAPPING(parent, name) \
	{								\
		HANDLE hFile, hMapping;		\
		DAFILEDESC fdesc = name;	\
		void *pImage;				\
		if ((hFile = parent->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)	\
			CQFILENOTFOUND(fdesc.lpFileName);	\
		hMapping = parent->CreateFileMapping(hFile);	\
		pImage = parent->MapViewOfFile(hMapping)

#define END_MAPPING(parent) \
		parent->UnmapViewOfFile(pImage); \
		parent->CloseHandle(hMapping); \
		parent->CloseHandle(hFile); }

//--------------------------------------------------------------------------//
//
struct QUEUECONTROLTYPE
{
	PGENTYPE pArchetype;

	QUEUECONTROLTYPE (void)
	{
	}

	~QUEUECONTROLTYPE (void)
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
struct DACOM_NO_VTABLE QueueControl : BaseHotRect, IQueueControl
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(QueueControl)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IQueueControl)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	QUEUECONTROLTYPE * pQType;
	COMPTR<IDrawAgent> shapes[FAB_MAX_QUEUE_SIZE][GTHBSHP_MAX_SHAPES];
	U32 slotIDs[FAB_MAX_QUEUE_SIZE];
	COMPTR<IDrawAgent> noMoney;
	COMPTR<IDrawAgent> unitLimit;

	SINGLE progress;
	U32 stallType;
	U32 numInQueue;
	U16 aveWidth;
	bool bPostMessageBlocked;
	U32 controlID;
	
	//
	// class methods
	//

	QueueControl (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~QueueControl (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IQueueControl methods  */

	virtual void InitQueueControl (const QUEUECONTROL_DATA & data, BaseHotRect * parent); 

	virtual void SetVisible (bool bVisible);

	virtual void AddToQueue(IDrawAgent ** shape, U32 slotID);

	virtual void ResetQueue();

	virtual void SetPercentage(SINGLE percent, U32 _stallType);

	virtual void SetControlID(U32 newID)
	{
		controlID = newID;
	}

	/* QueueControl methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (QUEUECONTROLTYPE * _pQType);

	void draw (void);

	void doLPushAction (U32 mouseX, U32 mouseY)
	{
		if(numInQueue)
		{
			U16 firstWidth,firstHeight;
			shapes[0][GTHBSHP_NORMAL]->GetDimensions(firstWidth,firstHeight);
			if(S32(mouseY) <= screenRect.top+S32(firstHeight))
			{
				mouseX -= screenRect.left;
				if(mouseX <= firstWidth)
				{
					TOOLBAR->PostMessage(CQE_BUILDQUEUE_REMOVE,(void *)slotIDs[0]);
					return;
				}	
				if(numInQueue-1)
				{
					mouseX -= firstWidth;
					
					U32 width = (screenRect.right-screenRect.left);
					U32 useAveWidth = aveWidth;
					if(width < aveWidth*numInQueue)
					{
						useAveWidth = (width/(numInQueue+1));
					}

					for(U32 i = 1; i < numInQueue; ++i)
					{
						if(mouseX <= useAveWidth)
						{
							TOOLBAR->PostMessage(CQE_BUILDQUEUE_REMOVE,(void *)slotIDs[i]);
							return;
						}
						mouseX -= useAveWidth;
					}
				}
			}
		}
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
QueueControl::~QueueControl (void)
{
	GENDATA->Release(pQType->pArchetype);
	for(U32 i = 0; i < FAB_MAX_QUEUE_SIZE; ++i)
	{
		for(U32 j = 0; j < GTHBSHP_MAX_SHAPES;++j)
			shapes[i][j].free();
	}
}
//--------------------------------------------------------------------------//
//
void QueueControl::InitQueueControl (const QUEUECONTROL_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, 0x40000000);
	}

	U32 width,height;
	width = IDEAL2REALX(data.width);
	height = IDEAL2REALY(data.height);

	controlID = 0;

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;
	
	stallType = IActiveButton::NO_STALL;
	progress = 0.0;
	numInQueue = 0;
}
//--------------------------------------------------------------------------//
//
void QueueControl::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
void QueueControl::AddToQueue(IDrawAgent ** shape,U32 slotID)
{
	CQASSERT(numInQueue < FAB_MAX_QUEUE_SIZE);
	slotIDs[numInQueue] = slotID;
	for(U32 i = 0; i < GTHBSHP_MAX_SHAPES;++i)
	{
		shapes[numInQueue][i] = shape[i];
	}
	if(!numInQueue)
	{
		U16 height;
		shape[0]->GetDimensions(aveWidth,height);
	}
	++numInQueue;
}
//--------------------------------------------------------------------------//
//
void QueueControl::ResetQueue()
{
	for(U32 i = 0; i < numInQueue; ++i)
	{
		for(U32 j = 0; j < GTHBSHP_MAX_SHAPES; ++j)
			shapes[i][j].free();
	}
	stallType = IActiveButton::NO_STALL;
	progress = 0.0;
	numInQueue = 0;
}
//--------------------------------------------------------------------------//
//
void QueueControl::SetPercentage(SINGLE percent, U32 _stallType)
{
	progress = percent;
	stallType = _stallType;
}
//--------------------------------------------------------------------------//
//
GENRESULT QueueControl::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_ENDFRAME:
		if ((CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		draw();
		break;
	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if ((CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			desiredOwnedFlags = (RF_CURSOR|RF_STATUS);
			grabAllResources();
		}
		else
		if (actualOwnedFlags)
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if ((CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		if(bAlert)
		{
			U32 mouseX = LOWORD(msg->lParam);
			U32 mouseY = HIWORD(msg->lParam);

			doLPushAction(mouseX,mouseY);
		}
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void QueueControl::draw (void)
{
	if(numInQueue)
	{
		S32 readyIndex = -1;
		if(bAlert)
		{
			S32 mouseX,mouseY;
			WM->GetCursorPos(mouseX,mouseY);
			
			U16 firstWidth,firstHeight;
			shapes[0][GTHBSHP_NORMAL]->GetDimensions(firstWidth,firstHeight);
			if(mouseY <= (S32)(screenRect.top+firstHeight))
			{
				mouseX -= screenRect.left;
				if(mouseX <= (S32)(firstWidth))
				{
					readyIndex = 0;
				}	
				else if(numInQueue-1)
				{
					mouseX -= firstWidth;
					
					U32 width = (screenRect.right-screenRect.left);
					U32 useAveWidth = aveWidth;
					if(width < aveWidth*numInQueue)
					{
						useAveWidth = (width/(numInQueue+1));
					}

					for(U32 i = 1; i < numInQueue; ++i)
					{
						if(mouseX <= (S32)(useAveWidth))
						{
							readyIndex = i;
							break;
						}
						mouseX -= useAveWidth;
					}
				}
			}
		}

		if(numInQueue-1)
		{
			U32 width = (screenRect.right-screenRect.left);
			U32 useAveWidth = aveWidth;
			if(width < aveWidth*(numInQueue))
			{
				useAveWidth = (width/(numInQueue+1));
			}
			for(S32 i = numInQueue-1; i; --i)
			{
				if(shapes[i][GTHBSHP_NORMAL])
					shapes[i][GTHBSHP_NORMAL]->Draw(0,screenRect.left+(useAveWidth*i),screenRect.top);
				if(readyIndex == i)
					if(shapes[i][GTHBSHP_MOUSE_FOCUS])
						shapes[i][GTHBSHP_MOUSE_FOCUS]->Draw(0,screenRect.left+(useAveWidth*i),screenRect.top);
			}
		}
		//draw progress on first in queue
		U16 firstWidth,firstHeight;
		shapes[0][GTHBSHP_NORMAL]->GetDimensions(firstWidth,firstHeight);

		S32 midPoint = progress*firstWidth;
		
		PANE pane;
		pane.window = 0;
		pane.x0 = screenRect.left;
		pane.x1 = screenRect.left+midPoint;
		pane.y0 = screenRect.top;
		pane.y1 = screenRect.top+firstHeight;
		if(pane.x0 < pane.x1)
		{
			if (shapes[0][GTHBSHP_MOUSE_FOCUS])
				shapes[0][GTHBSHP_MOUSE_FOCUS]->Draw(&pane, 0, 0);
		}
		pane.x0 = screenRect.left+midPoint;
		pane.x1 = screenRect.left+firstWidth;
		if(pane.x0 < pane.x1)
		{
			if (shapes[0][GTHBSHP_DISABLED])
				shapes[0][GTHBSHP_DISABLED]->Draw(&pane, -midPoint, 0);
		}
		if(shapes[0][GTHBSHP_SELECTED]!=0)
			shapes[0][GTHBSHP_SELECTED]->Draw(0, screenRect.left, screenRect.top);

	}
}
//--------------------------------------------------------------------------//
//
void QueueControl::init (QUEUECONTROLTYPE * _pQType)
{
	COMPTR<IDAComponent> pBase;
	pQType = _pQType;
}

//--------------------------------------------------------------------------//
//-----------------------QueueControl Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE QueueControlFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(QueueControlFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	QueueControlFactory (void) { }

	~QueueControlFactory (void);

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

	/* QueueControlFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
QueueControlFactory::~QueueControlFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void QueueControlFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE QueueControlFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_QUEUECONTROL)
	{
//		GT_QUEUECONTROL * data = (GT_QUEUECONTROL *) _data;
		QUEUECONTROLTYPE * result = new QUEUECONTROLTYPE;

		result->pArchetype = pArchetype;

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 QueueControlFactory::DestroyArchetype (HANDLE hArchetype)
{
	QUEUECONTROLTYPE * type = (QUEUECONTROLTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT QueueControlFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	QUEUECONTROLTYPE * type = (QUEUECONTROLTYPE *) hArchetype;
	QueueControl * result = new DAComponent<QueueControl>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _QueueControlFactory : GlobalComponent
{
	QueueControlFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<QueueControlFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _QueueControlFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End QueueControl.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
