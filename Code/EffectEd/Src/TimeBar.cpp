//TimeBar.cpp

#include <stdafx.h>
#include "globals.h"
#include "IEffectFile.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "TimeBar.h"
#include "InfoArea.h"

IEffectAction * selectedAction;
IEffectEvent * selectedEvent;

void TimeBar::Deselect()
{
	InfoArea::Deselect();
	selectedAction = NULL;
	selectedEvent = NULL;
}

void TimeBar::SelectEvent(IEffectEvent * event)
{
	TimeBar::Deselect();
	selectedEvent = event;
	InfoArea::SelectEvent(event);
}

void TimeBar::SelectAction(IEffectAction * action)
{
	TimeBar::Deselect();
	selectedAction = action;
	InfoArea::SelectAction(action);
}

S32 fontHeight;
HFONT drawFont;
HBRUSH actionBrush;
HBRUSH eventBrush;

RECT viewArea;
RECT neededArea;
S32 sx,sy;
#define FONT_BUFFER 4
#define SECOND_SCALE 10

void computeSizeNeed(HDC hdc, U32 xOffset,IEffectEvent * event,RECT & neededArea);

void computeSizeNeed(HDC hdc, U32 xOffset,IEffectAction * action,RECT & neededArea)
{
	neededArea.bottom += fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,action->GetName(),strlen(action->GetName()),&size);
	
	S32 right = (xOffset*SECOND_SCALE)+size.cx+(2*FONT_BUFFER);
	if(right > neededArea.right)
		neededArea.right = right;

	if(action->IsOpen())
	{
		IEffectEvent * event = action->GetFirstEvent();
		while(event)
		{
			computeSizeNeed(hdc,xOffset,event,neededArea);
			event = event->GetNextEvent();
		}
	}
}

void computeSizeNeed(HDC hdc, U32 xOffset,IEffectEvent * event,RECT & neededArea)
{
	if(!event)
		return;
	neededArea.bottom += fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,event->GetName(),strlen(event->GetName()),&size);
	
	S32 right = (S32)(((xOffset+event->GetOffset())*SECOND_SCALE)+size.cx+(2*FONT_BUFFER));
	if(right > neededArea.right)
		neededArea.right = right;

	IEffectAction * action = event->GetFirstAction();
	while(action)
	{
		computeSizeNeed(hdc,(U32)(xOffset+event->GetOffset()),action,neededArea);
		action = action->GetNextAction();
	}
}

