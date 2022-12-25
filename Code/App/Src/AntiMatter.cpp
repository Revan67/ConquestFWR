//--------------------------------------------------------------------------//
//                                                                          //
//                             AntiMatter.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/Src/AntiMatter.cpp 65    9/21/00 3:50p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "AntiMatter.h"
#include "Field.h"
#include "Mission.h"
#include "Camera.h"
#include "DField.h"
#include "Sector.h"
#include "MyVertex.h"
#include "FogOfWar.h"
#include "MGlobals.h"
#include "MPart.h"
#include "sysmap.h"
#include "CQBatch.h"
#include "DEffectOpts.h"

#include <FileSys.h>
#include <IDDBackDoor.h>
#include <D3D.h>

//
// Catmull-Rom basis.
//
extern float Bcr[4][4];/* =
{
	{-1.0/2.0,  3.0/2.0, -3.0/2.0,  1.0/2.0},
	{ 2.0/2.0, -5.0/2.0,  4.0/2.0, -1.0/2.0},
	{-1.0/2.0,  0.0/2.0,  1.0/2.0,  0.0/2.0},
	{ 0.0/2.0,  2.0/2.0,  0.0/2.0,  0.0/2.0}
};*/

struct Vector2
{
	SINGLE x;
	SINGLE y;

	Vector2(){}

	Vector2(SINGLE _x,SINGLE _y)
	{
		x = _x;
		y = _y;
	}

	inline void set(SINGLE xx,SINGLE yy)
	{
		x=xx;
		y=yy;
	}

	inline SINGLE magnitude(void) const
	{
		return (SINGLE) sqrt(x*x + y*y);
	}

	inline const Vector2& normalize(void)
	{
		SINGLE inv_m = 1.0F / magnitude();	// 1*DIV+2*MUL is faster than 2*DIV??
		x *= inv_m;
		y *= inv_m;
		return *this;
	}

	// *this += v.
	inline const Vector2& add(const Vector2 & v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	// *this -= v.
	inline const Vector2 & subtract(const Vector2 & v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	inline const Vector2& operator += (const Vector2 & v)
	{
		return add(v);
	}
//
// friend functions follow.
//
	inline friend Vector2 add(const Vector2 & v1, const Vector2 & v2)
	{
		Vector2 result;
		result.x = v1.x + v2.x;
		result.y = v1.y + v2.y;
		return result;
	}

	inline friend Vector2 subtract(const Vector2 & v1, const Vector2 & v2)
	{
		Vector2 result;
		result.x = v1.x - v2.x;
		result.y = v1.y - v2.y;
		return result;
	}
	


	inline friend Vector2 scale(const Vector2 & v, SINGLE s)
	{
		Vector2 result;
		result.x = v.x * s;
		result.y = v.y * s;
		return result;
	}
//
// Friend overloaded operators follow.
//
	inline friend Vector2 operator + (const Vector2 & v1, const Vector2 & v2)
	{
		return ::add(v1, v2);
	}

	inline friend Vector2 operator - (const Vector2 & v1, const Vector2 & v2)
	{
		return ::subtract(v1, v2);
	}

	inline friend Vector2 operator * (const Vector2 & v, SINGLE s)
	{
		return ::scale(v, s);
	}

	inline friend Vector2 operator * (SINGLE s, const Vector2 & v)
	{
		return ::scale(v, s);
	}
};

void ComputeCoeff(Vector2 & a, Vector2 & b, Vector2 & c, Vector2 & d, Vector2 p[4]);


void AntiMatterArchetype::Notify(U32 message, void *param)
{
	MSG *msg = (MSG *)param;
	
	switch (message)
	{
	case WM_LBUTTONUP:
		{
			Vector vec(mouseX,mouseY,0);
			vec.x = max(vec.x,0);
			vec.y = max(vec.y,0);
			if (CAMERA->ScreenToPoint(vec.x, vec.y, 0) != 0)
			{
				lastPt.noEdges(vec);
				int gx,gy;
				int vx,vy;
				vx = gridX*GRIDSIZE-HALFGRID-vec.x;
				vy = gridY*GRIDSIZE-HALFGRID-vec.y;
				gx = floor(vec.x/GRIDSIZE+1.0);
				gy = floor(vec.y/GRIDSIZE+1.0);
				int factor = abs(gridX-gx)+abs(gridY-gy);
				
				if (bLaying)
				{
					if (factor != 1)
					{
						if (abs(vx) > abs(vy))
						{
							if (vx < 0)
								gx = gridX+1;
							else
								gx = gridX-1;
							gy = gridY;
						}
						else
						{
							if (vy < 0)
								gy = gridY+1;
							else
								gy = gridY-1;
							gx = gridX;
						}

						vec.x = gx*GRIDSIZE-HALFGRID;
						vec.y = gy*GRIDSIZE-HALFGRID;
					}

					if (numSegPts < MAX_SEGS+1)
					{
			
						seg[numSegPts].gv.noEdges(vec);
				
						numSegPts++;
					}
				}
				else
				{
					bLaying = TRUE;
					vec.x = gx*GRIDSIZE-HALFGRID,0;
					vec.y = gy*GRIDSIZE-HALFGRID,0;
					seg[0].gv = vec;
					numSegPts = 1;
				}

				gridX = gx;
				gridY = gy;

			}
		}
		break;
	case WM_MOUSEMOVE:
		mouseX = LOWORD(msg->lParam);
		mouseY = HIWORD(msg->lParam);
		break;
	}
}

void AntiMatterArchetype::EndEdit()
{
	if (bLaying == 0 || numSegPts < 2)
	{
		bLaying = 0;
		return;
	}

	IField *obj;
	
	
	obj = (IField *)ARCHLIST->CreateInstance(pArchetype);
	if (obj==0)
		return;

	MPartNC(obj)->dwMissionID = MGlobals::CreateNewPartID(0);
	_ltoa(MPartNC(obj)->dwMissionID,MPartNC(obj)->partName,16);
	OBJLIST->AddPartID(obj,obj->GetPartID());

	obj->Setup();
	
	OBJLIST->AddObject(obj);
}

void AntiMatterArchetype::Edit()
{
	bLaying = FALSE;
	numSegPts = 0;
}

void AntiMatterArchetype::RenderEdit()
{
	if (bLaying)
	{
		CAMERA->SetPerspective();
		CAMERA->SetModelView();
		DisableTextures();
		PB.Color3ub(255,255,200);
		PB.Begin(PB_LINES);
		for (int i=0;i<numSegPts-1;i++)
		{
			Vector vec0 = seg[i].gv;
			Vector vec1 = seg[i+1].gv;
			//draw line
			PB.Vertex3f(vec0);
			PB.Vertex3f(vec1);
		}
	/*	Vector vec1 = seg[i].gv;
		Vector vecl = lastPt;
		PB.Vertex3f(vec1);
		PB.Vertex3f(vecl);*/
		
		Vector vec(mouseX,mouseY,0);
		if (CAMERA->ScreenToPoint(vec.x, vec.y, 0) != 0)
		{
			int gx,gy;
			int vx,vy;
			vx = gridX*GRIDSIZE-HALFGRID-vec.x;
			vy = gridY*GRIDSIZE-HALFGRID-vec.y;
			gx = floor(vec.x/GRIDSIZE+1.0);
			gy = floor(vec.y/GRIDSIZE+1.0);
			int factor = abs(gridX-gx)+abs(gridY-gy);
			
			if (factor != 1)
			{
				if (abs(vx) > abs(vy))
				{
					if (vx < 0)
						gx = gridX+1;
					else
						gx = gridX-1;
					gy = gridY;
				}
				else
				{
					if (vy < 0)
						gy = gridY+1;
					else
						gy = gridY-1;
					gx = gridX;
				}
				
				vec.x = gx*GRIDSIZE-HALFGRID;
				vec.y = gy*GRIDSIZE-HALFGRID;
			}
			
			
			//draw last line
			Vector vecl = lastPt;
			PB.Vertex3f(vecl);
			PB.Vertex3f(vec.x,vec.y,0);
		}
		PB.End();
	}
}

void AntiMatterArchetype::SetUpPosition(S32 * arrX, S32 * arrY, U32 numPoints)
{
	numSegPts = numPoints;
	CQASSERT(numSegPts <= MAX_SEGS+1);
	for(S32 i = 0; i <numSegPts; ++i)
	{
		seg[i].gv = Vector(arrX[i],arrY[i],0);
	}
}

struct AntiMatterArchetype *CreateAntiMatterArchetype(PARCHETYPE pArchetype,BT_ANTIMATTER_DATA *data)
{
	AntiMatterArchetype *arch = new AntiMatterArchetype;
	
	arch->pArchetype = pArchetype;
	arch->pData = data;

	PFenum alphaPF;
	PFenum opaquePF;
	
	if (CQRENDERFLAGS.b32BitTextures)
	{
		alphaPF = PF_4CC_DAA8;
		opaquePF = PF_4CC_DAA8;
	}
	else
	{
		alphaPF = PF_4CC_DAA4;
		opaquePF = PF_4CC_DAOT;
	}
	
	const char *fname;
	if(CQEFFECTS.bExpensiveTerrain==0)
	{
		fname = arch->pData->softwareTexClearName;
		if (fname[0])
		{
			arch->softwareTexClearID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAOT);
		}
		fname = arch->pData->softwareTexFogName;
		if (fname[0])
		{
			arch->softwareTexFogID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAOT);
		}
	}
	else
	{
		fname = data->textureName;
		arch->textureID = TMANAGER->CreateTextureFromFile(fname,TEXTURESDIR, DA::TGA,opaquePF);
		
		fname = "amod.tga";
		arch->modTexID = TMANAGER->CreateTextureFromFile(fname,TEXTURESDIR, DA::TGA,opaquePF);
	}

	
	if (data->mapTexName[0])
	{
		arch->mapTex = SYSMAP->RegisterIcon(data->mapTexName);
	}else
	{
		arch->mapTex = -1;
	}

