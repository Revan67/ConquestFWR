// SequenceView.cpp : implementation of the SequenceView class
//

#include "stdafx.h"
#include "Sequence.h"
#include <math.h>
#include <assert.h>

#include "SequenceDoc.h"
#include "SequenceView.h"
#include "motion.h"
#include "basecam.h"
#include "ChannelEventTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	PREVIEW_WIDTH	320
#define	PREVIEW_HEIGHT	400


//needed to get at the status bar
extern	SequenceApp theApp;

/////////////////////////////////////////////////////////////////////////////
// SequenceView

IMPLEMENT_DYNCREATE(SequenceView, CScrollView)

BEGIN_MESSAGE_MAP(SequenceView, CScrollView)
	//{{AFX_MSG_MAP(SequenceView)
	ON_WM_MOUSEMOVE()
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SequenceView construction/destruction

SequenceView::SequenceView()
{
	Dragging				=FALSE;
	DrawnOnce				=FALSE;
	PickingTransitionTarget	=FALSE;
	ClickTarget				=-1;
	DropTarget				=-1;
	DragPos.x				=0;
	DragPos.y				=0;
	
	SetScrollSizes(MM_TEXT, CSize(10000, 10000));
}

SequenceView::~SequenceView()
{
	::CloseWindow(ActWnd);
}

BOOL SequenceView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}


static	void	LRectToDRect(const CClientDC *const dc, const CRect *const srct, CRect *drct)
{
	assert(dc);
	assert(srct);
	assert(drct);

	POINT	tleft	=srct->TopLeft();
	POINT	bright	=srct->BottomRight();

	dc->LPtoDP(&tleft);
	dc->LPtoDP(&bright);

	drct->SetRect(tleft, bright);
}



/////////////////////////////////////////////////////////////////////////////
// SequenceView drawing

