// EdTimelineDlg.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "resource.h"
#include "EdTimelineDlg.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Compile switches
//

#define USE_FIXED_LENGTH 0   // set to 1 for length invarient with view size, 0 for scale invarient with view size

//
// Constants
//

const int MARKER_RAD_PERCENT = 80;
const int CURSOR_GRAB_RAD = 5;
const int TIP_MARGIN = 2;
const int TIP_Y_OFFSET = 16;
const int LEVEL_INDENT = 16;
const int SCALE_HEIGHT = 28;
const int LABEL_RECT_PERCENT = 20;
const int MAX_LABEL_RECT_WIDTH = 120;
const int TIME_SCALE_PIP_PIXELS = 24;
const int TIME_SCALE_POSITIONS = 12;
const int TIME_SCALE_PIP_MIN = 2;
const COLORREF TIME_TIP_COLOR = RGB(255,255,128);
const COLORREF STRING_TIP_COLOR = RGB(128,255,255);
const COLORREF START_COLOR = RGB(0,255,0);
const COLORREF STOP_COLOR = RGB(255,0,0);
const COLORREF CURSOR_COLOR = RGB(0,0,255);

static const float timeTable[TIME_SCALE_POSITIONS] =
{
	1.0f/32.0f, // 1/32 second
	1.0f/16.0f, // 1/16 second
	1.0f/8.0f,  // 1/8 second
	1.0f/4.0f,  // 1/4 second
	0.5,        // 1/2 second
	1.0,        // 1 second
	2.0,        // 2 second
	4.0,        // 4 second
	10.0,       // 10 second
	20.0,       // 20 second
	120.0,   	// 1 minute
	240.0       // 2 minutes
};

static const char *timeStringTable[TIME_SCALE_POSITIONS] =
{
	"1/32 second",
	"1/16 second",
	"1/8 second",
	"1/4 second",
	"1/2 second",
	"1 second",
	"2 second",
	"4 second",
	"10 second",
	"20 second",
	"1 minute",
	"2 minutes"
};

/////////////////////////////////////////////////////////////////////////////
// EdTimelineDlg dialog


