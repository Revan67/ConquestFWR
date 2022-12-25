#if !defined(AFX_EDTIMELINEDLG_H__8AAE123C_56EC_11D2_85B3_0000F4A24553__INCLUDED_)
#define AFX_EDTIMELINEDLG_H__8AAE123C_56EC_11D2_85B3_0000F4A24553__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EdTimelineDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Supporting classes and structures
struct Marker
{
	Marker *   next;
	DWORD      userData;
	int        id;
	int        trackId;
	float      pos;
	float      duration;
	COLORREF   color;
	int        linkId;      // linked marker id, if any
	int        drawCount;   // the drawCount the last time this marker was drawn.
};

struct Track
{
	Track *    next;
	DWORD      userData;
	int        id;
	CString    name;

	// Members used to store the hierarchy.
	int        parentId;    // id of the parent track, 0 if none
	int        childId;     // id of the first child track, 0 if none
	int        peerId;      // id of the next peer track, 0 if none
	int        level;       // number of links from here to top level, i.e. parentId == 0
	bool       expanded:1;  // true if this is expanded, false otherwise. Only valid if childId != 0

	~Track() {name.Empty();}
};

enum DragMode
{
	DRAG_NONE = 0,
	DRAG_MARKER,
	DRAG_CURSOR
};

static const unsigned int TIP_STRING_LENGTH = 1024;
/////////////////////////////////////////////////////////////////////////////
// EdTimelineDlg dialog

class EdTimelineDlg : public CDialog
{
// Construction
public:
	EdTimelineDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(EdTimelineDlg)
	enum { IDD = IDD_TIMELINE };
	CSliderCtrl	m_TimeScale;
	CScrollBar	m_TimeScroll;
	//}}AFX_DATA

	virtual ~EdTimelineDlg();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EdTimelineDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Public Incoming Interface
public:
	float GetCursorPos();
	void SetCursorPos(float newValue);
	float GetLength();
	void SetLength(float newValue);
	float GetVScroll();
	void SetVScroll(float newValue);
	float GetHScroll();
	void SetHScroll(float newValue);
	long GetUILock();
	void SetUILock(long nNewValue);
	void AddTrack(long trackId, long parentId);
	void DelTrack(long trackId);
	void SetTrackName(long trackId, const char *Name);
	void GetTrackName(long trackId, CString &pName);
	void AddMarker(long trackId, long markerId);
	void DelMarker(long markerId);
	void SetMarkerPos(long markerId, float pos);
	void GetMarkerPos(long markerId, float* pos);
	void SetMarkerColor(long markerId, COLORREF color);
	void GetMarkerColor(long markerId, COLORREF* color);
	void SetMarkerData(long markerId, DWORD data);
	void GetMarkerData(long markerId, DWORD* data);
	void GetMarkerData(long markerId, DWORD* data) const;
	void SetMarkerTrack(long markerId, long trackId);
	void GetMarkerTrack(long markerId, long* trackId);
	float GetStartTime();
	void SetStartTime(float newValue);
	float GetStopTime();
	void SetStopTime(float newValue);
	void SetTrackData(long trackId, DWORD userData);
	void GetTrackData(long trackId, DWORD* userData) const;
	void ExpandTrack(long trackId, long expand);
	void DelAllTracks();
	void DelAllMarkers();
	void DelAllTrackMarkers(long trackId);
	long GetHeight();
	void SetMarkerDuration(long markerId, float duration);
	void GetMarkerDuration(long markerId, float* duration);
	long GetShowScale();
	void SetShowScale(long nNewValue);
	void LinkMarkers(long mId1, long mId2);
	void UnlinkMarkers(long mId);

// Event Interface. The default methods do nothing. Overload them to perform your own work.
	virtual void OnMarkerChangeStarted(long markerId);
	virtual void OnMarkerChanged(long markerId);
	virtual void OnMarkerChangeFinished(long markerId);
	virtual void OnTrackChanged(long trackId);
	virtual void OnCursorChanged(float newPos);
	virtual void OnTrackLabelContextRequest(long trackId, long mouseX, long mouseY);
 	virtual void OnMarkerContextRequest(long markerId, long mouseX, long mouseY);
	virtual void OnTrackContextRequest(long trackId, float time, long mouseX, long mouseY);
 	virtual void OnMarkerTipRequest(long markerId, long mouseX, long mouseY, char* tip);
	
// Internal methods
protected:
	int timeToPixel(float time);
	int trackToPixel(int trackIndex);
	float pixelToTime (int pixel);
	int pixelToTrack(int y);
	int getTrackIndex(int trackId);
	int getTrackId(int trackIndex);
	Marker *getMarker(int markerId);
	Track *getTrack (int trackId);
	int getMarkerId(int trackId, float minTime, float maxTime);
	int buildTrackList (Track *head, Track **list);
	void onTrackChange ();
	void checkVerticalScroll ();
	void drawTracks(HDC hDc, RECT lr, RECT tr);
	void drawSingleMarker (HDC hDc, RECT tr, int mRad, Marker *here);
	void drawMarkers(HDC hDc, RECT tr);
	void drawScale(HDC hDc, RECT tr);
	void positionControls();
	void calcRects(RECT newClientRect);
	void updateTimeScroll();
	void updateTimeScale();
	void setTrackLength(float newValue);  // this doesn't modify the scale slider.
	void setTrackScale(float newValue);   // this doesn't modify the scale slider.