void SequenceView::OnDraw(CDC* pDC)
{
	SequenceDoc	*doc	=GetDocument();
	int			i, x, j, maxheight;
	float		XWidth;
	POINT		ofs;

	static	float	timey	=0;

	timey	+=0.69f;

	assert(doc);
	assert(pDC);

	CharacterArchetype	*c	=doc->m_CharacterArch;

	if(doc->m_Character)
	{
		theApp.m_BATCH->set_viewport(0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT);
		theApp.m_PIPE->set_window(ActWnd, 0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT);
		theApp.CurChar	=doc->m_Character;
//		theApp.m_ENG->update_instance(theApp.CurChar->deform->get_root(), timey);

//		theApp.m_ENG->update(69);
//		doc->m_Character->Update(timey);
//		theApp.EngineMainDraw(doc->m_Character);
//		doc->m_Character->Render(doc->game_camera, 0);
	}
	if(!c)
	{
		return;
	}
	if(c->GDat->num_sequences)
	{

		//inflate all the rects to get a metric for a wild ass layout guess
		CRect	All(0,0,0,0);

		for(i=0;i < c->GDat->CurCluster;i++)
		{
			CRect	cr(0,
						0,
						c->GDat->ClusterBoxes[i].TextSize.cx,
						c->GDat->ClusterBoxes[i].TextSize.cy);

			All	+=cr;
		}

		XWidth	=(float)sqrt(All.right * All.bottom) * ((float)All.bottom / (float)All.right);

		ofs.x	=ofs.y	=16;

		for(i=x=maxheight=0;i < c->GDat->CurCluster;i++)
		{
			CBrush	ClusterBrush(c->GDat->ClusterBoxes[i].Color);
			CRect	ofsrect(0,
							0,
							c->GDat->ClusterBoxes[i].TextSize.cx,
							c->GDat->ClusterBoxes[i].TextSize.cy);

			if(maxheight < (c->GDat->ClusterBoxes[i].TextSize.cy + 16))
			{
				maxheight	=c->GDat->ClusterBoxes[i].TextSize.cy + 16;
			}

			if(ofs.x > XWidth)
			{
				ofs.x	=16;
				ofs.y	+=maxheight;
			}

			ofsrect	+=ofs;
			for(j=0;j < c->GDat->num_sequences;j++)
			{
				CRect	nodeofs(0,0,0,0);

				if(c->GDat->ClusterNum[j] != i)
				{
					continue;
				}

				CBrush	NodeBrush(c->GDat->NodeBoxes[j].Color);

				nodeofs	=c->GDat->NodeBoxes[j].NodeRect + ofsrect.TopLeft();


				pDC->FillRect(&nodeofs, &NodeBrush);
				pDC->TextOut(nodeofs.left + 8, nodeofs.top + 8, c->sequences[j].name, strlen(c->sequences[j].name));

				//check for a connection
				if(c->sequences[j].end)
				{
					pDC->MoveTo(ofsrect.TopLeft() + c->GDat->NodeBoxes[j].NodeRect.CenterPoint());
					pDC->LineTo(c->GDat->NodeBoxes[(((int)c->sequences[j].end - (int)c->sequences) / sizeof(MotionSequence))].NodeRect.CenterPoint() + ofsrect.TopLeft());
				}
			}
//			pDC->FrameRect(&ofsrect, &ClusterBrush);

			//assign the doc stored cluster rects in worldspace
			c->GDat->ClusterBoxes[i].NodeRect	=ofsrect;

			ofs.x	+=c->GDat->ClusterBoxes[i].TextSize.cx + 16;
		}

		if(!DrawnOnce)	//this fixes up the state extent boxes
		{
			c->CalcStateExtents();
		}

		//draw connections between states if connected
		if(Dragging)
		{
			if((ClickTarget >=0 && DropTarget >=0)
				&& (ClickTarget != DropTarget))
			{
				POINT	pnt, center;
				int		sgn;

				GetClientRect(&All);

				CRect	rct;

				LRectToDRect((CClientDC *)pDC, &All, &rct);

				All	+=All.TopLeft() - rct.TopLeft();

				pnt.x	=All.right - c->GDat->StateListBox.TextSize.cx;
				pnt.y	=All.top + c->GDat->NodeBoxes[0].TextSize.cy * c->sequences[ClickTarget].type
						+c->GDat->NodeBoxes[0].TextSize.cy / 2;

				pDC->MoveTo(pnt);

				pnt.x	-=10;

				pDC->LineTo(pnt);
				pDC->MoveTo(pnt);

				center.y	=All.top + c->GDat->NodeBoxes[0].TextSize.cy * c->sequences[DropTarget].type
							+c->GDat->NodeBoxes[0].TextSize.cy / 2;

				sgn			=((pnt.y - center.y) > 0)? 1 : -1;

				//placehold
				center.x	=center.y;

				center.y	-=((center.y - pnt.y) / 2);

				pnt.y		=center.x;
				center.x	=pnt.x;

				pDC->LineTo(pnt);
				pDC->MoveTo(pnt);

				pnt.x	+=10;

				pDC->LineTo(pnt);
				pDC->MoveTo(center);

				pnt.x	=center.x + 5;
				pnt.y	=center.y + 5 * sgn;

				pDC->LineTo(pnt);
				pDC->MoveTo(center);

				pnt.x	=center.x - 5;

				pDC->LineTo(pnt);
			}
		}

		//draw the wire for dragging
		if(Dragging)
		{
			pDC->MoveTo(c->GDat->ClusterBoxes[c->GDat->ClusterNum[ClickTarget]].NodeRect.TopLeft() + c->GDat->NodeBoxes[ClickTarget].NodeRect.CenterPoint());

			//draw to the mouse position, or pop to a node if over one
			if(ClickTarget >=0 && DropTarget >=0)
			{
				pDC->LineTo(c->GDat->ClusterBoxes[c->GDat->ClusterNum[DropTarget]].NodeRect.TopLeft() + c->GDat->NodeBoxes[DropTarget].NodeRect.CenterPoint());
			}
			else
			{
				pDC->LineTo(DragPos);
			}
		}

		//draw the state list box
		{
			GetClientRect(&All);
			All.left	=All.right - c->GDat->StateListBox.TextSize.cx;
			All.bottom	=c->GDat->NodeBoxes[0].TextSize.cy;

			CRect	rct;

			LRectToDRect((CClientDC *)pDC, &All, &rct);

			All	+=All.TopLeft() - rct.TopLeft();

			for(i=0;i < MT_NUM_MOTION_TYPES;i++)
			{
				CBrush	StateListBrush(c->GDat->StateColors[i]);

				pDC->FillRect(&All, &StateListBrush);

				pDC->SetBkColor(c->GDat->StateColors[i]);
				
				pDC->TextOut(All.left, All.top, MotionTypeNames[i], strlen(MotionTypeNames[i]));
				
				All.bottom	+=c->GDat->NodeBoxes[0].TextSize.cy;
				All.top		+=c->GDat->NodeBoxes[0].TextSize.cy;
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// SequenceView printing

BOOL SequenceView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void SequenceView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void SequenceView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// SequenceView diagnostics

#ifdef _DEBUG
void SequenceView::AssertValid() const
{
	CScrollView::AssertValid();
}

void SequenceView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

SequenceDoc* SequenceView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(SequenceDoc)));
	return (SequenceDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// SequenceView message handlers

//why the standard win32 you might ask?
//try just making an extra view with mfc sometime... just try it, I dare you
//I spent two hours of worthless digging through the docs to do something
//I could do without mfc in 30 seconds... 
static long FAR PASCAL PreviewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
    case WM_CREATE:
        break;

	case WM_KEYDOWN:
//		OnKeyDown(int(wParam));
		break;
	case WM_KEYUP:
//		OnKeyUp(int(wParam));
		break;

/*
    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_CLOSE:
		PostQuitMessage(0);	   // always quit with window valid
		Quit = true;
		return 0;
*/
    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


void SequenceView::OnInitialUpdate() 
{
	WNDCLASS	wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = PreviewWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = theApp.m_hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Preview";
	
	RegisterClass(&wc);
	
	//
	// Create application's main window
	//
	
	ActWnd	=CreateWindowEx(
							0,
							"Preview",
							"Preview",
							WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
							0,
							0,
							PREVIEW_WIDTH,
							PREVIEW_HEIGHT,
							this->m_hWnd,
							NULL,
							theApp.m_hInstance,
							NULL);
	
	::ShowWindow(ActWnd, SW_NORMAL);

	RECT	ClientRect;
	int		Style	=GetWindowLong(ActWnd, GWL_STYLE);

	ClientRect.left		=0;
	ClientRect.top		=0;
	ClientRect.right	=PREVIEW_WIDTH -1;
	ClientRect.bottom	=PREVIEW_HEIGHT - 1;
	AdjustWindowRect(&ClientRect, Style, FALSE);
	{
		int	WWidth	=ClientRect.right - ClientRect.left + 1;
		int	WHeight	=ClientRect.bottom - ClientRect.top + 1;

		::SetWindowPos(ActWnd, this->m_hWnd, 0, 0, WWidth, WHeight, NULL);
	}
	SetTimer(69, 10, NULL);	//trigger this in the view

	CScrollView::OnInitialUpdate();
}

void SequenceView::OnMouseMove(UINT nFlags, CPoint point) 
{
	int				i, StateOver	=-1;
	static	int		LastStateOver	=-1;
	SequenceDoc		*doc			=GetDocument();
	CMDIFrameWnd	*pMain			=NULL;
	CWnd			*pParent		=NULL;
	int				OldDropTarget	=DropTarget;
	CRect			ofsrect;
	static	int		OverStateBox	=-1;
	char			temp[_MAX_PATH];

	CharacterArchetype	*c	=doc->m_CharacterArch;

	assert(doc);

	if(!c)
	{
		return;
	}

	//convert point to logical for scroll fixing
	CClientDC	dc(this);

	OnPrepareDC(&dc);	//set up mapping mode and viewport origin
	dc.DPtoLP(&point);


	if(nFlags & MK_SHIFT && nFlags & MK_LBUTTON && ClickTarget >= 0 && !PickingTransitionTarget)
	{
		Dragging	=TRUE;

		SetCapture();
	}

	//there really should be an isactive type thing
	pParent	=GetParent();
	if(pParent)
	{
		pMain	=((CMDIChildWnd *)pParent)->GetMDIFrame();
	}

	if(pMain)
	{
		if(pMain->MDIGetActive()==(CWnd *)pParent)
		{
			//check for mouseover the state list
			GetClientRect(&ofsrect);

			ofsrect.left	=ofsrect.right - c->GDat->StateListBox.TextSize.cx;
			ofsrect.bottom	=c->GDat->StateListBox.TextSize.cy;

			CRect	rct;

			LRectToDRect(&dc, &ofsrect, &rct);

			ofsrect	+=ofsrect.TopLeft() - rct.TopLeft();

			if(ofsrect.PtInRect(point))	//in the state list rect?
			{
				int	hit	=(point.y - ofsrect.top) / c->GDat->NodeBoxes[0].TextSize.cy;

				if(OverStateBox != hit)
				{
					OverStateBox	=hit;
					c->UpdateNodeColors();
				}

				for(i=0;i < c->GDat->num_sequences;i++)
				{
					if(c->sequences[i].type==hit)
					{
						continue;
					}

					//grey out all other states temporarily
					c->GDat->NodeBoxes[i].Color	=RGB(69,69,69);
				}
				sprintf(temp, "Motion state %s", MotionTypeNames[hit]);
				theApp.SetStatusText(temp);
				//mouseover transitions a bad idea
//				if(doc->m_Character)
//				{
//					doc->m_CharacterArch->start_sequence((MotionType)hit, doc->m_Character);
//				}

				//lazy update colors
				InvalidateRect(NULL, FALSE);
			}
			else
			{
				if(OverStateBox >= 0)
				{
					c->UpdateNodeColors();
					OverStateBox	=-1;

					//lazy update colors
					InvalidateRect(NULL, FALSE);
				}
			}
			//passive mousover stuff for transition target selection
			if(PickingTransitionTarget)
			{
				for(i=0;i < c->GDat->num_sequences;i++)
				{
					c->GetWorldSpaceNodeRect(&ofsrect, i);

					if(ofsrect.PtInRect(point))
					{
						if(i != DropTarget)
						{
							CRect	rct;

							c->GDat->NodeBoxes[i].Color	=RGB(69,69,169);
							LRectToDRect(&dc, &ofsrect, &rct);

							InvalidateRect(&rct, FALSE);

							if(DropTarget >= 0)	//clear last highlight 
							{
								c->GDat->NodeBoxes[DropTarget].Color
									=c->GetStateColorFromNodeIndex(DropTarget);
									
								c->GetWorldSpaceNodeRect(&ofsrect, DropTarget);
								
								CRect	rct;

								LRectToDRect(&dc, &ofsrect, &rct);

								InvalidateRect(&rct, FALSE);
							}

							DropTarget	=i;
						}
						break;
					}
				}
				if(i >= c->GDat->num_sequences)	//loop past end?
				{
					if(DropTarget >= 0)	//clear last highlight 
					{
						c->GDat->NodeBoxes[DropTarget].Color
							=c->GetStateColorFromNodeIndex(DropTarget);

						c->GetWorldSpaceNodeRect(&ofsrect, DropTarget);
						
						CRect	rct;

						LRectToDRect(&dc, &ofsrect, &rct);

						InvalidateRect(&rct, FALSE);
					}
					DropTarget	=-1;
				}
			}
			else
			{
				if((nFlags & MK_SHIFT) || Dragging)	//check for mouseover on all rects
				{
					for(i=0;i < c->GDat->num_sequences;i++)
					{
						c->GetWorldSpaceNodeRect(&ofsrect, i);

						if(ofsrect.PtInRect(point))
						{
							c->SetStateColor(c->sequences[i].type, RGB(69,69,169));

							if(Dragging)
							{
								DropTarget	=i;
								if(c->seq_grid[c->sequences[ClickTarget].type][c->sequences[i].type])
								{
									sprintf(temp, "Make transition from %s to %s  Current transition target:%s",
										MotionTypeNames[c->sequences[ClickTarget].type],
										MotionTypeNames[c->sequences[i].type],
										c->seq_grid[c->sequences[ClickTarget].type][c->sequences[i].type]->name);

									//turn the old selection purple
									int	idx	=((int)c->seq_grid[c->sequences[ClickTarget].type][c->sequences[i].type]
											- (int)c->sequences) / sizeof(MotionSequence);

									c->GDat->NodeBoxes[idx].Color	=RGB(169, 69, 169);									

									//invalidate old sel rect
									c->GetWorldSpaceNodeRect(&ofsrect, idx);
									CRect	rct;

									LRectToDRect(&dc, &ofsrect, &rct);

									InvalidateRect(&rct, FALSE);
								}
								else
								{
									sprintf(temp, "Make transition from %s to %s",
										MotionTypeNames[c->sequences[ClickTarget].type],
										MotionTypeNames[c->sequences[i].type]);
								}

								theApp.SetStatusText(temp);
							}
							else
							{
								sprintf(temp, "Select state to transition to from %s", MotionTypeNames[c->sequences[i].type]);
								theApp.SetStatusText(temp);
							}

							StateOver	=c->sequences[i].type;
							break;
						}
					}

					if(i >= c->GDat->num_sequences)	//loop past end?
					{
						if(Dragging)
						{
							if(DropTarget >= 0)
							{
								if(c->seq_grid[c->sequences[ClickTarget].type][c->sequences[DropTarget].type])
								{
									//clear old purple indication
									int	idx	=((int)c->seq_grid[c->sequences[ClickTarget].type][c->sequences[DropTarget].type]
											- (int)c->sequences) / sizeof(MotionSequence);

									c->GDat->NodeBoxes[idx].Color	=c->GetStateColorFromNodeIndex(idx);

									//invalidate old sel rect
									c->GetWorldSpaceNodeRect(&ofsrect, idx);

									CRect	rct;

									LRectToDRect(&dc, &ofsrect, &rct);

									InvalidateRect(&rct, FALSE);
								}
							}
						}
						DropTarget	=-1;
					}
				}
			}
		}
	}

	if(StateOver != LastStateOver)
	{
		if(nFlags & MK_LBUTTON)
		{
			if(ClickTarget >= 0)
			{
				if(LastStateOver != c->sequences[ClickTarget].type)
				{
					if(LastStateOver >= 0)
					{
						c->SetStateColor(LastStateOver, c->GDat->StateColors[LastStateOver]);
					}
				}
			}
			else
			{
				if(LastStateOver >= 0)
				{
					c->SetStateColor(LastStateOver, c->GDat->StateColors[LastStateOver]);
				}
			}
		}
		else
		{
			if(LastStateOver >= 0)
			{
				c->SetStateColor(LastStateOver,  c->GDat->StateColors[LastStateOver]);
			}
		}

		//repaint
		if(StateOver >= 0)
		{
			CRect	rct;

			LRectToDRect(&dc, &c->GDat->StateBoxes[StateOver].NodeRect, &rct);

			InvalidateRect(&rct, FALSE);
		}
		if(LastStateOver >= 0)
		{
			CRect	rct;

			LRectToDRect(&dc, &c->GDat->StateBoxes[LastStateOver].NodeRect, &rct);

			InvalidateRect(&rct, FALSE);
		}

		LastStateOver	=StateOver;
	}

	//invalidate the rect affected by the previous wire drawing
	if(Dragging)
	{
		CRect	wrect(c->GDat->ClusterBoxes[c->GDat->ClusterNum[ClickTarget]].NodeRect.TopLeft() + c->GDat->NodeBoxes[ClickTarget].NodeRect.CenterPoint(),
				DragPos);

		wrect.NormalizeRect();

		wrect.bottom	+=1;
		wrect.right		+=1;
		wrect.top		-=1;
		wrect.left		-=1;

		CRect	rct;

		LRectToDRect(&dc, &wrect, &rct);

		InvalidateRect(&rct, TRUE);

		if(ClickTarget >=0 && DropTarget >=0)
		{
			DragPos	=c->GDat->ClusterBoxes[c->GDat->ClusterNum[DropTarget]].NodeRect.TopLeft() + c->GDat->NodeBoxes[DropTarget].NodeRect.CenterPoint();
		}
		else
		{
			DragPos	=point;
		}
	}

	//invalidate the connection rect
	if(DropTarget != OldDropTarget)	
	{
		GetClientRect(&ofsrect);
		ofsrect.right	=ofsrect.right - c->GDat->StateListBox.TextSize.cx;
		ofsrect.left	=ofsrect.right - 15;
		ofsrect.bottom	=c->GDat->StateListBox.TextSize.cy;
		ofsrect.top		=0;

		InvalidateRect(&ofsrect, TRUE);
	}

	CScrollView::OnMouseMove(nFlags, point);
}

void SequenceView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(nChar == VK_SHIFT)
	{
		UINT	flg;
		POINT	pnt;
		
		flg	=(-(!(!(GetAsyncKeyState(VK_CONTROL) & 0x8000)))) & MK_CONTROL;
		flg	|=(-(!(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)))) & MK_LBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)))) & MK_RBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_MBUTTON) & 0x8000)))) & MK_MBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_SHIFT) & 0x8000)))) & MK_SHIFT;

		if(GetCursorPos(&pnt))
		{
			ScreenToClient(&pnt);

			//annoying
			CPoint	cpnt(pnt);

			OnMouseMove(flg, cpnt);
		}
	}
	
	CScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void SequenceView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(nChar == VK_SHIFT)
	{
		UINT	flg;
		POINT	pnt;
		
		flg	=(-(!(!(GetAsyncKeyState(VK_CONTROL) & 0x8000)))) & MK_CONTROL;
		flg	|=(-(!(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)))) & MK_LBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)))) & MK_RBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_MBUTTON) & 0x8000)))) & MK_MBUTTON;
		flg	|=(-(!(!(GetAsyncKeyState(VK_SHIFT) & 0x8000)))) & MK_SHIFT;

		if(GetCursorPos(&pnt))
		{
			ScreenToClient(&pnt);

			//annoying
			CPoint	cpnt(pnt);

			OnMouseMove(flg, cpnt);
		}
	}
	
	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void SequenceView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect		ofsrect;
	int			i;
	SequenceDoc	*doc		=GetDocument();

	CharacterArchetype	*c	=doc->m_CharacterArch;

	//convert point to logical for scroll fixing
	CClientDC	dc(this);

	OnPrepareDC(&dc);	//set up mapping mode and viewport origin
	dc.DPtoLP(&point);

	//check for state list hits
	{
		GetClientRect(&ofsrect);

		ofsrect.left	=ofsrect.right - c->GDat->StateListBox.TextSize.cx;
		ofsrect.bottom	=c->GDat->StateListBox.TextSize.cy;

		CRect	rct;

		LRectToDRect(&dc, &ofsrect, &rct);

		ofsrect	+=ofsrect.TopLeft() - rct.TopLeft();

		if(ofsrect.PtInRect(point))	//in the state list rect?
		{
			int	hit	=(point.y - ofsrect.top) / c->GDat->NodeBoxes[0].TextSize.cy;

			if(doc->m_Character)
			{
				doc->m_CharacterArch->start_sequence((MotionType)hit, doc->m_Character);
			}

		}
	}

	//check for rect hits
	for(i=0;i < doc->m_CharacterArch->GDat->num_sequences;i++)
	{
		CRect	ofsrect;
		
		doc->m_CharacterArch->GetWorldSpaceNodeRect(&ofsrect, i);

		if(ofsrect.PtInRect(point))
		{
			ClickTarget	=i;
			break;
		}
	}
	
	CScrollView::OnLButtonDown(nFlags, point);
}