	return arch;
}

AntiMatter::AntiMatter()
{
	multiStages = 0xffffffff;
}

AntiMatter::~AntiMatter()
{
	COMPTR<ITerrainMap> map;

	SECTOR->GetTerrainMap(systemID, map);
	if (map)
		unsetTerrainFootprint(map);

	if (seg)
	{
		delete [] seg;
		CQASSERT(seg_lengths);
		delete [] seg_lengths;
	}
	
	if (points)
		delete [] points;
	if (norms)
		delete [] norms;

	if(softwareNeb)
	{
		delete [] softwareNeb;
		softwareNeb = 0;
	}	
}

struct AntiMatter *CreateAntiMatter(FieldArchetype<BASIC_DATA> *pField)//AntiMatterArchetype *arch)
{
	AntiMatter *obj = new ObjectImpl<AntiMatter>;
//	BT_ANTIMATTER_DATA *data = (BT_ANTIMATTER_DATA *)pField->pData;
	
	obj->fieldType = FC_ANTIMATTER;
	
	obj->pArchetype = 0;
	obj->objClass = OC_FIELD;
	obj->SetSystemID(SECTOR->GetCurrentSystem());
//	obj->attributes = data->attributes;
	strcpy(obj->name, ARCHLIST->GetArchName(pField->pArchetype));
//	field = obj;
	obj->Init(pField);
				
	return obj;
}

//---------------------------------------------------------------------------
//
void AntiMatter::View (void)
{
	ANTIMATTER_DATA data;
	
	memcpy(data.name,name, sizeof(data.name));
	memset(data.pts,0,sizeof(data.pts));
	memcpy(data.pts,seg, sizeof(GRIDVECTOR)*(numSegPts));
	data.numSegPts = numSegPts;
	
	if (DEFAULTS->GetUserData("ANTIMATTER_DATA", " ", &data, sizeof(data)))
	{
		memcpy(name, data.name, sizeof(data.name));
		memcpy(seg, data.pts, sizeof(GRIDVECTOR)*(numSegPts));
		numSegPts = data.numSegPts;
		bounds.top = 9999999;
		bounds.bottom = -9999999;
		bounds.left = 9999999;
		bounds.right = -9999999;
		for (int i=0;i<numSegPts;i++)
		{
			Vector vec0 = seg[i].gv;
			
			if (vec0.x < bounds.left)
				bounds.left = vec0.x;
			if (vec0.x > bounds.right)
				bounds.right = vec0.x;
			
			if (vec0.y < bounds.top)
				bounds.top = vec0.y;
			if (vec0.y > bounds.bottom)
				bounds.bottom = vec0.y;
		}
		transform.translation.x = (bounds.left+bounds.right)*0.5;
		transform.translation.y = (bounds.top+bounds.bottom)*0.5;
		transform.translation.z = 0;

		Setup();

		SYSMAP->InvalidateMap(systemID);
	}
}