	virtual void OnOK();

// Data members
private:
	enum TipType
	{
		kNone,		// No tip
		kTime,		// Show the time
		kString		// Show the string
	};

	// Child Controls 
	CScrollBar m_VScroll;
	CScrollBar m_HScroll;

	// User interface related members
	float startTime;
	float stopTime;
	float cursorPos;  // current cursor position
	float length;     // current length
	float scale;      // current scale, pixels/sec
	float vScroll;    // current scroll position of top of view
	float hScroll;    // current scroll position of the track portion of the display only
	BOOL  uiLock;     // if TRUE, user cannot manipulate the cursor or markers.

	int      dragOriginX;
	int      dragMarkerId;
	float    dragOffsetX;   // The offset in time from a marker's position to where it was clicked.
	DragMode dragMode;

	HPEN    hStopPen;
	HPEN    hStartPen;
	HPEN    hCursorPen;
	HBRUSH  hTimeTipBrush;
	HBRUSH  hStringTipBrush;
	HFONT   hFont;
	HBITMAP hOpenBmp;
	HBITMAP hClosedBmp;
	SIZE    openSize;
	SIZE    closedSize;

	TipType tipType;	
	float   tipTime;     // time to display in tip
	int     tipX;        // x position of time tip
	int     tipY;        // y position of time tip
	char	tipString[TIP_STRING_LENGTH + 1];

	BOOL    useScale;    // TRUE if the time scale should be drawn.

	POINT   mouseLoc;    // last mouse location
	DWORD   mouseFlags;  // last mouse flags

	int     drawCount;   // incremented each time the control is drawn.

	long    trackHeight; // cached track height.
	long    trackWidth;  // cached track view width.
	long    trackOrigin; // cached width of the label portion

	Track **trackList;   // trackList[trackCount+1], the list of drawn track pointers, NULL terminated

	// Database related members

	Track   trackHead;   // first (invalid) track entry. head of the list.
	Marker  markerHead;  // first (invalid) marker entry.  head of the list.

	int     trackCount;  // current number of tracks in the list.
	int     visCount;    // current number of visible tracks in the list.

	CRect   labelRect;       // rectangle for the label area
	CRect   trackRect;       // rectangle for the track area
	CRect   controlRect;     // rectangle for the bottom control area
	HDC     memDc;           // memory DC used to expidite drawing
	HBITMAP memBits;         // bitmap used for offscreen drawing
	HBITMAP oldBitmap;       // default bitmap selecting into memDc

	int     lastHeight;      // the last height of this control

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(EdTimelineDlg)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDTIMELINEDLG_H__8AAE123C_56EC_11D2_85B3_0000F4A24553__INCLUDED_)