void SequenceView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	int			i;
	SequenceDoc	*doc	=GetDocument();

	assert(doc);

	CharacterArchetype	*c	=doc->m_CharacterArch;

	//convert point to logical for scroll fixing
	CClientDC	dc(this);

	OnPrepareDC(&dc);	//set up mapping mode and viewport origin
	dc.DPtoLP(&point);

	if(PickingTransitionTarget)
	{
		if(ClickTarget >= 0)
		{
			char	temp[256];
			CRect	ofsrect;

			//invalidate rect of current selection
			c->GDat->NodeBoxes[ClickTarget].Color	=c->GetStateColorFromNodeIndex(ClickTarget);

			c->GetWorldSpaceNodeRect(&ofsrect, ClickTarget);

			CRect	rct;

			LRectToDRect(&dc, &ofsrect, &rct);

			InvalidateRect(&rct, FALSE);

			//invalidate rect of old selection
			if(c->seq_grid[c->sequences[TransFrom].type][c->sequences[TransTo].type])
			{
				int	idx	=((int)c->seq_grid[c->sequences[TransFrom].type][c->sequences[TransTo].type]
						- (int)c->sequences) / sizeof(MotionSequence);

				c->GDat->NodeBoxes[idx].Color	=c->GetStateColorFromNodeIndex(idx);

				c->GetWorldSpaceNodeRect(&ofsrect, idx);

				LRectToDRect(&dc, &ofsrect, &rct);

				InvalidateRect(&rct, FALSE);
			}

			sprintf(temp, "Animation %s set for transition from %s to %s", c->sequences[ClickTarget].name, MotionTypeNames[c->sequences[TransFrom].type], MotionTypeNames[c->sequences[TransTo].type]);
			theApp.SetStatusText(temp);

			c->seq_grid[c->sequences[TransFrom].type][c->sequences[TransTo].type]	=&c->sequences[DropTarget];

			doc->SetModifiedFlag();

			ClickTarget				=-1;
			PickingTransitionTarget	=FALSE;
		}
		else
		{
			MessageBeep(MB_ICONEXCLAMATION);
		}
	}
	else if(Dragging)
	{
		//invalidate the rect affected by the previous wire drawing
		CRect	wrect(c->GDat->ClusterBoxes[c->GDat->ClusterNum[ClickTarget]].NodeRect.TopLeft() + c->GDat->NodeBoxes[ClickTarget].NodeRect.CenterPoint(),
				DragPos);

		wrect.NormalizeRect();

		wrect.bottom	+=1;
		wrect.right		+=1;
		wrect.top		-=1;
		wrect.left		-=1;

		CRect	rct;

		LRectToDRect(&dc, &wrect, &rct);

		InvalidateRect(&rct, TRUE);

		ReleaseCapture();

		c->SetStateColor(c->sequences[ClickTarget].type, c->GetStateColorFromNodeIndex(ClickTarget));

		LRectToDRect(&dc, &c->GDat->StateBoxes[c->sequences[ClickTarget].type].NodeRect, &rct);

		InvalidateRect(&rct, TRUE);

		//check for rect hits
		for(i=0;i < c->GDat->num_sequences;i++)
		{
			CRect	ofsrect;
			
			c->GetWorldSpaceNodeRect(&ofsrect, i);

			if(ofsrect.PtInRect(point))
			{
				//see what link was made here
				char	temp[256];

				//time to select an animation to happen when this transition occurs				
				PickingTransitionTarget	=TRUE;

				//check for previous transition setting
				if(c->seq_grid[c->sequences[ClickTarget].type][c->sequences[i].type])
				{
					sprintf(temp, "Select motion to start on transition from %s to %s.  Previous anim=%s",
						MotionTypeNames[c->sequences[ClickTarget].type],
						MotionTypeNames[c->sequences[i].type],
						c->seq_grid[c->sequences[ClickTarget].type][c->sequences[i].type]->name);
				}
				else
				{
					sprintf(temp, "Select motion to start on transition from %s to %s", MotionTypeNames[c->sequences[ClickTarget].type], MotionTypeNames[c->sequences[i].type]);
				}

				theApp.SetStatusText(temp);

				TransTo	=i;

				break;
			}
		}
		TransFrom	=ClickTarget;

		ClickTarget	=-1;
		Dragging	=FALSE;
	}
	
	CScrollView::OnLButtonUp(nFlags, point);
}

