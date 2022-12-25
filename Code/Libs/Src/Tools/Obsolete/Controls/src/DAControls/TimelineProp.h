// TimelineProp.h : Declaration of the CTimelineProp

#ifndef __TIMELINEPROP_H_
#define __TIMELINEPROP_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_TimelineProp;

/////////////////////////////////////////////////////////////////////////////
// CTimelineProp
class ATL_NO_VTABLE CTimelineProp :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTimelineProp, &CLSID_TimelineProp>,
	public IPropertyPageImpl<CTimelineProp>,
	public CDialogImpl<CTimelineProp>
{
public:
	CTimelineProp() 
	{
		m_dwTitleID = IDS_TITLETimelineProp;
		m_dwHelpFileID = IDS_HELPFILETimelineProp;
		m_dwDocStringID = IDS_DOCSTRINGTimelineProp;
	}

	enum {IDD = IDD_TIMELINEPROP};

DECLARE_REGISTRY_RESOURCEID(IDR_TIMELINEPROP)

BEGIN_COM_MAP(CTimelineProp) 
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CTimelineProp)
	CHAIN_MSG_MAP(IPropertyPageImpl<CTimelineProp>)
END_MSG_MAP()

	STDMETHOD(Apply)(void)
	{
		ATLTRACE(_T("CTimelineProp::Apply\n"));
		for (UINT i = 0; i < m_nObjects; i++)
		{
			// Do something interesting here
			// ICircCtl* pCirc;
			// m_ppUnk[i]->QueryInterface(IID_ICircCtl, (void**)&pCirc);
			// pCirc->put_Caption(CComBSTR("something special"));
			// pCirc->Release();
		}
		m_bDirty = FALSE;
		return S_OK;
	}
};

#endif //__TIMELINEPROP_H_
