// TrackViewUI.cpp : implementation file
//
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "TrackViewUI.h"
#include "TrackController.h"
#include "ModelNS.h"
#include "Utils.h"
#include "ARole.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "MotionRole.h"
#include "LiveCameraRole.h"
#include "AudioRole.h"
#include "TrackData.h"
#include "MarkerData.h"
#include "KeyPropertiesUI.h"
#include "InterpolationKeyProperties.h"
#include "OrientationKeyProperties.h"
#include "DeformableKeyPropertiesUI.h"
#include "ConstSceneEntityStateAccessor.h"
#include "SceneEntityStateAccessor.h"
#include "ConstMotionStateAccessor.h"
#include "MotionStateAccessor.h"
#include "KeyPointOperations.h"
#include "ADynamicCamera.h"
#include "DeformableSceneEntity.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// --------------------------------------------------------------------------
const long	kSceneTrackId = -1;

const ROS::Color	kDefaultMarkerColor(1, 1, 1, 1);
const ROS::Color	kSplineMarkerColor(0, 1, 1, 1);

const unsigned int	kMaxCommandID = 0xDFFF;

// --------------------------------------------------------------------------
enum PopupIndex
{
	kKeyPointPopupMenu,
	kLiveCameraPopupMenuIndex,
	kTrackPopupMenuIndex
};

const int kSelectRollingCameraIndex = 0;	// This should correspond to the index of the position of the 
											// "Switch To Camera" menu item in the "Live Camera Track" menu in the resource file.
