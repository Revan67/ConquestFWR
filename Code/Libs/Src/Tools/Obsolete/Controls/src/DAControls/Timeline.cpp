// Timeline.cpp : Implementation of CTimeline
#include "stdafx.h"
#include "DAControls.h"
#include "Timeline.h"
#include "TrackObject.h"

//
// Constants
//

const int MARKER_RAD_PERCENT = 80;
const int CURSOR_GRAB_RAD = 5;
const int TIP_MARGIN = 2;
const int TIP_Y_OFFSET = 16;
const int LEVEL_INDENT = 16;
const COLORREF TIP_COLOR = RGB(255,255,128);
const COLORREF START_COLOR = RGB(0,255,0);
const COLORREF STOP_COLOR = RGB(255,0,0);
const COLORREF CURSOR_COLOR = RGB(0,0,255);

/////////////////////////////////////////////////////////////////////////////
// CTimeline

CTimeline::CTimeline()
{
	cursorPos = 0.0;
	vScale = 1.0;
	hScale = 1.0;  // this is not going to be used.
	length = 100.0;  // number of seconds of information to display, i.e. secs/view
	vScroll = 0.0;
	hScroll = 0.0;
	startTime = 0.0;
	stopTime = 20.0;
	uiLock = FALSE;
	hFont = NULL;
	trackHeight = 1;  // this is a cache of the last drawn track height.
	trackWidth = 1;   // this is a cache of the last drawn track view width;
	trackOrigin = 1;  // this is a cache of the last drawn track view origin.

	dragOriginX = 0;
	dragMarkerId = 0;
	dragOffsetX = 0.0;
	dragMode = DRAG_NONE;

	mouseLoc.x = 0;
	mouseLoc.y = 0;
	mouseFlags = 0;

	tipTimeDraw = FALSE;
	tipTime = 0.0;
	tipX = tipY = 0;

	useScale = FALSE;

	trackCount = 0;
	trackList = NULL;

	drawCount = 0;
	
	// Initialize the tracks and markers.
	trackHead.next = NULL;
	trackHead.id = 0;
	trackHead.level = 0;
	trackHead.childId = 0;
	markerHead.next = NULL;
	markerHead.id = 0;
	markerHead.trackId = 0;

	// Create the GDI resources we will use for drawing.
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = CURSOR_COLOR;
	lb.lbHatch = 0;
	hCursorPen = ExtCreatePen(PS_COSMETIC | PS_DASH, 1, &lb, 0, NULL);

	lb.lbStyle = BS_SOLID;
	lb.lbColor = START_COLOR;
	lb.lbHatch = 0;
	hStartPen = ExtCreatePen(PS_COSMETIC | PS_SOLID, 1, &lb, 0, NULL);

	lb.lbStyle = BS_SOLID;
	lb.lbColor = STOP_COLOR;
	lb.lbHatch = 0;
	hStopPen = ExtCreatePen(PS_COSMETIC | PS_SOLID, 1, &lb, 0, NULL);

	hTipBrush = CreateSolidBrush (TIP_COLOR);

	hClosedBmp = (HBITMAP) LoadImage (_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_CLOSED), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	hOpenBmp = (HBITMAP) LoadImage (_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_OPEN), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

	if (hClosedBmp)
	{
		BITMAP bm;
		GetObject (hClosedBmp, sizeof(BITMAP), &bm);
		closedSize.cx = bm.bmWidth;
		closedSize.cy = bm.bmHeight;
	}

	if (hOpenBmp)
	{
		BITMAP bm;
		GetObject (hOpenBmp, sizeof(BITMAP), &bm);
		openSize.cx = bm.bmWidth;
		openSize.cy = bm.bmHeight;
	}

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);
    hFont = CreateFontIndirect(&info.lfMenuFont);

	// Indicate that this should be windowed only.

	m_bWindowOnly = true;
}

CTimeline::~CTimeline()
{
	// Destroy any allocated resources
	if (hCursorPen != NULL)
	{
		DeleteObject(hCursorPen);
	}
	if (hStartPen != NULL)
	{
		DeleteObject(hStartPen);
	}
	if (hStopPen != NULL)
	{
		DeleteObject(hStopPen);
	}
	if (hFont != NULL)
	{
		DeleteObject (hFont);
	}
	if (hTipBrush != NULL)
	{
		DeleteObject (hTipBrush);
	}
	if (hOpenBmp)
	{
		DeleteObject (hOpenBmp);
	}
	if (hClosedBmp)
	{
		DeleteObject (hClosedBmp);
	}

	{
		Track *here = trackHead.next;
		while (here)
		{
			Track *next = here->next;
			delete here;
			here = next;
		}
	}

	{
		Marker *here = markerHead.next;
		while (here)
		{
			Marker *next = here->next;
			delete here;
			here = next;
		}
	}

	if (trackList != NULL)
	{
		free(trackList);
		trackList = NULL;
	}
}

int CTimeline::timeToPixel(float time)
{
	// Returns the pixel offset of a given time, relative to the scroll position, the displayed length, and
	// the pixel width of the window.

	float t = time - hScroll;

	// t now is time relative to scroll position.
	// scroll position is alway the origin of scaling, so apply scale here.
	// The formula is:
	//    pixels = secs * (pixels/view) / (secs/view) = t * viewWidth / length;
	// t is in pixels. Truncate it to an integer.

	// WARNING: Is is assumed that length is never 0 or negative here, because we control how it is
	// set in the interface.
	t = t * trackWidth / length;
	return (int) t;
}

