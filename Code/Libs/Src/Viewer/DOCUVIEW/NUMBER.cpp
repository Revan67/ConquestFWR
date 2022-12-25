//--------------------------------------------------------------------------//
//                                                                          //
//                               Number.cpp                                 //
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
#include "FileSys.h"

#include "tokendef.h"
#include "prttoken.h"
#include "lxtables.h"
#include "filestrm.h"
#include "ferror.h"

extern FileStream stream;
#define MAX_POWER 38

double positive_powers[] = 
{
	1.0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10,
	1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20,
	1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30,
	1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38
};
double negative_powers[] = 
{
	1.0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9, 1e-10,
	1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16, 1e-17, 1e-18, 1e-19, 1e-20,
	1e-21, 1e-22, 1e-23, 1e-24, 1e-25, 1e-26, 1e-27, 1e-28, 1e-29, 1e-30,
	1e-31, 1e-32, 1e-33, 1e-34, 1e-35, 1e-36, 1e-37, 1e-38
};


//-----------------------------------------------------------------
// read an integer number from the input stream, return its value
// assumes that the first character in the stream is a number
// returns the number read from stream, digits = number of digits
// taken from the input stream. There could still be some numbers
// in the input stream if we were about to overflow.
//
static unsigned read_number (int & digits)
{
	unsigned result = 0;
	unsigned test;
	unsigned base = 10;
	unsigned limit = 0xFFFFFFFF/10;
	UBYTE c,cc;

	c = stream.peek_char();
	cc = stream.peek2_char();

	if (c == '0' && CHARCLASS[cc] == NUMERIC)
	{
		stream.get_char();
		c = stream.peek_char();
		base = 8;
		limit = 0xFFFFFFFF/8;
	}
	else
	if (c == '0' && (cc == 'x' || cc == 'X'))
	{
		stream.get_char();
		stream.get_char();
		c = stream.peek_char();
		base = 16;
		limit = 0xFFFFFFFF/16;
	}
			
	while (CHARCLASS[c] == NUMERIC || (base==16 && NUMBERCLASS[c] == NUMERIC))
	{
		if (result > limit)	
			break;
		test = result * base;
		if (CHARCLASS[c] == NUMERIC)
			c -= '0';
		else
			c = UBYTE ((c & ~0x20) - 'A' + 10);
		test += c;
		if (test < c)	// we rolled over
			break;
		result = test;
		c = stream.get_char();
		c = stream.peek_char();
		digits++;
	}

	return result;
}

//------------------------------------------------------------
//
TOKEN number (TOKEN tok)
{
	UBYTE c;
	int digits=0, total_digits=0;
	UBYTE int_overflow = 0;			// set true if potention integer overflow
	double temp, accum;
	int float_shifts_needed = 0;	// positive means use positve powers

	tok->tokentype = NUMBERTOK;
	tok->datatype = INTEGER;
	tok->intval = read_number (digits);

	c = stream.peek_char();
	if (CHARCLASS[c] == NUMERIC)		// uh oh! overflow time
	{
		accum = (double) ((unsigned)tok->intval);
		int_overflow = 1;
		
		while (CHARCLASS[c] == NUMERIC)
		{
			total_digits += digits;
			digits = 0;
			temp = (double) read_number(digits);
			if (total_digits+digits <= MAX_POWER)
			{
				accum *= positive_powers[digits];
				accum += temp;
			}
			else
				float_shifts_needed =+ digits;
			c = stream.peek_char();
		}
		tok->datatype = REAL;
		tok->realval = accum;
	}

	if (c == '.' && stream.peek2_char() != '.')	 // watch out for '..'
	{
		int_overflow = 0;
		digits = 0;
		accum = 0.0;
	 	stream.get_char();		// eat the '.'
		c = stream.peek_char();
		if (CHARCLASS[c] != NUMERIC)
			fatal(BAD_NUMERIC_FORMAT);
		else
		{
			char first_time_through = 1;
			while (CHARCLASS[c] == NUMERIC)
			{
				temp = (double) read_number(digits);
				if ((float_shifts_needed==0) && (first_time_through) && 
					(tok->datatype == INTEGER) && (tok->intval == 0))
				{
				 	float_shifts_needed = -digits;
					digits = 0;
				}
				if (digits <= MAX_POWER)
					accum += (temp * negative_powers[digits]);
				c = stream.peek_char();
				first_time_through = 0;
			}
		}
		if (tok->datatype != REAL)
		{
			tok->datatype = REAL;
			tok->realval = ((double)((unsigned)tok->intval)) + accum;
		}
		else
		{
		 	tok->realval += accum;
		}
	}	

	if (CONVERTCASE[c] == 'E')
	{
		int e_val = 1;

		int_overflow = 0;
		if (tok->datatype != REAL)
		{
			tok->realval = ((double)((unsigned)tok->intval));
			tok->datatype = REAL;
		}
	
		stream.get_char();	// eat the 'E'
		c = stream.peek_char();
		if (c == '+')
		{
			stream.get_char();		// eat the '+'
			c = stream.peek_char();
		}
		else
		if (c == '-')
		{
			stream.get_char();		// eat the '-'
			c = stream.peek_char();
			e_val = -1;
		}
		if (CHARCLASS[c] != NUMERIC)
			fatal(BAD_NUMERIC_FORMAT);
		else
		{
			e_val *= read_number(digits);
			c = stream.peek_char();
			if (CHARCLASS[c] == NUMERIC)
			{
				while (CHARCLASS[c] == NUMERIC)
				{
					read_number(digits);	// flush the rest of the numbers
					c = stream.peek_char();
				}
				e_val = 1000;	// large number
			}
			e_val += float_shifts_needed;
			if (e_val >= 0 && e_val <= MAX_POWER)
			{
				tok->realval *= positive_powers[e_val];
			}
			else
			if (e_val < 0 && e_val >= -MAX_POWER)
			{
				tok->realval *= negative_powers[-e_val];
			}
			else
				fatal(0,1, "Exponent exceeds acceptable +/-38 range.\n");
		}
	}
	else  // no 'E'
	if (float_shifts_needed>=0)
	{
		if (tok->datatype == REAL)
		 	tok->realval *= positive_powers[float_shifts_needed];
	}
	else
	if (float_shifts_needed < 0)
	{
	 	tok->realval *= negative_powers[-float_shifts_needed];
	}

	if (int_overflow)
	{
		fatal(0,1, "Number exceeds 32 bit range.\n");
		tok->datatype = INTEGER;
		tok->intval = ((unsigned)tok->realval);
	}

	return tok;
}

//--------------------------------------------------------------------------//
//----------------------------End Number.cpp--------------------------------//
//--------------------------------------------------------------------------//
