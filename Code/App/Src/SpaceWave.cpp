//--------------------------------------------------------------------------//
//                                                                          //
//                             SpaceWave.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 2003 Warthog TX, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Sector.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DSpecial.h"
#include "IWeapon.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mesh.h"
#include "ArchHolder.h"
#include "Anim2d.h"
#include "FogOfWar.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "TerrainMap.h"
#include "CQLight.h"
#include "GridVector.h"
#include "TObjMission.h"
#include "OpAgent.h"
#include <MGlobals.h>
#include "TManager.h"
#include "IRecon.h"
#include "ObjMap.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "MPart.h"
#include "IVertexBuffer.h"
#include <stdio.h>
#include <DEffectOpts.h>
#include "ISpaceWave.h"

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
//#include <ITextureLibrary.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>

struct SPACEWAVE_INIT
{
	BT_SPACEWAVE_DATA * pData;
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
};

#define SPACEMESH_SEGMENTS 20

struct SpaceWaveMesh : IVertexBufferOwner
{
	Vector ringCenter[SPACEMESH_SEGMENTS];
	Vector upperRing[SPACEMESH_SEGMENTS];
	U32 ringTexID;
	U32 moveTexID;
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	SpaceWaveMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~SpaceWaveMesh()
	{
		vb_mgr->Delete(this);
	}

}spaceWaveMesh;

void SpaceWaveMesh::RestoreVertexBuffers()
{
	if (CQRENDERFLAGS.bSoftwareRenderer==0)
	{
		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		if (vb_handle)
		{
			result = PIPE->destroy_vertex_buffer(vb_handle);
			CQASSERT(result == GR_OK);
		}
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, SPACEMESH_SEGMENTS*2-1, IRP_VBF_SYSTEM, &spaceWaveMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( spaceWaveMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < SPACEMESH_SEGMENTS; ++i)
		{
			vb_data[i*2].color = 0xffffffff;
			vb_data[i*2].u2 = i+0.5f;
			vb_data[i*2].v2 = 0;
			vb_data[i*2].u = i+0.5f;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = spaceWaveMesh.upperRing[i];
			
			if(i != SPACEMESH_SEGMENTS-1)
			{
				vb_data[i*2+1].color = 0;
				vb_data[i*2+1].u2 = i;
				vb_data[i*2+1].v2 = 0;
				vb_data[i*2+1].u = i+1;
				vb_data[i*2+1].v = 0;
				vb_data[i*2+1].pos = spaceWaveMesh.ringCenter[i];
			}
		}
		
		result = PIPE->unlock_vertex_buffer( spaceWaveMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		spaceWaveMesh.vb_handle = 0;
}

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};

struct _NO_VTABLE SpaceWave : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,struct SPACEWAVE_INIT> >, ISpaceWave
{
	BEGIN_MAP_INBOUND(SpaceWave)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISpaceWave)
	END_MAP()

	const BT_SPACEWAVE_DATA * data;

	U32 multiStages;
	
	U32 objMapSystemID;
	U32 objMapSquare;

	U32 systemID;
	U32 playerID;

	Vector startPos;
	Vector endPos;
	SINGLE travelTime;
	SINGLE time;

	//------------------------------------------

	SpaceWave (void)
	{
		travelTime = 0;
		multiStages = 0xFFFFFFFF;
		objMapSystemID = 0;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~SpaceWave (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return playerID;
	}

	virtual void CastVisibleArea();

	//ISpaceWave

	virtual void StartSpaceWave(Vector & start, Vector & end, SINGLE travelTime, U32 _systemID, U32 _playerID);

	//spaceWave
	void init (SPACEWAVE_INIT &initData);
	
};

//----------------------------------------------------------------------------------
//
SpaceWave::~SpaceWave (void)
{
	if(objMapSystemID)
	{
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);
		objMapSystemID = 0;
	}
}
//----------------------------------------------------------------------------------
//
U32 SpaceWave::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void SpaceWave::CastVisibleArea()
{
	const U32 mask = MGlobals::GetAllyMask(playerID);
	SetVisibleToAllies(mask);
}
//----------------------------------------------------------------------------------
//
void SpaceWave::StartSpaceWave(Vector & start, Vector & end, SINGLE _travelTime, U32 _systemID, U32 _playerID)
{
	startPos = start;
	endPos = end;
	travelTime = _travelTime;
	time = 0;
	systemID = _systemID;
	playerID = _playerID;
	SetPosition(end,systemID);

	CQASSERT(objMapNode==0);
	objMapSquare = OBJMAP->GetMapSquare(systemID, end);
	objMapSystemID = systemID;
	objMapNode = OBJMAP->AddObjectToMap(this, objMapSystemID, objMapSquare, 0);
	CQASSERT(objMapNode);

}
//----------------------------------------------------------------------------------
//
void SpaceWave::PhysicalUpdate (SINGLE dt)
{
	time = time+dt;
}
//----------------------------------------------------------------------------------
//
BOOL32 SpaceWave::Update (void)
{
	if(time > travelTime+1)
		return 0;
	return 1;
}
//----------------------------------------------------------------------------------
//
void SpaceWave::Render (void)
{
	if (bVisible && time > 1.0)
	{					
		if (CQRENDERFLAGS.bSoftwareRenderer)
		{
			return;
		}

		SINGLE delta = (time-1)/travelTime;
		Vector position = (delta*(startPos-endPos))+endPos;
		position.z = 0;

		Vector dir = startPos-endPos;
		dir.z = 0;
		dir.fast_normalize();
		Vector up(0,0,1);
		Vector side = cross_product(up,dir);

		TRANSFORM trans;
		trans.set_identity();
		trans.set_position(position);
		trans.set_i(-dir);
		trans.set_j(side);
		trans.set_k(up);
		trans.scale(GRIDSIZE*3*((1.0-delta)+0.1));

		BATCH->set_state(RPR_BATCH,false);
		CAMERA->SetModelView(&trans);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
		BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

		if(multiStages == 1 || multiStages == 0xffffffff)
		{
		//	BATCH->set_state(RPR_BATCH,false);
			BATCH->set_texture_stage_texture( 0, spaceWaveMesh.ringTexID );
			BATCH->set_texture_stage_texture( 1, spaceWaveMesh.moveTexID );
			
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE  );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
						
			// addressing - clamped
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
			
			BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
						
			// addressing - clamped
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
		}
		
		if (multiStages == 0xffffffff)
		{
			multiStages = (BATCH->verify_state() == GR_OK);
		}
		
		if (multiStages != 1)
		{
			SetupDiffuseBlend(spaceWaveMesh.ringTexID,FALSE);
		}
	
		Vertex2 *vb_data;
		U32 dwSize;
		GENRESULT result;
		result = PIPE->lock_vertex_buffer( spaceWaveMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);

		SINGLE timeDif = (delta)*3;
		SINGLE timeDif2 = timeDif-1;
		U32 alpha = 0;
		if(delta < 0.2 && delta > 0)
			alpha = delta*5*255;
		else if(delta > 0.8&& delta < 1.0)
			alpha = (1.0-delta)*5*255;
		else if (delta >= 0.2 && delta <= 0.8)
			alpha = 255;
		else
			alpha = 0;
		int i;
		for(i = 0; i < SPACEMESH_SEGMENTS; ++i)
		{
			vb_data[i*2].v2 = i+timeDif2;				
			if(i == 0 || i == SPACEMESH_SEGMENTS-1)
				vb_data[i*2].color = 0;
			else
			{
				U32 thisAlpha = alpha* (1.0-(fabs((i*2.0f)-(SPACEMESH_SEGMENTS-1))/(SPACEMESH_SEGMENTS-1)));
				vb_data[i*2].color = thisAlpha<<24 | 0x00ffffff;
			}
			if(i != SPACEMESH_SEGMENTS-1)
			{
				vb_data[i*2+1].v2 = i+timeDif;
			}
		}

		result = PIPE->unlock_vertex_buffer( spaceWaveMesh.vb_handle );
		CQASSERT(result == GR_OK);

		PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, spaceWaveMesh.vb_handle, 0, SPACEMESH_SEGMENTS*2-1, 0 );
	}
}

