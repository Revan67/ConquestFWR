#if !defined(AFX_DEFORMABLEKEYPROPERTIESUI_H__4253AA41_51DC_11D2_968C_FA8CD1F91444__INCLUDED_)
#define AFX_DEFORMABLEKEYPROPERTIESUI_H__4253AA41_51DC_11D2_968C_FA8CD1F91444__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DeformableKeyPropertiesUI.h : header file
//
#include "TimeType.h"
/////////////////////////////////////////////////////////////////////////////
// TDeformableKeyPropertiesUIForm dialog

class TDeformableKeyPropertiesUIForm : public CDialog
{
	// Construction
	public:
		enum Axis
		{
			kXAxis,
			kYAxis,
			kZAxis,
			kNXAxis,
			kNYAxis,
			kNZAxis
		};

		TDeformableKeyPropertiesUIForm(CWnd* pParent = NULL);   // standard constructor

		ROS::Time GetTrackTime() const;
		ROS::Time GetStartTime() const;
		ROS::Time GetTransitionTime() const;
		float GetDampingFactor() const;
		Axis GetAxis() const;
		Axis GetUpAxis() const;
		bool GetPointAtFlag() const;
		bool GetMoveToFlag() const;

		void SetTrackTime(ROS::Time time);
		void SetStartTime(ROS::Time time);
		void SetTransitionTime(ROS::Time time);
		void SetDampingFactor (float factor);
		void SetAxis(Axis axis);
		void SetUpAxis(Axis axis);
		void SetPointAtFlag (bool pointAt);
		void SetMoveToFlag(bool moveTo);
		void EnableDampingFactor (bool enable = true);
		void EnableAxis(bool enable = true);
		void EnableIKProperties (bool enable = true);

		// Dialog Data
			//{{AFX_DATA(TDeformableKeyPropertiesUIForm)
	enum { IDD = IDD_DEFORMABLE_KEY_PROPERTIES_DIALOG };
	CButton	mNegateUp;
	CButton	mNegateFront;
		CButton	mXUpAxisRadio;
		CButton	mYUpAxisRadio;
		CButton	mZUpAxisRadio;
		CStatic	mUpAxisStatic;
		CStatic	mDampingStatic;
		CStatic	mAxisStatic;
		CButton	mPointAtBox;
		CButton	mMoveToBox;
		CButton	mXAxisRadio;
		CButton	mYAxisRadio;
		CButton	mZAxisRadio;
		CEdit	mDampingFactorEdit;
		CEdit	mTrackTimeEdit;
		CEdit	mStartTimeEdit;
		CEdit	mTransitionTimeEdit;
	//}}AFX_DATA


		// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(TDeformableKeyPropertiesUIForm)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(TDeformableKeyPropertiesUIForm)
		virtual void OnOK();
		virtual BOOL OnInitDialog();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	private:
		ROS::Time	mTrackTime;
		ROS::Time	mStartTime;
		ROS::Time	mTransitionTime;
		float       mDampingFactor;
		bool        mUseDamping;
		Axis		mAxis;
		Axis        mUpAxis;
		bool		mUseAxis;
		bool        mUseIK;
		bool        mMoveTo;
		bool        mPointAt;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEFORMABLEKEYPROPERTIESUI_H__4253AA41_51DC_11D2_968C_FA8CD1F91444__INCLUDED_)
