//--------------------------------------------------------------------------//
//                                                                          //
//                                RuseMap.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RuseMap.cpp 55    11/15/00 1:03p Jasony $
*/			    
//--------------------------------------------------------------------------//
#include "pch.h"
#include <globals.h>

#include "RuseMap.h"
#include "Resource.h"
#include "UserDefaults.h"
#include "CQTrace.h"
#include "DrawAgent.h"
#include <MGlobals.h>
#include "DSysMap.h"
#include "Sector.h"
#include "ObjList.h"
#include "GridVector.h"

#include <TSmartPointer.h>
#include <FileSys.h>
#include <math.h>
#include "stdio.h"

#define GATE_SIZE 12
#define RGB_WHITE RGB(255,255,255)
#define RGB_YELLOW RGB(255,255,0)
#define RGB_RED RGB(255, 0, 0)
#define RGB_GRAY RGB(180,180,180)
#define RGB_CYAN RGB(0,255,255)

S32 ZOOM = 14;
S32 GROW_FACTOR = 1;
BOOL32 SIZING = FALSE;
U32 availableSystemIDs=0;		//bitfield        U32 nextSystemID = 1;
U32 nextGateID = 0;
S32 baseX = INVALID_BASE,baseY = INVALID_BASE;
U32 saveCounter=0;
U32 current_arch, gateA_arch, gateB_arch;

extern S32 mouseX,mouseY;
extern IFileSystem * g_OutFile;

GateLink *firstLink = NULL,*lastLink = NULL;
System   *firstSystem = NULL,*lastSystem = NULL;
//Jump	 *firstJump   = NULL,*lastJump = NULL;

System *linking = NULL;
DRAGGER *dragging = NULL;

U32 linkX=0,linkY=0;
char currentFilename[MAX_PATH];

void GetScreenCoords(S32 *sx,S32 *sy,S32 gx,S32 gy);
void GetWorldCoords(S32 *wx,S32 *wy,S32 sx,S32 sy);

struct Jump * MakeJumpgate(IFileSystem *file,U32 archID);

struct RUSELoadArchEnum : IArchetypeEnum
{
	COMPTR<IFileSystem> inFile;
	
	virtual	BOOL32 __stdcall ArchetypeEnum (const char * name, void *data, U32 size);
};

BOOL32 __stdcall RUSELoadArchEnum::ArchetypeEnum (const char * name, void *data, U32 size)
{
	BOOL32 result = 0;

	BASIC_DATA *objData = (BASIC_DATA *)data;
	if (objData->objClass == OC_JUMPGATE)
	{
		//get jumpgates from objectlist
		HANDLE handle;
		WIN32_FIND_DATA data;
		
		if (inFile->SetCurrentDirectory("\\ObjectList\\QuickSave") == 0)
			goto Done;
		
		if (inFile->SetCurrentDirectory(name) == 0)
		{
			result = 1;
			goto Done;
		}

		if (inFile->SetCurrentDirectory("MT_QJGATELOAD") == 0)
			goto Done;
		
		if ((handle = inFile->FindFirstFile("*", &data)) != INVALID_HANDLE_VALUE)
			do
			{
				COMPTR<IFileSystem> file;
				DAFILEDESC fdesc = data.cFileName;
				if (inFile->CreateInstance(&fdesc, file) != GR_OK)
					goto Done;
				Jump * jump = MakeJumpgate(file,ARCHLIST->GetArchetypeDataID(name));
				jump->partName = name;
			} while (inFile->FindNextFile(handle, &data));
	}

	result = 1;
	
Done:

	CQASSERT(HEAP->EnumerateBlocks());
	return result;
}

RUSELoadArchEnum *l_enumer;

struct GateNode
{
	struct Jump *gate;
	struct GateNode *next;
};

Jump * MakeJumpgate(IFileSystem *file,U32 archID)
{
	Jump *jump = new Jump();
	jump->Load(file);
	jump->archID = archID;

/*	if (lastJump)
		lastJump->next = jump;
	else
		firstJump = jump;

	lastJump = jump;*/

	GateLink *linkPos = firstLink;
	//GateLink *lastLink=0;
	bool bFoundLink=false;
	while (linkPos)
	{
		if (jump->d.gate_id == linkPos->gateID1)
		{
			CQBOMB0("Huh?");
			linkPos->jump1 = jump;
			jump->link = linkPos;
			bFoundLink=true;
		}
		if (jump->d.gate_id == linkPos->gateID2)
		{
			//finish up link
			linkPos->jump2 = jump;
			linkPos->end_id2 = jump->system->GetID();
			jump->link = linkPos;
			bFoundLink=true;
		}
		lastLink = linkPos;
		linkPos = linkPos->next;
	}

	//make a link between this jumpgate and one we haven't loaded yet
	if (bFoundLink==false)
	{
		linkPos = new GateLink(lastLink);
		if (lastLink)
			lastLink->next = linkPos;
		else
			firstLink = linkPos;
		lastLink = linkPos;
		linkPos->jump1 = jump;
		linkPos->gateID1 = jump->d.gate_id;
		linkPos->gateID2 = jump->d.exit_gate_id;
		linkPos->end_id1 = jump->system->GetID();
	}

	return jump;
}

DRAGGER::~DRAGGER()
{
}

BOOL32
DRAGGER::IsSystem()
{
	return FALSE;
}

