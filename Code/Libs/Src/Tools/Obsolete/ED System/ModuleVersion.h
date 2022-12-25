// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ModuleVersion_h
#define ModuleVersion_h
//---------------------------------------------------------------------------
struct ModuleVersion
{
	public:
		unsigned int mMajorVersion;
		unsigned int mMinorVersion;
		unsigned int mBuildNumber;
		unsigned int mSubBuildNumber;

		ROS::ROSString GetVersionString() const;
};
//---------------------------------------------------------------------------
ModuleVersion GetModuleVersion(const ROS::ROSString& moduleName);
//---------------------------------------------------------------------------
#endif 