BOOL32 AntiMatter::Setup ()
{
	if (seg_lengths)
	{
		delete [] seg_lengths;
	}
	
	if (points)
		delete [] points;
	if (norms)
		delete [] norms;
	
	if(softwareNeb)
	{
		delete [] softwareNeb;
		softwareNeb = 0;
	}	

	BOOL32 result=0;
	COMPTR<ITerrainMap> map;

	SECTOR->GetTerrainMap(systemID, map);
	if (map)
		unsetTerrainFootprint(map);

	if (seg ==0)
	{
		numSegPts = arch->numSegPts;
		seg = new SegmentPt[numSegPts];
		memcpy(seg,arch->seg,sizeof(SegmentPt)*numSegPts);
	}
	
	seg_lengths = new SINGLE[numSegPts-1];
	norms = new Vector[numSegPts-1];
	int i;
	for (i=0;i<numSegPts-1;i++)
	{
		Vector vec0 = seg[i].gv;
		Vector vec1 = seg[i+1].gv;

		norms[i].set(vec0.y-vec1.y,vec1.x-vec0.x,0);
		if (norms[i].magnitude())
			norms[i].normalize();
		else
		{
			CQASSERT(0 && "Invalid antimatter nebula in map - ignorable");
			norms[i].set(1.0f,1.0f,0.0f);
		}
		seg_lengths[i] = sqrt(pow(vec1.x-vec0.x,2)+pow(vec1.y-vec0.y,2));
	}

	for (i=0;i<numSegPts;i++)
	{
		Vector vec = seg[i].gv;
		gvec[i].bigGridSquare(vec);
	}

	bounds.top = 9999999;
	bounds.bottom = -9999999;
	bounds.left = 9999999;
	bounds.right = -999999;
	for (i=0;i<numSegPts;i++)
	{
		Vector vec0 = seg[i].gv;

		if (vec0.x < bounds.left)
			bounds.left = vec0.x;
		if (vec0.x > bounds.right)
			bounds.right = vec0.x;

		if (vec0.y < bounds.top)
			bounds.top = vec0.y;
		if (vec0.y > bounds.bottom)
			bounds.bottom = vec0.y;
	}
	transform.translation.x = (bounds.left+bounds.right)*0.5;
	transform.translation.y = (bounds.top+bounds.bottom)*0.5;

	makePts();

	if (map)
		setTerrainFootprint(map);

	//set up software mesh if needed
	if(CQEFFECTS.bExpensiveTerrain==0)
	{
		softwareNeb = new SoftwareAntiField[numSegPts];

		for(S32 i = 0; i <= numSegPts-1; ++i)
		{
			softwareNeb[i].centerPos = gvec[i];
			softwareNeb[i].dirBits =0;
			Vector basePos = softwareNeb[i].centerPos;
			if(i != 0)
			{
				Vector extraPos = gvec[i-1];
				if(basePos.x > extraPos.x)
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x20;
					else if(basePos.y < extraPos.y)
						softwareNeb[i].dirBits |= 0x80;
					else // ==
						softwareNeb[i].dirBits |= 0x40;
				}
				else if(basePos.x < extraPos.x)
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x02;
					else if(basePos.y < extraPos.y)
						softwareNeb[i].dirBits |= 0x08;
					else // ==
						softwareNeb[i].dirBits |= 0x04;
				}else // ==
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x01;
					else // <
						softwareNeb[i].dirBits |= 0x10;
				}
			}
			if(i != numSegPts-1)
			{
				Vector extraPos = gvec[i+1];
				if(basePos.x > extraPos.x)
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x20;
					else if(basePos.y < extraPos.y)
						softwareNeb[i].dirBits |= 0x80;
					else // ==
						softwareNeb[i].dirBits |= 0x40;
				}
				else if(basePos.x < extraPos.x)
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x02;
					else if(basePos.y < extraPos.y)
						softwareNeb[i].dirBits |= 0x08;
					else // ==
						softwareNeb[i].dirBits |= 0x04;
				}else // ==
				{
					if(basePos.y > extraPos.y)
						softwareNeb[i].dirBits |= 0x01;
					else // <
						softwareNeb[i].dirBits |= 0x10;
				}
			}

/*			softwareNeb[i].dirBits = hasSquareAt(squares[i].x,squares[i].y-GRIDSIZE,i);
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x+GRIDSIZE,squares[i].y-GRIDSIZE),i) << 1;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x+GRIDSIZE,squares[i].y),i) << 2;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x+GRIDSIZE,squares[i].y+GRIDSIZE),i) << 3;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x,squares[i].y+GRIDSIZE),i) << 4;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x-GRIDSIZE,squares[i].y+GRIDSIZE),i) << 5;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x-GRIDSIZE,squares[i].y),i) << 6;
			softwareNeb[i].dirBits |= (hasSquareAt(squares[i].x-GRIDSIZE,squares[i].y-GRIDSIZE),i) << 7;
*/		}
	}

	return result;
}

BOOL32 AntiMatter::Init (HANDLE hArchetype)
{
	BOOL32 result=0;

	arch = (AntiMatterArchetype *)hArchetype;

	FRAME_init(*arch);

	infoHelpID = arch->pData->infoHelpID;

	return result;
}

Vector AntiMatter::GetCenterPos (void)
{

	return transform.translation;
}

//---------------------------------------------------------------------------
//

BOOL32 AntiMatter::Save(IFileSystem *inFile)
{
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file,file2;
	BOOL32 result = 0;

	DWORD dwWritten;

	ANTIMATTER_SAVELOAD save;

	
	fdesc.lpFileName = "ANTIMATTER_SAVELOAD";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	FRAME_save(save);

	if (file->WriteFile(0,&save,sizeof(save),&dwWritten,0) == 0)
		goto Done;

	fdesc.lpFileName = "SegPoints";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->WriteFile(0,&numSegPts,sizeof(numSegPts),&dwWritten,0) == 0)
		goto Done;

	if (file->WriteFile(0,seg,sizeof(SegmentPt)*numSegPts,&dwWritten,0) == 0)
		goto Done;


	file.free();
	
	result = 1;

Done:

	return result;

}

BOOL32 AntiMatter::Load(IFileSystem *inFile)
{
	ANTIMATTER_SAVELOAD load;
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file,file2;
	BOOL32 result = 0;

	DWORD dwRead;
	Vector pos;

	fdesc.lpFileName = "ANTIMATTER_SAVELOAD";
	fdesc.lpImplementation = "DOS";
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
	{
		goto Done;
	}

	U8 buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("ANTIMATTER_SAVELOAD", buffer, &load);

	CQASSERT(sizeof(ANTIMATTER_SAVELOAD) < 1024);

	FRAME_load(load);

	fdesc.lpFileName = "SegPoints";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (file->ReadFile(0,&numSegPts,sizeof(numSegPts),&dwRead,0) == 0)
		goto Done;

	seg = new SegmentPt[numSegPts];

	if (file->ReadFile(0,seg,sizeof(SegmentPt)*numSegPts,&dwRead,0) == 0)
		goto Done;

	Setup();

	SYSMAP->InvalidateMap(systemID);

	result = 1;

Done:

	return result;

}

/* IQuickSaveLoad methods */