//---------------------------------------------------------------------------
//
void SpaceWave::init (SPACEWAVE_INIT &initData)
{
	FRAME_init(initData);
	data = initData.pData;

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_SPACEWAVE);
	CQASSERT(data->objClass == OC_WEAPON);

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
}
//------------------------------------------------------------------------------------------
//---------------------------SpaceWave Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SpaceWaveFactory : public IObjectFactory
{
	struct OBJTYPE : SPACEWAVE_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		
		OBJTYPE (void)
		{
			archIndex = -1;
		}
		
		~OBJTYPE (void)
		{
			ENGINE->release_archetype(archIndex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(SpaceWaveFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	SpaceWaveFactory (void) { }

	~SpaceWaveFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	// IObjectFactory methods 

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	// SpaceWaveFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SpaceWaveFactory::~SpaceWaveFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void SpaceWaveFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE SpaceWaveFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_SPACEWAVE_DATA * data = (BT_SPACEWAVE_DATA *)_data;
		if (data->wpnClass == WPN_SPACEWAVE)
		{
			result = new OBJTYPE;
			
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pData = data;
			//build the global mesh
			SINGLE degreePerSeg = (PI*0.5f)/(SPACEMESH_SEGMENTS-1);
			SINGLE startDegree = -(PI*0.25f);
			for(U32 i = 0; i < SPACEMESH_SEGMENTS; ++i)
			{
				spaceWaveMesh.ringCenter[i] = Vector(cos(startDegree+degreePerSeg*(i+0.5))-1.0,sin(startDegree+degreePerSeg*(i+0.5)),0);
				spaceWaveMesh.upperRing[i] = Vector(cos(startDegree+degreePerSeg*i)*0.7-1.0,sin(startDegree+degreePerSeg*i)*0.7,0.1);
			}

			spaceWaveMesh.ringTexID = TMANAGER->CreateTextureFromFile("shockwave_rw_color.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			if (CQRENDERFLAGS.bMultiTexture)
				spaceWaveMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				spaceWaveMesh.moveTexID = 0;
			
			spaceWaveMesh.RestoreVertexBuffers();

			goto Done;
		}
	}

	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 SpaceWaveFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	TMANAGER->ReleaseTextureRef(spaceWaveMesh.ringTexID);
	if (spaceWaveMesh.moveTexID)
		TMANAGER->ReleaseTextureRef(spaceWaveMesh.moveTexID);
	if (spaceWaveMesh.vb_handle)
	{
		PIPE->destroy_vertex_buffer(spaceWaveMesh.vb_handle);
		spaceWaveMesh.vb_handle = 0;
	}

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SpaceWaveFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	SpaceWave * repClound = new ObjectImpl<SpaceWave>;

	repClound->init(*objtype);

	return repClound;
}
//-------------------------------------------------------------------
//
void SpaceWaveFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _spaceWave : GlobalComponent
{
	SpaceWaveFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SpaceWaveFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _spaceWave __spaceWave;
//---------------------------------------------------------------------------
//------------------------End SpaceWave.cpp----------------------------------------
//---------------------------------------------------------------------------