void SequenceView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CRect			rct;
	CColorDialog	picker;
	int				hit;
	SequenceDoc		*doc	=GetDocument();
	CharacterArchetype	*c	=doc->m_CharacterArch;

	assert(doc);
	
	//convert point to logical for scroll fixing
	CClientDC	dc(this);

	OnPrepareDC(&dc);	//set up mapping mode and viewport origin
	dc.DPtoLP(&point);

	GetClientRect(&rct);

	rct.left	=rct.right - c->GDat->StateListBox.TextSize.cx;
	rct.bottom	=c->GDat->StateListBox.TextSize.cy;

	CRect	rct2;

	LRectToDRect(&dc, &rct, &rct2);

	rct	+=rct.TopLeft() - rct2.TopLeft();

	if(rct.PtInRect(point))	//in the state list rect?
	{
		if(picker.DoModal())
		{
			hit	=point.y / c->GDat->NodeBoxes[0].TextSize.cy;

			c->GDat->StateColors[hit]	=picker.GetColor();

			//lazy update colors
			InvalidateRect(NULL, FALSE);

			c->UpdateNodeColors();
		}
	}
	
	CScrollView::OnLButtonDblClk(nFlags, point);
}

BOOL SequenceView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) 
{
	CRect			ofsrect;
	SequenceDoc		*doc	=GetDocument();
	CharacterArchetype	*c	=doc->m_CharacterArch;

	GetClientRect(&ofsrect);

	ofsrect.left	=ofsrect.right - c->GDat->StateListBox.TextSize.cx;
	ofsrect.bottom	=c->GDat->StateListBox.TextSize.cy;


	InvalidateRect(&ofsrect, TRUE);

	return CScrollView::OnScroll(nScrollCode, nPos, bDoScroll);
}

