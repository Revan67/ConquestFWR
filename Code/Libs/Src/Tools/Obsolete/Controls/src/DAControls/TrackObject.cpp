// TrackObject.cpp : Implementation of CDAControlsApp and DLL registration.

#include "stdafx.h"
#include "DAControls.h"
#include "TrackObject.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP TrackObject::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITrackObject,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP TrackObject::get_Id(long * pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP TrackObject::get_Name(BSTR * pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP TrackObject::put_Name(BSTR newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP TrackObject::AddMarker(long markerId)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP TrackObject::get_Expanded(BOOL * pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP TrackObject::put_Expanded(BOOL newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}
