#ifndef ISTRINGSET_H
#define ISTRINGSET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IStringSet.H                                   //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#ifndef DACOM_H
#include "DACOM.H"
#endif

struct STRINGSETDESC : public DACOMDESC
{
	STRINGSETDESC (const C8 *_interfaceName = "IStringSet") : DACOMDESC(_interfaceName)
	{
		size = sizeof(*this);
	};
};

//

#define IID_IStringSet MAKE_IID( "IStringSet", 1 )

//

struct IStringSetCallback
{
	virtual GENRESULT StringSetCallback(const char * string) = 0;
};

struct DACOM_NO_VTABLE IStringSet : public IDAComponent
{
	virtual GENRESULT COMAPI AddString (const char * string) = 0;

	virtual GENRESULT COMAPI RemoveString (const char * string) = 0;

	virtual BOOL32 COMAPI HasString (const char * string) = 0;

	virtual GENRESULT COMAPI Clear (void) = 0;

	virtual GENRESULT COMAPI CallBack(const char * string) = 0;

	virtual GENRESULT COMAPI SetCallBack(IStringSetCallback * callback) = 0;

	virtual const char* COMAPI GetString(const char* lastString) = 0;
};

//--------------------------------------------------------------------------//
//----------------------------End Viewer.h----------------------------------//
//--------------------------------------------------------------------------//

#endif