void AntiMatter::QuickSave (struct IFileSystem * file)
{
	CQASSERT(numSegPts > 1);

	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QANTIMATTERLOAD");
	if (file->SetCurrentDirectory("MT_QANTIMATTERLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_QANTIMATTERLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_QANTIMATTERLOAD qload;
		DWORD dwWritten;

		qload.systemID = systemID;
		qload.numSegments = numSegPts-1;
		for (int i=0;i<numSegPts;i++)
		{
			qload.pts[i] = seg[i].gv;
		}

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void AntiMatter::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QANTIMATTERLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetSystemID(qload.systemID);
	numSegPts = qload.numSegments+1;
	dwMissionID = MGlobals::CreateNewPartID(0);

	MGlobals::InitMissionData(this, dwMissionID);
	partName = szInstanceName;

	OBJLIST->AddPartID(this, dwMissionID);

	seg = new SegmentPt[numSegPts];

	Vector pos;
	for (int i=0;i<numSegPts;i++)
	{
		seg[i].gv = qload.pts[i];
	}

	Setup();
};

#if 0
void AntiMatter::Render (void)
{
	if (bVisible)
	{
		BATCH->set_state(RPR_BATCH,true);
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

		SINGLE i = arch->pData->intensity;
		SetupDiffuseBlend(arch->textureID,TRUE);
		PB.Color3ub(255*i,255*i,255*i);
		PB.Begin(PB_QUADS);
		for (int i=0;i<numSegments;i++)
		{
		//	Vector half((seg[i].x0+seg[i].x1)*0.5,(seg[i].y0+seg[i].y1)*0.5,0);
			Vector look = CAMERA->GetPosition()-CAMERA->GetLookAtPosition();
			look.z = 0;
			look.normalize();
			SINGLE dp = 0;
			if (i>0)
			{
				Vector n = norms[i-1]+norms[i];
				n.normalize();
				dp = fabs(dot_product(n,look));
			}
/*
			PB.Color3ub(255*dp,255*dp,255*dp);

			PB.TexCoord2f(1,0);
			PB.Vertex3f(seg[i].x0,seg[i].y0,-4000);
			PB.TexCoord2f(1,1);
			PB.Vertex3f(seg[i].x0,seg[i].y0,1000);

			PB.Color3ub(255,255,255);

			PB.TexCoord2f(0.5,1);
			PB.Vertex3f(half.x,half.y,1000);
			PB.TexCoord2f(0.5,0);
			PB.Vertex3f(half.x,half.y,-4000);

			PB.TexCoord2f(0.5,0);
			PB.Vertex3f(half.x,half.y,-4000);
			PB.TexCoord2f(0.5,1);
			PB.Vertex3f(half.x,half.y,1000);

			dp = 0;
			if (i<numSegments-1)
			{
				Vector n = norms[i]+norms[i+1];
				n.normalize();
				dp = fabs(dot_product(n,look));
			}

			PB.Color3ub(255*dp,255*dp,255*dp);

			PB.TexCoord2f(0,1);
			PB.Vertex3f(seg[i].x1,seg[i].y1,1000);
			PB.TexCoord2f(0,0);
			PB.Vertex3f(seg[i].x1,seg[i].y1,-4000);*/

			PB.TexCoord2f(1,0);
			PB.Vertex3f(seg[i].x0,seg[i].y0,-4000);
			PB.TexCoord2f(1,1);
			PB.Vertex3f(seg[i].x0,seg[i].y0,1000);
			PB.TexCoord2f(0,1);
			PB.Vertex3f(seg[i].x1,seg[i].y1,1000);
			PB.TexCoord2f(0,0);
			PB.Vertex3f(seg[i].x1,seg[i].y1,-4000);
		}

		PB.End();
	}
}
#endif

void AntiMatter::RenderBackground()
{
	if(bVisible)
	{
		if(CQEFFECTS.bExpensiveTerrain==0)
		{
            bool bVisRule = DEFAULTS->GetDefaults()->bVisibilityRulesOff || DEFAULTS->GetDefaults()->bEditorMode;
			BATCH->set_state(RPR_BATCH,FALSE);
			CAMERA->SetModelView();
			BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			SetupDiffuseBlend(arch->softwareTexClearID,TRUE);
			BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE );
			BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			PB.Color(0xffffffff);
			PB.Begin(PB_QUADS);
			S32 i;
			for(i = 0; i <= numSegPts-1; ++i)
			{
				Vector testPos1 = softwareNeb[i].centerPos +Vector(GRIDSIZE/4,-(GRIDSIZE/4),0);
				Vector testPos2 = softwareNeb[i].centerPos +Vector(GRIDSIZE/4,GRIDSIZE/4,0);
				Vector testPos3 = softwareNeb[i].centerPos +Vector(-(GRIDSIZE/4),GRIDSIZE/4,0);
				Vector testPos4 = softwareNeb[i].centerPos +Vector(-(GRIDSIZE/4),-(GRIDSIZE/4),0);
				softwareNeb[i].fogRender = 0;
                if(bVisRule)
                {
                    softwareNeb[i].fogRender = 0x00;
                }
                else
                {
				    if(!FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(),SECTOR->GetCurrentSystem(),testPos1))
					    softwareNeb[i].fogRender |= 0x10;
				    if(!FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(),SECTOR->GetCurrentSystem(),testPos2))
					    softwareNeb[i].fogRender |= 0x20;
				    if(!FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(),SECTOR->GetCurrentSystem(),testPos3))
					    softwareNeb[i].fogRender |= 0x40;
				    if(!FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(),SECTOR->GetCurrentSystem(),testPos4))
					    softwareNeb[i].fogRender |= 0x80;

				    if((softwareNeb[i].fogRender &0x10) || !FOGOFWAR->CheckVisiblePosition(testPos1))
					    softwareNeb[i].fogRender |= 0x01;
				    if((softwareNeb[i].fogRender &0x20) ||!FOGOFWAR->CheckVisiblePosition(testPos2))
					    softwareNeb[i].fogRender |= 0x02;
				    if((softwareNeb[i].fogRender &0x40) ||!FOGOFWAR->CheckVisiblePosition(testPos3))
					    softwareNeb[i].fogRender |= 0x04;
				    if((softwareNeb[i].fogRender &0x80) ||!FOGOFWAR->CheckVisiblePosition(testPos4))
					    softwareNeb[i].fogRender |= 0x08;
                }

				if((~softwareNeb[i].fogRender)&0x0F)
				{
					setupSoftwareQuads(i,(~softwareNeb[i].fogRender)&0x0F);
				}
			}
			PB.End();
			SetupDiffuseBlend(arch->softwareTexFogID,TRUE);
			BATCH->set_texture_stage_state(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			BATCH->set_texture_stage_state(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE );
			BATCH->set_texture_stage_state(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			PB.Begin(PB_QUADS);
			for(i = 0; i <= numSegPts-1; ++i)
			{
				softwareNeb[i].fogRender = (softwareNeb[i].fogRender & (~(softwareNeb[i].fogRender >> 4)))&0x0F;
				if(softwareNeb[i].fogRender)
				{
					setupSoftwareQuads(i,softwareNeb[i].fogRender);
				}
			}
			PB.End();
		}
	}
}

