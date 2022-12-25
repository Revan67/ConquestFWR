#ifndef FERROR_H
#define FERROR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               FError.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//
 
/*
	Error reporting functions.

	void Fatal(char *fmt, ...);
	OUTPUT:
		Calls printf() with the parameters, then calls exit().

//--------------------------------------------------------------------------//

	void fatal(int error_code, char warning, char *msg=0);
	INPUT:
		error_code: Predefined error code.
		warning: set non-zero if this is just a warning
		msg: optional message to also display along with 
			predefined string.
	OUTPUT:
		Prints error message and returns. Keeps a count of errors.
		If the total number of errors exceedes MAX_ERRORS, it will
		call Fatal(), and control will not return to the caller.
	
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define MAX_ERRORS	10

#define DEFAULT_ERROR	  	0xFFFF
#define UNEXPECTED_EOF	  	0x10000
#define BAD_STRING_FORMAT 	0x10001
#define OUT_OF_STRINGS		0x10002
#define OUT_OF_TSTRINGS		0x10003
#define BAD_NUMERIC_FORMAT  0x10004
#define TOO_MANY_LEVELS		0x10005
#define DOUBLE_DEFINE_VAR	0x10006

extern int num_errors;	// number of errors encountered

void fatal(int error_code, char warning=0, char *msg=0);


//--------------------------------------------------------------------------//
//-----------------------------End FError.h---------------------------------//
//--------------------------------------------------------------------------//

#endif
