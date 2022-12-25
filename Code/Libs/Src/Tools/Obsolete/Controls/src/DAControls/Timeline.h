// Timeline.h : Declaration of the CTimeline

#ifndef __TIMELINE_H_
#define __TIMELINE_H_

#include "resource.h"       // main symbols
#include "CPDAControls.h"

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
	int        linkId;      // linked marker id, if any
	int        drawCount;   // the drawCount the last time this marker was drawn.
};

struct Track
{
	Track *    next;
	DWORD      userData;
	int        id;
	CComBSTR   name;
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

/////////////////////////////////////////////////////////////////////////////
// CTimeline
class ATL_NO_VTABLE CTimeline : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTimeline, &CLSID_Timeline>,
	public CComControl<CTimeline>,
	public IDispatchImpl<ITimeline, &IID_ITimeline, &LIBID_DACONTROLSLib>,
	public IProvideClassInfo2Impl<&CLSID_Timeline, &DIID__TimelineEvents, &LIBID_DACONTROLSLib>,
	public IPersistStreamInitImpl<CTimeline>,
	public IPersistStorageImpl<CTimeline>,
	public IQuickActivateImpl<CTimeline>,
	public IOleControlImpl<CTimeline>,
	public IOleObjectImpl<CTimeline>,
	public IOleInPlaceActiveObjectImpl<CTimeline>,
	public IViewObjectExImpl<CTimeline>,
	public IOleInPlaceObjectWindowlessImpl<CTimeline>,
	public IDataObjectImpl<CTimeline>,
	public CProxy_TimelineEvents<CTimeline>,
	public IConnectionPointContainerImpl<CTimeline>,
	public IPropertyNotifySinkCP<CTimeline>,
	public ISpecifyPropertyPagesImpl<CTimeline>
{
public:
	CTimeline();
	 ~CTimeline();

DECLARE_REGISTRY_RESOURCEID(IDR_TIMELINE)

BEGIN_COM_MAP(CTimeline)
	COM_INTERFACE_ENTRY(ITimeline)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
	COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
	COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY_IMPL(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CTimeline)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	//PROP_ENTRY("Timeline", 1, CLSID_TimelineProp)
	PROP_PAGE(CLSID_TimelineProp)
	//PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_CONNECTION_POINT_MAP(CTimeline)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    CONNECTION_POINT_ENTRY(DIID__TimelineEvents)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CTimeline)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	// TNB: My event handlers 
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouse)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnMouse)
	MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMouse)
	MESSAGE_HANDLER(WM_RBUTTONUP, OnMouse)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouse)
	MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouse)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
		return S_OK;
	}

