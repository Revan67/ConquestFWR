//
// CmpndView.h
//
// Common definitions and a few externs.
//

#ifndef CMPNDVIEW_H_
#define CMPNDVIEW_H_

//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//

#include "DACOM.h"
#include "FDUMP.h"
#include "TempStr.h"
#include "3DMath.h"
#include "System.h"
#include "Engine.h"
#include "TSmartPointer.h"
#include "IRenderPrimitive.h"
#include "rendpipeline.h"
#include "ILight.h"
#include "ICamera.h"
#include "LightMan.h"
#include "ITextureLibrary.h"
#include "IMaterialLibrary.h"
#include "FileSys.h"
#include "RPUL.h"
#include "BaseLight.h"
#include "matrix4.h"
#include "packed_argb.h"

//

#include "Lights.h"
#include "resource.h"
//

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define HIGH_BIT 0x80000000

#define TOLERANCE 1e-4
#define NEAR_ZERO 1e-4

#define exit(X) {  GENERAL_TRACE_1( TEMPSTR( "Exiting: File: %s  Line: %d  Code: %d\n", __FILE__, __LINE__, X ) ); exit(X); }

//

const int MAX_OBJS = 1024;
const float font_scale = .125f;

//

extern INSTANCE_INDEX LoadedInstIndices[MAX_OBJS];
extern int			  LoadedInstCount;

extern float initial_radius;
extern float global_scale;
extern float CameraStartDistance;


extern PrimitiveBuilder pb;
extern RPFont3D			Font;

extern IEngine			*Engine;
extern ISystemContainer	*System;
extern IRenderPrimitive	*RenderPrim;
extern IRenderPipeline	*RenderPipe;
extern ILightManager	*LightMan;
extern ICamera			*TheCamera;
extern ITextureLibrary  *TextureLib;
extern IMaterialLibrary *MaterialLib;

extern HWND				hWnd;

extern U32				texture_count;
extern U32				material_count;
//

void Err( char* fmt, ... );
bool GetFileNameFromUser( HWND hParent, bool save_dialog, const char *filter, char *out_buffer, U32 max_buffer_len );
GENRESULT SetRenderVolume( const class Transform *object_to_world );
GENRESULT SetRender2D( void );

//

template<class _Ty> 
inline const _Ty& _MAX( const _Ty& _X, const _Ty& _Y )
{
	return (_X < _Y ? _Y : _X); 
}

//
		
template<class _Ty> 
inline const _Ty& _MIN( const _Ty& _X, const _Ty& _Y )
{
	return (_Y < _X ? _Y : _X); 
}

//
		
template<class _Ty> 
inline const _Ty& _CLAMP( const _Ty& _X, const _Ty& _L, const _Ty& _U )
{
	return _MAX( _L, _MIN( _U, _X ) );
}

//

#endif