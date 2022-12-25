// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef FlaggedOrientation_h
#define FlaggedOrientation_h

#include "Orientation.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticSceneEntity;
// --------------------------------------------------------------------------
// FlaggedOrientation
// --------------------------------------------------------------------------
class CPP_DECL FlaggedOrientation: public Orientation
{
	public:
		enum InterpolationType
		{
			kLinear,
			kTangent,
			kLookAt,
			kSpline
		};

		FlaggedOrientation()
		:mInterpolation(kLinear), mTargetEntity(NULL)
		{
		}

		FlaggedOrientation(const FlaggedOrientation& orientation)
		:BaseClass(orientation), mInterpolation(orientation.GetInterpolationType()), mTargetEntity(orientation.GetTargetEntity())
		{
		}

		FlaggedOrientation(const Orientation& orientation, InterpolationType type, AStaticSceneEntity* targetEntity)
		:BaseClass(orientation), mInterpolation(type), mTargetEntity(targetEntity)
		{
			ASSERT	(	(mInterpolation == kLookAt && targetEntity != NULL) ||
						(mInterpolation != kLookAt && targetEntity == NULL)
					);
		}

		void SetInterpolationType(InterpolationType type)
		{
			mInterpolation = type;

			if(type != kLookAt)
			{
				mTargetEntity = NULL;
			}
		}

		InterpolationType GetInterpolationType() const
		{
			return mInterpolation;
		}

		void SetTargetEntity(AStaticSceneEntity* targetEntity)
		{
			ASSERT(targetEntity == NULL || mInterpolation == kLookAt);

			mTargetEntity = targetEntity;
		}

		AStaticSceneEntity* GetTargetEntity() const
		{
			return mTargetEntity;
		}

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& ostreamR);

	private:
		typedef Orientation BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		InterpolationType	mInterpolation;
		AStaticSceneEntity*	mTargetEntity;
};
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::FlaggedOrientation& orientation)
{
	orientation.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::FlaggedOrientation& orientation)
{
	orientation.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif