//EventGraph.cpp

#include <stdafx.h>
#include "globals.h"
#include "IEffectFile.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "EventGraph.h"
#include "InfoArea.h"

IEffectAction * selectedAction;
IEffectEvent * selectedEvent;

void EventGraph::Deselect()
{
	InfoArea::Deselect();
	selectedAction = NULL;
	selectedEvent = NULL;
	InvalidateEventGraph();
}

void EventGraph::SelectEvent(IEffectEvent * event)
{
	EventGraph::Deselect();
	selectedEvent = event;
	InfoArea::SelectEvent(event);
	InvalidateEventGraph();
}

void EventGraph::SelectAction(IEffectAction * action)
{
	EventGraph::Deselect();
	selectedAction = action;
	InfoArea::SelectAction(action);
	InvalidateEventGraph();
}

S32 fontHeight;
HFONT drawFont;
HBRUSH actionBrush;
HBRUSH eventBrush;
HBRUSH eventSelectedBrush;
HBRUSH eventHighBoarderBrush;
HBRUSH eventLowBoarderBrush;
HBRUSH actionSelectedBrush;
HBRUSH actionHighBoarderBrush;
HBRUSH actionLowBoarderBrush;

RECT viewArea;
RECT neededArea;
S32 sx,sy;
#define FONT_BUFFER 4

bool bPanning = false;
bool bMoving = false;
S32 mouseX;
S32 mouseY;

SINGLE graphZoomLevel = 1.0f;
SINGLE maxGraphZoom = 4.0f;
SINGLE minGraphZoom = 1.0f;
SINGLE zoomSpeed = 0.001f;

S32 graphAreaWidth = 2000;
S32 graphAreaHeight = 2000;

HDC graphDC = NULL;
HBITMAP graphBitmap = NULL;
bool bGraphInvalid = true;

void EventGraph::InvalidateEventGraph()
{
	bGraphInvalid = true;
	InvalidateRect(eventGraph,NULL,false);
}

void allocGraphDC(HDC winDC)
{
	if(!graphDC)
	{
		graphDC = CreateCompatibleDC(winDC);
		graphBitmap = CreateCompatibleBitmap(winDC,graphAreaWidth,graphAreaHeight);
		SelectObject(graphDC,graphBitmap);
	}
};

void drawLine(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2)
{
	SelectObject(hdc,GetStockObject(BLACK_PEN));
	MoveToEx(hdc,x1,y1,NULL);
	LineTo(hdc,x2,y2);
}

void drawRect(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2, HBRUSH type)
{
	RECT rect;
	rect.left = x1;
	rect.right = x2;
	rect.top = y1;
	rect.bottom = y2;
	FillRect(hdc,&rect,type);
}

void handleEventHit(IEffectEvent * event, S32 x, S32 y)
{
	if(!bPanning)
	{
		EventGraph::SelectEvent(event);

		bMoving = true;
		mouseX = x;
		mouseY = y;

		SetCapture(eventGraph);
	}
}

void handleActionHit(IEffectAction * action, S32 x, S32 y)
{
	if(!bPanning)
	{
		EventGraph::SelectAction(action);

		bMoving = true;
		mouseX = x;
		mouseY = y;

		SetCapture(eventGraph);
	}
}

bool hitEventTest(IEffectEvent * event,S32 x,S32 y)
{
	IEffectAction * action = event->GetFirstAction();
	while(action)
	{
		IEffectEvent * subEvent = action->GetFirstEvent();
		while(subEvent)
		{
			if(hitEventTest(subEvent,x,y))
				return true;
			subEvent = subEvent->GetNextEvent();
		}
		if(action->HitTest(x+sx,y+sy))
		{
			handleActionHit(action,x,y);
			return true;
		}
		action = action->GetNextAction();
	}
	if(event->HitTest(x+sx,y+sy))
	{
		handleEventHit(event,x,y);
		return true;
	}
	return false;
}

void hitTest(S32 x, S32 y)
{
	x = (S32)(x*graphZoomLevel);
	y = (S32)(y*graphZoomLevel);
	IEffectEvent * event = EFFECTFILE->GetStartEvent();
	hitEventTest(event,x,y);
};

