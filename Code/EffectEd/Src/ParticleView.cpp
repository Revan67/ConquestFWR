//ParticleView.cpp

#include "stdafx.h"
#include "globals.h"
#include "ParticleView.h"
#include "resource.h"
#include "IEffectAction.h"
#include "IEffectFile.h"
#include "EventGraph.h"
#include <stdio.h>
#include <commdlg.h>

#include <DACOM.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <IParticleManager.h>

HWND particleWin;

HWND particleHeader;//the top dialog
HWND particleWorkArea; //the work area

static bool registeredPWin = false;

LONG CALLBACK particleWinProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
LONG CALLBACK particleWorkProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK partHeaderProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK newParticleEffectProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
void drawLine(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2);
void drawRect(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2, HBRUSH type);

extern IEffectAction * selectedAction;

#define PARTICLE_WORK_SIZE 2000
S32 partOffX;
S32 partOffY;
RECT partViewArea;
RECT partNeededArea;

HDC particleDC = NULL;
HBITMAP particleBitmap = NULL;
bool bParticleViewInvalid = true;

SINGLE partZoomLevel = 1.0f;
SINGLE maxPartZoom = 4.0f;
SINGLE minPartZoom = 1.0f;
SINGLE partZoomSpeed = 0.001f;

void ParticleView::InvalidateView()
{
	bParticleViewInvalid = true;
	InvalidateRect(particleWorkArea,NULL,false);
}

void allocParticleDC(HDC winDC)
{
	if(!particleDC)
	{
		particleDC = CreateCompatibleDC(winDC);
		particleBitmap = CreateCompatibleBitmap(winDC,PARTICLE_WORK_SIZE,PARTICLE_WORK_SIZE);
		SelectObject(particleDC,particleBitmap);
	}
};

HWND ParticleView::Open()
{
	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = particleWinProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hMainInst;
	wc.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL; //(HBRUSH)(COLOR_APPWORKSPACE+1); // GetStockObject(BLACK_BRUSH); 
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Particle View";

	if(!registeredPWin)
	{
		RegisterClassEx(&wc);
	}

	RECT rect;
	GetClientRect(infoArea,&rect);

	particleWin = CreateWindowEx(
		0,
		wc.lpszClassName,
		"Particle View",
		WS_CHILD, 
		0,
		0,
		rect.right,
		rect.bottom,
		infoArea,
		NULL,
		hMainInst,
		NULL);

	particleHeader = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_PARTICLE_ACTION_HEADER), particleWin, partHeaderProc);

	RECT headerRect;
	GetClientRect(particleHeader,&headerRect);

	wc.lpfnWndProc   = particleWorkProc;
	wc.lpszClassName = "Particle Workarea";
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
	if(!registeredPWin)
	{
		RegisterClassEx(&wc);
	}
	particleWorkArea = CreateWindowEx(
		0,
		wc.lpszClassName,
		"Particle Workarea",
		WS_CHILD|WS_HSCROLL|WS_VSCROLL, 
		0,
		headerRect.bottom,
		rect.right,
		rect.bottom-headerRect.bottom,
		particleWin,
		NULL,
		hMainInst,
		NULL);
	partOffX = 0;
	partOffY = 0;

	registeredPWin = true;

	if(selectedAction)
	{
		IActionParticle * part = selectedAction->GetActionParticle();
		if(part)
		{
			part->ActivateFilterWindows();
		}
	}

	ShowWindow(particleHeader,true);
	ShowWindow(particleWorkArea,true);
	ShowWindow(particleWin,true);

	return particleWin;
};

void ParticleView::Null()
{
	//the InfoArea destroyed our window
	particleWin = NULL;
};

void ParticleView::Resize()
{
	if(particleWin)
	{
		RECT rect;
		GetClientRect(infoArea,&rect);
		RECT headerRect;
		GetClientRect(particleHeader,&headerRect);

		MoveWindow(particleWin,0,0,rect.right,rect.bottom,true);
		MoveWindow(particleWorkArea,0,headerRect.bottom,rect.right,rect.bottom-headerRect.bottom,true);
	}
}

HWND selectedFilterWin;
S32 basePosX;
S32 basePosY;

void ParticleView::EmmiterWinSelect(HWND hWindow,S32 xPos, S32 yPos)
{
	selectedFilterWin = hWindow;
	basePosX =xPos;
	basePosY = yPos;
	SetCapture(hWindow);
}

void ParticleView::EmmiterWinUpdateMove(HWND hWindow,S32 xPos, S32 yPos)
{
	if(hWindow == selectedFilterWin)
	{
		if(selectedAction)
		{
			IActionParticle * part = selectedAction->GetActionParticle();
			if(part)
			{
				IParticleFilter * filter = part->FindFilterFromHWND(hWindow);
				if(filter)
				{
					S32 dx = xPos-basePosX;
					S32 dy = yPos-basePosY;
					if(dx || dy)
					{
						filter->SetWindowPos(filter->GetWinX()+dx,filter->GetWinY()+dy);
						basePosX = xPos;
						basePosY = yPos;
						ParticleView::InvalidateView();
					}
				}
			}
		}
	}
}

