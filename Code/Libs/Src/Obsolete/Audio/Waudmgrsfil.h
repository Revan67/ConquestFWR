//############################################################################
//##                                                                        ##
//##  Digital Anvil Hacked file from Miles Sound System                     ##
//##                                                                        ##
//##  Waudmgrsfil.c																			 ##
//##                                                                        ##
//##  Hacked version of																		 ##
//##      WAILSFIL.C: Digital sound API module for digital sound file access##
//##		 Hacks allow for the return of various information parsed from the ##
//##      file: channels, sample_rate, bits_per_sample, sample_data, and    ##
//##      sample_size.                                                      ##
//##                                                                        ##                                                                      ##
//##  Author: John Miles, hacks by Gary Boswood                             ##
//##                                                                        ##
//############################################################################

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include "mssw.h"

class AudioFile;

//############################################################################
//##                                                                        ##
//## Set parameters of existing HSAMPLE according to file data              ##
//##                                                                        ##
//## Returns 0 on error, else 1                                             ##
//##                                                                        ##
//############################################################################

extern S32 AUDMGR_set_sample_file(HSAMPLE S,
											 AudioFile* file,
											 S32    block,
											 S16*   channels			= NULL,
											 S32*   sample_rate		= NULL,
											 S16*   bits_per_sample	= NULL,
											 S32*   sample_size		= NULL,
											 S32*   sample_offset   = NULL);
