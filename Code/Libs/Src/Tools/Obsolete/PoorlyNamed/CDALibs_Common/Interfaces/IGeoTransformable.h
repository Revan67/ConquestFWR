// IGeoTransformable
//
//
//

#ifndef IGeoTransformable_H
#define IGeoTransformable_H

#include "DACOM.h"
#include "3DMath.h"

#include "DACOM_Utility.h"

static const char *IID_IGeoTransformable = "IGeoTransformable";

dacom_interface( IGeoTransformable )
{
	DACOM_INTERFACE_METHOD( SetIdentity,					(void));
	DACOM_INTERFACE_METHOD( Multiply,						(const Transform *T));
	DACOM_INTERFACE_METHOD( GetTranspose,					(Transform *out_T));
	DACOM_INTERFACE_METHOD( GetInverse,						(Transform *out_T));

	DACOM_INTERFACE_METHOD( SetTransform,					(const Transform *Transform));
	DACOM_INTERFACE_METHOD( GetTransform,					(Transform *out_Transform));

	DACOM_INTERFACE_METHOD( SetTranslation,					(const Vector *translation));
	DACOM_INTERFACE_METHOD( GetTranslation,					(Vector *out_translation));

	DACOM_INTERFACE_METHOD( SetOrientationFromMatrix,		(const Matrix *M));
	DACOM_INTERFACE_METHOD( SetOrientationFromTransform,	(const Transform *T));
	DACOM_INTERFACE_METHOD( SetOrientationFromQuaternion,	(const Quaternion *Q));
	DACOM_INTERFACE_METHOD( SetOrientationFromAxisAngle,	(const Vector *axis, const float angle_rad));
	DACOM_INTERFACE_METHOD( GetOrientation,					(Matrix *out_M));
	
	DACOM_INTERFACE_METHOD( SetBasisI,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD( GetBasisI,						(Vector *out_V));

	DACOM_INTERFACE_METHOD( SetBasisJ,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD( GetBasisJ,						(Vector *out_V));
	
	DACOM_INTERFACE_METHOD( SetBasisK,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD( GetBasisK,						(Vector *out_V));

	DACOM_INTERFACE_METHOD( Rotate,							(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD( RotateAndTranslate,				(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD( RotateByInverse,				(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD( RotateAndTranslateByInverse,	(const Vector *V, Vector *out_V));
};

#endif