void	SequenceView::DrawPreview(void)
{
	static	BOOL	walkin		=FALSE;
	static	BOOL	crouchin	=FALSE;
	static	BOOL	stoppin		=FALSE;

	SequenceDoc		*doc	=GetDocument();

	assert(doc);

	if(doc->m_Character)
	{
		static	float	timey	=0;

		timey	+=0.1f;
//		doc->m_Character->Update(0.269f);
//		game_camera->Update(0.269f);
//		m_ENG->update(0.0269f);
		doc->m_Character->Update(1.0f);
		theApp.game_camera->Update(1.0f);
		theApp.m_ENG->update(0.1f);
/*
		if(timey > 120.0f)
		{
			if(!walkin)
			{
				doc->m_Character->command(CT_WALK);
				walkin	=TRUE;
			}
		}
		if(timey > 696.0f)
		{
			if(!stoppin)
			{
				doc->m_Character->command(CT_STOP);
				stoppin	=TRUE;
			}
		}
		if(timey > 1196.0f)
		{
			if(!crouchin)
			{
				doc->m_Character->command(CT_TOGGLE_SPECIAL);
				crouchin	=TRUE;
			}
		}
		if(timey > 1300)
		{
			timey	=0;
			crouchin	=stoppin	=walkin	=FALSE;
		}
//		doc->m_Character->Update(lCount);
//		CurChar->Update(timey);
//		CurChar->Update(69);
//		m_ENG->update_instance(CurChar->deform->get_root(), 69);
//		game_camera->Update(lCount);
//		game_camera->Update(timey);
//		game_camera->Update(69);
*/		
		theApp.EngineMainDraw(doc->m_Character);
	}	
}