EdTimelineDlg::EdTimelineDlg(CWnd* pParent /*=NULL*/)
	: CDialog(EdTimelineDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(EdTimelineDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	cursorPos = 0.0;
	length = 100.0;                  // number of seconds of information to display, i.e. secs/view
	scale = TIME_SCALE_PIP_PIXELS;   // number of pixels per second to display.
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

	tipType = kNone;
	tipTime = 0.0;
	tipX = tipY = 0;
	tipString[TIP_STRING_LENGTH] = 0;

	useScale = TRUE;

	trackCount = 0;
	visCount = 0;
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

	hTimeTipBrush = CreateSolidBrush (TIME_TIP_COLOR);
	hStringTipBrush = CreateSolidBrush (STRING_TIP_COLOR);

	hClosedBmp = (HBITMAP) LoadImage (AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_CLOSED), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	hOpenBmp = (HBITMAP) LoadImage (AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_OPEN), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

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

	memDc = NULL;
	memBits = NULL;
	oldBitmap = NULL;

	lastHeight = -1;
}


EdTimelineDlg::~EdTimelineDlg()
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
	if (hTimeTipBrush != NULL)
	{
		DeleteObject (hTimeTipBrush);
	}
	if (hStringTipBrush != NULL)
	{
		DeleteObject (hStringTipBrush);
	}
	if (hOpenBmp)
	{
		DeleteObject (hOpenBmp);
	}
	if (hClosedBmp)
	{
		DeleteObject (hClosedBmp);
	}
	if (memDc)
	{
		if (oldBitmap)
		{
			SelectObject (memDc, oldBitmap);
		}
		DeleteDC (memDc);
		memDc = NULL;
	}
	if (memBits)
	{
		DeleteObject (memBits);
		memBits = NULL;
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


// Internal functions

#if USE_FIXED_LENGTH
// The old code, where the length of time displayed in the entire window is constant.
// In this case, the scale value is not set directly, but is derived from the size of window and the
// length.
int EdTimelineDlg::timeToPixel(float time)
{
	// Returns the pixel offset of a given time, relative to the scroll position, the displayed length, and
	// the pixel width of the window.
	// NOTE: hScroll is in seconds.

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

float EdTimelineDlg::pixelToTime (int pixel)
{
	// Inverse of timeToPixel;
	if (trackWidth < 1)
	{
		trackWidth = 1;
	}

	float time = pixel * length / trackWidth + hScroll;
	return time;
}
#else
// The new method, where the pixel-to-time ratio is a fixed constant, and thus more pixels equals more
// time.
// In this case, the length value is not set directly, but is derived from the size of window and the
// scale.
int EdTimelineDlg::timeToPixel(float time)
{
	// Returns the pixel offset of a given time, relative to the scroll position, the displayed length, and
	// the pixel width of the window.
	// NOTE: hScroll is in seconds.

	float t = time - hScroll;

	// t now is time relative to scroll position.
	// scroll position is alway the origin of scaling, so apply scale here.
	// The formula is:
	//    pixels = secs * (pixels/sec) = t * scale;
	// t is in pixels. Truncate it to an integer.

	// WARNING: Is is assumed that length is never 0 or negative here, because we control how it is
	// set in the interface.
	t = t * scale;
	return (int) t;
}

float EdTimelineDlg::pixelToTime (int pixel)
{
	// Inverse of timeToPixel;
	if (trackWidth < 1)
	{
		trackWidth = 1;
	}

	float time = pixel / scale + hScroll;
	return time;
}
#endif

int EdTimelineDlg::trackToPixel(int trackIndex)
{
	// Returns the vertical pixel offset of an indexed track, taking the vertical scroll position
	// into account.
	// NOTE: vScroll is in pixels.

	int y = trackIndex * trackHeight - (int) vScroll;
	return y;
}

int EdTimelineDlg::pixelToTrack(int y)
{
	// Inverse of trackToPixel;
	if (trackHeight < 1)
	{
		trackHeight = 1;
	}

	int trackIndex = (y + (int) vScroll) / trackHeight;
	return trackIndex;
}

int EdTimelineDlg::getTrackIndex(int trackId)
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

int EdTimelineDlg::getTrackId(int trackIndex)
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

Marker *EdTimelineDlg::getMarker(int markerId)
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

Track *EdTimelineDlg::getTrack (int trackId)
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

int EdTimelineDlg::getMarkerId(int trackId, float minTime, float maxTime)
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

int EdTimelineDlg::buildTrackList (Track *head, Track **list)
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

void EdTimelineDlg::checkVerticalScroll ()
{
	// This is called each time the track rectangle changes in response to a change in the client
	// rectangle size.
	// It relies on visCount being properly set by onTrackChange.

	// Enable or disable the vertical scroll bar, as needed. Also set its range.
	// NOTE: We assume here that the track rectangle is valid.
	int height = visCount * trackHeight;
	CRect r = trackRect;

	// DANGER: Calling SetScrollRange() can cause a SendMessage(WM_SIZE, ...) to be sent to this 
	// window if it causes the scrollbar to appear or disappear. To prevent infinite loops, we will always have
	// a scrollbar, and will set the range to minimal when there is no need to have one.

	ShowScrollBar (SB_VERT, TRUE);
	if (height > r.Height())
	{
		int minPos, maxPos;
		GetScrollRange(SB_VERT, &minPos, &maxPos);
		if (maxPos != height-r.Height())
		{
			SetScrollRange (SB_VERT, 0, height - r.Height(), TRUE);
		}
	}
	else
	{
		// DO NOT SET TO (0,0) OR PERFORMANCE WILL SUFFER DUE TO HIDDEN CLIENT RECT SIZE CHANGES!
		SetScrollRange (SB_VERT, 0, 1, FALSE);
	}
}

void EdTimelineDlg::onTrackChange ()
{
	// Call this each time a track is created, destroyed, expanded, or collapsed, so that
	// the track information can be recalculated.
	// This is also called when the size of the trackRect changes.

	// Count the number of tracks, then reallocate the tracklist.
	trackCount = 0;
	visCount = 0;
	Track *here = trackHead.next;
	while (here)
	{
		++trackCount;
		here = here->next;
	}
	trackList = (Track **) realloc(trackList, sizeof(Track *) * (trackCount+1));

	// Build the visible track list. NULL terminate it.
	visCount = buildTrackList (trackHead.next, trackList);
	trackList[visCount] = NULL;

	// Adjust for vertical scroll.
	checkVerticalScroll ();

	// Cause the control to be redrawn.
	InvalidateRect (trackRect, FALSE);
	InvalidateRect (labelRect, FALSE);
}

void EdTimelineDlg::drawTracks(HDC hDc, RECT lr, RECT tr)
{
	// Draw each of the labels and outline each of the tracks

	RECT rText, rTrack;
	rText.left = lr.left;
	rText.right = lr.right;
	rText.top = lr.top - (long) vScroll;
	rText.bottom = rText.top + trackHeight;
	rTrack.left = tr.left;
	rTrack.right = tr.right;
	rTrack.top = tr.top - (long) vScroll;
	rTrack.bottom = rTrack.top + trackHeight;

	// Set the clipping region to the the union of lr and tr.
	CRect clipRect;
	clipRect.UnionRect(&lr, &tr);
	HRGN oldRgn = CreateRectRgn(clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);
	if (GetClipRgn (hDc, oldRgn) != 1)
	{
		// There is no clip region, or an error occurred.  Either way, delete the old region
		DeleteObject (oldRgn);
		oldRgn = NULL;		
	}
	IntersectClipRect (hDc, clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);

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
			DrawText(hDc, (LPCSTR) (*here)->name, -1, &atr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
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

	// Restore the clip region, or lack thereof.
	SelectClipRgn (hDc, oldRgn);
	if (oldRgn != NULL)
	{
		DeleteObject (oldRgn);
	}
}

void EdTimelineDlg::drawSingleMarker (HDC hDc, RECT tr, int mRad, Marker *here)
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
			HBRUSH	markerBrush = CreateSolidBrush(here->color);
			HBRUSH	oldBrush = (HBRUSH)SelectObject(hDc, markerBrush);

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

			markerBrush = (HBRUSH)SelectObject(hDc, oldBrush);
			DeleteObject(markerBrush);

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

void EdTimelineDlg::drawMarkers(HDC hDc, RECT tr)
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
		drawSingleMarker (hDc, tr, mRad, here);
		here = here->next;
	}

	// Restore the clip region, or lack thereof.
	SelectClipRgn (hDc, oldRgn);
	if (oldRgn != NULL)
	{
		DeleteObject (oldRgn);
	}
}

struct ScaleTickData
{
	float timeStep;
	int   pixelHeight;
};

void EdTimelineDlg::drawScale(HDC hDc, RECT tr)
{
#if USE_FIXED_LENGTH
	// Figure out the tick frequency and time text frequency based on the current width of the
	// track view.
	// NOTE: Since drawing these ticks is expensive, and the pattern is repeating, we should build
	// a bitmap when the size changes and just blit it enough times here.

	// With our scaling view, the scale should be uniform for a given length, regardless of the
	// size of the track rect. In fact, the scale view should make sense regardless of how
	// much of the scene is visible.
	// This means that we should be drawing ticks at fixed time intervals.
	// To cover the entire scale, we need to draw only those ticks at time steps which are not
	// too close together on the screen.

	// So, the length of a tick is fixed for its time value, while the fact that a tick is drawn
	// is dependant on the distance from the last drawn tick.

	// The maximum tick height is 32 pixels, the minimum is 10 pixels
	// Tick ranges are:
	// 2 minute ticks == 32 pixels
	// 1 minute ticks == 30 pixels
	// 20 second ticks == 28 pixels
	// 10 second ticks == 26 pixels
	// 4 second ticks == 24 pixels
	// 2 second ticks == 22 pixels
	// 1 second ticks == 20 pixels
	// 1/2 second ticks == 18 pixels
	// 8/30 second ticks == 16 pixels
	// 4/30 second ticks == 14 pixels
	// 2/30 second ticks == 12 pixels
	// 1/30 second ticks == 10 pixels
	// 

	const int TICK_COUNT = 12;
	const int TICK_HEIGHT = SCALE_HEIGHT/2;
	const ScaleTickData tickData[TICK_COUNT] =
	{
		{120.0f,      TICK_HEIGHT-0},
		{60.0f,       TICK_HEIGHT-2},
		{20.0f,       TICK_HEIGHT-0},
		{10.0f,       TICK_HEIGHT-2},
		{4.0f,        TICK_HEIGHT-4},
		{2.0f,        TICK_HEIGHT-6},
		{1.0f,        TICK_HEIGHT-8},
		{0.5f,        TICK_HEIGHT-0},
		{1.0f/4.0f,   TICK_HEIGHT-2},
		{1.0f/8.0f,   TICK_HEIGHT-4},
		{1.0f/16.0f,  TICK_HEIGHT-6},
		{1.0f/32.0f,  TICK_HEIGHT-8}
	};

	// Draw each step size, aborting if the distance between to draws is smaller than the threshold.

	const int MIN_PIXEL_STEP = 4;
	int p0 = timeToPixel (0);
	float t0 = pixelToTime(0);
	float tend = t0 + length;
	int height = TICK_HEIGHT;
	for (int i = 0; i < TICK_COUNT && height >= 2; ++i)
	{
		// Check the pixel step against the threshold
		int p1 = timeToPixel(tickData[i].timeStep);
		if (p1 - p0 < MIN_PIXEL_STEP)
		{
			// Since we are going from largest steps to smallest steps, we can abort the outer
			// loop here; all subsequent pixel steps will be too small.
			break;
		}

		// The step is valid, so draw ticks.
		// Start at the first time before t0 which is an multiple of the time step

		float step = tickData[i].timeStep;
		float start = (float) floor(t0 / step) * step;

		if (start + step < tend)
		{
			// We will be drawing at least one of them.
			for (float f = start; f < tend; f += step)
			{
				// Draw the tick
				int x = trackOrigin + timeToPixel(f);
				MoveToEx(hDc, x, tr.top, NULL);
				LineTo(hDc, x, tr.top + height);
			}

			// Use a shorter height next time.
			height -= 2;
		}
	}
#else
	// The major pip pixel count is constant, and the time between pips is determined by the
	// scale slider.
	// WARNING: By doing the scale drawing based on the scale slider, we preclude setting the scale to
	// an arbitrary value. This should be ok in practice, since there is no external scale setting function.
	const int TICK_HEIGHT = SCALE_HEIGHT/2;

	// For now, we will just draw a major pip at the selected time scale. Minor pips can be added later

	// Get the proper time step from the time table and the scale slider, then draw major pips.
	int scalePos = m_TimeScale.GetPos();
	float t0 = pixelToTime(0);
	float tend = t0 + length;
	float step = timeTable[scalePos];
	float start = (float) floor(t0/step) * step;  // round the start time to the nearest step
	int height = TICK_HEIGHT;
	if (start + step < tend)
	{
		// We will be drawing at least one of them.
		for (float f = start; f < tend; f += step)
		{
			// Draw the tick
			int x = trackOrigin + timeToPixel(f);
			MoveToEx(hDc, x, tr.top, NULL);
			LineTo(hDc, x, tr.top + height);
		}

		// Use a shorter height next time.
		height -= 2;
	}

	// Draw minor pips
	// NOTE: This is a wierd algorithm, stolen from Adobe Premiere
	int lastMinor = 4 * (scalePos/4);
	for (int pos = lastMinor; pos < scalePos; ++pos)
	{
		step = timeTable[pos];
		start = (float) floor(t0/step) * step;  // round the start time to the nearest step
		height = TICK_HEIGHT-4;
		int p0 = timeToPixel (0);
		int p1 = timeToPixel(step);
		if (p1 - p0 < TIME_SCALE_PIP_MIN)
		{
			// Since we are going from largest steps to smallest steps, we can abort the outer
			// loop here; all subsequent pixel steps will be too small.
			break;
		}

		if (start + step < tend)
		{
			// We will be drawing at least one of them.
			for (float f = start; f < tend; f += step)
			{
				// Draw the tick
				int x = trackOrigin + timeToPixel(f);
				MoveToEx(hDc, x, tr.top, NULL);
				LineTo(hDc, x, tr.top + height);
			}

			// Use a shorter height next time.
			height -= 2;
		}
	}

	// Print the current time scale from the timeStringTable.
	RECT lr = tr;
	lr.top += TICK_HEIGHT;
	DrawText(hDc, (LPCSTR) timeStringTable[scalePos], -1, &lr, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
#endif
}

///////////////////////////////////////////////////////////////////
// Public interface
void EdTimelineDlg::ExpandTrack(long trackId, long expand)
{
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
	}
}

float EdTimelineDlg::GetCursorPos()
{
	// TODO: Add your implementation code here
	return cursorPos;
}

void EdTimelineDlg::SetCursorPos(float newVal)
{
	// TODO: Add your implementation code here
	cursorPos = newVal;
	InvalidateRect(trackRect, FALSE);
}

float EdTimelineDlg::GetLength()
{
	// TODO: Add your implementation code here
	return length;
}

void EdTimelineDlg::SetLength(float newVal)
{
	// TODO: Add your implementation code here
#if USE_FIXED_LENGTH
	setTrackLength (newVal);
#else
	// The length cannot be set when not in fixed length mode.
#endif
	updateTimeScale();
}

void EdTimelineDlg::setTrackLength(float newVal)
{
	if (newVal > 0.0)
	{
		length = newVal;
	}
//	scale = trackWidth / length;
	updateTimeScroll();
	InvalidateRect(trackRect, FALSE);
	InvalidateRect(controlRect, FALSE);
}

void EdTimelineDlg::setTrackScale(float newVal)
{
	if (newVal > 0.0)
	{
		scale = newVal;
	}
	length = trackWidth / scale;
	if (!(length > 0.0))
	{
		length = 1.0;
	}
	updateTimeScroll();
	InvalidateRect(trackRect, FALSE);
	InvalidateRect(controlRect, FALSE);
}

void EdTimelineDlg::OnOK()
{
}

float EdTimelineDlg::GetVScroll()
{
	// TODO: Add your implementation code here
	return vScroll;
}

void EdTimelineDlg::SetVScroll(float newVal)
{
	// TODO: Add your implementation code here

	int low, high;
	GetScrollRange (SB_VERT, &low, &high);
	if (newVal < low)
	{
		newVal = (float) low;
	}
	else if (newVal > high)
	{
		newVal = (float) high;
	}

	vScroll = newVal;
	SetScrollPos (SB_VERT, (int) vScroll, TRUE);
	InvalidateRect(trackRect, FALSE);
	InvalidateRect(labelRect, FALSE);
}

float EdTimelineDlg::GetHScroll()
{
	// TODO: Add your implementation code here
	return hScroll;
}

void EdTimelineDlg::SetHScroll(float newVal)
{
	// TODO: Add your implementation code here
	if (m_TimeScroll.GetSafeHwnd() != NULL)
	{
		int low, high;
		int intVal = (int) (newVal * 1000);
		m_TimeScroll.GetScrollRange (&low, &high);
		if (intVal < low)
		{
			intVal = low;
		}
		else if (newVal > high)
		{
			intVal = high;
		}
		newVal = (float) (intVal / 1000.0);
		m_TimeScroll.SetScrollPos (intVal);
	}
	hScroll = newVal;
	InvalidateRect(trackRect, FALSE);
	InvalidateRect(controlRect, FALSE);
}

long EdTimelineDlg::GetUILock()
{
	// TODO: Add your implementation code here
	return uiLock;
}

void EdTimelineDlg::SetUILock(long newVal)
{
	// TODO: Add your implementation code here
	uiLock = newVal;
	Invalidate(FALSE);
}

void EdTimelineDlg::AddTrack(long trackId, long _parentId)
{
	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is not already in use. If it is, do nothing.

	Track *prev = &trackHead;
	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			return;
		}
		prev = here;
		here = here->next;
	}

	// The id is unique, so create a new track and append it to the end of the list.
	Track *nt = new Track;
	nt->next = NULL;
	prev->next = nt;
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
}