int CTimeline::trackToPixel(int trackIndex)
{
	// Returns the vertical pixel offset of an indexed track, taking the vertical scroll position
	// into account.
	// NOTE: vScroll is in pixels.

	int y = trackIndex * trackHeight - (int) vScroll;
	return y;
}

float CTimeline::pixelToTime (int pixel)
{
	// Inverse of timeToPixel;
	if (trackWidth < 1)
	{
		trackWidth = 1;
	}

	float time = pixel * length / trackWidth + hScroll;
	return time;
}

int CTimeline::pixelToTrack(int y)
{
	// Inverse of trackToPixel;
	if (trackHeight < 1)
	{
		trackHeight = 1;
	}

	int trackIndex = (y + (int) vScroll) / trackHeight;
	return trackIndex;
}

int CTimeline::getTrackIndex(int trackId)
{
	// NOTE: This is an index into the visible track list, not
	// the entire track list.
	if (trackId != 0 && trackList != NULL)
	{
		Track **here = trackList;
		int index = 0;
		while (*here)
		{
			if ((*here)->id == trackId)
			{
				return index;
			}
			++index;
			++here;
		}
	}

	return -1;
}

int CTimeline::getTrackId(int trackIndex)
{
	// NOTE: This is an index into the visible track list, not
	// the entire track list.
	if (trackList == NULL)
	{
		return 0;
	}

	Track **here = trackList;
	while (*here && trackIndex)
	{
		--trackIndex;
		++here;
	}

	if (*here == NULL)
	{
		return 0;
	}
	else
	{
		return (*here)->id;
	}
}

Marker *CTimeline::getMarker(int markerId)
{
	if (markerId == 0)
	{
		return NULL;
	}

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			return here;
		}
		here = here->next;
	}

	return NULL;
}

Track *CTimeline::getTrack (int trackId)
{
	if (trackId == 0)
	{
		return NULL;
	}

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			return here;
		}
		here = here->next;
	}

	return NULL;
}

int CTimeline::getMarkerId(int trackId, float minTime, float maxTime)
{
	// Find the marker on the given track between the two time periods which has the largest
	// start position.

	Marker *found = NULL;
	Marker *here = markerHead.next;
	while (here)
	{
		if (here->trackId == trackId)
		{
			if (minTime <= here->pos + here->duration && here->pos <= maxTime)
			{
				if (!found || here->pos >= found->pos)
				{
					found = here;
				}
			}
		}
		here = here->next;
	}

	if (found == NULL)
	{
		return 0;
	}
	else
	{
		return found->id;
	}
}

int CTimeline::buildTrackList (Track *head, Track **list)
{
	// Walk the given list of peers, inserting them into the list, and their children
	// if expanded.

	Track *t = head;
	int count = 0;
	while (t)
	{
		// Add this peer to the list.
		list[count] = t;
		++count;

		// If this peer has children and is expanded, add them.
		if (t->childId != 0 && t->expanded)
		{
			count += buildTrackList (getTrack(t->childId), &list[count]);
		}

		// Go to the next peer
		t = getTrack (t->peerId);
	}
	return count;
}

void CTimeline::onTrackChange ()
{
	// Call this each time a track is created, destroyed, expanded, or collapsed, so that
	// the track information can be recalculated.

	// Count the number of tracks, then reallocate the tracklist.
	trackCount = 0;
	Track *here = trackHead.next;
	while (here)
	{
		++trackCount;
		here = here->next;
	}
	trackList = (Track **) realloc(trackList, sizeof(Track *) * (trackCount+1));

	// Build the visible track list. NULL terminate it.
	int visCount = buildTrackList (trackHead.next, trackList);
	trackList[visCount] = NULL;

	// Cause the control to be redrawn.
	FireViewChange();
}

