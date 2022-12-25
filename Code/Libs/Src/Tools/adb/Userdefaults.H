#ifndef USERDEFAULTS_H
#define USERDEFAULTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             UserDefaults.H                               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Ajackson $

    $Header: /Libs/Src/Tools/adb/Userdefaults.H 4     10/03/03 2:54p Ajackson $
*/			    
//--------------------------------------------------------------------------//
/*
        Stores user preferences in the registry
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//#ifndef DATA_H
//#include "Data.h"
//#endif

//--------------------------------------------------------------------------//
// On GetDefaults(), if dwSize member is wrong, the values stored may not be valid
//
//--------------------------------------------------------------------------//

#define DEFAULTS_VERSION 4

enum _STATE
{
	MT_,
	BT_,
	GT_
};

struct USER_DEFAULTS
{
	U32  dwSize;
	U32  dwVersion;
	U32  iMainX,iMainY;

	//U32  showState;
	BOOL32  autoLoad;
	_STATE  _state;
	BOOL32  disableHeader;
	char    defaultH[32];
	char    path[128];
	char    pathToResourceFile[256];
	char    pathToObjects[256];
};


struct DACOM_NO_VTABLE IUserDefaults : public IDAComponent
{
	DEFMETHOD_(BOOL32,LoadDefaults) (void) = 0;

	DEFMETHOD_(BOOL32,StoreDefaults) (void) = 0;

	DEFMETHOD_(BOOL32,GetDirectoryFromRegistry) (char *szDirName, U32 id) = 0;
	
	DEFMETHOD_(BOOL32,SetDirectoryInRegistry) (const char *szDirName, U32 id) = 0;

	DEFMETHOD_(BOOL32,GetInputFilename) (char *dst, U32 id) = 0;	// stores resulting directory name in registry

	DEFMETHOD_(BOOL32,GetOutputFilename) (char *dst, U32 id) = 0;	// stores resulting directory name in registry

	DEFMETHOD_(USER_DEFAULTS *,GetDefaults) (void) = 0;

	DEFMETHOD_(BOOL32,SetNameInMRU) (const char *szFileName, U32 id, U32 pos=0) = 0;

	DEFMETHOD_(BOOL32,GetNameInMRU) (char *szFileName, U32 id, U32 pos=0) = 0;

	DEFMETHOD_(S32,FindNameInMRU) (const char *szFileName, U32 id) = 0;

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (const char *szFileName, U32 id) = 0;

	DEFMETHOD_(BOOL32,RemoveNameFromMRU) (U32 id, U32 pos) = 0;

	DEFMETHOD_(BOOL32,InsertNameIntoMRU) (const char *szFileName, U32 id) = 0;

	DEFMETHOD_(BOOL32,InsertDataPath) (const char *szFilePath) = 0;
};


#endif
