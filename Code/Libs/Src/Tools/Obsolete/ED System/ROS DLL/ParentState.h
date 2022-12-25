//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ParentState_h
#define ParentState_h
// --------------------------------------------------------------------------
#include <iostream>
#include "StringType.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ACompoundSceneEntity;
// --------------------------------------------------------------------------
class CPP_DECL ParentState
{
	public:
        enum ParentEventID
        {
			kAttachToParent,
            kDetachFromParent,
			kInternalParentEvent	// For internal use. 
        };

        ParentState();
		ParentState(ParentEventID parentEvent, const ROSString& childHardPointName, ACompoundSceneEntity* parentEntity, const ROSString& parentHardPointName);

        void SetEvent(ParentEventID parentEvent);
        void SetChildHardPointName(const ROSString& childHardPointName);
		void SetParentEntity(ACompoundSceneEntity* parentEntity);
        void SetParentHardPointName(const ROSString& parentHardPointName);
        void Set(ParentEventID parentEvent, const ROSString& childHardPointName, ACompoundSceneEntity* parentEntity, const ROSString& parentHardPointName);

        ParentEventID GetEvent() const;
        ROSString GetChildHardPointName() const;
		ACompoundSceneEntity* GetParentEntity() const;
        ROSString GetParentHardPointName() const;

        void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

    private:
        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

    	ParentEventID			mParentEventID;
		ROSString				mChildHardPointName;
		ACompoundSceneEntity*	mParentEntity;
		ROSString				mParentHardPointName;
};
// --------------------------------------------------------------------------
inline ParentState::ParentState()
: mParentEventID(kInternalParentEvent), mParentEntity(NULL)  
{
}
// --------------------------------------------------------------------------
inline ParentState::ParentState(ParentEventID parentEvent, const ROSString& childHardPointName, ACompoundSceneEntity* parentEntity, const ROSString& parentHardPointName)
: mParentEventID(parentEvent), mChildHardPointName(childHardPointName), mParentEntity(parentEntity), mParentHardPointName(parentHardPointName)
{
}
// --------------------------------------------------------------------------
inline void ParentState::SetEvent(ParentEventID parentEvent)
{
	mParentEventID = parentEvent;
}
// --------------------------------------------------------------------------
inline void ParentState::SetChildHardPointName(const ROSString& childHardPointName)
{
	mChildHardPointName = childHardPointName;
}
// --------------------------------------------------------------------------
inline void ParentState::SetParentEntity(ACompoundSceneEntity* parentEntity)
{
	mParentEntity = parentEntity;
}
// --------------------------------------------------------------------------
inline void ParentState::SetParentHardPointName(const ROSString& parentHardPointName)
{
	mParentHardPointName = parentHardPointName;
}
// --------------------------------------------------------------------------
inline void ParentState::Set(ParentEventID parentEvent, const ROSString& childHardPointName, ACompoundSceneEntity* parentEntity, const ROSString& parentHardPointName)
{
    SetEvent(parentEvent);
	SetChildHardPointName(childHardPointName);
	SetParentEntity(parentEntity);
    SetParentHardPointName(parentHardPointName);
}
// --------------------------------------------------------------------------
inline ParentState::ParentEventID ParentState::GetEvent() const
{
	return mParentEventID;
}
// --------------------------------------------------------------------------
inline ROSString ParentState::GetChildHardPointName() const
{
	return mChildHardPointName;
}
// --------------------------------------------------------------------------
inline ACompoundSceneEntity* ParentState::GetParentEntity() const
{
	return mParentEntity;
}
// --------------------------------------------------------------------------
inline ROSString ParentState::GetParentHardPointName() const
{
	return mParentHardPointName;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::ParentState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::ParentState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif