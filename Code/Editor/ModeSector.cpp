//
// ModeSector
//

#include "stdafx.h"
#include "globals.h"

#include "Mode.h"
#include "Startup.h"
#include "CQTrace.h"
#include "GameTypes.H"
#include "Scenario.h"
#include "Campaign.h"
#include "SystemStructs.h"
#include "TRect.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_SYSTEM_SIZE (MAX_SECTOR_SIZE / 3) // max size is one third of sector
#define MIN_SYSTEM_SIZE (MAX_SECTOR_SIZE / 6) // min size is one sixth of sector

class ModeSector : public IMode, public IEventCallback
{
public:

	BEGIN_DACOM_MAP_INBOUND(ModeSector)
		DACOM_INTERFACE_ENTRY(IMode)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	virtual bool OnCreate( LPCREATESTRUCT lpcs, CCreateContext* pContext ){ return true; }

	virtual bool Start();

	virtual bool Stop();

	virtual void Update();

	virtual void Draw();

	// IEventCallback methods

	DEFMETHOD(Notify) (U32 message, void *param);

	// locals

	enum
	{
		MAX_BUFFER = 1024,

		COLOR_GREY   = RGB(128,128,128),
		COLOR_BLACK  = RGB(0,0,0),
		COLOR_BLUE   = RGB(0,0,200),
		COLOR_GREEN  = RGB(0,200,0),
		COLOR_RED    = RGB(200,0,0),
		COLOR_YELLOW = RGB(200,200,0),
		COLOR_PURPLE = RGB(200,0,200),
		COLOR_ORANGE = RGB(255,102,51),

		WINDOW_ID = WM_USER + 101,
	};

	enum Mode
	{
		M_NOTHING,
		M_CREATING,
		M_SELECTING,
		M_DRAGGING,
		M_CONNECTING,
	};

	struct DragInfo
	{
		bool   bActive;
		Mode   mode;
		CRect  rect;
		CPoint point;
		U8     systemID;

		void reset()
		{
			mode     = M_NOTHING;
			bActive  = false;
			systemID = 0xFF;
			rect.SetRect(0,0,0,0);
			point = CPoint(0,0);
		}
	};

	struct PointPair
	{
		CPoint one;
		CPoint two;
	};

	U32        m_eventHandle;
	bool       m_bInit;
	IScenario* m_scenario;
	CBitmap    m_backbuffer;
	short*     m_bits;
	CRect      m_rectArea;
	DragInfo   m_dragInfo;
	DWORD      m_nextJumpId;
	CWnd       m_childWindow;

	ModeSector()
	{
		m_scenario    = NULL;
		m_bits        = NULL;
		m_nextJumpId  = 1;
		m_bInit       = false;
		m_eventHandle = 0;
	}

	~ModeSector()
	{
		if( m_bInit )
		{
			if( m_bits )
			{
				delete m_bits;
				m_bits = NULL;
			}
		}
		m_scenario    = NULL;
		m_bits        = NULL;
		m_nextJumpId  = 1;
		m_bInit       = false;
		m_eventHandle = 0;
	}

	void drawGrid( CPaintDC& _dc );
	void drawSystems( CPaintDC& _dc );
	void drawJumpLines( CPaintDC& _dc );
	void drawDragger( CPaintDC& _dc );

	void onButtonDown( MSG* );
	void onButtonUp( MSG* );
	void onMouseMove( MSG* );
	void onKeyUp( MSG* );

	void fixupCoords();
	void createSystem( CRect& );
	bool setCurrentSystem( System* );

	System* selectSystem( CPoint& );
	System* selectSystemById( BYTE _id );

	void invalidate( CRect* _rect = NULL )
	{
		if( m_childWindow.m_hWnd )
		{
			m_childWindow.InvalidateRect( _rect, true );
		}
	}
};

//-----------------------------------------------------------------------------------------------------

