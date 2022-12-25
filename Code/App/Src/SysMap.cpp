//--------------------------------------------------------------------------//
//                                                                          //
//                                SysMap.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/SysMap.cpp 169   8/23/01 1:53p Tmauer $

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "TDocClient.h"
#include "SysMap.h"
#include "Menu.h"
#include "Resource.h"
#include "Camera.h"
#include "UserDefaults.h"
#include "IObject.h"
#include "ObjList.h"
#include "DBHotkeys.h"
#include "Hotkeys.h"
#include "SuperTrans.h"
#include "Cursor.h"
#include "StatusBar.h"
#include "IImageReader.h"
#include "DrawAgent.h"
#include "Startup.h"
#include "TResClient.h"
#include "fogofwar.h"
#include "DSysMap.h"
#include "Sector.h"
#include "BaseHotRect.h"
#include "IShapeLoader.h"
#include "MPart.h"
#include "GenData.h"
#include <DMBaseData.h>
#include "MGlobals.h"
#include "Mission.h"
#include "CQBatch.h"
#include "GridVector.h"
#include "TManager.h"
#include "INugget.h"
#include "Field.h"
#include "ObjMapIterator.h"
#include "OpAgent.h"
#include "ObjSet.h"
#include "Anim2d.h"
#include "IFabricator.h"
#include "IToolbar.h"
#include <DSector.h>

//#include <ITextureLibrary.h>
#include <HKEvent.h>
#include <EventSys.h>
#include <IDocClient.h>
#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <3DMath.h>
#include <Viewer.h>
#include <ViewCnst.h>
#include <Document.h>
#include <FileSys.h>
#include <WindowManager.h>
#include <RendPipeline.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define SCROLL_X  100
#define SCROLL_Y  100

#define SCROLL_SLOP 4000

#define SYSMAP_PRIORITY  (RES_PRIORITY_TOOLBAR+1)

static char szRegKey[] = "Thumbnail";

#define MOVE_DISTANCEX IDEAL2REALX(2)
#define MOVE_DISTANCEY IDEAL2REALY(2)

//#define COMPASS_TEX_RES 128 ???

static S32 sysmap_left;
static S32 sysmap_right;
static S32 sysmap_top;
static S32 sysmap_bottom;

//placement of sysmap
#define SYSMAP_LEFT   (sysmap_left)
#define SYSMAP_TOP    (sysmap_top)
#define SYSMAP_SIZE  (sysmap_right-sysmap_left)

#define TGRID_SIZE 64

//icon info
#define MAX_ICONS 100
#define MAX_PLAYER_ICONS 5

static Vector mapPie[17];
static Vector mapPieTex[17];

#define DEFAULT_SYSTEM_PING_TIME 20

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

enum SYSMAP_MODE
{
	SYSMODE_NONE,
	SYSMODE_MOVE,
	SYSMODE_RALLY,	// same action in both right click and left click mouse model
	SYSMODE_GOTO,
	SYSMODE_BAN
};

#define ATTACK_DEFAULT_RAD 2
#define ATTACK_DEFAULT_LIFETIME 10

struct AttackBubble
{
	AttackBubble * next;
	GRIDVECTOR pos;
	SINGLE radius;
	SINGLE lifetime;
	bool bRendered;
};

struct MissionAnim
{
	GRIDVECTOR pos;
	U32 systemID;
	U32 animID;
	MissionAnim * next;

	MissionAnim (void)
	{
		memset(this, 0, sizeof(MissionAnim));
	}
};

