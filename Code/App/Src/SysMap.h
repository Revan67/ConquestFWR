#ifndef SYSMAP_H
#define SYSMAP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                SysMap.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/SysMap.h 25    8/11/00 12:14p Tmauer $

*/
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

#ifndef GRIDVECTOR_H
#include "Gridvector.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ISystemMap : public IDAComponent
{
	DEFMETHOD(SetRect) (struct tagRECT * rect) = 0;

	virtual void GetSysMapRect(int sys,struct tagRECT * rect) = 0;

	DEFMETHOD(ScrollUp) (BOOL32 bSingleStep=1) = 0;

	DEFMETHOD(ScrollDown) (BOOL32 bSingleStep=1) = 0;

	DEFMETHOD(ScrollLeft) (BOOL32 bSingleStep=1) = 0;

	DEFMETHOD(ScrollRight) (BOOL32 bSingleStep=1) = 0;

	DEFMETHOD(ZoneOn) (const Vector & pos) = 0;

	virtual void RegisterAttack(U32 systemID, GRIDVECTOR pos,U32 playerID) = 0;

	virtual S32 GetLastFrameTime () = 0;

	virtual U64 GetMapTiming () = 0;

	virtual const Transform &GetMapTrans () = 0;

	virtual void SetScrollRate(SINGLE rate) = 0;

	virtual SINGLE GetScrollRate (void) = 0;

	// methods call by toolbar
	virtual void BeginFullScreen (void) = 0;

	virtual GENRESULT __stdcall Notify (U32 message, void *param) = 0;

	virtual void InvalidateMap(U32 systemID) = 0;

//render functions should only be called durring a MapRender Call

	virtual void DrawCircle(Vector worldPos, SINGLE worldSize, COLORREF color) = 0;

	virtual void DrawRing(Vector worldPos, SINGLE worldSize, U32 width, COLORREF color) = 0;

	virtual void DrawSquare(Vector worldPos, SINGLE worldSize, COLORREF color) = 0;	

	virtual void DrawHashSquare(Vector worldPos, SINGLE worldSize, COLORREF color);

	virtual void DrawArc(Vector worldPos,SINGLE worldSize,SINGLE ang1, SINGLE ang2, U32 width, COLORREF color) = 0;

	virtual void DrawWaypoint(Vector worldPos, U32 frameMod) = 0;

	virtual void DrawMissionAnim(Vector worldPos, U32 frameMod) = 0;

	virtual void DrawIcon(Vector worldPos,SINGLE worldSize,U32 iconID) = 0;

	virtual void DrawPlayerIcon(Vector worldPos,SINGLE worldSize,U32 iconID,U32 playerID) = 0;

//these functions are for registering icon types with the sysmap
	virtual U32 RegisterIcon(char * filename) = 0;

	virtual U32 RegisterPlayerIcon(char * filename) = 0;

	virtual U32 GetPadding(U32 systemID) = 0;

	virtual void PingSystem(U32 systemID, Vector position, U32 playerID) = 0;

	virtual U32 CreateMissionAnim(U32 systemID, GRIDVECTOR position) = 0;
	
	virtual void StopMissionAnim(U32 animID) = 0;

	virtual void FlushMissionAnims() = 0;

	virtual void Save(struct IFileSystem * outFile) = 0;

	virtual void Load(struct IFileSystem * inFile) = 0;
};

//teture map size
#define TMAP_SIZE  512

#define TEX_PADDING 15
#define TEX_PAD_RATIO (((SINGLE)TEX_PADDING)/TMAP_SIZE)

//--------------------------------------------------------------------------//
//------------------------------End SysMap.h--------------------------------//
//--------------------------------------------------------------------------//
#endif