void updateGraphViewRegion()
{
	neededArea.top = 0;
	neededArea.left = 0;
	neededArea.bottom = (U32)(graphAreaHeight/graphZoomLevel);
	neededArea.right = (U32)(graphAreaWidth/graphZoomLevel);
	GetClientRect(eventGraph,&viewArea);
	if(neededArea.right <= viewArea.right)
	{
		sx = 0;
		SetScrollPos(eventGraph,SB_HORZ,0,true);
		EnableScrollBar(eventGraph,SB_HORZ,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(eventGraph,SB_HORZ,ESB_ENABLE_BOTH);
		S32 width = neededArea.right-viewArea.right;
		SetScrollRange(eventGraph,SB_HORZ,0,width,true);
		if(sx > width)
		{
			sx = width;
			SetScrollPos(eventGraph,SB_HORZ,sx,true);
		}
	}
	if(neededArea.bottom <= viewArea.bottom)
	{
		sy = 0;
		SetScrollPos(eventGraph,SB_VERT,0,true);
		EnableScrollBar(eventGraph,SB_VERT,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(eventGraph,SB_VERT,ESB_ENABLE_BOTH);
		S32 height = neededArea.bottom-viewArea.bottom;
		SetScrollRange(eventGraph,SB_VERT,0,height,true);
		if(sy > height)
		{
			sy = height;
			SetScrollPos(eventGraph,SB_VERT,sy,true);
		}
	}
}

void drawEvent(IEffectEvent * event, HDC hdc);

void drawAction(IEffectAction * action,HDC hdc)
{
	//draw the action at (offset,currentRow)
	RECT rect;
	rect.left = action->GetIconXPos();
	rect.top = action->GetIconYPos();
	rect.bottom = rect.top+fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,action->GetName(),strlen(action->GetName()),&size);
	rect.right = rect.left+size.cx+(2*FONT_BUFFER);
	action->SetDimentions(rect.right-rect.left,rect.bottom-rect.top);

	//now draw the action
	SetBkMode(hdc,TRANSPARENT);
	if(action == selectedAction)
		FillRect(hdc,&rect,actionSelectedBrush);
	else
		FillRect(hdc,&rect,actionBrush);

	drawRect(hdc,rect.left,rect.top,rect.left+1,rect.bottom,actionHighBoarderBrush);
	drawRect(hdc,rect.left,rect.top,rect.right,rect.top+1,actionHighBoarderBrush);
	drawRect(hdc,rect.left,rect.bottom-1,rect.right,rect.bottom,actionLowBoarderBrush);
	drawRect(hdc,rect.right-1,rect.top,rect.right,rect.bottom,actionLowBoarderBrush);

	TextOut(hdc,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,action->GetName(),strlen(action->GetName()));


	//draw child eventss
	IEffectEvent * event = action->GetFirstEvent();
	while(event)
	{
		drawEvent(event,hdc);
		drawLine(hdc,rect.right,rect.top+(fontHeight/2),event->GetIconXPos(),event->GetIconYPos());
		event = event->GetNextEvent();
	}
}

void drawEvent(IEffectEvent * event, HDC hdc)
{
	if(!event)
		return;
	//draw the event at (offset+event->GetOffset(),currentRow)
	RECT rect;
	rect.left = event->GetIconXPos();
	rect.top = event->GetIconYPos();
	rect.bottom = rect.top+fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,event->GetName(),strlen(event->GetName()),&size);
	rect.right = rect.left+size.cx+(2*FONT_BUFFER);
	event->SetDimentions(rect.right-rect.left,rect.bottom-rect.top);

	//now draw the event
	SetBkMode(hdc,TRANSPARENT);
	if(event == selectedEvent)
		FillRect(hdc,&rect,eventSelectedBrush);
	else
		FillRect(hdc,&rect,eventBrush);

	drawRect(hdc,rect.left,rect.top,rect.left+1,rect.bottom,eventHighBoarderBrush);
	drawRect(hdc,rect.left,rect.top,rect.right,rect.top+1,eventHighBoarderBrush);
	drawRect(hdc,rect.left,rect.bottom-1,rect.right,rect.bottom,eventLowBoarderBrush);
	drawRect(hdc,rect.right-1,rect.top,rect.right,rect.bottom,eventLowBoarderBrush);

	TextOut(hdc,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,event->GetName(),strlen(event->GetName()));

	//draw child actions
	IEffectAction * action = event->GetFirstAction();
	while(action)
	{
		drawAction(action,hdc);
		drawLine(hdc,rect.right,rect.top+(fontHeight/2),action->GetIconXPos(),action->GetIconYPos());
		action = action->GetNextAction();
	}

};

