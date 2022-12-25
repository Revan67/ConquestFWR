//
// ModeSystem
//

#include "stdafx.h"
#include "globals.h"

#include "Mode.h"
#include "Startup.h"
#include "SysMap.h"
#include "Object.h"
#include "CQTrace.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "GridVector.h"
#include "DataList.h"
#include "Undo.h"
#include "SystemStructs.h"
#include "MainFrm.h"
#include "Editor.h"
#include "Campaign.h"
#include "Scenario.h"
#include "StringTable.h"
#include "Clipboard.h"
#include "ObjectFamily.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>

#include <FileSys.h>
#include <RendPipeline.h>
#include <renderer.h>
#include <IHardPoint.h>
#include <IRenderPrimitive.h>
#include <LightMan.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <BaseCam.h>
#include <IConnection.h>
#include <ObjClass.h>
#include <VFX.h>

#include <windowsx.h>
#include <d3dtypes.h>

// from Camera.cpp
void OrthoView (const PANE *pane);

INT_PTR CALLBACK DialogProc_ChooseGroup( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------

struct ObjectFamilyEnum : public IObjectFamilyEnum
{
	struct FamilyInfoList : std::list<IObjectFamilyEnum::FamilyInfo> {};
	struct ObjectInfoList : std::list<IObjectFamilyEnum::ObjectInfo> {};

	IObjectFamily* family;
	FamilyInfoList familyInfoList;
	ObjectInfoList objectInfoList;

	virtual void EnumFamilyInfo( FamilyInfo& _info )
	{
		familyInfoList.push_back( _info );
	}

	virtual void EnumObjectInfo( ObjectInfo& _info )
	{
		objectInfoList.push_back( _info );
	}
};

//----------------------------------------------------------------------------------------------

class ModeSystem : public IMode, public IEventCallback, public IObjectFamilyEnum
{
public:

	BEGIN_DACOM_MAP_INBOUND(ModeSystem)
		DACOM_INTERFACE_ENTRY(IMode)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	// IMode methods

	virtual bool OnCreate( LPCREATESTRUCT lpcs, CCreateContext* pContext ){ return true; }
	virtual bool Start();
	virtual bool Stop();
	virtual void Update();
	virtual void Draw();

	// IEventCallback methods

	DEFMETHOD(Notify) (U32 message, void *param);

	// IObjectFamilyEnum methods

	virtual void EnumFamilyInfo( FamilyInfo& _info );
	virtual void EnumObjectInfo( ObjectInfo& _info );

	// locals

	enum Mode
	{
		MSM_NONE,
		MSM_PLACEMENT,
		MSM_SELECTION,
		MSM_LASSO,
	};

	enum
	{
		MAX_PLANET_POINTS = 12,
	};

	struct PlatformData
	{
		UniqueID planetID;
		bool     slots[MAX_PLANET_POINTS];
		U8       numSlots;

		PlatformData() : planetID(0)
		{			
		}

		void reset()
		{
			numSlots = 0;
			memset( slots, false, sizeof(bool) * MAX_PLANET_POINTS );
		}

		bool& operator[]( int nIndex )
		{
			nIndex += MAX_PLANET_POINTS;
			nIndex %= MAX_PLANET_POINTS;
			return slots[ nIndex ];
		}
	};

	struct JumpGateInfo
	{
		UniqueID gateID;
		U32      sourceSystemID;
        U32      sourceWormholeID;
		U32      destinationSystemID;
		U32      destinationWormholeID;
	};
	
	struct Dragger
	{
		IObject*     object;        // dragging an object
		CRect        area;          // dragging an area
		bool         bCanBePlaced;
		PlatformData platformData;
		bool         bLassoStart;
		bool         bLassoDragging;

		Dragger()
		{
			object = NULL;
			bCanBePlaced = false;
			bLassoStart = bLassoDragging = false;
		}

		~Dragger()
		{
			if(	object )
			{
				object->Delete();
				delete object;
			}
			object = NULL;
		}
	};

	struct PlanetCursor : public PlatformData
	{
		IObject* planet;
		Vector   points[MAX_PLANET_POINTS];
	};

	bool                 m_bInit;
	bool                 m_bInFocus;
	UNDO::CommandManager m_commandManager;
	TRANSFORM            m_xformCursor;
	U32                  m_eventHandle;
	System*              m_currentSystem;
	Dragger              m_dragger;
	Mode                 m_mode;
	PlanetCursor         m_planetCursor;
	IObject*             m_contextMenuObject;

	ModeSystem()
	{
		m_eventHandle = 0;
		m_bInit = false;
		m_currentSystem = NULL;
		m_mode = MSM_NONE;
		m_planetCursor.planet = NULL;
		m_bInFocus = false;
		m_contextMenuObject = NULL;
	}

	~ModeSystem()
	{
		if( m_bInit )
		{
			m_eventHandle = 0;
			m_bInit = false;
			m_currentSystem = NULL;
			m_mode = MSM_NONE;
		}
		m_eventHandle = 0;
		m_bInit = false;
		m_planetCursor.planet = NULL;
		m_bInFocus = false;
		m_contextMenuObject = NULL;
	}

	void prepareForRender();
	void renderSector();
	void renderGridLines();
	void renderSystem();
	void renderCursor();
	void renderPlanetSlots();
	void renderDebug();
	void renderDebugRect( CRect& _rect, U32 _color );
	void renderSelectedGroup();

	void mouseMessage( U32 _message, CPoint& _pt, void* _params );
	void updateDragger( void );
	void gridCellInfo( FPoint& _point, SINGLE& _cellSize, IObject* _object, Transform* _xform = NULL );
	void findclosetPlanetToCursor();
	void updateBars();
	void movementUpdate(void*);
	void displayContextMenu(HWND _hwnd, CPoint& _pt);
	void contextEvent( enum CQ_CONTEXTEVENT _event );
	void updateCameraUsingScrollBars();
	void deleteObject( IObject* );
	void copyObjectsToClipboard();
	void pasteObjectsFromClipboard();
	void moveObject( IObject* _object, Transform& _xform );
	void resolveLassoSelect();
	void pasteObject( IObject* );
	bool putIntoValidSpot( IObject* _object );

	void clickPlacement( CPoint& point );
	void clickSelect( CPoint& point );
	void clickMovement( CPoint& point );
	void dblClickSelect( CPoint& point );

	IObject* screenToObject( CPoint& _point );
	IObject* findAttachedObject( IObject* _baseObject );

	bool validatePlatformPlacement( PlatformData& _platformData, PlanetCursor& _planetCursor, int _slotIndex );
	bool testWormholeWarp( Transform& _xform );

	//----------------------------------------------------------------------------------------------
	// inlines

	inline int findNearestPlanetPoint( Transform& _xform )
	{
		if( !m_planetCursor.planet )
		{
			return -1;
		}
		else
		{
			// make sure that the "cursor" is close enough to a planet at all
			SINGLE maxDistance = GRIDSIZE * 3;

			if( (m_planetCursor.planet->GetTransform().translation - m_xformCursor.translation).magnitude() > maxDistance )
			{
				return -1;
			}
		}

		SINGLE distance = FLT_MAX;
		int    closetPointIdx = -1;

		// find the "best place" for placing the platform
		for( int i = 0; i < MAX_PLANET_POINTS; i++ )
		{
			SINGLE d = (m_planetCursor.points[i] - m_xformCursor.translation).magnitude();

			if( d < distance )
			{
				distance = d;
				closetPointIdx = i;
			}
		}

		// set up the place object struct
		if( closetPointIdx != -1 )
		{
			SINGLE slice = ((PI*2) / MAX_PLANET_POINTS) * closetPointIdx;

			TRANSFORM xform;
			xform.z_rotate_right( slice );
			xform.rotate_about_i( CQ2EDToRadian(90) );
			xform.translation = m_planetCursor.points[closetPointIdx];

			_xform = xform;
		}

		return closetPointIdx;
	}

	inline bool findNearestWormhole( Transform& _xform, DWORD* _wormholeID, SINGLE _distance = 1.0f )
	{
		TRANSFORM xform = _xform;

		for( int i = 0; i < m_currentSystem->jList.GetCount(); i++ )
		{
			// make sure that the "cursor" is close enough to a planet at all
			SINGLE maxDistance = GRIDSIZE * _distance;

			JumpPoint& jumpPoint = m_currentSystem->jList.ElementAt(i);

			if( jumpPoint.wormholeObject )
			{
				if( (jumpPoint.wormholeObject->GetTransform().translation - xform.translation).magnitude() < maxDistance )
				{
					_xform = jumpPoint.wormholeObject->GetTransform();

					if( _wormholeID )
					{
						*_wormholeID = jumpPoint.id;
					}
					return true;
				}
			}
		}
		return false;
	}

	inline void resetObjectSelector()
	{
		OBJECTSELECTION->Reset();

		if( EVENTSYS )
			EVENTSYS->Send( CQE_ENTITY_SELECT );
	}

	inline void updateScrollBars()
	{
		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
		if( frame )
		{
			CView* view = frame->GetActiveView();
			if( view )
			{
				Vector look = CAMERA->GetLookAtPosition();
				::SetScrollPos( view->m_hWnd, SB_HORZ, look.x, true );
				::SetScrollPos( view->m_hWnd, SB_VERT, look.y, true );
			}
		}
	}

	//------------
	// COMMANDS

	struct PlaceObject : UNDO::Command
	{
		U32          systemID;
		CString      archname;
		UniqueID     objectID;
		TRANSFORM    xform;
		PlatformData platformData;

		PlaceObject() : objectID(0), systemID(0)
		{ 
		}

		void update()
		{
			CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
			pFrame->UpdateBars();
		}

		virtual ~PlaceObject() 
		{ 
		}

		virtual const char* GetName()
		{
			return "Place Object";
		}

		virtual bool Execute()
		{
			IObject* obj = Object::Create(archname);
			if( obj )
			{
				objectID = obj->GetID();

				obj->SetTransform( xform );
				obj->SetSystemID( systemID );
				obj->SetPlayerID( Editor::playerID );

				ObjectData data;
				obj->GetObjectData( data );
				if( data.objectClass == OC_PLATFORM )
				{
					obj->SetCustomData( &platformData, sizeof(platformData) );
				}

				System* system = Editor::GetActiveSystem();
				if( system )
				{
					system->objectList.push_back( obj );
					update();
					return true;
				}
			}
			return false;
		}

		virtual bool Unexecute()
		{
			System* system = Editor::GetActiveSystem();
			if( system && system->id == systemID )
			{
				IObject* obj = system->find(objectID);
				if( obj )
				{
					objectID = 0;
					system->objectList.Delete(obj);
					update();
					return true;
				}
			}
			return false;
		}
	};

	//----------------------------------------------------------------------------------------------

	struct PlaceJumpGate : PlaceObject
	{
		U32 mainWormholeID;
		U32 pairedSystemID;
		U32 pairedWormholeID;

		PlaceJumpGate() : pairedSystemID(0), pairedWormholeID(0), mainWormholeID(0)
		{ 
		}

		virtual ~PlaceJumpGate() 
		{ 
		}

		virtual const char* GetName()
		{
			return "Place Jump Gate";
		}

		virtual bool Execute()
		{
			if( PlaceObject::Execute() )
			{
				System* mainSystem = Editor::GetSystem( 0, systemID );
				JumpPoint* mainJumpPoint = NULL;

				if( mainSystem )
				{
					mainJumpPoint = mainSystem->jList.FindByJumpIdx( mainWormholeID );
					if( !mainJumpPoint )
					{
						return false;
					}

					pairedSystemID   = mainJumpPoint->destSystemID;
					pairedWormholeID = mainJumpPoint->destWormholeID;
				}

				System* pairSystem = Editor::GetSystem( 0, pairedSystemID );
				if( mainSystem && pairSystem )
				{
					JumpPoint* pairJumpPoint = pairSystem->jList.FindByJumpIdx( pairedWormholeID );
					if( mainJumpPoint && pairJumpPoint )
					{
						IObject* gateObject = mainSystem->find(objectID);

						pairJumpPoint->parentJumpGate = gateObject;

						JumpGateInfo jumpGateInfo;
						jumpGateInfo.gateID				   = gateObject->GetID();
						jumpGateInfo.sourceSystemID		   = systemID;;
						jumpGateInfo.sourceWormholeID	   = mainWormholeID;
						jumpGateInfo.destinationSystemID   = pairedSystemID;
						jumpGateInfo.destinationWormholeID = pairedWormholeID;

						gateObject->SetCustomData( &jumpGateInfo, sizeof(jumpGateInfo) );

						return true;
					}
				}
			}
			return false;
		}

		virtual bool Unexecute()
		{
			System* pairSystem = Editor::GetSystem( 0, pairedSystemID );
			if( pairSystem )
			{
				JumpPoint* pairJumpPoint = pairSystem->jList.FindByJumpIdx( pairedWormholeID );
				if( pairJumpPoint )
				{
					pairJumpPoint->parentJumpGate = NULL;
				}
			}

			return PlaceObject::Unexecute();
		}
	};

	//----------------------------------------------------------------------------------------------

	struct MoveObject : UNDO::Command
	{
		System*   system;
		UniqueID  objectID;
		TRANSFORM xformNew;
		TRANSFORM xformOld;
		UniqueID  attachedObject;

		MoveObject() : objectID(0), system(NULL), attachedObject(0)
		{ 
		}

		void update( IObject* _object )
		{
			if( attachedObject )
			{
				IObject* obj = system->find( attachedObject );
				if( obj )
				{
					obj->SetTransform( _object->GetTransform() );
				}
			}

			CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
			pFrame->UpdateBars();
		}

		virtual ~MoveObject() 
		{ 
		}

		virtual const char* GetName()
		{
			return "Move Object";
		}

		virtual bool Execute()
		{
			if( system )
			{
				IObject* obj = system->find(objectID);
				if( obj )
				{
					obj->SetTransform( xformNew );
					if( !system->objectList.ValidatePlacement(obj) )
					{
						// did not pass the test
						obj->SetTransform( xformOld );
						return false;
					}
					update(obj);
					return true;
				}
			}
			return false;
		}

		virtual bool Unexecute()
		{
			if( system )
			{
				IObject* obj = system->find(objectID);
				if( obj )
				{
					obj->SetTransform( xformOld );
					update(obj);
					return true;
				}
			}
			return false;
		}
	};

	//----------------------------------------------------------------------------------------------

	struct DeleteObject : UNDO::Command
	{
		U32      systemID;
		IObject* object;
		bool     bInSystem;
		UniqueID attachedObjectID;
		IObject* attachedObject;
		U32      jumpPointIndex;

		DeleteObject() : object(NULL), bInSystem(true), attachedObjectID(0), attachedObject(NULL), jumpPointIndex(0), systemID(0)
		{ 
		}

		void update()
		{
			CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
			pFrame->UpdateBars();
		}

		bool unlink( IObject* _object )
		{
			System* system = Editor::GetSystem(0,systemID);
			if( system && _object )
			{
				// SpaceObjects
				for( ObjectList::iterator it = system->objectList.begin(); it != system->objectList.end(); it++ )
				{
					IObject* obj = *it;
					if( obj == _object )
					{
						system->objectList.erase( it );
						return true;
					}
				}

				// Wormholes
				for( int i = 0; i < system->jList.GetCount(); i++ )
				{
					JumpPoint& jumpPoint = system->jList.ElementAt(i);

					if( jumpPoint.wormholeObject == _object )
					{
						jumpPointIndex = i;
						jumpPoint.wormholeObject = NULL;
						return true;
					}
				}
			}

			return false;
		}

		bool link( IObject* _object )
		{
			System* system = Editor::GetSystem(0,systemID);
			if( _object )
			{
				ObjectData data;
				_object->GetObjectData(data);

				if( data.objectClass == OC_JUMPGATE )
				{
					// Wormholes
					JumpPoint& jumpPoint = system->jList.ElementAt(jumpPointIndex);
					jumpPoint.wormholeObject = _object;
					return true;
				}
				else
				{
					// SpaceObjects
					system->objectList.push_back( object );
					return true;
				}
			}

			return false;
		}

		virtual ~DeleteObject() 
		{
			System* system = Editor::GetSystem(0,systemID);

			// now this command is in charge of the object's memory
			if( !bInSystem && object )
			{
				object->Delete();
				delete object;
				object = NULL;
			}

			if( system )
				system->updateWormholes();
		}

		virtual const char* GetName()
		{
			return "Delete Object";
		}

		virtual bool Execute()
		{
			System* system = Editor::GetSystem(0,systemID);
			if( unlink(object) )
			{
				attachedObject = system->find(attachedObjectID);
				if( attachedObject )
				{	
					if( unlink(attachedObject) )
					{
						bInSystem = false;
						update();
						return true;
					}
				}
				bInSystem = false;
				update();
				return true;
			}
			return false;
		}

		virtual bool Unexecute()
		{
			System* system = Editor::GetSystem(0,systemID);
			if( system && object && !bInSystem )
			{
				if( link(object) )
				{
					if( attachedObject )
					{
						if( link(attachedObject) )
						{
							bInSystem = true;
							update();
							return true;
						}
					}
					bInSystem = true;
					update();
					return true;
				}
			}
			return false;
		}
	};

	//----------------------------------------------------------------------------------------------

	struct DeleteJumpGate : public DeleteObject
	{
		DeleteJumpGate()
		{ 
		}

		virtual ~DeleteJumpGate() 
		{
		}

		virtual const char* GetName()
		{
			return "Delete Jump Gate";
		}

		virtual bool Execute()
		{
			JumpGateInfo* jumpGateInfo = (JumpGateInfo*)object->GetCustomData( NULL );

			if( object->GetID() == jumpGateInfo->gateID )
			{
				System* dstSys = Editor::GetSystem( 0, jumpGateInfo->destinationSystemID );
				if( dstSys )
				{
					JumpPoint* jumpPoint = dstSys->jList.FindByJumpIdx( jumpGateInfo->destinationWormholeID );
					if( jumpPoint )
						jumpPoint->parentJumpGate = NULL;

				}
			}

			return DeleteObject::Execute();
		}

		virtual bool Unexecute()
		{
			if( object )
			{
				JumpGateInfo* jumpGateInfo = (JumpGateInfo*)object->GetCustomData( NULL );

				if( object->GetID() == jumpGateInfo->gateID )
				{
					System* dstSys = Editor::GetSystem( 0, jumpGateInfo->destinationSystemID );
					if( dstSys )
					{
						JumpPoint* jumpPoint = dstSys->jList.FindByJumpIdx( jumpGateInfo->destinationWormholeID );
						if( jumpPoint )
							jumpPoint->parentJumpGate = object;

					}
				}
			}

			return DeleteObject::Unexecute();
		}
	};
};

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

bool ModeSystem::Start()
{ 
	m_bInit = true;

	CQEDITORMODE = EM_SYSTEM;

	// tell old system we are done with it
	if( m_currentSystem )
	{
		m_currentSystem->refresh();
		m_currentSystem = NULL;
	}

	// start next system
	m_currentSystem = Editor::GetActiveSystem();

	if( m_currentSystem )
	{
		// prepare the scenario
		if( CAMPAIGN->GetCurrentScenario() )
		{
			CAMPAIGN->GetCurrentScenario()->Prepare(0);
		}

		// then, prepare the system
		m_currentSystem->prepareForEditing();
	}

	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		CView* view = frame->GetActiveView();
		if( view )
		{
			::ShowScrollBar( view->m_hWnd, SB_BOTH, true );

			if( m_currentSystem )
			{
				::SetScrollRange( view->m_hWnd, SB_HORZ, 0, m_currentSystem->sizeX, true );
				::SetScrollRange( view->m_hWnd, SB_VERT, 0, m_currentSystem->sizeY, true );

				updateScrollBars();
			}
		}
	}

	m_mode = MSM_SELECTION;
	m_bInFocus = true;

	TRANSFORM t;
	t.rotate_about_i( CQ2EDToRadian(90) );
	m_xformCursor = t;

	updateDragger();

	resetObjectSelector();

	updateBars();

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool ModeSystem::Stop()
{ 
	m_commandManager.Clear();

	m_mode = MSM_SELECTION;

	resetObjectSelector();

	updateBars();

	return true;
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::Update()
{
	if( !m_bInit )
	{
		Start();
	}

	if( m_currentSystem != Editor::GetActiveSystem() )
	{
		m_commandManager.Clear();
		m_currentSystem = Editor::GetActiveSystem();

		if( m_currentSystem )
		{
			m_currentSystem->prepareForEditing();
			updateBars();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::Draw()
{
	if( CQFLAGS.bGameActive && m_currentSystem )
	{
		// start the drawing
		PIPE->set_pipeline_state( RP_CLEAR_COLOR, 0x00000000 );
		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT|RP_CLEAR_STENCIL_BIT,0);
		PIPE->begin_scene();

		// do the drawing
		CAMERA->SetPerspective();
		prepareForRender();
		renderSector();
		renderGridLines();
		renderSystem();
		renderCursor();
		renderDebug();

		// end the drawing
		PIPE->end_scene();
		PIPE->swap_buffers();
	}
}

//-----------------------------------------------------------------------------------------------------

GENRESULT ModeSystem::Notify(U32 message, void *param)
{
	if( CQEDITORMODE == EM_SYSTEM && m_currentSystem )
	{
		if( message == CQE_SET_FOCUS )
		{
			m_bInFocus = true;
		}
		else if( message == CQE_KILL_FOCUS )
		{
			m_bInFocus = false;
		}
		if( message == CQE_MOUSE_MOVE )
		{
			movementUpdate(param);
		}
		else if( message == CEQ_CONTEXT_EVENT )
		{
			contextEvent( (CQ_CONTEXTEVENT) (int)param );
		}
		else if( message == CEQ_START_CONTEXT )
		{
			if( m_contextMenuObject == NULL && OBJECTSELECTION->HasObjects() )
			{
				ObjectQuickList list;
				OBJECTSELECTION->GetList(list);
				m_contextMenuObject = list.front();
			}

			CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
			displayContextMenu( frame->GetActiveView()->m_hWnd, *(CPoint*)param );
		}
		else if( message == CQE_ASSET_CHANGE )
		{
			updateDragger();
			m_mode = MSM_PLACEMENT;
		}
		else if( message >= WM_MOUSEFIRST && message <= WM_MOUSEWHEEL )
		{
			CFrameWnd* frame = static_cast<CFrameWnd*>( ::AfxGetApp()->GetMainWnd() );
			if( !frame )
			{
				return GR_GENERIC;
			}

			CView* view = frame->GetActiveView();
			if( !view )
			{
				return GR_GENERIC;
			}

			// test against view's rect

			POINT p;
			GetCursorPos(&p);

			CRect r;
			view->GetWindowRect(r);

			if( r.PtInRect(p) )
			{
				view->ScreenToClient(&p);
				mouseMessage( message, CPoint(p), param );
			}
		}

		else if( message == WM_VSCROLL || message == WM_HSCROLL )
		{
			updateCameraUsingScrollBars();
		}

		// keyboard input
		else if( m_bInFocus )
		{
			// delete object command
			if( message == WM_KEYUP )
			{
				MSG* msg = (MSG*)param;
				if( msg->wParam == VK_DELETE )
				{
					if( OBJECTSELECTION->HasObjects() )
					{
						ObjectQuickList list;
						OBJECTSELECTION->GetList(list);

						for( ObjectList::iterator it = list.begin(); it != list.end(); it++ )
						{
							deleteObject( *it );
						}
					}
				}
			}
			else if( message == WM_KEYDOWN )
			{
			}

			// command list tasks
			else if( message == CQE_UNDO )
			{
				m_commandManager.Undo();
				resetObjectSelector();
			}
			else if( message == CQE_REDO )
			{
				m_commandManager.Redo();
				resetObjectSelector();
			}
			else if( message == CQE_COPY )
			{
				copyObjectsToClipboard();
			}
			else if( message == CQE_PASTE )
			{
				pasteObjectsFromClipboard();
			}
		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::updateBars()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	pFrame->UpdateBars();
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::prepareForRender()
{
	// set up default system rendering states

	CAMERA->SetModelView();

	PIPE->set_texture_stage_texture(0,0);
	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

	PIPE->set_texture_stage_texture(1,0);
	PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

// D3DRS_DESTBLEND 

	PIPE->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	PIPE->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	PIPE->set_render_state( D3DRS_ALPHATESTENABLE, TRUE );  
	PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state( D3DRS_ALPHAREF, 0x02 );  
	PIPE->set_render_state( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
	PIPE->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );

	LIGHT->set_ambient_light( 255, 255, 255 );
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderSector()
{
	if( CAMPAIGN->GetCurrentScenario() && CAMPAIGN->GetCurrentScenario()->GetActiveSector() )
	{
		CAMPAIGN->GetCurrentScenario()->GetActiveSector()->Render();
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderGridLines (void)
{
	// render terrain's footprint

	if( m_currentSystem && m_currentSystem->terrainMap != NULL )
	{
		m_currentSystem->terrainMap->RenderEdit();
	}

	RECT rc;
	rc.left   = 0;
	rc.top    = 165887;
	rc.right  = 165887;
	rc.bottom = 0;

	if( m_currentSystem )
	{
		rc.right = m_currentSystem->sizeX;
		rc.top   = m_currentSystem->sizeY;
	}

	// white grid lines

	SINGLE xpos = 0.0f, ypos = 0.0f;
	SINGLE size = SINGLE(rc.right - rc.left);

	PB.Begin(PB_LINES);
	PB.Color4ub( 200, 200, 200, 200 );

	while (xpos < size)
	{
		PB.Vertex3f(xpos, 0, 0);
		PB.Vertex3f(xpos, size, 0);
		PB.Vertex3f(0, ypos, 0);
		PB.Vertex3f(size, ypos, 0);

		xpos += GRIDSIZE;
		ypos += GRIDSIZE;
	}


	PB.End();

	// render selected group

	renderSelectedGroup();

	// selected object(s)

	if( OBJECTSELECTION->HasObjects() )
	{
		ObjectQuickList list;
		OBJECTSELECTION->GetList(list);

		for( ObjectList::iterator it = list.begin(); it != list.end(); it++ )
		{
			ObjectData data;
			(*it)->GetObjectData(data);

			if( data.gridSize.x == 0 && data.bJumpGate )
			{
				data.gridSize.x = 2;
			}

			SINGLE cellSize = (GRIDSIZE / 4) * data.gridSize.x;

			NETGRIDVECTOR grid;
			grid.init( data.xform.translation, 0 );

			FPoint point;
			point.X = grid.getX() * GRIDSIZE;
			point.Y = grid.getY() * GRIDSIZE;

			PB.Begin(PB_QUADS);

				PB.Color4ub( 128, 128, 0, 64 );
				PB.Vertex3f(point.X - cellSize, point.Y - cellSize, 0);
				PB.Vertex3f(point.X - cellSize, point.Y + cellSize, 0);
				PB.Vertex3f(point.X + cellSize, point.Y + cellSize, 0);
				PB.Vertex3f(point.X + cellSize, point.Y - cellSize, 0);

			PB.End();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderSystem()
{
	if( m_currentSystem )
	{
		for( int i = 0; i < m_currentSystem->jList.GetSize(); i++ )
		{
			JumpPoint& jumpPoint = m_currentSystem->jList[i];

			if( jumpPoint.wormholeObject )
			{
				jumpPoint.wormholeObject->Render();

				if( jumpPoint.parentJumpGate )
				{
					// remember last xform
					TRANSFORM oldXform = jumpPoint.parentJumpGate->GetTransform();

					// move object to this wormhole space and render
					jumpPoint.parentJumpGate->SetTransform( jumpPoint.wormholeObject->GetTransform() );
					jumpPoint.parentJumpGate->Render();

					// go back to old transform
					jumpPoint.parentJumpGate->SetTransform( oldXform );
				}

			}
			else
			{
				Vector pos( jumpPoint.x, jumpPoint.y, 0 );

				GRIDVECTOR grid;
				grid.bigGridSquare( pos );

				SINGLE cellSize = GRIDSIZE / 2;

				FPoint point;
				point.X = grid.getX() * GRIDSIZE;
				point.Y = grid.getY() * GRIDSIZE;

				PB.Begin(PB_QUADS);

					PB.Color4ub( 0, 0, 255, 255 );
					PB.Vertex3f(point.X - cellSize, point.Y - cellSize, 0);
					PB.Vertex3f(point.X - cellSize, point.Y + cellSize, 0);
					PB.Vertex3f(point.X + cellSize, point.Y + cellSize, 0);
					PB.Vertex3f(point.X + cellSize, point.Y - cellSize, 0);

				PB.End();

				if( jumpPoint.parentJumpGate )
				{
					TRANSFORM oldXform = jumpPoint.parentJumpGate->GetTransform();

					// move object to this wormhole space and render
					TRANSFORM newXform = oldXform;
					newXform.translation.x = point.X;
					newXform.translation.y = point.Y;
					jumpPoint.parentJumpGate->SetTransform( newXform );
					jumpPoint.parentJumpGate->Render();

					// go back to old transform
					jumpPoint.parentJumpGate->SetTransform( oldXform );
				}
			}
		}
	}

	// render system objects
	if( m_currentSystem && m_currentSystem->objectList.size() )
	{
		for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
		{
			(*it)->Render();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderCursor()
{
	if( m_mode == MSM_PLACEMENT && m_dragger.object )
	{
		ObjectData data;
		m_dragger.object->GetObjectData( data );

		FPoint point;
		SINGLE cellSize;
		gridCellInfo( point, cellSize, m_dragger.object );

		CAMERA->SetModelView();

		PIPE->set_render_state( D3DRS_ALPHATESTENABLE, TRUE );  
		PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,TRUE);
		PIPE->set_render_state( D3DRS_ALPHAREF, 0x02 );  
		PIPE->set_render_state( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

		// draw box around dragging object

		PB.Begin(PB_QUADS);

			if( m_dragger.bCanBePlaced )
				PB.Color4ub( 128, 128, 128, 128 );
			else
				PB.Color4ub( 255, 64, 64, 128 );

			PB.Vertex3f(point.X - cellSize, point.Y - cellSize, 0);
			PB.Vertex3f(point.X - cellSize, point.Y + cellSize, 0);
			PB.Vertex3f(point.X + cellSize, point.Y + cellSize, 0);
			PB.Vertex3f(point.X + cellSize, point.Y - cellSize, 0);

		PB.End();

		if( data.slotsNeeded )
		{
			renderPlanetSlots();
		}

		// render this last because the renderstates go wonky on me
		m_dragger.object->Render();
	}
	else if( m_mode == MSM_LASSO && m_dragger.bLassoDragging )
	{
		CFrameWnd* frame = static_cast<CFrameWnd*>( ::AfxGetApp()->GetMainWnd() );
		if( frame )
		{
			CView* view = frame->GetActiveView();
			if( view )
			{
				CRect r;
				view->GetClientRect(r);

				PANE pane;
				pane.x0 = r.left;
				pane.y0 = r.top;
				pane.x1 = r.right - 1;
				pane.y1 = r.bottom - 1;
				OrthoView(&pane);
			}
		}

		PIPE->set_render_state( D3DRS_ALPHATESTENABLE, TRUE );  
		PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,TRUE);
		PIPE->set_render_state( D3DRS_ALPHAREF, 0x02 );  
		PIPE->set_render_state( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

		// draw box around dragging object

		float x0 = __min( m_dragger.area.left, m_dragger.area.right );
		float y0 = __min( m_dragger.area.top, m_dragger.area.bottom );
		float x1 = __max( m_dragger.area.left, m_dragger.area.right );
		float y1 = __max( m_dragger.area.top, m_dragger.area.bottom );

		PB.Begin(PB_LINES);

			PB.Color4ub( 128, 255, 128, 128 );

			PB.Vertex3f(x0, y0, 0); PB.Vertex3f(x1, y0, 0);
			PB.Vertex3f(x1, y0, 0);	PB.Vertex3f(x1, y1, 0);
			PB.Vertex3f(x1, y1, 0); PB.Vertex3f(x0, y1, 0);
			PB.Vertex3f(x0, y1, 0);	PB.Vertex3f(x0, y0, 0);

		PB.End();
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderPlanetSlots()
{
	if( m_planetCursor.planet )
	{
		SINGLE size  = 100;

		for( int i = 0; i < MAX_PLANET_POINTS; i++ )
		{
			PB.Begin(PB_QUADS);

				PB.Color4ub( 0, 255, 0, 255 );
				PB.Vertex3f(m_planetCursor.points[i].x - size, m_planetCursor.points[i].y - size, 0);
				PB.Vertex3f(m_planetCursor.points[i].x - size, m_planetCursor.points[i].y + size, 0);
				PB.Vertex3f(m_planetCursor.points[i].x + size, m_planetCursor.points[i].y + size, 0);
				PB.Vertex3f(m_planetCursor.points[i].x + size, m_planetCursor.points[i].y - size, 0);

			PB.End();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderDebug()
{
	int renderDebug_numCol = 0;
	static bool renderDebug_init = false;
	static DWORD renderDebug_colors[256];
	if( !renderDebug_init )
	{
		renderDebug_init = true;

		for( int i = 0; i < 256; i++ )
		{
			renderDebug_colors[i] = RGBA_MAKE( rand()%256, rand()%256, rand()%256, 128 );
		}
	}

	if( Editor::testRect1.Width() && Editor::testRect1.Height() )
	{
		renderDebugRect( Editor::testRect1, RGBA_MAKE(255,255,0,128) );
	}

	if( Editor::testRect2.Width() && Editor::testRect2.Height() )
	{
		renderDebugRect( Editor::testRect2, RGBA_MAKE(0,255,255,128) );
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderDebugRect( CRect& _rect, U32 _color )
{
	PIPE->set_render_state( D3DRS_ALPHATESTENABLE, TRUE );  
	PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state( D3DRS_ALPHAREF, 0x02 );  
	PIPE->set_render_state( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

	SINGLE gridSlice = GRIDSIZE / 4;

	SINGLE gridX0 = _rect.left * gridSlice;
	SINGLE gridY0 = _rect.top * gridSlice;
	SINGLE gridX1 = _rect.right * gridSlice;
	SINGLE gridY1 = _rect.bottom * gridSlice;

	PB.Begin(PB_QUADS);

		PB.Color4ub( RGBA_GETRED(_color), RGBA_GETGREEN(_color), RGBA_GETBLUE(_color), RGBA_GETALPHA(_color) );
		PB.Vertex3f( gridX0, gridY0, 0);
		PB.Vertex3f( gridX0, gridY1, 0);
		PB.Vertex3f( gridX1, gridY1, 0);
		PB.Vertex3f( gridX1, gridY0, 0);

	PB.End();
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::renderSelectedGroup()
{
	ObjectFamilyEnum ofe;
	ofe.family = CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily;

	ofe.family->EnumFamilyList( ofe, (DWORD)0 );
	if( ofe.familyInfoList.size() )
	{
		for( ObjectFamilyEnum::FamilyInfoList::iterator it = ofe.familyInfoList.begin(); it != ofe.familyInfoList.end(); it++ )
		{
			if( (*it).selected )
			{
				ObjectFamilyEnum ofe_objects;
				ofe_objects.family = ofe.family;
				ofe_objects.family->EnumObjectsInFamily( ofe_objects, (*it).family, 0 );

				PB.Begin(PB_QUADS);

				ObjectFamilyEnum::ObjectInfoList::iterator nfoIt = ofe_objects.objectInfoList.begin();
				while( nfoIt != ofe_objects.objectInfoList.end()  )
				{
					IObjectFamilyEnum::ObjectInfo& nfo = *nfoIt;

					NETGRIDVECTOR grid;
					grid.init( nfo.object->GetTransform().translation, 0 );

					FPoint point;
					point.X = grid.getX() * GRIDSIZE;
					point.Y = grid.getY() * GRIDSIZE;

					SINGLE cellSize = GRIDSIZE / 2;

					PB.Begin(PB_QUADS);

						PB.Color4ub( 128, 128, 255, 64 );
						PB.Vertex3f(point.X - cellSize, point.Y - cellSize, 0);
						PB.Vertex3f(point.X - cellSize, point.Y + cellSize, 0);
						PB.Vertex3f(point.X + cellSize, point.Y + cellSize, 0);
						PB.Vertex3f(point.X + cellSize, point.Y - cellSize, 0);

					PB.End();

					nfoIt++;
				}

				PB.End();
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::gridCellInfo( FPoint& _point, SINGLE& _cellSize, IObject* _object, Transform* _xform )
{
	TRANSFORM xform = m_xformCursor;
	if( _xform )
	{
		xform  = *_xform;
	}

	if( _object )
	{
		ObjectData data;
		_object->GetObjectData( data );

		if( data.gridSize == CPoint(0,0) ) // for example platforms
		{
			_cellSize = 1;
			_point.X  = xform.translation.x;
			_point.Y  = xform.translation.y;
		}
		else if( data.gridSize == CPoint(1,1) ) // for example smaller ships
		{
			int gridX = xform.translation.x / GRIDSIZE;
			int gridY = xform.translation.y / GRIDSIZE;

			// find minor grid
			int subSize  = GRIDSIZE / 2;
			int subGridX = ((int)xform.translation.x) % GRIDSIZE;
			int subGridY = ((int)xform.translation.y) % GRIDSIZE;
			subGridX /= subSize;
			subGridY /= subSize;

			_cellSize = GRIDSIZE / 4;
			_point.X  = (gridX * GRIDSIZE) + _cellSize + (subGridX * subSize);
			_point.Y  = (gridY * GRIDSIZE) + _cellSize + (subGridY * subSize);
		}
		else if( data.gridSize == CPoint(2,2) ) // for example larger ships
		{
			int gridX = xform.translation.x / GRIDSIZE;
			int gridY = xform.translation.y / GRIDSIZE;

			_cellSize = GRIDSIZE / 2;
			_point.X  = (gridX * GRIDSIZE) + _cellSize;
			_point.Y  = (gridY * GRIDSIZE) + _cellSize;
		}
		else if( data.gridSize == CPoint(4,4) ) // for example planets
		{
			int gridX = xform.translation.x / GRIDSIZE;;
			int gridY = xform.translation.y / GRIDSIZE;

			_cellSize = GRIDSIZE;
			_point.X  = (gridX * GRIDSIZE) + _cellSize;
			_point.Y  = (gridY * GRIDSIZE) + _cellSize;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::mouseMessage( U32 _message, CPoint& _pt, void* _msg )
{
	if( !m_bInFocus ) return;

	// lasso selection
	if( _message == WM_LBUTTONDOWN || _message == WM_LBUTTONUP )
	{
		m_dragger.bLassoStart = (_message == WM_LBUTTONDOWN);

		if( !m_dragger.bLassoStart )
		{
			if( m_dragger.bLassoDragging && m_mode == MSM_LASSO )
			{
				m_dragger.bLassoDragging = false;
				m_mode = MSM_SELECTION;
				resolveLassoSelect();
				return;
			}
		}
		else
		{
			m_dragger.area.left = _pt.x;
			m_dragger.area.top  = _pt.y;
		}
	}

	if( m_mode == MSM_PLACEMENT )
	{
		if( _message == WM_LBUTTONUP )
		{
			clickPlacement( _pt  );
		}
		else if( _message == WM_RBUTTONUP )
		{
			m_mode = MSM_SELECTION;
		}
	}
	else if( m_mode == MSM_SELECTION )
	{
		if( _message == WM_LBUTTONUP )
		{
			MSG* msg = (MSG*)_msg;

			if( msg->wParam & MK_CONTROL )
			{
				clickSelect( _pt  );
			}
			else if( OBJECTSELECTION->HasObjects() )
			{
				clickMovement( _pt );
			}
			else
			{
				resetObjectSelector();
				clickSelect( _pt  );
			}
		}
		else if( _message == WM_LBUTTONDBLCLK )
		{
			dblClickSelect( _pt );
		}
		else if( _message == WM_RBUTTONUP )
		{
			IObject* selectedObject = screenToObject( _pt );
			if( selectedObject )
			{
				OBJECTSELECTION->Add( selectedObject );

				ObjectData data;
				selectedObject->GetObjectData(data);

				if( data.objectClass == OC_JUMPGATE )
				{
					testWormholeWarp(data.xform);
					resetObjectSelector();
					return;
				}
				else
				{
					CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();

					CRect rect;
					::GetWindowRect( frame->GetActiveView()->m_hWnd, rect );
					_pt.x += rect.left;
					_pt.y += rect.top;

					displayContextMenu( frame->GetActiveView()->m_hWnd, _pt );
				}
			}
		}
	}
	else if( m_mode == MSM_LASSO )
	{
		if( m_dragger.bLassoDragging )
		{
			m_dragger.area.right  = _pt.x;
			m_dragger.area.bottom = _pt.y;
		}
		else
		{
			m_dragger.bLassoDragging = true;
			m_dragger.area.left = _pt.x;
			m_dragger.area.top  = _pt.y;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::clickPlacement( CPoint& point )
{
	if( m_dragger.object && m_currentSystem && m_dragger.bCanBePlaced )
	{
		PlaceObject* placeObject = new PlaceObject;
		placeObject->systemID = m_currentSystem->id;

		// get object data
		ObjectData data;
		m_dragger.object->GetObjectData( data );

		// placing a platform?
		if( data.slotsNeeded && m_planetCursor.planet )
		{
			placeObject->archname = data.archetype;

			memcpy( &placeObject->platformData, &m_dragger.platformData, sizeof(placeObject->platformData) );

			if( findNearestPlanetPoint(placeObject->xform) == -1 )
			{
				delete placeObject;
				return;
			}
		}
		else if( data.bJumpGate )
		{
			delete placeObject;

			TRANSFORM xform = m_xformCursor;
			DWORD wormholeID;

			if( !findNearestWormhole(xform,&wormholeID) )
			{
				return;
			}

			PlaceJumpGate* placeJumpGate  = new PlaceJumpGate;
			placeJumpGate->archname		  = data.archetype;
			placeJumpGate->mainWormholeID = wormholeID;
			placeJumpGate->systemID		  = m_currentSystem->id;
			placeJumpGate->xform          = m_dragger.object->GetTransform();

			// do command and remember it
			if( !m_commandManager.DoCommand(placeJumpGate) )
			{
				delete placeJumpGate;
			}

			return;
		}
		else
		{
			// find the grid section for object
			FPoint point;
			SINGLE cellSize;
			gridCellInfo( point, cellSize, m_dragger.object );

			TRANSFORM xform = m_xformCursor;
			xform.translation.x = point.X;
			xform.translation.y = point.Y;

			// prepare the place object struct
			placeObject->archname = data.archetype;
			placeObject->xform    = xform;
		}

		if( placeObject )
		{
			// is this a valid location for this type of object?
			if( m_dragger.object->SetTransform(placeObject->xform) && m_currentSystem->objectList.ValidatePlacement(m_dragger.object) )
			{
				// do command and remember it
				if( !m_commandManager.DoCommand(placeObject) )
				{
					delete placeObject;
				}
			}
			else
			{
				// this was in invalid command
				delete placeObject;
			}

			// reset to previous location
			m_dragger.object->SetTransform(data.xform);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::clickSelect( CPoint& _point )
{
	if( m_currentSystem )
	{
		IObject* object = screenToObject( _point );
		if( object )
		{
			ObjectQuickList list;
			OBJECTSELECTION->GetList(list);

			// already on list?
			if( list.Find(object->GetID()) )
			{
				OBJECTSELECTION->Remove(object);
			}
			else
			{
				OBJECTSELECTION->Add(object);
			}

			if( EVENTSYS )
				EVENTSYS->Send( CQE_ENTITY_SELECT );
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::clickMovement( CPoint& _point )
{
#if 0
	// can not move onto an object's grid area
	if( OBJECTSELECTION->HasObjects() && screenToObject(_point) != NULL )
	{
		return;
	}

	// find new cursor
	TRANSFORM xform = m_xformCursor;
	xform.translation.x = _point.x;
	xform.translation.y = _point.y;
	xform.translation.z = 0;

	if( !CAMERA->ScreenToPoint(xform.translation.x,xform.translation.y) )
	{
		return;
	}

	// get list of selected objects
	ObjectQuickList list;
	OBJECTSELECTION->GetList(list);

	// find the offset using the first element of the list
	Vector offset = list.front()->GetTransform().translation - xform.translation;
	
	if( CAMERA->ScreenToPoint(xform.translation.x,xform.translation.y) )
	{
		for( ObjectQuickList::iterator it = list.begin(); it != list.end(); it++ )
		{
			TRANSFORM xformMove = (*it)->GetTransform();
			xformMove.translation += offset;
			moveObject( *it, xformMove );
		}
	}
#else
	// can not move onto an object's grid area
	if( OBJECTSELECTION->HasObjects() && screenToObject(_point) != NULL )
	{
		return;
	}

	// get list of selected objects
	ObjectQuickList list;
	OBJECTSELECTION->GetList(list);
	IObject* selectedObject = list.front();

	// only allow one object to be moved at a time
	OBJECTSELECTION->Reset();
	OBJECTSELECTION->Add( selectedObject );

	// update entity bar
	if( EVENTSYS )
		EVENTSYS->Send( CQE_ENTITY_SELECT );

	// save cursor
	TRANSFORM xformCursor = m_xformCursor;

	// find new cursor
	TRANSFORM xform = m_xformCursor;
	xform.translation.x = _point.x;
	xform.translation.y = _point.y;
	xform.translation.z = 0;
	
	if( CAMERA->ScreenToPoint(xform.translation.x,xform.translation.y) )
	{
		// get object data
		ObjectData data;
		selectedObject->GetObjectData( data );

		// find the grid section for object
		m_xformCursor = xform;
		FPoint point;
		SINGLE cellSize;
		gridCellInfo( point, cellSize, selectedObject );

		xform.translation.x = point.X;
		xform.translation.y = point.Y;

		MoveObject* moveObject = new MoveObject;

		moveObject->system   = m_currentSystem;
		moveObject->objectID = data.id;
		moveObject->xformNew = xform;
		moveObject->xformOld = data.xform;

		IObject* obj = findAttachedObject( selectedObject );
		if( obj )
		{
			moveObject->attachedObject = obj->GetID();
		}

		if( !m_commandManager.DoCommand(moveObject) )
		{
			delete moveObject;
		}
	}

	// restore cursor
	m_xformCursor = xformCursor;
#endif
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::dblClickSelect( CPoint& _point )
{
	IObject* object = screenToObject(_point);
	if( object )
	{
		extern INT_PTR CALLBACK DialogProc_ObjectProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

		CreateDialogParam( 
			::AfxGetApp()->m_hInstance, 
			MAKEINTRESOURCE(IDD_OBJPROPS),
			::AfxGetApp()->GetMainWnd()->m_hWnd,
			(DLGPROC)DialogProc_ObjectProperties,
			(DWORD)object );
	}
}

//-----------------------------------------------------------------------------------------------------

IObject* ModeSystem::screenToObject( CPoint& _point )
{
	m_xformCursor.translation.x = _point.x;
	m_xformCursor.translation.y = _point.y;
	m_xformCursor.translation.z = 0;
	
	if( CAMERA->ScreenToPoint(m_xformCursor.translation.x,m_xformCursor.translation.y) == false )
	{
		return NULL;
	}

	for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
	{
		IObject* obj = *it;

		ObjectData data;
		obj->GetObjectData(data);

		if( data.gridSize.x == 0 && data.bJumpGate )
		{
			data.gridSize.x = 2;
		}

		SINGLE cellSize = (GRIDSIZE / 4) * data.gridSize.x;

		NETGRIDVECTOR grid;
		grid.init( data.xform.translation, 0 );

		FPoint point;
		point.X = grid.getX() * GRIDSIZE;
		point.Y = grid.getY() * GRIDSIZE;

		FRect rect;
		rect.UpperLeftCorner.X  = point.X - cellSize;
		rect.UpperLeftCorner.Y  = point.Y - cellSize;
		rect.LowerRightCorner.X = point.X + cellSize;rect.LowerRightCorner.Y = point.Y + cellSize;

		if( rect.isPointInside( FPoint(m_xformCursor.translation.x, m_xformCursor.translation.y) ) )
		{
			return obj;
		}
	}

	for( int i = 0; i < m_currentSystem->jList.GetCount(); i++ )
	{
		JumpPoint& jumpPoint = m_currentSystem->jList.ElementAt(i);

		if( jumpPoint.wormholeObject == NULL )
		{
			continue;
		}

		ObjectData data;
		jumpPoint.wormholeObject->GetObjectData(data);

		SINGLE cellSize = (GRIDSIZE / 4) * data.gridSize.x;

		NETGRIDVECTOR grid;
		grid.init( data.xform.translation, 0 );

		FPoint point;
		point.X = grid.getX() * GRIDSIZE;
		point.Y = grid.getY() * GRIDSIZE;

		FRect rect;
		rect.UpperLeftCorner.X  = point.X - cellSize;
		rect.UpperLeftCorner.Y  = point.Y - cellSize;
		rect.LowerRightCorner.X = point.X + cellSize;
		rect.LowerRightCorner.Y = point.Y + cellSize;

		if( rect.isPointInside( FPoint(m_xformCursor.translation.x, m_xformCursor.translation.y) ) )
		{
			return jumpPoint.wormholeObject;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

IObject* ModeSystem::findAttachedObject( IObject* _baseObject )
{
	ObjectData data;
	_baseObject->GetObjectData(data);

	// is this a wormhole?
	if( data.objectClass == OC_JUMPGATE )
	{
		for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
		{
			IObject* obj = *it;

			ObjectData objData;
			obj->GetObjectData( objData );
			if( objData.bJumpGate && objData.xform.translation.equal(data.xform.translation,10) )
			{
				return obj;
			}
		}
	}

	else if( data.bJumpGate )
	{
		for( int i = 0; i < m_currentSystem->jList.GetCount(); i++ )
		{
			JumpPoint& jumpPoint = m_currentSystem->jList.ElementAt(i);

			if( jumpPoint.wormholeObject->GetTransform().translation.equal(data.xform.translation,10) )
			{
				return jumpPoint.wormholeObject;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::updateDragger( void )
{
	const AssetData* pAssetData = Editor::GetSelectedAsset();

	if( pAssetData && m_currentSystem && pAssetData->archID )
	{
		m_planetCursor.planet = NULL;

		IObject* obj = Object::Create( GAMETYPES->GetArchName(pAssetData->archID) );
		if( obj )
		{
			if( m_dragger.object )
			{
				m_dragger.object->Delete();
				m_dragger.object = NULL;
			}

			m_dragger.object = obj;

			// setting up the number of slots this object will require on a planet
			ObjectData data;
			obj->GetObjectData(data);
			m_dragger.platformData.reset();
			for( U32 i = 0; i < data.slotsNeeded; i++ )
			{
				m_dragger.platformData.slots[i] = true;
			}
			m_dragger.platformData.numSlots = data.slotsNeeded;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::movementUpdate(void* _param)
{
	// have a valid drag object & mode is in placement mode?
	if( m_dragger.object && m_mode == MSM_PLACEMENT )
	{
		CPoint* point = (CPoint*)_param;

		m_xformCursor.translation.x = point->x;
		m_xformCursor.translation.y = point->y;
		m_xformCursor.translation.z = 0;
		CAMERA->ScreenToPoint( m_xformCursor.translation.x, m_xformCursor.translation.y  );

		FPoint p;
		SINGLE cellSize;
		gridCellInfo( p, cellSize, m_dragger.object );

		TRANSFORM xform = m_xformCursor;
		xform.translation.x = p.X;
		xform.translation.y = p.Y;

		findclosetPlanetToCursor();

		// update dragger info like location & is this a valid location for this type of object?
		m_dragger.object->SetTransform(xform);
		m_dragger.bCanBePlaced = m_currentSystem->objectList.ValidatePlacement(m_dragger.object);

		// move a slotted platform?
		ObjectData data;
		m_dragger.object->GetObjectData(data);
		if( data.slotsNeeded )
		{
			int planetSlot = findNearestPlanetPoint(xform);

			if( planetSlot != -1 )
			{
				m_dragger.object->SetTransform(xform);
				m_dragger.bCanBePlaced = validatePlatformPlacement( m_dragger.platformData, m_planetCursor, planetSlot );
			}
		}
		else if( data.bJumpGate )
		{
			m_dragger.bCanBePlaced = findNearestWormhole(xform,NULL,2.0f);
			if( m_dragger.bCanBePlaced )
			{
				m_dragger.object->SetTransform(xform);
			}
		}
	}
	else if( m_dragger.bLassoStart )
	{
		CPoint* point = (CPoint*)_param;

		m_dragger.area.right  = point->x;
		m_dragger.area.bottom = point->y;

		if( m_mode != MSM_LASSO )
		{
			if( m_dragger.area.Height() > 8 || m_dragger.area.Width() > 8 )
			{
				m_dragger.bLassoDragging = true;
				m_mode = MSM_LASSO;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSystem::findclosetPlanetToCursor()
{
	SINGLE distance = FLT_MAX;
	IObject* newPlanet = NULL;

	for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
	{
		IObject* obj = *it;

		ObjectData objectData;
		obj->GetObjectData( objectData );

		void* archetypeData = GAMETYPES->GetArchetypeData(objectData.archetype);

		if( archetypeData && ((BASIC_DATA*)archetypeData)->objClass == OC_PLANETOID )
		{
			Vector v = objectData.xform.translation - m_xformCursor.translation;
			SINGLE d = v.magnitude();
			if( d < distance )
			{
				distance  = d;
				newPlanet = obj;
			}
		}
	}

	if( newPlanet && newPlanet != m_planetCursor.planet )
	{
		m_planetCursor.planet = newPlanet;

		ObjectData data;
		m_planetCursor.planet->GetObjectData( data );

		SINGLE p = (PI * 2.0f) / (SINGLE)MAX_PLANET_POINTS;

		for( int i = 0; i < MAX_PLANET_POINTS; i++ )
		{
			SINGLE radius = (GRIDSIZE / 2) + 300; 
			SINGLE angle = p * i;
			Matrix m;
			m.set_identity();
			m.z_rotate_right( angle );

			m_planetCursor.points[i] = data.xform.translation + (m.get_j() * radius);
		}
	}
}

//----------------------------------------------------------------------------------------------

bool ModeSystem::validatePlatformPlacement( PlatformData& _platformData, PlanetCursor& _planetCursor, int _slotIndex )
{
	ObjectData data;

	// fill out planet cursor
	_planetCursor.reset();

	for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
	{
		IObject* obj = *it;

		if( obj == _planetCursor.planet )
		{
			continue;
		}

		obj->GetObjectData(data);
		if( data.objectClass != OC_PLATFORM || !data.slotsNeeded )
		{
			continue;
		}

		SINGLE maxDistance = GRIDSIZE * 2;
		if( (data.xform.translation - _planetCursor.planet->GetTransform().translation).magnitude() > maxDistance )
		{
			continue;
		}

		// so this is a platform that connects to this planet

		int dataSize = 0;
		PlatformData* platformData = (PlatformData*)obj->GetCustomData(&dataSize);
		if( platformData && dataSize == sizeof(PlatformData) )
		{
			for( int i = 0; i < MAX_PLANET_POINTS; i++ )
			{
				if( platformData->slots[i] )
				{
					_planetCursor.slots[i] = true;
				}
			}
		}
	}

	// find relative slot start and stop based in the slot index
	float numSlots        = _platformData.numSlots;
	float startIndexFloat = ceilf( (float)_slotIndex - (numSlots / 2.0f) );
	float stopIndexFloat  = floorf( (float)_slotIndex + (numSlots / 2.0f) );

	int startIndex = (int)startIndexFloat;
	int stopIndex  = (int)stopIndexFloat;

	// so from start to stop index, the planet needs to be empty
	for( int i = startIndex; i <= stopIndex; i++ )
	{
		if( _planetCursor[i] == true )
		{
			return false;
		}
	}

	// passed the test, set up the data
	_platformData.reset();
	_platformData.numSlots = numSlots;

	for( i = startIndex; i <= stopIndex; i++ )
	{
		_platformData[i] = true;
	}

	return true;
}

//----------------------------------------------------------------------------------------------

bool ModeSystem::testWormholeWarp( Transform& _xform )
{
	TRANSFORM xform = _xform;
	DWORD wormholeID;

	if( !findNearestWormhole(xform,&wormholeID) )
	{
		return false;
	}

	System* mainSystem = Editor::GetSystem( 0, m_currentSystem->id );
	JumpPoint* mainJumpPoint = NULL;

	if( mainSystem )
	{
		mainJumpPoint = mainSystem->jList.FindByJumpIdx( wormholeID );
		if( mainJumpPoint )
		{
			System* pairSystem = Editor::GetSystem( 0, mainJumpPoint->destSystemID );
			if( pairSystem )
			{
				JumpPoint* pairJumpPoint = pairSystem->jList.FindByJumpIdx( mainJumpPoint->destWormholeID );
				if( pairJumpPoint )
				{
					m_currentSystem = NULL;
					CAMPAIGN->GetCurrentScenario()->GetActiveSector()->SetCurrentSystem( pairSystem->id );
					
					System* nextSystem = Editor::GetActiveSystem();
					if( nextSystem )
					{
						Vector newPosition( pairJumpPoint->x, pairJumpPoint->y, 0 );
						CAMERA->SetLookAtPosition( newPosition );
					}

					return true;
				}
			}
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------

int ModeSystem_FindMenuItem(CMenu* Menu, LPCTSTR MenuString)
{
	// FindMenuItem() will find a menu item string from the specified
	// popup menu and returns its position (0-based) in the specified 
	// popup menu. It returns -1 if no such menu item string is found.

	ASSERT(Menu);
	ASSERT(::IsMenu(Menu->GetSafeHmenu()));

	int count = Menu->GetMenuItemCount();
	for (int i = 0; i < count; i++)
	{
		CString str;
		if (Menu->GetMenuString(i, str, MF_BYPOSITION) && (strcmp(str, MenuString) == 0))
		{
			return i;
		}
	}

	return -1;
}

//----------------------------------------------------------------------------------------------

void ModeSystem::EnumFamilyInfo( FamilyInfo& _info )
{
	int count = ::GetMenuItemCount( (HMENU)_info.context );

	MENUITEMINFO menuItemInfo;
	menuItemInfo.cbSize		= sizeof(menuItemInfo);
	menuItemInfo.dwTypeData = (LPSTR)_info.family;
	menuItemInfo.cch		= strlen(menuItemInfo.dwTypeData);
	menuItemInfo.fMask      = MIIM_STRING | MIIM_ID | MIIM_STATE;
	menuItemInfo.fType      = MFT_STRING;
	menuItemInfo.fState     = MFS_ENABLED;
	menuItemInfo.wID        = (count << 8) | 0x00ff;

	::InsertMenuItem( (HMENU)_info.context, count, true, &menuItemInfo );
}

//----------------------------------------------------------------------------------------------

void ModeSystem::EnumObjectInfo( ObjectInfo& _info )
{
}

//----------------------------------------------------------------------------------------------

void ModeSystem::displayContextMenu(HWND hwnd, CPoint& pt) 
{ 
	if( OBJECTSELECTION->HasObjects() && m_currentSystem )
	{
		ObjectQuickList list;
		OBJECTSELECTION->GetList(list);

		m_contextMenuObject = list.front();

		HMENU hmenu;           // top-level menu 
		HMENU hmenuTrackPopup; // shortcut menu 

		// Load the menu resource
		if( m_contextMenuObject )
		{
			ObjectData data;
			m_contextMenuObject->GetObjectData(data);

			if( data.objectClass == OC_JUMPGATE )
			{
				hmenu = LoadMenu(::AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDR_WORMHOLE_MENU));
			}
			else if( list.size() > 1 )
			{
				hmenu = LoadMenu(::AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDR_MENU_GROUPS));
			}
			else 
			{
				hmenu = LoadMenu(::AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDR_OBJECT_MENU));
			}
		}
		else
			return;

		// TrackPopupMenu cannot display the menu bar so get a handle to the first shortcut menu. 
		hmenuTrackPopup = GetSubMenu(hmenu, 0); 

//		// adding object families
//		HMENU hPopUpMenu = ::CreatePopupMenu();
//		CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->EnumFamilyList( *this, (DWORD)hPopUpMenu );
//		if( ::GetMenuItemCount(hPopUpMenu) )
//		{
//			CString addToGroup("Add To Group");
//			AppendMenu(hmenuTrackPopup, MF_POPUP | MF_ENABLED, (DWORD)hPopUpMenu, addToGroup); 
//		}

		// make sure to send all WM_COMMAND messages to MainFrame
		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();

		// Display the shortcut menu. Track the right mouse button.
		DWORD dwFlags = TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON;
		BOOL ret = TrackPopupMenuEx(hmenuTrackPopup, dwFlags, pt.x, pt.y, frame->m_hWnd, NULL); 

		// Destroy the menu.
		DestroyMenu(hmenu);
	}
} 

//----------------------------------------------------------------------------------------------

void ModeSystem::contextEvent( enum CQ_CONTEXTEVENT _event )
{
	// context menu has "One At A Time" logic
	if( !m_contextMenuObject )
	{
		return;
	}

	if( _event == CQE_CE_DELETE )
	{
		deleteObject( m_contextMenuObject );
	}
	else if( _event == CQE_CE_RENAME )
	{
		// TODO: code renaming objects
	}
	else if( _event == CQE_CE_CLONE )
	{
		// TODO: code cloning object instanaces
	}
	else if( _event == CQE_CE_PROPERTIES )
	{
		// in DialogProc_ObjectProperties.cpp
		extern INT_PTR CALLBACK DialogProc_ObjectProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

		CreateDialogParam( 
			::AfxGetApp()->m_hInstance, 
			MAKEINTRESOURCE(IDD_OBJPROPS),
			::AfxGetApp()->GetMainWnd()->m_hWnd,
			(DLGPROC)DialogProc_ObjectProperties,
			(DWORD)m_contextMenuObject );
	}
	else if( _event == CQE_CE_WARP )
	{
		ObjectData data;
		m_contextMenuObject->GetObjectData(data);

		if( data.objectClass == OC_JUMPGATE )
		{
			testWormholeWarp( data.xform );
		}
	}
	else if( _event == CQE_CE_GOTO )
	{
		ObjectData data;
		m_contextMenuObject->GetObjectData(data);
		CAMERA->SetLookAtPosition( data.xform.translation );
	}
	else if( _event == CQE_CE_ADDTOGROUP )
	{
		ObjectFamilyEnum ofe;
		ofe.family = CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily;

		m_bInFocus = false;

		int r = DialogBoxParam( 
					::AfxGetApp()->m_hInstance, 
					MAKEINTRESOURCE(IDD_CHOOSE_GROUP), 
					::AfxGetApp()->GetMainWnd()->m_hWnd, 
					(DLGPROC)DialogProc_ChooseGroup,
					(DWORD)&ofe
					);

		m_bInFocus = true;

		if( r == IDOK && ofe.familyInfoList.size() >= 1 )
		{
			IObjectFamilyEnum::FamilyInfo& nfo = ofe.familyInfoList.front();

			ObjectQuickList list;
			OBJECTSELECTION->GetList(list);

			for( ObjectQuickList::iterator it = list.begin(); it != list.end(); it++ )
			{
				ofe.family->RemoveObjectFromFamily( NULL, *it );
				ofe.family->AddObjectToFamily( nfo.family, *it );
			}

			updateBars();
		}
	}
	else if( _event == CQE_CE_SELECT )
	{
		// what to do here?
	}

	m_contextMenuObject = NULL;
}

//----------------------------------------------------------------------------------------------

void ModeSystem::updateCameraUsingScrollBars()
{
	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		CView* view = frame->GetActiveView();
		if( view )
		{
			Vector lookAt = CAMERA->GetPosition();

			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask  = SIF_ALL;

			::GetScrollInfo(view->m_hWnd, SB_VERT, &si);
			lookAt.y = -si.nPos;

			::GetScrollInfo(view->m_hWnd, SB_HORZ, &si);
			lookAt.x = si.nPos;

			CAMERA->SetPosition( &lookAt, true );
		}
	}
}

//----------------------------------------------------------------------------------------------

void ModeSystem::deleteObject( IObject* _selectedObject )
{
	ObjectData data;
	_selectedObject->GetObjectData(data);

	IObject* linkedObject = findAttachedObject( _selectedObject );

	if( data.bJumpGate )
	{
		DeleteJumpGate* deleteJumpGate = new DeleteJumpGate;
		deleteJumpGate->systemID = m_currentSystem->id;
		deleteJumpGate->object   = _selectedObject;

		if( linkedObject )
			deleteJumpGate->attachedObjectID = linkedObject->GetID();

		if( !m_commandManager.DoCommand(deleteJumpGate) )
		{
			delete deleteJumpGate;
		}
	}
	else
	{
		DeleteObject* deleteObject = new DeleteObject;
		deleteObject->systemID = m_currentSystem->id;
		deleteObject->object   = _selectedObject;

		if( linkedObject )
			deleteObject->attachedObjectID = linkedObject->GetID();

		if( !m_commandManager.DoCommand(deleteObject) )
		{
			delete deleteObject;
		}
	}

	resetObjectSelector();
}

//----------------------------------------------------------------------------------------------

void ModeSystem::moveObject( IObject* _object, Transform& _xform )
{
	// get object data
	ObjectData data;
	_object->GetObjectData( data );

	// find the grid section for object
	TRANSFORM xform = _xform;
	FPoint point;
	SINGLE cellSize;
	gridCellInfo( point, cellSize, _object, &xform );

	xform.translation.x = point.X;
	xform.translation.y = point.Y;

	MoveObject* moveObject = new MoveObject;

	moveObject->system   = m_currentSystem;
	moveObject->objectID = data.id;
	moveObject->xformNew = xform;
	moveObject->xformOld = data.xform;

	IObject* obj = findAttachedObject( _object );
	if( obj )
	{
		moveObject->attachedObject = obj->GetID();
	}

	if( !m_commandManager.DoCommand(moveObject) )
	{
		delete moveObject;
	}
}

//----------------------------------------------------------------------------------------------

void ModeSystem::copyObjectsToClipboard(void)
{
	if( OBJECTSELECTION->HasObjects() )
	{
		COMPTR<IClipboardObject> clip;
		if( OBJECTSELECTION->QueryInterface("IClipboardObject",clip) == GR_OK )
		{
			CLIPBOARD->Copy( *clip.ptr );
		}
	}
}

//----------------------------------------------------------------------------------------------

void ModeSystem::pasteObjectsFromClipboard()
{
	COMPTR<IClipboardObject> clip;
	if( OBJECTSELECTION->QueryInterface("IClipboardObject",clip) == GR_OK )
	{
		if( CLIPBOARD->HasDataForType(clip->GetType()) )
		{
			OBJECTSELECTION->Reset();

			if( CLIPBOARD->Paste(*clip.ptr) )
			{
				ObjectQuickList list;
				OBJECTSELECTION->GetList(list);

				for( ObjectQuickList::iterator it = list.begin(); it != list.end(); )
				{
					IObject* temp = *it;
					it = list.erase(it);
					pasteObject(temp);
					temp->Delete();
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------

void ModeSystem::pasteObject( IObject* _object )
{
	if( _object && m_currentSystem )
	{
		PlaceObject* placeObject = new PlaceObject;
		placeObject->systemID = m_currentSystem->id;

		// get object data
		ObjectData data;
		_object->GetObjectData( data );

		// placing a platform? 
		if( data.slotsNeeded || data.bJumpGate )
		{
			// can not "paste" this
			return;
		}
		else
		{
			// find the grid section for object
			FPoint point;
			SINGLE cellSize;
			gridCellInfo( point, cellSize, _object, &data.xform );

			TRANSFORM xform = data.xform;
			xform.translation.x = point.X;
			xform.translation.y = point.Y;
			xform.translation.z = 0;

			// prepare the place object struct
			placeObject->archname = data.archetype;
			placeObject->xform    = xform;
		}

		if( placeObject )
		{
			_object->SetTransform( placeObject->xform );

			// is this a valid location for this type of object?
			if( !putIntoValidSpot(_object) )
			{
				delete placeObject;
				return;
			}

			// put into "new" place for object
			_object->GetObjectData( data );
			placeObject->xform = data.xform;

			if( !m_commandManager.DoCommand(placeObject) )
			{
				delete placeObject;
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

bool ModeSystem::putIntoValidSpot( IObject* _object )
{
	System* system = m_currentSystem;
	if( !system )
	{
		return false;
	}

	if( system->objectList.ValidatePlacement(_object) )
	{
		return true;
	}

	ObjectData data;
	_object->GetObjectData(data);

	TRANSFORM xformCurrent = data.xform;
	CSize size = data.gridSize;
	while( size.cx < 8 && size.cy < 8 )
	{
		float halfgrid = (GRIDSIZE/2);
		float gridstep = halfgrid * size.cx;

		float minX = -1 * size.cx * halfgrid;
		float maxX =  1 * size.cx * halfgrid;
		float minY = -1 * size.cy * halfgrid;
		float maxY =  1 * size.cy * halfgrid;

		for( float xpos = minX; xpos <= maxX; xpos += gridstep )
		{
			for( float ypos = minY; ypos <= maxY; ypos += gridstep )
			{
				data.xform = xformCurrent;
				data.xform.translation.x = xformCurrent.translation.x + xpos;
				data.xform.translation.y = xformCurrent.translation.y + ypos;
				_object->SetTransform(data.xform);

				if( system->objectList.ValidatePlacement(_object) )
				{
					return true;
				}
			}
		}
		size.cx += data.gridSize.x;
		size.cy += data.gridSize.y;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

void ModeSystem::resolveLassoSelect()
{
	Vector posOne( m_dragger.area.left, m_dragger.area.top, 0 );
	Vector posTwo( m_dragger.area.right, m_dragger.area.bottom, 0 );

	if( !CAMERA->ScreenToPoint(posOne.x,posOne.y) || !CAMERA->ScreenToPoint(posTwo.x,posTwo.y) )
	{
		return;
	}

	FRect lassoArea( 
		__min(posOne.x, posTwo.x),
		__min(posOne.y, posTwo.y),
		__max(posOne.x, posTwo.x),
		__max(posOne.y, posTwo.y)
		);

	OBJECTSELECTION->Reset();

	for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
	{
		IObject* obj = *it;

		FPoint pt( obj->GetTransform().translation.x, obj->GetTransform().translation.y );

		if( lassoArea.isPointInside(pt) )
		{
			OBJECTSELECTION->Add(obj);
		}
	}

	if( OBJECTSELECTION->HasObjects() && EVENTSYS )
	{
		EVENTSYS->Send( CQE_ENTITY_SELECT );
	}
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK DialogProc_ChooseGroup( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)lParam );

		ObjectFamilyEnum* ofe = (ObjectFamilyEnum*)lParam;
		if( ofe->family->EnumFamilyList(*ofe) )
		{
			HWND combo = GetDlgItem(hwndDlg,IDC_COMBO);
			if( combo )
			{
				for( std::list<IObjectFamilyEnum::FamilyInfo>::iterator it = ofe->familyInfoList.begin(); it != ofe->familyInfoList.end(); it++ )
				{
					IObjectFamilyEnum::FamilyInfo& nfo = *it;
					ComboBox_AddString( combo, nfo.family );
				}
			}
		}

		return true;
	}
	else if( uMsg == WM_COMMAND )
	{
		if( LOWORD(wParam) == IDOK )
		{
			ObjectFamilyEnum* ofe = (ObjectFamilyEnum*)GetWindowLong( hwndDlg, GWL_USERDATA );
			HWND combo = GetDlgItem(hwndDlg,IDC_COMBO);
            
			if( ComboBox_GetCurSel(combo) != CB_ERR )
			{
				char buffer[128];
				ComboBox_GetLBText( combo, ComboBox_GetCurSel(combo), buffer );

				// selecting the matching family
				for( std::list<IObjectFamilyEnum::FamilyInfo>::iterator it = ofe->familyInfoList.begin(); it != ofe->familyInfoList.end(); )
				{
					IObjectFamilyEnum::FamilyInfo& nfo = *it;
					if( strcmp(nfo.family,buffer) )
					{
						it = ofe->familyInfoList.erase(it);
					}
					else
					{
						it++;
					}
				}
			}

			EndDialog(hwndDlg,IDOK);
			return true;
		}
		else if( LOWORD(wParam) == IDCANCEL )
		{
			EndDialog(hwndDlg,IDCANCEL);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _ModeSystem : GlobalComponent
{
	ModeSystem * mode;

	virtual void Startup (void)
	{
		MODE_SYSTEM = mode = new DAComponent<ModeSystem>;
		AddToGlobalCleanupList((IDAComponent **) &MODE_SYSTEM);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise( static_cast<IEventCallback *>(mode), &mode->m_eventHandle);
		}
	}
};
static _ModeSystem __ModeSystem;
