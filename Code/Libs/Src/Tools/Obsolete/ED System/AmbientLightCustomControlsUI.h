#if !defined(AFX_AMBIENTLIGHTCUSTOMCONTROLSUI_H__D71EDAC3_7AF7_11D2_823F_0000F4A24556__INCLUDED_)
#define AFX_AMBIENTLIGHTCUSTOMCONTROLSUI_H__D71EDAC3_7AF7_11D2_823F_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AmbientLightCustomControlsUI.h : header file
//
//---------------------------------------------------------------------------
#include "SceneEntityControls.h"
#include "resource.h"
//---------------------------------------------------------------------------
namespace ROS
{
class Color;
}
/////////////////////////////////////////////////////////////////////////////
// AmbientLightCustomControlsUI dialog

class AmbientLightCustomControlsUI: public TSceneEntityForm
{
	// Construction
	public:
		AmbientLightCustomControlsUI(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity);   // standard constructor

		virtual void UpdateForm();

	// Dialog Data
		//{{AFX_DATA(AmbientLightCustomControlsUI)
		enum { IDD = IDD_AMBIENT_LIGHT_CUSTOM_DIALOG };
		CSpinButtonCtrl	mIntensitySpin;
		CSpinButtonCtrl	mBlueSpin;
		CSpinButtonCtrl	mGreenSpin;
		CSpinButtonCtrl	mRedSpin;
		CStatic	mColorBitmap;
		CEdit	mRedEdit;
		CEdit	mGreenEdit;
		CEdit	mBlueEdit;
	//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(AmbientLightCustomControlsUI)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(AmbientLightCustomControlsUI)
		afx_msg void OnChangeRedEdit();
		afx_msg void OnChangeGreenEdit();
		afx_msg void OnChangeBlueEdit();
		afx_msg void OnDeltaposRedSpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDeltaposGreenSpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDeltaposBlueSpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDeltaposIntensitySpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnColorPaletteButton();
		afx_msg void OnPaint();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	private:
		typedef TSceneEntityForm	BaseClass;

		void UpdateColorBitmap(const ROS::Color& color);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AMBIENTLIGHTCUSTOMCONTROLSUI_H__D71EDAC3_7AF7_11D2_823F_0000F4A24556__INCLUDED_)