void SequenceView::OnTimer(UINT nIDEvent) 
{
	DrawPreview();
	
	CScrollView::OnTimer(nIDEvent);
}

class PropertyDlg : public CDialog
{
public:
	int					ClickTarget;
	int					NumTEvents;
	char				**TEvents;
	char				**TempTargets;
	CharacterArchetype	*c;

	PropertyDlg(int ct, CharacterArchetype *ca, char **te, int nte, UINT rsid, CWnd *parent) : CDialog(rsid, parent)
	{
		ClickTarget	=ct;
		c			=ca;
		TEvents		=te;
		NumTEvents	=nte;
	}
	~PropertyDlg();

	virtual	BOOL	OnInitDialog(void);
	virtual	void	OnOK();

	//this actually works!  OMFG!
	afx_msg	void	HandleSelChangeEvents(void)
	{
		CComboBox	*ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITEVENTS);
		CComboBox	*ctrl2	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITTARGETS);
		if(ctrl && ctrl2)
		{
			int	idx	=ctrl->GetCurSel();
			if(TempTargets[idx][0])
			{
				ctrl2->SelectString(0, TempTargets[idx]);
			}
			else
			{
				ctrl2->SetCurSel(c->num_scripts);
			}
/*code using actual events
			for(i=0;i < c->num_wait_states;i++)
			{
				if(!strcmp(TEvents[idx], c->wait_events[i])
				{
					ctrl2->SelectString(0, c->sequences[ClickTarget].wait_targets[0]);
				}
			}*/
		}
	}
	afx_msg	void	HandleSelChangeTargets(void)
	{
		CComboBox	*ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITEVENTS);
		CComboBox	*ctrl2	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITTARGETS);
		if(ctrl && ctrl2)
		{
			int	idx	=ctrl->GetCurSel();

			::GetWindowText(ctrl2->m_hWnd, TempTargets[idx], 80);
		}
	}

	DECLARE_MESSAGE_MAP()
};

