#ifndef IDISPATCH_H
#define IDISPATCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IDispatch.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*
	//-------------------------------------------
	//
	GENRESULT GetProperty (const C8 *propName, DACOM_VARIANT value);
		INPUT:
			propName: Pointer to an ASCIIZ string identifying the property.
			value: Address of (user supplied) data area to receive the property information.
		RETURNS:
			GR_OK if the property is supported by the instance.
			GR_INVALID_PARMS if property is not supported
			GR_GENERIC if property is supported, but unable to get the property.
		OUTPUT:
			If the property is supported, the memory at 'value' is filled with the property value.

  
			Data type			Return value
			----------          ------------
			BOOL32				Boolean value
			ASCIIZ string		buffer at 'value' is filled with ASCIIZ string.
			HWND				Handle value.
			RECT				buffer at 'value' is filled with a RECT structure.
			LONG				32 bit long value

	  
	//-------------------------------------------
	//
	GENRESULT SetProperty (const C8 *propName, DACOM_VARIANT value);
		INPUT:
			propName: Pointer to an ASCIIZ string identifying the property.
			value: Data used to send the property information.
		RETURNS:
			GR_OK if the property is supported by the instance, and can be modified.
			GR_INVALID_PARMS if property is not supported
			GR_GENERIC if property is supported, but unable to set the property.
		OUTPUT:
			If the property is supported and can accept the modification, the internal
			property is set to the new value.


			Data type			   value
			----------          ------------
			BOOL32				Boolean value
			ASCIIZ string		Address of caller-supplied ASCIIZ string
			HWND				Handle value.
			RECT				Address of RECT structure constructed by caller.
			LONG				32 bit long value

			In the case where 'value' is the address of a caller-defined structure or ASCIIZ string,
			the object stores a copy of the string in its instance data.


	//-------------------------------------------
	//
	void SetProperties (const _PROPVAL_ENTRY list[]);
		INPUT:
			list: Array of property name / value entries, terminated with {0,0,0} entry.
		OUTPUT:
			Calls SetProperty() for each entry in the list.


*/

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef DAVARIANT_H
#include "DAVariant.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

//

#define IID_IDADispatch MAKE_IID("IDADispatch", 1)

//

struct DACOM_NO_VTABLE IDADispatch : public IDAComponent
{
	DEFMETHOD(Invoke) (const C8 *methodName, DACOM_VARIANT parm1 = DACOM_VARIANT() ) = 0;

	GENRESULT GetProperty (const C8 *propName, DACOM_VARIANT value);

	GENRESULT SetProperty (const C8 *propName, DACOM_VARIANT value);

	void SetProperties (const PROPERTY list[]);
};


//------------------------------------------
//
inline GENRESULT IDADispatch::GetProperty (const C8 *propName, DACOM_VARIANT value)
{
	C8 buffer[64];

	strncpy(buffer+4, propName, sizeof(buffer)-5);
	buffer[sizeof(buffer)-1] = 0;

	*((U32*)buffer) = '_teg';

	return Invoke(buffer, value);
}
//------------------------------------------
//
inline GENRESULT IDADispatch::SetProperty (const C8 *propName, DACOM_VARIANT value)
{
	C8 buffer[64];

	strncpy(buffer+4, propName, sizeof(buffer)-5);
	buffer[sizeof(buffer)-1] = 0;

	*((U32*)buffer) = '_tes';

	return Invoke(buffer, value);
}
//------------------------------------------
//
inline void IDADispatch::SetProperties (const PROPERTY list[])
{
	int i=0;

	while (list[i].name)
	{
		SetProperty(list[i].name, list[i]);
		i++;
	}
}




//--------------------------------------------------------------------------//
//--------------------------End IDispatch.h---------------------------------//
//--------------------------------------------------------------------------//

#endif