void ParticleView::EmmiterWinRelease(HWND hWindow)
{
	if(hWindow == selectedFilterWin)
	{
		selectedFilterWin = NULL;
		ReleaseCapture();
	}
}

HWND connOutputWindow;
U32 connOutputID;

void ParticleView::OutputClicked(HWND hWindow, U32 id)
{
	if(selectedAction)
	{
		IActionParticle * part = selectedAction->GetActionParticle();
		if(part)
		{
			IParticleFilter * filter = part->FindFilterFromHWND(hWindow);
			if(filter)
			{
				filter->DestroyOutput(id);
				connOutputWindow = hWindow;
				connOutputID = id;
				ParticleView::InvalidateView();
			}
		}
	}
}

void ParticleView::InputClicked(HWND hWindow)
{
/*	if(selectedAction)
	{
		IActionParticle * part = selectedAction->GetActionParticle();
		if(part)
		{
			IParticleFilter * filter = part->FindFilterFromHWND(hWindow);
			if(filter)
			{
				if(connOutputWindow)//I have a target to connect to
				{
					IParticleFilter * outFilter = part->FindFilterFromHWND(connOutputWindow);
					if(outFilter)
					{
						if(outFilter != filter)
						{
							outFilter->ConnectOutput(filter,connOutputID);
							filter->ConnectInput(outFilter,connOutputID);
						}
						connOutputWindow = NULL;
						ParticleView::InvalidateView();
					}
				}
			}
		}
	}
*/}

LONG CALLBACK particleWinProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWindow,message,wParam,lParam);
}

HBRUSH filterBkBrush;
HBRUSH filterShadowBrush;
HBRUSH filterHighlightBrush;

#define FONT_BUFFER 3
#define BASE_FILTER_HEIGHT 40
#define FILTER_LINE_HEIGHT 20
#define FILTER_WIDTH 100
#define CIRC_WIDTH 10
#define CIRC_WIDTH_HALF (CIRC_WIDTH >> 1)
#define FONT_POINT_SIZE 9

void drawFilter(HDC hdc,IParticleFilter * filter)
{
	IParticleProgramer * prog = filter->GetProgramer();
	U32 numOutput = prog->GetNumOutput();
	U32 numInput = prog->GetNumInput();
	S32 height = BASE_FILTER_HEIGHT+(numInput*FILTER_LINE_HEIGHT)+(numOutput*FILTER_LINE_HEIGHT);

	//background
	SetBkMode(hdc,TRANSPARENT);
	drawRect(hdc,filter->GetWinX(),filter->GetWinY(),filter->GetWinX()+FILTER_WIDTH,filter->GetWinY()+height,filterBkBrush);

	//draw main border
	drawRect(hdc,filter->GetWinX(),filter->GetWinY(),filter->GetWinX()+FILTER_WIDTH,filter->GetWinY()+1,filterHighlightBrush);
	drawRect(hdc,filter->GetWinX(),filter->GetWinY(),filter->GetWinX()+1,filter->GetWinY()+height,filterHighlightBrush);
	drawRect(hdc,filter->GetWinX(),filter->GetWinY()+height-1,filter->GetWinX()+FILTER_WIDTH,filter->GetWinY()+height,filterShadowBrush);
	drawRect(hdc,filter->GetWinX()+FILTER_WIDTH-1,filter->GetWinY(),filter->GetWinX()+FILTER_WIDTH,filter->GetWinY()+height,filterShadowBrush);

	//draw title boarder
	drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+2,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+3,filterHighlightBrush);
	drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+2,filter->GetWinX()+3,filter->GetWinY()+BASE_FILTER_HEIGHT-2,filterHighlightBrush);
	drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+BASE_FILTER_HEIGHT-3,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+BASE_FILTER_HEIGHT-2,filterShadowBrush);
	drawRect(hdc,filter->GetWinX()+FILTER_WIDTH-3,filter->GetWinY()+2,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+BASE_FILTER_HEIGHT-2,filterShadowBrush);

	//draw param border
	if(numInput ||numOutput)
	{
		drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+2+BASE_FILTER_HEIGHT,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+3+BASE_FILTER_HEIGHT,filterShadowBrush);
		drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+2+BASE_FILTER_HEIGHT,filter->GetWinX()+3,filter->GetWinY()+height-2,filterShadowBrush);
		drawRect(hdc,filter->GetWinX()+2,filter->GetWinY()+height-3,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+height-2,filterHighlightBrush);
		drawRect(hdc,filter->GetWinX()+FILTER_WIDTH-3,filter->GetWinY()+2+BASE_FILTER_HEIGHT,filter->GetWinX()+FILTER_WIDTH-2,filter->GetWinY()+height-2,filterHighlightBrush);
	}

	S32 displayHeight = filter->GetWinY()+FONT_BUFFER;
	//type
	TextOut(hdc,filter->GetWinX()+FONT_BUFFER,displayHeight,PARTMAN->GetFilterName(filter->GetType()),strlen(PARTMAN->GetFilterName(filter->GetType())));
	displayHeight += FILTER_LINE_HEIGHT;
	//name
	TextOut(hdc,filter->GetWinX()+FONT_BUFFER,displayHeight,filter->GetFilterName(),strlen(filter->GetFilterName()));
	displayHeight += FILTER_LINE_HEIGHT;

	for(U32 i = 0; i < numInput; ++i)
	{
		SelectObject(hdc,GetStockObject(BLACK_PEN));
		SelectObject(hdc,GetStockObject(WHITE_BRUSH));
		Ellipse(hdc,filter->GetWinX()+FONT_BUFFER,displayHeight+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2),filter->GetWinX()+FONT_BUFFER+CIRC_WIDTH,displayHeight+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2)+CIRC_WIDTH);
		TextOut(hdc,filter->GetWinX()+FONT_BUFFER+FONT_BUFFER+CIRC_WIDTH,displayHeight,prog->GetInputName(i),strlen(prog->GetInputName(i)));
		displayHeight += FILTER_LINE_HEIGHT;
	}

	for(U32 i = 0; i < numOutput; ++i)
	{
		SelectObject(hdc,GetStockObject(BLACK_PEN));
		SelectObject(hdc,GetStockObject(WHITE_BRUSH));
		Ellipse(hdc,filter->GetWinX()+FILTER_WIDTH-(FONT_BUFFER+CIRC_WIDTH),
			displayHeight+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2),
			filter->GetWinX()+FILTER_WIDTH-(FONT_BUFFER),
			displayHeight+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2)+CIRC_WIDTH);
		TextOut(hdc,filter->GetWinX()+FONT_BUFFER,displayHeight,prog->GetOutputName(i),strlen(prog->GetOutputName(i)));
		displayHeight += FILTER_LINE_HEIGHT;
	}
}

