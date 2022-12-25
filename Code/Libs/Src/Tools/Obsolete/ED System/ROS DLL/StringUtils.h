// StringUtils.h
//
//---------------------------------------------------------------------------
#ifndef _h_StringUtils
#define _h_StringUtils
//---------------------------------------------------------------------------

#include "StringType.h"
//---------------------------------------------------------------------------

ROS::ROSString GetFilePath(const ROS::ROSString& fullPath);
ROS::ROSString GetFileName(const ROS::ROSString& fullPath);
ROS::ROSString GetFileExtension(const ROS::ROSString& fullPath);
//---------------------------------------------------------------------------

#endif