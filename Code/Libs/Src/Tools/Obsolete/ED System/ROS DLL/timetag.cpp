// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "TimeTag.h"
//---------------------------------------------------------------------------
enum TimeTagListFieldID
{
	kSize,
	kFirstTimeTag
};
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& oStream, const ROS::TimeTagList& list)
{
	OStreamWiz<TimeTagListFieldID>	oWiz(oStream);

	int size = list.size();
	oWiz.Put(kSize, size);

	ROS::TimeTagList::const_iterator		begin = list.begin();
	const ROS::TimeTagList::const_iterator	end = list.end();
	unsigned int							timeTagIdx = 0;

    while(begin < end)
    {
		oWiz.Put(static_cast<TimeTagListFieldID>(kFirstTimeTag + timeTagIdx), *begin);

    	++begin;
		++timeTagIdx;
    }

	return oStream;
}
//---------------------------------------------------------------------------
std::istream& operator>>(std::istream& iStream, ROS::TimeTagList& list)
{
	IStreamWiz<TimeTagListFieldID>	iWiz(iStream);

 	int size;

    iWiz.Get(kSize, size);

	list.resize(size);

	ROS::TimeTagList::iterator				begin = list.begin();
	const ROS::TimeTagList::const_iterator	end = list.end();
	unsigned int							timeTagIdx = 0;

    while(begin < end)
    {
		iWiz.Get(static_cast<TimeTagListFieldID>(kFirstTimeTag + timeTagIdx), *begin);
    	
		++begin;
		++timeTagIdx;
    }

	return iStream;
}
//---------------------------------------------------------------------------
