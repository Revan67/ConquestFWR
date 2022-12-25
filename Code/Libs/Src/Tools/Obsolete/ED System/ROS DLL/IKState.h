// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef IKState_h
#define IKState_h
//---------------------------------------------------------------------------
#include "StringType.h"
//---------------------------------------------------------------------------
namespace ROS
{
class AStaticSceneEntity;
//---------------------------------------------------------------------------
class IKState
{
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

		IKState()
		: mCountToRootEffector(0), mTargetEntity(NULL), mDampingFactor(1), mEndEffectorAxis(kZAxis)
		{
		}

		IKState(const ROSString& endEffectorName, unsigned int countToRootEffector, const AStaticSceneEntity& targetEntity)
		: mEndEffectorName(endEffectorName), mCountToRootEffector(countToRootEffector)
		, mTargetEntity(&targetEntity), mDampingFactor(1), mEndEffectorAxis(kZAxis)
		, mPointAt(false), mMoveTo(true), mEndEffectorUpAxis(kYAxis)
		{
		}

		ROSString GetEndEffectorName() const
		{
			ASSERT(mTargetEntity);

			return mEndEffectorName;
		}

		unsigned int GetCountToRootEffector() const
		{
			ASSERT(mTargetEntity);

			return mCountToRootEffector;
		}

		const AStaticSceneEntity& GetTargetEntity() const
		{
			ASSERT(mTargetEntity);

			return *mTargetEntity;
		}

		const float GetDampingFactor() const
		{
			ASSERT(mTargetEntity);

			return mDampingFactor;
		}

		Axis GetEndEffectorAxis() const
		{
			return mEndEffectorAxis;
		}

		Axis GetEndEffectorUpAxis() const
		{
			return mEndEffectorUpAxis;
		}

		bool GetPointAtFlag () const
		{
			return mPointAt;
		}

		bool GetMoveToFlag () const
		{
			return mMoveTo;
		}

		void SetPointAtFlag (bool pointAt)
		{
			mPointAt = pointAt;
		}

		void SetMoveToFlag (bool moveTo)
		{
			mMoveTo = moveTo;
		}

		void SetDampingFactor(float factor)
		{
			ASSERT(mTargetEntity);

			mDampingFactor = factor;
		}

		void SetEndEffectorAxis(Axis axis)
		{
			mEndEffectorAxis = axis;
		}

		void SetEndEffectorUpAxis(Axis axis)
		{
			mEndEffectorUpAxis = axis;
		}

        void Write(std::ostream& oStream) const
		{
			ASSERT(mTargetEntity);

			WriteSubObject(oStream);
		}

		void Read(std::istream& iStream)
		{
			ReadSubObject(iStream);
		}

        void WriteSubObject(std::ostream& oStream) const;

        void ReadSubObject(std::istream& iStream);

	private:
		enum FieldID
		{
			kEndEffectorName,
			kCountToRoot,
			kTargetEntityName,
			kDampingFactor,
			kEndEffectorAxis,	// Added in ROS 3.1.0.0
			kPointAtFlag,       // Added in ROS 3.1.1.0
			kMoveToFlag,        // Added in ROS 3.1.1.0
			kEndEffectorUpAxis  // Added in ROS 3.1.1.0
		};

		ROSString					mEndEffectorName;
		unsigned int				mCountToRootEffector;
		const AStaticSceneEntity*	mTargetEntity;			// Non-NULL if the IKState object is properly initialized
		float						mDampingFactor;
		Axis						mEndEffectorAxis;
		Axis                        mEndEffectorUpAxis;
		bool                        mPointAt;
		bool                        mMoveTo;
};
//---------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::IKState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::IKState& state)
{
	state.Read(iStream);

	return iStream;
}
//---------------------------------------------------------------------------
#endif