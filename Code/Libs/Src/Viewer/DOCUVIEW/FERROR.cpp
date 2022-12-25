//--------------------------------------------------------------------------//
//                                                                          //
//                                FError.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ferror.h"
#include "ParsErrs.h"
#include "fdump.h"

//#include <stdio.h>
//#include <stdarg.h>
//#include <stdlib.h>

// Static variables

extern int lineno;
int num_errors = 0;
extern int EOFFLG;

/*
void Fatal (char *fmt, ...)
{
	char ExitWorkString[512];        // Holds the exit string.

    va_list ap;
    va_start(ap, fmt);
    wvsprintf (ExitWorkString, fmt, ap);
    va_end(ap);

	if (DUMP)
		DUMP->bomb("DataViewer", ExitWorkString); 
	EOFFLG = 1;
//    ::exit(1);
}
*/
//---------------------------------------------------

void fatal(int error_code, char warning, char *msg)
{
#if DA_ERROR_LEVEL >= __SEV_ERROR
	char *standard_msg = "UNKNOWN at line %d ";
//	char buffer[64];

	if (warning)
		standard_msg = "Line #%d: ";	

	switch (error_code)
	{
		case DUPLICATE_SYMBOL:
			standard_msg = "PARSER: Duplicate symbol at line %d: \"%s\"";
			break;
		case THEN_EXPECTED:
			standard_msg = "PARSER: THEN expected at line %d.";
			break;
		case LABEL_USED_TWICE:
			standard_msg = "PARSER: LABEL used more than once at line %d.";
			break;
		case BAD_SUBRANGE:
			standard_msg = "PARSER: Assignment out of bounds of subrange at line %d.";
			break;
		case BAD_LABEL:
			standard_msg = "PARSER: Invalid LABEL used at line %d.";
			break;
		case BAD_TPARAMETERS:
			standard_msg = "PARSER: Type mismatch in function parameters at line %d.";
			break;
		case BAD_NPARAMETERS:
			standard_msg = "PARSER: Incorrect number of parameters to function call at line %d.";
			break;
		case ASSIGN_CONSTANT:
			standard_msg = "PARSER: Attempt to assign a constant at line %d.";
			break;
		case DIVIDE_OVERFLOW:
			standard_msg = "PARSER: Divide overflow at line %d.";
			break;
		case CONSTANT_EXPRESSION:
			standard_msg = "PARSER: Constant expression at line %d.";
			break;
		case UNDEFINED_SYMBOL:
			standard_msg = "PARSER: Undefined symbol at line %d: \"%s\"";
			break;
		case BAD_TYPE:
			standard_msg = "PARSER: Bad type identifier at line %d.\n";
			break;
		case UNSUPPORTED_RESERVED:
			standard_msg = "PARSER: Line %d: Reserved word not supported: \"%s\"";
			break;
		case CONST_EXPECTED:
			standard_msg = "PARSER: Constant expected at line %d.\n";
			break;
		case IDENTIFIER_EXPECTED:
			standard_msg = "PARSER: Identifier expected at line %d.\n";
			break;
		case VARIABLE_EXPECTED:
			standard_msg = "PARSER: Variable symbol expected at line %d.\n";
			break;
		case TYPE_EXPECTED:
			standard_msg = "PARSER: TYPE symbol expected at line %d.\n";
			break;
		case COLON_EXPECTED:
			standard_msg = "PARSER: Colon expected at line %d.\n";
			break;
		case EQUALS_EXPECTED:
			standard_msg = "PARSER: Equals expected at line %d.\n";
			break;
		case NUMBER_EXPECTED:
			standard_msg = "PARSER: Number expected at line %d.\n";
			break;
		case STATEMENT_EXPECTED:
			standard_msg = "PARSER: Statement expected at line %d.\n";
			break;
		case END_EXPECTED:
			standard_msg = "PARSER: END keyword expected at line %d.\n";
			break;
		case TOO_MANY_LEVELS:
			standard_msg = "PARSER: Too many levels at line %d.\n";
			break;
		case DOUBLE_DEFINE_VAR:
			standard_msg = "PARSER: Doubly defined variable at line %d.\n";
			break;
		case BAD_PARSE:
			standard_msg = "PARSER: Parse error at line %d.\n";
			break;
		case SEMICOLON_EXPECTED:
			standard_msg = "PARSER: Semicolon expected at line %d.\n";
			break;
		case BAD_NUMERIC_FORMAT:
			standard_msg = "PARSER: Bad numeric format at line %d.\n";
			break;
		case OUT_OF_TSTRINGS:
			standard_msg = "PARSER: Out of IDENTIFIER space.\n";
			break;
		case OUT_OF_STRINGS:
			standard_msg = "PARSER: Out of string space.\n";
			break;
		case UNEXPECTED_EOF:
			standard_msg = "PARSER: Unexpected end of file (EOF) at line %d.\n";
			break;
		case BAD_STRING_FORMAT:
			standard_msg = "PARSER: Unexpected end of line in string at line %d.\n";
			break;
		case BITFIELD_TOO_LARGE:
			standard_msg = "PARSER: Bitfield size exceeds base type's size at line %d.\n";
			break;
		case NONALIGNED_BITFIELD:
			standard_msg = "PARSER: Non-aligned bitfield detected at line %d. Flushing extra bits.\n";
			break;
		case BITFIELD_OVERFLOW:
			standard_msg = "PARSER: Bitfield overflows size boundary at line %d. Flushing extra bits.\n";
			break;
	}

	if (!warning)
		FDUMP(ErrorCode(ERR_PARSER, SEV_ERROR), standard_msg, lineno, msg);
	
#if DA_ERROR_LEVEL >= __SEV_WARNING
	if (error_code == NONALIGNED_BITFIELD)
		FDUMP(ErrorCode(ERR_PARSER, SEV_TRACE_1), standard_msg, lineno, msg);
	else
	if (warning)
		FDUMP(ErrorCode(ERR_PARSER, SEV_WARNING), standard_msg, lineno, msg);
#endif  // DA_ERROR_LEVEL >= __SEV_WARNING

#endif	// DA_ERROR_LEVEL >= __SEV_ERROR

	if (!warning)
		num_errors++;

	if (num_errors >= MAX_ERRORS)
		FDUMP(ErrorCode(ERR_PARSER, SEV_FATAL), "PARSER TERMINATING! EXCEEDED MAX ERRORS!");

}

//--------------------------------------------------------------------------//
//----------------------------End Ferror.cpp--------------------------------//
//--------------------------------------------------------------------------//
