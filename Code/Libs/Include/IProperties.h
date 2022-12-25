#ifndef IPROPERTIES_H
#define IPROPERTIES_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IProperties.h                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/

/**
		This header described the IProperty and IProperties interfaces. The IProperties interface is used to manage
	a named list of components that implement IProperty. The engine will automatically load properties for
	engine instances, and the application can store their own.

	//-----------------------------
	// IProperty Methods
	//-----------------------------

	PROPERTY_TYPE COMAPI get_type ()
		INPUT:
			NONE

		RETURNS:
			PROPERTY_TYPE indicating the type of the property represented by this component

		NOTES:
			None.

  	GENRESULT COMAPI get_int (int &value)
	GENRESULT COMAPI get_uint (unsigned int &value)
	GENRESULT COMAPI get_single (SINGLE &value)
	GENRESULT COMAPI get_double (DOUBLE &value)
	GENRESULT COMAPI get_vector (Vector &value)
	GENRESULT COMAPI get_matrix (Matrix &value)
	GENRESULT COMAPI get_transform (Transform &value)
	GENRESULT COMAPI get_component (IDAComponent *&value)
	GENRESULT COMAPI get_string (const char *&value)
	GENRESULT COMAPI get_void (void *&value)
		INPUT:
			A reference to storage for the retrieved property value

		RETURNS:
			GR_INVALID_PARMS if the type of the property does not match the requested type
			GR_OK if the value was retrieved.

		NOTES:
			None.

	//-----------------------------
	// IProperties Methods
	//-----------------------------
	int COMAPI get_count (INSTANCE_INDEX idx)
		INPUT:
			idx - the instance index whose property count is to be retrieved

		RETURNS:
			the count of properties for the indexed instance.
			0 if idx is invalid or does not have any properties.

		NOTES:
			None.

	const char* COMAPI get_name (INSTANCE_INDEX idx, int propIndex)
		INPUT:
			idx       - the index of the engine instance in question
			propIndex - the 0 based index of the property in question

		RETURNS:
			NULL if propIndex is < 0 or >= get_count(), or idx is invalid.
			A pointer to the name of the property is otherwise returned.

		NOTES:
			None.

	bool COMAPI exists (INSTANCE_INDEX idx, const char *name)
		INPUT:
			idx  - the index of the engine instance in question
			name - the name of the property to be checked for existance

		RETURNS:
			false if the property doesn't exist, name is NULL, or idx is invalid. true otherwise.

		NOTES:
			None.

	GENRESULT COMAPI get_by_name (INSTANCE_INDEX idx, const char *name, IProperty **prop)
		INPUT:
			idx  - the index of the engine instance in question
			name - the name of the property to be retrieved
			prop - a pointer to storage for the IProperty pointer

		RETURNS:
			GR_OK if the property exists and its pointer was stored in *prop.
			GR_INVALID_PARAMS if name or prop is NULL, or idx is an invalid index.

		NOTES:
			None.

	GENRESULT COMAPI set_by_name (INSTANCE_INDEX idx, const char *name, IProperty *prop)
		INPUT:
			idx  - the index of the engine instance in question
			name - the name of the property to be retrieved
			prop - a pointer to the IProperty to store.

		RETURNS:
			GR_OK if the property was set.
			GR_INVALID_PARAMS if name or prop is NULL, or idx is an invalid index.

		NOTES:
			The reference count for prop is incremented, and if the property already existed, its reference
		count is decremented.

	bool COMAPI del_by_name (INSTANCE_INDEX idx, const char *name)
		INPUT:
			idx  - the index of the engine instance in question
			name - the name of the property to be retrieved

		RETURNS:
			true if the property existed and was deleted.
			false if idx is invalid, name is NULL, or the property does not exist.

		NOTES:
			If the property existed, its reference count is decremented.

	GENRESULT COMAPI get_by_index (INSTANCE_INDEX idx, int propIndex, IProperty **prop)
		INPUT:
			idx       - the index of the engine instance in question
			propIndex - the name of the property to be retrieved
			prop      - a pointer to storage for the IProperty pointer

		RETURNS:
			GR_OK if propIndex >= 0 && propIndex < get_count(), and its pointer was stored in *prop.
			GR_INVALID_PARAMS if prop is NULL, idx is an invalid index, or propIndex is out of range.

		NOTES:
			None.

	GENRESULT COMAPI set_by_index (INSTANCE_INDEX idx, int propIndex, IProperty *prop)
		INPUT:
			idx       - the index of the engine instance in question
			propIndex - the name of the property to be retrieved
			prop      - a pointer to storage for the IProperty pointer

		RETURNS:
			GR_OK if the property was set.
			GR_INVALID_PARAMS if prop is NULL, idx is an invalid index, or propIndex is out of range.

		NOTES:
			The reference count for prop is incremented, and if the property already existed, its reference
		count is decremented.

	bool COMAPI del_by_index (INSTANCE_INDEX idx, int propIndex)
		INPUT:
			idx  - the index of the engine instance in question
			propIndex - the name of the property to be deleted

		RETURNS:
			true if the property existed and was deleted.
			false if idx is invalid, propIndex is invalid, or the property does not exist.

		NOTES:
			If the property existed, its reference count is decremented.

**/

#ifndef DACOM_H
#include "dacom.h"
#endif

#ifndef DAVARIANT_H
#include "davariant.h"
#endif

#ifndef ENGINE_H
#include "engine.h"
#endif

#ifndef FILESYS_H
#include "filesys.h"
#endif

const int INVALID_PROPERTY_INDEX = -1;

//********************************************************************************************
#define IID_IProperty MAKE_IID("IProperty", 1)
typedef unsigned long PROP_TYPE;

