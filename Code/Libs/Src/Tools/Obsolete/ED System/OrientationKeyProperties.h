// Author: Shaival Varma
// --------------------------------------------------------------------------
#if !defined(AFX_ORIENTATIONKEYPROPERTIES_H__5B13F040_B437_11D2_968C_0040333267EF__INCLUDED_)
#define AFX_ORIENTATIONKEYPROPERTIES_H__5B13F040_B437_11D2_968C_0040333267EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OrientationKeyProperties.h : header file
//
#include "KeyPropertiesUI.h"
#include "ASceneEntity.h"
/////////////////////////////////////////////////////////////////////////////
// OrientationKeyProperties dialog

class OrientationKeyProperties : public TKeyPropertiesUIForm
{
	// Construction
	public:
		enum InterpolationType
		{
			kLinear,
			kTangent,
			kLookAt,
			kSpline
		};

		OrientationKeyProperties(CWnd* pParent = NULL);   // standard constructor

		void SetInterpolationType(InterpolationType type);
		InterpolationType GetInterpolationType() const;

		void SetTargets(const ROS::SceneEntityCollection& targets, ROS::ASceneEntity* currentTarget);
		ROS::ASceneEntity* GetTarget() const;

	// Dialog Data
		//{{AFX_DATA(OrientationKeyProperties)
	enum { IDD = IDD_ORIENTATION_KEY_PROPERTIES_DIALOG };
		CComboBox	mTargetComboBox;
		CButton		mLinearRadio;
		CButton		mSplineRadio;
		CButton		mTangentRadio;
		CButton		mLookAtRadio;
	//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(OrientationKeyProperties)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(OrientationKeyProperties)
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		afx_msg void OnSelchangeTargetComboBox();
		afx_msg void OnLinearRadio();
		afx_msg void OnSplineRadio();
		afx_msg void OnTangentRadio();
		afx_msg void OnLookAtRadio();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	private:
		InterpolationType			mInterpolationType;
		ROS::ASceneEntity*			mSelectedTarget;
		ROS::SceneEntityCollection	mTargets;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORIENTATIONKEYPROPERTIES_H__5B13F040_B437_11D2_968C_0040333267EF__INCLUDED_)
