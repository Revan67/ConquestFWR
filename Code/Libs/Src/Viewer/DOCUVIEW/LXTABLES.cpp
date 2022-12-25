//--------------------------------------------------------------------------//
//                                                                          //
//                               LxTables.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//


#include "lxtables.h"

//--------------------------------------------------------------------------//
//---------------------------GLOBAL TABLES----------------------------------//

char specchar[] = "#+-*/:=<>^.,;()[]{}!%&\\";   /* initialize special characters */

int CHARCLASS[MAXCHARCLASS];              /* character class array */
int NUMBERCLASS[MAXCHARCLASS];

UBYTE CONVERTCASE[MAXCHARCLASS];
 
//--------------------------------------------------------------------------//
static void init_charclass() /* initialize character class array */
  { int i;
    for (i = 0; i < MAXCHARCLASS; ++i) CHARCLASS[i] = NOTHING;
    for (i = 'A'; i <= 'Z'; ++i)       CHARCLASS[i] = ALPHA;
    for (i = 'a'; i <= 'z'; ++i)       CHARCLASS[i] = ALPHA;
    for (i = '0'; i <= '9'; ++i)       CHARCLASS[i] = NUMBERCLASS[i] = NUMERIC;
    for (i = 'a'; i <= 'f'; ++i)       NUMBERCLASS[i] = NUMERIC;
    for (i = 'A'; i <= 'F'; ++i)       NUMBERCLASS[i] = NUMERIC;
    for (i = 0 ; specchar[i] != '\0';  ++i) CHARCLASS[specchar[i]] = SPECIAL;
	CHARCLASS['_'] = ALPHA;
    }
//--------------------------------------------------------------------------//
static void init_convert_case(void)
{
	int i;

	for (i = 0; i < MAXCHARCLASS; i++)
		CONVERTCASE[i] = UBYTE(i);
    for (i = 'a'; i <= 'z'; ++i)
		CONVERTCASE[i] &= 0xDF;
}	

//--------------------------------------------------------------------------//
void InitTables(void)
{
	init_charclass();
	init_convert_case();
}
	
//--------------------------------------------------------------------------//
//------------------------------End LxTables.cpp----------------------------//
//--------------------------------------------------------------------------//

