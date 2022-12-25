#ifndef ILIGHT_H
#define ILIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ILight.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/ILight.h 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------

/*

	//-------------------------------------------
	//
	GENRESULT ILight::GetTransform (class Transform & transform) const;
		OUTPUT:
			transform:	Orientation of the light.

	//-------------------------------------------
	//
	GENRESULT ILight::GetPosition (class Vector & position) const;
		OUTPUT:
			position: 3D position of the light, in WORLD coordinates.

	//-------------------------------------------
	//
	GENRESULT ILight::GetColor (struct LightRGB & color) const;
		OUTPUT:
			color: RGB color value, Range is 0 to 255.

	//-------------------------------------------
	//
	GENRESULT ILight::GetDirection (class Vector & direction) const;
		OUTPUT:
			direction: direction light is pointing, in the light's orientation.
	
	//-------------------------------------------
	//
	SINGLE ILight::GetRange (void) const;
		RETURNS:
			Max distance light is effective, in WORLD units.

	//-------------------------------------------
	//
	BOOL32 ILight::IsInfinite (void) const;
		RETURNS:
			TRUE if the light has infinite range.

	//-------------------------------------------
	//
	SINGLE ILight::GetCutoff (void) const;
		RETURNS:
			Angle (in degrees) of effective lighting, where 0 = no lighting, 
			180 = full arc lighting.

	//-------------------------------------------
	//
	U32 ILight::GetMap (void) const;
		RETURNS:
			0 if light mapping is not enabled, else returns the GL texture ID to use for light mapping.

  //-------------------------------------------

*/

#ifndef DACOM_H
#include "dacom.h"   
#endif

#ifndef LIGHTRGB_DEFINED
#define LIGHTRGB_DEFINED

struct LightRGB
{
	int r, g, b;
};

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define IID_ILight MAKE_IID("ILight",1)

struct DACOM_NO_VTABLE ILight : public IDAComponent
{
	DEFMETHOD(GetTransform) (class Transform & transform) const = 0;

	DEFMETHOD(GetPosition) (class Vector & position) const = 0;

	DEFMETHOD(GetColor) (struct LightRGB & color) const = 0;

	DEFMETHOD(GetDirection) (class Vector & direction) const = 0;
	
	DEFMETHOD_(SINGLE,GetRange) (void) const = 0;

	DEFMETHOD_(BOOL32,IsInfinite) (void) const = 0;

	DEFMETHOD_(SINGLE,GetCutoff) (void) const = 0;

	DEFMETHOD_(U32,GetMap) (void) const = 0;
};

//------------------------------------------------------------------------------
//------------------------------END ILight.h------------------------------------
//------------------------------------------------------------------------------

#endif