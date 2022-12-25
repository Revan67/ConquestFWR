// IPersistableOptions
//
//
//

#ifndef IPersistableOptions_H
#define IPersistableOptions_H

#include "DACOM.h"
#include "DACOM_Utility.h"
#include "FileSys.h"

static const char *IID_IPersistableOptions = "IPersistableOptions";

dacom_interface( IPersistableOptions )
{
	DACOM_INTERFACE_METHOD( LoadFromSection,	(const char * name, IProfileParser *IPP ));
	DACOM_INTERFACE_METHOD( SaveToSection,		(const char * name, IProfileParser *IPP ));
};

#endif