Jump::Jump(S32 locX,S32 locY,struct System *syst,GateLink *gLink)
{
	system = syst;
	link = gLink;

	d.pos.systemID = syst->GetID();

	x = locX;
	y = locY;

	//	assumes sizeX == sizeY cause systems are round anyway
	S32 sysRad=syst->d.sizeX/2;
	S32 center_dist;
	center_dist = (S32)sqrt(pow((float)(locX-sysRad),2.0f)+pow((float)(locY-sysRad),2.0f));
	if (center_dist > sysRad-GRIDSIZE)
	{
		Vector diff(locX-sysRad,locY-sysRad,0);
		diff.normalize();
		diff*=sysRad-GRIDSIZE;
		x = sysRad+diff.x;
		y = sysRad+diff.y;
	}

	x = (S32)(floor((float)(x/GRIDSIZE))+0.5f)*GRIDSIZE;//who coded this ... floor on an integer.... -tom
	y = (S32)(floor((float)(y/GRIDSIZE))+0.5f)*GRIDSIZE;

	d.gate_id = nextGateID;
	nextGateID++;

	bJumpAllowed = true;

}

Jump::Jump()//GateLink *gLink)
{
//	link = gLink;
	link = NULL;
	x = 0;
	y = 0;
	system = NULL;
	d.pos.systemID = NULL;
	bJumpAllowed = true;
}

Jump::~Jump()
{
	if (link)
	{
		link->CutMe(this);
		if (link->alive)
			delete link;
	}
}

void
Jump::GetScreenLoc(S32 *lx,S32*ly)
{
	S32 ox,oy;

	system->GetOrigin(&ox,&oy);
	
	GetScreenCoords(lx,ly,x+ox,y+oy);
	//*lx = ((d.x + ox)*ZOOM)/10000;
	//*ly = SCREENRESY-((d.y + oy)*ZOOM)/10000;
}

void Jump::Draw()
{
	S32 gx,gy;

/*	HBITMAP JumpBMP = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_Jump));
	HDC hdc;

	hdc = GetDC(hMainWindow);
	HDC src_hdc = CreateCompatibleDC(hdc);
	HGDIOBJ test = SelectObject(hdc,JumpBMP);*/
	
	GetScreenLoc(&gx,&gy);
	//BitBlt(hdc,gx,gy,GATE_SIZE,GATE_SIZE,src_hdc,0,0,SRCCOPY);
	DrawRect(gx-GATE_SIZE/2,gy-GATE_SIZE/2,gx+GATE_SIZE/2,gy+GATE_SIZE/2,RGB_WHITE);//,ColorRefToPixel(_CLR_WHITE));
	//DeleteDC (src_hdc);
	//ReleaseDC(hMainWindow,hdc);
	//DeleteObject(JumpBMP);
}

void
Jump::SetAnchor(S32 ax,S32 ay)
{
	GetWorldCoords(&anchorX,&anchorY,ax,ay);
	startX = x;
	startY = y;
}

void
Jump::Move(S32 mx,S32 my)
{
	
	GetWorldCoords(&mx,&my,mx,my);
	
	x = startX+(mx-anchorX);
	y = startY+(my-anchorY);

/*	if (x<0)
		x=0;
	if (y<0)
		y=0;
	S32 maxX=system->d.sizeX,maxY=system->d.sizeY;
	if (x > maxX)
		x = maxX;
	if (y > maxY)
		y = maxY;*/

	//	assumes sizeX == sizeY cause systems are round anyway
	S32 sysRad=system->d.sizeX/2;
	S32 center_dist;
	center_dist = (S32)sqrt(pow((float)(x-sysRad),2.0f)+pow((float)(y-sysRad),2.0f));
	if (center_dist > sysRad-GRIDSIZE)
	{
		Vector diff(x-sysRad,y-sysRad,0);
		diff.normalize();
		diff*=sysRad-GRIDSIZE;
		x = sysRad+diff.x;
		y = sysRad+diff.y;
	}
	x = (S32)(floor((float)(x/GRIDSIZE))+0.5f)*GRIDSIZE;
	y = (S32)(floor((float)(y/GRIDSIZE))+0.5f)*GRIDSIZE;
}

void 
Jump::SetXY(S32 _x,S32 _y)
{
	x = _x;
	y = _y;
}

void 
Jump::GetXY(S32 *_x,S32 *_y)
{
	*_x = x;
	*_y = y;
}