//no way this will work
BEGIN_MESSAGE_MAP(PropertyDlg, CDialog)
	ON_CBN_SELENDOK(IDC_SEQUENCE_WAITEVENTS, PropertyDlg::HandleSelChangeEvents)
	ON_CBN_SELENDOK(IDC_SEQUENCE_WAITTARGETS, PropertyDlg::HandleSelChangeTargets)
END_MESSAGE_MAP()

PropertyDlg::~PropertyDlg()
{
	int	i;

	if(TempTargets)
	{
		for(i=0;i < NumTEvents;i++)
		{
			delete	[]	TempTargets[i];
		}
		delete	[]	TempTargets;
	}
}

void	PropertyDlg::OnOK(void)
{
	int		i, j, NumNewStates;
	BOOL	bEndChanged	=FALSE;
	char	temp[80];

	//get new num wait states
	for(i=NumNewStates=0;i < NumTEvents;i++)
	{
		if(TempTargets[i][0])
		{
			NumNewStates++;
		}
	}

	delete	[]	c->sequences[ClickTarget].wait_events;
	delete	[]	c->sequences[ClickTarget].wait_targets;

	c->sequences[ClickTarget].num_wait_states	=NumNewStates;

	if(NumNewStates)
	{
		c->sequences[ClickTarget].wait_events	=new char *[NumNewStates];
		c->sequences[ClickTarget].wait_targets	=new char *[NumNewStates];
		for(i=j=0;i < NumTEvents;i++)
		{
			if(TempTargets[i][0])
			{
				c->sequences[ClickTarget].wait_events[j]	=new char[80];
				c->sequences[ClickTarget].wait_targets[j]	=new char[80];

				//watch the j++ below
				strcpy(c->sequences[ClickTarget].wait_events[j], TEvents[i]);
				strcpy(c->sequences[ClickTarget].wait_targets[j++], TempTargets[i]);
			}
		}
	}
	else
	{
		c->sequences[ClickTarget].wait_events	=NULL;
		c->sequences[ClickTarget].wait_targets	=NULL;
	}

	CEdit	*ed	=(CEdit *)GetDlgItem(IDC_SEQUENCE_NAME);

	::GetWindowText(ed->m_hWnd, temp, 80);
	if(strcmp(temp, c->sequences[ClickTarget].name))
	{
		bEndChanged	=TRUE;	//box size changed
	}
	strcpy((char *)c->sequences[ClickTarget].name, temp);

	CComboBox	*ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_SCRIPT);
	::GetWindowText(ctrl->m_hWnd, (char *)c->sequences[ClickTarget].script, 80);

	ed	=(CEdit *)GetDlgItem(IDC_SEQUENCE_TRANSITIONDURATION);
	::GetWindowText(ed->m_hWnd, temp, 80);
	c->sequences[ClickTarget].transition_duration	=atof(temp);

	ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_TYPE);
	c->sequences[ClickTarget].type	=(MotionType)ctrl->GetCurSel();

	ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_END);
	::GetWindowText(ctrl->m_hWnd, temp, 80);
	for(i=0;i < c->num_sequences;i++)
	{
		if(!strcmp(c->sequences[i].name, temp))
		{
			if(c->sequences[ClickTarget].end != &c->sequences[i])
			{
				bEndChanged	=TRUE;
			}
			c->sequences[ClickTarget].end	=&c->sequences[i];
		}
	}

	CButton	*box	=(CButton *)GetDlgItem(IDC_SEQUENCE_LOOP);
	c->sequences[ClickTarget].loop		=box->GetCheck();

	box	=(CButton *)GetDlgItem(IDC_SEQUENCE_REVERSE);
	c->sequences[ClickTarget].reverse	=box->GetCheck();

	box	=(CButton *)GetDlgItem(IDC_SEQUENCE_INTERRUPT);
	c->sequences[ClickTarget].interrupt	=box->GetCheck();

	EndDialog(bEndChanged);
}


