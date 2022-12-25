#ifndef LXTABLES_H
#define LXTABLES_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               LxTables.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//

/*
	Lex Tables. Various tables and access functions that speed up
	lexigraphical analysis of input file.

//--------------------------------------------------------------------------//
	void InitTables(void);
	OUTPUT:
		Sets up whatever global tables we are going to use later.

//--------------------------------------------------------------------------//
*/

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define MAXCHARCLASS 256 /* limit for character class array */

#define NOTHING 0
#define ALPHA   1        /* character class values */  
#define NUMERIC 2
#define SPECIAL 3

typedef unsigned char UBYTE;

extern int CHARCLASS[MAXCHARCLASS];              /* character class array */
extern UBYTE CONVERTCASE[MAXCHARCLASS];
extern int NUMBERCLASS[MAXCHARCLASS];

extern char *opprnt[];		// opperator names
extern char *delprnt[];		// delimeter names
extern char *resprnt[];		// reserved names

void InitTables(void);

//--------------------------------------------------------------------------//
//---------------------------End LxTables.h---------------------------------//
//--------------------------------------------------------------------------//
#endif