BOOL32
Jump::MouseIn()
{
	S32 lx,ly;

	GetScreenLoc(&lx,&ly);
	
	if ((mouseX >= lx-GATE_SIZE/2) && (mouseX <= lx+GATE_SIZE/2))
	{
		if ((mouseY <= ly+GATE_SIZE/2) && (mouseY >= ly-GATE_SIZE/2))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL32
Jump::Save(IFileSystem *outFile)
{
	BOOL32 result = 0;
//	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "JUMPGATE_SAVELOAD";
//	char buffer[MAX_PATH];
	const char *archName = ARCHLIST->GetArchName(archID);

	outFile->CreateDirectory(archName);
	if (outFile->SetCurrentDirectory(archName) == 0)
		goto Done;

	outFile->CreateDirectory("MT_QJGATELOAD");
	if (outFile->SetCurrentDirectory("MT_QJGATELOAD") == 0)
		CQERROR0("QuickSave failed");

	partName = "Gate";

	_ltoa(d.gate_id, partName+4, 10);
	fdesc.lpFileName = partName;
	
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	HANDLE hFile;
	if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR0("QuickSave failed");
	}
	else
	{
		MT_QJGATELOAD qload;
		DWORD dwWritten;

		qload.pos.init(Vector(x,y,0),system->GetID());
		qload.gate_id = d.gate_id;
		qload.exit_gate_id = d.exit_gate_id;
		qload.bJumpAllowed = bJumpAllowed;

		outFile->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		outFile->CloseHandle(hFile);
	}

	if (outFile->SetCurrentDirectory("..\\..") == 0)
		goto Done;

//	d.mission.partName = "Gate";



	/*if (link->gateID1 == d.gate_id)
		_ltoa(link->gateID2, exitGate+4, 10);
	else
	if (link->gateID2 == d.gate_id)
		_ltoa(link->gateID1, exitGate+4, 10);
	else
		CQBOMB0("Where's my end gate1?");*/
	
	//.mission.mObjClass = M_JUMPGATE;
	
	result = 1;

Done:
	return result;
}

BOOL32
Jump::Load(IFileSystem *file)
{
	U32 dwRead;
	System *sysPos = firstSystem;
	BOOL32 temp;


	temp = file->ReadFile(0,&d,sizeof(d),&dwRead,0);
//	x = d.trans_SL.transform.translation.x;
//	y = d.trans_SL.transform.translation.y;
	Vector vec = d.pos;
	x = vec.x;
	y = vec.y;
	bJumpAllowed = d.bJumpAllowed;

	while (sysPos)
	{
		if (sysPos->GetID() == d.pos.systemID)
			system = sysPos;
		sysPos = sysPos->next;
	}
	
	if (d.gate_id >= nextGateID)
		nextGateID = d.gate_id+1;

	return temp;
}

GateLink::GateLink(System *A,System *B,GateLink *lastL)
{
	end1 = A;
	end2 = B;
	end_id1 = A->GetID();
	end_id2 = B->GetID();
	A->AddLink(this);
	B->AddLink(this);

	jump1 = jump2 = NULL;
	
	next = NULL;
	last = lastL;

	alive = TRUE;
}

GateLink::GateLink(GateLink *lastL)
{
	
	end1 = NULL;
	end2 = NULL;
	
	jump1 = jump2 = NULL;

	next = NULL;
	last = lastL;
}

GateLink::~GateLink()
{
	alive = FALSE;
	if (end1)
		end1->Detach(this);
	if (end2)
		end2->Detach(this);
	if (lastLink == this)
	{
		lastLink = lastLink->last;
	}
	if (firstLink == this)
	{
		firstLink = firstLink->next;
	}
		
	if (last)
		last->next = next;
	if (next)
		next->last = last;

	if (jump1)
		delete jump1;
	if (jump2)
		delete jump2;
}

void
GateLink::Draw()
{
	S32 c0x,c0y,c1x,c1y;
	U32 pen = RGB_WHITE;
	
	jump1->Draw();
	jump2->Draw();

	if (MouseOn())
	{
		pen = RGB_YELLOW;
		if (ObjectClicked())
			pen = RGB_WHITE;
	}
	else
		pen = RGB_WHITE;

	jump1->GetScreenLoc(&c0x,&c0y);
	jump2->GetScreenLoc(&c1x,&c1y);

	DA::LineDraw(0, c0x, c0y, c1x, c1y, pen);
}

BOOL32
GateLink::MouseOn()
{
	//SINGLE theta1;
	SINGLE x0,y0,x1,y1,x,y;
	SINGLE dist;

	/*	end1->GetCenter(&x0,&y0);
	theta1 = (SINGLE)atan((mouseY-y0)/(mouseX-x0));
	theta1 -= theta;
	dist = sqrt((mouseY-y0)/(mouseX-x0))*sin(theta1);*/

//	end1->GetScreenCenter(&x,&y);
//	end2->GetScreenCenter(&x0,&y0);

	S32 a,b;
	jump1->GetScreenLoc(&a,&b);
	x = a;
	y = b;
	jump2->GetScreenLoc(&a,&b);
	x0=a;
	y0=b;

	x1 = mouseX - x;
	y1 = mouseY - y;

	x0 -= x;
	y0 -= y;

	//get the normalized dot product
	dist = (x0*x1+y0*y1);
	if (dist)
		dist /= (sqrt(x0*x0+y0*y0));

	//quit if past the line endpoints
	if ((dist <= 0) || (dist >= (SINGLE)sqrt(x0*x0+y0*y0)))
		return 0;

	//find the distance to line with Pythagorean thereom
	dist = 0;
	SINGLE num = x1*x1+y1*y1-dist*dist;
	if (num > 0)
		dist = (SINGLE)sqrt(num);
	else
		return TRUE;

	return (dist < 6);
}

void
GateLink::GenerateJumps(System *A,System *B)
{
	S32 wx,wy;

	GetWorldCoords(&wx,&wy,(linkX),(linkY));
	wx -= A->d.x;
	wy -= A->d.y;
	
	jump1 = new Jump(wx,wy,A,this);
	jump1->archID = gateA_arch;
	
	GetWorldCoords(&wx,&wy,(mouseX),(mouseY));
	wx -= B->d.x;
	wy -= B->d.y;

	jump2 = new Jump(wx,wy,B,this);
	jump2->archID = gateB_arch;

	gateID1 = jump1->d.gate_id;
	gateID2 = jump2->d.gate_id;

	jump1->d.exit_gate_id = jump2->d.gate_id;
	jump2->d.exit_gate_id = jump1->d.gate_id;
}

void
GateLink::GenerateJumpsXY(System *A,System *B,const Vector &a,const Vector &b)
{
	jump1 = new Jump(a.x,a.y,A,this);
	jump1->archID = gateA_arch;
	jump2 = new Jump(b.x,b.y,B,this);
	jump2->archID = gateB_arch;
	gateID1 = jump1->d.gate_id;
	gateID2 = jump2->d.gate_id;

	jump1->d.exit_gate_id = jump2->d.gate_id;
	jump2->d.exit_gate_id = jump1->d.gate_id;
}


BOOL32
GateLink::Save(IFileSystem *outFile)
{
	//U32 dwWritten;

	//file->WriteFile(0,&d,sizeof(d),&dwWritten,0);
	if (jump1)
		jump1->Save(outFile);
	else
		MessageBox(hMainWindow,"Invalid data","Error",MB_OK);

	if (jump2)
		jump2->Save(outFile);
	else
		MessageBox(hMainWindow,"Invalid data","Error",MB_OK);

	return TRUE;
}

/*BOOL32
GateLink::Load(IFileSystem *file)
{
	U32 dwRead;
	
	file->ReadFile(0, &d, sizeof(d), &dwRead, 0);
	if (dwRead)
	{
		end1 = firstSystem;
		while (end1->GetID() != d.end_id1)
		{
			end1 = end1->next;
			if (end1 == NULL)
				goto Done;
		}

		end2 = firstSystem;
		while (end2->GetID() != d.end_id2)
		{
			end2 = end2->next;
			if (end2 == NULL)
				goto Done;
		}
		
		end1->AddLink(this);
		end2->AddLink(this);
	
		bIndestructable = true;		// loaded from disk, cannot destroy
	}

Done:
	
	return dwRead;
}*/

/*U8
GateLink::GetMultiplicity()
{
	return multiplicity;
} */

/*void
GateLink::AssignID()
{
	d.id = nextLinkID;
	nextLinkID++;
} */

/*U32
GateLink::GetID()
{
	return d.id;
} */

void GateLink::CutMe(Jump *Jump)
{
	if (jump1 == Jump)
		jump1 = NULL;
	if (jump2 == Jump)
		jump2 = NULL;
}

System::System(S32 locX,S32 locY,System *lastS)
{
//	GetWorldCoords(&d.x,&d.y,locX,locY);
	d.x = locX;
	d.y = locY;
	d.sizeX = 100000;
	d.sizeY = 100000;

//	SECTOR_DATA sector_data;

	S32 newSize;
	
//	newSize = DEFAULTS->GetDataFromRegistry("Sector", &sector_data, sizeof(sector_data));

	/*if (newSize == sizeof(SECTOR_DATA))
	{
		d.sizeX = sector_data.defaultSizeX;
		d.sizeY = sector_data.defaultSizeY;
	}*/

	SECTOR->GetDefaultSystemSize(d.sizeX,d.sizeY);

	U8 i;
	for (i=0;i<MAX_GATES;i++)
		GateLinks[i] = NULL;

	xBuf = 0;
	yBuf = 0;
	next = NULL;
	last = lastS;

	newSize = DEFAULTS->GetDataFromRegistry("Camera", &d.cameraBuffer, sizeof(d.cameraBuffer));
	if (d.cameraBuffer.version != CAMERA_DATA_VERSION)
	{
		if (d.cameraBuffer.version < CAMERA_DATA_VERSION)
		{
			memset(&d.cameraBuffer, 0, sizeof(d.cameraBuffer));
		}
		else
		{
			CQBOMB0("This version of RUSE is out-of-date.  Please recompile.");
		}
	}
	else if (newSize != sizeof(d.cameraBuffer))
	{
		CQBOMB0("Game registry corrupted or Camera version not incremented.");
	}
}

System::~System()
{
	U32 i;

	if (d.id)
		availableSystemIDs &= ~(1 << d.id);			// clear available bit

	for (i=0;i<MAX_GATES;i++)
	{
		if (GateLinks[i])
		{
			delete GateLinks[i];
		}
	}
	if (lastSystem == this)
	{
		lastSystem = lastSystem->last;
	}
	if (firstSystem == this)
	{
		firstSystem = firstSystem->next;
	}
		
	if (last)
		last->next = next;
	if (next)
		next->last = last;

}

void 
System::ClearSizeBuf()
{
	xBuf = 0;
	yBuf = 0;
}

void 
System::AddLink(GateLink *link)
{
	U8 i;
	
	//count bacwards until i flips
	for (i=MAX_GATES-1;i<MAX_GATES;i--)
	{
		if (GateLinks[i] == NULL)
		{
			GateLinks[i] = link;
			return;
		}
	}

	CQBOMB0("Out of link slots.");
}


void
System::Detach(GateLink *link)
{
	U8 i;
	
	for (i=0;i<MAX_GATES;i++)
	{
		if (GateLinks[i] == link)
			GateLinks[i] = NULL;	
	}

	bMapChanged = 1;
}

void
System::Draw2D()
{
	char sizeBuf[64];

	if (MouseIn() || dragging == this)
	{
		DEBUGFONT->StringDraw(NULL,8,SCREENRESY-15,d.name);
		
		sprintf(sizeBuf,"%i",d.sizeX);
		
		DEBUGFONT->StringDraw(NULL,SCREENRESX-64,SCREENRESY-15,sizeBuf);
	}

}

void
System::Draw()
{
	COLORREF color = RGB_GRAY;
	const U32 playerID = MGlobals::GetThisPlayer();

	if (d.alertState[playerID] & S_LOCKED)
	{
		color = RGB_CYAN;
	}
	else if (d.alertState[playerID] & S_VISIBLE)
	{
		color = RGB_WHITE;
	}
	
	if (MouseIn() || dragging == this)
	{
		color = RGB_YELLOW;
		if (SIZING)
			color = RGB_RED;
		else
		{
			if (dragging == NULL)
			{
				if (GotEdge())
					color = RGB_RED;
			}
		}

	}

	S32 destX,destY,x1,y1;
	GetScreenCoords(&destX,&destY,d.x,d.y);
	GetScreenCoords(&x1,&y1,d.x+d.sizeX,d.y+d.sizeY);

//	DrawRect(destX,destY,x1,y1, color);
	BATCH->set_state(RPR_BATCH,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	OrthoView();
	SetupDiffuseBlend(sysTextureID,FALSE);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	SINGLE offs = 1/64.0;
	PB.Begin(PB_QUADS);
	PB.TexCoord2f(offs,-offs);   PB.Vertex3f(destX,destY,0);
	PB.TexCoord2f(1+offs,-offs);   PB.Vertex3f(x1,destY,0);
	PB.TexCoord2f(1+offs,1-offs);   PB.Vertex3f(x1,y1,0);
	PB.TexCoord2f(offs,1-offs);   PB.Vertex3f(destX,y1,0);
	PB.End();
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
}

BOOL32
System::MouseIn()
{
	S32 mx,my;

	GetWorldCoords(&mx,&my,mouseX,mouseY);

	
	if ((mx >= d.x) && (mx <= (d.x+d.sizeX+1)))
	{
		if ((my >= d.y) && (my <= (d.y+d.sizeY+1)))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL32
System::GotEdge()
{
	//S32 x0,y0,
	S32 x1,y1;
	S32 mx,my;

	//GetScreenCoords(&x0,&y0,d.x,d.y);
	GetScreenCoords(&x1,&y1,d.x+d.sizeX,d.y+d.sizeY);
	GetWorldCoords(&mx,&my,mouseX,mouseY);

	if ((mouseX >= x1-3) && (mouseX <= x1+1))
	{
		xBuf = mx-d.x;
		yBuf = my-d.y;

		return TRUE;
	}
	if ((mouseY >= y1-3) && (mouseY <= y1+1))
	{
		xBuf = mx-d.x;
		yBuf = my-d.y;

		return TRUE;
	}

	return FALSE;
}

/*void
System::GetScreenCenter(S32 *cx,S32 *cy)
{
	*cx = (d.x+d.sizeX/2)*ZOOM;
	*cy = SCREENRESY-(d.y+d.sizeY/2)*ZOOM;
} */

void
System::GetOrigin(S32 *ox,S32 *oy)
{
	*ox = d.x;
	*oy = d.y;
}

U32
System::GetID()
{
	return d.id;
}

void
System::SetAnchor(S32 ax,S32 ay)
{
	GetWorldCoords(&anchorX,&anchorY,ax,ay);
	startX = d.x;
	startY = d.y;
}

void
System::Move(S32 mx,S32 my)
{
	
	GetWorldCoords(&mx,&my,mx,my);
	
	d.x = startX+(mx-anchorX);
	d.y = startY+(my-anchorY);

	if (d.x<0)
		d.x=0;
	if (d.y<0)
		d.y=0;

	bMapChanged = 1;
}

void 
System::SetXY(S32 x,S32 y)
{
	d.x = x;
	d.y = y;
}

void 
System::GetXY(S32 *x,S32 *y)
{
	*x = d.x;
	*y = d.y;
}

BOOL32 
System::Grow(S32 factor)
{
	BOOL32 temp = TRUE;
	
	d.sizeX += factor;
	d.sizeY += factor;

	if (d.sizeX < 1)
	{
		d.sizeX = 4;
	}
	if (d.sizeY < 1)
	{
		d.sizeY = 4;
	}

	temp = (d.sizeX >= 1 && d.sizeY >=1);
	bMapChanged = 1;

	return temp;
}

void
System::Size(S32 oldX,S32 oldY,S32 mouseX,S32 mouseY)
{
	S32 mx,my;
	
	GetWorldCoords(&oldX,&oldY,oldX,oldY);
	GetWorldCoords(&mx,&my,mouseX,mouseY);
	
	S32 diffX = (mx - oldX);
	S32 diffY = (my - oldY);

	xBuf += diffX;
	yBuf += diffY;

	if (xBuf >= 1 && yBuf >=1)
	{
		if (xBuf < yBuf)
		{
			d.sizeY = max((yBuf/GROW_FACTOR)*GROW_FACTOR,10000);

			if (d.sizeY > MAX_SYS_SIZE)
				d.sizeY = MAX_SYS_SIZE;
			d.sizeX = d.sizeY;

		}
		else
		{
			d.sizeX = max((xBuf/GROW_FACTOR)*GROW_FACTOR,10000);
			if (d.sizeX > MAX_SYS_SIZE)
				d.sizeX = MAX_SYS_SIZE;
			d.sizeY = d.sizeX;
		}
	}
	
/*	while (xBuf >= (SINGLE)GROW_FACTOR)
	{
		Grow(GROW_FACTOR);
		xBuf -= GROW_FACTOR;
	//	yBuf -= GROW_FACTOR;
		yBuf = 0;
	}
	while (yBuf >= (SINGLE)GROW_FACTOR)
	{
		Grow(GROW_FACTOR);
	//	xBuf -= GROW_FACTOR;
		xBuf = 0;
		yBuf -= GROW_FACTOR;
	}

	while (xBuf <= (SINGLE)(-GROW_FACTOR))
	{
		Grow(-GROW_FACTOR);
		xBuf += GROW_FACTOR;
		yBuf = 0;
	//	yBuf += GROW_FACTOR;
	}
	while (yBuf <= (SINGLE)(-GROW_FACTOR))
	{
		Grow(-GROW_FACTOR);
	//	xBuf += GROW_FACTOR;
		xBuf = 0;
		yBuf += GROW_FACTOR;
	}	  */

	bMapChanged = 1;
}

BOOL32
System::Save(SYSTEM_DATA *save)
{
	memcpy(save,&d,sizeof(SYSTEM_DATA));

	return 1;
}

BOOL32
System::Load(SYSTEM_DATA *load)
{
	if (d.id)
		availableSystemIDs &= ~(1 << d.id);			// clear available bit

	memcpy(&d,load,sizeof(SYSTEM_DATA));

	CQASSERT(d.id <= MAX_SYSTEMS && "Bad system id");

	availableSystemIDs |= (1 << d.id);			// set available bit

	bIndestructable = true;		// loaded from disk, cannot destroy

	return (d.id != 0);

}

void
System::DoDialog()
{
	DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_RUSE_NAME_DIALOG), hMainWindow, 
		DlgProc, (LPARAM) this);

}

BOOL 
System::DlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
        SetWindowLong(hwnd, DWL_USER, lParam);
	System *system = (System *) GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwnd,IDC_RUSE_EDIT_NAME, system->GetName());
		SetFocus(GetDlgItem(hwnd,IDC_RUSE_EDIT_NAME));
		EnableWindow(GetDlgItem(hwnd, IDC_RUSE_DELETE), (system->bIndestructable==0));
		if (system->d.alertState[MGlobals::GetThisPlayer()] & S_VISIBLE)
			CheckDlgButton(hwnd,IDC_RUSE_ISVISIBLE,BST_CHECKED);
		else
			CheckDlgButton(hwnd,IDC_RUSE_ISVISIBLE,BST_UNCHECKED);
		if (system->d.alertState[MGlobals::GetThisPlayer()] & S_LOCKED)
			CheckDlgButton(hwnd,IDC_RUSE_ISLOCKED,BST_CHECKED);
		else
			CheckDlgButton(hwnd,IDC_RUSE_ISLOCKED,BST_UNCHECKED);

		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			char buffer[256];
			
			GetDlgItemText(hwnd, IDC_RUSE_EDIT_NAME, buffer, 256);

											 
			if (buffer[0])
				system->SetName(buffer);

			memset(system->d.alertState, 0, sizeof(system->d.alertState));
			if (IsDlgButtonChecked(hwnd,IDC_RUSE_ISVISIBLE))
			{
				for (int i=0; i <= MAX_PLAYERS; i++)
					system->d.alertState[i] |= S_VISIBLE;
			}
			if (IsDlgButtonChecked(hwnd,IDC_RUSE_ISLOCKED))
			{
				for (int i=0; i <= MAX_PLAYERS; i++)
					system->d.alertState[i] |= S_LOCKED;
			}
			bMapChanged = 1;
			EndDialog(hwnd, 0);
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		case IDC_RUSE_DELETE:
			if (system->bIndestructable)
			{
				CQERROR0("Cannot delete a system that was loaded from disk!");
			}
			else
			if (MessageBox(hMainWindow,"Are you sure you want to delete this system?","Delete system",MB_YESNO) == IDYES)
			{
				delete system;
				bMapChanged = 1;
				EndDialog(hwnd,0);
			}
			break;
		}
		break;
	}

	return 0;
}		

