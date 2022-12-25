#ifndef TimeTagSet_h
#define TimeTagSet_h

#include <fstream.h>
#include "TimeTag.h"

namespace ROS
{
class TimeTagSet
{
	public:
		TimeTagSet(const char* filename)
		: mTimeTags(NULL), mCount(0)
		{
			ifstream	fileStream(filename);

			fileStream >> mCount;

			mTimeTags = new TimeTag[mCount];

			for(int idx = 0; idx < mCount; ++idx)
			{	float	time;
				int		tag;

				fileStream >> time;
				fileStream >> tag; 

				mTimeTags[idx] = TimeTag(time, tag);
			}
		}

		TimeTagSet(const TimeTagSet& sourceTimeTagSet)
		{
			Copy(sourceTimeTagSet);
		}

		TimeTagSet::~TimeTagSet()
		{
			delete[] mTimeTags;
		}

		TimeTagSet& operator=(const TimeTagSet& sourceTimeTagSet)
		{
			if(this != &sourceTimeTagSet)
			{	Copy(sourceTimeTagSet);
			}

			return *this;
		}

		int GetCount() const
		{
			return mCount;
		}

		const TimeTag* GetBegin() const
		{
			return mTimeTags;
		}

		const TimeTag* GetEnd() const
		{
			return mTimeTags + mCount;
		}

	private:
		void Copy(const TimeTagSet& sourceTimeTagSet)
		{
			delete[] mTimeTags;
			mTimeTags = NULL;
			mCount = 0;
			
			mTimeTags = new TimeTag[sourceTimeTagSet.GetCount()];
			mCount = sourceTimeTagSet.GetCount();

			for(int idx = 0; idx < mCount; ++idx)
			{	mTimeTags[idx] = sourceTimeTagSet.mTimeTags[idx];
			}				
		}

		TimeTag*	mTimeTags;
		int			mCount;
};

}
#endif