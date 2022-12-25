// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef UserPreferences_h
#define UserPreferences_h
// --------------------------------------------------------------------------
#include <istream>
#include "StringType.h"
// --------------------------------------------------------------------------
class UserPreferences
{
	public:
						UserPreferences();
		
		void			Read(std::istream& iStream);

		bool			IsValid() const;

		ROS::ROSString	GetDataPath() const;
		ROS::ROSString	GetDBExtensionFilename() const;

	private:
		bool			mIsValid;
		ROS::ROSString	mDataPath;
		ROS::ROSString	mDBExtensionFilename;
};
// --------------------------------------------------------------------------
#endif