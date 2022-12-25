// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "StringList.h"
//---------------------------------------------------------------------------
enum StringCollectionFieldID
{
	kSize,
	kFirstString
};
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
void StringList::Read(std::istream& iStream)
{
	IStreamWiz<FieldID> iWiz(iStream);

	iWiz.Get(kStringCollection, mStrings);
}
//---------------------------------------------------------------------------
void StringList::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
	
	oWiz.Put(kStringCollection, mStrings);
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& oStream, const ROS::StringList::StringCollection& strings)
{
	OStreamWiz<StringCollectionFieldID>	oWiz(oStream);

	const unsigned int	numEntities = strings.size();

	oWiz.Put(kSize, numEntities);

	ROS::StringList::StringCollection::const_iterator		begin = strings.begin();
	const ROS::StringList::StringCollection::const_iterator	end = strings.end();
	unsigned int							idx = 0;

	while(begin != end)
	{	
		oWiz.Put(static_cast<StringCollectionFieldID>(kFirstString + idx), *begin);
    	
		++begin;
		++idx;
	}

	return oStream;
}
//---------------------------------------------------------------------------
std::istream& operator>>(std::istream& iStream, ROS::StringList::StringCollection& strings)
{
	IStreamWiz<StringCollectionFieldID>	iWiz(iStream);

	unsigned int	numEntries;

	iWiz.Get(kSize, numEntries);

	strings.resize(numEntries);

	for(unsigned int entryIdx = 0; entryIdx < numEntries; ++entryIdx)
	{
		ROS::ROSString	stringEntry;

		iWiz.Get(static_cast<StringCollectionFieldID>(kFirstString + entryIdx), stringEntry);

		strings[entryIdx] = stringEntry;
	}

	return iStream;
}
//---------------------------------------------------------------------------

