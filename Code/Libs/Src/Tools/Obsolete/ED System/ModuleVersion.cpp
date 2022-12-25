// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ModuleVersion.h"
//---------------------------------------------------------------------------
ROS::ROSString ModuleVersion::GetVersionString() const
{
	char	convBuff[255];
	char	version[255];
	char	separator[] = ".";

	version[0] = 0;

	sprintf(convBuff, "%d", this->mMajorVersion);
	strcpy(version, convBuff);
	strcat(version, separator);

	sprintf(convBuff, "%d", this->mMinorVersion);
	strcat(version, convBuff);
	strcat(version, separator);
	
	sprintf(convBuff, "%d", this->mBuildNumber);
	strcat(version, convBuff);
	strcat(version, separator);
	
	sprintf(convBuff, "%d", this->mSubBuildNumber);
	strcat(version, convBuff);

	return ROS::ROSString(version);
}
//---------------------------------------------------------------------------
ModuleVersion GetModuleVersion(const ROS::ROSString& moduleName)
{
	// Obtain file name and set up a buffer for the version info.
	const HMODULE	moduleH = GetModuleHandle(moduleName.c_str());
	ASSERT(moduleH);
	const int		size = 255;
	char			moduleFileName[size];
	
	GetModuleFileName(moduleH, moduleFileName, size);

	DWORD		dummy;
	const DWORD	verInfoSize = GetFileVersionInfoSize(moduleFileName, &dummy);
	ASSERT(verInfoSize > 0);
	void*		verInfo = malloc(verInfoSize);

	const BOOL	gotVersion = GetFileVersionInfo(moduleFileName, NULL, verInfoSize, verInfo);
	ASSERT(gotVersion == TRUE);

	VS_FIXEDFILEINFO*	fixedFileInfo;
	UINT				fixedFileInfoLen;

	const BOOL	gotProductVersion = VerQueryValue(verInfo, 
													TEXT("\\"),
													(LPVOID *)&fixedFileInfo,
													&fixedFileInfoLen);
	ASSERT(gotProductVersion);

	ModuleVersion	moduleVersion;

	moduleVersion.mMajorVersion = HIWORD(fixedFileInfo->dwFileVersionMS);
	moduleVersion.mMinorVersion = LOWORD(fixedFileInfo->dwFileVersionMS);
	moduleVersion.mBuildNumber = HIWORD(fixedFileInfo->dwFileVersionLS);
	moduleVersion.mSubBuildNumber = LOWORD(fixedFileInfo->dwFileVersionLS);

	free(verInfo);

	return moduleVersion;
}
//---------------------------------------------------------------------------
