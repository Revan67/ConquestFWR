//--------------------------------------------------------------------------//
//                                                                          //
//                      LineManager.cpp                                        //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//																			//
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

	Control that scrolls informative text to the user			

*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "ILineManager.h"
#include "BaseHotRect.h"
#include "EventPriority.h"
#include "Startup.h"
#include "ObjWatch.h"
#include "IObject.h"
#include "ObjList.h"
#include "Camera.h"
#include "Vector.h"

#include "frame.h"
#include "hotkeys.h"
#include "Sfx.h"

#include <da_vector>
using namespace da_std;

//
//--------------------------------------------------------------------------//
//
class LineBaseObj
{
public:
	LineBaseObj (COLORREF _color, U32 _lifetime, U32 _displayTime) : color(_color), lifetime(_lifetime), displayTime(_displayTime)
	{
		bDead		= false;
		timer		= 0;
		percentage	= 0;
		lineID = idcounter++;

		if (displayTime == 0)
		{
			percentage = 1.0f;
		}
	}

	virtual ~LineBaseObj (void)
	{
	}

	virtual void Draw (void) = 0;

	// call this implementation before a child class's implementation
	virtual void Update (U32 dt)
	{
		// if timer has gone out then set death flag
		timer += dt;
		if (timer >= lifetime)
		{
			bDead = true;
			return;
		}

		if (percentage < 1.0f)
		{
			percentage = SINGLE(timer)/(SINGLE)displayTime;
			if (percentage > 1.0f)
			{
				percentage = 1.0f;
			}
		}
	}
	
	U32 lineID;
	static U32 idcounter;

	const COLORREF	color;
	const U32 displayTime;
	const U32 lifetime;

	U32		timer;
	SINGLE	percentage;
	bool	bDead;
};

U32 LineBaseObj::idcounter = 0;

//
//--------------------------------------------------------------------------//
//
class Line2DTo2D : public LineBaseObj
{
public:
	Line2DTo2D (S32 _x1, S32 _y1, S32 _x2, S32 _y2, COLORREF _color, U32 _lifetime, U32 _displayTime) 
		: LineBaseObj(_color, _lifetime, _displayTime), x1(_x1), y1(_y1), x2(_x2), y2(_y2)
	{
	}

	void Draw (void)
	{
		DA::LineDraw(NULL, x1, y1, xpos, ypos, color);
	}

	void Update (U32 dt)
	{
		LineBaseObj::Update(dt);

		xpos = x1 + S32((x2 - x1)*percentage);
		ypos = y1 + S32((y2 - y1)*percentage);
	}

	const S32 x1, y1;
	const S32 x2, y2;
	S32 xpos, ypos;
};
//
//--------------------------------------------------------------------------//
//
class Line2DToObj : public LineBaseObj
{
public:
	Line2DToObj (S32 _x, S32 _y, IBaseObject* _obj, COLORREF _color, U32 _lifetime, U32 _displayTime)
		: LineBaseObj(_color, _lifetime, _displayTime), x(_x), y(_y)
	{
		_obj->QueryInterface(IBaseObjectID, obj, SYSVOLATILEPTR);
		pos.x = 0; 
		pos.y = 0; 
		pos.z = 0;
	}

	void Draw (void)
	{
		if (obj)
		{
			DA::LineDraw(NULL, x, y, xpos, ypos, color);
		}
	}

	void Update (U32 dt)
	{
		LineBaseObj::Update(dt);

		if (obj)
		{
			xf  = obj->GetTransform();
			CAMERA->PointToScreen(pos, &xpos, &ypos, &xf);

			xpos = x + S32((xpos - x)*percentage);
			ypos = y + S32((ypos - y)*percentage);
		}
	}

	const S32 x, y;
	OBJPTR<IBaseObject> obj;
	
	S32 xpos, ypos;
	S32 xobj, yobj;
	Vector pos;
	Transform xf;
};
//
//--------------------------------------------------------------------------//
//
class LineObjToObj : public LineBaseObj
{
public:
	LineObjToObj (IBaseObject* _obj1, IBaseObject* _obj2, COLORREF _color, U32 _lifetime, U32 _displayTime)
		: LineBaseObj(_color, _lifetime, _displayTime)
	{
		_obj1->QueryInterface(IBaseObjectID, obj1, SYSVOLATILEPTR);
		_obj2->QueryInterface(IBaseObjectID, obj2, SYSVOLATILEPTR);
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
	}

	void Draw (void)
	{
		if (obj1 && obj2)
		{
			DA::LineDraw(NULL, x1, y1, x2, y2, color);
		}
	}

	void Update (U32 dt)
	{
		LineBaseObj::Update(dt);

		if (obj1 && obj2)
		{
			xf  = obj1->GetTransform();
			CAMERA->PointToScreen(pos, &x1, &y1, &xf);

			xf = obj2->GetTransform();
			CAMERA->PointToScreen(pos, &x2, &y2, &xf);

			x2 = x1 + S32((x2 - x1)*percentage);
			y2 = y1 + S32((y2 - y1)*percentage);
		}
	}