void
System::SetName(char buffer[256])
{
	strncpy(d.name,buffer,sizeof(d.name));
}

char *
System::GetName()
{
	return d.name;
}

void
System::AssignID()
{
	//
	// find an available system ID
	//
	U32 i;

	CQASSERT(MAX_SYSTEMS < 31);		// sizeof bitfield

	for (i = 1; i <= MAX_SYSTEMS; i++)
	{
		if ((availableSystemIDs & (1 << i)) == 0)
		{
			availableSystemIDs |= (1 << i);
			d.id = i;
			break;
		}
	}

	CQASSERT(i <= MAX_SYSTEMS && "Ran out of system ID's");	

	/*
	d.id = nextSystemID;
	nextSystemID++;
	*/
}

BOOL32
System::IsSystem()
{
	return TRUE;
}

void LinkAB(System *A,System *B)
{
	if (firstLink)
	{
		lastLink->next = new GateLink(A,B,lastLink);
		lastLink = lastLink->next;
	}
	else
	{
		firstLink = new GateLink(A,B,NULL);
		lastLink = firstLink;
	}
	lastLink->GenerateJumps(A,B);
}

void LinkSystemsXY(System *A, System *B,Vector &a,Vector &b)
{
	if (firstLink)
	{
		lastLink->next = new GateLink(A,B,lastLink);
		lastLink = lastLink->next;
	}
	else
	{
		firstLink = new GateLink(A,B,NULL);
		lastLink = firstLink;
	}
	lastLink->GenerateJumpsXY(A,B,a,b);
}


