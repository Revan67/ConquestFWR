// TrackObject.h: Definition of the TrackObject class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRACKOBJECT_H__94B5A6D4_52FA_11D2_85B3_0000F4A24553__INCLUDED_)
#define AFX_TRACKOBJECT_H__94B5A6D4_52FA_11D2_85B3_0000F4A24553__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// TrackObject

class TrackObject : 
	public CComDualImpl<ITrackObject, &IID_ITrackObject, &LIBID_DACONTROLSLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<TrackObject,&CLSID_TrackObject>
{
public:
	TrackObject() {}
BEGIN_COM_MAP(TrackObject)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ITrackObject)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(TrackObject) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_TrackObject)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITrackObject
public:
	STDMETHOD(get_Expanded)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Expanded)(/*[in]*/ BOOL newVal);
	STDMETHOD(AddMarker)(long markerId);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Id)(/*[out, retval]*/ long *pVal);
};

#endif // !defined(AFX_TRACKOBJECT_H__94B5A6D4_52FA_11D2_85B3_0000F4A24553__INCLUDED_)