struct DACOM_NO_VTABLE SysMap : public ISystemMap, 
								  IEventCallback,
								  DocumentClient,
								  ResourceClient<>,
								  SYSMAP_DATA
{
	BEGIN_DACOM_MAP_INBOUND(SysMap)
	DACOM_INTERFACE_ENTRY(ISystemMap)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	END_DACOM_MAP()

	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;
	COMPTR<IDrawAgent> frameShp;

	BOOL32 bIgnoreUpdate:1;
	U32 menuID;
	U32 eventHandle;
	SYSMAP_MODE sysmapMode;
	BOOL32 bHasFocus:1;
	BOOL32 bCameraDragActive;	// right button down

	BOOL32 bCameraDataChanged;
	Vector p[4];					// rotated world coordinates on viewable area
	U32 scrollRemainderX, scrollRemainderY;  // fractionall part of scrolling not taken
	
	S32 nViewWidth, nViewHeight;		// screen width,height of thumbnail view
	S32 lastFrameTime;				// frame period, in microseconds
	U64 lastMapTiming;

	const Transform *trans2;
	Transform trans;

	// fancy map tricks
	SINGLE zoneTime,zoneRate;
//	S32 zoneX,zoneY;
	Vector zonePos;
	BOOL32 bZoning:1;
	U32 zoneSys;
//	COMPTR<IDrawAgent> zoneFlash;
	COMPTR<IDrawAgent> zeroDegShape;
	U32 compassTextureID,sysRingTexID;

	RECT systemRect;
	RECT sysMapRect;
	RECT thumbRect;

	bool bLeftButtonValid;
	bool bRightButtonValid;
	bool bRallyEnabled;				// true if we are in rally point mode

	// lasso stuff
	S32 oldMouseX, oldMouseY;

	//fullscreen stuff
	int lit_sys;
	RECT currentRect;
	SINGLE zoom;
//	S32 xorg,yorg;
	S32 x_cent,y_cent;

	//new gdi draw stuff
	U32 terrainTex[MAX_SYSTEMS]; //the cached terrain texture
	U32 unitTex[MAX_SYSTEMS]; // the real texture
	U32 texInit; //current systems 
	SINGLE lastUpdateTime[MAX_SYSTEMS]; //last game time update
	bool bInMapRender;//true if our hdc is valid and it is safe to call map render functions
	SINGLE sysWidth;
	HDC hdc;
	U32 currentTex; //the current texture for the HDC

	//background texture info
	U32 backTex;
	U32 mapTexW, mapTexH;

	//icon management
	U32 numIcons;
	U32 iconId[MAX_ICONS];
	U32 numPlayerIcons;
	U32 playerIconId[MAX_PLAYERS][MAX_PLAYER_ICONS];

	AnimInstance * waypointAnim;
	AnimArchetype * waypointArch;
	AnimInstance * missionAnim;
	AnimArchetype * missionArch;
	U32 waypointFrame;

	AttackBubble * attackBubbles[MAX_SYSTEMS];

	MissionAnim * missionAnimList;
	U32 lastAnimID;

	GRIDVECTOR systemPingPos[MAX_PLAYERS][MAX_SYSTEMS];
	SINGLE systemPingTime[MAX_PLAYERS][MAX_SYSTEMS];

	//------------------------

	SysMap (void);

	~SysMap (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ISysMap methods */

	DEFMETHOD(SetRect) (struct tagRECT * rect);

	virtual void GetSysMapRect (int sys,struct tagRECT * rect);

	DEFMETHOD(ScrollUp) (BOOL32 bSingleStep);

	DEFMETHOD(ScrollDown) (BOOL32 bSingleStep);

	DEFMETHOD(ScrollLeft) (BOOL32 bSingleStep);

	DEFMETHOD(ScrollRight) (BOOL32 bSingleStep);

	DEFMETHOD(ZoneOn) (const Vector & pos);

	virtual void RegisterAttack(U32 systemID, GRIDVECTOR pos, U32 playerID);

	virtual S32 GetLastFrameTime();

	virtual U64 GetMapTiming()
	{
		return lastMapTiming;
	}

	virtual const Transform &GetMapTrans ();

	virtual void SetScrollRate(SINGLE rate)
	{
		DEFAULTS->GetDefaults()->scrollRate = rate;
	}

	virtual SINGLE GetScrollRate (void)
	{
		return DEFAULTS->GetDefaults()->scrollRate;
	}

	virtual void BeginFullScreen (void)
	{
		bHasFocus = 1;
	}

	virtual void DrawCircle(Vector worldPos, SINGLE worldSize, COLORREF color);

	virtual void DrawRing(Vector worldPos, SINGLE worldSize, U32 width, COLORREF color);

	virtual void DrawSquare(Vector worldPos, SINGLE worldSize, COLORREF color);

	virtual void DrawHashSquare(Vector worldPos, SINGLE worldSize, COLORREF color);

	virtual void DrawArc(Vector worldPos,SINGLE worldSize,SINGLE ang1, SINGLE ang2, U32 width, COLORREF color);

	virtual void DrawWaypoint(Vector worldPos, U32 frameMod);

	virtual void DrawMissionAnim(Vector worldPos, U32 frameMod);

	virtual void DrawIcon(Vector worldPos,SINGLE worldSize,U32 iconID);

	virtual void DrawPlayerIcon(Vector worldPos,SINGLE worldSize,U32 iconID,U32 playerID);

	virtual U32 RegisterIcon(char * filename);

	virtual U32 RegisterPlayerIcon(char * filename);

	virtual U32 GetPadding(U32 systemID);

	virtual void PingSystem(U32 systemID, Vector position, U32 playerID);

	virtual U32 CreateMissionAnim(U32 systemID, GRIDVECTOR position);
	
	virtual void StopMissionAnim(U32 animID);

	virtual void FlushMissionAnims();

	virtual void Save(struct IFileSystem * outFile);

	virtual void Load(struct IFileSystem * inFile);

	virtual void InvalidateMap(U32 systemID);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	/* SysMap methods */

	void OnNoOwner (void)
	{
	}

	void endFrame (void);

	BOOL32 createViewer (void);

	BOOL32 updateViewer (void);

	void updateSysMap (void);

	void drawNormalMap ();

	void drawFullScreenMap ();

	void resetBubbles(U32 systemID);

	void mergeNextBubble(AttackBubble & targetBubble, U32 systemID);

	bool getNextBubble(AttackBubble & targetBubble, U32 systemID);

	void updateTerrainMap(U32 systemID);

	void drawSystem (int sys,int x,int y,int sizex,int sizey);

	void drawSystemScaled (int sys,int x,int y,int size);

//	void draw2D ();

	void cursorInSys (const RECT & srect,S32 x, S32 y,SYSMAP_MODE &newMode, Vector &movePos);

	void updateResourceState (SYSMAP_MODE newMode);

	void updateCursorState (S32 x, S32 y);

	void onMouseWheel (S32 zDelta);

	bool onLeftButtonDown (S32 x, S32 y);

	void onLeftButtonUp (S32 x, S32 y, U32 wParam);

	void onRightButtonDown (S32 x, S32 y);

	void onRightButtonUp (S32 x, S32 y, U32 wParam);

	void thumbToWorld (S32 x, S32 y, Vector & pos);	// convert from screen coordinates to world coordinates

	void sysMapToWorld (S32 x, S32 y, Vector & result, bool bRotated=0);

	void updateCameraDrag (S32 x, S32 y);

	void destroyGDIObjects();

	void createGDIObjects();
	
	void loadTextures (bool bEnable);

	void loadInterface (IShapeLoader *);

	void drawCompass(int sys,int x,int y,int sizex,int sizey);

	static bool isInside (const Vector & pt, const Vector & p0, const Vector & p1);

	void GetMapRectForSystem(int sys,struct tagRECT *rect);

	void MakeFullScreenMapTrans();

	void onFullScreenScroll (const Vector & diff);

	BOOL32 mouseHasMoved (S32 x, S32 y) const
	{
		return ( (abs(x - oldMouseX) > S32(MOVE_DISTANCEX)) || (abs(y - oldMouseY) > S32(MOVE_DISTANCEY)) );
	}

	IDAComponent * getBase (void)
	{
		return (ISystemMap *) this;
	}

	//assumes a square rect.
	inline bool inCircle(S32 x, S32 y, const RECT &rect)
	{
		S32 width = (rect.right-rect.left)/2;
		width = width*width;
		S32 xVal = ((rect.right+rect.left)/2)-x;
		S32 yVal = ((rect.bottom+rect.top)/2)-y;
		S32 dist = xVal*xVal+yVal*yVal;
		return dist < width;
	}
};
//--------------------------------------------------------------------------//
//
//#define DEFAULT_SIZE 49999
SysMap::SysMap (void) 
{
	bCameraDataChanged = bHasFocus = 1;
	resPriority = SYSMAP_PRIORITY;
	
/*	systemRect.left = 0;
	systemRect.right =  DEFAULT_SIZE;
	systemRect.top =  DEFAULT_SIZE;
	systemRect.bottom = 0;*/
	drawSystemLines = false;
	drawGrid = false;

/*	thumbRect.left   = SYSMAP_LEFT;
	thumbRect.right  = SYSMAP_RIGHT;
	thumbRect.top    = SYSMAP_TOP;
	thumbRect.bottom = SYSMAP_BOTTOM;*/

	currentRect = thumbRect;
	zoom = 1.0;
	x_cent = y_cent = 100000;

	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		attackBubbles[i] = 0;	
	}
	missionAnimList = NULL;
}
//--------------------------------------------------------------------------//
//
SysMap::~SysMap (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (TOOLBAR && TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	DEFAULTS->SetDataInRegistry(szRegKey, static_cast<SYSMAP_DATA *>(this), sizeof(SYSMAP_DATA));

	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		while(attackBubbles[i])
		{
			AttackBubble * bub = attackBubbles[i];
			attackBubbles[i] = attackBubbles[i]->next;
			delete bub;
		}
	}

	while(missionAnimList)
	{
		MissionAnim * delMe = missionAnimList;
		missionAnimList = missionAnimList->next;
		delete delMe;
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT SysMap::SetRect (struct tagRECT * rect)
{
	systemRect.left = rect->left;
	systemRect.right = rect->right;
	systemRect.top = rect->top;
	systemRect.bottom = rect->bottom;

	S32 padding =  (TEX_PAD_RATIO*(rect->right-rect->left));//((GRIDSIZE*TGRID_SIZE)-(rect->right-rect->left))/2;

	sysMapRect.left = rect->left-padding;
	sysMapRect.right = rect->right+padding;
	sysMapRect.top = rect->top+padding;
	sysMapRect.bottom = rect->bottom-padding;

	updateViewer();

	bCameraDataChanged = TRUE;

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void SysMap::GetSysMapRect (int sys,struct tagRECT * rect)
{
	RECT sysRect;
	SECTOR->GetSystemRect(sys,&sysRect,0);
	S32 padding = (TEX_PAD_RATIO*(sysRect.right-sysRect.left));//((GRIDSIZE*TGRID_SIZE)-(sysRect.right-sysRect.left))/2;
	rect->left    = sysRect.left-padding;
	rect->right	  = sysRect.right+padding;
	rect->top	  = sysRect.top+padding;
	rect->bottom  = sysRect.bottom-padding;

}
//--------------------------------------------------------------------------//
//
GENRESULT SysMap::ScrollUp (BOOL32 bSingleStep)
{
	if (CQFLAGS.bFullScreenMap)
	{
		Vector vec(0, 1, 0);
		onFullScreenScroll(vec);
	}
	else
	{
	 	Vector pos = CAMERA->GetRotatedPosition();

		if (bSingleStep)
		{
			pos.y += SCROLL_Y;
			scrollRemainderY=scrollRemainderX=0;
		}
		else
		{
			U32 scrollRate = DEFAULTS->GetDefaults()->scrollRate * fabs(p[2].y - p[1].y);
			U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderY;
			
			pos.y += value >> 10;
			scrollRemainderY = value & 1023;
		}
		CAMERA->SetRotatedPosition(&pos, 1);
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT SysMap::ScrollDown (BOOL32 bSingleStep)
{
	if (CQFLAGS.bFullScreenMap)
	{
		Vector vec(0, -1, 0);
		onFullScreenScroll(vec);
	}
	else
	{
	 	Vector pos = CAMERA->GetRotatedPosition();

		if (bSingleStep)
		{
			pos.y -= SCROLL_Y;
			scrollRemainderY=scrollRemainderX=0;
		}
		else
		{
			U32 scrollRate = DEFAULTS->GetDefaults()->scrollRate * fabs(p[2].y - p[1].y);
			U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderY;
			
			pos.y -= value >> 10;
			scrollRemainderY = value & 1023;
		}
		CAMERA->SetRotatedPosition(&pos, 1);
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT SysMap::ScrollLeft (BOOL32 bSingleStep)
{
	if (CQFLAGS.bFullScreenMap)
	{
		Vector vec(-1, 0, 0);
		onFullScreenScroll(vec);
	}
	else
	{
	 	Vector pos = CAMERA->GetRotatedPosition();

		if (bSingleStep)
		{
			pos.x -= SCROLL_X;
			scrollRemainderY=scrollRemainderX=0;
		}
		else
		{
			U32 scrollRate = DEFAULTS->GetDefaults()->scrollRate * fabs(p[1].x - p[0].x);
			U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderX;
			
			pos.x -= value >> 10;
			scrollRemainderX = value & 1023;
		}
		CAMERA->SetRotatedPosition(&pos, 1);
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT SysMap::ScrollRight (BOOL32 bSingleStep)
{
	if (CQFLAGS.bFullScreenMap)
	{
		Vector vec(1, 0, 0);
		onFullScreenScroll(vec);
	}
	else
	{
	 	Vector pos = CAMERA->GetRotatedPosition();

		if (bSingleStep)
		{
			pos.x += SCROLL_X;
			scrollRemainderY=scrollRemainderX=0;
		}
		else
		{
			U32 scrollRate = DEFAULTS->GetDefaults()->scrollRate * fabs(p[1].x - p[0].x);
			U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderX;
			
			pos.x += value >> 10;
			scrollRemainderX = value & 1023;
		}
		CAMERA->SetRotatedPosition(&pos, 1);
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
// do the map trick anim doobie
//
GENRESULT SysMap::ZoneOn (const Vector & pos)
{
	//zoneX = x-thumbRect.left;
	//zoneY = y-thumbRect.top;
	zonePos = pos;
	zoneTime = 0;
	zoneRate = 0.0000015;
	bZoning = TRUE;
	zoneSys = SECTOR->GetCurrentSystem();

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void SysMap::RegisterAttack(U32 systemID, GRIDVECTOR pos,U32 playerID)
{
	if(playerID != MGlobals::GetThisPlayer())
		return;

/*	AttackBubble * bubble = attackBubbles[systemID-1];
	while(bubble)
	{
		SINGLE dist = (bubble->pos-pos);
		if(dist <= bubble->radius+ATTACK_DEFAULT_RAD)//do I merge these bubbles
		{
			bubble->lifetime = ATTACK_DEFAULT_LIFETIME;
			if(dist+ATTACK_DEFAULT_RAD > bubble->radius)//does this bubble expand us
			{
				Vector vPos1 = bubble->pos;
				Vector vPos2 = pos;
				Vector dir = vPos1-vPos2;
				dir.normalize();
				vPos1 = vPos1+(dir*bubble->radius*GRIDSIZE);
				vPos2 = vPos2-(dir*ATTACK_DEFAULT_RAD*GRIDSIZE);
				dir = vPos1-vPos2;
				dir = dir*0.5;
				bubble->radius = dir.fast_magnitude()/GRIDSIZE;
				bubble->pos = vPos2+dir;
			}

			return;		
		}
		bubble = bubble->next;
	}
*/	//must be a new bubble
	if(systemID >0 && systemID <= MAX_SYSTEMS)
	{
		AttackBubble * bubble = new AttackBubble;
		bubble->next = attackBubbles[systemID-1];
		bubble->radius = ATTACK_DEFAULT_RAD;
		bubble->pos = pos;
		bubble->lifetime = ATTACK_DEFAULT_LIFETIME;
		attackBubbles[systemID-1] = bubble;
	}
}
//--------------------------------------------------------------------------//
//
S32 SysMap::GetLastFrameTime ()
{
	return lastFrameTime;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT SysMap::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_SYSTEM_CHANGED:
		InvalidateMap(U32(param));
		updateTerrainMap(U32(param));
		break;
	case CQE_LOAD_INTERFACE:
		{
			COMPTR<IToolbar> toolbar;

			if (TOOLBAR && TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
			{
				toolbar->GetSystemMapRect(sysmap_left,sysmap_top,sysmap_right,sysmap_bottom);\
			}

			thumbRect.left = IDEAL2REALX(SYSMAP_LEFT);  
			thumbRect.right = IDEAL2REALX(SYSMAP_LEFT+SYSMAP_SIZE);
			thumbRect.top = IDEAL2REALY(SYSMAP_TOP);
			thumbRect.bottom = IDEAL2REALY(SYSMAP_TOP+SYSMAP_SIZE);

			currentRect = thumbRect;

			loadInterface((IShapeLoader*)param);
				
			zoom = 1.0;
			SECTOR->GetSectorCenter(&x_cent,&y_cent);
			MakeFullScreenMapTrans();
		}
		break;

	case CQE_CAMERA_MOVED:
		bCameraDataChanged = 1;
		break;

	case CQE_KILL_FOCUS:
		if (bHasFocus && (SysMap *)param != this)
		{
			desiredOwnedFlags = 0;
			releaseResources();
			bHasFocus = 0;
		}
		break;
	case CQE_SET_FOCUS:
		if ((SysMap *)param != this)
		{
			bHasFocus = 1;
		}
		break;

	case CQE_UI_RALLYPOINT:
		bRallyEnabled = (param != 0);
		break;

	case CQE_ENDFRAME:
		endFrame();
		break;
	
	case WM_CLOSE:
		if (viewer)
			viewer->set_display_state(0);
		break;

	case WM_LBUTTONDOWN:
		if (bHasFocus)
		{
			if (onLeftButtonDown(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam))))
			{
				return GR_GENERIC; // eat the mouse down
			}
		}
		break;

	case WM_LBUTTONUP:
		if (bHasFocus)
		{
			onLeftButtonUp(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
		}
		break;

	case WM_RBUTTONDOWN:
		if (bHasFocus)
		{
			onRightButtonDown(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
		}
		break;

	case WM_RBUTTONUP:
		if (bHasFocus)
		{
			onRightButtonUp(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
		}
		break;
		
	case WM_MOUSEWHEEL:
		if (bHasFocus && CQFLAGS.bFullScreenMap && CQFLAGS.bGameActive && ownsResources())
		{
			onMouseWheel(short(HIWORD(msg->wParam)));

			//really incredibly dirty and underhanded and prone to breakage
			return GR_GENERIC;
		}
		break;

	case CQE_DEBUG_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch ((U32)param)
			{
			case IDH_VIEWSECTOR:
				if (viewer)
					viewer->set_display_state(1);
				break;
			}
		}
		break;

	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			if (viewer)
				viewer->set_display_state(1);
		}
		break;

	case CQE_RENDER_LAST3D:
		if (CQFLAGS.bGameActive)
		{
			CQASSERT(CQFLAGS.b3DEnabled);

			BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_OPAQUE);//RP_OPAQUE | RP_TRANSLUCENT_UNSORTED);

			{
				S32 x, y;
				WM->GetCursorPos(x, y);
				updateCursorState(x, y);
			}
			U64 pretick,posttick;
			QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
			lastFrameTime = S32(param);
			if (bZoning)
			{
				zoneTime += lastFrameTime*zoneRate;
				if (zoneTime > 2.0)
					bZoning = FALSE;
			}
			if(waypointArch)
				waypointFrame = (waypointFrame+1)%(waypointArch->frame_cnt*3);
			if (bCameraDataChanged) 
				updateSysMap();
			
			if((!CQFLAGS.bGamePaused) && (!DEFAULTS->GetDefaults()->bEditorMode))
			{
				SINGLE dt = OBJLIST->GetRealRenderTime();
				U32 i;
				for(i = 0; i < MAX_SYSTEMS; ++i)
				{
					AttackBubble * bubble = attackBubbles[i];
					AttackBubble * prev = NULL;
					while(bubble)
					{
						bubble->lifetime -= dt;
						if(bubble->lifetime <= 0)
						{
							AttackBubble * delBub = bubble;
							bubble = bubble->next;
							if(prev)
								prev->next = bubble;	
							else
								attackBubbles[i] = bubble;
							delete delBub;
						}
						else
						{
							prev = bubble;
							bubble = bubble->next;
						}
					}
				}
				for(i=0; i < MAX_PLAYERS; ++ i)
				{
					for(U32 j = 0; j < MAX_SYSTEMS; ++ j)
					{
						if(systemPingTime[i][j] != 0.0)
						{
							systemPingTime[i][j] -= dt;
							if(systemPingTime[i][j] < 0)
							{
								systemPingTime[i][j] = 0;
							}
						}
					}
				}
			}

			if(CQFLAGS.bFullScreenMap)
			{
				bool bUpdated = false;
				for(int i = 0; i < SECTOR->GetNumSystems(); ++i)
				{
					lastUpdateTime[i] += OBJLIST->GetRealRenderTime();
					if(lastUpdateTime[i] > 1.0 && !bUpdated)
					{
						bUpdated = true;
						lastUpdateTime[i] = 0;
						updateTerrainMap(i+1);
					}
					else if(lastUpdateTime[i] > 2.0)
					{
						lastUpdateTime[i] = 0;
						updateTerrainMap(i+1);
					}
				}
			}
			else if(!(CQFLAGS.bNoToolbar))
			{
				U32 sysID = SECTOR->GetCurrentSystem();
				lastUpdateTime[sysID] += OBJLIST->GetRealRenderTime();
				if(lastUpdateTime[sysID] > 1.0)
				{
					lastUpdateTime[sysID] = 0;
					updateTerrainMap(sysID);
				}
			}
			
			if (CQFLAGS.bFullScreenMap)
				drawFullScreenMap();
			else
			if (CQFLAGS.bNoToolbar==0)
				drawNormalMap();
			
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			lastMapTiming = posttick-pretick;
		}
		break;  // end case CQE_UPDATE

	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
// receive notifications that document has changed
//
GENRESULT SysMap::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	if (bIgnoreUpdate == 0)
	{
		doc->SetFilePointer(0,0);
		doc->ReadFile(0, static_cast<SYSMAP_DATA *>(this), sizeof(SYSMAP_DATA), &dwRead, 0);
	}
	bCameraDataChanged = 1;

	return GR_OK;
}
//--------------------------------------------------------------------------//
// camera has moved, update scroll bars and other things
//
void SysMap::updateSysMap (void)
{
	if ((bCameraDataChanged = CAMERA->PaneToPoints(p[0], p[1], p[2], p[3])) != 0)
	{
		RECT rect;
		if (lit_sys)
		{
			GetMapRectForSystem(lit_sys,&rect);
			currentRect = rect;
		}
		else
			currentRect = thumbRect;

		bCameraDataChanged = 0;
	}
}

/*void SysMap::draw2D ()
{
	if (bZoning && zoneSys == SECTOR->GetCurrentSystem())
	{
		Vector pos = trans2->rotate_translate(zonePos);
		if (zoneTime > 1.0)
		{
			if (fmod(zoneTime*4,1) < 0.5)
			{
				OrthoView();
				zoneFlash->Draw(0,pos.x-5,pos.y-5);
			}
		}
	}
}*/

//--------------------------------------------------------------------------//
//
struct WayPointEnum: public IAgentEnumerator
{
	U32 systemID;
	U32 frameMod;

	WayPointEnum(U32 _sysID)
	{
		systemID = _sysID;
		frameMod = 0;
	}

	virtual bool EnumerateAgent (const NODE & node)
	{
		if(node.targetSystemID == systemID)
		{
			SYSMAP->DrawWaypoint(node.targetPosition,frameMod);	
			++frameMod;
		}
		return frameMod < 10;
	}
};
//--------------------------------------------------------------------------//
//
void SysMap::drawNormalMap ()
{
	U32 currentSystemID = SECTOR->GetCurrentSystem();
	PANE tPane;
	tPane.window = 0;		// not needed
	tPane.x0 = thumbRect.left;
	tPane.x1 = thumbRect.right;
	tPane.y0 = thumbRect.top;
	tPane.y1 = thumbRect.bottom;
//	OrthoView(&tPane);
	OrthoView();

	//draw a kludgey little rectangle
	int width,height;
	width = thumbRect.right-thumbRect.left;
	height = thumbRect.bottom-thumbRect.top;
	DisableTextures();
	PIPE->set_render_state(D3DRS_ZENABLE,0);
	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,0);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PB.Color3ub(0,0,0);
	PB.Begin(PB_QUADS);
	PB.Vertex3f(tPane.x1-width*0.25, tPane.y0,0);
	PB.Vertex3f(tPane.x0+width*0.25, tPane.y0,0);
	PB.Vertex3f(tPane.x0+width*0.1, tPane.y0+height*0.2,0);
	PB.Vertex3f(tPane.x1-width*0.1, tPane.y0+height*0.2,0);
	PB.End();
	//

	int centerX = (thumbRect.right+thumbRect.left)*0.5;
	int centerY = (thumbRect.bottom+thumbRect.top)*0.5;

//	width = min(SYSMAP_RIGHT-SYSMAP_LEFT,SYSMAP_BOTTOM-SYSMAP_TOP);
	drawSystem(currentSystemID,centerX,centerY,IDEAL2REALX(SYSMAP_SIZE),IDEAL2REALY(SYSMAP_SIZE));

	// draw the viewable area of the sector
	S32 x[4];
	S32 y[4];
	
	SINGLE xConv,yConv;
	xConv = (SINGLE)(thumbRect.right-thumbRect.left) / (sysMapRect.right-sysMapRect.left);//+1);
	yConv = (SINGLE)(thumbRect.top-thumbRect.bottom) / (sysMapRect.top-sysMapRect.bottom);//+1);
	x[0] = (S32(p[0].x - sysMapRect.left) * xConv)+tPane.x0;
	y[0] = (S32(p[0].y - sysMapRect.top) * yConv)+tPane.y0;

	x[1] = (S32(p[1].x - sysMapRect.left) * xConv)+tPane.x0;
	y[1] = (S32(p[1].y - sysMapRect.top) * yConv)+tPane.y0;

	x[2] = (S32(p[2].x - sysMapRect.left) * xConv)+tPane.x0;
	y[2] = (S32(p[2].y - sysMapRect.top) * yConv)+tPane.y0;

	x[3] = (S32(p[3].x - sysMapRect.left) * xConv)+tPane.x0;
	y[3] = (S32(p[3].y - sysMapRect.top) * yConv)+tPane.y0;

/*	PIPE->set_render_state(D3DRS_ZENABLE,0);
	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,0);*/
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	DisableTextures();

	PB.Color3ub(60, 60, 60);

	PB.Begin(PB_QUADS);
	PB.Vertex3f(x[3]+0.5,y[3]+0.5,0);
	PB.Vertex3f(x[0]+0.5,y[0]+0.5,0);
	PB.Vertex3f(x[1]+0.5,y[1]+0.5,0);
	PB.Vertex3f(x[2]+0.5,y[2]+0.5,0);

	PB.End();

	drawCompass(0,centerX,centerY,IDEAL2REALX(SYSMAP_SIZE),IDEAL2REALY(SYSMAP_SIZE));

	DisableTextures();

	if (DEFAULTS->GetDefaults()->bDrawHotrects)
	{
		PB.Color3ub(255,255,255);
		PB.Begin(PB_LINES);
		PB.Vertex3f(thumbRect.right+0.5, thumbRect.top+0.5,0);
		PB.Vertex3f(thumbRect.left+0.5, thumbRect.top+0.5,0);
		
		PB.Vertex3f(thumbRect.right+0.5, thumbRect.top+0.5,0);
		PB.Vertex3f(thumbRect.right+0.5, thumbRect.bottom+0.5,0);
		
		PB.Vertex3f(thumbRect.right+0.5, thumbRect.bottom+0.5,0); 
		PB.Vertex3f(thumbRect.left+0.5, thumbRect.bottom+0.5,0);
		
		PB.Vertex3f(thumbRect.left+0.5, thumbRect.bottom+0.5,0); 
		PB.Vertex3f(thumbRect.left+0.5, thumbRect.top+0.5,0);
		PB.End();
	}

	if (bZoning)
	{
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		OrthoView();
		Vector pos = trans2->rotate_translate(zonePos);
		if (zoneTime <= 1.0)
		{
			pos.x -= thumbRect.left;
			pos.y -= thumbRect.top;
			PB.Color3ub(0,255,255);
			PB.Begin(PB_LINES);
			PB.Vertex3f(thumbRect.left+zoneTime*pos.x,thumbRect.top,0);
			PB.Vertex3f(thumbRect.left+zoneTime*pos.x,thumbRect.bottom,0);
			
			PB.Vertex3f(thumbRect.left,thumbRect.top+zoneTime*pos.y,0);
			PB.Vertex3f(thumbRect.right,thumbRect.top+zoneTime*pos.y,0);
			PB.End();
		}
		else
		{
			pos.x -= thumbRect.left;
			pos.y -= thumbRect.top;
			PB.Color3ub(0,255,255);
			PB.Begin(PB_LINES);
			PB.Vertex3f(thumbRect.left+pos.x,thumbRect.top,0);
			PB.Vertex3f(thumbRect.left+pos.x,thumbRect.bottom,0);
			
			PB.Vertex3f(thumbRect.left,thumbRect.top+pos.y,0);
			PB.Vertex3f(thumbRect.right,thumbRect.top+pos.y,0);
			PB.End();
		}
	}

	//render waypoints
	WayPointEnum wayEnum(currentSystemID);
	ObjSet set;
	IBaseObject * selected = OBJLIST->GetSelectedList();

	while (selected)
	{
		set.objectIDs[set.numObjects++] = selected->GetPartID();
		selected = selected->nextSelected;
	}

	CQASSERT(set.numObjects <= MAX_SELECTED_UNITS);

	if(set.numObjects)
		THEMATRIX->EnumerateQueuedMoveAgents(set, &wayEnum);

	//render rally Points;
	selected = OBJLIST->GetSelectedList();
	if(selected && (!(selected->nextSelected)))
	{
		MPart part(selected);
		if(part.isValid() && part->caps.buildOk)
		{
			VOLPTR(IPlatform) plat = selected;
			if(plat)
			{
				NETGRIDVECTOR vect;
				if(plat->GetRallyPoint(vect))
					if(vect.systemID == currentSystemID)
						SYSMAP->DrawWaypoint(vect,0);	
			}
		}
	}
				
	//render mission animations
	MissionAnim * anim = missionAnimList;
	U32 frameMod = 0;
	while(anim)
	{
		if(anim->systemID == currentSystemID)
		{
			SYSMAP->DrawMissionAnim(anim->pos,frameMod);
			++frameMod;
		}
		anim = anim->next;
	}
}

void SysMap::MakeFullScreenMapTrans()
{
#define SPACEX S32(SCREENRESX)	//IDEAL2REALX(320)
#define SPACEY S32(SCREENRESY)	//IDEAL2REALY(320)

	S32 sctr_sizex,sctr_sizey,scrt_size;
	SECTOR->GetSectorCenter(&sctr_sizex,&sctr_sizey);
	if(sctr_sizex > sctr_sizey)
		scrt_size = sctr_sizex;
	else
		scrt_size = sctr_sizey;
	scrt_size*=2/zoom;

	//scary idea
//	sctr_sizex += 2*PADDING*100000;
//	sctr_sizey += 2*PADDING*100000;
	
	PANE tPane;
	tPane.window = NULL;
	tPane.x0 = 0;
	tPane.y0 = 0;
	tPane.x1 = SPACEX-1;
	tPane.y1 = SPACEY-1;
//	OrthoView(&tPane);
	OrthoView();
	
	SINGLE xConv = SINGLE(SPACEX)/scrt_size;
	SINGLE yConv = -xConv;//-SINGLE(SPACEY)/sctr_sizey;

	//this should be the quickest way to compose the rotation knowing what we know
	SINGLE rotation = CAMERA->GetWorldRotation();

	TRANSFORM transa,transb;
	//compose rotation
	transb.rotate_about_k(-rotation * MUL_DEG_TO_RAD);

	//do the first translation
	transa.set_position(Vector(-x_cent,-y_cent,0));
	trans = transb.multiply(transa);

	//the transform back is a simple add
//	trans.translation.x += x_cent;
//	trans.translation.y += y_cent;

	//factor in all the scales
	trans.d[0][0] *= xConv;
	trans.d[0][1] *= xConv;
	trans.d[0][2] *= xConv;
	trans.translation.x *= xConv;
				
	trans.d[1][0] *= yConv;
	trans.d[1][1] *= yConv;
	trans.d[1][2] *= yConv;
	trans.translation.y *= yConv;

	trans.translation.x += SPACEX/2;
	trans.translation.y += SPACEY/2;
}

void SysMap::drawFullScreenMap ()
{

	int numSystems = SECTOR->GetNumSystems();
	int currentSysID = SECTOR->GetCurrentSystem();

	S32 sctr_sizex,sctr_sizey,scrt_size;
	SECTOR->GetSectorCenter(&sctr_sizex,&sctr_sizey);
	if(sctr_sizex > sctr_sizey)
		scrt_size = sctr_sizex;
	else
		scrt_size = sctr_sizey;
	scrt_size*=2/zoom;

	//scary idea
//	sctr_sizex += 2*PADDING*100000;
//	sctr_sizey += 2*PADDING*100000;
	
	PANE tPane;
	tPane.window = NULL;
	tPane.x0 = 0;
	tPane.y0 = 0;
	tPane.x1 = SPACEX-1;
	tPane.y1 = SPACEY-1;
//	OrthoView(&tPane);
	OrthoView();
	
	MakeFullScreenMapTrans();
	
	for (int s=1;s<numSystems+1;s++)
	{
		if (SECTOR->IsVisibleToPlayer(s,MGlobals::GetThisPlayer()) || 
			(DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn))
		{
			RECT sysRect;
			SECTOR->GetSystemRect(s,&sysRect,1);
			

			Vector pos;
			pos.x = (sysRect.right+sysRect.left)*0.5;
			pos.y = (sysRect.top+sysRect.bottom)*0.5;
			pos.z = 0;
			pos = trans*pos;
//			int width = (sysRect.right-sysRect.left)*320/sctr_sizex;		(jy)
			int width = (sysRect.right-sysRect.left)*SCREEN_WIDTH/scrt_size;

			
			//new convention, height is positive
		//	int height = -(sysRect.bottom-sysRect.top)*SPACEY/sctr_sizey;

		//	width = height = min(width,height);
			//width is in ideal coords
//			drawSystem(s,pos.x,pos.y,width);
			int maxWidth = (GRIDSIZE*TGRID_SIZE)*SCREEN_WIDTH/scrt_size;
			drawSystemScaled(s,pos.x,pos.y,maxWidth);

			drawCompass(s,pos.x,pos.y,IDEAL2REALX(width)*1.2,IDEAL2REALY(width)*1.2);
			
			if (s == currentSysID)
			{
				//can't draw a compass?
				//drawCompass();
				
				// draw the viewable area of the sector
				Vector pp[4];
				S32 padding = 0;//((GRIDSIZE*TGRID_SIZE)-(sysRect.right-sysRect.left))/2;
			
				S32 left = -padding;
				S32 top = sysRect.top-sysRect.bottom+padding;

				SINGLE scrn_left=pos.x-IDEAL2REALX(width)*0.5;
				SINGLE scrn_top=pos.y-IDEAL2REALY(width)*0.5;

				SINGLE sys_xConv,sys_yConv;
				sys_xConv = (SINGLE)IDEAL2REALX(width) / (sysRect.right-sysRect.left+2*padding);//+1);
				sys_yConv = -(SINGLE)IDEAL2REALY(width) / (sysRect.top-sysRect.bottom+2*padding);//+1);
				pp[0].x = (S32(p[0].x - left) * sys_xConv)+scrn_left;
				pp[0].y = (S32(p[0].y - top) * sys_yConv)+scrn_top;
				
				pp[1].x = (S32(p[1].x - left) * sys_xConv)+scrn_left;
				pp[1].y = (S32(p[1].y - top) * sys_yConv)+scrn_top;
				
				pp[2].x = (S32(p[2].x - left) * sys_xConv)+scrn_left;
				pp[2].y = (S32(p[2].y - top) * sys_yConv)+scrn_top;
				
				pp[3].x = (S32(p[3].x - left) * sys_xConv)+scrn_left;
				pp[3].y = (S32(p[3].y - top) * sys_yConv)+scrn_top;

				
				PIPE->set_render_state(D3DRS_ZENABLE,0);
				PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
				PIPE->set_render_state(D3DRS_ZWRITEENABLE,0);
				PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
				PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
				DisableTextures();
				
				PB.Color3ub(60, 60, 60);
				
				PB.Begin(PB_QUADS);
				PB.Vertex3f(pp[3].x+0.5,pp[3].y+0.5,0);
				PB.Vertex3f(pp[0].x+0.5,pp[0].y+0.5,0);
				PB.Vertex3f(pp[1].x+0.5,pp[1].y+0.5,0);
				PB.Vertex3f(pp[2].x+0.5,pp[2].y+0.5,0);
				
				PB.End();
				
			}
		}
	}

	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	SECTOR->DrawLinks(tPane,trans,1);
}


struct SysRECT : RECT
{
	SysRECT(LONG _left,LONG _top,LONG _right,LONG _bottom)
	{
		left = _left;
		top = _top;
		right = _right;
		bottom = _bottom;
	}
};

#define WORLD_TO_TEX(VALUE,SYSWIDTH) ((((VALUE)*(TMAP_SIZE-(2*TEX_PADDING)))/(SYSWIDTH))+TEX_PADDING)
#define SCALE_WORLD_TO_TEX(VALUE,SYSWIDTH) ((((VALUE)*(TMAP_SIZE-(2*TEX_PADDING)))/(SYSWIDTH)))
//--------------------------------------------------------------------------//
//
void SysMap::DrawCircle(Vector worldPos, SINGLE worldSize, COLORREF color)
{
	CQASSERT(bInMapRender);
	HBRUSH brush = CreateSolidBrush(color);
	HGDIOBJ oldBrush = SelectObject(hdc, brush);
	HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	SINGLE scaleSize = SCALE_WORLD_TO_TEX(worldSize,sysWidth);
	SINGLE smallest = ((((SINGLE)TMAP_SIZE)/IDEAL2REALY(SYSMAP_SIZE))*2.0)+1.0;
	if(scaleSize < smallest)
		scaleSize = smallest;
	Ellipse(hdc, centerX-(scaleSize/2), centerY-(scaleSize/2), centerX-(scaleSize/2)+scaleSize, centerY-(scaleSize/2)+scaleSize);
	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(brush);	
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawRing(Vector worldPos, SINGLE worldSize, U32 width, COLORREF color)
{
	CQASSERT(bInMapRender);
	HPEN pen = CreatePen(PS_SOLID,width,color);
	HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	HGDIOBJ oldPen = SelectObject(hdc, pen);

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth);
	Ellipse(hdc, centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize);
	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(pen);	
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawSquare(Vector worldPos, SINGLE worldSize, COLORREF color)
{
	CQASSERT(bInMapRender);
	HBRUSH brush = CreateSolidBrush(color);

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth);
	SysRECT rect(centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize);
	FillRect(hdc, &rect,brush);

	DeleteObject(brush);
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawHashSquare(Vector worldPos, SINGLE worldSize, COLORREF color)
{
	CQASSERT(bInMapRender);
	U32 oldMode = SetBkMode(hdc,TRANSPARENT);
	HBRUSH brush = CreateHatchBrush(HS_DIAGCROSS,color);
	HGDIOBJ oldBrush = SelectObject(hdc, brush);
	HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth);
	Rectangle(hdc, centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize);

	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	SetBkMode(hdc,oldMode);

	DeleteObject(brush);
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawIcon(Vector worldPos,SINGLE worldSize,U32 _iconID)
{
	CQASSERT(bInMapRender);

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth)+1;

	U32 texW,texH,l;
	SysRECT rect(centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize);
	if(PIPE->get_texture_dim(iconId[_iconID], &texW, &texH, &l)== GR_OK)
	{
		HDC tempDC;
		PIPE->get_texture_dc(iconId[_iconID], &tempDC);

		StretchBlt(hdc,centerX-scaleSize, centerY-scaleSize, scaleSize*2, scaleSize*2,
			tempDC,0,0,texW,texH,SRCCOPY);

		PIPE->release_texture_dc(iconId[_iconID], tempDC);
	}
	
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawArc(Vector worldPos,SINGLE worldSize,SINGLE ang1, SINGLE ang2, U32 width, COLORREF color)
{
	CQASSERT(bInMapRender);
	HPEN pen = CreatePen(PS_SOLID,width,color);
	HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	HGDIOBJ oldPen = SelectObject(hdc, pen);

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth);

	U32 endX1 = centerX+cos(ang1)*scaleSize;
	U32 endY1 = centerY+sin(ang1)*scaleSize;
	U32 endX2 = centerX+cos(ang2)*scaleSize;
	U32 endY2 = centerY+sin(ang2)*scaleSize;
	Arc(hdc, centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize,
		endX1,endY1,endX2,endY2);

	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(pen);	
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawWaypoint(Vector worldPos, U32 frameMod)
{

	if(waypointAnim)
	{
		//BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_state(RPR_BATCH,FALSE);
		U32 frameNum = ((waypointFrame/3)+frameMod)%waypointArch->frame_cnt;
		AnimFrame *frame = &waypointArch->frames[frameNum];
		SetupDiffuseBlend(frame->texture,TRUE);

		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		

		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_ZENABLE,false);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE, false);
		OrthoView();

		Vector pos = trans2->rotate_translate(worldPos);

		Vector v0,v1,v2,v3;
		SINGLE sizeX = IDEAL2REALX(3);
		SINGLE sizeY = IDEAL2REALY(3);
		v0 = pos + Vector(sizeX,sizeY,0);
		v1 = pos + Vector(-sizeX,sizeY,0);
		v2 = pos + Vector(-sizeX,-sizeY,0);
		v3 = pos + Vector(sizeX,-sizeY,0);

		PB.Begin(PB_QUADS);
		PB.TexCoord2f(frame->x0,frame->y0);  PB.Vertex3f(v0.x,v0.y,v0.z);
		PB.TexCoord2f(frame->x1,frame->y0);  PB.Vertex3f(v1.x,v1.y,v1.z);
		PB.TexCoord2f(frame->x1,frame->y1);  PB.Vertex3f(v2.x,v2.y,v2.z);
		PB.TexCoord2f(frame->x0,frame->y1);  PB.Vertex3f(v3.x,v3.y,v3.z);
		PB.End();
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawMissionAnim(Vector worldPos, U32 frameMod)
{
	if(missionAnim)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		U32 frameNum = ((waypointFrame/3)+frameMod)%missionArch->frame_cnt;
		AnimFrame *frame = &missionArch->frames[frameNum];
		SetupDiffuseBlend(frame->texture,TRUE);

		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_ZENABLE,false);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE, false);
		OrthoView();

		Vector pos = trans2->rotate_translate(worldPos);

		Vector v0,v1,v2,v3;
		SINGLE sizeX = IDEAL2REALX(8);
		SINGLE sizeY = IDEAL2REALY(8);
		v0 = pos + Vector(sizeX,sizeY,0);
		v1 = pos + Vector(-sizeX,sizeY,0);
		v2 = pos + Vector(-sizeX,-sizeY,0);
		v3 = pos + Vector(sizeX,-sizeY,0);

		PB.Begin(PB_QUADS);
		PB.TexCoord2f(frame->x0,frame->y0);  PB.Vertex3f(v0.x,v0.y,v0.z);
		PB.TexCoord2f(frame->x1,frame->y0);  PB.Vertex3f(v1.x,v1.y,v1.z);
		PB.TexCoord2f(frame->x1,frame->y1);  PB.Vertex3f(v2.x,v2.y,v2.z);
		PB.TexCoord2f(frame->x0,frame->y1);  PB.Vertex3f(v3.x,v3.y,v3.z);
		PB.End();
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::DrawPlayerIcon(Vector worldPos,SINGLE worldSize,U32 _iconID,U32 playerID)
{
	CQASSERT(bInMapRender);

	U32 centerX = WORLD_TO_TEX(worldPos.x,sysWidth);
	U32 centerY = WORLD_TO_TEX(worldPos.y,sysWidth);
	centerY = TMAP_SIZE-centerY;
	U32 scaleSize = SCALE_WORLD_TO_TEX(worldSize/2,sysWidth);

	U32 texW,texH,l;
	SysRECT rect(centerX-scaleSize, centerY-scaleSize, centerX+scaleSize, centerY+scaleSize);
	if(PIPE->get_texture_dim(playerIconId[playerID-1][_iconID], &texW, &texH, &l)== GR_OK)
	{
		HDC tempDC;
		PIPE->get_texture_dc(playerIconId[playerID-1][_iconID], &tempDC);

		StretchBlt(hdc,centerX-scaleSize, centerY-scaleSize, scaleSize*2, scaleSize*2,
			tempDC,0,0,texW,texH,SRCCOPY);

		PIPE->release_texture_dc(playerIconId[playerID-1][_iconID], tempDC);
	}
}
//--------------------------------------------------------------------------//
//
U32 SysMap::RegisterIcon(char * filename)
{
	CQASSERT(numIcons < MAX_ICONS);
	iconId[numIcons] = TMANAGER->CreateTextureFromFile(filename, TEXTURESDIR, DA::BMP, PF_4CC_DAOT);		// create opaque texture
	if(iconId[numIcons] == 0)
		return -1;
	++numIcons;
	return numIcons -1;
}
//--------------------------------------------------------------------------//
//
U32 SysMap::RegisterPlayerIcon(char * filename)
{
	CQASSERT(numPlayerIcons < MAX_ICONS);
	playerIconId[0][numPlayerIcons] = TMANAGER->CreateTextureFromFile(filename, TEXTURESDIR, DA::BMP, PF_4CC_DAOT);		// create opaque texture
	if(playerIconId[0][numPlayerIcons] == 0)
		return -1;
	for(U32 k = 0; k < numPlayerIcons; ++k)
	{
		if(playerIconId[0][k] == playerIconId[0][numPlayerIcons])
		{
			for(U32 j = 1; j < MAX_PLAYERS; ++j)
			{
				playerIconId[j][numPlayerIcons]= playerIconId[j][k];
			}
			++numPlayerIcons;
			return numPlayerIcons-1;
		}
	}
	U32 l,texW,texH;
	if (PIPE->get_texture_dim(playerIconId[0][numPlayerIcons], &texW, &texH, &l) == GR_OK)
	{
		PixelFormat pf;
		if (PIPE->get_texture_format(playerIconId[0][numPlayerIcons], &pf) == GR_OK)
		{
			for(U32 i = 1; i < MAX_PLAYERS; ++i)
			{
				if (PIPE->create_texture(texW, texH, pf, 0, 0, playerIconId[i][numPlayerIcons]) == GR_OK)
				{
					PIPE->blit_texture(playerIconId[i][numPlayerIcons], 0, SysRECT(0,0,texW,texH), playerIconId[0][numPlayerIcons], 0, SysRECT(0,0,texW,texH));
				}
			}
		}
	}
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		PixelFormat pf;
		if (PIPE->get_texture_format(playerIconId[i][numPlayerIcons], &pf) == GR_OK)
		{
			RPLOCKDATA lockData;
			PIPE->lock_texture(playerIconId[i][numPlayerIcons],0,&lockData);

			COLORREF color = COLORTABLE[MGlobals::GetColorID(i+1)];
			if(lockData.pf.num_bits() == 32)
			{
				U32 * pixels = (U32 *)(lockData.pixels);
				U32 yOff = 0;
				U32 pitch = lockData.pitch >> 2;
				for(U32 y = 0; y < lockData.height;++y)
				{
					for(U32 x = 0; x < lockData.width; ++x)
					{
						U32 base = pixels[x+yOff];
						pixels[x+yOff] = 
							((((base&lockData.pf.get_r_mask() >> lockData.pf.rl)*GetRValue(color)) >> 8) << lockData.pf.rl) |
							((((base&lockData.pf.get_g_mask() >> lockData.pf.gl)*GetGValue(color)) >> 8) << lockData.pf.gl) |
							((((base&lockData.pf.get_b_mask() >> lockData.pf.bl)*GetBValue(color)) >> 8) << lockData.pf.bl);
					}
					yOff += pitch;
				}
			}
			else if(lockData.pf.num_bits() == 16)
			{
				U16 * pixels = (U16 *)(lockData.pixels);
				U32 yOff = 0;
				U32 pitch = lockData.pitch >> 1;
				for(U32 y = 0; y < lockData.height;++y)
				{
					for(U32 x = 0; x < lockData.width; ++x)
					{
						U32 base = pixels[x+yOff];
						pixels[x+yOff] = 
							((((base&lockData.pf.get_r_mask() >> lockData.pf.rl)*GetRValue(color)) >> 8) << lockData.pf.rl) |
							((((base&lockData.pf.get_g_mask() >> lockData.pf.gl)*GetGValue(color)) >> 8) << lockData.pf.gl) |
							((((base&lockData.pf.get_b_mask() >> lockData.pf.bl)*GetBValue(color)) >> 8) << lockData.pf.bl);
					}
					yOff += pitch;
				}
			}
			else
			{
				CQASSERT(0 && "Unsupported pixel format");
			}
			PIPE->unlock_texture(playerIconId[i][numPlayerIcons],0);
		}
	}

	++numPlayerIcons;
	return numPlayerIcons -1;
}
//--------------------------------------------------------------------------//
//
U32 SysMap::GetPadding(U32 systemID)
{
	RECT rect;
	SECTOR->GetSystemRect(systemID,&rect);
	return TEX_PAD_RATIO*(rect.right-rect.left);//((GRIDSIZE*TGRID_SIZE)-(rect.right-rect.left))/2;
}
//--------------------------------------------------------------------------//
//
void SysMap::PingSystem(U32 systemID, Vector position, U32 playerID)
{
	systemPingPos[playerID-1][systemID-1] = position;
	systemPingTime[playerID-1][systemID-1] = DEFAULT_SYSTEM_PING_TIME;
}
//--------------------------------------------------------------------------//
//
U32 SysMap::CreateMissionAnim(U32 systemID, GRIDVECTOR position)
{
	MissionAnim * newAnim = new MissionAnim;
	newAnim->animID = lastAnimID;
	++lastAnimID;
	newAnim->pos = position;
	newAnim->systemID = systemID;
	newAnim->next = missionAnimList;
	missionAnimList = newAnim;
	return newAnim->animID;
}
//--------------------------------------------------------------------------//
//
void SysMap::StopMissionAnim(U32 animID)
{
	MissionAnim * anim = missionAnimList;
	MissionAnim * prev = NULL;
	while(anim)
	{
		if(anim->animID == animID)
		{	
			if(prev)
				prev->next = anim->next;
			else
				missionAnimList = anim->next;
			MissionAnim * oldAnim = anim;
			anim = anim->next;
			delete oldAnim;
		}
		else
		{
			prev = anim;
			anim = anim->next;
		}
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::FlushMissionAnims()
{
	while(missionAnimList)
	{
		MissionAnim * delMe = missionAnimList;
		missionAnimList = missionAnimList->next;
		delete delMe;
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::Save(struct IFileSystem * outFile)
{
	if(missionAnimList)
	{
		COMPTR<IFileSystem> file;
		U32 dwWritten;

		DAFILEDESC fdesc = "SysMap";

		outFile->CreateDirectory("\\SysMapSaveload");
		
		if (outFile->SetCurrentDirectory("\\SysMapSaveload") == 0)
			return;

		RecursiveDelete(outFile);

		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = 0;  // no sharing
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		
		fdesc.lpFileName = "SysMap";
		if (outFile->CreateInstance(&fdesc, file) != GR_OK)
			return;

		U32 numAnim = 0;
		MissionAnim * anim = missionAnimList;
		while(anim)
		{
			++numAnim;
			anim = anim->next;
		}
		file->WriteFile(0,&lastAnimID,sizeof(U32),&dwWritten);
		file->WriteFile(0,&numAnim,sizeof(U32),&dwWritten);
		anim = missionAnimList;
		while(anim)
		{
			file->WriteFile(0,&(anim->animID),sizeof(U32),&dwWritten);
			file->WriteFile(0,&(anim->systemID),sizeof(U32),&dwWritten);
			file->WriteFile(0,&(anim->pos),sizeof(GRIDVECTOR),&dwWritten);
			anim = anim->next;
		}
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::Load(struct IFileSystem * inFile)
{
	MT_SECTOR_SAVELOAD load;

	DAFILEDESC fdesc = "SysMap";
	COMPTR<IFileSystem> file;
	U32 dwRead;

	if (inFile->SetCurrentDirectory("\\SysMapSaveload") == 0)
		return;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		return;

	file->ReadFile(0,&lastAnimID,sizeof(U32),&dwRead);

	U32 numAnims;
	file->ReadFile(0,&numAnims,sizeof(U32),&dwRead);

	for(U32 i = 0; i < numAnims; ++i)
	{
		MissionAnim * anim = new MissionAnim;
		anim->next = missionAnimList;
		missionAnimList = anim;

		file->ReadFile(0,&(anim->animID),sizeof(U32),&dwRead);
		file->ReadFile(0,&(anim->systemID),sizeof(U32),&dwRead);
		file->ReadFile(0,&(anim->pos),sizeof(GRIDVECTOR),&dwRead);
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::InvalidateMap(U32 systemID)
{
	texInit &= (~(0x01 << (systemID-1)));
}
//--------------------------------------------------------------------------//
//
void SysMap::resetBubbles(U32 systemID)
{
	AttackBubble * bubble = attackBubbles[systemID-1];
	while(bubble)
	{
		bubble->bRendered = false;
		bubble = bubble->next;
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::mergeNextBubble(AttackBubble & targetBubble, U32 systemID)
{
	AttackBubble * bubble = attackBubbles[systemID-1];
	while(bubble && bubble->bRendered)
	{
		bubble = bubble->next;
	}
	while(bubble)
	{
		if(!bubble->bRendered)
		{
			SINGLE dist = (bubble->pos-targetBubble.pos);
			if(dist <= bubble->radius+targetBubble.radius)//do I merge these bubbles
			{
				bubble->bRendered = true;
				if(dist+bubble->radius > targetBubble.radius)//does this bubble expand us
				{
					Vector vPos1 = bubble->pos;
					Vector vPos2 = targetBubble.pos;
					Vector dir = vPos1-vPos2;
					dir.normalize();
					vPos1 = vPos1+(dir*bubble->radius*GRIDSIZE);
					vPos2 = vPos2-(dir*targetBubble.radius*GRIDSIZE);
					dir = vPos1-vPos2;
					dir = dir*0.5;
					targetBubble.radius = dir.fast_magnitude()/GRIDSIZE;
					targetBubble.pos = vPos2+dir;

					mergeNextBubble(targetBubble,systemID);

					return;
				}
			}
		}
		bubble = bubble->next;
	}
}
//--------------------------------------------------------------------------//
//
bool SysMap::getNextBubble(AttackBubble & targetBubble, U32 systemID)
{
	AttackBubble * bubble = attackBubbles[systemID-1];
	while(bubble && bubble->bRendered)
	{
		bubble = bubble->next;
	}
	if(bubble)
	{
		targetBubble.pos = bubble->pos;
		targetBubble.radius = bubble->radius;
		bubble->bRendered = true;
		bubble = bubble->next;
		while(bubble)
		{
			if(!bubble->bRendered)
			{
				SINGLE dist = (bubble->pos-targetBubble.pos);
				if(dist <= bubble->radius+targetBubble.radius)//do I merge these bubbles
				{
					bubble->bRendered = true;
					if(dist+bubble->radius > targetBubble.radius)//does this bubble expand us
					{
						Vector vPos1 = bubble->pos;
						Vector vPos2 = targetBubble.pos;
						Vector dir = vPos1-vPos2;
						dir.normalize();
						vPos1 = vPos1+(dir*bubble->radius*GRIDSIZE);
						vPos2 = vPos2-(dir*targetBubble.radius*GRIDSIZE);
						dir = vPos1-vPos2;
						dir = dir*0.5;
						targetBubble.radius = dir.fast_magnitude()/GRIDSIZE;
						targetBubble.pos = vPos2+dir;

						mergeNextBubble(targetBubble,systemID);

						return true;
					}
				}
			}
			bubble = bubble->next;
		}
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void SysMap::updateTerrainMap(U32 systemID)
{
	if(!(terrainTex[systemID-1]))
		return;
	RECT sysRect;
	SECTOR->GetSystemRect(systemID,&sysRect);
	sysWidth = sysRect.right-sysRect.left;

	if(!(texInit & (0x01 << (systemID-1))))//redo terrain texture
	{
		PIPE->blit_texture(terrainTex[systemID-1], 0, SysRECT(0,0,TMAP_SIZE,TMAP_SIZE), backTex, 0, SysRECT(0,0,mapTexW,mapTexH));

		PIPE->get_texture_dc(terrainTex[systemID-1], &hdc);
		currentTex = terrainTex[systemID-1];
		bInMapRender = true;

		//black background
		SelectObject(hdc, GetStockObject(BLACK_BRUSH));
		SelectObject(hdc, GetStockObject(NULL_PEN));
		Ellipse(hdc, TEX_PADDING, TEX_PADDING, TMAP_SIZE-TEX_PADDING, TMAP_SIZE-TEX_PADDING);

		FIELDMGR->MapRenderFields(systemID);
		
		bInMapRender = false;
		PIPE->release_texture_dc(terrainTex[systemID-1], hdc);

		texInit |= ((0x01 << (systemID-1)));
	}

	// start with a fresh copy of the texture
	PIPE->blit_texture(unitTex[systemID-1], 0, SysRECT(0,0,TMAP_SIZE,TMAP_SIZE), terrainTex[systemID-1], 0, SysRECT(0,0,TMAP_SIZE,TMAP_SIZE));


	PIPE->get_texture_dc(unitTex[systemID-1], &hdc);
	currentTex = unitTex[systemID-1];
	bInMapRender = true;
	
	//render dynamic(sight dependant) terrain
	ObjMapIterator iter(systemID,Vector(0,0,0),GRIDSIZE*200,0);
	while(iter)
	{
		if(iter->flags & OM_SYSMAP_FIRSTPASS)
			iter->obj->MapRender(false);
		++iter;
	}

//	NUGGETMANAGER->MapRender();

	U32 curPlayerID = MGlobals::GetThisPlayer();
	SINGLE pingVal = systemPingTime[curPlayerID-1][systemID-1];
	SINGLE pingRange = 0;
	if(pingVal > 0)
	{
		if(pingVal > DEFAULT_SYSTEM_PING_TIME/2)//draw The circle
		{
			pingRange = (64)*((DEFAULT_SYSTEM_PING_TIME-pingVal)/(DEFAULT_SYSTEM_PING_TIME/2));
		}
		else
		{
			pingRange = -1.0;
		}
	}

	//other players here

	//render other players units
	GRIDVECTOR pingPos = systemPingPos[curPlayerID-1][systemID-1];
	iter.SetFirst();
	while(iter)
	{
		if(!(iter->flags & OM_SYSMAP_FIRSTPASS))
		{
			if(iter->obj->GetPlayerID() != curPlayerID)
			{
				if(pingRange > 0)
				{
					if(pingPos-iter->obj->GetGridPosition() <= pingRange)
						iter->obj->MapRender(true);
					else
						iter->obj->MapRender(false);
				}
				else if(pingRange < 0)
					iter->obj->MapRender(true);
				else
					iter->obj->MapRender(false);
			}
		}
		++iter;
	}

	//render current players units
	iter.SetFirst();
	while(iter)
	{
		if(!(iter->flags & OM_SYSMAP_FIRSTPASS))
		{
			if(iter->obj->GetPlayerID() == curPlayerID)
				iter->obj->MapRender(false);
		}
		++iter;
	}

	resetBubbles(systemID);
	AttackBubble bubble;
	while(getNextBubble(bubble,systemID))
	{
		DrawRing(bubble.pos,bubble.radius*GRIDSIZE*2,1,RGB(255,0,0));
	}

	if(pingRange >0)
	{
		DrawRing(systemPingPos[curPlayerID-1][systemID-1],pingRange*GRIDSIZE*2,1,RGB(255,255,255));
	}

/*	AttackBubble * bubble = attackBubbles[systemID-1];
	while(bubble)
	{
		DrawRing(bubble->pos,bubble->radius*GRIDSIZE*2,1,RGB(255,0,0));
		bubble = bubble->next;
	}
*/
	bInMapRender = false;
	PIPE->release_texture_dc(unitTex[systemID-1], hdc);
}
//#pragma warning (disable : 4310)	// truncating constant value
//--------------------------------------------------------------------------//
//
void SysMap::drawSystem (int sys,int x,int y,int sizex,int sizey)
{
	if(!(texInit & (0x01 << (sys-1))))
		updateTerrainMap(sys);

	RECT rect;
	SECTOR->GetSystemRect(sys,&rect);

	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	// draw the background texture
	Vector v[17];

	SINGLE rot = -CAMERA->GetWorldRotation()*PI/180.0;

	TRANSFORM mapTrans;
	mapTrans.translation.x -= sizex/2;
	mapTrans.translation.y -= sizey/2;

	SINGLE cosine, sine;

	cosine = cos(rot);
	sine = sin(rot);

	mapTrans.d[0][0] = (cosine)*sizex/2;
	mapTrans.d[1][0] = (sine)*sizex/2;
	mapTrans.d[2][0] = 0;

	mapTrans.d[0][1] = - (sine)*sizey/2;
	mapTrans.d[1][1] = (cosine)*sizey/2;
	mapTrans.d[2][1] = 0;

	SINGLE transx = mapTrans.translation.x;
	mapTrans.translation.x = mapTrans.translation.x*cosine-mapTrans.translation.y*sine;
	mapTrans.translation.y = transx*sine+mapTrans.translation.y*cosine;

	mapTrans.translation.x += x;
	mapTrans.translation.y += y;

	//TEMP DEBUG
	/*DisableTextures();
	PB.Color(0xffffffff);
	PB.Begin(PB_QUADS);
	PB.Vertex3f(x-sizex/2,y-sizey/2,0);
	PB.Vertex3f(x+sizex/2,y-sizey/2,0);
	PB.Vertex3f(x+sizex/2,y+sizey/2,0);
	PB.Vertex3f(x-sizex/2,y+sizey/2,0);
	PB.End();*/
	//END TEMP DEBUG

	int i;
	for (i=0;i<17;i++)
	{
		v[i] = mapTrans.rotate_translate(mapPie[i]+Vector(1,1,0));
	}

  	SetupDiffuseBlend(unitTex[sys-1],TRUE);

	PB.Color4ub(255,255,255, 255);
	PB.Begin(PB_TRIANGLE_FAN);
	for(i = 0; i < 17; ++i)
	{
		PB.TexCoord2f(mapPieTex[i].x, mapPieTex[i].y);    PB.Vertex3f(v[i].x, v[i].y, 0);
	}
	PB.TexCoord2f(mapPieTex[1].x, mapPieTex[1].y);    PB.Vertex3f(v[1].x, v[1].y, 0);
	PB.End();
	
	//Vector pos;
	trans2 = CAMERA->GetInverseWorldTransform();
	static TRANSFORM trans;
	trans = *trans2;
	//redundant - fix later
	SINGLE xConv,yConv;
	int width = sizex;
	int height = sizey;

	// for every object in the list, draw a dot
	RECT sysRect;
	SECTOR->GetSystemRect(sys,&sysRect,0);
	S32 padding = GetPadding(sys);
	RECT smRect;
	smRect.left = sysRect.left-padding;
	smRect.right = sysRect.right+padding;
	smRect.top = sysRect.top+padding;
	smRect.bottom = sysRect.bottom-padding;
	xConv = (SINGLE)width / (smRect.right-smRect.left);//+1);
	yConv = (SINGLE)height / (smRect.bottom-smRect.top);//+1);

	//TEMP!!! total optimization case
	Vector pos,negpos;
	SINGLE rotation = CAMERA->GetWorldRotation();
	Transform trans1;
	trans.set_identity();
	pos.set((sysRect.right-sysRect.left)*0.5,(sysRect.top-sysRect.bottom)*0.5,0);
	negpos = -pos;
	trans1.set_position(negpos);
	trans.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans = trans.multiply(trans1);
	trans1.set_position(pos);
	trans1 = trans1.multiply(trans);
	trans = trans1.get_inverse();

	//whoo hoo!  now self consistent!
	//this operation gets done in lower-left origin space
	trans.translation.x -= smRect.left;
	trans.translation.y -= smRect.bottom;

	trans.d[0][0] *= xConv;
	trans.d[0][1] *= xConv;
	trans.d[0][2] *= xConv;
	trans.translation.x *= xConv;

	//yConv is negative
	//flips to upper-left origin space
	trans.d[1][0] *= yConv;  
	trans.d[1][1] *= yConv;
	trans.d[1][2] *= yConv;
	trans.translation.y *= yConv;

	//done in upper-left origin space
	trans.translation.x += x-F2LONG(width*0.5);
	//height gets inverted
	trans.translation.y += y+F2LONG(height*0.5);

	trans2 = &trans;

	Transform id_trans;

	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_modelview(id_trans);

	/*RECT paneRect;
	paneRect.left = x-width*0.5;
	paneRect.right = x+width*0.5;
	paneRect.top = y-height*0.5;
	paneRect.bottom = y+height*0.5;*/

	if (DEFAULTS->GetDefaults()->bSpectatorModeOn == false)
	{
		FOGOFWAR->MapRender(sys,x,y,sizex*0.5,sizey*0.5);
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::drawSystemScaled (int sys,int x,int y,int size)
{
#define BOARDER 0.03

	if(!(texInit & (0x01 << (sys-1))))
		updateTerrainMap(sys);

	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	// draw the background texture
	Vector v[17];
	Vector t[17];

	SINGLE rot;
	if (DEFAULTS->GetDefaults()->bSectormapRotates)
		rot = -CAMERA->GetWorldRotation()*PI/180.0;
	else
		rot = 0;

	RECT rect;
	SECTOR->GetSystemRect(sys,&rect);
	SINGLE percentSize = ((SINGLE)(rect.right-rect.left))/(GRIDSIZE*TGRID_SIZE)+BOARDER;
	if(percentSize > 1.0)
		percentSize = 1.0;

	TRANSFORM mapTrans;
	mapTrans.translation.x -= size/2;
	mapTrans.translation.y -= size/2;

	SINGLE cosine, sine;

	cosine = cos(rot);
	sine = sin(rot);

	mapTrans.d[0][0] = (cosine)*size/2;
	mapTrans.d[1][0] = (sine)*size/2;
	mapTrans.d[2][0] = 0;

	mapTrans.d[0][1] = - (sine)*size/2;
	mapTrans.d[1][1] = (cosine)*size/2;
	mapTrans.d[2][1] = 0;

	SINGLE transx = mapTrans.translation.x;
	mapTrans.translation.x = mapTrans.translation.x*cosine-mapTrans.translation.y*sine;
	mapTrans.translation.y = transx*sine+mapTrans.translation.y*cosine;

	mapTrans.translation.x += REAL2IDEALX(x);
	mapTrans.translation.y += REAL2IDEALX(y);

//	mapTrans.scale(size/2);

	int i;
	for (i=0;i<17;i++)
	{
		v[i] = mapTrans.rotate_translate((mapPie[i]*percentSize)+Vector(1,1,0));
		v[i].x = IDEAL2REALX(v[i].x);
		v[i].y = IDEAL2REALY(v[i].y);
	}

  	SetupDiffuseBlend(unitTex[sys-1],TRUE);

	PB.Color4ub(255,255,255, 255);
	PB.Begin(PB_TRIANGLE_FAN);
	for(i = 0; i < 17; ++i)
	{
		PB.TexCoord2f(mapPieTex[i].x, mapPieTex[i].y);    PB.Vertex3f(v[i].x, v[i].y, 0);
	}
	PB.TexCoord2f(mapPieTex[1].x, mapPieTex[1].y);    PB.Vertex3f(v[1].x, v[1].y, 0);
	PB.End();
	
	//Vector pos;
	trans2 = CAMERA->GetInverseWorldTransform();
	static TRANSFORM trans;
	trans = *trans2;
	//redundant - fix later
	SINGLE xConv,yConv;
	int width = IDEAL2REALX(size);
	int height = IDEAL2REALY(size);

	// for every object in the list, draw a dot
	RECT sysRect;
	SECTOR->GetSystemRect(sys,&sysRect,0);
	S32 padding = GetPadding(sys);
	RECT smRect;
	smRect.left = sysRect.left-padding;
	smRect.right = sysRect.right+padding;
	smRect.top = sysRect.top+padding;
	smRect.bottom = sysRect.bottom-padding;
	xConv = (SINGLE)width / (smRect.right-smRect.left);//+1);
	yConv = (SINGLE)height / (smRect.bottom-smRect.top);//+1);

	//TEMP!!! total optimization case
	Vector pos,negpos;
	SINGLE rotation = CAMERA->GetWorldRotation();
	Transform trans1;
	trans.set_identity();
	pos.set((sysRect.right-sysRect.left)*0.5,(sysRect.top-sysRect.bottom)*0.5,0);
	negpos = -pos;
	trans1.set_position(negpos);
	trans.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans = trans.multiply(trans1);
	trans1.set_position(pos);
	trans1 = trans1.multiply(trans);
	trans = trans1.get_inverse();

	//whoo hoo!  now self consistent!
	//this operation gets done in lower-left origin space
	trans.translation.x -= smRect.left;
	trans.translation.y -= smRect.bottom;

	trans.d[0][0] *= xConv;
	trans.d[0][1] *= xConv;
	trans.d[0][2] *= xConv;
	trans.translation.x *= xConv;

	//yConv is negative
	//flips to upper-left origin space
	trans.d[1][0] *= yConv;  
	trans.d[1][1] *= yConv;
	trans.d[1][2] *= yConv;
	trans.translation.y *= yConv;

	//done in upper-left origin space
	trans.translation.x += x-F2LONG(width*0.5);
	//height gets inverted
	trans.translation.y += y+F2LONG(height*0.5);

	trans2 = &trans;

	Transform id_trans;

	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_modelview(id_trans);

	/*RECT paneRect;
	paneRect.left = x-width*0.5;
	paneRect.right = x+width*0.5;
	paneRect.top = y-height*0.5;
	paneRect.bottom = y+height*0.5;*/
//	FOGOFWAR->MapRender(trans2,sys,x,y,width*0.5*(percentSize-BOARDER));

}
//--------------------------------------------------------------------------//
//
void SysMap::drawCompass(int sys,int x,int y,int sizex,int sizey)
{ 
	return ;//temp removed for new interface
	if (CQRENDERFLAGS.bSoftwareRenderer)
		return;

	U32 tex;
	// transform the points to show the rotation of the camera
//	CQASSERT(sys == 0);
		
	int s = SECTOR->GetCurrentSystem();
	if(s == sys)
	{
		tex = compassTextureID;
	}
	else
	{
		tex = sysRingTexID;
		sizex = sizex*1.2;
		sizey = sizey*1.2;
	}

	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	// draw the background texture
	Vector v[4];
	v[0].set(0,0,0);
	v[1].set(1.0f,0,0);
	v[2].set(1.0f,1.0f,0);
	v[3].set(0,1.0f,0);

	SINGLE rot;
	if (DEFAULTS->GetDefaults()->bSectormapRotates)
		rot = -CAMERA->GetWorldRotation()*PI/180.0;
	else
		rot = 0;

	TRANSFORM mapTrans;
	mapTrans.translation.x -= sizex/2;
	mapTrans.translation.y -= sizey/2;

	SINGLE cosine, sine;

	cosine = cos(rot);
	sine = sin(rot);

	mapTrans.d[0][0] = (cosine)*sizex;
	mapTrans.d[1][0] = (sine)*sizex;
	mapTrans.d[2][0] = 0;

	mapTrans.d[0][1] = - (sine)*sizey;
	mapTrans.d[1][1] = (cosine)*sizey;
	mapTrans.d[2][1] = 0;

	SINGLE transx = mapTrans.translation.x;
	mapTrans.translation.x = mapTrans.translation.x*cosine-mapTrans.translation.y*sine;
	mapTrans.translation.y = transx*sine+mapTrans.translation.y*cosine;

	mapTrans.translation.x += x;
	mapTrans.translation.y += y;

	for (int i=0;i<4;i++)
	{
		v[i] = mapTrans.rotate_translate(v[i]);
	}

  	SetupDiffuseBlend(tex,TRUE);

	PB.Color4ub(255,255,255, 255);
	PB.Begin(PB_QUADS);
	PB.TexCoord2f(0, 0);    PB.Vertex3f(v[0].x, v[0].y, 0);
	PB.TexCoord2f(1, 0);	PB.Vertex3f(v[1].x, v[1].y, 0);
	PB.TexCoord2f(1, 1);	PB.Vertex3f(v[2].x, v[2].y, 0);
	PB.TexCoord2f(0, 1);	PB.Vertex3f(v[3].x, v[3].y, 0);
	PB.End();
}
#pragma warning (default : 4310)	// truncating constant value
//--------------------------------------------------------------------------//
//
void SysMap::endFrame (void)
{
//	draw2D(); 
	
	if (drawSystemLines)
	{
		S32 x[4];
		S32 y[4];
		Vector point;

		point.z = 0;
		point.x = systemRect.left;
		point.y = systemRect.top;
		if (CAMERA->PointToScreen(point, x+0, y+0, 0) == BEHIND_CAMERA)
			goto Done;

		point.x = systemRect.right;
		point.y = systemRect.top;
		if (CAMERA->PointToScreen(point, x+1, y+1, 0) == BEHIND_CAMERA)
			goto Done;

		point.x = systemRect.right;
		point.y = systemRect.bottom;
		if (CAMERA->PointToScreen(point, x+2, y+2, 0) == BEHIND_CAMERA)
			goto Done;

		point.x = systemRect.left;
		point.y = systemRect.bottom;
		if (CAMERA->PointToScreen(point, x+3, y+3, 0) == BEHIND_CAMERA)
			goto Done;

		//
		// draw the box
		//
		DA::LineDraw(CAMERA->GetPane(), x[0], y[0], x[1], y[1], -1);
		DA::LineDraw(CAMERA->GetPane(), x[1], y[1], x[2], y[2], -1);
		DA::LineDraw(CAMERA->GetPane(), x[2], y[2], x[3], y[3], -1);
		DA::LineDraw(CAMERA->GetPane(), x[3], y[3], x[0], y[0], -1);
	}

Done:
	return;
}

const Transform &SysMap::GetMapTrans ()
{
	return *trans2;
}
//--------------------------------------------------------------------------//
//
BOOL32 SysMap::createViewer (void)
{
	if (CQFLAGS.bNoGDI)
		return 1;

	DACOMDESC desc = "IViewConstructor";
	DOCDESC ddesc;

	ddesc.memory = static_cast<SYSMAP_DATA *>(this);
	ddesc.memoryLength = sizeof(SYSMAP_DATA);

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "SYSMAP_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);
			MoveWindow(hwnd, 120, 120, 200, 200, 1);

			viewer->set_instance_name("SysMap");

			MakeConnection(doc);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
//
BOOL32 SysMap::updateViewer (void)
{
	DWORD dwWritten;

	bIgnoreUpdate++;
	
	if (doc)
	{
		doc->SetFilePointer(0,0);
		doc->WriteFile(0, static_cast<SYSMAP_DATA *>(this), sizeof(SYSMAP_DATA), &dwWritten, 0);
		doc->UpdateAllClients(0);
	}

	bIgnoreUpdate--;
	return 1;
}
//--------------------------------------------------------------------------//
//
void SysMap::updateResourceState (SYSMAP_MODE newMode)
{
	switch (newMode)
	{
	case SYSMODE_RALLY:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (statusTextID == IDS_SYSMAPRALLY && cursorID == IDC_CURSOR_MICRO_RALLY)
		{
			if (ownsResources() == 0)
				grabAllResources();
		}
		else
		{
			statusTextID = IDS_SYSMAPRALLY;
			cursorID = IDC_CURSOR_MICRO_RALLY;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SYSMODE_MOVE:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (DEFAULTS->GetDefaults()->bRightClickOption)
		{
			if (statusTextID == IDS_SYSMAPMOVE_R && cursorID == IDC_CURSOR_MICRO_MOVE)
			{
				if (ownsResources() == 0)
					grabAllResources();
			}
			else
			{
				statusTextID = IDS_SYSMAPMOVE_R;
				cursorID = IDC_CURSOR_MICRO_MOVE;
				if (ownsResources())
					setResources();
				else
					grabAllResources();
			}
		}
		else
		{
			if (statusTextID == IDS_SYSMAPMOVE && cursorID == IDC_CURSOR_MICRO_MOVE)
			{
				if (ownsResources() == 0)
					grabAllResources();
			}
			else
			{
				statusTextID = IDS_SYSMAPMOVE;
				cursorID = IDC_CURSOR_MICRO_MOVE;
				if (ownsResources())
					setResources();
				else
					grabAllResources();
			}
		}
		break;

	case SYSMODE_GOTO:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (DEFAULTS->GetDefaults()->bRightClickOption)
		{
			if (statusTextID == IDS_SYSMAPGOTO_R && cursorID == IDC_CURSOR_MICRO_GOTO)
			{
				if (ownsResources() == 0)
					grabAllResources();
			}
			else
			{
				statusTextID = IDS_SYSMAPGOTO_R;
				cursorID = IDC_CURSOR_MICRO_GOTO;
				if (ownsResources())
					setResources();
				else
					grabAllResources();
			}
		}
		else
		{
			if (statusTextID == IDS_SYSMAPGOTO && cursorID == IDC_CURSOR_MICRO_GOTO)
			{
				if (ownsResources() == 0)
					grabAllResources();
			}
			else
			{
				statusTextID = IDS_SYSMAPGOTO;
				cursorID = IDC_CURSOR_MICRO_GOTO;
				if (ownsResources())
					setResources();
				else
					grabAllResources();
			}
		}
		break;

	case SYSMODE_BAN:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (statusTextID == IDS_OUTOFBOUNDS && cursorID == IDC_CURSOR_BAN)
		{
			if (ownsResources() == 0)
				grabAllResources();
		}
		else
		{
			statusTextID = IDS_OUTOFBOUNDS;
			cursorID = IDC_CURSOR_BAN;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SYSMODE_NONE:
		desiredOwnedFlags = 0;
		releaseResources();
		break;

	};

	sysmapMode = newMode;
}
//--------------------------------------------------------------------------//
//
void SysMap::cursorInSys (const RECT & srect,S32 x, S32 y, SYSMAP_MODE &newMode, Vector &movePos)
{
	bool bInRect = (x >= srect.left && x <= srect.right && y >= srect.top && y <= srect.bottom && inCircle(x,y,srect));

	if (bInRect)
	{
		/*
		if (bInRect && bLeftButtonValid && DEFAULTS->GetDefaults()->bRightClickOption == true && sysmapMode != SYSMODE_RALLY)
		{
			newMode = SYSMODE_MOVE;
		}
		else if (bInRect && bRightButtonValid && DEFAULTS->GetDefaults()->bRightClickOption == false && sysmapMode != SYSMODE_RALLY)
		{
			newMode = SYSMODE_MOVE;
		}
		*/
		MPart ship;
		
		if (CQFLAGS.bGamePaused==0)
		{
			ship = OBJLIST->GetSelectedList();
			while (ship.isValid())
			{
				if (ship->caps.moveOk||(ship.obj->objClass==OC_PLATFORM&&ship->caps.buildOk))
					break;
				ship = ship.obj->nextSelected;
			}
		}
		
		// check for circle coordinates
		RECT rect;
		sysMapToWorld(x, y, movePos);
		
		//detect if we are in the interface bar
		int s=lit_sys;
		if (s==0)
			s = SECTOR->GetCurrentSystem();

		SECTOR->GetSystemRect(s, &rect);
		Vector temp((rect.left-rect.right)/2, (rect.bottom-rect.top)/2, 0);
		SINGLE dist = (temp.x * temp.x);// + (temp.y * temp.y);
		temp += movePos;
		
		if (dist >= ((temp.x * temp.x) + (temp.y * temp.y)) && ship.isValid()) 
		{
			if (ship->caps.buildOk && !ship->caps.moveOk)
			{
				if (bRallyEnabled==0)
				{
					newMode = SYSMODE_GOTO;
				}
				else
				{
					newMode = SYSMODE_RALLY;
				}
			}
			else if (ship->caps.moveOk)
			{
				newMode = SYSMODE_MOVE;
			}
		}
		else
			newMode = SYSMODE_GOTO;
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::updateCursorState (S32 x, S32 y)
{
	SYSMAP_MODE newMode = SYSMODE_NONE;
	Vector movePos;
	movePos.zero();

	if (bHasFocus)
	{
		if (CQFLAGS.bFullScreenMap)
		{
			// (x < SPACEX && y < SPACEY) || 
			//if (sysmapMode == SYSMODE_LASSO)
			{
				int numSystems = SECTOR->GetNumSystems();
				int s=1;
				while (s<numSystems+1)
				{
					if (SECTOR->IsVisibleToPlayer(s,MGlobals::GetThisPlayer()))
					{
						RECT rect;
						GetMapRectForSystem(s,&rect);
						bool bInRect = (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom && inCircle(x,y,rect));
						
						if (bInRect)
						{
							if (lit_sys != s)
							{
								currentRect = rect;
							}
							lit_sys = s;
							cursorInSys(rect,x,y,newMode,movePos);
							break;
						}
					}
					s++;
				}
				
				//we're in the map but we aren't over a system
				if (s == numSystems+1)
				{
					newMode = SYSMODE_BAN;
				}
			}
		}
		else
		{
			bool bInRect = (x >= thumbRect.left && x <= thumbRect.right && y >= thumbRect.top && y <= thumbRect.bottom &&
				inCircle(x,y,thumbRect));

			if (bInRect)
			{
				if (lit_sys != 0)
					currentRect = thumbRect;
				lit_sys = 0;
				cursorInSys(thumbRect,x,y,newMode,movePos);
			}
		}
	}

	updateResourceState(newMode);
	updateCameraDrag(x, y);

	if (ownsResources() && (sysmapMode == SYSMODE_MOVE || sysmapMode == SYSMODE_RALLY))
	{
		SECTOR->HighlightMoveSpot(movePos);//,this);
	}

}
//--------------------------------------------------------------------------//
//
bool SysMap::isInside (const Vector & pt, const Vector & p0, const Vector & p1)
{
	return (pt.x >= p0.x && pt.x <= p1.x && pt.y >= p0.y && pt.y <= p1.y);
}
//--------------------------------------------------------------------------//
// -zDelta = rolled toward user, +zDelta = rolled away from user
//
void SysMap::onMouseWheel (S32 zDelta)
{
	S32 x, y;
	WM->GetCursorPos(x, y);

	S32 sctr_sizex,sctr_sizey;
	SECTOR->GetSectorCenter(&sctr_sizex,&sctr_sizey);
	sctr_sizex*=2;
	sctr_sizey*=2;

	SINGLE w;
	Transform inv = trans.get_general_inverse(w);
	Vector map_pos;
	map_pos = inv.rotate_translate(Vector(x,y,0));
//	map_pos.x = (sctr_sizex/zoom)*(SINGLE(x)/SPACEX-0.5)+x_cent;
//	map_pos.y = sctr_sizey-((sctr_sizey/zoom)*(SINGLE(SPACEY-y)/SPACEY-0.5)+y_cent);
	
	SINGLE oldZoom = zoom;
	Vector diff = Vector(x_cent,y_cent,0)-map_pos;

	zoom += 0.001*zDelta;
	if (zoom < 0.5)
		zoom = 0.5;
	if (zoom > 5.0)
		zoom = 5.0;

	x_cent = map_pos.x+diff.x*oldZoom/zoom;
	y_cent = map_pos.y+diff.y*oldZoom/zoom;

	x_cent = max(0,x_cent);
	y_cent = max(0,y_cent);

	x_cent = min(sctr_sizex,x_cent);
	y_cent = min(sctr_sizey,y_cent);

	MakeFullScreenMapTrans();

	//x_cent = map_pos.x-(sctr_sizex/zoom)*(SINGLE(x)/SPACEX-0.5);
	//y_cent = -map_pos.y+sctr_sizey-(sctr_sizey/zoom)*(SINGLE(SPACEY-y)/SPACEY-0.5);

		
	bCameraDataChanged = TRUE;
}
//--------------------------------------------------------------------------//
//
void SysMap::onFullScreenScroll (const Vector & dir)
{
	TRANSFORM trans;
	trans.rotate_about_k(CAMERA->GetWorldRotation()* MUL_DEG_TO_RAD);
	Vector realDir = trans*dir;


	S32 sctr_sizex,sctr_sizey;
	SECTOR->GetSectorCenter(&sctr_sizex,&sctr_sizey);
	sctr_sizex*=2;
	sctr_sizey*=2;
	S32 deltaPos = max(sctr_sizey,sctr_sizex);

	x_cent += realDir.x * (deltaPos/30);
	y_cent += realDir.y * (deltaPos/30);

	x_cent = max(0,x_cent);
	y_cent = max(0,y_cent);

	x_cent = min(sctr_sizex,x_cent);
	y_cent = min(sctr_sizey,y_cent);

	MakeFullScreenMapTrans();

	//x_cent = map_pos.x-(sctr_sizex/zoom)*(SINGLE(x)/SPACEX-0.5);
	//y_cent = -map_pos.y+sctr_sizey-(sctr_sizey/zoom)*(SINGLE(SPACEY-y)/SPACEY-0.5);

		
	bCameraDataChanged = TRUE;
}
//--------------------------------------------------------------------------//
//
void SysMap::onLeftButtonUp (S32 x, S32 y, U32 wParam)
{
	if (bLeftButtonValid)
	{
		updateCameraDrag(x, y);

		bLeftButtonValid = false;

		if (DEFAULTS->GetDefaults()->bRightClickOption == true && sysmapMode != SYSMODE_RALLY)
		{
			bCameraDragActive = false;
			updateResourceState(SYSMODE_NONE);
		}
		else if (sysmapMode == SYSMODE_RALLY)
		{
			Vector pos;
			NETGRIDVECTOR netvector;

			sysMapToWorld(x, y, pos);

			netvector.init(pos, lit_sys);
			if (lit_sys == 0)
				netvector.systemID = SECTOR->GetCurrentSystem();

			EVENTSYS->Send(CQE_SET_RALLY_POINT, &netvector);
		}
		else if (ownsResources())
		{
			MPart ship = OBJLIST->GetSelectedList();
			if (sysmapMode == SYSMODE_MOVE && ship.isValid() && ship->caps.moveOk)
			{
				Vector pos;
				NETGRIDVECTOR netvector;

				sysMapToWorld(x, y, pos);

				netvector.init(pos,lit_sys);
				if (lit_sys == 0)
					netvector.systemID = SECTOR->GetCurrentSystem();

				if(wParam & MK_CONTROL)
				{
					EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_SQMOVE_SELECTED_UNITS : CQE_SMOVE_SELECTED_UNITS, &netvector);
				}
				else
				{
					EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_QMOVE_SELECTED_UNITS: CQE_MOVE_SELECTED_UNITS, &netvector);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
bool SysMap::onLeftButtonDown (S32 x, S32 y)
{
	if (sysmapMode != SYSMODE_NONE && ownsResources())
	{
		bLeftButtonValid = true;
		oldMouseX = x;
		oldMouseY = y;

		if (DEFAULTS->GetDefaults()->bRightClickOption == true && sysmapMode != SYSMODE_RALLY)
		{
			// right click model
			bCameraDragActive = true;
			updateResourceState(SYSMODE_GOTO);
			updateCameraDrag(x, y);
		}
		return true;  // eat the message
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void SysMap::onRightButtonDown (S32 x, S32 y)
{
	if (sysmapMode != SYSMODE_NONE && ownsResources())
	{
		bRightButtonValid = true;
		oldMouseX = x;
		oldMouseY = y;

		// use right-click to move camera during rally point set
		if (DEFAULTS->GetDefaults()->bRightClickOption == false || sysmapMode == SYSMODE_RALLY)
		{
			// left click model
			bCameraDragActive = true;
			updateResourceState(SYSMODE_GOTO);
			updateCameraDrag(x, y);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::onRightButtonUp (S32 x, S32 y, U32 wParam)
{
	if (bRightButtonValid)
	{
		updateCameraDrag(x, y);
		bRightButtonValid = false;
		if (DEFAULTS->GetDefaults()->bRightClickOption == false || sysmapMode == SYSMODE_RALLY)
		{
			bCameraDragActive = false;
			updateResourceState(SYSMODE_NONE);
		}
		else if (ownsResources())
		{
			MPart ship = OBJLIST->GetSelectedList();
			if (sysmapMode == SYSMODE_MOVE && ship.isValid() && ship->caps.moveOk)
			{
				Vector pos;
				NETGRIDVECTOR netvector;

				sysMapToWorld(x, y, pos);

				netvector.init(pos,lit_sys);
				if (lit_sys == 0)
				{
					netvector.systemID = SECTOR->GetCurrentSystem();
				}

				if(wParam & MK_CONTROL)
				{
					EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_SQMOVE_SELECTED_UNITS : CQE_SMOVE_SELECTED_UNITS, &netvector);
				}
				else
				{
					EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_QMOVE_SELECTED_UNITS: CQE_MOVE_SELECTED_UNITS, &netvector);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void SysMap::updateCameraDrag (S32 x, S32 y)
{
	if (bCameraDragActive && ownsResources() && (sysmapMode == SYSMODE_MOVE || sysmapMode == SYSMODE_GOTO || sysmapMode == SYSMODE_RALLY))
	{
		Vector newPos;
		if (lit_sys != 0)
			SECTOR->SetCurrentSystem(lit_sys);
		sysMapToWorld(x, y, newPos);
		CAMERA->SetLookAtPosition(newPos);
	}
}
//--------------------------------------------------------------------------//
// convert from screen coordinates to world coordinates
//
void SysMap::thumbToWorld (S32 x, S32 y, Vector & result)
{
	const Transform *trans = CAMERA->GetWorldTransform();		// rotated -> world coordinates
	Vector pos;

	//
	// convert to rotated world coordinates
	//
	pos.x = x - thumbRect.left;
	pos.x = ((pos.x * (sysMapRect.right-sysMapRect.left+1)) /
				(thumbRect.right-thumbRect.left+1)) + sysMapRect.left;
	pos.y = thumbRect.top - y;
	pos.y = ((pos.y * (sysMapRect.top-sysMapRect.bottom+1)) /
				(thumbRect.bottom-thumbRect.top+1)) + sysMapRect.top;
	pos.z = 0;
	//
	// convert to world coordinates
	//
	result = trans->rotate_translate(pos);
}
//--------------------------------------------------------------------------//
// convert from screen coordinates to world coordinates
//
void SysMap::sysMapToWorld (S32 x, S32 y, Vector & result,bool bRotated)
{
/*	if (sys == 0)
	{
		thumbToWorld (x,y,result);
		return;
	}*/
	
	const Transform *trans = CAMERA->GetWorldTransform();		// rotated -> world coordinates
	Vector pos;
	
	RECT sysRect;
	int sys = lit_sys;
	if (sys ==0)
		sys = SECTOR->GetCurrentSystem();
	SECTOR->GetSystemRect(sys,&sysRect,0);
	S32 padding;
	if(CQFLAGS.bFullScreenMap)
		padding = 0;
	else
		padding = GetPadding(sys);
	RECT smRect;
	smRect.left = sysRect.left-padding;
	smRect.right = sysRect.right+padding;
	smRect.top = sysRect.top+padding;
	smRect.bottom = sysRect.bottom-padding;
	//
	// convert to rotated world coordinates
	//
	pos.x = x - currentRect.left;
	pos.x = ((pos.x * (smRect.right-smRect.left+1)) /
				(currentRect.right-currentRect.left+1)) + smRect.left;
	pos.y = currentRect.top - y;
	pos.y = ((pos.y * (smRect.top-smRect.bottom+1)) /
				(currentRect.bottom-currentRect.top+1)) + smRect.top;
	pos.z = 0;
	//
	// convert to world coordinates
	//
	if (bRotated)
		result = pos;
	else
		result = trans->rotate_translate(pos);
}

void SysMap::GetMapRectForSystem(int sys,struct tagRECT *rect)
{
	S32 sctr_sizex,sctr_sizey;
	SECTOR->GetSectorCenter(&sctr_sizex,&sctr_sizey);
	if(sctr_sizex > sctr_sizey)
		sctr_sizey = sctr_sizex;
	else
		sctr_sizex = sctr_sizey;
	sctr_sizex*=2/zoom;
	sctr_sizey*=2/zoom;

	SINGLE xConv = SINGLE(SPACEX)/sctr_sizex;
/*	trans2 = CAMERA->GetInverseSectorTransform();
	Transform trans;
	trans = *trans2;

	trans.translation.x += x_cent;//-= xorg;
	trans.translation.y += y_cent;//-= yorg;
				
	trans.d[0][0] *= xConv;
	trans.d[0][1] *= xConv;
	trans.d[0][2] *= xConv;
	trans.translation.x *= xConv;
				
	trans.d[1][0] *= yConv;
	trans.d[1][1] *= yConv;
	trans.d[1][2] *= yConv;
	trans.translation.y *= yConv;

	trans.translation.y += SPACEY;*/

	MakeFullScreenMapTrans();

	RECT sysRect;
	SECTOR->GetSystemRect(sys,&sysRect,1);
	Vector pos;
	SINGLE width = (sysRect.right-sysRect.left)*xConv*0.5;
	pos.x = (sysRect.left+sysRect.right)*0.5;
	pos.y = (sysRect.bottom+sysRect.top)*0.5;
	pos.z = 0;
	pos = trans*pos;

	rect->left = pos.x-width;
	rect->top = pos.y-width;
	rect->right = pos.x+width;
	rect->bottom = pos.y+width;

}
//--------------------------------------------------------------------------//
//

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

void SysMap::createGDIObjects()
{
}

void SysMap::destroyGDIObjects()
{
}

void SysMap::loadTextures (bool bEnable)
{
	if (bEnable)
	{	
		U32 i;
		for(i = 0; i < MAX_SYSTEMS; ++i)
		{
			while(attackBubbles[i])
			{
				AttackBubble * bub = attackBubbles[i];
				attackBubbles[i] = attackBubbles[i]->next;
				delete bub;
			}
		}
		while(missionAnimList)
		{
			MissionAnim * delMe = missionAnimList;
			missionAnimList = missionAnimList->next;
			delete delMe;
		}
		waypointFrame = 0;
		bInMapRender = false;
		backTex = TMANAGER->CreateTextureFromFile("systemmap_bg.tga", TEXTURESDIR, DA::TGA, PF_4CC_DAOT);		// create opaque texture
		for(i = 0; i < MAX_SYSTEMS; ++i)
		{
			lastUpdateTime[i] = 0;
			terrainTex[i] = 0;
		}
		//
		// create the draw texture
		//
		U32 l;
		if (PIPE->get_texture_dim(backTex, &mapTexW, &mapTexH, &l) == GR_OK)
		{
			PixelFormat pf;
			if (PIPE->get_texture_format(backTex, &pf) == GR_OK)
			{
				for(i = 0; i < MAX_SYSTEMS; ++i)
				{
					if (PIPE->create_texture(TMAP_SIZE, TMAP_SIZE, pf, 0, 0, terrainTex[i]) == GR_OK)
					{
					}
					if (PIPE->create_texture(TMAP_SIZE, TMAP_SIZE, pf, 0, 0, unitTex[i]) == GR_OK)
					{
					}
				}
			}
		}
		createGDIObjects();

		SINGLE offs=0;
		if (CQFLAGS.bTextureBias)
			offs = 1.0f/(2*TMAP_SIZE);

		mapPie[0] = Vector(0,0,0);
		mapPieTex[0] = Vector(0.5+offs,0.5+offs,0);
		for(i = 1; i < 17; ++i)
		{
			mapPie[i] = Vector(cos((PI/8)*(i-1)),sin((PI/8)*(i-1)),0);
			mapPieTex[i] = Vector((mapPie[i].x/2)+0.5+offs,(mapPie[i].y/2)+0.5+offs,0);
		}

		waypointAnim = new AnimInstance;
		if (waypointAnim)
		{
			waypointArch = ANIM2D->create_archetype("moveecho.anm");
			waypointAnim->Init(waypointArch);
		}
		missionAnim = new AnimInstance;

		if (missionAnim)
		{
			missionArch = ANIM2D->create_archetype("moveecho.anm");
			missionAnim->Init(missionArch);
		}
	}
	else
	{
		if(waypointAnim)
		{
			delete waypointAnim;
			waypointAnim = 0;
		}
		if(waypointArch)
		{
			delete waypointArch;
			waypointArch = 0;
		}

		if(missionAnim)
		{
			delete missionAnim;
			missionAnim = 0;
		}
		if(missionArch)
		{
			delete missionArch;
			missionArch = 0;
		}

		U32 i;
		for(i = 0; i < MAX_SYSTEMS; ++i)
		{
			while(attackBubbles[i])
			{
				AttackBubble * bub = attackBubbles[i];
				attackBubbles[i] = attackBubbles[i]->next;
				delete bub;
			}
		}

		while(missionAnimList)
		{
			MissionAnim * delMe = missionAnimList;
			missionAnimList = missionAnimList->next;
			delete delMe;
		}

		texInit = 0;
		TMANAGER->ReleaseTextureRef(backTex);
		backTex = 0;
		for(i = 0; i < MAX_SYSTEMS; ++i)
		{
			PIPE->destroy_texture(terrainTex[i]);
			terrainTex[i] = 0;
			PIPE->destroy_texture(unitTex[i]);
			unitTex[i] = 0;
		}
		destroyGDIObjects();
		for(i = 0 ; i < numIcons; ++i)
		{
			TMANAGER->ReleaseTextureRef(iconId[i]);
		}
		numIcons = 0;
		for(i = 0 ; i < numPlayerIcons; ++i)
		{
			TMANAGER->ReleaseTextureRef(playerIconId[0][i]);
			for(U32 j = 1; j < MAX_PLAYERS; ++j)
			{
				if(playerIconId[j][i] != -1)
				{
					for(U32 k = i+1;k < numPlayerIcons; ++k)
					{
						if(playerIconId[j][i] == playerIconId[j][k])
							playerIconId[k][i] = -1;
					}
					PIPE->destroy_texture(playerIconId[j][i]);
				}
			}
		}
		numPlayerIcons = 0;
	}
}

void SysMap::loadInterface (IShapeLoader * loader)
{
	loadTextures(loader != 0);
	if (loader)
	{
//		loader->CreateDrawAgent(38, zoneFlash);

//		CreateDrawAgent("ZeroDeg.bmp", TEXTURESDIR, DA::BMP, 0, zeroDegShape);
//		zoneFlash = zeroDegShape;		// fix this!!! (jy)

		// load our compass texture
		M_RACE race = MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());
		char buffer[20];
		switch (race)
		{
		case M_TERRAN:
			strcpy(buffer,"Compass_T.tga");
			break;
		case M_MANTIS:
			strcpy(buffer,"Compass_M.tga");
			break;
		case M_SOLARIAN:
			strcpy(buffer,"Compass_S.tga");
			break;
		}
		compassTextureID = TMANAGER->CreateTextureFromFile(buffer, TEXTURESDIR, DA::TGA, PF_4CC_DAA1);
		sysRingTexID = TMANAGER->CreateTextureFromFile("sysRing.tga",TEXTURESDIR, DA::TGA, PF_4CC_DAA1);
	}
	else
	{
//		zoneFlash.free();
		zeroDegShape.free();
		TMANAGER->ReleaseTextureRef(compassTextureID);
		compassTextureID = 0;
		TMANAGER->ReleaseTextureRef(sysRingTexID);
		sysRingTexID = 0;
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

struct _sysmap : GlobalComponent
{
	SysMap * sysmap;

	virtual void Startup (void)
	{
		SYSMAP = sysmap = new DAComponent<SysMap>;
		AddToGlobalCleanupList((IDAComponent **) &SYSMAP);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		COMPTR<IImageReader> reader;
		HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
		MENUITEMINFO minfo;

		if (sysmap->createViewer() == 0)
			CQBOMB0("Viewer could not be created.");

		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDS_VIEWSYSMAP;
		minfo.dwTypeData = "SysMap";
		minfo.cch = 6;	// length of string "SysMap"
			
		if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
			sysmap->menuID = IDS_VIEWSYSMAP;

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(SYSMAP, &sysmap->eventHandle);

		sysmap->initializeResources();

		SYSMAP_DATA tmpData;
		if (DEFAULTS->GetDataFromRegistry(szRegKey, &tmpData, sizeof(tmpData)) == sizeof(tmpData))
		{
			DWORD dwWritten;
			if (sysmap->doc)
			{
				sysmap->doc->SetFilePointer(0,0);
				sysmap->doc->WriteFile(0, &tmpData, sizeof(tmpData), &dwWritten, 0);
				sysmap->doc->UpdateAllClients(0);
			}
		}
	}
};

static _sysmap startup;

//--------------------------------------------------------------------------//
//-----------------------------End SysMap.cpp-------------------------------//
//--------------------------------------------------------------------------//