	OBJPTR<IBaseObject> obj1, obj2;
	S32 x1, x2, y1, y2;
	Vector pos;
	Transform xf;
};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE LineManager : public IEventCallback, ILineManager
{
	BEGIN_DACOM_MAP_INBOUND(LineManager)
	DACOM_INTERFACE_ENTRY(ILineManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	typedef vector<LineBaseObj*> LINE_VEC;

	LINE_VEC lines;
	U32 lineIDs;

	U32 handle;			// connection handle

	LineManager (void);

	~LineManager (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ILineManager methods */

	DEFMETHOD_(U32, CreateLineDisplay) (S32 x1, S32 y1, S32 x2, S32 y2, COLORREF color, U32 lifetime = 6000, U32 displayTime = 2000);
	
	DEFMETHOD_(U32, CreateLineDisplay) (S32 x1, S32 y1, IBaseObject* obj, COLORREF color, U32 lifetime = 6000, U32 displayTime = 2000);
	
	DEFMETHOD_(U32, CreateLineDisplay) (IBaseObject* obj1, IBaseObject* obj2, COLORREF color, U32 lifetime = 6000, U32 displayTime = 2000);

	DEFMETHOD_(void, DeleteLine) (U32 lineID);	
	
	DEFMETHOD_(void, Flush) (void);


	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* LineManager methods */

	void draw (void);

	IDAComponent * GetBase (void)
	{
		return (ILineManager *) this;
	}

	void onUpdate (U32 dt)			// dt is milliseconds
	{
		U32 i;

		// erase all "dead" lines
		for (i = 0; i < lines.size(); i++)
		{
			if (lines[i]->bDead)
			{
				LineBaseObj* pLine = lines[i];
				lines.erase(lines.begin() + i);
				delete pLine;
			}
		}

		// update all live lines
		for (i = 0; i < lines.size(); i++)
		{
			lines[i]->Update(dt);
		}
	}
};
//--------------------------------------------------------------------------//
//
LineManager::LineManager (void)
{
}
//--------------------------------------------------------------------------//
//
LineManager::~LineManager (void)
{
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}
}

//
//	All CreateLineDisplay functions return to control ID for the line object
//
//--------------------------------------------------------------------------//
//
U32 LineManager::CreateLineDisplay (S32 x1, S32 y1, S32 x2, S32 y2, COLORREF color, U32 lifetime, U32 displayTime)
{
	LineBaseObj* pLine = new Line2DTo2D(x1, y1, x2, y2, color, lifetime, displayTime);
	lines.push_back(pLine);

	U32 index = lines.size() - 1;

	return lines[index]->lineID;
}
//--------------------------------------------------------------------------//
//	
U32 LineManager::CreateLineDisplay (S32 x, S32 y, IBaseObject* obj, COLORREF color, U32 lifetime, U32 displayTime)
{
	LineBaseObj* pLine = new Line2DToObj(x, y, obj, color, lifetime, displayTime);
	lines.push_back(pLine);

	U32 index = lines.size() - 1;

	return lines[index]->lineID;
}
//--------------------------------------------------------------------------//
//	
U32 LineManager::CreateLineDisplay (IBaseObject* obj1, IBaseObject* obj2, COLORREF color, U32 lifetime, U32 displayTime)
{
	LineBaseObj* pLine = new LineObjToObj(obj1, obj2, color, lifetime, displayTime);
	lines.push_back(pLine);

	U32 index = lines.size() - 1;

	return lines[index]->lineID;

}
//--------------------------------------------------------------------------//
//
void LineManager::DeleteLine (U32 lineID)
{
	U32 i;
	for (i = 0; i < lines.size(); i++)
	{
		if (lines[i]->lineID == lineID)
		{
			LineBaseObj * obj = lines[i];
			lines.erase(lines.begin() + i);
			delete obj;
			break;
		}
	}
}
//--------------------------------------------------------------------------//
//
void LineManager::Flush (void)
{
	U32 i;
	for (i = 0; i < lines.size(); i++)
	{
		delete lines[i];
	}

	lines.erase(lines.begin(), lines.end());
}
//--------------------------------------------------------------------------//
//
void LineManager::draw (void)
{
	if (CQFLAGS.bGameActive)
	{
		bool bFrameLocked = (CQFLAGS.bFrameLockEnabled != 0);

		if (bFrameLocked)
		{
			if (SURFACE->Lock() == false)
			{
				return;
			}
		}

		U32 i;
		for (i = 0; i < lines.size(); i++)
		{
			lines[i]->Draw();
		}
			
		if (bFrameLocked)
		{
			SURFACE->Unlock();
		}
	}

}

//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT LineManager::Notify (U32 message, void *param)
{
	static BOOL32 bBrief = FALSE;

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		Flush();
		break;

	case CQE_ENDFRAME:
		draw();
		break;

	case CQE_UPDATE:
		if (!CQFLAGS.bGamePaused)
			onUpdate(S32(param) >> 10);
		break;
	}

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
struct _linemanager : GlobalComponent
{
	LineManager * LMAN;
	
	virtual void Startup (void)
	{
		LINEMAN = LMAN = new DAComponent<LineManager>;
		AddToGlobalCleanupList((IDAComponent **) &LINEMAN);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(LMAN->GetBase(), &LMAN->handle);
			FULLSCREEN->SetCallbackPriority(LMAN, EVENT_PRIORITY_LINEDISPLAY);
		}
	}
};

static _linemanager startup;

//--------------------------------------------------------------------------//
//--------------------------End LineManager.cpp-----------------------------//
//--------------------------------------------------------------------------//