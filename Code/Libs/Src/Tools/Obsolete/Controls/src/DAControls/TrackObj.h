// TrackObj.h : Declaration of the CTrackObj

#ifndef __TRACKOBJ_H_
#define __TRACKOBJ_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTrackObj
class ATL_NO_VTABLE CTrackObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTrackObj, &CLSID_TrackObj>,
	public IDispatchImpl<ITrackObj, &IID_ITrackObj, &LIBID_DACONTROLSLib>
{
public:
	CTrackObj()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TRACKOBJ)

BEGIN_COM_MAP(CTrackObj)
	COM_INTERFACE_ENTRY(ITrackObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ITrackObj
public:
	STDMETHOD(AddMarker)(long markerId);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Id)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Expanded)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Expanded)(/*[in]*/ BOOL newVal);
};

#endif //__TRACKOBJ_H_
