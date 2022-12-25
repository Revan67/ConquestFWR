// Author: Shaival Varma
//---------------------------------------------------------------------------
#include <string>
#include "IHardPoint.h"
#include "CodeMsg.h"

class ArchetypeHardPoints;
struct IEngine;

class SingleHardPoint
{
	friend class HardPoints;

	public:
		SingleHardPoint(const std::string& name, const HardpointInfo& hardpointInfo)
		: mName(name), mHardpointInfo(hardpointInfo)
		{
		}

		const std::string& GetName() const
		{
			return mName;
		}

		Vector GetPosition() const
		{
			return mHardpointInfo.point;
		}

		Matrix GetOrientation() const
		{
			return mHardpointInfo.orientation;
		}

		void Draw() const;

	private:
		SingleHardPoint()
		: mName("Unitialized HardPoint Instance")
		{
		}

		std::string		mName;
		HardpointInfo	mHardpointInfo;
};


class HardPoints
{
	public:
		HardPoints(INSTANCE_INDEX instanceIndex, IEngine* engine);

		~HardPoints();

		INSTANCE_INDEX GetInstanceIndex() const
		{
			return mInstanceIndex;
		}

		const SingleHardPoint* Begin() const
		{
			return mHardPoints;
		}

		const SingleHardPoint* End() const
		{
			return mHardPoints + mHardPointCount;
		}

		const SingleHardPoint& operator[](unsigned int idx) const
		{
			ASSERT(idx < mHardPointCount);

			return *(mHardPoints + idx);
		}

		unsigned int Count() const
		{
			return mHardPointCount;
		}

		void Draw(const Transform& modelView) const;
		
	private:
		INSTANCE_INDEX		mInstanceIndex;
		int					mHardPointCount;
		SingleHardPoint*	mHardPoints;
		IEngine* const		mEngine;
};