void updateViewRegion(HDC hdc)
{
	neededArea.top = 0;
	neededArea.left = 0;
	neededArea.bottom = 0;
	neededArea.right = 0;
	IEffectEvent * event = EFFECTFILE->GetStartEvent();
	computeSizeNeed(hdc,0,event,neededArea);
	GetClientRect(timeBar,&viewArea);
	if(neededArea.right <= viewArea.right)
	{
		sx = 0;
		SetScrollPos(timeBar,SB_HORZ,0,true);
		EnableScrollBar(timeBar,SB_HORZ,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(timeBar,SB_HORZ,ESB_ENABLE_BOTH);
		S32 width = neededArea.right-viewArea.right;
		SetScrollRange(timeBar,SB_HORZ,0,width,true);
		if(sx > width)
		{
			sx = width;
			SetScrollPos(timeBar,SB_HORZ,sx,true);
		}
	}
	if(neededArea.bottom <= viewArea.bottom)
	{
		sy = 0;
		SetScrollPos(timeBar,SB_VERT,0,true);
		EnableScrollBar(timeBar,SB_VERT,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(timeBar,SB_VERT,ESB_ENABLE_BOTH);
		S32 height = neededArea.bottom-viewArea.bottom;
		SetScrollRange(timeBar,SB_VERT,0,height,true);
		if(sy > height)
		{
			sy = height;
			SetScrollPos(timeBar,SB_VERT,sy,true);
		}
	}
}


void drawEvent(HWND hWindow, HDC hdc, IEffectEvent * event, U32 & currentRow, SINGLE offset, S32 parentHeight);

void drawRect(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2, HBRUSH type)
{
	RECT rect;
	rect.left = x1;
	rect.right = x2;
	rect.top = y1;
	rect.bottom = y2;
	FillRect(hdc,&rect,type);
}

void drawAction(HWND hWindow, HDC hdc, IEffectAction * action, U32 & currentRow, SINGLE offset)
{
	//draw the action at (offset,currentRow)
	RECT rect;
	rect.left = (U32)(offset*SECOND_SCALE)-sx;
	rect.top = currentRow*fontHeight-sy;
	rect.bottom = rect.top+fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,action->GetName(),strlen(action->GetName()),&size);
	IEffectEvent * event = action->GetFirstEvent();
	S32 width;
	if(event)
		width = max(size.cx+(2*FONT_BUFFER)+fontHeight,action->GetDrawWidth()*SECOND_SCALE);
	else
		width = max(size.cx+(2*FONT_BUFFER),action->GetDrawWidth()*SECOND_SCALE);
	rect.right = rect.left+width;
	SetBkMode(hdc,TRANSPARENT);
	FillRect(hdc,&rect,actionBrush);
	drawRect(hdc,0,rect.bottom,viewArea.right,rect.bottom+1,(HBRUSH)GetStockObject(LTGRAY_BRUSH));

	if(action == selectedAction)
	{
		FrameRect(hdc,&rect,(HBRUSH)(GetStockObject(WHITE_BRUSH)));
	}
	if(event)
	{
		drawRect(hdc,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,rect.left+fontHeight-FONT_BUFFER,rect.bottom-FONT_BUFFER,(HBRUSH)GetStockObject(WHITE_BRUSH));
		drawRect(hdc,rect.left+(FONT_BUFFER*2),((rect.top+rect.bottom)/2)-1,rect.left+fontHeight-(FONT_BUFFER*2),((rect.top+rect.bottom)/2)+1,(HBRUSH)GetStockObject(BLACK_BRUSH));
		if(!action->IsOpen())
			drawRect(hdc,(rect.left+(fontHeight/2))-1,rect.top+(FONT_BUFFER*2),(rect.left+(fontHeight/2))+1,rect.bottom-(FONT_BUFFER*2),(HBRUSH)GetStockObject(BLACK_BRUSH));


		TextOut(hdc,(U32)((offset*SECOND_SCALE)+fontHeight)-sx,currentRow*fontHeight+FONT_BUFFER-sy,action->GetName(),strlen(action->GetName()));
	}
	else
	{
		TextOut(hdc,(U32)((offset*SECOND_SCALE)+FONT_BUFFER)-sx,currentRow*fontHeight+FONT_BUFFER-sy,action->GetName(),strlen(action->GetName()));
	}
	++currentRow;
	//draw child eventss
	if(action->IsOpen())
	{
		while(event)
		{
			drawEvent(hWindow,hdc,event,currentRow,offset,rect.bottom);
			event = event->GetNextEvent();
		}
	}
}

void drawEvent(HWND hWindow, HDC hdc, IEffectEvent * event, U32 & currentRow, SINGLE offset, S32 parentHeight)
{
	if(!event)
		return;
	//draw the event at (offset+event->GetOffset(),currentRow)
	RECT rect;
	rect.left = (U32)((offset+event->GetOffset())*SECOND_SCALE)-sx;
	rect.top = currentRow*fontHeight-sy;
	rect.bottom = rect.top+fontHeight;
	SIZE size;
	GetTextExtentPoint32(hdc,event->GetName(),strlen(event->GetName()),&size);
	rect.right = rect.left+size.cx+(2*FONT_BUFFER);
	SetBkMode(hdc,TRANSPARENT);
	FillRect(hdc,&rect,eventBrush);
	drawRect(hdc,0,rect.bottom,viewArea.right,rect.bottom+1,(HBRUSH)GetStockObject(LTGRAY_BRUSH));
	drawRect(hdc,rect.left,rect.top,viewArea.right,rect.top+1,eventBrush);
	drawRect(hdc,rect.left,parentHeight,rect.left+1,rect.bottom,(HBRUSH)GetStockObject(LTGRAY_BRUSH));

	if(event == selectedEvent)
	{
		FrameRect(hdc,&rect,(HBRUSH)(GetStockObject(WHITE_BRUSH)));
	}
	TextOut(hdc,(U32)((offset+event->GetOffset())*SECOND_SCALE)+FONT_BUFFER-sx,currentRow*fontHeight+FONT_BUFFER-sy,event->GetName(),strlen(event->GetName()));

	++currentRow;
	//draw child actions
	IEffectAction * action = event->GetFirstAction();
	while(action)
	{
		drawAction(hWindow,hdc,action,currentRow,offset+event->GetOffset());
		action = action->GetNextAction();
	}
}

void drawEffectTree(HWND hWindow, HDC hdc)
{
	drawFont = (HFONT)(GetStockObject(DEFAULT_GUI_FONT));
	TEXTMETRIC textMetric;
	GetTextMetrics(hdc,&textMetric);
	fontHeight = textMetric.tmHeight+(2*FONT_BUFFER);

	LOGBRUSH logBrush;
	memset(&logBrush,0,sizeof(LOGBRUSH));
	logBrush.lbStyle = BS_SOLID;
	logBrush.lbColor = RGB(180,180,255);
	eventBrush = CreateBrushIndirect(&logBrush);
	logBrush.lbColor = RGB(255,180,180);
	actionBrush = CreateBrushIndirect(&logBrush);

	updateViewRegion(hdc);

	U32 currentRow = 0;
	IEffectEvent * event = EFFECTFILE->GetStartEvent();
	drawEvent(hWindow,hdc,event,currentRow, 0.0,0);
	
	DeleteObject(eventBrush);
	DeleteObject(actionBrush);
}

bool hitEventTest(IEffectEvent * event, S32 x, S32 y, S32 offset, S32 & currentRow);

bool hitActionTest(IEffectAction * action, S32 x, S32 y, S32 offset, S32 & currentRow)
{
	++currentRow;
	if(y < currentRow*fontHeight)
	{
		if(action->GetFirstEvent())
		{
			RECT rect;
			rect.left = (U32)((offset*SECOND_SCALE)+FONT_BUFFER);
			rect.top = ((currentRow-1)*fontHeight)+FONT_BUFFER;
			rect.bottom = rect.top+fontHeight-FONT_BUFFER;
			rect.right = rect.left+fontHeight;
			if(x >= rect.left && x <= rect.right && y >= rect.top && y <=rect.bottom)
			{
				action->SetOpen(!action->IsOpen());
			}
			else
			{
				TimeBar::SelectAction(action);
			}
		}
		else
		{
			TimeBar::SelectAction(action);
		}
		return true;
	}
	else if(action->IsOpen())
	{
		IEffectEvent * event = action->GetFirstEvent();
		while(event)
		{
			if(hitEventTest(event,x,y,offset,currentRow))
				return true;
			event = event->GetNextEvent();
		}
	}
	return false;
}

bool hitEventTest(IEffectEvent * event, S32 x, S32 y, S32 offset, S32 & currentRow)
{
	++currentRow;
	if(y < currentRow*fontHeight)
	{
		TimeBar::SelectEvent(event);
		return true;
	}
	else
	{
		IEffectAction * action = event->GetFirstAction();
		while(action)
		{
			if(hitActionTest(action,x,y,(S32)(offset+event->GetOffset()),currentRow))
			{
				return true;
			}
			action = action->GetNextAction();
		}
	}
	return false;
}

void hitTest(S32 x, S32 y)
{
	S32 currentRow = 0;
	if(hitEventTest(EFFECTFILE->GetStartEvent(),x,y,0,currentRow))
		InvalidateRect(mainWindow,NULL,false);
};

LONG CALLBACK timeBarProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
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
	case WM_LBUTTONDOWN:
		{
			hitTest(LOWORD(lParam)+sx,HIWORD(lParam)+sy);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWindow,&ps);
			
			HBRUSH brush = (HBRUSH)(GetStockObject(DKGRAY_BRUSH));
			RECT rect;
			GetClientRect(hWindow,&rect);
			FillRect(hdc,&rect,brush);

			drawEffectTree(hWindow, hdc);

			EndPaint(hWindow,&ps);
		}
		return 1;
	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}