void drawFilterConnections(HDC hdc,IParticleFilter * filter)
{
	SelectObject(hdc,GetStockObject(BLACK_PEN));
	U32 numInput = filter->GetProgramer()->GetNumInput();
	for(U32 i = 0; i < numInput; ++i)
	{
		IParticleFilter * source = NULL;
		U32 sourceOutputID = 0;
		if(filter->GetInputInfo(i,sourceOutputID,source))
		{
			POINT pt1,pt2;

			pt1.x = filter->GetWinX()+CIRC_WIDTH_HALF+FONT_BUFFER;
			pt1.y = filter->GetWinY()+CIRC_WIDTH_HALF+FONT_BUFFER+(i*FILTER_LINE_HEIGHT)+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2);

			pt2.x = source->GetWinX()+FILTER_WIDTH-(CIRC_WIDTH_HALF+FONT_BUFFER);
			pt2.y = source->GetWinY()+CIRC_WIDTH_HALF+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+
				(FILTER_LINE_HEIGHT*sourceOutputID)+(source->GetProgramer()->GetNumInput()*FILTER_LINE_HEIGHT)+
				((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2);

			MoveToEx(hdc,pt1.x,pt1.y,NULL);
			LineTo(hdc,pt2.x,pt2.y);

			SelectObject(hdc,GetStockObject(BLACK_BRUSH));

			Ellipse(hdc,filter->GetWinX()+FONT_BUFFER,
				filter->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+(i*FILTER_LINE_HEIGHT)+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2),
				filter->GetWinX()+FONT_BUFFER+CIRC_WIDTH,
				filter->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+(i*FILTER_LINE_HEIGHT)+CIRC_WIDTH+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2));

			Ellipse(hdc,source->GetWinX()+FILTER_WIDTH-FONT_BUFFER,
				source->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+(FILTER_LINE_HEIGHT*sourceOutputID)+(source->GetProgramer()->GetNumInput()*FILTER_LINE_HEIGHT)+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2),
				source->GetWinX()+FILTER_WIDTH-(FONT_BUFFER+CIRC_WIDTH),
				source->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+(FILTER_LINE_HEIGHT*sourceOutputID)+(source->GetProgramer()->GetNumInput()*FILTER_LINE_HEIGHT)+CIRC_WIDTH+((FILTER_LINE_HEIGHT-CIRC_WIDTH)/2));
		}
	}
}

bool bConnectingFilter = false;
IParticleFilter * sourceConnect = NULL;
IParticleFilter * destConnect = NULL;
U32 connectionOutputID = 0;
U32 connectionInputID = 0;

bool bMovingFilter = false;
IParticleFilter * targetMoveFilter = NULL;
S32 filterMouseX = 0; 
S32 filterMouseY = 0;

bool bPanningFilter = false;

