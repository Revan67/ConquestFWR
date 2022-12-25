#if !defined(AFX_INTERPOLATIONKEYPROPERTIES_H__166A7AC2_ABAB_11D2_8240_0000F4A24556__INCLUDED_)
#define AFX_INTERPOLATIONKEYPROPERTIES_H__166A7AC2_ABAB_11D2_8240_0000F4A24556__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InterpolationKeyProperties.h : header file
//
#include "KeyPropertiesUI.h"
/////////////////////////////////////////////////////////////////////////////
// InterpolationKeyProperties dialog

class InterpolationKeyProperties: public TKeyPropertiesUIForm
{
	// Construction
	public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

		InterpolationKeyProperties(CWnd* pParent = NULL);   // standard constructor

		void SetInterpolationType(InterpolationType type);
		InterpolationType GetInterpolationType() const;

	// Dialog Data
		//{{AFX_DATA(InterpolationKeyProperties)
		enum { IDD = IDD_INTERPOLATION_KEY_PROPERTIES_DIALOG };
		CButton	mLinearRadio;
		CButton	mSplineRadio;
		CButton mFixedRadio;
		CButton mBlendRadio;
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(InterpolationKeyProperties)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(InterpolationKeyProperties)
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	private:
		InterpolationType	mInterpolationType;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTERPOLATIONKEYPROPERTIES_H__166A7AC2_ABAB_11D2_8240_0000F4A24556__INCLUDED_)
