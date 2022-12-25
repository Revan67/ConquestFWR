#ifndef MOVIEPLAY_H
#define MOVIEPLAY_H
//
// MoviePlay.h - Movie playing routines header
//

//
// Design Notes:
//    This header will include interfaces and routines for playing movie files using the DA libraries.
// The availble functionality will include simple blocking movie playing using a single function, and
// more advanced kinds of movie playing, including playing to a texture and playing to an offscreen surface.
//

//
// Include files
//

#include <rendpipeline.h>

//
// Simple type definitions
//

// Return true to continue movie playback, false otherwise.
// You are passed a pointer to the destination rectangle used for playback. You may modify it
// to change where the next frame will be drawn. This is required if you are playing back using the
// render pipeline's windowed mode; if the window moves, you must change the rectangle to match the
// client area of your main window.
typedef bool PlayMovieCallback (RECT *destRect);

//
// Functions
//

// Plays the given movie file onto the primary surface. It defaults to fullscreen, but will
// play into the given rectangle if it is provided.
// WARNING: You must call create_buffers() before using this function, because it gets the DirectDraw and
// primary surface pointers from the pipeline.
// This function is blocking, so it will not return until it fails, or the movie is complete.
// Returns:
//     DD_OK if all is well
//     A DirectDraw error code on failure.
extern HRESULT PlayMovie (IRenderPipeline *PIPE, const char *filename, RECT *destRect = NULL, PlayMovieCallback *callback=NULL);

#endif
