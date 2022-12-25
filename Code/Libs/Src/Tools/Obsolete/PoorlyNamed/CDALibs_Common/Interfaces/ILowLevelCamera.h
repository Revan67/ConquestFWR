// ILowLevelCamera
//
//
//

#ifndef ILowLevelCamera_H
#define ILowLevelCamera_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_ILowLevelCamera = "ILowLevelCamera";

enum ILLC_ASPECTTYPE
{
	ILLC_ASPECT_H2V,
	ILLC_ASPECT_V2H,

	ILLC_ASPECT_MAX
};

dacom_interface( ILowLevelCamera )
{
	DACOM_INTERFACE_METHOD( SetHorizontalFieldOfView,	(float fov));
	DACOM_INTERFACE_METHOD( GetHorizontalFieldOfView,	(float *fov));

	DACOM_INTERFACE_METHOD( SetVerticalFieldOfView,		(float fov));
	DACOM_INTERFACE_METHOD( GetVerticalFieldOfView,		(float *fov));

	DACOM_INTERFACE_METHOD( SetNearClipDistance,		(float dist_z));
	DACOM_INTERFACE_METHOD( GetNearClipDistance,		(float *dist_z));

	DACOM_INTERFACE_METHOD( SetFarClipDistance,			(float dist_z));
	DACOM_INTERFACE_METHOD( GetFarClipDistance,			(float *dist_z));

	DACOM_INTERFACE_METHOD( SetAspect,					(ILLC_ASPECTTYPE type, float aspect));
	DACOM_INTERFACE_METHOD( GetAspect,					(float *aspect));

	DACOM_INTERFACE_METHOD( SetViewport,				(float x, float y, float w, float h));
	DACOM_INTERFACE_METHOD( GetViewport,				(float *x, float *y, float *w, float *h));
};

#endif

