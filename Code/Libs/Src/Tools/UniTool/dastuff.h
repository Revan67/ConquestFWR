#ifndef DASTUFF_H
#define DASTUFF_H
//
// DAStuff.h - Header for all of the standard DA things provided by UniTool
//

//
// Design Notes:
//     This should be turned into a plugin at some point, but for now it will be compiled straight in.
//

//
// Include files
//

#include "typedefs.h"

//
// Global variables
//

extern SINGLE frameTime;

//
// Global functions
//

extern bool dastuff_open (const char *ini_filename);
extern void dastuff_close ();
extern bool dastuff_update ();

#endif