void CTimeline::drawTracks(HDC hDc, RECT lr, RECT tr)
{
	// This allows us to use OLE2CT below to covert the BSTR to the proper format for DrawText().
	USES_CONVERSION;

	// Draw each of the labels and outline each of the tracks

	RECT rText, rTrack;
	rText.left = lr.left;
	rText.right = lr.right;
	rText.top = lr.top;
	rText.bottom = lr.top + trackHeight;
	rTrack.left = tr.left;
	rTrack.right = tr.right;
	rTrack.top = tr.top;
	rTrack.bottom = tr.top + trackHeight;

	if (trackList != NULL)
	{
		// Allocate the bitmap DCs
		HDC openDC = CreateCompatibleDC (hDc);
		HBITMAP oldOpenBmp = (HBITMAP) SelectObject (openDC, hOpenBmp);
		HDC closedDC = CreateCompatibleDC (hDc);
		HBITMAP oldClosedBmp = (HBITMAP) SelectObject (closedDC, hClosedBmp);

		Track **here = trackList;
		while (*here)
		{
			// If this track has a child, draw a bitmap to indicate it's open or closed state.
			RECT br = rText;
			br.left += ((*here)->level-1) * LEVEL_INDENT;
			if ((*here)->childId != 0)
			{
				HDC hBmpDC;
				SIZE size;
				if ((*here)->expanded)
				{
					hBmpDC = openDC;
					size = openSize;
				}
				else
				{
					hBmpDC = closedDC;
					size = closedSize;
				}
				BitBlt
				(
					hDc,
					br.left + LEVEL_INDENT/2 - size.cx/2,
					(br.top + br.bottom)/2 - size.cy/2,
					size.cx, size.cy,
					hBmpDC,
					0, 0,
					SRCCOPY
				);
			}

			// Adjust the text rectangle's left side according to the track's level value.
			RECT atr = rText;
			atr.left += (*here)->level * LEVEL_INDENT;
			// Draw the label and the bottom and top edges of the track
			DrawText(hDc, OLE2CT((BSTR) (*here)->name), -1, &atr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			DrawEdge(hDc, &rTrack, EDGE_ETCHED, BF_BOTTOM | BF_MIDDLE /*| BF_TOP*/);

			// Increment the values
			rText.top += trackHeight;
			rText.bottom += trackHeight;
			rTrack.top += trackHeight;
			rTrack.bottom += trackHeight;
			++here;
		}

		// Free the bitmap DCs
		SelectObject (openDC, oldOpenBmp);
		SelectObject (closedDC, oldClosedBmp);
		DeleteDC (openDC);
		DeleteDC (closedDC);
	}
}

void CTimeline::drawSingleMarker (HDC hDc, RECT tr, int mRad, Marker *here)
{
	// WARNING: This assumes that the clipping region has been set appropriately.

	if (here->drawCount != drawCount)
	{
		// Indicate that this marker has been processed this frame.
		here->drawCount = drawCount;
		
		// Draw the marker if its track is visible.
		int ti = getTrackIndex (here->trackId);
		if (ti != -1)
		{
			int y = trackToPixel (ti) + trackHeight/2 + tr.top;
			int x = timeToPixel (here->pos) + tr.left;

			// x & y are now on the center of the marker.
			// Draw a circle.
			Ellipse (hDc, x - mRad, y - mRad, x + mRad, y + mRad);

			// A vertical line in the circle at the exact time point.
			HPEN oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
			MoveToEx (hDc, x, y - mRad + 1, NULL);
			LineTo (hDc, x, y + mRad - 1);
			SelectObject (hDc, oldPen);

			// If the duration of the marker is > 0.0, draw an end marker and
			// a line from the center of the first marker to the other.

			if (here->duration > 0.0)
			{
				int x1 = timeToPixel (here->pos + here->duration) + tr.left;

				// x & y are now on the center of the marker.
				// Draw a circle.
				Ellipse (hDc, x1 - mRad, y - mRad, x1 + mRad, y + mRad);

				// A vertical line in the circle at the exact time point.
				oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
				MoveToEx (hDc, x1, y - mRad + 1, NULL);
				LineTo (hDc, x1, y + mRad - 1);

				// The line between the markers
				MoveToEx (hDc, x, y, NULL);
				LineTo (hDc, x1, y);
				SelectObject (hDc, oldPen);
			}

			// If this marker has a linked marker, draw it, then draw a line to it from this
			// marker.

			if (here->linkId != 0)
			{
				Marker *m = getMarker (here->linkId);
				if (m != NULL)
				{
					drawSingleMarker (hDc, tr, mRad, m);

					int x1 = timeToPixel (m->pos) + tr.left;

					// The line between the markers
					oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
					MoveToEx (hDc, x, y, NULL);
					LineTo (hDc, x1, y);
					SelectObject (hDc, oldPen);
				}
			}
		}
	}
}

void CTimeline::drawMarkers(HDC hDc, RECT tr)
{
	// Save a copy of the current clip region, then intersect the track rectangle with the
	// current clipping region.

	HRGN oldRgn = CreateRectRgn(tr.left, tr.top, tr.right, tr.bottom);
	if (GetClipRgn (hDc, oldRgn) != 1)
	{
		// There is no clip region, or an error occurred.  Either way, delete the old region
		DeleteObject (oldRgn);
		oldRgn = NULL;		
	}
	IntersectClipRect (hDc, tr.left, tr.top, tr.right, tr.bottom);

	// Calculate the marker radius

	int mRad = (trackHeight * MARKER_RAD_PERCENT) / 200;

	// Run through the markers, drawing them in the order of their creation.
	// *** TODO: Draw the markers for a given track from earliest to latest.
	Marker *here = markerHead.next;
	while (here)
	{
#if 0
		if (here->drawCount != drawCount)
		{
			int ti = getTrackIndex (here->trackId);
			if (ti != -1)
			{
				int y = trackToPixel (ti) + trackHeight/2 + tr.top;
				int x = timeToPixel (here->pos) + tr.left;

				// x & y are now on the center of the marker.
				// Draw a circle.
				Ellipse (hDc, x - mRad, y - mRad, x + mRad, y + mRad);

				// A vertical line in the circle at the exact time point.
				HPEN oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
				MoveToEx (hDc, x, y - mRad + 1, NULL);
				LineTo (hDc, x, y + mRad - 1);
				SelectObject (hDc, oldPen);

				// If the duration of the marker is > 0.0, draw an end marker and
				// a line from the center of the first marker to the other.

				if (here->duration > 0.0)
				{
					int x1 = timeToPixel (here->pos + here->duration) + tr.left;

					// x & y are now on the center of the marker.
					// Draw a circle.
					Ellipse (hDc, x1 - mRad, y - mRad, x1 + mRad, y + mRad);

					// A vertical line in the circle at the exact time point.
					oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
					MoveToEx (hDc, x1, y - mRad + 1, NULL);
					LineTo (hDc, x1, y + mRad - 1);

					// The line between the markers
					MoveToEx (hDc, x, y, NULL);
					LineTo (hDc, x1, y);
					SelectObject (hDc, oldPen);
				}
			}
			here->drawCount = drawCount;
		}
#else
		drawSingleMarker (hDc, tr, mRad, here);
#endif
		here = here->next;
	}

	// Restore the clip region, or lack thereof.
	SelectClipRgn (hDc, oldRgn);
	if (oldRgn != NULL)
	{
		DeleteObject (oldRgn);
	}
}

void CTimeline::drawScale(HDC hDc, RECT tr)
{
	// Draw the scale in factors of 2, choosing the biggest step that results in less than
	// a percentage of the displayed area.

	const int WIDTH_PERCENT = 10;
	const float STEP_FACTOR = 4.0;

	int p0 = timeToPixel(0.0);
	int maxWidth = ((tr.right - tr.left) * WIDTH_PERCENT) / 100;

	float step;
	for (step = 0.01f; step < 10000.0f; step *= STEP_FACTOR)
	{
		if (timeToPixel(step) - p0 > maxWidth)
		{
			// We are done. Break out of the loop.
			break;
		}
	}

	// Reduce the step by the factor, since the loop above will stop 1 factor past where we want.
	step /= STEP_FACTOR;

	if (step > 0.0)
	{
		RECT r;
		r.top = tr.top;
		r.bottom = tr.bottom;

		for (float f = 0.0; f <= length; f += step)
		{
			r.left = timeToPixel(f) + trackOrigin;
			r.right = r.left+1;
			DrawEdge(hDc, &r, EDGE_ETCHED, BF_LEFT | BF_RIGHT);
		}

		// Draw a divider for the scale text area.
		DrawEdge(hDc, &tr, EDGE_RAISED, BF_TOP);

		// Draw the scale text.
		// *** TODO: Write this code.
	}
}

HRESULT CTimeline::OnDraw(ATL_DRAWINFO& di)
{
	RECT& rc = *(RECT*)di.prcBounds;

	HDC hDc = di.hdcDraw;
	HDC memDc = NULL;
	HBITMAP memBits = NULL;
	HBITMAP oldBitmap = NULL;

	// Create a memory DC instead of drawing directly.
	memDc = CreateCompatibleDC (di.hdcDraw);
	if (memDc != NULL)
	{
		memBits = CreateCompatibleBitmap (di.hdcDraw, rc.right - rc.left, rc.bottom - rc.top);
		if (memBits == NULL)
		{
			DeleteDC (memDc);
			memDc = NULL;
		}
		else
		{
			oldBitmap = (HBITMAP) SelectObject (memDc, memBits);
			hDc = memDc;
		}
	}

	// Save the current font for restoration.
	HFONT oldFont = NULL;
	if (hFont != NULL)
	{
		oldFont = (HFONT) SelectObject (hDc, hFont);
	}

	// This is done phases
	// 1) Clear the text and track parts, then draw the divider and scales
	// 2) Draw each track
	// 3) Draw the current cursors.

	// Part 1 - Clear the label and track areas, then draw the divider and scales
	// Fill with the standard window background color
	RECT lr = rc;  // label area rect
	lr.right = lr.left + (int) ((lr.right - lr.left) * 0.20);
	RECT tr = rc;  // track area rect
	tr.left = lr.right;

	// Clear the label area
	{
		RECT fr = lr;
		fr.right -= GetSystemMetrics(SM_CYDLGFRAME);
		FillRect(hDc, &fr, (HBRUSH) (COLOR_WINDOW+1));
	}

	// Clear the track area by filling with the standard window background color
	FillRect(hDc, &tr, (HBRUSH) (COLOR_SCROLLBAR+1));

	// Draw the divider
	{
		RECT er = lr;
		er.left = er.right - GetSystemMetrics(SM_CXDLGFRAME);
		DrawEdge(hDc, &er, EDGE_RAISED, BF_TOPLEFT | BF_BOTTOMRIGHT | BF_MIDDLE);
	}

	// Calculate the height of the tracks by baseing them on the height of the font.
	TEXTMETRIC tm;
	GetTextMetrics(hDc, &tm);
	trackHeight = tm.tmHeight + GetSystemMetrics(SM_CYDLGFRAME) * 2;
	trackWidth = tr.right - tr.left;
	trackOrigin = tr.left;

	// Adjust the track rectangle to make room for the scale.
	if (useScale)
	{
		tr.bottom -= trackHeight;
	}

	// Part 2 - Draw the tracks, scale, and markers

	drawTracks(hDc, lr, tr);
	if (useScale)
	{
		RECT sr;
		sr.top = tr.bottom;
		sr.bottom = sr.top + trackHeight;
		sr.left = tr.left;
		sr.right = tr.right;
		drawScale(hDc, sr);
	}
	drawMarkers(hDc, tr);

	// Part 3 - Draw the cursors.
	// The scale and scrolling values are currently ignored, but the length of the timeline is assumed to be
	// mapped to the entire control, whatever its size.
	// The formula for calculating the cursor pixel position is:
	//   cp = trackBarStart + trackBarWidth * cursorPos / length;
	// To take scale and scroll into account, the value becomes:
	//   cp = trackBarStart + trackBarWidth * (cursorPos - hScroll) * scale / length;
	// if the resulting cp < trackBarStart, or creater than trackBarEnd, the cursor does not draw.

	int sp = timeToPixel(startTime);
	int ep = timeToPixel(stopTime);
	int cp = timeToPixel(cursorPos);
	if (sp >= 0 && sp < trackWidth)
	{
		HPEN oldPen = (HPEN) SelectObject (hDc, hStartPen);
		sp += tr.left;
		MoveToEx (hDc, sp, tr.top, NULL);
		LineTo (hDc, sp, tr.bottom);
		SelectObject (hDc, oldPen);
	}
	if (ep >= 0 && ep < trackWidth)
	{
		HPEN oldPen = (HPEN) SelectObject (hDc, hStopPen);
		ep += tr.left;
		MoveToEx (hDc, ep, tr.top, NULL);
		LineTo (hDc, ep, tr.bottom);
		SelectObject (hDc, oldPen);
	}
	if (cp >= 0 && cp < trackWidth)
	{
		HPEN oldPen = (HPEN) SelectObject (hDc, hCursorPen);
		cp += tr.left;
		MoveToEx (hDc, cp, tr.top, NULL);
		LineTo (hDc, cp, tr.bottom);
		SelectObject (hDc, oldPen);
	}

	// TEMP: Label this control
#if 0
	char buffer[1024];
	wsprintf (buffer, "MousePos (%d, %d), Flags %x", mouseLoc.x, mouseLoc.y, mouseFlags);
	DrawText(hDc, buffer, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
#endif

	// If a tip draw was requested, draw it.

	if (tipTimeDraw)
	{
		char buffer[1024];
		wsprintf(buffer, "%d.%02d", (int) tipTime, (int) ((tipTime - ((int) tipTime)) * 100));
		SIZE size;
		if (GetTextExtentPoint32 (hDc, buffer, lstrlen(buffer), &size) != 0)
		{
			// Draw the text opaque, then outline the rectangle.
			HPEN oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
			HBRUSH oldBrush = (HBRUSH) SelectObject (hDc, hTipBrush);
			COLORREF oldBkColor = GetBkColor(hDc);
			SetBkColor (hDc, TIP_COLOR);
			Rectangle(hDc, tipX-TIP_MARGIN, tipY-TIP_MARGIN, tipX + size.cx + TIP_MARGIN, tipY + size.cy + TIP_MARGIN);
			TextOut (hDc, tipX, tipY, buffer, lstrlen(buffer));
			SelectObject (hDc, oldPen);
			SelectObject (hDc, oldBrush);
			SetBkColor (hDc, oldBkColor);
		}
	}

	// If we selected a font, restore the original.
	if (hFont != NULL)
	{
		SelectObject (hDc, oldFont);
	}

	// If we are using a memory DC, bit it to the main DC, then destroy it.
	if (memDc != NULL)
	{
		BitBlt (di.hdcDraw, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDc, 0, 0, SRCCOPY);
		SelectObject (memDc, oldBitmap);
		DeleteObject (memBits);
		DeleteDC (memDc);
	}

	// Increment the draw count. It is ok to roll over.
	++drawCount;

	return S_OK;
}

STDMETHODIMP CTimeline::get_CursorPos(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = cursorPos;
	return S_OK;
}

STDMETHODIMP CTimeline::put_CursorPos(float newVal)
{
	// TODO: Add your implementation code here
	cursorPos = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_VScale(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = vScale;
	return S_OK;
}

STDMETHODIMP CTimeline::put_VScale(float newVal)
{
	// TODO: Add your implementation code here
	vScale = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_HScale(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = hScale;
	return S_OK;
}

STDMETHODIMP CTimeline::put_HScale(float newVal)
{
	// TODO: Add your implementation code here
	hScale = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_Length(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = length;
	return S_OK;
}

STDMETHODIMP CTimeline::put_Length(float newVal)
{
	// TODO: Add your implementation code here
	if (newVal > 0.0)
	{
		length = newVal;
		FireViewChange();
	}
	return S_OK;
}

STDMETHODIMP CTimeline::get_VScroll(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = vScroll;
	return S_OK;
}

STDMETHODIMP CTimeline::put_VScroll(float newVal)
{
	// TODO: Add your implementation code here
	vScroll = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_HScroll(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = hScroll;
	return S_OK;
}

STDMETHODIMP CTimeline::put_HScroll(float newVal)
{
	// TODO: Add your implementation code here
	hScroll = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_UILock(BOOL * pVal)
{
	// TODO: Add your implementation code here
	*pVal = uiLock;
	return S_OK;
}

STDMETHODIMP CTimeline::put_UILock(BOOL newVal)
{
	// TODO: Add your implementation code here
	uiLock = newVal;
	FireViewChange();
	return S_OK;
}

LRESULT CTimeline::OnMouse(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Set bHandled to TRUE if the message is processed.
	// Return proper result for the handled message.

	DWORD fwKeys = wParam;        // key flags 
	WORD xPos = LOWORD(lParam);  // horizontal position of cursor 
	WORD yPos = HIWORD(lParam);  // vertical position of cursor

	if (uiLock)
	{
		bHandled = FALSE;
	}
	else
	{
		mouseLoc.x = xPos;
		mouseLoc.y = yPos;
		mouseFlags = fwKeys;
		bHandled = TRUE;

		switch (nMsg)
		{
		case WM_LBUTTONDOWN:
			{
				// If we are in the label area, this is a potential track expansion.
				// Otherwise, check for marker or cursor dragging.
				// *** TODO: Add support for dragging the divider bar between the label and
				// *** track area. 
				if (xPos < trackOrigin)
				{
					int trackIndex = pixelToTrack(yPos);
					int trackId = getTrackId(trackIndex);
					if (trackId != 0)
					{
						// Track is valid. Toggle its expansion state.
						// Any value but 0 or 1 will toggle the expansion state.
						ExpandTrack (trackId, 2);
					}
				}
				else
				{
					// Are we close enough to drag the cursor?  If so, it gets precidence.

					float minCTime = pixelToTime(xPos - trackOrigin - CURSOR_GRAB_RAD);
					float maxCTime = pixelToTime(xPos - trackOrigin + CURSOR_GRAB_RAD);
					if (minCTime <= cursorPos && cursorPos <= maxCTime)
					{
						// Cursor drag.
						dragMode = DRAG_CURSOR;
						dragMarkerId = 0;
						dragOriginX = xPos;
						dragOffsetX = 0.0;
					}
					else
					{
						// Not dragging the cursor, so check for marker drag.
						// See if we are in a valid track.  If not, do nothing.
						int trackIndex = pixelToTrack(yPos);
						int trackId = getTrackId(trackIndex);
						if (trackId != 0)
						{
							// Track is valid. Check for markers

							int mRad = (trackHeight * MARKER_RAD_PERCENT) / 200;
							float minTime = pixelToTime(xPos - trackOrigin - mRad);
							float maxTime = pixelToTime(xPos - trackOrigin + mRad);
							int markerId = getMarkerId (trackId, minTime, maxTime);

							if (markerId != 0)
							{
								// We are on a marker, so save off the xPos and marker id for dragging
								// purposes.
								dragMode = DRAG_MARKER;
								dragMarkerId = markerId;
								dragOriginX = xPos;

								Marker *m = getMarker (markerId);
								dragOffsetX = pixelToTime(xPos - trackOrigin) - m->pos;
							}
						}
					}
				}
			}
			break;

		case WM_LBUTTONUP:
			// Turn off dragging.
			dragMode = DRAG_NONE;
			break;

		case WM_RBUTTONDOWN:
			// Do nothing.
			break;

		case WM_RBUTTONUP:
			// Potential context message request.
			// See if we are in a valid track.  If not, do nothing.
			{
				int trackIndex = pixelToTrack(yPos);
				int trackId = getTrackId(trackIndex);
				if (trackId != 0)
				{
					// Track is valid. Check for markers

					int mRad = (trackHeight * MARKER_RAD_PERCENT) / 200;
					float minTime = pixelToTime(xPos - trackOrigin - mRad);
					float maxTime = pixelToTime(xPos - trackOrigin + mRad);
					int markerId = getMarkerId (trackId, minTime, maxTime);

					if (markerId == 0)
					{
						// Not a marker, so fire one of the two track requests.
						// If in the label region, fire the label context request.
						if (xPos < trackOrigin)
						{
							Fire_TrackLabelContextRequest (trackId, xPos, yPos);
						}
						else
						{
							Fire_TrackContextRequest (trackId, pixelToTime(xPos-trackOrigin), xPos, yPos);
						}
					}
					else
					{
						// Fire a marker request
						Fire_MarkerContextRequest (markerId, xPos, yPos);
					}
				}
				else
				{
					// By convention, a click outside of a valid track yields a track label request on id
					// 0.

					Fire_TrackLabelContextRequest(0, xPos, yPos);
				}
				
				// Turn off the time tip.
				tipTimeDraw = FALSE;
				FireViewChange();
			}
			break;

		case WM_MOUSEMOVE:
			if (fwKeys & MK_LBUTTON)
			{
				// Potentially dragging. Check the drag mode
				if (dragMode == DRAG_MARKER)
				{
					float time = pixelToTime (xPos - trackOrigin);
					Marker *m = getMarker (dragMarkerId);
					if (m == NULL)
					{
						// The marker has been deleted.
						// Forcibly abort drag mode.
						dragMode = DRAG_NONE;
						dragMarkerId = 0;
						dragOriginX = 0;
						dragOffsetX = 0.0;
					}
					else
					{
						// A marker is being dragged.  Position it and fire events.
						m->pos = time - dragOffsetX;
						tipTimeDraw = TRUE;
						tipTime = time;
						tipX = xPos;
						tipY = yPos + TIP_Y_OFFSET;
						FireViewChange();
						Fire_MarkerChanged(dragMarkerId);
					}
				}
				else if (dragMode == DRAG_CURSOR)
				{
					// The cursor is being dragged. Position it and fire events.
					float time = pixelToTime (xPos - trackOrigin);
					cursorPos = time;
					tipTimeDraw = TRUE;
					tipTime = time;
					tipX = xPos;
					tipY = yPos + TIP_Y_OFFSET;
					FireViewChange();
					Fire_CursorChanged(cursorPos);
				}
			}
			else
			{
				// Absolutely not dragging, so force a drag mode abort
				dragMode = DRAG_NONE;
				dragMarkerId = 0;
				dragOriginX = 0;
				dragOffsetX = 0.0;

				// If the cursor in the track view, display the time tip.
				if (xPos >= trackOrigin)
				{
					tipTimeDraw = TRUE;
					tipTime = pixelToTime (xPos - trackOrigin);
					tipX = xPos;
					tipY = yPos + TIP_Y_OFFSET;
					FireViewChange();
				}
				else
				{
					tipTimeDraw = FALSE;
				}
			}
			break;

		case WM_MOUSEWHEEL:
			// Do nothing for now.
			break;

		default:
			bHandled = FALSE;
			break;
		}
	}
	return 0;
}

LRESULT CTimeline::OnEraseBackground(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	// Pretend that we erased the background.
	bHandled = TRUE;
	return TRUE;
}

STDMETHODIMP CTimeline::AddTrack(int trackId, int _parentId)
{
	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is not already in use. If it is, do nothing.

	Track *here = &trackHead;
	while (here->next)
	{
		if (here->id == trackId)
		{
			return S_OK;
		}
		here = here->next;
	}

	// The id is unique, so create a new track and append it to the end of the list.
	Track *nt = new Track;
	nt->next = NULL;
	here->next = nt;
	nt->userData = 0;
	nt->id = trackId;

	// If the parentId is a valid track, make this track the last child of that track.
	nt->childId = 0;
	nt->expanded = true;
	nt->peerId = 0;

	Track *pt = getTrack(_parentId);
	if (pt == NULL)
	{
		// No parent, so use trackHead as the parent.
		pt = &trackHead;
	}

	nt->parentId = pt->id;
	nt->level = pt->level + 1;
	if (pt->childId == 0)
	{
		pt->childId = trackId;
	}
	else
	{
		Track *ct = getTrack(pt->childId);
		while (ct->peerId)
		{
			ct = getTrack(ct->peerId);
		}
		ct->peerId = trackId;
	}

	// Perform standard track change work.
	onTrackChange ();
	return S_OK;
}

STDMETHODIMP CTimeline::DelTrack(int trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *prev = &trackHead;
	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			// Delete all the markers associated with this track.
			DelAllTrackMarkers (here->id);

			// Delete all children of this track.
			while (here->childId)
			{
				// NOTE: This will automatically change childId
				DelTrack (here->childId);
			}

			// Remove this from its peer list, if any.
			Track *pt = getTrack(here->parentId);
			if (pt == NULL)
			{
				pt = &trackHead;
			}
			if (pt->childId == trackId)
			{
				pt->childId = here->peerId;
			}
			else
			{
				Track *ct = getTrack(pt->childId);
				while (ct->peerId)
				{
					if (ct->peerId == trackId)
					{
						ct->peerId = here->peerId;
						break;
					}
					ct = getTrack(ct->peerId);
				}
			}

			// Remove this from the track list.
			// NOTE: Do this only after removing all children and markers.
			prev->next = here->next;
			delete here;
			onTrackChange();
			return S_OK;
		}
		prev = here;
		here = here->next;
	}

	// The id is not in use.
	return E_FAIL;
}

STDMETHODIMP CTimeline::SetTrackName(int trackId, BSTR name)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			// Clear out the current name, then attach the name to the given
			// BSTR.
			// NOTE: By OLE convention, we take ownership of the name string.
			here->name.Empty();
			here->name = name;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetTrackName(int trackId, BSTR *pName)
{
	// TODO: Add your implementation code here

	// Default return value.
	*pName = NULL;

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			// Make a copy of the name string to return to the caller.
			*pName = here->name.Copy();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::AddMarker(int trackId, int markerId)
{
	// TODO: Add your implementation code here

	// 0 is invalid as a track or marker id.
	if (trackId == 0 || markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is not already in use. If it is, do nothing.
	Marker *here = &markerHead;
	while (here->next)
	{
		if (here->id == markerId)
		{
			return S_OK;
		}
		here = here->next;
	}

	// The id is unique, so create a new marker and append it to the end of the list.
	Marker *nm = new Marker;
	nm->next = NULL;
	here->next = nm;
	nm->userData = 0;
	nm->id = markerId;
	nm->trackId = trackId;
	nm->pos = 0.0;
	nm->duration = 0.0;
	nm->linkId = 0;
	nm->drawCount = drawCount - 1; // guarantee that it will be drawn.

	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::DelMarker(int markerId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Unlink the marker from any markers it might be linked to.
	UnlinkMarkers(markerId);

	// Verify that the given ID is in use. If not, do nothing.

	Marker *prev = &markerHead;
	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			prev->next = here->next;
			delete here;
			FireViewChange();
			return S_OK;
		}
		prev = here;
		here = here->next;
	}

	// The id is not in use.
	return E_FAIL;
}

STDMETHODIMP CTimeline::SetMarkerPos(int markerId, float pos)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->pos = pos;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetMarkerPos(int markerId, float * pos)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*pos = here->pos;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::SetMarkerData(int markerId, LPUNKNOWN data)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->userData = (DWORD) data;
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetMarkerData(int markerId, LPUNKNOWN * data)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*data = (LPUNKNOWN) here->userData;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::SetMarkerTrack(int markerId, int trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->trackId = trackId;
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetMarkerTrack(int markerId, int * trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*trackId = here->trackId;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}



STDMETHODIMP CTimeline::get_StartTime(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = startTime;
	return S_OK;
}

STDMETHODIMP CTimeline::put_StartTime(float newVal)
{
	// TODO: Add your implementation code here
	startTime = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_StopTime(float * pVal)
{
	// TODO: Add your implementation code here
	*pVal = stopTime;
	return S_OK;
}

STDMETHODIMP CTimeline::put_StopTime(float newVal)
{
	// TODO: Add your implementation code here
	stopTime = newVal;
	FireViewChange();
	return S_OK;
}


STDMETHODIMP CTimeline::SetTrackData(int trackId, LPUNKNOWN userData)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			here->userData = (DWORD) userData;
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetTrackData(int trackId, LPUNKNOWN * userData)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			*userData = (LPUNKNOWN) here->userData;
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

int CTimeline::countTracks()
{
	int count = 0;
	Track *here = trackHead.next;
	while (here)
	{
		++count;
		here = here->next;
	}

	return count;
}


STDMETHODIMP CTimeline::ExpandTrack(int trackId, int expand)
{
	// TODO: Add your implementation code here
	Track *t = getTrack (trackId);
	if (t != NULL)
	{
		// If expand is 0, set expand to false.
		// If expand is 1, set expand to true.
		// Anything else, toggle the current value.
		if (expand == 0)
		{
			t->expanded = false;
		}
		else if (expand == 1)
		{
			t->expanded = true;
		}
		else
		{
			t->expanded = !(t->expanded);
		}
		onTrackChange ();
		return S_OK;
	}
	return E_FAIL;
}

STDMETHODIMP CTimeline::DelAllTracks()
{
	// TODO: Add your implementation code here

	// Repeatedly delete the first track, until there are no more.
	while(trackHead.next)
	{
		DelTrack (trackHead.next->id);
	}
	return S_OK;
}

STDMETHODIMP CTimeline::DelAllMarkers()
{
	// TODO: Add your implementation code here

	// Deletes all markers in the control.

	Marker *here = markerHead.next;
	while (here)
	{
		Marker *next = here->next;
		delete here;
		here = next;
	}

	// Clear out the linked list.
	markerHead.next = NULL;

	// Redraw and return
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::DelAllTrackMarkers(int trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Traverse the list of markers, deleting all those whose track
	// id matches the given trackId.

	Marker *prev = &markerHead;
	Marker *here = markerHead.next;
	while (here)
	{
		if (here->trackId == trackId)
		{
			prev->next = here->next;
			delete here;
			here = prev->next;
		}
		else
		{
			prev = here;
			here = here->next;
		}
	}
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_Height(long * pVal)
{
	// TODO: Add your implementation code here
	// Return the height of the visible track area.
	// This is used by the parent of the control to know when to
	// enable a scroll bar.

	if (trackList != NULL)
	{
		int count = 0;
		Track **here = trackList;
		while (*here)
		{
			++count;
			++here;
		}

		*pVal = count * trackHeight;
	}

	return S_OK;
}

STDMETHODIMP CTimeline::SetMarkerDuration(int markerId, float duration)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Negative durations are not allowed.

	if (duration < 0.0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->duration = duration;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::GetMarkerDuration(int markerId, float * duration)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return E_FAIL;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*duration = here->duration;
			FireViewChange();
			return S_OK;
		}
		here = here->next;
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::get_ShowScale(BOOL * pVal)
{
	// TODO: Add your implementation code here

	*pVal = useScale;
	return S_OK;
}

STDMETHODIMP CTimeline::put_ShowScale(BOOL newVal)
{
	// TODO: Add your implementation code here

	useScale = newVal;
	FireViewChange();
	return S_OK;
}

STDMETHODIMP CTimeline::get_MarkerPosition(int markerId, float * pVal)
{
	// TODO: Add your implementation code here

	return GetMarkerPos (markerId, pVal);
}

STDMETHODIMP CTimeline::put_MarkerPosition(int markerId, float newVal)
{
	// TODO: Add your implementation code here

	return SetMarkerPos (markerId, newVal);
}

STDMETHODIMP CTimeline::get_Track(long index, LPDISPATCH * pVal)
{
	// TODO: Add your implementation code here

	return CoCreateInstance (CLSID_TrackObject, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void **) &pVal);
}

STDMETHODIMP CTimeline::LinkMarkers(long mId1, long mId2)
{
	// TODO: Add your implementation code here

	// Find the two markers, linking them togather.
	// They MUST be on the same track in order to link to each other.
	// Old links are broken and cleaned up with this method.

	Marker *m1 = getMarker(mId1);
	Marker *m2 = getMarker(mId2);

	if (m1 != NULL && m2 != NULL)
	{
		if (m1->trackId == m2->trackId)
		{
			// Remove all markers which are already linked to these markers.

			UnlinkMarkers(mId1);
			UnlinkMarkers(mId2);

			// Link the markers togather.

			m1->linkId = mId2;
			m2->linkId = mId1;

			// Indicate that the view has changed.
			FireViewChange();

			return S_OK;
		}
	}

	return E_FAIL;
}

STDMETHODIMP CTimeline::UnlinkMarkers(long mId)
{
	// TODO: Add your implementation code here

	// This unlinks the given marker from any other marker it is linked to.

	Marker *m = getMarker (mId);
	if (m != NULL)
	{
		Marker *lm = getMarker(m->linkId);
		if (lm != NULL)
		{
			if (lm->linkId == mId)
			{
				lm->linkId = 0;
			}
			m->linkId = 0;
		}
		FireViewChange();
		return S_OK;
	}

	return E_FAIL;
}
