// INamedProperty
//
//
//

#ifndef INamedProperty_H
#define INamedProperty_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_INamedProperty = "INamedProperty";

dacom_interface( INamedProperty )
{
	DACOM_INTERFACE_METHOD( SetPropertyFromString,	(const char *name, const char *value));
	DACOM_INTERFACE_METHOD( SetPropertyFromInt,		(const char *name, const U32 value));
	DACOM_INTERFACE_METHOD( SetPropertyFromReal,	(const char *name, const float value));
	DACOM_INTERFACE_METHOD( SetPropertyFromUnknown,	(const char *name, const void *value));

	DACOM_INTERFACE_METHOD( GetPropertyAsString,	(const char *name, char *value, U32 max_len));
	DACOM_INTERFACE_METHOD( GetPropertyAsInt,		(const char *name, U32 *value));
	DACOM_INTERFACE_METHOD( GetPropertyAsReal,		(const char *name, float *value));
	DACOM_INTERFACE_METHOD( GetPropertyAsUnknown,	(const char *name, void **value));
};

#endif
