#ifndef PARSERRS_H
#define PARSERRS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Parserrs.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//


// error codes for PARSING...

#define BAD_PARSE			0x10007
#define SEMICOLON_EXPECTED	0x10008
#define STATEMENT_EXPECTED	0x10009
#define END_EXPECTED		0x1000A
#define IDENTIFIER_EXPECTED 0x1000B
#define EQUALS_EXPECTED		0x1000C
#define NUMBER_EXPECTED  	0x1000D
#define COLON_EXPECTED  	0x1000E
#define TYPE_EXPECTED		0x1000F
#define VARIABLE_EXPECTED	0x10010
#define CONST_EXPECTED		0x10011
#define UNSUPPORTED_RESERVED 0x10012
#define BAD_TYPE			0x10013
#define UNDEFINED_SYMBOL	0x10014
#define DIVIDE_OVERFLOW		0x10015
#define CONSTANT_EXPRESSION 0x10016
#define ASSIGN_CONSTANT		0x10017
#define BAD_NPARAMETERS		0x10018
#define BAD_TPARAMETERS		0x10019
#define BAD_LABEL			0x1001A
#define BAD_SUBRANGE		0x1001B
#define LABEL_USED_TWICE	0x1001C
#define THEN_EXPECTED		0x1001D
#define BITFIELD_TOO_LARGE	0x1001E
#define NONALIGNED_BITFIELD	0x1001F
#define BITFIELD_OVERFLOW	0x10020
#define DUPLICATE_SYMBOL	0x10021



//--------------------------------------------------------------------------//
//----------------------------End Parserrs.h--------------------------------//
//--------------------------------------------------------------------------//
#endif