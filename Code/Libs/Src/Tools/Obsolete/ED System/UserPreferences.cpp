// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "UserPreferences.h"
#include "StringList.h"
// --------------------------------------------------------------------------
ROS::ROSString FindDataString(const ROS::ROSString& string)
{
	int idx = string.find("=");

	if(idx == ROS::ROSString::npos)
	{	return "";
	}

	ROS::ROSString	dataString = string.substr(idx + 1);
	dataString = ROS::TrimLeadingWhiteSpace(dataString);

	return dataString;
}
// --------------------------------------------------------------------------
UserPreferences::UserPreferences()
: mIsValid(false)
{
}
// --------------------------------------------------------------------------
void UserPreferences::Read(std::istream& iStream)
{
	// Read the strings
	ROS::StringList	strings;

	try
	{	strings.ReadStringsTillEndOfStream(iStream);
	}
	catch(...)
	{	return;
	}

	// Clean out leading and trailing white space
	const int stringCount = strings.GetStringCount();

	unsigned int	stringIdx;
	ROS::ROSString	entry;

	for(stringIdx = 0; stringIdx < stringCount; ++stringIdx)
	{	strings.Replace(stringIdx, ROS::TrimLeadingWhiteSpace(ROS::TrimTrailingWhiteSpace(strings.GetString(stringIdx))));
	}

	// Find the preferences
	
	// Find the data section
	stringIdx = strings.Find("[Data]");
	if(stringIdx == -1)
	{	return;
	}

	// Find data path
	stringIdx = strings.FindSubString("Data path");
	if(stringIdx == -1)
	{	return;
	}
	
	entry = FindDataString(strings.GetString(stringIdx));
		
	if(entry.size() == 0)
	{	return;
	}

	if(entry[entry.size() - 1] == '\\')
	{	mDataPath = entry;
	}
	else
	{	mDataPath = entry + '\\';
	}

	// Find DB extension filename
	stringIdx = strings.FindSubString("Data DB extension");
	if(stringIdx == -1)
	{	return;
	}
	
	entry = FindDataString(strings.GetString(stringIdx));
		
	if(entry.size() == 0)
	{	return;
	}

	mDBExtensionFilename = entry;

	// Found all the preferences
	mIsValid = true;
}
// --------------------------------------------------------------------------
bool UserPreferences::IsValid() const
{
	return mIsValid;
}
// --------------------------------------------------------------------------
ROS::ROSString UserPreferences::GetDataPath() const
{
	return mDataPath;
}
// --------------------------------------------------------------------------
ROS::ROSString UserPreferences::GetDBExtensionFilename() const
{
	return mDBExtensionFilename;
}
// --------------------------------------------------------------------------