void EdTimelineDlg::DelTrack(long trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
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
			return;
		}
		prev = here;
		here = here->next;
	}

	// The id is not in use.
}

void EdTimelineDlg::SetTrackName(long trackId, const char *name)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			// Set the name of the track to the given name.
			here->name = name;
			InvalidateRect(labelRect, FALSE);
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetTrackName(long trackId, CString &pName)
{
	// TODO: Add your implementation code here

	// Default return value.
	pName.Empty();

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			// Make a copy of the name string to return to the caller.
			pName += here->name;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::AddMarker(long trackId, long markerId)
{
	// TODO: Add your implementation code here

	// 0 is invalid as a track or marker id.
	if (trackId == 0 || markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is not already in use. If it is, do nothing.
	Marker *here = &markerHead;
	while (here->next)
	{
		if (here->id == markerId)
		{
			return;
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
	nm->color = RGB(255, 255, 255);
	nm->linkId = 0;
	nm->drawCount = drawCount - 1; // guarantee that it will be drawn.

	InvalidateRect(trackRect, FALSE);
}

void EdTimelineDlg::DelMarker(long markerId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
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
			InvalidateRect(trackRect, FALSE);
			return;
		}
		prev = here;
		here = here->next;
	}

	// The id is not in use.
}

void EdTimelineDlg::SetMarkerPos(long markerId, float pos)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->pos = pos;
			InvalidateRect(trackRect, FALSE);
			return;
		}
		here = here->next;
	}
}