DRAGGER *ObjectClicked()
{
	System *SystemPos = firstSystem;
	DRAGGER *clicked = NULL;
	GateLink *linkPos = firstLink;

	while (linkPos)
	{
		if (linkPos->jump1->MouseIn())
			clicked = linkPos->jump1;
		if (linkPos->jump2->MouseIn())
			clicked = linkPos->jump2;
		linkPos = linkPos->next;
	}

	if (clicked == NULL)
	{
		while (SystemPos)
		{
			if (SystemPos->MouseIn())
				clicked = SystemPos;
			SystemPos = SystemPos->next;
		}

	//	if (clicked)
	//		SIZING = ((System *)clicked)->GotEdge();
	}
	else
		SIZING = FALSE;

	return clicked;
}

GateLink *LinkClicked()
{
	GateLink *linkPos = firstLink;
	GateLink *clicked = NULL;

	while (linkPos)
	{
		if (linkPos->MouseOn())
			clicked = linkPos;
		linkPos = linkPos->next;
	}

	return clicked;
}



void KillMap()
{
	GateLink *linkPos;
	System *SystemPos;

	while (firstLink)
	{
		linkPos = firstLink->next;
		delete firstLink;
		firstLink = linkPos;
	}

	while (firstSystem)
	{
		SystemPos = firstSystem->next;
		delete firstSystem;
		firstSystem = SystemPos;
	}

	availableSystemIDs = 0;	// nextSystemID = 1;
	nextGateID = 0;

	dragging = NULL;
	SIZING = FALSE;
}

