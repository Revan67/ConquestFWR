//{{AFX_INCLUDES()
//#include "timeline.h"
//}}AFX_INCLUDES
#if !defined(AFX_TRACKVIEWUI_H__15E9CF04_48C4_11D2_823E_0000F4A24556__INCLUDED_)
#define AFX_TRACKVIEWUI_H__15E9CF04_48C4_11D2_823E_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TrackViewUI.h : header file

#include "TrackController.h"
#include "Links.h"
#include "EdTimelineDlg.h"
#include "Color.h"

namespace ROS
{
class SceneModel;
class ARole;
class ADynamicCamera;
}

class CMenu;
/////////////////////////////////////////////////////////////////////////////
// TTrackViewUIForm dialog

class TTrackViewUIForm : public EdTimelineDlg
{
	// Construction
	public:
		TTrackViewUIForm(CWnd* pParent, ROS::SceneModel& sceneModel);   // standard constructor
		~TTrackViewUIForm();

	// Dialog Data
		//{{AFX_DATA(TTrackViewUIForm)
		enum { IDD = IDD_TIMELINE };
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(TTrackViewUIForm)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void PostNcDestroy();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	// Implementation
	protected:
		virtual void OnCancel();
		virtual void OnOK();

		// Generated message map functions
		//{{AFX_MSG(TTrackViewUIForm)
		afx_msg void OnDestroy();
		afx_msg void OnTrackViewDelete();
		afx_msg void OnTrackViewProperties();
		afx_msg void OnTrackViewSetTimeToMarkerTime();
		afx_msg void OnTrackViewAddMarkerAtCurrentTime();	
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

		// Overloaded message handling functions from EdTimelineDialog
		virtual void OnMarkerChangeStarted(long markerId);
		virtual void OnMarkerChanged(long markerId);
		virtual void OnMarkerChangeFinished(long markerId);
		virtual void OnCursorChanged(float newPos);
		virtual void OnTrackLabelContextRequest(long trackId, long mouseX, long mouseY);
		virtual void OnTrackContextRequest(long trackId, float time, long mouseX, long mouseY);
	 	virtual void OnMarkerContextRequest(long markerId, long mouseX, long mouseY);
		virtual void OnMarkerTipRequest(long markerId, long mouseX, long mouseY, char* tip);

	private:
		typedef std::vector<long>	TrackIdCollection;
		typedef std::vector<ROS::ADynamicCamera*> CameraCollection;

		void UpdateGUI(int updateID);
        void UpdateTracks();
		void UpdateTrack(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);
        void UpdateTrackSelection();
		void UpdateTrackForSceneEntity(ROS::ASceneEntity& sceneEntity);
		void UpdateTracksForSceneDuration();
        void UpdateTracksForCurrentTimePointUpdate();
                        
		void AddTrack(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);
		void AddMarker(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber, ROS::Time time, const ROS::Color& color);

		void RemoveAllTracks();
		void DeleteAllChildTracks(long trackId);
		void RemoveAllMarkers(long trackId);
		void RemoveMarker(long markerId);

		ROS::ROSString GetMarkerName(long markerId) const;

#ifdef PORTED
        ROS::ASceneEntity* GetSceneEntity(long trackId);
#endif
		long GetTrackId(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);
		long GetParentTrackId(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);

		const ROS::ARole& GetRole(const ROS::ASceneEntity& sceneEntity, unsigned int trackNumber) const;
		ROS::ARole& GetRole(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);

		CString GetTrackName(const ROS::ASceneEntity& sceneEntity, unsigned int trackNumber) const;

		long GetUniqueTrackId();
		long GetUniqueMarkerId();
		
		CMenu* GetLiveCameraTrackPopupMenu(unsigned int trackId);

		void SetLiveCameraClick(unsigned int cameraIdx);

		void GetCameraCollection(unsigned int trackId, CameraCollection& cameras) const;

		void OnMotionTrackMarkerProperties();
    	void OnLocationTrackMarkerProperties();
		void OnOrientationTrackMarkerProperties();

		/**#: [Cardinalities = "1..1/"]*/
		AggAPointer<TrackController>	mTrackControllerP;
		long							mNewMarkerId;
		long							mNewTrackId;
		TrackIdCollection				mTrackIds;
        CMenu							mPopupMenus;
		long							mFocussedMarkerId;
		unsigned int					mFocussedTrackId;
		ROS::Time						mMarkerStartTime;
		unsigned int					mMinCommandID;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACKVIEWUI_H__15E9CF04_48C4_11D2_823E_0000F4A24556__INCLUDED_)