void drawEventGraph(HDC hdc)
{
	drawFont = (HFONT)(GetStockObject(DEFAULT_GUI_FONT));
	TEXTMETRIC textMetric;
	GetTextMetrics(hdc,&textMetric);
	fontHeight = textMetric.tmHeight+(2*FONT_BUFFER);

	LOGBRUSH logBrush;
	memset(&logBrush,0,sizeof(LOGBRUSH));
	logBrush.lbStyle = BS_SOLID;
	logBrush.lbColor = RGB(100,100,200);
	eventBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(150,150,255);
	eventSelectedBrush  = CreateBrushIndirect(&logBrush);
		
	logBrush.lbColor = RGB(200,200,255);
	eventHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(50,50,100);
	eventLowBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(200,100,100);
	actionBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(255,150,150);
	actionSelectedBrush  = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(255,200,200);
	actionHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(100,50,50);
	actionLowBoarderBrush = CreateBrushIndirect(&logBrush);

    IEffectEvent * event = EFFECTFILE->GetStartEvent();
	drawEvent(event,hdc);

	DeleteObject(eventBrush);
	DeleteObject(eventSelectedBrush);
	DeleteObject(eventHighBoarderBrush);
	DeleteObject(eventLowBoarderBrush);
	DeleteObject(actionBrush);
	DeleteObject(actionSelectedBrush);
	DeleteObject(actionHighBoarderBrush);
	DeleteObject(actionLowBoarderBrush);
}