const int kMaxChildTracksPerRole = 256;		// Max child tracks a role can have 
const int kMaxRoleTracksPerScene = 32;		// Max child role tracks an entity can have
/////////////////////////////////////////////////////////////////////////////
// TTrackViewUIForm dialog
TTrackViewUIForm::TTrackViewUIForm(CWnd* pParent, ROS::SceneModel& sceneModel)
: EdTimelineDlg(pParent), mNewMarkerId(1), mNewTrackId(1), mFocussedMarkerId(0)
, mFocussedTrackId(0), mMarkerStartTime(0), mMinCommandID(kMaxCommandID)
{
	Create(TTrackViewUIForm::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	mPopupMenus.LoadMenu(IDR_TRACK_VIEW_POPUP_MENU);

    TrackController::UpdateCB updateCB = makeFunctor((TrackController::UpdateCB*)0, *this, &TTrackViewUIForm::UpdateGUI);

	mTrackControllerP = AggAPointer<TrackController>(new TrackController(sceneModel, updateCB));

#if 1
	// From TTrackPanelUIForm
	SetStartTime(0);
	SetLength(100.0);
#endif

    UpdateGUI(ModelNS::kAll);
	positionControls();

	//{{AFX_DATA_INIT(TTrackViewUIForm)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
// --------------------------------------------------------------------------
TTrackViewUIForm::~TTrackViewUIForm()
{
	DestroyWindow();
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::DoDataExchange(CDataExchange* pDX)
{
	EdTimelineDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TTrackViewUIForm)
	//}}AFX_DATA_MAP
}
// --------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TTrackViewUIForm, EdTimelineDlg)
	//{{AFX_MSG_MAP(TTrackViewUIForm)
	ON_WM_DESTROY()
	ON_COMMAND(ID_TRACK_VIEW_DELETE, OnTrackViewDelete)
	ON_COMMAND(ID_TRACK_VIEW_PROPERTIES, OnTrackViewProperties)
	ON_COMMAND(ID_TRACK_VIEW_SET_SCENE_TIME_TO_MARKER_TIME, OnTrackViewSetTimeToMarkerTime)
	ON_COMMAND(ID_TRACK_VIEW_ADD_MARKER_AT_CURRENT_TIME, OnTrackViewAddMarkerAtCurrentTime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateGUI(int updateID)
{
    if(updateID == ModelNS::kAll || updateID == ModelNS::kEntitySelectionChanged || updateID == ModelNS::kEntityAdded || updateID == ModelNS::kSelectedEntityRemoved)
    {
		if(IsNotNull(mTrackControllerP))
        {
			if(mTrackControllerP->IsScenePresent())
            {
				if(updateID == ModelNS::kAll)
				{
					UpdateTracksForSceneDuration();
					UpdateTracksForCurrentTimePointUpdate();
				}

				if(updateID == ModelNS::kAll || updateID == ModelNS::kEntityAdded || updateID == ModelNS::kSelectedEntityRemoved)
                {
					UpdateTracks();
                }
            }
        }

        if(updateID == ModelNS::kAll || updateID == ModelNS::kEntitySelectionChanged)
        {
			UpdateTrackSelection();
        }
    }
    else if(updateID == ModelNS::kSelectedEntityUpdated || updateID == ModelNS::kSecondaryEntityUpdated || updateID == ModelNS::kSecondaryDependentEntityUpdated)
    {
		ROS::ASceneEntity* sceneEntity;
	
		if(updateID == ModelNS::kSelectedEntityUpdated)
		{
			sceneEntity = mTrackControllerP->GetSelectedSceneEntity();
		}
		else
		{
			ASSERT(updateID == ModelNS::kSecondaryEntityUpdated || updateID == ModelNS::kSecondaryDependentEntityUpdated);
			sceneEntity = mTrackControllerP->GetSecondarySceneEntity();
		}

		ASSERT(sceneEntity);

        SetTrackName(GetTrackId(*sceneEntity, 0), sceneEntity->GetConstSceneEntityStateAccessor()->GetName().c_str());

    	UpdateTrackForSceneEntity(*sceneEntity);
    }
    else if(updateID == ModelNS::kSceneDurationUpdated)
    {
		UpdateTracksForSceneDuration();
    }
    else if(updateID == ModelNS::kSceneCurrentTimePointUpdated)
    {
		UpdateTracksForCurrentTimePointUpdate();
    }
}
//---------------------------------------------------------------------------
CString TTrackViewUIForm::GetTrackName(const ROS::ASceneEntity& sceneEntity, unsigned int trackNumber) const
{
	if(trackNumber == 0)
	{
		return sceneEntity.GetConstSceneEntityStateAccessor()->GetName().c_str();
	}
	else
	{
//		ASSERT(sceneEntity.GetConstSceneEntityStateAccessor()->GetRoleCount() > (trackNumber - 1));

		return sceneEntity.GetConstSceneEntityStateAccessor()->GetRole(trackNumber - 1).GetName().c_str();
	}
}
//---------------------------------------------------------------------------
const ROS::ARole& TTrackViewUIForm::GetRole(const ROS::ASceneEntity& sceneEntity, unsigned int trackNumber) const
{
	ASSERT(trackNumber > 0 && sceneEntity.GetConstSceneEntityStateAccessor()->GetRoleCount() > (trackNumber - 1));

	return sceneEntity.GetConstSceneEntityStateAccessor()->GetRole(trackNumber - 1);
}
//---------------------------------------------------------------------------
ROS::ARole& TTrackViewUIForm::GetRole(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber)
{
	ASSERT(trackNumber > 0 && sceneEntity.GetConstSceneEntityStateAccessor()->GetRoleCount() > (trackNumber - 1));

	return sceneEntity.GetSceneEntityStateAccessor()->GetRole(trackNumber - 1);
}
//---------------------------------------------------------------------------
long TTrackViewUIForm::GetParentTrackId(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber)
{
	if(trackNumber == 0)
	{
		return kSceneTrackId;
	}
	else
	{
		ASSERT(sceneEntity.GetConstSceneEntityStateAccessor()->GetRoleCount() >= (trackNumber - 1));

		return GetTrackId(sceneEntity, 0);
	}
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::AddTrack(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber)
{
	long			trackId = GetTrackId(sceneEntity, trackNumber);
	long			parentTrackId = GetParentTrackId(sceneEntity, trackNumber);
	ROS::ROSString	trackName = GetTrackName(sceneEntity, trackNumber);

	TrackData*	trackData = new TrackData(sceneEntity, trackNumber);

	EdTimelineDlg::AddTrack(trackId, parentTrackId);
	SetTrackName(trackId, trackName.c_str());
	SetTrackData(trackId, reinterpret_cast<DWORD>(trackData));

	mTrackIds.push_back(trackId);

	UpdateTrack(sceneEntity, trackNumber);

}
//---------------------------------------------------------------------------
void TTrackViewUIForm::RemoveAllMarkers(long trackId)
{
	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	const unsigned int	markerCount = trackData->GetMarkerCount();

	for(unsigned int idx = 0; idx < markerCount; ++idx)
	{
		const long	markerId = trackData->GetMarkerId(idx);

		DWORD	markerDataUnknown = NULL;

		GetMarkerData(markerId, &markerDataUnknown);
		
		if (markerDataUnknown)
		{
//			ASSERT(markerDataUnknown);

			MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);

			delete markerData;

			SetMarkerData(markerId, NULL);
		}
	}

	DelAllTrackMarkers(trackId);

	trackData->RemoveAllMarkerIds();
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::DeleteAllChildTracks(long trackId)
{
	// This function just deletes the fake child tracks, no data is deleted
	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	const unsigned int	childTrackCount = trackData->GetNumChildTracks();

	for(unsigned int idx = 1; idx <= childTrackCount; ++idx)
	{
		const long	childTrackId = trackId + idx;
		EdTimelineDlg::DelTrack(childTrackId);
	}
	trackData->ResetNumChildTracks();
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::RemoveAllTracks()
{
	TrackIdCollection::iterator				begin = mTrackIds.begin();
	const TrackIdCollection::const_iterator	end = mTrackIds.end();

	while(begin != end)
	{
		DeleteAllChildTracks(*begin);
		RemoveAllMarkers(*begin);

		DWORD	trackDataUnknown = NULL;
		GetTrackData(*begin, &trackDataUnknown);
		ASSERT(trackDataUnknown);

		TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);
		delete trackData;

		SetTrackData(*begin, NULL);

		++begin;
	}

	DelAllTracks();

	mTrackIds.clear();
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTracks()
{
    if(IsNotNull(mTrackControllerP))
    {
		if(mTrackControllerP->IsScenePresent())
        {
			ROS::SceneEntityCollection  sceneEntities;

            mTrackControllerP->GetSceneEntities(sceneEntities);

			RemoveAllTracks();

			// Add the root for the whole scene
			EdTimelineDlg::AddTrack(kSceneTrackId, 0);
			SetTrackName(kSceneTrackId, "Scene");
			SetTrackData(kSceneTrackId, NULL);

			// Add all the scene entities
            ROS::SceneEntityCollection::const_iterator  begin = sceneEntities.begin();
            const ROS::SceneEntityCollection::const_iterator  end = sceneEntities.end();

            while(begin != end)
            {
				ROS::ASceneEntity*	sceneEntity = *begin;
				const unsigned int	roleCount = sceneEntity->GetSceneEntityStateAccessor()->GetRoleCount();

				if(roleCount > 0)
				{
	            	AddTrack(*sceneEntity, 0);

					for(unsigned int roleIdx = 0; roleIdx < roleCount; ++roleIdx)
					{
						AddTrack(*sceneEntity, roleIdx + 1);
					}
				}

                ++begin;
            }

#ifdef PORTED
            mTrackTreeView->FullExpand();
#endif

        }
    }
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTrackSelection()
{
#ifdef PORTED
    if(IsNotNull(mTrackControllerP))
    {
		const ROS::ASceneEntity* sceneEntity = mTrackControllerP->GetSelectedSceneEntity();

        if(sceneEntity)
        {
			int nodeCount = mTrackTreeView->Items->Count;
            int nodeIdx = 0;
            for(; nodeIdx < nodeCount; ++nodeIdx)
            {
				if(sceneEntity == mTrackTreeView->Items->Item[nodeIdx]->Data)
                {
					mTrackTreeView->Selected = mTrackTreeView->Items->Item[nodeIdx];
                	break;
                }
            }
            if(nodeIdx == nodeCount)
            {
				mTrackTreeView->Selected = NULL;
            }
        }
        else
		{
			mTrackTreeView->Selected = NULL;
        }

		mTrackPanel->SetTrackSelection(sceneEntity);
    }
#endif
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::AddMarker(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber, ROS::Time time, const ROS::Color& color)
{
	long	trackId = GetTrackId(sceneEntity, trackNumber);
	long	markerId = GetUniqueMarkerId();


	const ROS::ARole&			role = GetRole(sceneEntity, trackNumber);
	const ROS::AudioRole*		audioRole = dynamic_cast<const ROS::AudioRole*>(&role);
	const ROS::MotionRole*		mRole = dynamic_cast<const ROS::MotionRole*>(&role);

	DWORD	trackDataUnknown;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);
	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);


	// Adds markers to seperate tracks for audio roles
	if (audioRole)
	{
		// this is the parent audio track,  create a new child and move the marker (VISUALLY) to the child

		// determine the name of this track
		ROS::ROSString	childTrackName = role.GetName(time);
		int iPos = childTrackName.find_first_of(":",0);
		if (iPos) iPos+=2; // get rid of the ": "
		childTrackName = childTrackName.substr(iPos);
		unsigned int childTrackNumber = 0;
		unsigned int numChildTracks = trackData->GetNumChildTracks();
		long childTrackId = 0;
		
		for (int idx = 1; idx <= numChildTracks; idx++)
		{
			CString thisChildName;
			childTrackId = trackId + idx;
			EdTimelineDlg::GetTrackName(childTrackId, thisChildName);
			if (!strcmp(thisChildName, childTrackName.c_str()))
			{
				break;
			}
			childTrackId = 0;
		}
		
		if (!childTrackId)
		{
			childTrackNumber = trackData->AddChildTrack();
			childTrackId = trackId + childTrackNumber;
			EdTimelineDlg::AddTrack(childTrackId, trackId);
			SetTrackName(childTrackId, childTrackName.c_str());
			// give the child track the parent's data
			SetTrackData(childTrackId, trackDataUnknown);
		}
		// change trackId to child so marker gets (VISUALLY) added to the correct track
		trackId = childTrackId;
	}

	// Add the marker to the control
	MarkerData*	markerData = new MarkerData(sceneEntity, trackNumber, time);

	EdTimelineDlg::AddMarker(trackId, markerId);
	SetMarkerData(markerId, reinterpret_cast<DWORD>(markerData));
	SetMarkerPos(markerId, time.GetTime());

	const COLORREF	colorRef = RGB(color.GetRed() * 255, color.GetGreen() * 255, color.GetBlue() * 255);
	SetMarkerColor(markerId, colorRef);

	// Update the track data

	trackData->InsertMarkerId(markerId);
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTrack(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber)
{
	const long		trackId = GetTrackId(sceneEntity, trackNumber);
	const CString	trackName = GetTrackName(sceneEntity, trackNumber);

	SetTrackName(trackId, trackName);
#ifdef PORTED
    Hint = mName + AnsiString(" -- ") + aRole->GetName().c_str();
#endif
    RemoveAllMarkers(trackId);
	
	if(trackNumber > 0)
	{
		const ROS::ARole&			role = GetRole(sceneEntity, trackNumber);
		const ROS::LocationRole*	lRole = dynamic_cast<const ROS::LocationRole*>(&role);
		const ROS::AudioRole*		audioRole = dynamic_cast<const ROS::AudioRole*>(&role);

		if (audioRole)
		{
			DeleteAllChildTracks(trackId);
		}
		const unsigned int	timePointCount = role.CountTimePoints();

		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			ROS::Time	time = role.GetTime(timePointIdx);

			ROS::Color	color = kDefaultMarkerColor;

			if(lRole)
			{
				const ROS::FlaggedLocation::InterpolationType	type = lRole->GetState(timePointIdx).GetInterpolationType();
				if(type == ROS::FlaggedLocation::kSplineFixed || type == ROS::FlaggedLocation::kSplineBlend)
				{
					color = kSplineMarkerColor;
				}
			}

			AddMarker(sceneEntity, trackNumber, time, color);
		}
	}
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTrackForSceneEntity(ROS::ASceneEntity& sceneEntity)
{
	const unsigned int	roleCount = sceneEntity.GetSceneEntityStateAccessor()->GetRoleCount();
	
	for(unsigned int roleIdx = 0; roleIdx < roleCount; ++roleIdx)
    {	
		UpdateTrack(sceneEntity, roleIdx + 1);
	}    
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTracksForSceneDuration()
{
	SetStopTime(mTrackControllerP->GetSceneDuration().GetTime());
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::UpdateTracksForCurrentTimePointUpdate()
{
    SetCursorPos(mTrackControllerP->GetCurrentSceneTime().GetTime());
}
#ifdef PORTED
//---------------------------------------------------------------------------
void __fastcall TTrackViewUIForm::TrackTreeViewClick(TObject *Sender)
{
    if(mTrackTreeView->Selected)
    {
		ROS::ASceneEntity* sceneEntity = GetSceneEntity(mTrackTreeView->Selected);
    	if(sceneEntity)
        {
			mTrackControllerP->SetSelectedSceneEntity(sceneEntity);
        }
    }
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* TTrackViewUIForm::GetSceneEntity(long trackId)
{
	if(node)
    {
		return dynamic_cast<ROS::ASceneEntity*>(reinterpret_cast<ROS::ASceneEntity*>(node->Data));
    }

    return NULL;
}
#endif
//---------------------------------------------------------------------------
long TTrackViewUIForm::GetTrackId(ROS::ASceneEntity& sceneEntity, unsigned int trackNum)
{
	long trackId = sceneEntity.GetSceneEntityStateAccessor()->GetTrackId();
	if (!trackId)
	{
		trackId = GetUniqueTrackId();
		sceneEntity.GetSceneEntityStateAccessor()->SetTrackId(trackId);
	}

	return trackId + trackNum*kMaxChildTracksPerRole;
}
//---------------------------------------------------------------------------
long TTrackViewUIForm::GetUniqueTrackId()
{
	long returnVal = mNewTrackId;
	
	mNewTrackId+= (kMaxChildTracksPerRole * kMaxRoleTracksPerScene);

	return returnVal;
}
//---------------------------------------------------------------------------
long TTrackViewUIForm::GetUniqueMarkerId()
{
	return mNewMarkerId++;
}
/////////////////////////////////////////////////////////////////////////////
// TTrackViewUIForm message handlers
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnCancel()
{
	ShowWindow(SW_HIDE);
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnOK()
{
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::PostNcDestroy() 
{
	delete this;

//	CDialog::PostNcDestroy();
	EdTimelineDlg::PostNcDestroy();
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnCursorChanged(float newPos) 
{
	float	currTime = newPos;

    if(currTime < 0)
    {
		currTime = 0;
    }
    else
	{
		const float	sceneDuration = mTrackControllerP->GetSceneDuration().GetTime();

		if(currTime > sceneDuration)
		{
			currTime = sceneDuration;
		}
	}

	mTrackControllerP->SetCurrentSceneTime(ROS::Time(currTime));

	if(currTime != newPos)
	{	
		SetCursorPos(currTime);
	}
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnMarkerChangeStarted(long markerId) 
{
	float	floatCurrMarkerTime;
	
	GetMarkerPos(markerId, &floatCurrMarkerTime);

	mMarkerStartTime.SetTime(floatCurrMarkerTime < 0 ? 0 : floatCurrMarkerTime);
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnMarkerChanged(long markerId) 
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(markerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);

	ROS::ARole&	role = GetRole(markerData->GetSceneEntity(), markerData->GetTrackNumber());

	float	floatCurrMarkerTime;
	
	GetMarkerPos(markerId, &floatCurrMarkerTime);

	float	properFloatCurrMarkerTime = floatCurrMarkerTime < 0 ? 0 : floatCurrMarkerTime;
	
	ROS::Time	currMarkerTime(properFloatCurrMarkerTime);

	if (role.HasTime(currMarkerTime))
	{
		// role already has another marker. Reset this marker to correspond
		SetMarkerPos(markerId, markerData->GetTime().GetTime());
	}
	else
	{
		try
		{
			role.ChangeTime(markerData->GetTime(), currMarkerTime);
			markerData->SetTime(currMarkerTime);
		}
		catch(...)
		{
			// role refused to change! Reset marker to correspond
			SetMarkerPos(markerId, markerData->GetTime().GetTime());
		}
	}
	if(floatCurrMarkerTime != properFloatCurrMarkerTime)
	{
		SetMarkerPos(markerId, properFloatCurrMarkerTime);
	}
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnMarkerChangeFinished(long markerId) 
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(markerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);

	mTrackControllerP->AddUndoableOperation(new KeyPointPositionChange
												(
													markerData->GetSceneEntity()
													, markerData->GetTrackNumber() - 1	// Subtract one to account for the track that represents the entity itself
													, mMarkerStartTime
													, markerData->GetTime()
													, *mTrackControllerP
												)
											);
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackLabelContextRequest(long trackId, long mouseX, long mouseY)
{
	if(trackId == kSceneTrackId || trackId == 0)
	{
		return;
	}
	
	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	const unsigned int	trackNum = trackData->GetTrackNumber();

	if(trackNum > 0)
	{
		ROS::ASceneEntity*		entity = &trackData->GetSceneEntity();
		ROS::ARole*				aRole = &entity->GetSceneEntityStateAccessor()->GetRole(trackNum - 1); // Subtracting 1 to compensate for blank track for entity itself
		ROS::LiveCameraRole*	lRole = dynamic_cast<ROS::LiveCameraRole*>(aRole);

		if(lRole)
		{
			// Put up popup menu with all the appropriate cameras 
			CMenu* popupMenu = GetLiveCameraTrackPopupMenu(trackId);
			
			ASSERT(popupMenu);

			if(popupMenu)
			{
				RECT	rect;
				GetWindowRect(&rect);

				::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, rect.left + mouseX, rect.top + mouseY, 0, m_hWnd, NULL);	
			}
		}
	}
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackContextRequest(long trackId, float time, long mouseX, long mouseY)
{
	if(trackId == kSceneTrackId || trackId == 0)
	{
		return;
	}
	
	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	CMenu*	popupMenu = mPopupMenus.GetSubMenu(kTrackPopupMenuIndex);

	ASSERT(popupMenu);

    if(popupMenu)
    {
		mFocussedTrackId = trackId;

		RECT	rect;
		GetWindowRect(&rect);

        ::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, rect.left + mouseX, rect.top + mouseY, 0, m_hWnd, NULL);	
    }
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnMarkerContextRequest(long markerId, long mouseX, long mouseY) 
{
	CMenu*	popupMenu = mPopupMenus.GetSubMenu(kKeyPointPopupMenu);

	ASSERT(popupMenu);

    if(popupMenu)
    {
		mFocussedMarkerId = markerId;

		RECT	rect;
		GetWindowRect(&rect);

        ::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, rect.left + mouseX, rect.top + mouseY, 0, m_hWnd, NULL);	
    }
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnMarkerTipRequest(long markerId, long mouseX, long mouseY, char* tip)
{
	if(GetKeyState(VK_SHIFT) & 0x8000)
	{
		float	time;
		
		GetMarkerPos(markerId, &time);

		wsprintf(tip, "%d.%02d", (int) time, (int) ((time - ((int) time)) * 100));
	}
	else
	{
		const ROS::ROSString	name = GetMarkerName(markerId);

		if(name.length() <= TIP_STRING_LENGTH)
		{
			strcpy(tip, name.c_str());
		}
		else
		{
			memcpy(tip, name.c_str(), TIP_STRING_LENGTH);
			
			tip[TIP_STRING_LENGTH] = 0;
		}
	}
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnDestroy() 
{
	RemoveAllTracks();

//	CDialog::OnDestroy();
	EdTimelineDlg::OnDestroy();
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::RemoveMarker(long markerId)
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);

	// Remove marker id from track data
	const long	trackId = GetTrackId(markerData->GetSceneEntity(), markerData->GetTrackNumber());

	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	trackData->RemoveMarkerId(markerId);

	// delete marker data and remove marker from control
	delete	markerData;
	SetMarkerData(mFocussedMarkerId, NULL);
	DelMarker(mFocussedMarkerId);
}
// --------------------------------------------------------------------------
ROS::ROSString TTrackViewUIForm::GetMarkerName(long markerId) const
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(markerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time	currTime = markerData->GetTime();

	const ROS::ASceneEntity&	sceneEntity = markerData->GetSceneEntity();
	const ROS::ARole&			role = GetRole(sceneEntity, markerData->GetTrackNumber());

	return role.GetName(currTime);
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackViewDelete() 
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*	markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time	currTime = markerData->GetTime();

	ROS::ASceneEntity&	sceneEntity = markerData->GetSceneEntity();
	ROS::ARole&			role = GetRole(sceneEntity, markerData->GetTrackNumber());

	role.Remove(currTime);

	RemoveMarker(mFocussedMarkerId);

	mFocussedMarkerId = 0;

	sceneEntity.GetSceneEntityStateAccessor()->RoleUpdated();

	mTrackControllerP->LockSceneEntitySelection(false);
	mTrackControllerP->SetSelectedSceneEntity(NULL);

	mTrackControllerP->SetSecondarySceneEntity(&sceneEntity);
	mTrackControllerP->SecondarySceneEntityUpdated();
	mTrackControllerP->SetSecondarySceneEntity(NULL);
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackViewProperties() 
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*					markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time				currTime = markerData->GetTime();
	ROS::ASceneEntity*			sceneEntity = &(markerData->GetSceneEntity());
	ROS::ARole*					aRole = &GetRole(*sceneEntity, markerData->GetTrackNumber());
	
	ROS::MotionRole*			motRole = dynamic_cast<ROS::MotionRole*>(aRole);
	ROS::LocationRole*			lRole = dynamic_cast<ROS::LocationRole*>(aRole);
	ROS::OrientationRole*		oRole = dynamic_cast<ROS::OrientationRole*>(aRole);

	if(motRole)
    {
		OnMotionTrackMarkerProperties();
    }
	else if(lRole)
	{
		OnLocationTrackMarkerProperties();
	}
	else if(oRole)
	{
		OnOrientationTrackMarkerProperties();
	}
	
	mFocussedMarkerId = 0;
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackViewSetTimeToMarkerTime() 
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*					markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time				currTime = markerData->GetTime();
	float						newTime = currTime.GetTime();
	OnCursorChanged(newTime);
	mFocussedMarkerId = 0;
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::OnTrackViewAddMarkerAtCurrentTime() 
{ 
	DWORD	trackDataUnknown = NULL;
	GetTrackData(mFocussedTrackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);
	TrackData*					trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	const unsigned int	trackNum = trackData->GetTrackNumber();

	if(trackNum > 0)
	{
		ROS::ASceneEntity*		entity = &trackData->GetSceneEntity();
		mTrackControllerP->SetSelectedSceneEntity(entity);
		entity = mTrackControllerP->GetSelectedSceneEntity();
		if (entity)
		{
			ROS::ARole*				aRole = &entity->GetSceneEntityStateAccessor()->GetRole(trackNum - 1); // Subtracting 1 to compensate for blank track for entity itself
			ROS::Color				color = kDefaultMarkerColor;
			ROS::Time				time = mTrackControllerP->GetCurrentSceneTime();
			if (! aRole->HasTime(time))
			{
				ROS::MotionRole*		mRole = dynamic_cast<ROS::MotionRole*>(aRole);
				ROS::AudioRole*			audioRole = dynamic_cast<ROS::AudioRole*>(aRole);
				ROS::AStateRole*		sRole = dynamic_cast<ROS::AStateRole*>(aRole);
				if (mRole)
				{
					MessageBox("Adding motion markers not currently implemented.","Unimplemented");
				}
				else if (audioRole)
				{
					MessageBox("Adding audio markers not currently implemented.","Unimplemented");
				}
				else if(sRole)
				{
					sRole->GenerateState(time);
					mTrackControllerP->SceneUpdated();
				}
			}
		}
	}

	mFocussedTrackId = 0;
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::OnMotionTrackMarkerProperties()
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*					markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time				currTime = markerData->GetTime();
	ROS::ASceneEntity*			sceneEntity = &(markerData->GetSceneEntity());
	ROS::ARole*					aRole = &GetRole(*sceneEntity, markerData->GetTrackNumber());
	ROS::MotionRole*			motRole = dynamic_cast<ROS::MotionRole*>(aRole);
	ASSERT(motRole);

	ROS::MotionState	motionState = motRole->GetState(currTime);
	const ROS::Time		startTime = motionState.GetStartTime();
	const ROS::Time		transitionTime = motionState.GetTransitionTime();

    TDeformableKeyPropertiesUIForm	propertiesUI(this);

	propertiesUI.SetTrackTime(currTime);
	propertiesUI.SetStartTime(startTime);
	propertiesUI.SetTransitionTime(transitionTime);

	bool enableIK = false;

	if(motionState.GetMotionEvent() == ROS::MotionState::kStartIK)
	{
		ROS::DeformableSceneEntity * deform = dynamic_cast<ROS::DeformableSceneEntity*>(sceneEntity);
		if(deform)
		{
			enableIK = true;

			const ROS::IKState	iKState = deform->GetConstMotionStateAccessor()->GetIKState(currTime);

			propertiesUI.SetDampingFactor(iKState.GetDampingFactor());

			TDeformableKeyPropertiesUIForm::Axis	axis;
			TDeformableKeyPropertiesUIForm::Axis	upAxis;

			switch(iKState.GetEndEffectorAxis())
			{
				case ROS::IKState::kXAxis:
					axis = TDeformableKeyPropertiesUIForm::kXAxis;
					break;

				case ROS::IKState::kYAxis:
					axis = TDeformableKeyPropertiesUIForm::kYAxis;
					break;

				case ROS::IKState::kZAxis:
					axis = TDeformableKeyPropertiesUIForm::kZAxis;
					break;
			
				case ROS::IKState::kNXAxis:
					axis = TDeformableKeyPropertiesUIForm::kNXAxis;
					break;

				case ROS::IKState::kNYAxis:
					axis = TDeformableKeyPropertiesUIForm::kNYAxis;
					break;

				case ROS::IKState::kNZAxis:
					axis = TDeformableKeyPropertiesUIForm::kNZAxis;
					break;
			
				default:
					ASSERT(0 && "Unknown case");
					axis = TDeformableKeyPropertiesUIForm::kZAxis;
			}

			switch(iKState.GetEndEffectorUpAxis())
			{
				case ROS::IKState::kXAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kXAxis;
					break;

				case ROS::IKState::kYAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kYAxis;
					break;

				case ROS::IKState::kZAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kZAxis;
					break;
			
				case ROS::IKState::kNXAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kNXAxis;
					break;

				case ROS::IKState::kNYAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kNYAxis;
					break;

				case ROS::IKState::kNZAxis:
					upAxis = TDeformableKeyPropertiesUIForm::kNZAxis;
					break;
			
				default:
					ASSERT(0 && "Unknown IK up case");
					upAxis = TDeformableKeyPropertiesUIForm::kYAxis;
			}

			propertiesUI.SetAxis(axis);
			propertiesUI.SetUpAxis(upAxis);
			propertiesUI.SetMoveToFlag(iKState.GetMoveToFlag());
			propertiesUI.SetPointAtFlag(iKState.GetPointAtFlag());
		}
	}

	propertiesUI.EnableIKProperties(enableIK);

	if(propertiesUI.DoModal() == IDOK)
	{
		const ROS::Time	newTrackTime = propertiesUI.GetTrackTime();

		// Start time changes
		if(newTrackTime != currTime)
		{
			try
			{
				motRole->ChangeTime(currTime, newTrackTime);
			}
			catch(...)
			{
				// Failed to change the time in the role!
				mFocussedMarkerId = 0;
				return;
			}
			
			markerData->SetTime(newTrackTime);
			SetMarkerPos(mFocussedMarkerId, newTrackTime.GetTime());
		}

		bool	changed = false;
		// Start time changes
		const ROS::Time	newStartTime = propertiesUI.GetStartTime();

		if(motionState.GetStartTime() != newStartTime)
		{
			motionState.SetStartTime(newStartTime);
			changed = true;
		}

		// Transition time changes
		const ROS::Time	newTransitionTime = propertiesUI.GetTransitionTime();

		if(motionState.GetTransitionTime() != newTransitionTime)
		{
			motionState.SetTransitionTime(newTransitionTime);
			changed = true;
		}

		// IK properties change
		if(enableIK)
		{
			ROS::DeformableSceneEntity* deformableSE = dynamic_cast<ROS::DeformableSceneEntity*>(sceneEntity);

			if(deformableSE)
			{
				const float		newDampingFactor = propertiesUI.GetDampingFactor();
				ROS::IKState	iKState = deformableSE->GetConstMotionStateAccessor()->GetIKState(newTrackTime);
				bool			iKStateUpdated = false;

				if(iKState.GetDampingFactor() != newDampingFactor)
				{
					iKState.SetDampingFactor(newDampingFactor);
					iKStateUpdated = true;
				}

				const TDeformableKeyPropertiesUIForm::Axis	axis = propertiesUI.GetAxis();
				ROS::IKState::Axis							newAxis;

				switch(axis)
				{
					case TDeformableKeyPropertiesUIForm::kXAxis:
						newAxis = ROS::IKState::kXAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kYAxis:
						newAxis = ROS::IKState::kYAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kZAxis:
						newAxis = ROS::IKState::kZAxis;
						break;
				
					case TDeformableKeyPropertiesUIForm::kNXAxis:
						newAxis = ROS::IKState::kNXAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kNYAxis:
						newAxis = ROS::IKState::kNYAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kNZAxis:
						newAxis = ROS::IKState::kNZAxis;
						break;
				
					default:
						ASSERT(0 && "Unknown case");
						newAxis = ROS::IKState::kZAxis;
				}

				if(iKState.GetEndEffectorAxis() != newAxis)
				{
					iKState.SetEndEffectorAxis(newAxis);
					iKStateUpdated = true;
				}

				const TDeformableKeyPropertiesUIForm::Axis	upAxis = propertiesUI.GetUpAxis();

				switch(upAxis)
				{
					case TDeformableKeyPropertiesUIForm::kXAxis:
						newAxis = ROS::IKState::kXAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kYAxis:
						newAxis = ROS::IKState::kYAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kZAxis:
						newAxis = ROS::IKState::kZAxis;
						break;
				
					case TDeformableKeyPropertiesUIForm::kNXAxis:
						newAxis = ROS::IKState::kNXAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kNYAxis:
						newAxis = ROS::IKState::kNYAxis;
						break;

					case TDeformableKeyPropertiesUIForm::kNZAxis:
						newAxis = ROS::IKState::kNZAxis;
						break;
				
					default:
						ASSERT(0 && "Unknown IK up case");
						newAxis = ROS::IKState::kYAxis;
				}

				if(iKState.GetEndEffectorUpAxis() != newAxis)
				{
					iKState.SetEndEffectorUpAxis(newAxis);
					iKStateUpdated = true;
				}

				bool newPointAt= false;
				bool newMoveTo = false;

				newPointAt = propertiesUI.GetPointAtFlag();
				newMoveTo = propertiesUI.GetMoveToFlag();

				if (newPointAt != iKState.GetPointAtFlag())
				{
					iKState.SetPointAtFlag(newPointAt);
					iKStateUpdated = true;
				}

				if (newMoveTo != iKState.GetMoveToFlag())
				{
					iKState.SetMoveToFlag(newMoveTo);
					iKStateUpdated = true;
				}

				if(iKStateUpdated)
				{
					deformableSE->GetMotionStateAccessor()->SetIKState(iKState, newTrackTime);
				}

				// *** Do we need to set changed anywhere here, so that StateUpdated() is called below?
			}
		}

		if(changed)
		{
			motRole->StateUpdated(motionState, newTrackTime);
		}
	}
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::OnLocationTrackMarkerProperties()
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*					markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time				currTime = markerData->GetTime();
	ROS::ASceneEntity*			sceneEntity = &(markerData->GetSceneEntity());
	ROS::ARole*					aRole = &GetRole(*sceneEntity, markerData->GetTrackNumber());
	ROS::LocationRole*			lRole = dynamic_cast<ROS::LocationRole*>(aRole);
	ASSERT(lRole);

	std::auto_ptr<InterpolationKeyProperties>		keyProperties(new InterpolationKeyProperties(this));
	InterpolationKeyProperties::InterpolationType	locationInterpolationType;

	// Initialize the dialog
	switch(lRole->GetState(currTime).GetInterpolationType())
	{
		case ROS::FlaggedLocation::kLinearFixed:
			locationInterpolationType = InterpolationKeyProperties::kLinearFixed;
			break;
		case ROS::FlaggedLocation::kSplineFixed:
			locationInterpolationType = InterpolationKeyProperties::kSplineFixed;
			break;
		case ROS::FlaggedLocation::kLinearBlend:
			locationInterpolationType = InterpolationKeyProperties::kLinearBlend;
			break;
		case ROS::FlaggedLocation::kSplineBlend:
			locationInterpolationType = InterpolationKeyProperties::kSplineBlend;
			break;
	}

	keyProperties->SetInterpolationType(locationInterpolationType);
	keyProperties->SetTime(currTime);

	// Show the dialog
	if(keyProperties->DoModal() == IDOK)
	{
		// Check for change
		bool	changed = false;
		
		const ROS::Time	newTime = keyProperties->GetTime();

    	if(newTime != currTime)
		{
			ROS::ARole&	role = GetRole(*sceneEntity, markerData->GetTrackNumber());

			try
			{
				role.ChangeTime(currTime, newTime);
			}
			catch(...)
			{
				// Failed to change the time in the role!
				mFocussedMarkerId = 0;
				return;
			}

			markerData->SetTime(newTime);

			SetMarkerPos(mFocussedMarkerId, newTime.GetTime());

			changed = true;
		}


		ROS::FlaggedLocation								location = lRole->GetState(newTime);
		const InterpolationKeyProperties::InterpolationType	newType = keyProperties->GetInterpolationType();

		if(newType != locationInterpolationType)
		{
			// Interpolation type changed!
			ROS::FlaggedLocation::InterpolationType	type;
			ROS::Color	color;

			switch(newType)
			{
				case InterpolationKeyProperties::kLinearFixed:
					type = ROS::FlaggedLocation::kLinearFixed;
					color = kDefaultMarkerColor;
					break;
				case InterpolationKeyProperties::kSplineFixed:
					type = ROS::FlaggedLocation::kSplineFixed;
					color = kSplineMarkerColor;
					break;
				case InterpolationKeyProperties::kLinearBlend:
					type = ROS::FlaggedLocation::kLinearBlend;
					color = kDefaultMarkerColor;
					break;
				case InterpolationKeyProperties::kSplineBlend:
					type = ROS::FlaggedLocation::kSplineBlend;
					color = kSplineMarkerColor;
					break;
			}

			location.SetInterpolationType(type);

			lRole->StateUpdated(location, newTime);
			
			SetMarkerColor(mFocussedMarkerId, RGB(color.GetRed() * 255, color.GetGreen() * 255, color.GetBlue() * 255));

			changed = true;
		}

		if(changed)
		{
			sceneEntity->GetSceneEntityStateAccessor()->RoleUpdated();
			mTrackControllerP->SetSecondarySceneEntity(sceneEntity);
			mTrackControllerP->SecondarySceneEntityUpdated();
			mTrackControllerP->SetSecondarySceneEntity(NULL);
		}
	}
}
//---------------------------------------------------------------------------
void TTrackViewUIForm::OnOrientationTrackMarkerProperties()
{
	DWORD	markerDataUnknown = NULL;

	GetMarkerData(mFocussedMarkerId, &markerDataUnknown);
	ASSERT(markerDataUnknown);

	MarkerData*					markerData = reinterpret_cast<MarkerData*>(markerDataUnknown);
	const ROS::Time				currTime = markerData->GetTime();
	ROS::ASceneEntity*			sceneEntity = &(markerData->GetSceneEntity());
	ROS::ARole*					aRole = &GetRole(*sceneEntity, markerData->GetTrackNumber());
	ROS::OrientationRole*		oRole = dynamic_cast<ROS::OrientationRole*>(aRole);
	ASSERT(oRole);

	std::auto_ptr<OrientationKeyProperties>			keyProperties(new OrientationKeyProperties(this));
	OrientationKeyProperties::InterpolationType		orientationInterpolationType;

	// Initialize dialog
	// Determine orientation interpolation type
	const ROS::FlaggedOrientation	orientation = oRole->GetState(currTime);

	switch(orientation.GetInterpolationType())
	{
		case ROS::FlaggedOrientation::kLinear:
			orientationInterpolationType = OrientationKeyProperties::kLinear;
			break;
		case ROS::FlaggedOrientation::kSpline:
			orientationInterpolationType = OrientationKeyProperties::kSpline;
			break;
		case ROS::FlaggedOrientation::kTangent:
			orientationInterpolationType = OrientationKeyProperties::kTangent;
			break;
		case ROS::FlaggedOrientation::kLookAt:
			orientationInterpolationType = OrientationKeyProperties::kLookAt;
			break;
		default:
			ASSERT(0);	// Unknown type!
	}

	keyProperties->SetInterpolationType(orientationInterpolationType);

	// Determine valid targets
	ROS::SceneEntityCollection	targets;

	mTrackControllerP->GetSceneEntities(targets);

	ROS::SceneEntityCollection::iterator		begin = targets.begin();
	const ROS::SceneEntityCollection::iterator	end = targets.end();

	while(begin != end)
	{
		ROS::ASceneEntity*	entity = *begin;

		if(sceneEntity != entity)
		{
			if(entity->IsPersistent())
			{
				if(dynamic_cast<ROS::AStaticSceneEntity*>(entity) != NULL)
				{
					++begin;
					continue;
				}
			}
		}

		// The entity is either not persistent or does not have position
		ROS::SceneEntityCollection::iterator	erase = begin;
		++begin;
		targets.erase(erase);
	}

	ROS::ASceneEntity*	currentSelection = orientation.GetInterpolationType() == ROS::FlaggedOrientation::kLookAt ? orientation.GetTargetEntity() : NULL;

	keyProperties->SetTargets(targets, currentSelection);

	keyProperties->SetTime(currTime);

	// Show dialog
	if(keyProperties->DoModal() == IDOK)
	{
		// Check for change
		bool	changed = false;
		
		const ROS::Time	newTime = keyProperties->GetTime();

    	if(newTime != currTime)
		{
			ROS::ARole&	role = GetRole(*sceneEntity, markerData->GetTrackNumber());

			try
			{
				role.ChangeTime(currTime, newTime);
			}
			catch(...)
			{
				// Failed to change the time in the role!
				mFocussedMarkerId = 0;
				return;
			}
			markerData->SetTime(newTime);

			SetMarkerPos(mFocussedMarkerId, newTime.GetTime());

			changed = true;
		}

		ROS::FlaggedOrientation								orientation = oRole->GetState(newTime);
		const OrientationKeyProperties::InterpolationType	newType = keyProperties->GetInterpolationType();
		
		if(newType != orientationInterpolationType)
		{
			// Interpolation type changed!
			ROS::FlaggedOrientation::InterpolationType	type;

			switch(newType)
			{
				case OrientationKeyProperties::kLinear:
					type = ROS::FlaggedOrientation::kLinear;
					break;
				case OrientationKeyProperties::kSpline:
					type = ROS::FlaggedOrientation::kSpline;
					break;
				case OrientationKeyProperties::kTangent:
					type = ROS::FlaggedOrientation::kTangent;
					break;
				case OrientationKeyProperties::kLookAt:
					type = ROS::FlaggedOrientation::kLookAt;
					break;
				default:
					ASSERT(0);	// Unknown type!
			}
			
			orientation.SetInterpolationType(type);

			oRole->StateUpdated(orientation, newTime);
			
			changed = true;
		}
		
		if(newType == OrientationKeyProperties::kLookAt)
		{
			// Check if the target has changed
			ROS::ASceneEntity*	newTarget = keyProperties->GetTarget();
			ASSERT(newTarget);

			if(orientation.GetTargetEntity() != newTarget)
			{
				ROS::AStaticSceneEntity*	sEntity = dynamic_cast<ROS::AStaticSceneEntity*>(newTarget);
				ASSERT(sEntity);

				orientation.SetTargetEntity(sEntity);

				oRole->StateUpdated(orientation, newTime);
				
				changed = true;
			}
		}

		if(changed)
		{
			sceneEntity->GetSceneEntityStateAccessor()->RoleUpdated();
			mTrackControllerP->SetSecondarySceneEntity(sceneEntity);
			mTrackControllerP->SecondarySceneEntityUpdated();
			mTrackControllerP->SetSecondarySceneEntity(NULL);
		}
	}
}
//---------------------------------------------------------------------------
CMenu* TTrackViewUIForm::GetLiveCameraTrackPopupMenu(unsigned int trackId)
{
	CameraCollection	cameras;

	GetCameraCollection(trackId, cameras);

	CMenu* popupMenu = mPopupMenus.GetSubMenu(kLiveCameraPopupMenuIndex);
	ASSERT(popupMenu);
	
	// Construct the camera list popup menu
	CMenu	cameraListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope
	cameraListPopupMenu.CreateMenu();

	mMinCommandID = kMaxCommandID;	// Start at the highest available command to avoid conflicts with other commands

	// Put in entries for all the cameras
    CameraCollection::const_iterator	begin = cameras.begin();
    CameraCollection::const_iterator	kEnd = cameras.end();
	unsigned int						cameraCount = 0;

    while(begin != kEnd)
    {
		UINT	flags = MF_STRING;
		
		++cameraCount;

		if(cameraCount % 25 == 0)
		{
			flags |= MF_MENUBARBREAK;
		}
		
		ROS::ADynamicCamera*	camera = (*begin);

		cameraListPopupMenu.AppendMenu(flags, mMinCommandID, camera->GetConstSceneEntityStateAccessor()->GetName().c_str());
		--mMinCommandID;

        ++begin;
    }

	if(cameras.empty())
	{
		// No cameras found!
		cameraListPopupMenu.AppendMenu(MF_STRING | MF_GRAYED, mMinCommandID, "<None>");
		--mMinCommandID;
	}

	CString	label;

	popupMenu->GetMenuString(kSelectRollingCameraIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kSelectRollingCameraIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(cameraListPopupMenu.m_hMenu), label);

	cameraListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	mFocussedTrackId = trackId;

	return popupMenu;
}
// --------------------------------------------------------------------------
BOOL TTrackViewUIForm::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	const int	iD = LOWORD(wParam);

	// Check for camera commands
	if(mMinCommandID <= iD && iD <= kMaxCommandID)
	{
		SetLiveCameraClick(kMaxCommandID - iD);
		
		return 1;
	}
	else
	{
		return EdTimelineDlg::OnCommand(wParam, lParam);
	}
}
// --------------------------------------------------------------------------
void TTrackViewUIForm::GetCameraCollection(unsigned int trackId, CameraCollection& cameras) const
{	
	ROS::ADynamicCamera*	cameraToExclude = NULL;
	ROS::ADynamicCamera*	currentCamera = NULL;

	DWORD	trackDataUnknown = NULL;
	GetTrackData(trackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	const TrackData*	trackData = reinterpret_cast<const TrackData*>(trackDataUnknown);

	const unsigned int	trackNum = trackData->GetTrackNumber();

	if(trackNum > 0)
	{
		ROS::ASceneEntity*		entity = &trackData->GetSceneEntity();
		ROS::ARole*				aRole = &entity->GetSceneEntityStateAccessor()->GetRole(trackNum - 1); // Subtracting 1 to compensate for blank track for entity itself
		ROS::LiveCameraRole*	lRole = dynamic_cast<ROS::LiveCameraRole*>(aRole);

		if(lRole)
		{
			// Put up popup menu with all the cameras in the scene, excluding the current camera.
			cameraToExclude = dynamic_cast<ROS::ADynamicCamera*>(entity);
			currentCamera = lRole->GetState(mTrackControllerP->GetCurrentSceneTime()).GetRollingCamera();
		}
	}

	// Put in entries for the appropriate cameras in the scene
	ROS::SceneEntityCollection   sceneEntityColl;

    mTrackControllerP->GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

    while(begin != kEnd)
    {
		ROS::ADynamicCamera*	camera = dynamic_cast<ROS::ADynamicCamera*>(*begin);

		if(camera && camera != currentCamera && camera != cameraToExclude)	// Counting on lazy evaluation
		{
			cameras.push_back(camera);
		}
        ++begin;
    }

}
// --------------------------------------------------------------------------
void TTrackViewUIForm::SetLiveCameraClick(unsigned int cameraIdx)
{
	CameraCollection	cameras;

	GetCameraCollection(mFocussedTrackId, cameras);
	
	ASSERT(cameraIdx < cameras.size());

	ROS::ADynamicCamera*	selectedCamera = cameras[cameraIdx];

	// Add marker to the live camera track
	DWORD	trackDataUnknown = NULL;
	GetTrackData(mFocussedTrackId, &trackDataUnknown);
	ASSERT(trackDataUnknown);

	TrackData*	trackData = reinterpret_cast<TrackData*>(trackDataUnknown);

	const unsigned int	trackNum = trackData->GetTrackNumber();
	ASSERT(trackNum > 0);

	ROS::ASceneEntity*		entity = &trackData->GetSceneEntity();
	ROS::ARole*				aRole = &entity->GetSceneEntityStateAccessor()->GetRole(trackNum - 1); // Subtracting 1 to compensate for blank track for entity itself
	ROS::LiveCameraRole*	lRole = dynamic_cast<ROS::LiveCameraRole*>(aRole);

	ASSERT(lRole);

	lRole->StateUpdated(ROS::LiveCameraState(selectedCamera), mTrackControllerP->GetCurrentSceneTime());

	entity->GetSceneEntityStateAccessor()->RoleUpdated();

	mTrackControllerP->SceneUpdated();
}
// --------------------------------------------------------------------------
