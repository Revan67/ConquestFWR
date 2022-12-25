#ifndef RUSEMAP_H
#define RUSEMAP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               RuseMap.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RuseMap.h 22    11/15/00 1:03p Jasony $
*/			    
//--------------------------------------------------------------------------//
#define MAX_GATES 15

#define INVALID_BASE -10000000
#define INVALID_SYSTEM_ID 0

#ifndef FILESYS_H
#include <FileSys.h>
#endif

#ifndef DSECTOR_H
#include <DSector.h>
#endif

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#ifndef DQUICKSAVE_H
#include <DQuickSave.h>
#endif

//#include "MPart.h"

#ifndef HEAPOBJ_H
#include <HeapObj.h>
#endif

struct GateLink;
struct System;
struct Jump;

extern BOOL32 bMapChanged;
extern U32 sysTextureID;

#define MAX_SYSTEMS 16
#define FULL_MAP 0x1FFFE			// (availableSystemIDs == FULL_MAP)


struct DRAGGER
{
public:
	//virtual DRAGGER();
	virtual	~DRAGGER();
	virtual void Draw() = 0;
//	virtual BOOL32 Save() = 0;
//	virtual BOOL32 Load() = 0;
	virtual void SetAnchor(S32 ax,S32 ay) = 0;
	virtual void Move(S32 mX,S32 mY) = 0;
	virtual void SetXY(S32 x,S32 y) = 0;
	virtual void GetXY(S32 *x,S32 *y) = 0;
	virtual BOOL32 MouseIn() = 0;
	virtual BOOL32 IsSystem();

	bool bIndestructable;		// true if loaded from disk

    void * operator new (size_t size)
	{
		bMapChanged = 1;
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	S32 anchorX,anchorY;
};

struct Jump : public DRAGGER//,ProtoJump
{
public:
	Jump(S32 locX,S32 locY,System *syst,GateLink *gLink);
	Jump();//GateLink *gLink);
	virtual ~Jump();
	void GetScreenLoc(S32 *lx,S32*ly);
	void Draw();
	virtual void SetAnchor(S32 ax,S32 ay);
	void Move(S32 mX,S32 mY);
	virtual void SetXY(S32 x,S32 y);
	virtual void GetXY(S32 *x,S32 *y);
	virtual BOOL32 MouseIn();
	BOOL32 Save(IFileSystem *file);
	BOOL32 Load(IFileSystem *file);

	//JUMPGATE_SAVELOAD d;
	MT_QJGATELOAD d;
	S32 x,y;
	S32 startX,startY;
	System *system;
	GateLink *link;
	U32 archID;
	bool bJumpAllowed;
	M_STRING partName;

//	MPart part;

    void * operator new (size_t size)
	{
		bMapChanged = 1;
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};

struct GateLink
{

public:
	GateLink(System *A,System *B,GateLink *lastL);
	GateLink(GateLink *lastL);
	virtual ~GateLink();
	void CutMe(Jump * Jump);
	void Draw();
	BOOL32 MouseOn();
	void GenerateJumps(System *A,System *B);
	void GenerateJumpsXY(System *A,System *B,const Vector &a,const Vector &b);
	BOOL32 Save(IFileSystem * outFile);
//	BOOL32 Load(IFileSystem *file);
	U32 GetID();
	
	GateLink *next,*last;
	Jump *jump1,*jump2;
	BOOL32 alive;

//	GATELINK_DATA d;
	U32 gateID1,gateID2;
	U32 end_id1,end_id2;
	System *end1,*end2;

	bool bIndestructable;		// true if loaded from disk

    void * operator new (size_t size)
	{
		bMapChanged = 1;
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};



struct System : public DRAGGER
{
	
public:
	System(S32 locX,S32 locY,System *lastS);
	virtual ~System();
	void ClearSizeBuf();
	void AddLink(GateLink *link);
	void Detach(GateLink *link);
	void Draw();
	void Draw2D();
	BOOL32 MouseIn();
	BOOL32 GotEdge();
//	void GetScreenCenter(S32 *cx,S32 *cy);
	void GetOrigin(S32 *ox,S32 *oy);
	U32 GetID();
	virtual void SetAnchor(S32 ax,S32 ay);
	void Move(S32 mouseX,S32 mouseY);
	virtual void SetXY(S32 x,S32 y);
	virtual void GetXY(S32 *x,S32 *y);
	BOOL32 Grow(S32 factor);
	void Size(S32 oldX,S32 oldY,S32 mouseX,S32 mouseY);
	BOOL32 Save(SYSTEM_DATA *save);
	BOOL32 Load(SYSTEM_DATA *load);
	void DoDialog();
	static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
	void SetName(char buffer[256]);
	char *GetName();
	void AssignID();
	BOOL32 IsSystem();

	System *next,*last;

//private:
	
	
	SYSTEM_DATA d;

	S32 xBuf,yBuf;
	S32 startX,startY;
	GateLink *GateLinks[MAX_GATES];

    void * operator new (size_t size)
	{
		bMapChanged = 1;
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

};

//functions
			   
void LinkAB(System *A,System *B);
void LinkSystemsXY(System *A, System *B,Vector &a,Vector &b);
DRAGGER *ObjectClicked();
GateLink *LinkClicked();
void KillMap();
System * CreateSystem(S32 x0,S32 y0,S32 x1,S32 y1);
System * CreateSystem(S32 x0,S32 y0);
void DrawMap();
BOOL32 SaveMap();
BOOL32 SaveAs();
BOOL32 Save(struct IFileSystem *outFile);
BOOL32 LoadMap();
void DrawRect(S32 x0,S32 y0,S32 x1,S32 y1, U32 pen);
void GetWorldCoords(S32 *wx,S32 *wy,S32 sx,S32 sy);

//globals
extern S32 ZOOM;
extern S32 GROW_FACTOR;
extern BOOL32 SIZING;
extern GateLink *firstLink,*lastLink;
extern System   *firstSystem,*lastSystem;
extern System   *linking;
extern DRAGGER  *dragging;
extern U32 linkX,linkY;
extern U32 nextSystemID,nextGateID;
extern S32 baseX,baseY;
extern U32 availableSystemIDs;
extern U32 current_arch, gateA_arch,gateB_arch;


#endif