bool filterHitTest(IParticleFilter * filter, S32 xPos, S32 yPos)
{
	IParticleProgramer * prog = filter->GetProgramer();
	U32 numOutput = prog->GetNumOutput();
	U32 numInput = prog->GetNumInput();
	S32 height = BASE_FILTER_HEIGHT+(numInput*FILTER_LINE_HEIGHT)+(numOutput*FILTER_LINE_HEIGHT);
	if(xPos >= filter->GetWinX() && xPos <= filter->GetWinX()+FILTER_WIDTH && 
		yPos >= filter->GetWinY() && yPos <= filter->GetWinY()+height)
	{
		S32 testHeight = filter->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT;

		for(U32 i = 0; i < numInput; ++i)
		{
			//test input click
			if(xPos >= filter->GetWinX()+FONT_BUFFER && xPos <= filter->GetWinX()+FONT_BUFFER+CIRC_WIDTH && 
				yPos >= testHeight+FONT_BUFFER && yPos <= testHeight+FONT_BUFFER+CIRC_WIDTH)
			{
				IParticleFilter * source = NULL;
				U32 sourceOutputID = 0;
				if(filter->GetInputInfo(i,sourceOutputID,source))
				{
					source->DestroyOutput(sourceOutputID);
				}
				destConnect = filter;
				sourceConnect = NULL;
				bConnectingFilter = true;
				connectionInputID = i;
				ParticleView::InvalidateView();
				return true;
			}
			testHeight += FILTER_LINE_HEIGHT;
		}

		for(U32 i = 0; i < numOutput; ++i)
		{
			if(xPos >= filter->GetWinX()+FILTER_WIDTH-(FONT_BUFFER+CIRC_WIDTH) && xPos <= filter->GetWinX()+FILTER_WIDTH-FONT_BUFFER && 
				yPos >= testHeight+FONT_BUFFER && yPos <= testHeight+FONT_BUFFER+CIRC_WIDTH)
			{
				filter->DestroyOutput(i);
				destConnect = NULL;
				sourceConnect = filter;
				bConnectingFilter = true;
				connectionOutputID = i;
				ParticleView::InvalidateView();
				return true;
			}
			testHeight += FILTER_LINE_HEIGHT;
		}

		//ok just a normal click

		filterMouseX = xPos;
		filterMouseY = yPos;
		targetMoveFilter = filter;
		bMovingFilter = true;
		return true;
	}
	return false;
}

bool filterHitTestOpen(IParticleFilter * filter, S32 xPos, S32 yPos)
{
	IParticleProgramer * prog = filter->GetProgramer();
	U32 numOutput = prog->GetNumOutput();
	U32 numInput = prog->GetNumInput();
	S32 height = BASE_FILTER_HEIGHT+(numInput*FILTER_LINE_HEIGHT)+(numOutput*FILTER_LINE_HEIGHT);
	if(xPos >= filter->GetWinX() && xPos <= filter->GetWinX()+FILTER_WIDTH && 
		yPos >= filter->GetWinY() && yPos <= filter->GetWinY()+height)
	{
		filter->OpenWindow();
		return true;
	}
	return false;
}

bool filterHitTestRightClick(IParticleFilter * filter, S32 xPos, S32 yPos)
{
	IParticleProgramer * prog = filter->GetProgramer();
	U32 numOutput = prog->GetNumOutput();
	U32 numInput = prog->GetNumInput();
	S32 height = BASE_FILTER_HEIGHT+(numInput*FILTER_LINE_HEIGHT)+(numOutput*FILTER_LINE_HEIGHT);
	if(xPos >= filter->GetWinX() && xPos <= filter->GetWinX()+FILTER_WIDTH && 
		yPos >= filter->GetWinY() && yPos <= filter->GetWinY()+height)
	{
		POINT point;
		GetCursorPos(&point);
		HMENU menu = LoadMenu(hMainInst,MAKEINTRESOURCE(IDR_FILTER_POPUP));
		HMENU hmenuTrackPopup = GetSubMenu(menu, 0); 
		int command = TrackPopupMenuEx(hmenuTrackPopup,TPM_CENTERALIGN|TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,particleWorkArea,NULL);
		DestroyMenu(menu);
		switch(command)
		{
		case ID_DELETE:
			{
				if(selectedAction)
				{
					IActionParticle * action = selectedAction->GetActionParticle();
					if(action)
					{
						action->DestroyFilter(filter);
						ParticleView::InvalidateView();
					}
				}
			}
			break;
		case ID_EDIT:
			{
				filter->OpenWindow();
			}
			break;
		}
		return true;
	}
	return false;
}

bool filterHitTestRelease(IParticleFilter * filter, S32 xPos, S32 yPos)
{
	IParticleProgramer * prog = filter->GetProgramer();
	U32 numOutput = prog->GetNumOutput();
	U32 numInput = prog->GetNumInput();
	S32 height = BASE_FILTER_HEIGHT+(numInput*FILTER_LINE_HEIGHT)+(numOutput*FILTER_LINE_HEIGHT);
	if(xPos >= filter->GetWinX() && xPos <= filter->GetWinX()+FILTER_WIDTH && 
		yPos >= filter->GetWinY() && yPos <= filter->GetWinY()+height)
	{
		S32 testHeight = filter->GetWinY()+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT;

		for(U32 i = 0; i < numInput; ++i)
		{
			//test input click
			if(xPos >= filter->GetWinX()+FONT_BUFFER && xPos <= filter->GetWinX()+FONT_BUFFER+CIRC_WIDTH && 
				yPos >= testHeight+FONT_BUFFER && yPos <= testHeight+FONT_BUFFER+CIRC_WIDTH)
			{
				if(sourceConnect)
				{
					if(sourceConnect != filter)
					{
						sourceConnect->ConnectOutput(filter,connectionOutputID,i);
						filter->ConnectInput(sourceConnect,i,connectionOutputID);
					}
					ParticleView::InvalidateView();
				}
				return true;
			}
			testHeight += FILTER_LINE_HEIGHT;
		}

		for(U32 i = 0; i < numOutput; ++i)
		{
			if(xPos >= filter->GetWinX()+FILTER_WIDTH-(FONT_BUFFER+CIRC_WIDTH) && xPos <= filter->GetWinX()+FILTER_WIDTH-FONT_BUFFER && 
				yPos >= testHeight+FONT_BUFFER && yPos <= testHeight+FONT_BUFFER+CIRC_WIDTH)
			{
				if(destConnect)
				{
					if(destConnect != filter)
					{
						filter->ConnectOutput(destConnect,i,connectionInputID);
						destConnect->ConnectInput(filter,connectionInputID,i);
					}
					ParticleView::InvalidateView();
				}
				return true;
			}
			testHeight += FILTER_LINE_HEIGHT;
		}

		//ok just a normal click

		return false;
	}
	return false;
}

