#ifndef TDISPATCH_H
#define TDISPATCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TDispatch.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

                 Template class for DA's IDADispatch interface

*/
//--------------------------------------------------------------------------//

#ifndef IDISPATCH_H
#include "IDispatch.h"
#endif


#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef DAVARIANT_H
#include "DAVariant.h"
#endif

//--------------------------------------------------------------------------//

#define _DACOM_MAX_AUTO_PARMS		1					// max number of parms for automation method

#define _DACOM_PROPTYPE_ADDRESS		0x00000000			// absolute address of property
#define _DACOM_PROPTYPE_OFFSET		0x00000001			// offset within a class
#define _DACOM_PROPTYPE_CONST		0x00000002			// 'addr' is a constant (integer) value
#define _DACOM_PROPTYPE_METHOD		0x00000003

#define BEGIN_DACOM_DISPATCH_MAP(x) public: \
	const static _DACOM_DISPATCH_ENTRY* __stdcall _GetAutomationEntries() { \
	typedef x _DaComMapClass; \
	static const _DACOM_DISPATCH_ENTRY _entries[] = { 

#define DACOM_DISPATCH_STATIC_PROPERTY(x,y,z)\
	{x, \
	 (U32) &y, \
	 sizeof(y),  \
	 _DACOM_PROPTYPE_ADDRESS, \
	 (U16)z, \
	 0 },

#define DACOM_DISPATCH_MEMBER_PROPERTY(x,y,z)\
	{x, \
	 daoffsetofmember(_DaComMapClass, y), \
	 dasizeofmember(_DaComMapClass, y), \
	 _DACOM_PROPTYPE_OFFSET,  \
	 (U16)z, \
	 0 },

#define DACOM_DISPATCH_CONST_PROPERTY(x,y,z)\
	 {x, y, 	\
		4, \
		_DACOM_PROPTYPE_CONST, \
		(U16)z, \
	 0 },
	  
#define DACOM_DISPATCH_METHOD(v,w) \
	{ #v, \
	   0, 0, \
	   _DACOM_PROPTYPE_METHOD, \
	   (U16)w, \
	   ((DA_PROC) &v) }, 

#define DACOM_DISPATCH_METHOD2(u,v,w) \
	{ u, \
	   0, 0, \
	   _DACOM_PROPTYPE_METHOD, \
	   (U16)w, \
	   (DA_PROC) v }, 

#define END_DACOM_DISPATCH_MAP()   {0, 0, 0, 0, (U16)DAVT_EMPTY, 0}};\
	return _entries;}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <int dummy> 
struct __dispatch_type_coercion
{
#pragma pack (push, 1)

	struct _DUMMY_ENTRY
	{	
		const C8 *	name;
		U32			addr;				// address of property
		U8			size;				// size of property
		U8			type;				// record type
		U16			paramTypes[_DACOM_MAX_AUTO_PARMS];
	};

#pragma pack ( pop )

	virtual GENRESULT __stdcall get_property (void *instance, void * property, DACOM_VARIANT & value);

	virtual GENRESULT __stdcall set_property (void *instance, void * property, DACOM_VARIANT & value);

	virtual GENRESULT __stdcall call_method (void *instance, void * property, void *proc, DACOM_VARIANT list[]);
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Type, class Base=IDADispatch> 
struct DACOM_NO_VTABLE Dispatch : public Base
{
	typedef GENRESULT (__stdcall Type::*DA_PROC) (void);

#pragma pack (push, 1)

	struct _DACOM_DISPATCH_ENTRY
	{	
		const C8 *	name;
		U32			addr;				// address of property
		U8			size;				// size of property
		U8			type;				// record type
		U16			paramTypes[_DACOM_MAX_AUTO_PARMS];
		DA_PROC		proc;				// address of method
	};

#pragma pack ( pop )

	__dispatch_type_coercion<0> dtc;

	Dispatch (void)
	{
	}
	
	/* IDADispatch methods */
		 
	DEFMETHOD(Invoke) (const C8 *methodName, DACOM_VARIANT parm1 = DACOM_VARIANT() );
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
GENRESULT Dispatch<Type, Base>::Invoke (const C8 *methodName, DACOM_VARIANT parm1)
{
	U32 i;
	const _DACOM_DISPATCH_ENTRY * properties = Type::_GetAutomationEntries();
	void *instance = (void *) ( ((U32)this) - ((U32)
					(static_cast<Dispatch<Type, Base>*>((Type*)8) )
					-8) );

	for (i = 0; properties[i].name; i++)
	{
		if (properties[i].type == _DACOM_PROPTYPE_METHOD && 
			strcmp(properties[i].name, methodName) == 0)
		{
			DA_PROC _proc = properties[i].proc;
			void *proc;

			if (sizeof(_proc) > 4)
			__asm
			{
				mov eax, DWORD ptr [instance]
				mov edx, DWORD ptr [_proc+4]
				add eax, edx
				mov DWORD ptr [instance], eax
			}
			__asm
			{
				mov edx, DWORD ptr [_proc]
				mov DWORD PTR [proc], edx
			}
		
			return dtc.call_method(instance, (void *) (properties+i), proc, &parm1);
		}
	}

	if (*((U32*)methodName) == '_tes')
	for (i = 0; properties[i].name; i++)
	{
		if (properties[i].type != _DACOM_PROPTYPE_METHOD && 
			strcmp(properties[i].name, methodName+4) == 0)
		{
			return dtc.set_property(instance, (void *) (properties+i), parm1);
		}
	}
	else
	if (*((U32*)methodName) == '_teg')
	for (i = 0; properties[i].name; i++)
	{
		if (properties[i].type != _DACOM_PROPTYPE_METHOD && 
			strcmp(properties[i].name, methodName+4) == 0)
		{
			return dtc.get_property(instance, (void *) (properties+i), parm1);
		}
	}

	return GR_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------//
//
template <int dummy> 
GENRESULT __dispatch_type_coercion<dummy>::get_property (void *instance, void * _property, DACOM_VARIANT & value)
{
	_DUMMY_ENTRY * property = (_DUMMY_ENTRY *) _property;

	value.coerce((DAVARENUM)(property->paramTypes[0]|DAVT_BYREF));

	if (value.pVoid == 0)
		return GR_INVALID_PARMS;

	switch (property->type)
	{
	case _DACOM_PROPTYPE_ADDRESS:
		memcpy(value, (void *)property->addr, property->size);
		break;

	case _DACOM_PROPTYPE_OFFSET:
		{
			U32 address;

			address = ((U32)instance + property->addr);
			memcpy(value, (void *)address, property->size);
		}
		break;

	case _DACOM_PROPTYPE_CONST:
		memcpy(value, &property->addr, property->size);
		break;

	default:
		return GR_GENERIC;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
template <int dummy> 
GENRESULT __dispatch_type_coercion<dummy>::set_property (void *instance, void * _property, DACOM_VARIANT & value)
{
	_DUMMY_ENTRY * property = (_DUMMY_ENTRY *) _property;

	if ((value.variantType & DAVT_BYREF) == 0)
		value.coerce((DAVARENUM)property->paramTypes[0]);

	switch (property->type)
	{
	case _DACOM_PROPTYPE_ADDRESS:
		if (value.variantType & DAVT_BYREF)
			memcpy((void *)property->addr, value.pVoid, property->size);
		else
			memcpy((void *)property->addr, &value.longVal, property->size);
		break;

	case _DACOM_PROPTYPE_OFFSET:
		{
			U32 address;

			address = ((U32)instance + property->addr);

			if (value.variantType & DAVT_BYREF)
				memcpy((void *)address, value.pVoid, property->size);
			else
				memcpy((void *)address, &value.longVal, property->size);
		}
		break;

	case _DACOM_PROPTYPE_CONST:
	default:
		return GR_GENERIC;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
template <int dummy> 
GENRESULT __stdcall __dispatch_type_coercion<dummy>::call_method (void *instance, void * _property, void *proc, DACOM_VARIANT list[])
{
	_DUMMY_ENTRY * property = (_DUMMY_ENTRY *) _property;
	int i, iParmDwords, iBufferSize;
	U32 buffer[(sizeof(DACOM_VARIANT)*_DACOM_MAX_AUTO_PARMS) / sizeof(U32)];
	GENRESULT result;

	iBufferSize = sizeof(buffer);

	//
	// 1) convert passed in types to types expected by the method
	//
	for (i = 0; i < _DACOM_MAX_AUTO_PARMS; i++)
		list[i].coerce((DAVARENUM)property->paramTypes[i]);

	//
	// 2) assemble parameters on the stack
	//

	for (i = iParmDwords = 0; i < _DACOM_MAX_AUTO_PARMS; i++)
	{
		if (property->paramTypes[i] & DAVT_BYREF)
		{
			buffer[iParmDwords++] = list[i].longVal;
		}
		else
		switch (property->paramTypes[i] & DAVT_TYPEMASK)
		{
			case DAVT_VARIANT:
				*((DACOM_VARIANT *)(buffer+iParmDwords)) = list[i];
				iParmDwords += sizeof(DACOM_VARIANT) / sizeof(U32);
				break;

			case DAVT_DOUBLE:
			case DAVT_U64:
				*((U64 *)(buffer+iParmDwords)) = list[i].longLongVal;
				iParmDwords += 2;
				break;
			default:
				buffer[iParmDwords++] = list[i].longVal;
				break;
		}
	}


//; 269  : 	return (base->*(proc))();

	__asm
	{
		mov ebx, esp				// save the current stack location
		lea esi, DWORD PTR [buffer]
		mov ecx, DWORD PTR [iBufferSize]
		mov eax, DWORD PTR [instance]
		mov edx, DWORD PTR [proc]
		sub esp, ecx
		shr ecx, 2
		mov edi, esp
		rep movsd 
		mov ecx, eax
		push ecx					// push the 'this' pointer
		call edx					// call the method
		mov esp, ebx				// restore the stack position
		mov	DWORD ptr [result], eax
	}


	return result;
}
//--------------------------------------------------------------------------//
//-------------------------End TDispatch.h----------------------------------//
//--------------------------------------------------------------------------//
#endif