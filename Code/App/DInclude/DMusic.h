#ifndef DMUSIC_H
#define DMUSIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DMusic.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/DInclude/DMusic.h 5     8/14/00 12:38p Jasony $
*/			    
//--------------------------------------------------------------------------//
#ifndef DTYPES_H
#include "DTypes.h"
#endif

#define NUM_SONGS 4
typedef struct Streamer *HSTREAM;

struct SONG
{
	bool  playing:1;
	bool  looping:1;
	HSTREAM handle;
	SINGLE volume;
	char filename[32];
};

typedef SONG MT_MUSIC_DATA[NUM_SONGS];


#endif