void updatePartScrollRange(HWND hWindow)
{
	partNeededArea.top = 0;
	partNeededArea.left = 0;
	partNeededArea.bottom = (U32)(PARTICLE_WORK_SIZE/partZoomLevel);
	partNeededArea.right = (U32)(PARTICLE_WORK_SIZE/partZoomLevel);
	GetClientRect(hWindow,&partViewArea);
	if(partNeededArea.right <= partViewArea.right)
	{
		partOffX = 0;
		SetScrollPos(hWindow,SB_HORZ,0,true);
		EnableScrollBar(hWindow,SB_HORZ,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(hWindow,SB_HORZ,ESB_ENABLE_BOTH);
		S32 width = partNeededArea.right-partViewArea.right;
		SetScrollRange(hWindow,SB_HORZ,0,width,true);
		if(partOffX > width)
		{
			partOffX = width;
			SetScrollPos(hWindow,SB_HORZ,partOffX,true);
		}
	}
	if(partNeededArea.bottom <= partViewArea.bottom)
	{
		partOffY = 0;
		SetScrollPos(hWindow,SB_VERT,0,true);
		EnableScrollBar(hWindow,SB_VERT,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(hWindow,SB_VERT,ESB_ENABLE_BOTH);
		S32 height = partNeededArea.bottom-partViewArea.bottom;
		SetScrollRange(hWindow,SB_VERT,0,height,true);
		if(partOffY > height)
		{
			partOffY = height;
			SetScrollPos(hWindow,SB_VERT,partOffY,true);
		}
	}
};

LONG CALLBACK particleWorkProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		{
			updatePartScrollRange(hWindow);
		}
		break;
	case WM_MOUSEWHEEL:
		{
			S16 value = -(S16)(HIWORD(wParam));
			SINGLE change = (value*partZoomSpeed);
			SINGLE newZoom = partZoomLevel + change;
			if(newZoom > maxPartZoom)
				newZoom  = maxPartZoom;
			else if (newZoom < minPartZoom)
				newZoom = minPartZoom;
			change = newZoom-partZoomLevel;
			partZoomLevel = newZoom;

			POINT point;
			point.x = (S32)(LOWORD(lParam));
			point.y = (S32)(HIWORD(lParam));
			RECT rect;
			GetClientRect(hWindow,&rect);
			ScreenToClient(hWindow,&point);
			SINGLE horzPercent = (1.0f-(((SINGLE)(rect.right-point.x))/((SINGLE)rect.right)));
			SINGLE vertPercent = (1.0f-(((SINGLE)(rect.bottom-point.y))/((SINGLE)rect.bottom)) );

			updatePartScrollRange(hWindow);

			partOffX = (S32)(partOffX + (rect.right*(-change)*horzPercent));
			if(partOffX < 0 || partNeededArea.right-partViewArea.right < 0)
				partOffX = 0;
			else if(partOffX > (partNeededArea.right-partViewArea.right))
				partOffX = (partNeededArea.right-partViewArea.right);
			SetScrollPos(hWindow,SB_HORZ,partOffX,true);

			partOffY = (S32)(partOffY + (rect.bottom*(-change)*vertPercent));
			if(partOffY < 0 || partNeededArea.bottom-partViewArea.bottom < 0)
				partOffY = 0;
			else if(partOffY > (partNeededArea.bottom-partViewArea.bottom))
				partOffY = (partNeededArea.bottom-partViewArea.bottom);
			SetScrollPos(hWindow,SB_VERT,partOffY,true);

			InvalidateRect(hWindow,NULL,false);

			return 1;
		}
		break;
	case WM_HSCROLL:
		{
			RECT viewArea;
			GetClientRect(hWindow,&viewArea);
			S32 oldXPos = partOffX;
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
					if(partOffX> 0)
					{
						partOffX = partOffX-1;
						SetScrollPos(hWindow,SB_HORZ,partOffX,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(partOffX < (PARTICLE_WORK_SIZE))
					{
						partOffX = partOffX+1;
						SetScrollPos(hWindow,SB_HORZ,partOffX,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					partOffX = partOffX-viewArea.right;
					if(partOffX < 0)
						partOffX = 0;
					SetScrollPos(hWindow,SB_HORZ,partOffX,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					partOffX = partOffX+viewArea.right;
					if(partOffX > (PARTICLE_WORK_SIZE))
						partOffX = (PARTICLE_WORK_SIZE);
					SetScrollPos(hWindow,SB_HORZ,partOffX,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					partOffX = HIWORD(wParam);
					SetScrollPos(hWindow,SB_HORZ,partOffX,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_VSCROLL:
		{
			RECT viewArea;
			GetClientRect(hWindow,&viewArea);
			S32 oldYPos = partOffY;
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
					if(partOffY> 0)
					{
						partOffY = partOffY-1;
						SetScrollPos(hWindow,SB_VERT,partOffY,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(partOffY < (PARTICLE_WORK_SIZE))
					{
						partOffY = partOffY+1;
						SetScrollPos(hWindow,SB_VERT,partOffY,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					partOffY = partOffY-viewArea.bottom;
					if(partOffY < 0)
						partOffY = 0;
					SetScrollPos(hWindow,SB_VERT,partOffY,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					partOffY = partOffY+viewArea.bottom;
					if(partOffY > (PARTICLE_WORK_SIZE))
						partOffY = (PARTICLE_WORK_SIZE);
					SetScrollPos(hWindow,SB_VERT,partOffY,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					partOffY = HIWORD(wParam);
					SetScrollPos(hWindow,SB_VERT,partOffY,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_PAINT:
		{
			updatePartScrollRange(hWindow);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWindow,&ps);

			if(bParticleViewInvalid)
			{
				bParticleViewInvalid = false;
				allocParticleDC(hdc);
				HBRUSH brush = (HBRUSH)(GetStockObject(DKGRAY_BRUSH));
				HFONT font = CreateFont(-MulDiv(FONT_POINT_SIZE, GetDeviceCaps(particleDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Arial");
				SelectObject(particleDC,font);
				RECT rect;
				rect.left = 0;
				rect.top = 0;
				rect.right = PARTICLE_WORK_SIZE;
				rect.bottom = PARTICLE_WORK_SIZE;
				FillRect(particleDC,&rect,brush);
				if(selectedAction)
				{
					IActionParticle * part = selectedAction->GetActionParticle();
					if(part)
					{
						LOGBRUSH logBrush;
						memset(&logBrush,0,sizeof(LOGBRUSH));
						logBrush.lbStyle = BS_SOLID;
						logBrush.lbColor = RGB(100,200,100);
						filterBkBrush = CreateBrushIndirect(&logBrush);

						logBrush.lbColor = RGB(25,100,25);
						filterShadowBrush= CreateBrushIndirect(&logBrush);

						logBrush.lbColor = RGB(200,255,200);
						filterHighlightBrush= CreateBrushIndirect(&logBrush);

						IParticleFilter * filter = part->GetFirstFilter();
						while(filter)
						{
							drawFilter(particleDC,filter);
							filter = filter->GetNextFilter();
						}

						filter = part->GetFirstFilter();
						while(filter)
						{
							drawFilterConnections(particleDC,filter);
							filter = filter->GetNextFilter();
						}

						if(bConnectingFilter)
						{
							if(sourceConnect)
							{
								SelectObject(particleDC,GetStockObject(BLACK_PEN));
								POINT pt1,pt2;
								GetCursorPos(&pt2);
								ScreenToClient(hWindow,&pt2);
								pt2.x = (S32)((pt2.x*partZoomLevel)+partOffX);
								pt2.y = (S32)((pt2.y*partZoomLevel)+partOffY);

								pt1.x = sourceConnect->GetWinX()+FILTER_WIDTH-(CIRC_WIDTH_HALF+FONT_BUFFER);
								pt1.y = sourceConnect->GetWinY()+CIRC_WIDTH_HALF+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT+
									(FILTER_LINE_HEIGHT*connectionOutputID)+(sourceConnect->GetProgramer()->GetNumInput()*FILTER_LINE_HEIGHT);
								MoveToEx(particleDC,pt1.x,pt1.y,NULL);
								LineTo(particleDC,pt2.x,pt2.y);
							}
							else if(destConnect)
							{
								SelectObject(particleDC,GetStockObject(BLACK_PEN));
								POINT pt1,pt2;
								GetCursorPos(&pt2);
								ScreenToClient(hWindow,&pt2);
								pt2.x = (S32)((pt2.x*partZoomLevel)+partOffX);
								pt2.y = (S32)((pt2.y*partZoomLevel)+partOffY);

								pt1.x = destConnect->GetWinX()+CIRC_WIDTH_HALF+FONT_BUFFER;
								pt1.y = destConnect->GetWinY()+CIRC_WIDTH_HALF+FONT_BUFFER+FILTER_LINE_HEIGHT+FILTER_LINE_HEIGHT;
								MoveToEx(particleDC,pt1.x,pt1.y,NULL);
								LineTo(particleDC,pt2.x,pt2.y);
							}
						}

						DeleteObject(filterHighlightBrush);
						DeleteObject(filterShadowBrush);
						DeleteObject(filterBkBrush);
					}
				}

				DeleteObject(font);
			}

			RECT clientRect;
			GetClientRect(hWindow,&clientRect);
			StretchBlt(hdc,0,0,clientRect.right,clientRect.bottom,particleDC,partOffX,partOffY,(int)(clientRect.right*partZoomLevel),(int)(clientRect.bottom*partZoomLevel),SRCCOPY);
			
			EndPaint(hWindow,&ps);
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		{
			if(selectedAction)
			{
				IActionParticle * part = selectedAction->GetActionParticle();
				if(part)
				{
					S32 newX = (S32)(LOWORD(lParam)*partZoomLevel);
					S32 newY = (S32)(HIWORD(lParam)*partZoomLevel);
					IParticleFilter * filter = part->GetFirstFilter();
					while(filter)
					{
						if(filterHitTestOpen(filter,newX+partOffX,newY+partOffY))
						{
							break;
						}
						filter = filter->GetNextFilter();
					}
				}
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			SetFocus(hWindow);
			if(selectedAction)
			{
				IActionParticle * part = selectedAction->GetActionParticle();
				if(part)
				{
					S32 newX = (S32)(LOWORD(lParam)*partZoomLevel);
					S32 newY = (S32)(HIWORD(lParam)*partZoomLevel);
					IParticleFilter * filter = part->GetFirstFilter();
					while(filter)
					{
						if(filterHitTest(filter,newX+partOffX,newY+partOffY))
						{
							SetCapture(hWindow);
							break;
						}
						filter = filter->GetNextFilter();
					}
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		{
			if(bMovingFilter)
			{
				bMovingFilter = false;
				ReleaseCapture();
			}
			if(bConnectingFilter)
			{
				bConnectingFilter = false;
				ReleaseCapture();
				if(selectedAction)
				{
					IActionParticle * part = selectedAction->GetActionParticle();
					if(part)
					{
						S32 newX = (S32)(LOWORD(lParam)*partZoomLevel);
						S32 newY = (S32)(HIWORD(lParam)*partZoomLevel);
						IParticleFilter * filter = part->GetFirstFilter();
						while(filter)
						{
							if(filterHitTestRelease(filter,newX+partOffX,newY+partOffY))
							{
								break;
							}
							filter = filter->GetNextFilter();
						}
					}
				}
				ParticleView::InvalidateView();
			}
		}
		break;
	case WM_RBUTTONDOWN:
		{
			SetFocus(hWindow);
			if(selectedAction)
			{
				IActionParticle * part = selectedAction->GetActionParticle();
				if(part)
				{
					S32 newX = (S32)(LOWORD(lParam)*partZoomLevel);
					S32 newY = (S32)(HIWORD(lParam)*partZoomLevel);
					IParticleFilter * filter = part->GetFirstFilter();
					while(filter)
					{
						if(filterHitTestRightClick(filter,newX+partOffX,newY+partOffY))
						{
							break;
						}
						filter = filter->GetNextFilter();
					}
				}
			}
		};
		break;
	case WM_MBUTTONDOWN:
		{
			SetFocus(hWindow);
			if(!bMovingFilter)
			{
				bPanningFilter = true;
				filterMouseX = (S32)(LOWORD(lParam)*partZoomLevel);
				filterMouseY = (S32)(HIWORD(lParam)*partZoomLevel);
				SetCapture(hWindow);
			}
		}
		break;
	case WM_MBUTTONUP:
		{
			if(bPanningFilter)
			{
				bPanningFilter = false;
				ReleaseCapture();
			}
		}
		break;
	case WM_MOUSEMOVE:
		{
			if(bMovingFilter)
			{
				if(targetMoveFilter)
				{
					S32 newX = (S32)((LOWORD(lParam)*partZoomLevel))+partOffX;
					S32 newY = (S32)((HIWORD(lParam)*partZoomLevel))+partOffY;
					if(newX != filterMouseX || newY != filterMouseY)
					{
						targetMoveFilter->SetWindowPos(targetMoveFilter->GetWinX()+(newX -filterMouseX),targetMoveFilter->GetWinY()+(newY -filterMouseY));
						filterMouseX = newX;
						filterMouseY = newY;
						ParticleView::InvalidateView();
					}
				}
			}
			else if(bConnectingFilter)
			{
				ParticleView::InvalidateView();
			}
			else if(bPanningFilter)
			{
				S32 newX = (S32)(LOWORD(lParam)*partZoomLevel);
				S32 newY = (S32)(HIWORD(lParam)*partZoomLevel);
				if(newX != filterMouseX || newY != filterMouseY)
				{
					partOffX -= newX-filterMouseX;
					if(partOffX < 0)
						partOffX = 0;
					if(partOffX > (partNeededArea.right-partViewArea.right))
						partOffX = (partNeededArea.right-partViewArea.right);
					SetScrollPos(hWindow,SB_HORZ,partOffX,true);

					partOffY -= newY-filterMouseY;
					if(partOffY < 0)
						partOffY = 0;
					if(partOffY > (partNeededArea.bottom-partViewArea.bottom))
						partOffY = (partNeededArea.bottom-partViewArea.bottom);
					SetScrollPos(hWindow,SB_VERT,partOffY,true);
					filterMouseX = newX;
					filterMouseY = newY;
					InvalidateRect(hWindow,NULL,false);
				}
			}
		}
		break;
	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}

BOOL CALLBACK partHeaderProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				HWND name = GetDlgItem(hWindow,IDC_NAME_EDIT);
				SetWindowText(name,selectedAction->GetName());
				IActionParticle * part = selectedAction->GetActionParticle();
				if(part)
				{
					CheckDlgButton(hWindow,IDC_DISABLE,part->IsDisabled() ? BST_CHECKED: BST_UNCHECKED);	
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_NAME_EDIT:
						{
							if(selectedAction)
							{
								HWND name = GetDlgItem(hWindow,IDC_NAME_EDIT);
								char buffer[256];
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
							}
						}
						break;
					}
				}
				break;
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DISABLE:
						{
							if(IsDlgButtonChecked(hWindow,IDC_DISABLE) == BST_CHECKED)
							{
								if(selectedAction)
								{
									IActionParticle * particle = selectedAction->GetActionParticle();
									if(particle)
										particle->SetDisabled(true);
								}
							}
							else
							{
								if(selectedAction)
								{
									IActionParticle * particle = selectedAction->GetActionParticle();
									if(particle)
										particle->SetDisabled(false);
								}
							}
						}
						break;
					case IDC_EXPORT:
						{
							char buffer[255];
							buffer[0] = 0;
							OPENFILENAME fileName;
							memset(&fileName,0,sizeof(OPENFILENAME));
							fileName.lStructSize = sizeof(OPENFILENAME);
							fileName.lpstrFilter = "Particle Export\0*.pfx\0\0";
							fileName.nFilterIndex = 1;
							fileName.lpstrFile = buffer;
							fileName.nMaxFile = 255;
							fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
							if(GetSaveFileName(&fileName))
							{
								char testBuff[256];
								strcpy(testBuff,buffer);
								U32 filelen = strlen(testBuff);
								_strupr(testBuff);
								bool bAppend = false;
								if(filelen < 4)
									bAppend = true;
								else if(strcmp(&(testBuff[filelen-4]),".PFX") != 0)
									bAppend = true;
								if(bAppend)
								{
									if(filelen+4 > 255)
									{
										strcpy(&(buffer[251]),".PFX");
									}
									else
									{
										strcat(buffer,".PFX");
									}
								}
								//save code here

								COMPTR<IFileSystem> outFile;
								DAFILEDESC fdesc = buffer;
								fdesc.lpImplementation = "UTF";
								fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
								fdesc.dwCreationDistribution = CREATE_ALWAYS;
								if (DACOM->CreateInstance(&fdesc, outFile) == GR_OK)
								{
									if(selectedAction)
									{
										selectedAction->SaveCore(outFile);
										InvalidateRect(mainWindow,NULL,false);
									}
								}
							}
						}
						break;
					case IDC_IMPORT:
						{
							char buffer[255];
							buffer[0] = 0;
							OPENFILENAME fileName;
							memset(&fileName,0,sizeof(OPENFILENAME));
							fileName.lStructSize = sizeof(OPENFILENAME);
							fileName.lpstrFilter = "Particle Import\0*.pfx\0\0";
							fileName.nFilterIndex = 1;
							fileName.lpstrFile = buffer;
							fileName.nMaxFile = 255;
							fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;
							if(GetOpenFileName(&fileName))
							{
								DAFILEDESC fdesc = buffer;
								COMPTR<IFileSystem> inFile;

								fdesc.lpImplementation = "UTF";
								if (DACOM->CreateInstance(&fdesc, inFile) == GR_OK)
								{	
									if(selectedAction)
									{
										IActionParticle * particle = selectedAction->GetActionParticle();
										if(particle)
											particle->ImportLoadCore(inFile);
									}
								}
							}
						}
						break;
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_ADD_EFFECT:
						{
							if(selectedAction)
							{
								if(DialogBox(hMainInst,MAKEINTRESOURCE(IDD_NEW_PARTICLE_EFFECT), mainWindow, newParticleEffectProc))
								{
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
}

BOOL CALLBACK newParticleEffectProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND combo = GetDlgItem(hWindow, IDC_EFFECT_TYPE_COMBO);

			U32 numFilters = PARTMAN->GetFilterNumber();
			bool bSet = false;
			for(U32 count = 0; count < numFilters; ++ count)
			{
				ParticleEffectType pType = PARTMAN->GetFilterEnum(count);
				U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(PARTMAN->GetFilterName(pType)));
				SendMessage(combo,CB_SETITEMDATA,index,pType);
				if(!bSet)
				{
					bSet = true;
					SendMessage(combo,CB_SETCURSEL,index,0);
				}
			}
			SetFocus(combo);
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDOK:
						{
							if(selectedAction)
							{
								IActionParticle * part = selectedAction->GetActionParticle();
								if(part)
								{
									HWND combo = GetDlgItem(hWindow, IDC_EFFECT_TYPE_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										U32 val = SendMessage(combo,CB_GETITEMDATA,index,0);
										IParticleFilter * filter = part->CreateNewFilter((ParticleEffectType)val);
										if(filter)
										{
//											filter->InitWindow();
											ParticleView::InvalidateView();
										}
									}
								}
							}
							EndDialog(hWindow,1);
						}
						break;
					case IDCANCEL:
						{
							EndDialog(hWindow,0);
						}
						break;
					}
					break;
				}
			}
		}
		break;
	}
	return false;
};