bool ModeSector::Start()
{ 
	CQEDITORMODE = EM_SECTOR;

	if( m_childWindow.m_hWnd == NULL )
	{
		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
		if( frame )
		{
			CView* view = frame->GetActiveView();
			if( view )
			{
				DWORD dwFlags = CS_VREDRAW | CS_HREDRAW;
				CString strMyClass = AfxRegisterWndClass( dwFlags, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(NULL_BRUSH), ::LoadIcon(NULL, IDI_APPLICATION) );

				DWORD dwStyle = WS_CHILD;
				CRect rect(0,0,0,0);
				m_childWindow.Create( strMyClass, "sector_window", dwStyle, rect, view, WINDOW_ID );
			}
		}
	}

	if( !m_bInit )
	{
		m_bInit = true;
		m_bits = new short[MAX_BUFFER * MAX_BUFFER];
		m_backbuffer.CreateBitmap( MAX_BUFFER, MAX_BUFFER, 1, 16, m_bits );
	}

	// prepare the sector to be manipulated
	if( m_scenario != CAMPAIGN->GetCurrentScenario() )
	{
		if( m_scenario )
		{
			m_scenario->Finish(0);
		}

		m_scenario = CAMPAIGN->GetCurrentScenario();

		if( m_scenario )
		{
			m_scenario->Prepare(0);
		}
	}

	// setup window frame
	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		CView* view = frame->GetActiveView();
		if( view )
		{
			::ShowScrollBar( view->m_hWnd, SB_BOTH, false );
		}

		CRect rect;
		view->GetClientRect( rect );
		m_childWindow.SetWindowPos( NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW );
	}

	// find the "highest jump index to make the NEXT unique id for jump points
	if( m_scenario && m_scenario->GetActiveSector() )
	{
		ISector* sector = m_scenario->GetActiveSector();

		for( U32 i = 0; i < MAX_SYSTEMS; i++ )
		{
			System* system = sector->FindSystemByIdx(i);
			if( system )
			{
				for( int j = 0; j < system->jList.GetCount(); j++ )
				{
					JumpPoint& point = system->jList.ElementAt(j);

					if( point.id > m_nextJumpId )
					{
						m_nextJumpId = point.id + 1;
					}
				}
			}
		}
	}
	

	fixupCoords();
	invalidate();
	return true; 
}

//-----------------------------------------------------------------------------------------------------

