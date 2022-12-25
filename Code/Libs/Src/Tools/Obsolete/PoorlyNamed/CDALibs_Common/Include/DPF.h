// DPF.h
//





#ifndef DPF_H
#define DPF_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void DPF( char *fmt, ... ) 
{
	static char szOut[255+1];

	va_list args;

	va_start(args, fmt);
	vsprintf( szOut, fmt, args );
	va_end(args);

	OutputDebugString( szOut );
}

#endif
