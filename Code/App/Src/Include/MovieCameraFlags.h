#ifndef MOVIECAMERAFLAGS_H
#define MOVIECAMERAFLAGS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               MovieCameraFlags.h                         //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/MovieCameraFlags.h 3     11/24/99 11:41a Jasony $
*/			    
//--------------------------------------------------------------------------//

#define MOVIE_CAMERA_ZERO		0
#define MOVIE_CAMERA_QUEUE		0x00000001
#define MOVIE_CAMERA_EMPTYQUEUE 0			//default is to empty queue
#define MOVIE_CAMERA_JUMP_TO	0x00000002
#define MOVIE_CAMERA_SLIDE_TO	0x00000000	// default
#define MOVIE_CAMERA_TRACK_SHIP	0x00000004	//use the cameras position only
#define MOVIE_CAMERA_FROM_SHIP	0x00000008	//use the cameras look position only

//--------------------------------------------------------------------------//
//------------------------End MovieCameraFlags.h----------------------------//
//--------------------------------------------------------------------------//

#endif