bool ModeSector::Stop()
{ 
	m_childWindow.SetWindowPos( NULL, 0, 0, 0, 0, SWP_HIDEWINDOW );
	return true;
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::Update()
{
	if( !m_bInit )
	{
		Start();
	}

	IScenario* scenario = NULL;
	if( CAMPAIGN )
		scenario = CAMPAIGN->GetCurrentScenario();

	if( scenario != m_scenario )
	{
		// TODO: clean up scenario detail here
		m_scenario = scenario;
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::Draw()
{
	if( !m_scenario || !m_scenario->GetActiveSector() ) return;

	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		CView* view = frame->GetActiveView();
		if( view && m_childWindow.m_hWnd )
		{
			CRect rect;
			view->GetClientRect( rect );
			m_childWindow.SetWindowPos( NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW );

			m_childWindow.GetClientRect(m_rectArea);

			CPaintDC dc( &m_childWindow );
			dc.SelectObject(m_backbuffer);

			drawGrid(dc);
			drawSystems(dc);
			drawJumpLines(dc);
			drawDragger(dc);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

GENRESULT ModeSector::Notify(U32 message, void *param)
{
	if( CQEDITORMODE == EM_SECTOR && m_scenario )
	{
		if( message < WM_USER )
		{
			MSG* pMsg = (MSG*)param;

			if( pMsg->message == WM_PAINT)
			{
				Update();
				Draw();
			}
			else if( pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE )
			{
				// is this mouse message meant for the View?
				if( m_childWindow.m_hWnd )
				{
					CPoint pt;
					::GetCursorPos( &pt );
					CRect rect;
					m_childWindow.GetWindowRect( rect );
					if( !rect.PtInRect(pt) )
					{
						return GR_OK;
					}
				}

				if( pMsg->message == WM_LBUTTONDOWN )
				{
					onButtonDown(pMsg);
				}
				else if( pMsg->message == WM_LBUTTONUP )
				{
					onButtonUp(pMsg);
				}
				else if( pMsg->message == WM_MOUSEMOVE )
				{
					onMouseMove(pMsg);
				}
			}
			else if( pMsg->message == WM_KEYUP )
			{
				onKeyUp(pMsg);
			}
		}
		else if( message == CQE_WINDOW_RESIZE )
		{
			// initalizing drag info
			m_dragInfo.reset();

			// 
			fixupCoords();
			invalidate();
		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::drawGrid( CPaintDC& _dc )
{
	CBrush brush;
	brush.CreateSolidBrush( COLOR_BLACK );
	_dc.FillRect( m_rectArea, &brush );

	CPen pen;
	pen.CreatePen( PS_SOLID, 1, COLOR_GREY );
	CPen* p = _dc.SelectObject( &pen );

	int wide      = m_rectArea.Width();
	int high      = m_rectArea.Height();
	int wideChunk = wide / MAX_SECTOR_SIZE;
	int highChunk = high / MAX_SECTOR_SIZE;

	for( int i = 0; i <= MAX_SECTOR_SIZE+1; i++ )
	{
		int lineX = wideChunk * i;
		int lineY = highChunk * i;

		_dc.MoveTo( lineX, 0 );
		_dc.LineTo( lineX, high );

		_dc.MoveTo( 0, lineY );
		_dc.LineTo( wide, lineY );
	}

	_dc.SelectObject( p );
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::drawSystems( CPaintDC& _dc )
{
	LOGBRUSH brushType;
	brushType.lbStyle = BS_HOLLOW;

	CBrush brush;
	brush.CreateBrushIndirect( &brushType );

	CPen pen;
	pen.CreatePen( PS_SOLID, 1, COLOR_ORANGE );

	CBrush* b = _dc.SelectObject( &brush );
	CPen* p = _dc.SelectObject( &pen );

	for( int i = 1; i < MAX_SYSTEMS+1; i++ )
	{
		System* s = m_scenario->GetActiveSector()->FindSystemByIdx(i);

		if( s )
		{
			_dc.Ellipse( s->cRect );
		}
	}

	_dc.SelectObject( p );
	_dc.SelectObject( b );
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::drawJumpLines( CPaintDC& _dc )
{
	CArray<PointPair,PointPair> pairList;

	for( int i = 1; i < MAX_SYSTEMS+1; i++ )
	{
		System* pSystem = m_scenario->GetActiveSector()->FindSystemByIdx(i);
		if( pSystem )
		{
			System& system = *pSystem;

			for( int j = 0; j < system.jList.GetSize(); j++ )
			{
				System* s = selectSystemById( system.jList[j].destSystemID );

				// valid System / JumpPoint pair?
				if( s && s->getJumpPoint(system.jList[j].destWormholeID) )
				{
					JumpPoint jSrc = system.jList[j];
					JumpPoint jDst = *s->getJumpPoint(system.jList[j].destWormholeID);

					// get absolute screen positions for points
					CPoint here  = jSrc.cPoint;
					CPoint there = jDst.cPoint;

					// try to find a duplicate pair
					PointPair p;
					p.one = here;
					p.two = there;

					bool dupe = false;
					for( int k = 0; k < pairList.GetSize(); k++ )
					{
						if( p.one == pairList[k].two && p.two == pairList[k].one )
						{
							dupe = true;
							break;
						}
					}

					if( !dupe )
						pairList.Add( p );
				}
			}

		}
	}

	// draw points

	LOGBRUSH brushType;
	brushType.lbStyle = BS_HOLLOW;

	CBrush brush;
	brush.CreateBrushIndirect( &brushType );

	CPen pen;
	pen.CreatePen( PS_SOLID, 1, COLOR_YELLOW );

	CPen* p = _dc.SelectObject( &pen );
	CBrush* b = _dc.SelectObject( &brush );

	for( int k = 0; k < pairList.GetSize(); k++ )
	{
		CRect rectOne( pairList[k].one, pairList[k].one );
		CRect rectTwo( pairList[k].two, pairList[k].two );

		rectOne.InflateRect( 3, 3 );
		rectTwo.InflateRect( 3, 3 );

		_dc.MoveTo( pairList[k].one );
		_dc.LineTo( pairList[k].two );
		_dc.Rectangle( rectOne );
		_dc.Rectangle( rectTwo );
	}

	_dc.SelectObject( b );
	_dc.SelectObject( p );
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::drawDragger( CPaintDC& _dc )
{
	if( m_dragInfo.mode == M_CREATING )
	{
		_dc.Draw3dRect( m_dragInfo.rect, COLOR_GREEN, COLOR_GREEN );
	}

	else if( m_dragInfo.mode == M_CONNECTING )
	{
		CPen pen;
		pen.CreatePen( PS_SOLID, 1, COLOR_PURPLE );
		CPen* p = _dc.SelectObject( &pen );

		_dc.MoveTo( m_dragInfo.rect.TopLeft() );
		_dc.LineTo( m_dragInfo.rect.BottomRight() );

		_dc.SelectObject( p );
	}

	else if( m_dragInfo.mode == M_SELECTING || m_dragInfo.mode == M_DRAGGING )
	{
		LOGBRUSH brushType;
		brushType.lbStyle = BS_HOLLOW;

		CBrush brush;
		brush.CreateBrushIndirect( &brushType );

		CPen pen;
		pen.CreatePen( PS_SOLID, 1, COLOR_RED );

		CPen* p = _dc.SelectObject( &pen );
		CBrush* b = _dc.SelectObject( &brush );

		_dc.Rectangle( m_dragInfo.rect );

		_dc.SelectObject( b );
		_dc.SelectObject( p );
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::onButtonDown( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	// starting a new system?
	if( fwKeys & MK_SHIFT )
	{
		m_dragInfo.reset();
		m_dragInfo.mode = M_CREATING;
		m_dragInfo.bActive = true;
		m_dragInfo.rect.SetRect( xPos, yPos, xPos, yPos );
	}

	// starting a connection?
	else if( fwKeys & MK_CONTROL )
	{
		System* sys = selectSystem( CPoint(xPos,yPos) );
		if( sys )
		{
			m_dragInfo.reset();
			m_dragInfo.mode = M_CONNECTING;
			m_dragInfo.bActive = true;
			m_dragInfo.rect.SetRect( xPos, yPos, xPos, yPos );
			m_dragInfo.systemID = sys->id;
		}
	}

	// starting a drag?
	else if( fwKeys == MK_LBUTTON && m_dragInfo.mode == M_SELECTING )
	{
		CPoint pt(xPos,yPos);
		System* sys = selectSystem( pt );
		if( sys && sys->id == m_dragInfo.systemID )
		{
			m_dragInfo.mode    = M_DRAGGING;
			m_dragInfo.bActive = true;
			m_dragInfo.point   = m_dragInfo.rect.TopLeft() - pt;
		}
		else
			m_dragInfo.reset();
	}

	// reset drag info
	else
	{
		m_dragInfo.reset();
	}

	invalidate();
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::onButtonUp( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	m_dragInfo.bActive = false;

	// ending a connection?
	if( fwKeys & MK_CONTROL )
	{
		System* sysSrc = selectSystemById( m_dragInfo.systemID );
		System* sysDst = selectSystem( CPoint(xPos,yPos) );
		if( sysDst && sysSrc )
		{
			CPoint pntDst(xPos,yPos);
			pntDst -= sysDst->cRect.TopLeft();

			JumpPoint jumpPointDst;
			jumpPointDst.id       = m_nextJumpId++;
			jumpPointDst.fPoint.X = (float)pntDst.x / (float)sysDst->cRect.Width();
			jumpPointDst.fPoint.Y = (float)pntDst.y / (float)sysDst->cRect.Height();

			CPoint pntSrc(m_dragInfo.rect.left,m_dragInfo.rect.top);
			pntSrc -= sysSrc->cRect.TopLeft();

			JumpPoint jumpPointSrc;
			jumpPointSrc.id       = m_nextJumpId++;
			jumpPointSrc.fPoint.X = (float)pntSrc.x / (float)sysSrc->cRect.Width();
			jumpPointSrc.fPoint.Y = (float)pntSrc.y / (float)sysSrc->cRect.Height();

			// associate one another
			jumpPointSrc.destSystemID   = sysDst->id;
			jumpPointSrc.destWormholeID = jumpPointDst.id;
			jumpPointDst.destSystemID   = sysSrc->id;
			jumpPointDst.destWormholeID = jumpPointSrc.id;

			// add to systems' jumplists
			sysDst->jList.Add(jumpPointDst);
			sysSrc->jList.Add(jumpPointSrc);

			// reset all
			m_dragInfo.reset();
			fixupCoords();
		}
	}
	else if( fwKeys == 0 )
	{
		if( m_dragInfo.mode == M_DRAGGING )
		{
			System* sys = selectSystemById( m_dragInfo.systemID );
			if( sys )
			{
				CRect size = m_rectArea;

				sys->fRect.UpperLeftCorner.X  = (float)m_dragInfo.rect.left   / (float)size.Width();
				sys->fRect.UpperLeftCorner.Y  = (float)m_dragInfo.rect.top    / (float)size.Height();
				sys->fRect.LowerRightCorner.X = (float)m_dragInfo.rect.right  / (float)size.Width();
				sys->fRect.LowerRightCorner.Y = (float)m_dragInfo.rect.bottom / (float)size.Height();

				m_dragInfo.reset();
				fixupCoords();
			}
		}
		else
		{
			System* sys = selectSystem( CPoint(xPos,yPos) );
			if( sys )
			{
				m_dragInfo.reset();
				m_dragInfo.bActive  = true;
				m_dragInfo.mode     = M_SELECTING;
				m_dragInfo.rect     = sys->cRect;
				m_dragInfo.systemID = sys->id;

				setCurrentSystem(sys);
			}
		}
	}

	invalidate();
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::onMouseMove( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	if( m_dragInfo.bActive && m_dragInfo.mode == M_CREATING )
	{
		m_dragInfo.rect.right  = xPos;
		m_dragInfo.rect.bottom = yPos;

		// make a square
		if( m_dragInfo.rect.Width() != m_dragInfo.rect.Height() )
		{
			int maxLen = __max( m_dragInfo.rect.Width(), m_dragInfo.rect.Height() );

			m_dragInfo.rect.right  = m_dragInfo.rect.left + maxLen;
			m_dragInfo.rect.bottom = m_dragInfo.rect.top  + maxLen;
		}

		CRect eraseRect = m_dragInfo.rect;
		eraseRect.InflateRect( 5, 5 );
		invalidate( &eraseRect );
	}

	if( m_dragInfo.bActive && m_dragInfo.mode == M_CONNECTING )
	{
		m_dragInfo.rect.right  = xPos;
		m_dragInfo.rect.bottom = yPos;

		CRect eraseRect = m_dragInfo.rect;
		eraseRect.InflateRect( 5, 5 );
		invalidate( &eraseRect );
	}

	if( m_dragInfo.bActive && m_dragInfo.mode == M_DRAGGING )
	{
		CPoint newPos(xPos,yPos);

		CRect newRect( newPos + m_dragInfo.point, newPos + m_dragInfo.point );
		newRect.right  += m_dragInfo.rect.Width();
		newRect.bottom += m_dragInfo.rect.Height();

		m_dragInfo.rect = newRect;

		invalidate();
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::onKeyUp( MSG* _pMsg )
{
	int nVirtKey   = (int) _pMsg->wParam;    // virtual-key code 
	DWORD lKeyData = _pMsg->lParam;          // key data 

	// is user trying to create a system?
	if( nVirtKey == VK_RETURN && m_dragInfo.mode == M_CREATING )
	{
		// todo(aaj-4/28/2004): querry here to create the system
		createSystem( m_dragInfo.rect );
		m_dragInfo.reset();
	}

	// is the user trying to delete a system?
	if( nVirtKey == VK_DELETE && m_dragInfo.mode == M_SELECTING )
	{
		// todo(aaj-4/30/2004): need a query?
		m_scenario->GetActiveSector()->DeleteSystem( selectSystemById( m_dragInfo.systemID ) );
		invalidate();
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::fixupCoords()
{
	if( m_scenario )
	{
		// note: the FRect of the systems should be maintained so that the CRect can be figured out at any time.

		ISector* sector = m_scenario->GetActiveSector();

		// placing the systems (from 1 - 16)
		for( int i = 1; i < MAX_SYSTEMS+1; i++ )
		{
			System* s = sector->FindSystemByIdx(i);
			if( s )
			{
				System& system = *s;

				CRect& cr = system.cRect;
				FRect& fr = system.fRect;

				cr.left   = fr.UpperLeftCorner.X  * m_rectArea.Width();
				cr.top    = fr.UpperLeftCorner.Y  * m_rectArea.Height();
				cr.right  = fr.LowerRightCorner.X * m_rectArea.Width();
				cr.bottom = fr.LowerRightCorner.Y * m_rectArea.Height();

				// place jump points in system
				for( int j = 0; j < system.jList.GetSize(); j++ )
				{
					CPoint& cpt = system.jList[j].cPoint;
					FPoint& fpt = system.jList[j].fPoint;

					// apply position relative to system rect size
					cpt.x = (float)fpt.X * (float)cr.Width();
					cpt.y = (float)fpt.Y * (float)cr.Height();

					// place inside system rect
					cpt += cr.TopLeft();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeSector::createSystem( CRect& _rect )
{
	System* sys = m_scenario->GetActiveSector()->NewSystem(NULL);
	if( sys )
	{
		float winSize = __max( m_rectArea.Width(), m_rectArea.Height() );

		float chunkSize     = winSize / MAX_SECTOR_SIZE;
		float maxSystemSize = chunkSize * MAX_SYSTEM_SIZE;
		float minSystemSize = chunkSize * MIN_SYSTEM_SIZE;

		float sysLeft = _rect.left;
		float sysTop  = _rect.top;
		float sysSize = __max( _rect.Width(), _rect.Height() );

		if( sysSize < minSystemSize )
		{
			sysSize = minSystemSize;
		}
		else if( sysSize > maxSystemSize )
		{
			sysSize = maxSystemSize;
		}

		// normalize results
		sysLeft /= winSize;
		sysTop  /= winSize;
		sysSize /= winSize;
	
		FRect frect( sysLeft, sysTop, sysLeft + sysSize, sysTop + sysSize );
		sys->fRect = frect;
		sys->bEmpty = false;

		fixupCoords();
		invalidate();
	}
	else
	{
		::MessageBox( NULL, "<Too many systems.>\nNeed to delete system before creating a new one.", "WARNING!", MB_OK );
	}
}

//-----------------------------------------------------------------------------------------------------

System* ModeSector::selectSystemById( BYTE _id )
{
	return m_scenario->GetActiveSector()->FindSystemByIdx( _id );
}

//-----------------------------------------------------------------------------------------------------

System* ModeSector::selectSystem( CPoint& _pt )
{
	for( int i = 1; i < MAX_SYSTEMS+1; i++ )
	{
		System* s = selectSystemById(i);

		if( s && s->cRect.PtInRect(_pt) )
		{
			return s;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------

bool ModeSector::setCurrentSystem( System* _system )
{
	if( _system && m_scenario && m_scenario->GetActiveSector() )
	{
		return m_scenario->GetActiveSector()->SetCurrentSystem( _system->id ) != false;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _ModeSector : GlobalComponent
{
	ModeSector * mode;

	virtual void Startup (void)
	{
		MODE_SECTOR = mode = new DAComponent<ModeSector>;
		AddToGlobalCleanupList((IDAComponent **) &MODE_SECTOR);
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
static _ModeSector __ModeSector;

