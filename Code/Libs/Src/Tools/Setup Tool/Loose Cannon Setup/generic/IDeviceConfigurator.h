// IDeviceConfigurator
//
//
//

#ifndef IDeviceConfigurator_H
#define IDeviceConfigurator_H

#include "DACOM.h"
#include "DACOM_Utility.h"
#include "deviceselection.h"

static const char *IID_IDeviceConfigurator = "IDeviceConfigurator";

dacom_interface( IDeviceConfigurator )
{
	DACOM_INTERFACE_METHOD( SetValidateCB,	( DS_VALIDATIONCALLBACK fn));
	DACOM_INTERFACE_METHOD( SetHandleControlCommandCB,	( DS_CONTROLCALLBACK fn));
	DACOM_INTERFACE_METHOD( GetAbility,					( U32 * supported, U32 ability));
	DACOM_INTERFACE_METHOD( GetInfo,					( RPDEVICEINFO * info));
	DACOM_INTERFACE_METHOD( GetAudioDevice,				( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD( SetAudioDevice,				( char *device_descriptor));
	DACOM_INTERFACE_METHOD( GetAudioCaptureDevice,		( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD( SetAudioCaptureDevice,		( char *device_descriptor));
	DACOM_INTERFACE_METHOD( GetSelectedResolution,		( U32 * resolution));
	DACOM_INTERFACE_METHOD( SetSelectedResolution,		( U32 resolution));
};

#endif
