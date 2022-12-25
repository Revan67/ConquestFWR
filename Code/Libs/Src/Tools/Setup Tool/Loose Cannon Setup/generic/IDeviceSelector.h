// IDeviceSelector
//
//
//

#ifndef IDeviceSelector_H
#define IDeviceSelector_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_IDeviceSelector = "IDeviceSelector";

dacom_interface( IDeviceSelector )
{
	DACOM_INTERFACE_METHOD( SetSelectedDevice,			( char *device_descriptor ));
	DACOM_INTERFACE_METHOD( GetSelectedDevice,			( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD( SetAvailableDeviceClasses,	( U32 classes ));
	DACOM_INTERFACE_METHOD( GetAvailableDeviceClasses,	( U32 *classes ));
};

#endif
