//////////////////////////////////////////////////////////////////////////////
//  File:    SccGeneric.h
//
//  Purpose: definitions used for accessing source control system via a
//  generic interface through a scc specific dll.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

#ifndef _SCCGENERIC_H_
#define _SCCGENERIC_H_


//a structure to store file status information
struct SScFileStatus
{
	bool bControlled;
	bool bOpenForEdit;
	bool bOpenByOther;
	bool bNewerInSc;
};

//messages (optionally) sent by the dll
//
#define NM_SENDINFO		WM_APP + 22
#define NM_SENDERROR	WM_APP + 23


//function which sets a window handle to use to notify
#define FNNAME_SETNOTIFYHWND  "SetNotifyHwnd"
typedef int (*TFnSetNotifyWnd)(HWND);

//function to get the file status
#define FNNAME_FILESTATUS  "GetFileStatus"
typedef int (*TFnGetFileStatus)(const char*, SScFileStatus*);

//function to check a file out
#define FNNAME_CHECKOUT  "CheckFileOut"
typedef int (*TFnCheckFileOut)(const char*);

//function to revert a file to its sc version
#define FNNAME_REVERT  "RevertFile"
typedef int (*TFnRevertFile)(const char*);

//function to check a file in
#define FNNAME_CHECKIN  "CheckFileIn"
typedef int (*TFnCheckFileIn)(const char*);

//function to add a file to the scc
#define FNNAME_ADD  "AddFile"
typedef int (*TFnAddFile)(const char*);

//function to diff to files
#define FNNAME_DIFF  "DiffFile"
typedef int (*TFnDiffFile)(const char*);

//function to sync a file to the latest revision
#define FNNAME_SYNC  "SyncFile"
typedef int (*TFnSyncFile)(const char*);

#endif //_SCCGENERIC_H_