void AntiMatter::setupSoftwareQuads(U32 i,U8 fogBits)
{
	Vector centerPos = softwareNeb[i].centerPos;
	U8 bits = softwareNeb[i].dirBits;
	Vector vec[4];
	SINGLE halfGrid = GRIDSIZE/2;
	//draw four courners of square
	vec[0] = Vector(centerPos.x,centerPos.y-halfGrid,0);
	vec[1] = Vector(centerPos.x+halfGrid,centerPos.y-halfGrid,0);
	vec[2] = Vector(centerPos.x+halfGrid,centerPos.y,0);
	vec[3] = centerPos;
	if(fogBits &0x1)
		drawCournerQuad(bits&0x07,vec);
	vec[0] = vec[2];
	vec[1] = Vector(centerPos.x+halfGrid,centerPos.y+halfGrid,0);
	vec[2] = Vector(centerPos.x,centerPos.y+halfGrid,0);
	if(fogBits &0x2)
		drawCournerQuad((bits&0x1c) >> 2,vec);
	vec[0] = vec[2];
	vec[1] = Vector(centerPos.x-halfGrid,centerPos.y+halfGrid,0);
	vec[2] = Vector(centerPos.x-halfGrid,centerPos.y,0);
	if(fogBits &0x4)
		drawCournerQuad((bits&0x70)>>4,vec);
	vec[0] = vec[2];
	vec[1] = Vector(centerPos.x-halfGrid,centerPos.y-halfGrid,0);
	vec[2] = Vector(centerPos.x,centerPos.y-halfGrid,0);
	if(fogBits &0x8)
		drawCournerQuad(((bits&0xc0)>>6) | ((bits&0x01) << 2),vec);
}

void AntiMatter::drawCournerQuad(U8 bits,Vector * vec)
{
	if((bits & 0x01))
	{
		if(bits & 0x04)
		{
			if(bits & 0x02)//fff
			{
				PB.TexCoord2f(0,0);
				PB.Vertex3f(vec[0]);
				PB.TexCoord2f(0,0.5);
				PB.Vertex3f(vec[1]);
				PB.TexCoord2f(0.5,0.5);
				PB.Vertex3f(vec[2]);
				PB.TexCoord2f(0.5,0);
				PB.Vertex3f(vec[3]);
			}
			else//fef
			{
				PB.TexCoord2f(0.5,0.5);
				PB.Vertex3f(vec[0]);
				PB.TexCoord2f(0.5,1);
				PB.Vertex3f(vec[1]);
				PB.TexCoord2f(0,1);
				PB.Vertex3f(vec[2]);
				PB.TexCoord2f(0,0.5);
				PB.Vertex3f(vec[3]);
			}
		}
		else//fxe
		{
			PB.TexCoord2f(0.5,0);
			PB.Vertex3f(vec[0]);
			PB.TexCoord2f(1,0);
			PB.Vertex3f(vec[1]);
			PB.TexCoord2f(1,0.5);
			PB.Vertex3f(vec[2]);
			PB.TexCoord2f(0.5,0.5);
			PB.Vertex3f(vec[3]);
		}
	}
	else if(bits & 0x04) //exf
	{
		PB.TexCoord2f(1,0);
		PB.Vertex3f(vec[0]);
		PB.TexCoord2f(1,0.5);
		PB.Vertex3f(vec[1]);
		PB.TexCoord2f(0.5,0.5);
		PB.Vertex3f(vec[2]);
		PB.TexCoord2f(0.5,0);
		PB.Vertex3f(vec[3]);
	}
	else//exe
	{
		PB.TexCoord2f(1,0.5);
		PB.Vertex3f(vec[0]);
		PB.TexCoord2f(1,1);
		PB.Vertex3f(vec[1]);
		PB.TexCoord2f(0.5,1);
		PB.Vertex3f(vec[2]);
		PB.TexCoord2f(0.5,0.5);
		PB.Vertex3f(vec[3]);
	}
}

bool AntiMatter::supportsSubtract ()
{
	U32 flags;
	PIPE->query_device_ability( RP_A_TEXTURE_ARG_FLAGS, &flags);
	if (flags & RP_TA_COMPLEMENT)
		return true;
	else
		return false;
}