// The number of types supported by properties is strictly limited to the types given below.
const PROP_TYPE PT_UNKNOWN   = 0;  // type is unknown, data is invalid
const PROP_TYPE PT_LONG      = 1;  // type is 32 bit signed integer
const PROP_TYPE PT_ULONG     = 2;  // type is 32 bit unsigned integer
const PROP_TYPE PT_SINGLE    = 3;  // type is a single precision IEEE floating point number
const PROP_TYPE PT_DOUBLE    = 4;  // type is a double precision IEEE floating point number
const PROP_TYPE PT_VECTOR    = 5;  // type is a DA Vector
const PROP_TYPE PT_MATRIX    = 6;  // type is a DA Matrix
const PROP_TYPE PT_TRANSFORM = 7;  // type is a DA Transform
const PROP_TYPE PT_COMPONENT = 8;  // type is a DACOM component
const PROP_TYPE PT_STRING    = 9;  // type is a const char *
const PROP_TYPE PT_VOID      = 10; // type is a memory pointer

class Vector;
class Matrix;
class Transform;

struct IProperty : public IDAComponent
{
	virtual PROP_TYPE COMAPI get_type () = 0;

	// These methods retrieve the known value types. No coersion is done on the values.
	// If the type does not match the type implicit in the function, it returns GR_INVALID_PARMS.
	virtual GENRESULT COMAPI get_long (long &value) = 0;
	virtual GENRESULT COMAPI get_ulong (unsigned long &value) = 0;
	virtual GENRESULT COMAPI get_single (SINGLE &value) = 0;
	virtual GENRESULT COMAPI get_double (DOUBLE &value) = 0;
	virtual GENRESULT COMAPI get_vector (Vector &value) = 0;
	virtual GENRESULT COMAPI get_matrix (Matrix &value) = 0;
	virtual GENRESULT COMAPI get_transform (Transform &value) = 0;
	virtual GENRESULT COMAPI get_component (IDAComponent *&value) = 0;
	virtual GENRESULT COMAPI get_string (const char *&value) = 0;
	virtual GENRESULT COMAPI get_void (void *&value) = 0;
};

#define IID_ISetProperty MAKE_IID("ISetProperty", 1)
struct ISetProperty : public IDAComponent
{
	virtual void COMAPI clear () = 0;

	// These methods set the known value types. No coersion is done on the values.
	// The previous value, if any, is cleared before the new value is set.
	virtual void COMAPI set_long (long value) = 0;
	virtual void COMAPI set_ulong (unsigned long value) = 0;
	virtual void COMAPI set_single (SINGLE value) = 0;
	virtual void COMAPI set_double (DOUBLE value) = 0;
	// NOTE: Copies the vector, matrix, or transform
	virtual void COMAPI set_vector (const Vector &value) = 0;
	virtual void COMAPI set_matrix (const Matrix &value) = 0;
	virtual void COMAPI set_transform (const Transform &value) = 0;
	// NOTE: Calls AddRef() on the component.
	virtual void COMAPI set_component (IDAComponent *value) = 0;
	// NOTE: Copies the string.
	virtual void COMAPI set_string (const char *value) = 0;
	// NOTE: Simply copies the pointer. Does not delete on clear
	virtual void COMAPI set_void (void *value) = 0;
};

//********************************************************************************************
#define IID_IProperties MAKE_IID("IProperties", 3)
struct IProperties : public IDAComponent
{
	// These routines operate on instances
	virtual int COMAPI get_count (INSTANCE_INDEX idx) = 0;
	virtual const char* COMAPI get_name (INSTANCE_INDEX idx, int propIndex) = 0;

	virtual bool COMAPI exists (INSTANCE_INDEX idx, const char *name) = 0;

	virtual GENRESULT COMAPI get_by_name (INSTANCE_INDEX idx, const char *name, IProperty **prop) = 0;
	virtual GENRESULT COMAPI set_by_name (INSTANCE_INDEX idx, const char *name, IProperty *prop) = 0;
	virtual bool COMAPI del_by_name (INSTANCE_INDEX idx, const char *name) = 0;

	virtual GENRESULT COMAPI get_by_index (INSTANCE_INDEX idx, int propIndex, IProperty **prop) = 0;
	virtual GENRESULT COMAPI set_by_index (INSTANCE_INDEX idx, int propIndex, IProperty *prop) = 0;
	virtual bool COMAPI del_by_index (INSTANCE_INDEX idx, int propIndex) = 0;

	// These routines operate on archetypes
	// NOTE: Replacing an archetype's property with arch_set_by_name() or arch_set_by_index() will affect only
	// those instances subsequently created. Changing the value of an archetype's property via tha ISetProperty interface
	// will change it for all the instances that still have a reference to that property, i.e. those that have
	// not changed the property with set_by_name() or set_by_index().
	virtual int COMAPI arch_get_count (ARCHETYPE_INDEX idx) = 0;
	virtual const char* COMAPI arch_get_name (ARCHETYPE_INDEX idx, int propIndex) = 0;

	virtual bool COMAPI arch_exists (ARCHETYPE_INDEX idx, const char *name) = 0;

	virtual GENRESULT COMAPI arch_get_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty **prop) = 0;
	virtual GENRESULT COMAPI arch_set_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty *prop) = 0;
	virtual bool COMAPI arch_del_by_name (ARCHETYPE_INDEX idx, const char *name) = 0;

	virtual GENRESULT COMAPI arch_get_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty **prop) = 0;
	virtual GENRESULT COMAPI arch_set_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty *prop) = 0;
	virtual bool COMAPI arch_del_by_index (ARCHETYPE_INDEX idx, int propIndex) = 0;
};

//----------------------------------------------------------------------------------
//------------------------END IProperties.h---------------------------------------------
//----------------------------------------------------------------------------------
#endif