// ITimeline
public:
	STDMETHOD(UnlinkMarkers)(long mId);
	STDMETHOD(LinkMarkers)(long mId1, long mId2);
	STDMETHOD(get_Track)(long index, /*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(get_MarkerPosition)(int markerId, /*[out, retval]*/ float *pVal);
	STDMETHOD(put_MarkerPosition)(int markerId, /*[in]*/ float newVal);
	STDMETHOD(get_ShowScale)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ShowScale)(/*[in]*/ BOOL newVal);
	STDMETHOD(GetMarkerDuration)(int markerId, float *duration);
	STDMETHOD(SetMarkerDuration)(int markerId, float duration);
	STDMETHOD(get_Height)(/*[out, retval]*/ long *pVal);
	STDMETHOD(DelAllTrackMarkers)(int trackId);
	STDMETHOD(DelAllMarkers)();
	STDMETHOD(DelAllTracks)();
	STDMETHOD(ExpandTrack)(int trackId, BOOL expand);
	float startTime;
	float stopTime;
	float cursorPos;  // current cursor position
	float vScale;     // current vertical scale
	float hScale;     // current horizontal scale
	float length;     // current length
	float vScroll;    // current scroll position of top of view
	float hScroll;    // current scroll position of the track portion of the display only
	BOOL  uiLock;     // if TRUE, user cannot manipulate the cursor or markers.

	STDMETHOD(GetTrackData)(int trackId, LPUNKNOWN *userData);
	STDMETHOD(SetTrackData)(int trackId, LPUNKNOWN userData);
	STDMETHOD(GetMarkerTrack)(int markerId, int *trackId);
	STDMETHOD(SetMarkerTrack)(int markerId, int trackId);
	STDMETHOD(GetMarkerData)(int markerId, LPUNKNOWN *data);
	STDMETHOD(SetMarkerData)(int markerId, LPUNKNOWN data);
	STDMETHOD(GetMarkerPos)(int markerId, float *pos);
	STDMETHOD(SetMarkerPos)(int markerId, float pos);
	STDMETHOD(DelMarker)(int markerId);
	STDMETHOD(AddMarker)(int trackId, int markerId);
	STDMETHOD(SetTrackName)(int trackId, BSTR name);
	STDMETHOD(GetTrackName)(int trackId, BSTR *pName);
	STDMETHOD(DelTrack)(int trackId);
	STDMETHOD(AddTrack)(int trackId, int parentId);
	LRESULT OnMouse(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnEraseBackground(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	STDMETHOD(get_StopTime)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_StopTime)(/*[in]*/ float newVal);
	STDMETHOD(get_StartTime)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_StartTime)(/*[in]*/ float newVal);
	STDMETHOD(get_UILock)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UILock)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_HScroll)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_HScroll)(/*[in]*/ float newVal);
	STDMETHOD(get_VScroll)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_VScroll)(/*[in]*/ float newVal);
	STDMETHOD(get_Length)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_Length)(/*[in]*/ float newVal);
	STDMETHOD(get_HScale)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_HScale)(/*[in]*/ float newVal);
	STDMETHOD(get_VScale)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_VScale)(/*[in]*/ float newVal);
	STDMETHOD(get_CursorPos)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_CursorPos)(/*[in]*/ float newVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);
protected:
	int buildTrackList (Track *head, Track **list);
	void onTrackChange ();
	Marker *getMarker(int markerId);
	Track *getTrack (int trackId);
	int getMarkerId(int trackId, float minTime, float maxTime);
	int countTracks();
	float pixelToTime (int pixel);
	int pixelToTrack(int y);
	int getTrackId(int trackIndex);
	int getTrackIndex (int trackId);
	void drawSingleMarker(HDC hDc, RECT r, int mRad, Marker *m);
	void drawMarkers(HDC hDc, RECT r);
	void drawTracks(HDC hDc, RECT lr, RECT tr);
	void drawScale(HDC hDc, RECT sr);
	int timeToPixel (float time);
	int trackToPixel(int trackIndex);

	int      dragOriginX;
	int      dragMarkerId;
	float    dragOffsetX;   // The offset in time from a marker's position to where it was clicked.
	DragMode dragMode;

	HPEN    hStopPen;
	HPEN    hStartPen;
	HPEN    hCursorPen;
	HBRUSH  hTipBrush;
	HFONT   hFont;
	HBITMAP hOpenBmp;
	HBITMAP hClosedBmp;
	SIZE    openSize;
	SIZE    closedSize;

	Track   trackHead;  // first (invalid) track entry. head of the list.
	Marker  markerHead;// first (invalid) marker entry.  head of the list.

	POINT   mouseLoc;   // last mouse location
	DWORD   mouseFlags; // last mouse flags

	long    trackHeight; // cached track height.
	long    trackWidth;  // cached track view width.
	long    trackOrigin; // cached width of the label portion

	BOOL    tipTimeDraw; // TRUE if the time tip should be drawn
	float   tipTime;    // time to display in tip
	int     tipX;       // x position of time tip
	int     tipY;       // y position of time tip

	BOOL    useScale;    // TRUE if the time scale should be drawn.

	int     trackCount;  // current number of tracks in the list.
	Track **trackList;   // trackList[trackCount+1], the list of drawn track pointers, NULL terminated

	int     drawCount;   // incremented each time the control is drawn.
};

#endif //__TIMELINE_H_
