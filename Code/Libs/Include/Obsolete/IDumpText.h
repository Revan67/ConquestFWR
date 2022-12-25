#ifndef IDUMPTEXT_H
#define IDUMPTEXT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IDumpText.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Libs/Include/IDumpText.h 5     2/17/98 11:59a Jasony $
*/			    
//--------------------------------------------------------------------------//
//
/*
			    Interface used for debugging purposes.

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

  //-----------------------------
   //
   void debug_printf (const char *fmt, ...);
	  INPUT:
	   	 fmt: Formatting string.
	  OUTPUT:
		 Sends the result of doing a printf() to the debugging output stream.

   //-----------------------------
   //
   void alert_box (const C8 *caption, const C8 *fmt, ...);
	  INPUT:
		 caption: ASCIIZ string for the Dialog title bar. (Can be NULL for a default "Error" msg)
	   	 fmt: Formatting string.
	  OUTPUT:
		 Sends the result of doing a printf() to the text a dialog box. After the user clicks 'OK', 
		 the method returns.

   //-----------------------------
   //
   void bomb (const C8 *caption, const C8 *fmt, ...);
	  INPUT:
		 caption: ASCIIZ string for the Dialog title bar. (Can be NULL for a default "Error" msg)
	   	 fmt: Formatting string.
	  OUTPUT:
		 Same as alert_box() method except this function does NOT return.
		 After the user clicks OK to continue, the routine calls exit().

*/
//---------------------------------------------------------------------------


#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

//--------------------------------------------------------------------
//--------------------------------------------------------------------

struct DACOM_NO_VTABLE IDumpText : public IDAComponent
{
	virtual void __cdecl debug_printf (const char *fmt, ...) = 0;

    virtual void __cdecl alert_box (const C8 *caption, const C8 *fmt, ...) = 0;

    virtual void __cdecl bomb (const C8 *caption, const C8 *fmt, ...) = 0;
};

//--------------------------------------------------------------------
//---------------------DA replacement for ASSERT----------------------
//--------------------------------------------------------------------

#undef  assert

#ifdef  FINAL_RELEASE

#define assert(exp)     ((void)0)

#else

#ifdef  __cplusplus
extern "C" {
#endif

void __cdecl _assert(void *, void *, unsigned);

#ifdef  __cplusplus
}
#endif

template <int i> 
void _da_assert (void *exp, void *file, unsigned line)
{
	if (DUMP)
		DUMP->bomb("Assertion Failed!", "%s(%d) : \"%s\"", file, line, exp);
	_assert(exp, file, line);
}

#define assert(exp) (void)( (exp) || (_da_assert<0>(#exp, __FILE__, __LINE__), 0) )

#endif  /* FINAL_RELEASE */



//----------------------------------------------------------------------------
//------------------------End IDumpText.h-------------------------------------
//----------------------------------------------------------------------------
#endif