System * CreateSystem(S32 x0,S32 y0,S32 x1,S32 y1)
{
	S32 x = min (abs(x0),abs(x1));
	//remember y is flipped
	S32 y = min (abs(y0),abs(y1));

	if (firstSystem)
	{
		lastSystem->next = new System(x,y,lastSystem);
		lastSystem = lastSystem->next;
	}
	else
	{
		firstSystem = new System(x,y,NULL);
		lastSystem = firstSystem;
	}

//	lastSystem->d.sizeX = max(abs(x0-x1)*10000/ZOOM,10000);
//	lastSystem->d.sizeY = max(abs(y0-y1)*10000/ZOOM,10000);
	lastSystem->d.sizeX = min(max(abs(x0-x1),10000),MAX_SYS_SIZE);
	lastSystem->d.sizeY = min(max(abs(y0-y1),10000),MAX_SYS_SIZE);

	return lastSystem;

}

System * CreateSystem(S32 x0,S32 y0)
{
	if (firstSystem)
	{
		lastSystem->next = new System(x0,y0,lastSystem);
		lastSystem = lastSystem->next;
	}
	else
	{
		firstSystem = new System(x0,y0,NULL);
		lastSystem = firstSystem;
	}

	return lastSystem;
}

void DrawMap()
{  	
	BATCH->set_state(RPR_BATCH,FALSE);
	U32 i,j;
	COLORREF pen = RGB(100,100,100);

	for (i=0;i<=SCREENRESX/(ZOOM*4);i++)
	{
	/*	VFX_line_draw (GR->screen_pane(),
			i*4*ZOOM, 0, i*4*ZOOM, SCREENRESY-1, 
			LD_DRAW,
			ColorRefToPixel(_CLR_GRAY));*/

		DA::LineDraw(0, i*4*ZOOM,0, i*4*ZOOM,SCREENRESY-1, pen);
	}

	for (j=0;j<=SCREENRESY/(ZOOM*4);j++)
	{
		/*VFX_line_draw (GR->screen_pane(),
			0,SCREENRESY-(j*4*ZOOM), SCREENRESX-1,SCREENRESY-(j*4*ZOOM),
			LD_DRAW,
			ColorRefToPixel(_CLR_GRAY));*/

		DA::LineDraw(0, 0,SCREENRESY-(j*4*ZOOM), SCREENRESX-1,SCREENRESY-(j*4*ZOOM), pen);
	}

	System *SystemPos = firstSystem;
	
	while (SystemPos)
	{
		SystemPos->Draw();
		SystemPos = SystemPos->next;
	}

	GateLink *linkPos = firstLink;
	while (linkPos)
	{
		linkPos->Draw();
		linkPos = linkPos->next;
	}
}
//---------------------------------------------------------------------------
//
static int DeleteJumpgates (IFileSystem * file)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=1;

	if (file->SetCurrentDirectory("\\ObjectList\\QuickSave") == 0)
		goto Done;
	if ((handle = file->FindFirstFile("JGATE!!*", &data)) == INVALID_HANDLE_VALUE)
		goto Done;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.')
		{
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				if (file->SetCurrentDirectory(data.cFileName))
				{
					result = RecursiveDelete(file);
					file->SetCurrentDirectory("..");	// restore current directory
					file->RemoveDirectory(data.cFileName);
				}
			}
			else 
			{	
				if (file->DeleteFile(data.cFileName) == 0)
					result = 0;
			}
		}

	} while (file->FindNextFile(handle, &data));

	file->FindClose(handle);

