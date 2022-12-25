#ifndef IMOVIECAMERA_H
#define IMOVIECAMERA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IMovieCamera.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IMovieCamera.h 5     4/26/00 1:52p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IMovieCamera : IObject
{
	virtual void InitCamera (void) = 0;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IMovieCameraManager : public IDAComponent
{
	//for switching to a specific camera object
	virtual void ChangeCamera (IBaseObject * camera, SINGLE time = 0.0, U32 flags = 0) = 0;

	//for moving to a normal location(vied just like it was in the game)
	virtual void MoveCamera (Vector * location,U32 systemID,SINGLE time = 0.0, U32 flags = 0) = 0;
	
	//for moving to an object from the regular game view.
	virtual void MoveCamera (IBaseObject * location, SINGLE time = 0.0, U32 flags = 0) = 0;

	//Sets the source ship to be used with an upcomming ChangeCamera call
	virtual void SetSourceShip (IBaseObject * sourceShip, Vector * offset=0) = 0;

	//Sets the target ship to be used with an upcomming Change Camera call
	virtual void SetTargetShip (IBaseObject * targetShip) = 0;

	//Clears the Queue of all Camera calls
	virtual void ClearQueue (void) = 0;

	//Saves the current position of the camera
	virtual void SaveCameraPos (void) = 0;

	//Creates a ChangeCamera call that used the saved camera
	virtual void LoadCameraPos (SINGLE time = 0.0, U32 flags = 0) = 0;

	virtual void PhysUpdate (SINGLE dt) = 0;

	virtual BOOL32 Save (struct IFileSystem * file) = 0;

	virtual BOOL32 Load (struct IFileSystem * file) = 0;

	virtual void ResolveMovieCamera() = 0;

};
//---------------------------------------------------------------------------
//--------------------------END IMovieCamera.h------------------------------------
//---------------------------------------------------------------------------
#endif