void EdTimelineDlg::GetMarkerPos(long markerId, float * pos)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*pos = here->pos;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::SetMarkerColor(long markerId, COLORREF color)
{
	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->color = color;
			InvalidateRect(trackRect, FALSE);
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetMarkerColor(long markerId, COLORREF* color)
{
	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*color = here->color;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::SetMarkerData(long markerId, DWORD data)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->userData = data;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetMarkerData(long markerId, DWORD* data)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*data = here->userData;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetMarkerData(long markerId, DWORD* data) const
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*data = here->userData;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::SetMarkerTrack(long markerId, long trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->trackId = trackId;
			InvalidateRect(trackRect, FALSE);
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetMarkerTrack(long markerId, long * trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*trackId = here->trackId;
			return;
		}
		here = here->next;
	}

	return ;
}



float EdTimelineDlg::GetStartTime()
{
	// TODO: Add your implementation code here
	return startTime;
}

void EdTimelineDlg::SetStartTime(float newVal)
{
	// TODO: Add your implementation code here
	startTime = newVal;
	updateTimeScroll();
	InvalidateRect(trackRect, FALSE);
}

float EdTimelineDlg::GetStopTime()
{
	// TODO: Add your implementation code here
	return stopTime;
}

void EdTimelineDlg::SetStopTime(float newVal)
{
	// TODO: Add your implementation code here
	stopTime = newVal;
	updateTimeScroll();
	InvalidateRect(trackRect, FALSE);
}


void EdTimelineDlg::SetTrackData(long trackId, DWORD userData)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			here->userData = userData;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetTrackData(long trackId, DWORD * userData) const
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Track *here = trackHead.next;
	while (here)
	{
		if (here->id == trackId)
		{
			*userData = here->userData;
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::DelAllTracks()
{
	// TODO: Add your implementation code here

	// Repeatedly delete the first track, until there are no more.
	while(trackHead.next)
	{
		DelTrack (trackHead.next->id);
	}
}

void EdTimelineDlg::DelAllMarkers()
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
	InvalidateRect(trackRect, FALSE);
}

void EdTimelineDlg::DelAllTrackMarkers(long trackId)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (trackId == 0)
	{
		// Do nothing.
		return;
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
	InvalidateRect(trackRect, FALSE);
}

long EdTimelineDlg::GetHeight()
{
	// TODO: Add your implementation code here
	// Return the height of the visible track area.
	// This is used by the parent of the control to know when to
	// enable a scroll bar.

	if (trackList != NULL)
	{
		return visCount * trackHeight;
	}

	return 0;
}

void EdTimelineDlg::SetMarkerDuration(long markerId, float duration)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Negative durations are not allowed.

	if (duration < 0.0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			here->duration = duration;
			InvalidateRect(trackRect, FALSE);
			return;
		}
		here = here->next;
	}

	return;
}