BOOL	PropertyDlg::OnInitDialog(void)
{
	int	i, j;

	TempTargets	=new char*[NumTEvents];
	for(i=0;i < NumTEvents;i++)
	{
		TempTargets[i]		=new char[80];
		TempTargets[i][0]	=0;
	}

	//fill in temp targets
	if(c->sequences[ClickTarget].num_wait_states)
	{
		for(i=0;i < c->sequences[ClickTarget].num_wait_states;i++)
		{
			for(j=0;j < NumTEvents;j++)
			{
				if(!strcmp(TEvents[j], c->sequences[ClickTarget].wait_events[i]))
				{
					strcpy(TempTargets[j], c->sequences[ClickTarget].wait_targets[i]);
				}
			}
		}
	}

	CComboBox	*ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_TYPE);
	if(ctrl)
	{
		for(i=0;i < MT_NUM_MOTION_TYPES;i++)
		{
			ctrl->AddString(MotionTypeNames[i]);
		}
	}
	ctrl->SelectString(0, MotionTypeNames[c->sequences[ClickTarget].type]);

	ctrl				=(CComboBox *)GetDlgItem(IDC_SEQUENCE_SCRIPT);
	CComboBox	*ctrl2	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITTARGETS);
	if(ctrl && ctrl2)
	{
		for(i=0;i < c->num_scripts;i++)
		{
			ctrl->AddString(c->script_names[i]);
			ctrl2->AddString(c->script_names[i]);
		}
		ctrl->AddString("");
		ctrl2->AddString("");
	}
	ctrl->SelectString(0, c->sequences[ClickTarget].script);

	ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_END);
	if(ctrl)
	{
		for(i=0;i < c->num_sequences;i++)
		{
			ctrl->AddString(c->sequences[i].name);
		}
		ctrl->AddString("");
	}
	if(c->sequences[ClickTarget].end)
	{
		ctrl->SelectString(0, c->sequences[ClickTarget].end->name);
	}

	ctrl	=(CComboBox *)GetDlgItem(IDC_SEQUENCE_WAITEVENTS);
	if(ctrl)
	{
		for(i=0;i < NumTEvents;i++)
		{
			ctrl->AddString(TEvents[i]);
		}
		if(c->sequences[ClickTarget].num_wait_states)
		{
			ctrl->SelectString(0, c->sequences[ClickTarget].wait_events[0]);
			ctrl2->SelectString(0, c->sequences[ClickTarget].wait_targets[0]);
		}
		CEdit	*ed	=(CEdit *)GetDlgItem(IDC_SEQUENCE_NAME);

		ed->SetWindowText(c->sequences[ClickTarget].name);

		ed	=(CEdit *)GetDlgItem(IDC_SEQUENCE_TRANSITIONDURATION);
		char	temp[80];
		sprintf(temp, "%.2f", c->sequences[ClickTarget].transition_duration);
		ed->SetWindowText(temp);

		CButton	*box	=(CButton *)GetDlgItem(IDC_SEQUENCE_LOOP);
		box->SetCheck(c->sequences[ClickTarget].loop);
		box	=(CButton *)GetDlgItem(IDC_SEQUENCE_REVERSE);
		box->SetCheck(c->sequences[ClickTarget].reverse);
		box	=(CButton *)GetDlgItem(IDC_SEQUENCE_INTERRUPT);
		box->SetCheck(c->sequences[ClickTarget].interrupt);
	}

	return	TRUE;
}

void SequenceView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	SequenceDoc	*doc	=GetDocument();

	assert(doc);

	CharacterArchetype	*c	=doc->m_CharacterArch;

	//convert point to logical for scroll fixing
	CClientDC	dc(this);

	OnPrepareDC(&dc);	//set up mapping mode and viewport origin
	dc.DPtoLP(&point);

	if(!PickingTransitionTarget && !Dragging)
	{
		if(ClickTarget >= 0)
		{
			PropertyDlg	PropDlg(ClickTarget, c, doc->TotalEvents, doc->NumTotalEvents, IDD_SEQUENCEPROPS, this);

			if(PropDlg.DoModal())
			{
				c->CalcUIData();

				//lazy update
				InvalidateRect(NULL, TRUE);
			}

			ClickTarget	=-1;
		}
		else
		{
			MessageBeep(MB_ICONEXCLAMATION);
		}
	}
	
	CScrollView::OnRButtonUp(nFlags, point);
}

void	SequenceView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int			i;
	SequenceDoc	*doc		=GetDocument();

	//convert point to logical for scroll fixing
	{
		CClientDC	dc(this);

		OnPrepareDC(&dc);	//set up mapping mode and viewport origin
		dc.DPtoLP(&point);
	}

	//check for rect hits
	for(i=0;i < doc->m_CharacterArch->GDat->num_sequences;i++)
	{
		CRect	ofsrect;
		
		doc->m_CharacterArch->GetWorldSpaceNodeRect(&ofsrect, i);

		if(ofsrect.PtInRect(point))
		{
			ClickTarget	=i;
			break;
		}
	}
	
	CScrollView::OnRButtonDown(nFlags, point);
}

void SequenceView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	SequenceDoc	*doc	=GetDocument();

	if(nChar=='n')
	{
		doc->m_CharacterArch->CreateNewSequence();

		//lazy update
		InvalidateRect(NULL, TRUE);
	}
	
	CScrollView::OnChar(nChar, nRepCnt, nFlags);
}
