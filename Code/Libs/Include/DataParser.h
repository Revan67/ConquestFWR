#ifndef DATAPARSER_H
#define DATAPARSER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DataParser.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	DataParser objects are created by the View Constructor. (See ViewCnst.h)
  
	$Header: /Conquest/Libs/Include/DataParser.h 5     4/28/00 11:57p Rmarr $
*/			    
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include "DACOM.H"
#endif

struct IDataParser;
struct Symbol;
typedef Symbol *SYMBOL;
//--------------------------------------------------------------------------//
// structure used to create an instance of a DataParser
//
struct DPARSERDESC : public DACOMDESC
{
	SYMBOL symbol;

	DPARSERDESC (const C8 *_interfaceName = "IDataParser") : DACOMDESC(_interfaceName)
	{
		memset(((char *)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));
		size = sizeof(*this);
	};
};
//--------------------------------------------------------------------------//
// describes a variable instance within the structure that is being parsed
//
struct VARIABLEDESC
{
	const char *varName;		// can be NULL!
	const char *typeName;		// can be NULL!

	enum KIND
	{
		BASIC,					// int, float, enum
		ARRAY,					// []
		RECORD					// struct or union
	} kind;

	U32 offset;					// relative to top of local structure
	U32 size;					// size of this variable
	U32 arraySize;				// valid if instance is an array, number of elements in the array
	U8	bitOffset;				// valid on basic data types
	U8  bitCount;				// valid on basic data types
};
//--------------------------------------------------------------------------//
//
#define IID_IParserCallback MAKE_IID("IParserCallback",1)
//
struct DACOM_NO_VTABLE IParserCallback 
{
	// "newParser" will be valid if the currect variable is a struct or array,
	// else it will be NULL for basic types.
	// Use the "newParser" if you want to parse the internals of the variable.
	// The callback should return TRUE to continue the enumeration, or false to stop now.
	virtual BOOL32 __stdcall VarInstance (IDataParser *newParser, const VARIABLEDESC & varDesc) = 0;
};
//--------------------------------------------------------------------------//
//

#define IID_IDataParser MAKE_IID("IDataParser", 1 )

struct DACOM_NO_VTABLE IDataParser : public IDAComponent
{
	// returns TRUE if the enumeration completed, FALSE if the user defined callback returned FALSE to stop early.
	virtual BOOL32 __stdcall Enumerate (IParserCallback * callback) = 0;
};

//--------------------------------------------------------------------------//
//----------------------------End DataParser.h------------------------------//
//--------------------------------------------------------------------------//

#endif