void EdTimelineDlg::GetMarkerDuration(long markerId, float * duration)
{
	// TODO: Add your implementation code here

	// 0 is an invalid track id.
	if (markerId == 0)
	{
		// Do nothing.
		return;
	}

	// Verify that the given ID is in use. If not, do nothing.

	Marker *here = markerHead.next;
	while (here)
	{
		if (here->id == markerId)
		{
			*duration = here->duration;
			return;
		}
		here = here->next;
	}

	return;
}

long EdTimelineDlg::GetShowScale()
{
	// TODO: Add your implementation code here

	return useScale;
}

void EdTimelineDlg::SetShowScale(long newVal)
{
	// TODO: Add your implementation code here

	useScale = newVal;
	Invalidate(FALSE);
}

void EdTimelineDlg::LinkMarkers(long mId1, long mId2)
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
			InvalidateRect(trackRect, FALSE);
		}
	}
}

void EdTimelineDlg::UnlinkMarkers(long mId)
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
		InvalidateRect(trackRect, FALSE);
	}
}

///////////////////////////////////////////////////////////////////////
// Event Interface
void EdTimelineDlg::OnMarkerChangeStarted(long markerId)
{
}

void EdTimelineDlg::OnMarkerChanged(long markerId)
{
}

void EdTimelineDlg::OnMarkerChangeFinished(long markerId)
{
}

void EdTimelineDlg::OnTrackChanged(long trackId)
{
}

void EdTimelineDlg::OnCursorChanged(float newPos)
{
}

void EdTimelineDlg::OnTrackLabelContextRequest(long trackId, long mouseX, long mouseY)
{
}

void EdTimelineDlg::OnMarkerContextRequest(long markerId, long mouseX, long mouseY)
{
}

void EdTimelineDlg::OnTrackContextRequest(long trackId, float time, long mouseX, long mouseY)
{
}

void EdTimelineDlg::OnMarkerTipRequest(long markerId, long mouseX, long mouseY, char* tip)
{
	tipString[TIP_STRING_LENGTH] = 0;
}
///////////////////////////////////////////////////////////////////////
// Message handling stuff goes here

void EdTimelineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EdTimelineDlg)
	DDX_Control(pDX, IDC_TIMESCALE, m_TimeScale);
	DDX_Control(pDX, IDC_TIMESCROLL, m_TimeScroll);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(EdTimelineDlg, CDialog)
	//{{AFX_MSG_MAP(EdTimelineDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EdTimelineDlg message handlers

BOOL EdTimelineDlg::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	// Pretend we erased the background.
	return TRUE;
	//return CDialog::OnEraseBkgnd(pDC);
}

void EdTimelineDlg::calcRects(RECT newClientRect)
{
	// This will calculate the rectangles for the various areas of the
	// window, based on the given client rectangle.

	labelRect = newClientRect;  // label area rect
	labelRect.right = labelRect.left + (int) ((labelRect.right - labelRect.left) * (LABEL_RECT_PERCENT/100.0f));
	if (labelRect.right > MAX_LABEL_RECT_WIDTH)
	{
		labelRect.right = MAX_LABEL_RECT_WIDTH;
	}
	trackRect = newClientRect;  // track area rect
	trackRect.left = labelRect.right;

	controlRect = newClientRect;
	if (m_TimeScroll.GetSafeHwnd() != NULL && m_TimeScale.GetSafeHwnd() != NULL)
	{
		// Create the bottom rectangle as the height of the bottom scroll bar and the height of the scale field.
		// Adjust the other rects accordingly.
		CRect scrollRect, scaleRect;
		m_TimeScroll.GetWindowRect (&scrollRect);
		m_TimeScale.GetWindowRect (&scaleRect);
	
		int h = scrollRect.Height() + SCALE_HEIGHT + 2;
		controlRect.top = controlRect.bottom - h;
		controlRect.left = labelRect.right;

		labelRect.bottom -= h;
		trackRect.bottom -= h;
	}
	else
	{
		controlRect.top = controlRect.bottom;
	}
}

void EdTimelineDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	RECT rc, blitRect;
	GetClientRect(&rc);
	blitRect = rc;

	HDC hDc = dc.m_hDC;

	// Create a memory DC instead of drawing directly.
	if (!memDc)
	{
		memDc = CreateCompatibleDC (dc.m_hDC);
	}

	if (memDc != NULL)
	{
		if (!memBits)
		{
			memBits = CreateCompatibleBitmap (dc.m_hDC, blitRect.right - blitRect.left, blitRect.bottom - blitRect.top);

			if (memBits == NULL)
			{
				DeleteDC (memDc);
				memDc = NULL;
			}
			else
			{
				oldBitmap = (HBITMAP) SelectObject (memDc, memBits);
			}
		}
	}

	if (memDc && memBits)
	{
		hDc = memDc;
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
	calcRects (rc);

	// Clear the label area
	{
		RECT fr = labelRect;
		fr.right -= GetSystemMetrics(SM_CYDLGFRAME);
		FillRect(hDc, &fr, (HBRUSH) (COLOR_WINDOW+1));
	}

	// Clear the track area by filling with the standard window background color
	FillRect(hDc, &trackRect, (HBRUSH) (COLOR_SCROLLBAR+1));

	// Clear the bottom control area
//	FillRect(hDc, &controlRect, (HBRUSH) (COLOR_INACTIVECAPTION+1));
	{
		HBRUSH hb = CreateSolidBrush (0x000000FF);
		FillRect(hDc, &controlRect, hb);
		DeleteObject (hb);
	}

	// Draw the divider
	{
		RECT er = labelRect;
		er.left = er.right - GetSystemMetrics(SM_CXDLGFRAME);
		DrawEdge(hDc, &er, EDGE_RAISED, BF_TOPLEFT | BF_BOTTOMRIGHT | BF_MIDDLE);
	}

	// Calculate the height of the tracks by baseing them on the height of the font.
	TEXTMETRIC tm;
	GetTextMetrics(hDc, &tm);
	trackHeight = tm.tmHeight + GetSystemMetrics(SM_CYDLGFRAME) * 2;
	trackWidth = trackRect.right - trackRect.left;
	trackOrigin = trackRect.left;

	// Part 2 - Draw the tracks, scale, and markers

	drawTracks(hDc, labelRect, trackRect);
	if (useScale)
	{
		RECT sr;
		sr.left = trackRect.left;
		sr.right = trackRect.right;
		sr.top = controlRect.top;
		sr.bottom = controlRect.bottom;

		drawScale(hDc, sr);
	}
	drawMarkers(hDc, trackRect);

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
		sp += trackRect.left;
		MoveToEx (hDc, sp, trackRect.top, NULL);
		LineTo (hDc, sp, trackRect.bottom);
		SelectObject (hDc, oldPen);
	}
	if (ep >= 0 && ep < trackWidth)
	{
		HPEN oldPen = (HPEN) SelectObject (hDc, hStopPen);
		ep += trackRect.left;
		MoveToEx (hDc, ep, trackRect.top, NULL);
		LineTo (hDc, ep, trackRect.bottom);
		SelectObject (hDc, oldPen);
	}
	if (cp >= 0 && cp < trackWidth)
	{
		HPEN oldPen = (HPEN) SelectObject (hDc, hCursorPen);
		cp += trackRect.left;
		MoveToEx (hDc, cp, trackRect.top, NULL);
		LineTo (hDc, cp, trackRect.bottom);
		SelectObject (hDc, oldPen);
	}

	// If a tip draw was requested, draw it.

	if (tipType == kTime)
	{
		char buffer[1024];
		wsprintf(buffer, "%d.%02d", (int) tipTime, (int) ((tipTime - ((int) tipTime)) * 100));
		SIZE size;
		if (GetTextExtentPoint32 (hDc, buffer, lstrlen(buffer), &size) != 0)
		{
			// Draw the text opaque, then outline the rectangle.
			HPEN oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
			HBRUSH oldBrush = (HBRUSH) SelectObject (hDc, hTimeTipBrush);
			COLORREF oldBkColor = GetBkColor(hDc);
			SetBkColor (hDc, TIME_TIP_COLOR);
			Rectangle(hDc, tipX-TIP_MARGIN, tipY-TIP_MARGIN, tipX + size.cx + TIP_MARGIN, tipY + size.cy + TIP_MARGIN);
			TextOut (hDc, tipX, tipY, buffer, lstrlen(buffer));
			SelectObject (hDc, oldPen);
			SelectObject (hDc, oldBrush);
			SetBkColor (hDc, oldBkColor);
		}
	}
	else if (tipType == kString)
	{
		SIZE size;
		if (GetTextExtentPoint32 (hDc, tipString, lstrlen(tipString), &size) != 0)
		{
			// Draw the text opaque, then outline the rectangle.
			HPEN oldPen = (HPEN) SelectObject (hDc, GetStockObject(BLACK_PEN));
			HBRUSH oldBrush = (HBRUSH) SelectObject (hDc, hStringTipBrush);
			COLORREF oldBkColor = GetBkColor(hDc);
			SetBkColor (hDc, STRING_TIP_COLOR);
			Rectangle(hDc, tipX-TIP_MARGIN, tipY-TIP_MARGIN, tipX + size.cx + TIP_MARGIN, tipY + size.cy + TIP_MARGIN);
			TextOut (hDc, tipX, tipY, tipString, lstrlen(tipString));
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

	// If we are using a memory DC, bit it to the main DC.
	if (memDc != NULL)
	{
		BitBlt (dc.m_hDC, 0, 0, blitRect.right - blitRect.left, blitRect.bottom - blitRect.top, memDc, 0, 0, SRCCOPY);
	}

	// Increment the draw count. It is ok to roll over.
	++drawCount;

	// Do not call CDialog::OnPaint() for painting messages
}

void EdTimelineDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	int xPos = mouseLoc.x = point.x;
	int yPos = mouseLoc.y = point.y;
	mouseFlags = nFlags;

	if (nFlags & MK_LBUTTON)
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
				tipType = kTime;
				tipTime = time;
				tipX = xPos;
				tipY = yPos + TIP_Y_OFFSET;
				InvalidateRect(trackRect, FALSE);
				OnMarkerChanged(dragMarkerId);
			}
		}
		else if (dragMode == DRAG_CURSOR)
		{
			// The cursor is being dragged. Position it and fire events.
			float time = pixelToTime (xPos - trackOrigin);
			cursorPos = time;
			tipType = kTime;
			tipTime = time;
			tipX = xPos;
			tipY = yPos + TIP_Y_OFFSET;
			InvalidateRect(trackRect, FALSE);
			OnCursorChanged(cursorPos);
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
			// Potential tip message request.
			// See if we are in a valid track.  If not, do nothing.
			tipType = kNone;
		
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
					// A valid marker, so fire the tip request
					tipType = kString;
					OnMarkerTipRequest(markerId, xPos, yPos, tipString);
				}
			}

			if(tipType == kNone)
			{
				// Not a marker
				tipType = kTime;
				tipTime = pixelToTime (xPos - trackOrigin);
				tipX = xPos;
				tipY = yPos + TIP_Y_OFFSET;
			}

			InvalidateRect(trackRect, FALSE);
		}
		else
		{
			tipType = kNone;
		}
	}

	// Don't call the inherited behavior
	// CDialog::OnMouseMove(nFlags, point);
}

void EdTimelineDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	int xPos = mouseLoc.x = point.x;
	int yPos = mouseLoc.y = point.y;
	mouseFlags = nFlags;

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
					// purposes and fire events.
					dragMode = DRAG_MARKER;
					dragMarkerId = markerId;
					dragOriginX = xPos;

					Marker *m = getMarker (markerId);
					dragOffsetX = pixelToTime(xPos - trackOrigin) - m->pos;

					OnMarkerChangeStarted(dragMarkerId);
				}
			}
		}
	}

	// Don't call the inherited behavior
	// CDialog::OnLButtonDown(nFlags, point);
}

void EdTimelineDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	int xPos = mouseLoc.x = point.x;
	int yPos = mouseLoc.y = point.y;
	mouseFlags = nFlags;

	if(dragMode == DRAG_MARKER)
	{
		// Fire events
		OnMarkerChangeFinished(dragMarkerId);
	}

	// Turn off dragging.
	dragMode = DRAG_NONE;
	
	// Don't call the inherited behavior
	//CDialog::OnLButtonUp(nFlags, point);
}

void EdTimelineDlg::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	int xPos = mouseLoc.x = point.x;
	int yPos = mouseLoc.y = point.y;
	mouseFlags = nFlags;

	CDialog::OnRButtonDown(nFlags, point);
}

void EdTimelineDlg::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	int xPos = mouseLoc.x = point.x;
	int yPos = mouseLoc.y = point.y;
	mouseFlags = nFlags;

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
					OnTrackLabelContextRequest (trackId, xPos, yPos);
				}
				else
				{
					OnTrackContextRequest (trackId, pixelToTime(xPos-trackOrigin), xPos, yPos);
				}
			}
			else
			{
				// Fire a marker request
				OnMarkerContextRequest (markerId, xPos, yPos);
			}
		}
		else
		{
			// By convention, a click outside of a valid track yields a track label request on id
			// 0.

			OnTrackLabelContextRequest(0, xPos, yPos);
		}
		
		// Turn off the time tip.
		tipType = kNone;
		InvalidateRect(trackRect, FALSE);
	}

	// Don't call the inherited behavior
	// CDialog::OnRButtonUp(nFlags, point);
}

void EdTimelineDlg::positionControls()
{
	// Reposition the child controls based on the size of the dialog window.

	CRect rc;
	GetClientRect (&rc);
	calcRects(rc);

	if (m_TimeScroll.GetSafeHwnd() != NULL)
	{
		CRect scrollRect;
		m_TimeScroll.GetWindowRect(&scrollRect);
		int x, y;
		int w, h;
		x = trackRect.left;
		y = controlRect.bottom - scrollRect.Height();
		w = trackRect.Width();
		h = scrollRect.Height();
		m_TimeScroll.SetWindowPos (NULL, x, y, w, h, SWP_NOZORDER);
	}

	if (m_TimeScale.GetSafeHwnd() != NULL)
	{
		int x, y;
		int w, h;
		x = labelRect.left;
		y = controlRect.top;
		w = labelRect.Width();
		h = controlRect.Height();
		m_TimeScale.SetWindowPos (NULL, x, y, w, h, SWP_NOZORDER);
	}
}

void EdTimelineDlg::updateTimeScroll()
{
	// This function adjusts the range, activation, and position of the
	// track scrollbar based on the current length, stop, and start times.
	// You should call this when you change any of those values.

	// Disable or enable the horizontal scrollbar according to how much of the
	// availble time is visible.
	// NOTE: HScroll is in seconds. The scrollbar is in milliseconds.
	if (m_TimeScroll.GetSafeHwnd() != NULL)
	{
		float time = (float) fabs(stopTime - startTime);
		if (length < time)
		{
			m_TimeScroll.EnableScrollBar(ESB_ENABLE_BOTH);
			m_TimeScroll.SetScrollRange
				(
					(int) (min(stopTime, startTime) * 1000),
					(int) ((max(stopTime, startTime) - length) * 1000)
				);
		}
		else
		{
			m_TimeScroll.EnableScrollBar(ESB_DISABLE_BOTH);
			m_TimeScroll.SetScrollRange (0, 0);
			SetHScroll(0);
		}
	}
}