LONG CALLBACK eventGraphProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_HSCROLL:
		{
			switch(LOWORD(wParam))
			{
			case SB_LEFT:
				{
				}
				break;
			case SB_RIGHT:
				{
				}
				break;
			case SB_LINELEFT:
				{
					if(sx> 0)
					{
						sx = sx-1;
						SetScrollPos(hWindow,SB_HORZ,sx,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(sx < (neededArea.right-viewArea.right))
					{
						sx = sx+1;
						SetScrollPos(hWindow,SB_HORZ,sx,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					sx = sx-viewArea.right;
					if(sx < 0)
						sx = 0;
					SetScrollPos(hWindow,SB_HORZ,sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					sx = sx+viewArea.right;
					if(sx > (neededArea.right-viewArea.right))
						sx = (neededArea.right-viewArea.right);
					SetScrollPos(hWindow,SB_HORZ,sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					sx = HIWORD(wParam);
					SetScrollPos(hWindow,SB_HORZ,sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_VSCROLL:
		{
			switch(LOWORD(wParam))
			{
			case SB_LEFT:
				{
				}
				break;
			case SB_RIGHT:
				{
				}
				break;
			case SB_LINELEFT:
				{
					if(sy> 0)
					{
						sy = sy-1;
						SetScrollPos(hWindow,SB_VERT,sy,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(sy < (neededArea.bottom-viewArea.bottom))
					{
						sy = sy+1;
						SetScrollPos(hWindow,SB_VERT,sy,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					sy = sy-viewArea.bottom;
					if(sy < 0)
						sy = 0;
					SetScrollPos(hWindow,SB_VERT,sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					sy = sy+viewArea.bottom;
					if(sy > (neededArea.bottom-viewArea.bottom))
						sy = (neededArea.bottom-viewArea.bottom);
					SetScrollPos(hWindow,SB_VERT,sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					sy = HIWORD(wParam);
					SetScrollPos(hWindow,SB_VERT,sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			S16 value = -(S16)(HIWORD(wParam));
			SINGLE change = (value*zoomSpeed);
			SINGLE newZoom = graphZoomLevel + change;
			if(newZoom > maxGraphZoom)
				newZoom  = maxGraphZoom;
			else if (newZoom < minGraphZoom)
				newZoom = minGraphZoom;
			change = newZoom-graphZoomLevel;
			graphZoomLevel = newZoom;

			POINT point;
			point.x = (S32)(LOWORD(lParam));
			point.y = (S32)(HIWORD(lParam));
			RECT rect;
			GetClientRect(hWindow,&rect);
			ScreenToClient(hWindow,&point);
			SINGLE horzPercent = (1.0f-(((SINGLE)(rect.right-point.x))/((SINGLE)rect.right)));
			SINGLE vertPercent = (1.0f-(((SINGLE)(rect.bottom-point.y))/((SINGLE)rect.bottom)) );

			updateGraphViewRegion();

			sx = (S32)(sx + (rect.right*(-change)*horzPercent));
			if(sx < 0 || neededArea.right-viewArea.right < 0)
				sx = 0;
			else if(sx > (neededArea.right-viewArea.right))
				sx = (neededArea.right-viewArea.right);
			SetScrollPos(hWindow,SB_HORZ,sx,true);

			sy = (S32)(sy + (rect.bottom*(-change)*vertPercent));
			if(sy < 0|| neededArea.bottom-viewArea.bottom < 0)
				sy = 0;
			else if(sy > (neededArea.bottom-viewArea.bottom))
				sy = (neededArea.bottom-viewArea.bottom);
			SetScrollPos(hWindow,SB_VERT,sy,true);


			InvalidateRect(hWindow,NULL,false);

			return 1;
		}
		break;
	case WM_MBUTTONDOWN:
		{
			SetFocus(hWindow);
			if(!bMoving)
			{
				bPanning = true;
				mouseX = (S32)(LOWORD(lParam)*graphZoomLevel);
				mouseY = (S32)(HIWORD(lParam)*graphZoomLevel);
				SetCapture(hWindow);
			}
		}
		break;
	case WM_MBUTTONUP:
		{
			if(bPanning)
			{
				bPanning = false;
				ReleaseCapture();
			}
		}
		break;
	case WM_RBUTTONDOWN:
		{
			SetFocus(hWindow);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			SetFocus(hWindow);
			hitTest(LOWORD(lParam),HIWORD(lParam));
		}
		break;
	case WM_LBUTTONUP:
		{
			if(bMoving)
			{
				S32 newX = (S32)(LOWORD(lParam)*graphZoomLevel);
				S32 newY = (S32)(HIWORD(lParam)*graphZoomLevel);
				if(newX != mouseX && newY != mouseY)
				{
					if(selectedAction)
						selectedAction->RealitveIconMove(newX-mouseX,newY-mouseY);
					else if(selectedEvent)
						selectedEvent->RealitveIconMove(newX-mouseX,newY-mouseY);
					mouseX = newX;
					mouseY = newY;
					EventGraph::InvalidateEventGraph();
				}
				bMoving = false;
				ReleaseCapture();
			}
		}
		break;
	case WM_MOUSEMOVE:
		{
			if(bMoving)
			{
				S32 newX = (S32)(LOWORD(lParam)*graphZoomLevel);
				S32 newY = (S32)(HIWORD(lParam)*graphZoomLevel);
				if(newX != mouseX || newY != mouseY)
				{
					if(selectedAction)
						selectedAction->RealitveIconMove(newX-mouseX,newY-mouseY);
					else if(selectedEvent)
						selectedEvent->RealitveIconMove(newX-mouseX,newY-mouseY);
					mouseX = newX;
					mouseY = newY;
					EventGraph::InvalidateEventGraph();
				}
			}
			else if(bPanning)
			{
				S32 newX = (S32)(LOWORD(lParam)*graphZoomLevel);
				S32 newY = (S32)(HIWORD(lParam)*graphZoomLevel);
				if(newX != mouseX || newY != mouseY)
				{
					sx -= newX-mouseX;
					if(sx < 0)
						sx = 0;
					if(sx > (neededArea.right-viewArea.right))
						sx = (neededArea.right-viewArea.right);
					SetScrollPos(hWindow,SB_HORZ,sx,true);

					sy -= newY-mouseY;
					if(sy < 0)
						sy = 0;
					if(sy > (neededArea.bottom-viewArea.bottom))
						sy = (neededArea.bottom-viewArea.bottom);
					SetScrollPos(hWindow,SB_VERT,sy,true);
					mouseX = newX;
					mouseY = newY;
					InvalidateRect(eventGraph,NULL,false);
				}
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWindow,&ps);
			
			updateGraphViewRegion();

			if(bGraphInvalid)
			{
				allocGraphDC(hdc);
				HBRUSH brush = (HBRUSH)(GetStockObject(DKGRAY_BRUSH));
				RECT rect;
				rect.top = 0;
				rect.bottom = graphAreaHeight;
				rect.left = 0;
				rect.right = graphAreaWidth;
				FillRect(graphDC,&rect,brush);

				drawEventGraph(graphDC);
				bGraphInvalid = false;
			}

			RECT clientRect;
			GetClientRect(hWindow,&clientRect);
			StretchBlt(hdc,0,0,clientRect.right,clientRect.bottom,graphDC,sx,sy,(int)(clientRect.right*graphZoomLevel),(int)(clientRect.bottom*graphZoomLevel),SRCCOPY);
			EndPaint(hWindow,&ps);
		}
		return 1;
	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}
