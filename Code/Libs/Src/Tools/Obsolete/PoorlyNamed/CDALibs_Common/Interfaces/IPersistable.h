// IPersistable
//
//
//

#ifndef IPersistable_H
#define IPersistable_H

#include "DACOM.h"
#include "DACOM_Utility.h"
#include "FileSys.h"

static const char *IID_IPersistable = "IPersistable";

dacom_interface( IPersistable )
{
	DACOM_INTERFACE_METHOD( LoadFromFileSystem,	(const char * name, IFileSystem *IFS));
	DACOM_INTERFACE_METHOD( SaveToFileSystem,	(const char * name, IFileSystem *IFS));
};

#endif