Done:
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 SaveMap()
{
	BOOL32 result = 0;
	U32 dwWritten;

	saveCounter = 0;

	MT_SECTOR_SAVELOAD save;
	memset(&save,0,sizeof(MT_SECTOR_SAVELOAD));
	save.currentSystem = 1;

	System *SystemPos = firstSystem;


	COMPTR<IFileSystem> outFile,file;

	DAFILEDESC fdesc;
	outFile = g_OutFile;

	DeleteJumpgates(outFile);

	outFile->CreateDirectory("\\MT_SECTOR_SAVELOAD");
	if (outFile->SetCurrentDirectory("\\MT_SECTOR_SAVELOAD") == 0)
		goto Done;
	
	if (RecursiveDelete(outFile) == 0)
	{
		CQERROR0("Save failed - File is probably corrupted");
		goto Done;
	}
	
	int i;
	i=0;
	while (SystemPos)
	{
		SystemPos->Save(&save.sysData[i]);
		SystemPos = SystemPos->next;
		i++;
	}
	
	fdesc = "Sector";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	
	file->WriteFile(0,&save,sizeof(save),&dwWritten);
	
	outFile->CreateDirectory("\\ObjectList");

	if (outFile->SetCurrentDirectory("\\ObjectList") == 0)
		goto Done;

	outFile->CreateDirectory("QuickSave");
	if (outFile->SetCurrentDirectory("QuickSave") == 0)
		goto Done;

	GateLink *linkPos;
	linkPos = firstLink;
	
	while (linkPos)
	{
		linkPos->Save(outFile);
		linkPos = linkPos->next;
	}
	
	MGlobals::Save(outFile);
	result = 1;
	bMapChanged = 0;
		
Done:
	return result;
}