void AntiMatter::Render (void)
{
	if (bVisible && CQEFFECTS.bExpensiveTerrain)
	{
		BATCH->set_state(RPR_BATCH,TRUE);  //we're gonna use the prim buffers
		BATCH->set_state(RPR_STATE_ID,0);  //but we don't really want to batch
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);//SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
		BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

		if (multiStages == 0xffffffff)
		{
			if (!CQRENDERFLAGS.bMultiTexture || !supportsSubtract())
				multiStages = 0;
		}

		if (multiStages == 1 || multiStages == 0xffffffff)
		{
		//	BATCH->set_state(RPR_BATCH,false);
			BATCH->set_texture_stage_texture( 0, arch->modTexID );
			BATCH->set_texture_stage_texture( 1, arch->textureID );
			
			BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE2X );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			
			// filtering - bilinear with mips
			BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
		//	BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT );
			
			// addressing - clamped
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
			
			BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT | D3DTA_COMPLEMENT);
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT | D3DTA_COMPLEMENT);
			
			// filtering - bilinear with mips
			BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		//	BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
			
			// addressing - clamped
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
		}
		
		if (multiStages == 0xffffffff)
		{
			multiStages = (BATCH->verify_state() == GR_OK);
		}
		
		if (multiStages != 1)
		{
			SetupDiffuseBlend(arch->textureID,FALSE);
		}
		
		if (multiStages == 1)
		{
			BATCHDESC desc;
			desc.type = D3DPT_TRIANGLELIST;
			desc.vertex_format = D3DFVF_RPVERTEX2;
			desc.num_verts = 8*(numPts-1);
			desc.num_indices = 12*(numPts-1);
			CQBATCH->GetPrimBuffer(&desc);
			Vertex2 *v_list = (Vertex2 *)desc.verts;
			U16 *id_list = desc.indices;
			int v_cnt=0;
			int id_cnt=0;
			for (int i=0;i<numPts-1;i++)
			{
				Vector half((points[i].x+points[i+1].x)*0.5,(points[i].y+points[i+1].y)*0.5,0);
				Vector look = CAMERA->GetPosition()-CAMERA->GetLookAtPosition();
				look.normalize();
				Vector vv=cross_product(look,Vector(0,0,1));
				vv.normalize();
				
				Vector j = cross_product(vv,look);
				j.normalize();
				
				SINGLE length=0.5*arch->pData->height*(1+0.6*sin(i*0.5));
				SINGLE length2=0.5*arch->pData->height*(1+0.6*cos(0.6+i*0.4));
				
				//	SINGLE ratio = length/length2;
				
				S32 w = 0.5*arch->pData->segment_width;
				
				Vector v[4];
				v[0] = half-vv*w-j*length;
				v[1] = half-vv*w+j*length2;
				v[2] = half+vv*w+j*length2;
				v[3] = half+vv*w-j*length;
				
				Vector vmid[2];
				vmid[0] = half-vv*w;
				vmid[1] = half+vv*w;
				/*	SINGLE dp = 0;
				if (i>0)
				{
				Vector n(points[i].n.x,points[i].n.y,0);
				dp = fabs(dot_product(n,look));
				}
				
				  PB.Color3ub(255*dp,255*dp,255*dp);
				  if (1)//i==0)
				  PB.Color3ub(0,0,0);
				  
					PB.TexCoord2f(i*0.25,0);
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(i*0.25,1);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					
					  SINGLE dp = 0.5;
					  PB.Color3ub(255*dp,255*dp,255*dp);
					  
						Vector h = (v[1]+v[2])*0.5;
						
						  PB.TexCoord2f(i*0.25+0.125,1);
						  PB.Vertex3f(h.x,h.y,h.z);
						  
							Vector h2 = (v[0]+v[3])*0.5;
							PB.TexCoord2f(i*0.25+0.125,0);
							PB.Vertex3f(h2.x,h2.y,h2.z);
							
							  PB.TexCoord2f(i*0.25+0.125,0);
							  PB.Vertex3f(h2.x,h2.y,h2.z);
							  PB.TexCoord2f(i*0.25+0.125,1);
							  PB.Vertex3f(h.x,h.y,h.z);
							  
								if (1)//i==numPts-2)
								PB.Color3ub(0,0,0);
								
								  PB.TexCoord2f(i*0.25+0.25,1);
								  PB.Vertex3f(v[2].x,v[2].y,v[2].z);
								  PB.TexCoord2f(i*0.25+0.25,0);
				PB.Vertex3f(v[3].x,v[3].y,v[3].z);*/
				
				id_list[id_cnt] = v_cnt;
				id_list[id_cnt+1] = v_cnt+1;
				id_list[id_cnt+2] = v_cnt+2;
				
				id_list[id_cnt+3] = v_cnt;
				id_list[id_cnt+4] = v_cnt+2;
				id_list[id_cnt+5] = v_cnt+3;
				id_cnt += 6;
				
				
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0;
				v_list[v_cnt].v = roll+half.x*0.01;
				v_list[v_cnt].u2 = 0;
				v_list[v_cnt].v2 = 0;
				v_list[v_cnt].pos = v[3];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = roll+half.x*0.01;
				v_list[v_cnt].u2 = 1.0f;
				v_list[v_cnt].v2 = 0.0f;
				v_list[v_cnt].pos = v[0];
				v_cnt++;
				
				v_list[v_cnt].color = 0x0;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 0.5f+roll+half.x*0.01;
				v_list[v_cnt].u2 = 1.0f;
				v_list[v_cnt].v2 = 0.5f;
				v_list[v_cnt].pos = vmid[0];
				v_cnt++;
				v_list[v_cnt].color = 0x0;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 0.5f+roll+half.x*0.01;
				v_list[v_cnt].u2 = 0.0f;
				v_list[v_cnt].v2 = 0.5f;
				v_list[v_cnt].pos = vmid[1];
				v_cnt++;
				
				id_list[id_cnt] = v_cnt;
				id_list[id_cnt+1] = v_cnt+1;
				id_list[id_cnt+2] = v_cnt+2;
				
				id_list[id_cnt+3] = v_cnt;
				id_list[id_cnt+4] = v_cnt+2;
				id_list[id_cnt+5] = v_cnt+3;
				id_cnt += 6;

				v_list[v_cnt].color = 0x0;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 0.5f-roll+half.x*0.01;
				v_list[v_cnt].u2 = 0.0f;
				v_list[v_cnt].v2 = 0.5f;
				v_list[v_cnt].pos = vmid[1];
				v_cnt++;
				v_list[v_cnt].color = 0x0;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 0.5f-roll+half.x*0.01;
				v_list[v_cnt].u2 = 1.0f;
				v_list[v_cnt].v2 = 0.5f;
				v_list[v_cnt].pos = vmid[0];
				v_cnt++;
				
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 1.0f-roll+half.x*0.01;
				v_list[v_cnt].u2 = 1.0f;
				v_list[v_cnt].v2 = 1.0f;
				v_list[v_cnt].pos = v[1];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 1.0f-roll+half.x*0.01;
				v_list[v_cnt].u2 = 0.0f;
				v_list[v_cnt].v2 = 1.0f;
				v_list[v_cnt].pos = v[2];
				v_cnt++;
				
			}
			CQBATCH->ReleasePrimBuffer(&desc);
		}
		else
		{
			BATCHDESC desc;
			desc.type = D3DPT_TRIANGLELIST;
			desc.vertex_format = D3DFVF_RPVERTEX;
			desc.num_verts = 8*(numPts-1);
			desc.num_indices = 12*(numPts-1);
			CQBATCH->GetPrimBuffer(&desc);
			RPVertex *v_list = (RPVertex *)desc.verts;
			U16 *id_list = desc.indices;
			int v_cnt=0;
			int id_cnt=0;
			for (int i=0;i<numPts-1;i++)
			{
				Vector half((points[i].x+points[i+1].x)*0.5,(points[i].y+points[i+1].y)*0.5,0);
				Vector look = CAMERA->GetPosition()-CAMERA->GetLookAtPosition();
				look.normalize();
				Vector vv=cross_product(look,Vector(0,0,1));
				vv.normalize();
				
				Vector j = cross_product(vv,look);
				j.normalize();
				
				SINGLE length=0.5*arch->pData->height*(1+0.6*sin(i*0.5));
				SINGLE length2=0.5*arch->pData->height*(1+0.6*cos(0.6+i*0.4));
				
				//	SINGLE ratio = length/length2;
				
				S32 w = 0.5*arch->pData->segment_width;
				
				Vector v[4];
				v[0] = half-vv*w-j*length;
				v[1] = half-vv*w+j*length2;
				v[2] = half+vv*w+j*length2;
				v[3] = half+vv*w-j*length;
				
				Vector vmid[2];
				vmid[0] = half-vv*w;
				vmid[1] = half+vv*w;
			
				
				id_list[id_cnt] = v_cnt;
				id_list[id_cnt+1] = v_cnt+1;
				id_list[id_cnt+2] = v_cnt+2;
				
				id_list[id_cnt+3] = v_cnt;
				id_list[id_cnt+4] = v_cnt+2;
				id_list[id_cnt+5] = v_cnt+3;
				id_cnt += 6;
				
				
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 0.0f;
				v_list[v_cnt].pos = v[3];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 0.0f;
				v_list[v_cnt].pos = v[0];
				v_cnt++;

				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 0.5f;
				v_list[v_cnt].pos = vmid[0];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 0.5f;
				v_list[v_cnt].pos = vmid[1];
				v_cnt++;

				id_list[id_cnt] = v_cnt;
				id_list[id_cnt+1] = v_cnt+1;
				id_list[id_cnt+2] = v_cnt+2;
				
				id_list[id_cnt+3] = v_cnt;
				id_list[id_cnt+4] = v_cnt+2;
				id_list[id_cnt+5] = v_cnt+3;
				id_cnt += 6;

				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 0.5f;
				v_list[v_cnt].pos = vmid[1];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 0.5f;
				v_list[v_cnt].pos = vmid[0];
				v_cnt++;

				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 1.0f;
				v_list[v_cnt].v = 1.0f;
				v_list[v_cnt].pos = v[1];
				v_cnt++;
				v_list[v_cnt].color = 0xffffffff;
				v_list[v_cnt].u = 0.0f;
				v_list[v_cnt].v = 1.0f;
				v_list[v_cnt].pos = v[2];
				v_cnt++;
			}
			CQBATCH->ReleasePrimBuffer(&desc);
		}
	}
}

