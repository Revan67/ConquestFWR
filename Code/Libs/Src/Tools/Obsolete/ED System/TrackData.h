// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef _h_TrackData
#define _h_TrackData
// --------------------------------------------------------------------------
class TrackData
{
	public:
		TrackData(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber);

		ROS::ASceneEntity& GetSceneEntity() const;
		unsigned int GetTrackNumber() const;
		
		void InsertMarkerId(long markerId);

		void RemoveMarkerId(long markerId);
		void RemoveAllMarkerIds();

		unsigned int GetMarkerCount() const;
		long GetMarkerId(unsigned int idx) const;

		unsigned int AddChildTrack();
		void ResetNumChildTracks();
		unsigned int GetNumChildTracks() const;

	private:
		typedef std::vector<long>					MarkerIdCollection;
		typedef MarkerIdCollection::iterator		MarkerIdIterator;
		typedef MarkerIdCollection::const_iterator	MarkerIdConstIterator;

		ROS::ASceneEntity&	mSceneEntity;
		unsigned int		mTrackNumber;
		unsigned int		mNumChildTracks;
		MarkerIdCollection	mMarkerIds;		
};
// --------------------------------------------------------------------------
inline TrackData::TrackData(ROS::ASceneEntity& sceneEntity, unsigned int trackNumber)
:mSceneEntity(sceneEntity), mTrackNumber(trackNumber), mNumChildTracks(0)
{
}
// --------------------------------------------------------------------------
inline ROS::ASceneEntity& TrackData::GetSceneEntity() const
{
	return mSceneEntity;
}
// --------------------------------------------------------------------------
inline unsigned int TrackData::GetTrackNumber() const
{
	return mTrackNumber;
}
// --------------------------------------------------------------------------
inline void TrackData::InsertMarkerId(long markerId)
{
	mMarkerIds.push_back(markerId);
}
// --------------------------------------------------------------------------
inline void TrackData::RemoveMarkerId(long markerId)
{
	MarkerIdIterator			begin = mMarkerIds.begin();
	const MarkerIdConstIterator	end = mMarkerIds.end();

	while(begin != end)
	{	if(*begin == markerId)
		{	mMarkerIds.erase(begin);
			return;
		}

		++begin;
	}

	ASSERT(0);	// Specified marker id is not in the collection!
}
// --------------------------------------------------------------------------
inline void TrackData::RemoveAllMarkerIds()
{
	mMarkerIds.clear();
}
// --------------------------------------------------------------------------
inline unsigned int TrackData::GetMarkerCount() const
{
	return mMarkerIds.size();
}
// --------------------------------------------------------------------------
inline long TrackData::GetMarkerId(unsigned int idx) const
{
	ASSERT(idx < mMarkerIds.size());

	return mMarkerIds[idx];
}
// --------------------------------------------------------------------------
inline unsigned int TrackData::AddChildTrack()
{
	return ++mNumChildTracks;
}
// --------------------------------------------------------------------------
inline void TrackData::ResetNumChildTracks()
{
	mNumChildTracks = 0;
}
// --------------------------------------------------------------------------
inline unsigned int TrackData::GetNumChildTracks() const
{
	return mNumChildTracks;
}
// --------------------------------------------------------------------------
#endif