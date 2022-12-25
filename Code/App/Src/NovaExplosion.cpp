//--------------------------------------------------------------------------//
//                                                                          //
//                             NovaExplosion.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 2004 WARTHOG, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "DEffect.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "INovaExplosion.h"
#include "Objlist.h"
#include "CQlight.h"
#include "Camera.h"
#include "Objmap.h"
#include "TManager.h"
#include "Anim2d.h"
#include "IVertexBuffer.h"

#include <FileSys.h>

struct DUMMY_SAVESTRUCT			// required by template framework
{
	TRANS_SAVELOAD trans_SL;
};

struct NovaExplosionArchetype
{
	INSTANCE_INDEX archIndex;
	IMeshArchetype * meshArch;
	BT_NOVA_EXPLOSION * pData;
	
	NovaExplosionArchetype()
	{
		meshArch = NULL;
		archIndex = -1;
	}

	~NovaExplosionArchetype()
	{
	}
};

#define NOVA_SEGMENTS 80
#define EFFECT_RATE 3

struct NovaMesh : IVertexBufferOwner
{
	Vector ringCenter[NOVA_SEGMENTS];
	Vector upperRing[NOVA_SEGMENTS];
	U32 ringTexID;
	U32 moveTexID;
	U32 vb_handle;
	
	virtual void RestoreVertexBuffers();

	NovaMesh()
	{
		vb_handle = 0;
		vb_mgr->Add(this);
	}

	~NovaMesh()
	{
		vb_mgr->Delete(this);
	}

}novaMesh;

void NovaMesh::RestoreVertexBuffers()
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
		result = PIPE->create_vertex_buffer( D3DFVF_RPVERTEX2, NOVA_SEGMENTS*2+2, IRP_VBF_SYSTEM, &novaMesh.vb_handle );
		CQASSERT(result == GR_OK);
		result = PIPE->lock_vertex_buffer( novaMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
		CQASSERT(result == GR_OK);
		
		for(int i = 0; i < NOVA_SEGMENTS; ++i)
		{
			vb_data[i*2].u2 = i;
			vb_data[i*2].u = i+1;
			vb_data[i*2].v = 1;
			vb_data[i*2].pos = novaMesh.ringCenter[i];
			
			vb_data[i*2+1].color = 0x00ffffff;
			vb_data[i*2+1].u2 = i+0.5f;
			vb_data[i*2+1].u = i+0.5f;
			vb_data[i*2+1].v = 0;
			vb_data[i*2+1].pos = novaMesh.upperRing[i];
		}
		
		vb_data[NOVA_SEGMENTS*2].u = NOVA_SEGMENTS+1;
		vb_data[NOVA_SEGMENTS*2].v = 1;
		vb_data[NOVA_SEGMENTS*2].u2 = NOVA_SEGMENTS;
		vb_data[NOVA_SEGMENTS*2].pos = novaMesh.ringCenter[0];
		
		vb_data[NOVA_SEGMENTS*2+1].color = 0x00ffffff;
		vb_data[NOVA_SEGMENTS*2+1].u = NOVA_SEGMENTS+0.5f;
		vb_data[NOVA_SEGMENTS*2+1].v = 0;
		vb_data[NOVA_SEGMENTS*2+1].u2 = NOVA_SEGMENTS+0.5f;
		vb_data[NOVA_SEGMENTS*2+1].pos = novaMesh.upperRing[0];
		
		result = PIPE->unlock_vertex_buffer( novaMesh.vb_handle );
		CQASSERT(result == GR_OK);
	}
	else
		novaMesh.vb_handle = 0;
}

struct _NO_VTABLE NovaExplosion : public ObjectTransform<ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,NovaExplosionArchetype> >, INovaExplosion
{
	BEGIN_MAP_INBOUND(NovaExplosion)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(INovaExplosion)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	typedef INITINFO NOVAEXPLOSIONINITINFO;
	BT_NOVA_EXPLOSION * pData;

	OBJPTR<IBaseObject> owner;

	SINGLE time;
	U32 multiStages;

	NovaExplosion();

	~NovaExplosion();

	//IBaseObject

	virtual void Render();

	virtual U32 GetSystemID() const
	{
		if(owner)
			return owner.Ptr()->GetSystemID();
		return 0xFFFFFFFF;
	};

	virtual void CastVisibleArea();

	BOOL32 Update ();

	void PhysicalUpdate (SINGLE dt);