//---------------------------------------------------------------------------
//
void AntiMatter::MapTerrainRender ()
{
	for(S32 i = 0; i < numSegPts; ++i)
	{
		if(arch->mapTex != -1)
			SYSMAP->DrawIcon(gvec[i],GRIDSIZE,arch->mapTex);	
		else
			SYSMAP->DrawSquare(gvec[i],GRIDSIZE,RGB(128,128,255));
	}
}

//---------------------------------------------------------------------------
//
void AntiMatter::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = FALSE;
	if (GetSystemID() == currentSystem)
	{
		SINGLE vx_min,vx_max,vy_min,vy_max;
		Vector tl,tr,br,bl;
		CAMERA->PaneToPoints(tl,tr,br,bl);
		const Transform *inverseWorldROT = CAMERA->GetWorldTransform();
		tl = inverseWorldROT->rotate_translate(tl);
		tr = inverseWorldROT->rotate_translate(tr);
		br = inverseWorldROT->rotate_translate(br);
		bl = inverseWorldROT->rotate_translate(bl);
		vx_min = min(tl.x,min(bl.x,min(tr.x,br.x)))-4000;
		vx_max = max(tl.x,max(bl.x,max(tr.x,br.x)))+4000;
		vy_min = min(tl.y,min(bl.y,min(tr.y,br.y)))-4000;
		vy_max = max(tl.y,max(bl.y,max(tr.y,br.y)))+4000;
		for (int i=0;i<numSegPts-1;i++)
		{
			Vector vec0 = seg[i].gv;
			Vector vec1 = seg[i+1].gv;

			if (__max(vec1.x,vec0.x) > vx_min && __min(vec1.x,vec0.x) < vx_max)
			{
				if (__max(vec1.y,vec0.y) > vy_min && __min(vec1.y,vec0.y) < vy_max)
				{
					bVisible = TRUE;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
SINGLE AntiMatter::TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect,
{
	bHighlight = 0;
	// don't highlight in lasso mode
	if (bVisible && rect.left==rect.right && rect.top==rect.bottom)
	{
		Vector pos;
		pos.x = rect.left;
		pos.y = rect.top;
		pos.z = 0;
	
		CAMERA->ScreenToPoint(pos.x,pos.y,pos.z);

		if (FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(), systemID, pos))
		{
			if (pos.x > bounds.left-2300 && pos.x < bounds.right+2300)
			{
				if (pos.y > bounds.top-2300 && pos.y < bounds.bottom+2300)
				{
					for (int i=0;i<numSegPts-1;i++)
					{
						Vector norm = norms[i];
						Vector vec0 = seg[i].gv;
						Vector vec1 = seg[i+1].gv;

						Vector dir(vec1.x-vec0.x,vec1.y-vec0.y,0);
						if (dir.magnitude())
							dir.normalize();
						else
							dir.set(1.0f,1.0f,0.0f);

						Vector diff=pos-Vector(vec0.x,vec0.y,0);
						
						SINGLE dist = dot_product(diff,dir);

						if (dist > 0 && dist < seg_lengths[i])
						{
							dist = dot_product(diff,norm);
							
							if (dist > -2300 && dist < 2300)
							{
								bHighlight = TRUE;
								factory->FieldHighlighted(this);
							}
						}
					}
				}
			}
		}
	}

	return 1e20;		// a large number
}

void AntiMatter::PhysicalUpdate (SINGLE dt)
{
	roll += 0.1*dt;
}

BOOL32 AntiMatter::Update()
{
	return 1;
}

#define WIDTH 1000
void AntiMatter::setTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_IMPASSIBLE | TERRAIN_FULLSQUARE;
	terrainMap->SetFootprint(gvec,numSegPts,info);
}
void AntiMatter::unsetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	if (numSegPts)
	{
		FootprintInfo info;
		info.missionID = dwMissionID;
		info.height = 0;
		info.flags = TERRAIN_FIELD | TERRAIN_IMPASSIBLE | TERRAIN_FULLSQUARE;
		terrainMap->UndoFootprint(gvec,numSegPts,info);
	}
}

bool AntiMatter::GetObjectBox (OBJBOX & box) const
{
	memset(&box,0,sizeof(box));
	return true;
}

/*bool AntiMatter::IntersectFootprint(const Vector &pos, const Vector &dest, Vector & result)
{
	bool bResult=false;

	Vector n(dest.y-pos.y,pos.x-dest.x,0);
	n.normalize();

	for (int i=0;i<numSegments;i++)
	{
		Vector diff1,diff2;
		SINGLE dist1,dist2;
		diff1 = pos;
		diff1.x -= seg[i].x0;
		diff1.y -= seg[i].y0;
		dist1 = dot_product(diff1,norms[i]);
		diff2 = dest;
		diff2.x -= seg[i].x0;
		diff2.y -= seg[i].y0;
		dist2 = dot_product(diff2,norms[i]);
		if (dist1*dist2 < 0)
		{
			diff1.set(seg[i].x0,seg[i].y0,0);
			diff1 -= pos;
			dist1 = dot_product(diff1,n);
			diff2.set(seg[i].x1,seg[i].y1,0);
			diff2 -= pos;
			dist2 = dot_product(diff2,n);
			if (dist1*dist2 < 0)
			{
				bResult = TRUE;
				goto Done;
			}
		}
	}
	
//	if (dot_product(pos-c,dir)*dot_product(dest-c,dir) > 0)
//	{ 
//	}
		
Done:
	
	if (bResult)
	{
		Vector c = transform.translation;
		c.z = 0;
	
		SINGLE dist = sqrt(pow((seg[0].x0-pos.x),2)+pow((seg[0].y0-pos.y),2));
		Vector vec;
		if (dist < rad)
			//vec = Vector(seg[0].x0,seg[0].y0,0);
			vec = roundPt0;
		else
		//	vec = Vector(seg[numSegments-1].x1,seg[numSegments-1].y1,0);
			vec = roundPt1;

	//	Vector rad_dir = vec-transform.translation;
	//	rad_dir *= 1.3;
	//	vec = transform.translation+rad_dir;

		Vector dir = vec-pos;
		dir.normalize();
		result = pos+dir*4000;
	}

	return bResult;
	
}

SINGLE AntiMatter::GetRadius()
{
	return rad;
}*/

void AntiMatter::makePts()
{
	Vector2 *pts;
	pts = new Vector2[numSegPts+2];
//	numPts = num_steps*numSegments;
	
	SINGLE *lengths = new SINGLE[numSegPts-1];
	S32 *newPtCnt = new S32[numSegPts-1];
	S32 totalPtCnt=0;

	if (arch->pData->spacing == 0)
		arch->pData->spacing = 2000;

	Vector vec = seg[0].gv;
	pts[0].x = vec.x;
	pts[0].y = vec.y;
	int i;
	for (i=0;i<numSegPts;i++)
	{
		vec = seg[i].gv;
		pts[i+1].x = vec.x;
		pts[i+1].y = vec.y;

		if (i>0)
		{
			Vector2 diff = pts[i+1]-pts[i];
			lengths[i-1] = __max(diff.magnitude(),arch->pData->spacing);  //humor bad data
			newPtCnt[i-1] = S32(ceil(lengths[i-1]/arch->pData->spacing));
			totalPtCnt += newPtCnt[i-1];
		}
	}
	pts[i+1] = pts[i];
	//add one point to make the end of the ribbon
	totalPtCnt += 1;
	newPtCnt[i-2] += 1;

	numPts = totalPtCnt;

	points = new AMPts[numPts];

	S32 curPt=0;
	Vector2 a,b,c,d;
	S32 last_cp = 1;
	for (last_cp=1;last_cp<numSegPts;last_cp++)
	{
		S32 num_steps = newPtCnt[last_cp-1];
		ComputeCoeff(a,b,c,d,&pts[last_cp-1]);
		

		float delta;
		//this code is designed to lerp to the end of the antimatter more attractively
		if (last_cp != numSegPts-1)
			delta = 1.0 / num_steps;
		else
			delta = 1.0 / (num_steps-1);

		float delta2 = delta * delta;
		float delta3 = delta2 * delta;
		
		Vector2 df = a * delta3 + b * delta2 + c * delta;
		Vector2 ddf;
		Vector2 dddf;
		ddf = 6.0 * a * delta3 + 2.0 * b * delta2;
		dddf = 6.0 * a * delta3;
		
		Vector2 p=d;
		

		for (int n = 0; n < num_steps; n++)
		{
			points[curPt].x = p.x;
			points[curPt].y = p.y;
			
			p += df;
			df += ddf;
			ddf += dddf;
			
			curPt++;
		}
	}

	for (i=1;i<numPts-1;i++)
	{
		Vector2 fn1,fn2;
		Vector2 n;
		fn1.set(points[i].y-points[i-1].y,points[i-1].x-points[i].x);
		fn2.set(points[i+1].y-points[i].y,points[i].x-points[i+1].x);
		n = fn1+fn2;
		if (n.magnitude())
			n.normalize();
		else
		{
			CQASSERT(0 && "Invalid antimatter nebula in map - ignorable");
			n.set(1.0f,1.0f);
		}
		
		points[i].n.set(n.x,n.y,0);
	}
	points[0].n.set(points[1].y-points[0].y,points[0].x-points[1].x,0);
	if (points[0].n.magnitude())
		points[0].n.normalize();
	else
	{
		CQASSERT(0 && "Invalid antimatter nebula in map - ignorable");
		points[0].n.set(1.0f,1.0f,0.0f);
	}
	points[numPts-1].n.set(points[numPts-1].y-points[numPts-2].y,points[numPts-2].x-points[numPts-1].x,0);
	if (points[numPts-1].n.magnitude())
		points[numPts-1].n.normalize();
	else
	{
		CQASSERT(0 && "Invalid antimatter nebula in map - ignorable");
		points[numPts-1].n.set(1.0f,1.0f,0.0f);
	}

	delete [] lengths;
	delete [] newPtCnt;
	delete [] pts;
}

/*
void AntiMatter::makePts()
{
	Vector *pt_norms = new Vector[numSegments+1];
	SINGLE *lengths = new SINGLE[numSegments];
	SINGLE totalLength=0;

	pt_norms[0] = norms[0];
	pt_norms[numSegments] = norms[numSegments-1];
	for (int i=0;i<numSegments;i++)
	{
		
		if (i > 0 && i < numSegments)
		{
			pt_norms[i] = norms[i-1]+norms[i];
			pt_norms[i].normalize();
		}
		
		lengths[i] = sqrt(pow(seg[i].x1-seg[i].x0,2)+pow(seg[i].y1-seg[i].y0,2));
		totalLength += lengths[i];
	}

#define ARB 1000

	points = new AMPts[S32(totalLength/ARB)];

	S32 curPt=1;
	S32 x=seg[0].x0,y=seg[0].y0;
	S32 dist1,dist2;
	//a point normal
	Vector n;
	//face normals
	Vector last_n,face_n;

	points[0].x=x;
	points[0].y=y;
	points[0].n=norms[0];
	last_n=norms[0];

	for (i=0;i<numSegments;i++)
	{
		dist1 = sqrt(pow(seg[i].x0-x,2)+pow(seg[i].y0-y,2));
		dist2 = sqrt(pow(seg[i].x1-x,2)+pow(seg[i].y1-y,2));
		n = pt_norms[i]*dist1+pt_norms[i+1];
		n.normalize();
		face_n = n-last_n;
		face_n.normalize();
		last_n = face_n;
		points[curPt].n = n;
		x = points[curPt].x = x+ARB*face_n.y;
		y = points[curPt].y = y+ARB*face_n.x;
	}
}*/


///////////////////////////////////////////////////////////////////////////////////////
void ComputeCoeff(Vector2 & a, Vector2 & b, Vector2 & c, Vector2 & d, Vector2 p[4])
{
//
// Catmull-Rom basis.
//
#define B Bcr
	
	a = B[0][0] * p[0] + B[0][1] * p[1] + B[0][2] * p[2] + B[0][3] * p[3];
	b = B[1][0] * p[0] + B[1][1] * p[1] + B[1][2] * p[2] + B[1][3] * p[3];
	c = B[2][0] * p[0] + B[2][1] * p[1] + B[2][2] * p[2] + B[2][3] * p[3];
	d = B[3][0] * p[0] + B[3][1] * p[1] + B[3][2] * p[2] + B[3][3] * p[3];
}
//--------------------------------------------------------------------------//
//-------------------------END AntiMatter.cpp------------------------------//
//--------------------------------------------------------------------------//