BOOL32 LoadMap()
{
	MT_SECTOR_SAVELOAD load;
	BOOL32 result = 0;
	COMPTR<IFileSystem> inFile,file;
	DAFILEDESC fdesc;
	U32 dwRead;

	KillMap();

	inFile = g_OutFile;

	MGlobals::Load(inFile);

	if (inFile->SetCurrentDirectory("\\MT_SECTOR_SAVELOAD") == 0)
		goto Done;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.lpFileName = "Sector";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0,&load,sizeof(load),&dwRead);

	CreateSystem(0,0);
	
	int i;
	i=0;
	while (lastSystem->Load(&load.sysData[i]) && i < MAX_SYSTEMS)
	{
		CreateSystem(0,0);
		i++;
	}

	//goofy code
	if (lastSystem != firstSystem)
	{
		lastSystem = lastSystem->last;
		delete lastSystem->next;
		lastSystem->next = NULL;
	}
	else
	{
		delete lastSystem;
		firstSystem = lastSystem = NULL;
	}
	

/*	fdesc.lpFileName = "Links";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	firstLink = new GateLink(NULL);
	lastLink = firstLink;
	
	while (lastLink->Load(file)) 
	{
		lastLink->next = new GateLink(lastLink);
		lastLink = lastLink->next;
	}

	if (lastLink != firstLink)
	{
		lastLink = lastLink->last;
		delete lastLink->next;
		lastLink->next = NULL;
	}
	else
	{
		delete lastLink;
		firstLink = lastLink = NULL;
	}*/
	

	//get jumpgates from objectlist
/*	HANDLE handle;
	WIN32_FIND_DATA data;

	if (inFile->SetCurrentDirectory("\\ObjectList") == 0)
		goto Done;

	if (inFile->SetCurrentDirectory("Instances") == 0)
		goto Done;

	if ((handle = inFile->FindFirstFile("*-*", &data)) != INVALID_HANDLE_VALUE)
	do
	{
		char * ptr;

		if (inFile->SetCurrentDirectory(data.cFileName) == 0)
			goto Done;
		ptr = strchr(data.cFileName+1, '-') + 1;
		if (!strcmp(ptr,"Jumpgate"))
		{
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = "JUMPGATE_SAVELOAD";
			if (inFile->CreateInstance(&fdesc, file) != GR_OK)
				goto Done;
			MakeJumpgate(file,0);
		}
		inFile->SetCurrentDirectory("..");
	} while (inFile->FindNextFile(handle, &data));*/

	l_enumer = new RUSELoadArchEnum;
	l_enumer->inFile = inFile;
	result = ARCHLIST->EnumerateArchetypeData(l_enumer);
	delete l_enumer;

	bMapChanged = 0;

Done:
	return result;

}

void DrawRect(S32 x0,S32 y0,S32 x1,S32 y1, U32 pen)
{
	DA::LineDraw(0, x0, y0, x1, y0, (COLORREF)pen);
	DA::LineDraw(0, x1, y0, x1, y1, (COLORREF)pen);
	DA::LineDraw(0, x0, y1, x1, y1, (COLORREF)pen);
	DA::LineDraw(0, x0, y0, x0, y1, (COLORREF)pen);
}

void GetScreenCoords(S32 *sx,S32 *sy,S32 gx,S32 gy)
{
	*sx = (gx*ZOOM)/10000;
	*sy = SCREENRESY-(gy*ZOOM)/10000;
}

void GetWorldCoords(S32 *wx,S32 *wy,S32 sx,S32 sy)
{
	*wx = (sx*10000)/ZOOM;
	*wy = ((SCREENRESY-sy)*10000)/ZOOM;
}