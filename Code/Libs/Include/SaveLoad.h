//
// SaveLoad.h
//
// An interface for saving and loading data
//

#ifndef SAVER_LOADER_HEADER_H_FILE_
#define SAVER_LOADER_HEADER_H_FILE_

#ifndef DACOM_H
	#include "DACOM.h"
#endif

#define IID_ISaverLoader MAKE_IID("ISaverLoader",1)

struct DACOM_NO_VTABLE ISaverLoader : IDAComponent
{
	// for saving and loading using XML

	virtual bool Save( class TiXmlNode& ) = 0;
	virtual bool Load( class TiXmlNode& ) = 0;

	// for saving and loading using IFileSystem

	virtual bool Save( struct IFileSystem& ) = 0;
	virtual bool Load( struct IFileSystem& ) = 0;
};

#endif