void EdTimelineDlg::updateTimeScale()
{
	// This function adjusts the position of the time scale to match the
	// current track view length.

	if (m_TimeScale.GetSafeHwnd() != NULL)
	{
#if USE_FIXED_LENGTH
		// Make sure that the scale slider is adjusted to match the length
		m_TimeScale.SetRange(-2, 3);
		int newPos = (int) log10(length);
		m_TimeScale.SetPos(newPos);
#else
		// The scale slider doesn't need to change. Leave it as is.
#endif
	}
}

void EdTimelineDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here

	// Enable or disable the control, depending on the current size of the content
	// and the size of the client area.

	CRect rc;
	GetClientRect (&rc);

	calcRects(rc);

	// Check for vertical scroll if the height has changed.
	int height = rc.Height();
	if (lastHeight != height)
	{
		lastHeight = height;
		checkVerticalScroll();
	}

	// Reallocate the memory bitmap for the new size.
	if (memDc)
	{
		if (oldBitmap)
		{
			SelectObject (memDc, oldBitmap);
		}

		if (memBits)
		{
			DeleteObject (memBits);
			memBits = NULL;
		}

		CDC *myDC = GetDC();
		memBits = CreateCompatibleBitmap (myDC->m_hDC, rc.right - rc.left, rc.bottom - rc.top);
		ReleaseDC(myDC);

		if (memBits == NULL)
		{
			DeleteDC (memDc);
			memDc = NULL;
		}
		else
		{
			oldBitmap = (HBITMAP) SelectObject (memDc, memBits);
		}
	}

	// Reposition the controls
	positionControls ();

	Invalidate(FALSE);
}

void EdTimelineDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default

	switch (nSBCode)
	{
	case SB_BOTTOM:
	case SB_ENDSCROLL:
		break;

	case SB_LINEDOWN:
		SetVScroll(vScroll + trackHeight);
		break;

	case SB_LINEUP:
		SetVScroll(vScroll - trackHeight);
		break;

	case SB_PAGEDOWN:
		SetVScroll(vScroll + trackRect.Height());
		break;

	case SB_PAGEUP:
		SetVScroll(vScroll - trackRect.Height());
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		SetVScroll((float) nPos);
		UpdateWindow();
		break;

	case SB_TOP:
		break;
	}
	
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}


void EdTimelineDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	const float MSEC = 0.001f;
	const int LINE_IN_PIXELS = 1;


	if (pScrollBar->GetDlgCtrlID() == IDC_TIMESCROLL)
	{
		switch (nSBCode)
		{
			// Notifications for the scrollbar
		case SB_BOTTOM:
		case SB_ENDSCROLL:
			break;

		case SB_LINEDOWN:
			SetHScroll(hScroll + pixelToTime(LINE_IN_PIXELS) - pixelToTime(0));
			break;

		case SB_LINEUP:
			SetHScroll(hScroll - pixelToTime(LINE_IN_PIXELS) + pixelToTime(0));
			break;

		case SB_PAGEDOWN:
			SetHScroll(hScroll + pixelToTime(trackRect.Width()) - pixelToTime(0));
			break;

		case SB_PAGEUP:
			SetHScroll(hScroll - pixelToTime(trackRect.Width()) + pixelToTime(0));
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			// NOTE: The nPos value is limited to 16 bits of information. We need 32 bits, so
			// we will use the GetScrollInfo function to get to the full 32 bits of position.
			{
				SCROLLINFO si;
				memset(&si, 0, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SIF_TRACKPOS;
				pScrollBar->GetScrollInfo (&si, SIF_TRACKPOS);
//				SetHScroll((float) (nPos / 1000.0));
				SetHScroll((float) (si.nTrackPos / 1000.0));
				UpdateWindow();
			}
			break;

		case SB_TOP:
			break;
		}
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_TIMESCALE)
	{
		// The scale slider works in a fixed number of settings, each setting determining
		// the amount of information displayed in the track view.
		// For fixed length, each position determines how much time is displayed in the window
		// directly from 1/100th of a second, to 1000 seconds.
		// It goes in powers of 10: 10e-2 to 10e3. This is six divisions.

		// For the non-fixed length, each position determines the displayed scale, in pixels per sec.
		// This is done like adobe premiere. The scale displays pips at a fixed pixel interval, and the
		// slider determines how much time is displayed in that interval.

		CSliderCtrl *pSlider = (CSliderCtrl *) pScrollBar;
		switch (nSBCode)
		{
			// Notifications for the scale slider
		case TB_ENDTRACK:
			// Sent after position changes.
#if USE_FIXED_LENGTH
			// Change the length, the derive the track scale.
			setTrackLength((float) pow(10.0, pSlider->GetPos()));
#else
			// Change the scale.
			// Scale is pixels/sec, derived by dividing the pip pixels by the time in the table.
			setTrackScale ((float) TIME_SCALE_PIP_PIXELS / timeTable[pSlider->GetPos()]);
#endif
			UpdateWindow();
			break;

		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
#if USE_FIXED_LENGTH
			setTrackLength((float) pow(10.0, nPos));
#else
			// Change the scale.
			// Scale is pixels/sec, derived by dividing the pip pixels by the time in the table.
			setTrackScale ((float) TIME_SCALE_PIP_PIXELS / timeTable[nPos]);
#endif
			UpdateWindow();
			break;

		case TB_PAGEDOWN:
		case TB_PAGEUP:
		case TB_LINEDOWN:
		case TB_LINEUP:
		case TB_BOTTOM:
		case TB_TOP:
			break;
		}
	}
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL EdTimelineDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

#if USE_FIXED_LENGTH
	m_TimeScale.SetRange(-2, 3);
#else
	m_TimeScale.SetRange(0, TIME_SCALE_POSITIONS-1);
	m_TimeScale.SetPos(5);
#endif
	positionControls ();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