	//INovaExplosion
	virtual void InitNovaExplosion(IBaseObject * baseOwner);
};
//----------------------------------------------------------------------------------
//
NovaExplosion::NovaExplosion()
{
	time = 0;
	multiStages = 0xFFFFFFFF;
}
//----------------------------------------------------------------------------------
//
NovaExplosion::~NovaExplosion()
{
}
//----------------------------------------------------------------------------------
//
void NovaExplosion::Render()
{
	if (bVisible)
	{					
		SINGLE tempTime;
		if(time < pData->ringTime)
			tempTime = time;
		else
			tempTime = time - pData->interRingTime * ceil((time-pData->ringTime)/pData->interRingTime);

		while(tempTime > 0)
		{
			TRANSFORM trans;
			trans.set_identity();
			SINGLE scale = (tempTime/pData->ringTime)*(pData->range*GRIDSIZE)+1;
			trans.scale(scale);
			trans.set_position(transform.translation);

			U32 alpha;
			SINGLE t = tempTime/pData->ringTime;
			SINGLE timeDif = (t)*EFFECT_RATE;
			SINGLE timeDif2 = timeDif-1;
			if(tempTime < pData->ringTime)
				alpha = ((1.0 - (t*t))) *255;
			else
				alpha = 0;

			BATCH->set_state(RPR_BATCH,false);
			CAMERA->SetModelView(&trans);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
			BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);

			if(multiStages == 1 || multiStages == 0xffffffff)
			{
			//	BATCH->set_state(RPR_BATCH,false);
				BATCH->set_texture_stage_texture( 0, novaMesh.ringTexID );
				BATCH->set_texture_stage_texture( 1, novaMesh.moveTexID );
				
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
				SetupDiffuseBlend(novaMesh.ringTexID,FALSE);
			}

			Vertex2 *vb_data;
			U32 dwSize;
			GENRESULT result;
			result = PIPE->lock_vertex_buffer( novaMesh.vb_handle, DDLOCK_WRITEONLY, (void **)&vb_data, &dwSize );
			CQASSERT(result == GR_OK);

			int i;
			for(i = 0; i < NOVA_SEGMENTS; ++i)
			{
				vb_data[i*2].color = alpha<<24 | 0x00ffffff;
				vb_data[i*2].v2 = i+timeDif;
				vb_data[i*2+1].v2 = i+timeDif2;				
			}

			vb_data[NOVA_SEGMENTS*2].color = alpha<<24 | 0x00ffffff;
			vb_data[NOVA_SEGMENTS*2].v2 = NOVA_SEGMENTS+timeDif;
			vb_data[NOVA_SEGMENTS*2+1].v2 = NOVA_SEGMENTS+timeDif2;

			result = PIPE->unlock_vertex_buffer( novaMesh.vb_handle );
			CQASSERT(result == GR_OK);
			
			result = PIPE->draw_primitive_vb( D3DPT_TRIANGLESTRIP, novaMesh.vb_handle, 0, NOVA_SEGMENTS*2+2, 0 );
			CQASSERT(result == GR_OK);
			tempTime -= pData->interRingTime;
		}
	}
}
//----------------------------------------------------------------------------------
//
void NovaExplosion::CastVisibleArea()
{
	if(owner)
	{
		const U32 mask = MGlobals::GetAllyMask(owner.Ptr()->GetPlayerID());
		SetVisibleToAllies(mask);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 NovaExplosion::Update ()
{
	if(time > pData->duration)
		return 0;
	return 1;
}
//----------------------------------------------------------------------------------
//
void NovaExplosion::PhysicalUpdate (SINGLE dt)
{
	time += dt;
}
//----------------------------------------------------------------------------------
//
void NovaExplosion::InitNovaExplosion(IBaseObject * baseOwner)
{
	baseOwner->QueryInterface(IBaseObjectID,owner,NONSYSVOLATILEPTR);
	SetTransform(baseOwner->GetTransform(),baseOwner->GetSystemID());

	CastVisibleArea();
}
//----------------------------------------------------------------------------------
//---------------------------------NovaExplosion Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE NovaExplosionManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(NovaExplosionManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct BlastNode *explosionList;
	U32 factoryHandle;

	//BlastManager methods

	NovaExplosionManager (void) 
	{
	}

	~NovaExplosionManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// NovaExplosionManager methods

NovaExplosionManager::~NovaExplosionManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

}
//--------------------------------------------------------------------------
//
void NovaExplosionManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE NovaExplosionManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BT_NOVA_EXPLOSION* objData = (BT_NOVA_EXPLOSION * )data;
		if (objData->fxClass == FX_NOVA_EXPLOSION)
		{
			NovaExplosionArchetype * handle = new NovaExplosionArchetype;
			handle->pData = objData;
			//build the global mesh
			for(U32 i = 0; i < NOVA_SEGMENTS; ++i)
			{
				novaMesh.ringCenter[i] = Vector(cos((2*PI*i)/NOVA_SEGMENTS),sin((2*PI*i)/NOVA_SEGMENTS),0);
				novaMesh.upperRing[i] = Vector(cos((2*PI*(i+0.5))/NOVA_SEGMENTS)*0.7,sin((2*PI*(i+0.5))/NOVA_SEGMENTS)*0.7,0.2);
			}
			novaMesh.ringTexID = TMANAGER->CreateTextureFromFile("shockwave_color.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			if (CQRENDERFLAGS.bMultiTexture)
				novaMesh.moveTexID = TMANAGER->CreateTextureFromFile("fractal_tile.tga", TEXTURESDIR, DA::TGA,PF_RGB5_A1);
			else
				novaMesh.moveTexID = 0;
			
			novaMesh.RestoreVertexBuffers();

			return handle;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 NovaExplosionManager::DestroyArchetype(HANDLE hArchetype)
{
	NovaExplosionArchetype *deadguy = (NovaExplosionArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * NovaExplosionManager::CreateInstance(HANDLE hArchetype)
{
	NovaExplosionArchetype *pBlast = (NovaExplosionArchetype *)hArchetype;

	NovaExplosion * obj = new ObjectImpl<NovaExplosion>;
	obj->objClass = OC_EFFECT;
	obj->pData = pBlast->pData;

	obj->FRAME_init(*pBlast);

	return obj;
}
//--------------------------------------------------------------------------
//
void NovaExplosionManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct NovaExpGlobal : GlobalComponent
{
	struct NovaExplosionManager *blastMgr;

	virtual void Startup (void)
	{
		blastMgr = new DAComponent<NovaExplosionManager>;
		AddToGlobalCleanupList((IDAComponent **) &blastMgr);
	}

	virtual void Initialize (void)
	{
		blastMgr->init();
	}
};

static NovaExpGlobal _NovaExpGlobal;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End NovaExplosion.cpp------------------------------------
//---------------------------------------------------------------------------
