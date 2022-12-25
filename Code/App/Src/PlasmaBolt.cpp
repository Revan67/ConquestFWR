//--------------------------------------------------------------------------//
//                                                                          //
//                             Projectile.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/PlasmaBolt.cpp 39    11/02/00 1:24p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DWeapon.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjRender.h"
#include "Mission.h"
#include "MGlobals.h"
#include "IMissionActor.h"
#include "IEngineTrail.h"
#include "TManager.h"
#include "Anim2D.h"
#include "UserDefaults.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <ITextureLibrary.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct PlasmaBoltArchetype : RenderArch
{
	const char *name;
	BT_PLASMABOLT_DATA *data;
	ARCHETYPE_INDEX archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	PARCHETYPE PBlastType,pEngineTrailType;
	AnimArchetype *animArch;

	INSTANCE_INDEX textureID;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	PlasmaBoltArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}

	virtual ~PlasmaBoltArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		if (PBlastType)
			ARCHLIST->Release(PBlastType, OBJREFNAME);
//		if (pSparkType)
//			ARCHLIST->Release(pSparkType);
		if(pEngineTrailType)
			ARCHLIST->Release(pEngineTrailType, OBJREFNAME);
		TMANAGER->ReleaseTextureRef(textureID);
		delete animArch;
	}
};

//---------------------------------------------------------------------------
//-----------------------------Class PlasmaBolt------------------------------------
//---------------------------------------------------------------------------

struct _NO_VTABLE PlasmaBolt : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,BOLT_SAVELOAD,PlasmaBoltArchetype> > >, IWeapon, BASE_BOLT_SAVELOAD
{

	BEGIN_MAP_INBOUND(PlasmaBolt)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	END_MAP()

	//------------------------------------------

	const BT_PLASMABOLT_DATA * data;
	HSOUND hSound;
	OBJPTR<IWeaponTarget> target;
	PARCHETYPE PBlastType;
	OBJPTR<IEngineTrail> trail;
	SINGLE range;

	//------------------------------------------
	//non bolt copied code
	Vector spin[4];  //degrees of the spin;
	PlasmaBoltArchetype	*arch;
	AnimInstance *anim;
	//------------------------------------------

	PlasmaBolt (void)
	{
	}

	virtual ~PlasmaBolt (void);	// See ObjList.cpp

	/* IBaseObject methods */

	void update(SINGLE dt);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		transform.translation += (velocity * dt);
		if(trail != 0)
			trail->PhysicalUpdate(dt);
		for(U32 i = 0; i< data->numBolts;++i)
		{
			spin[i] += data->bolts[i].rollSpeed;
		}

		if (bVisible && bDeleteRequested==0)
			update(dt);
	}

	virtual void Render (void);

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
	{
		bVisible = (bDeleteRequested==0 && 
				    systemID == currentSystem &&
				   (IsVisibleToPlayer(currentPlayer) ||
					 defaults.bVisibilityRulesOff ||
					 defaults.bEditorMode) );

		if (bVisible)
		{
			S32 screenX,screenY;
			CAMERA->PointToScreen(transform.translation,&screenX,&screenY);
			
			RECT _rect;
			
			_rect.left  = screenX - 15;
			_rect.right	= screenX + 15;
			_rect.top = screenY - 15;
			_rect.bottom = screenY + 15;
			
			RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
			
			bVisible = RectIntersects(_rect, screenRect);
		}
	}

	virtual void CastVisibleArea(void)
	{
		SetVisibleToAllies(GetVisibilityFlags());
	}

	/* IWeapon methods */

	void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags, const class Vector * pos);

	/* PlasmaBolt methods */

	void init (PlasmaBoltArchetype * data);

	void drawBillboard(Vector offset, Vector & i, Vector & j, U32 boltNumber);

	void drawBolt(U32 i);
};
//---------------------------------------------------------------------------
//
static PlasmaBolt * CreatePlasmaBolt (PlasmaBoltArchetype * data)
{
	PlasmaBolt * bolt = new ObjectImpl<PlasmaBolt>;

	bolt->FRAME_init(*data);
	bolt->init(data);
	
	return bolt;
}
//----------------------------------------------------------------------------------
//
void PlasmaBolt::init (PlasmaBoltArchetype * _arch)
{
	CQASSERT(_arch->data->wpnClass == WPN_PLASMABOLT);
	CQASSERT(_arch->data->objClass == OC_WEAPON);

	arch = _arch;

	objClass = OC_WEAPON;
	pArchetype = arch->pArchetype;
	PBlastType = arch->PBlastType;
	data = (BT_PLASMABOLT_DATA *)arch->data;

//	if (data->MASS)
//		PHYSICS->set_mass(instanceIndex, data->MASS);

	if(arch->pEngineTrailType != 0)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(arch->pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail,NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}

	}

//	textureID = arch.textureID;
	if (arch->animArch)
	{
		anim = new AnimInstance;
		anim->Init(arch->animArch);
		anim->SetWidth(data->bolts[0].boltWidth);
		anim->ForceFront(1);
	}
}
//----------------------------------------------------------------------------------
//
void PlasmaBolt::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	Vector vel;
	TRANSFORM orient = orientation;
	U32 visFlags=owner->GetTrueVisibilityFlags();

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	launchFlags = flags;

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, target,SYSVOLATILEPTR);
		CQASSERT(target!=0);

		targetID = _target->GetPartID();
		visFlags |= _target->GetTrueVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (owner->GetPlayerID()-1));
	}

	SetVisibilityFlags(visFlags);
	bVisible = owner->bVisible;

	start = orientation.get_position();

	//Correct bolt to fire at target regardless of gun barrel
	SINGLE curPitch, desiredPitch, relPitch;
	Vector goal = (pos)? *pos : _target->GetPosition();

	curPitch = orient.get_pitch();
	goal -= orient.get_position();

	range = goal.magnitude() + 1000;


	desiredPitch = sqrt(goal.x * goal.x  + goal.y * goal.y);
	desiredPitch = get_angle(goal.z, desiredPitch);

	relPitch = desiredPitch - curPitch;

	orient.rotate_about_i(relPitch);
////------------------------
	transform = orient;
	initialPos = orientation.get_position();
	
	vel = -orient.get_k();
	vel *= data->maxVelocity;
	velocity = vel;
	direction = vel;

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&start);

	for(U32 i = 0; i <data->numBolts; ++i)
	{
		spin[i] = Vector(0,0,0);
	}
}
//----------------------------------------------------------------------------------
//
PlasmaBolt::~PlasmaBolt (void)
{
	OBJLIST->ReleaseProjectile();
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
	if(trail != 0)
	{
		delete trail.Ptr();
		trail = 0;
	}
	if (anim)
		delete anim;
}

BOOL32 PlasmaBolt::Update (void)
{
	if (bVisible == false && bDeleteRequested==0)
		update(ELAPSED_TIME);

	return (bDeleteRequested==0);
}
//----------------------------------------------------------------------------------
//
void PlasmaBolt::update (SINGLE dt)
{
	Vector pos;
	//
	// check to make sure we haven't exceeded our max range
	//
	
	pos = transform.get_position();
	pos -= initialPos;
	pos.z = 0;

	if (pos.magnitude() > range)
		bDeleteRequested = 1;
	
	if (target!=0)
	{
		Vector collide_point,dir;

		if (target->GetCollisionPosition(collide_point,dir,start,direction))
		{
			SINGLE distance;
			Vector diff = collide_point - transform.translation;
			diff.z = 0;
			distance = diff.magnitude();
			if (distance < data->maxVelocity * dt)
			{
				if (bVisible)
				{
					IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					if (blast)
						OBJLIST->AddObject(blast);
				}

				if ((launchFlags & IWF_ALWAYS_MISS)==0)
				{
				//	target->ApplyDamage(this, ownerID, collide_point, dir, data->damage);
					target->ApplyVisualDamage(this,ownerID,start,direction,data->damage);
				}
				bDeleteRequested = TRUE;
			}
		}
		else
		if (launchFlags & IWF_ALWAYS_HIT)
		{
			//
			// are we heading towards the target?
			//
			dir = target.Ptr()->GetTransform().translation - transform.translation;
			SINGLE relYaw = get_angle(dir.x, dir.y) - transform.get_yaw();

			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;

			if (fabs(relYaw) > 60*MUL_DEG_TO_RAD)	// we've gone past
			{
				if (bVisible)
				{
					IBaseObject * blast = CreateBlast(PBlastType, transform, systemID);

					if (blast)
						OBJLIST->AddObject(blast);
				}

				target->ApplyVisualDamage(this, ownerID, transform.translation, dir, data->damage);
				bDeleteRequested = TRUE;
			}
		}
	}
	if(trail != 0)
		trail->Update();
}

void PlasmaBolt::drawBillboard(Vector offset, Vector & i, Vector & j, U32 boltNumber)
{
	Transform transOp;
	transOp.set_orientation(spin[boltNumber].x,spin[boltNumber].y,spin[boltNumber].z);
	offset = transOp*offset;
	Vector epos = transform*offset;		
		
	Vector v[4];
	SINGLE size = data->bolts[boltNumber].boltWidth;
	v[0] = epos - i*size - j*size;
	v[1] = epos + i*size - j*size;
	v[2] = epos + i*size + j*size;
	v[3] = epos - i*size + j*size;

	PB.TexCoord2f(0,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
	PB.TexCoord2f(0,1.0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
	PB.TexCoord2f(1.0,1.0);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
	PB.TexCoord2f(1.0,0.0);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);
	

}

void PlasmaBolt::drawBolt(U32 boltNumber)
{
	PB.Color4ub(data->bolts[boltNumber].boltColor.red,data->bolts[boltNumber].boltColor.green,data->bolts[boltNumber].boltColor.blue,
		data->bolts[boltNumber].boltColor.alpha);
	
	Vector offset = data->bolts[boltNumber].offset;

	Vector pos = transform.translation+offset;
		
	Vector cpos (CAMERA->GetPosition());
		
	Vector look (pos - cpos);
		
	Vector k = look.normalize();

	Vector tmpUp(pos.x,pos.y,pos.z+50000);

	Vector j (cross_product(k,tmpUp));
	j.normalize();

	Vector i (cross_product(j,k));

	i.normalize();

	TRANSFORM trans;
	trans.set_i(i);
	trans.set_j(j);
	trans.set_k(k);

	i = trans.get_i();
	j = trans.get_j();
	
	SINGLE boltSpacing = data->bolts[boltNumber].boltSpacing;
	U32 segmentsX = data->bolts[boltNumber].segmentsX;
	U32 segmentsY = data->bolts[boltNumber].segmentsY;
	U32 segmentsZ = data->bolts[boltNumber].segmentsZ;

	SINGLE xPos = boltSpacing;
	SINGLE yPos;
	SINGLE zPos;
	U32 xLoc,yLoc,zLoc;


	for(xLoc = 1; xLoc <= segmentsX; ++xLoc)
	{
		yPos = boltSpacing;
		for(yLoc = 1; yLoc <= segmentsY; ++yLoc)
		{
			if(xLoc != segmentsX || yLoc != segmentsY)
			{
				zPos = boltSpacing;
				for(zLoc = 1; zLoc <= segmentsZ; ++zLoc)
				{
					if((!(xLoc == segmentsX || yLoc == segmentsY)) ||
						(zLoc != segmentsZ))
					{
						drawBillboard(offset+Vector(xPos,-yPos,-zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(-xPos,-yPos,-zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(xPos,yPos,-zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(-xPos,yPos,-zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(xPos,-yPos,zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(-xPos,-yPos,zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(xPos,yPos,zPos),i,j,boltNumber);
						drawBillboard(offset+Vector(-xPos,yPos,zPos),i,j,boltNumber);
					}
					zPos += boltSpacing;
				}
			}
			yPos += boltSpacing;
		}
		xPos += boltSpacing;
	}


	yPos = boltSpacing;
	for(yLoc = 1; yLoc <= segmentsY; ++yLoc)
	{
		zPos = boltSpacing;
		for(zLoc = 1; zLoc <= segmentsZ; ++zLoc)
		{
			if(zLoc != segmentsZ || yLoc != segmentsY)
			{
				drawBillboard(offset+Vector(0,yPos,zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(0,-yPos,zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(0,-yPos,-zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(0,yPos,-zPos),i,j,boltNumber);	
			}
			zPos += boltSpacing;
		}
		yPos += boltSpacing;
	}

	xPos = boltSpacing;
	for(xLoc = 1; xLoc <= segmentsX; ++xLoc)
	{
		zPos = boltSpacing;
		for(zLoc = 1; zLoc <= segmentsZ; ++zLoc)
		{
			if(xLoc != segmentsX || zLoc != segmentsZ)
			{
				drawBillboard(offset+Vector(xPos,0,zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(-xPos,0,zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(-xPos,0,-zPos),i,j,boltNumber);									
				drawBillboard(offset+Vector(xPos,0,-zPos),i,j,boltNumber);
			}
			zPos += boltSpacing;
		}
		xPos += boltSpacing;
	}
	xPos = boltSpacing;
	for(xLoc = 1; xLoc <= segmentsX; ++xLoc)
	{
		yPos = boltSpacing;
		for(yLoc = 1; yLoc <= segmentsY; ++yLoc)
		{
			if(xLoc != segmentsX || yLoc != segmentsY)
			{
				drawBillboard(offset+Vector(xPos,yPos,0),i,j,boltNumber);									
				drawBillboard(offset+Vector(-xPos,yPos,0),i,j,boltNumber);									
				drawBillboard(offset+Vector(-xPos,-yPos,0),i,j,boltNumber);									
				drawBillboard(offset+Vector(xPos,-yPos,0),i,j,boltNumber);
			}
			yPos += boltSpacing;
		}
		xPos += boltSpacing;
	}
	
	xPos = boltSpacing;
	for(xLoc = 1; xLoc <= segmentsX; ++xLoc)
	{
		drawBillboard(offset+Vector(-xPos,0,0),i,j,boltNumber);													
		drawBillboard(offset+Vector(xPos,0,0),i,j,boltNumber);													
		xPos += boltSpacing;
	}
	yPos = boltSpacing;
	for(yLoc = 1; yLoc <= segmentsY; ++yLoc)
	{
		drawBillboard(offset+Vector(0,-yPos,0),i,j,boltNumber);													
		drawBillboard(offset+Vector(0,yPos,0),i,j,boltNumber);													
		yPos += boltSpacing;
	}
	zPos = boltSpacing;
	for(zLoc = 1; zLoc <= segmentsZ; ++zLoc)
	{
		drawBillboard(offset+Vector(0,0,-zPos),i,j,boltNumber);													
		drawBillboard(offset+Vector(0,0,zPos),i,j,boltNumber);													
		zPos += boltSpacing;
	}
	
	drawBillboard(offset,i,j,boltNumber);
}

void PlasmaBolt::Render()
{
	if (bVisible)
	{
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		TreeRender(mc);
	
		if(arch->textureID)
		{
			CAMERA->SetModelView();
			SetupDiffuseBlend(arch->textureID,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);

			PB.Begin(PB_QUADS);
			for(int i = 0; i < data->numBolts; ++i)
			{
				drawBolt(i);
			}
			PB.End();
		}

		if (anim)
		{
			BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			SINGLE dt = OBJLIST->GetRealRenderTime();
			anim->update(dt);
			anim->SetPosition(transform.translation);
			ANIM2D->render(anim);
		}

		if(trail != 0)
			trail->Render();
	}
}

//----------------------------------------------------------------------------------------------
//-------------------------------class ProjectileManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE PlasmaBoltManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(PlasmaBoltManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	PlasmaBoltArchetype *pArchetype;

	//ProjectileManager methods

	PlasmaBoltManager (void) 
	{
	}

	~PlasmaBoltManager();
	
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
// ProjectileManager methods

PlasmaBoltManager::~PlasmaBoltManager()
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
void PlasmaBoltManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE PlasmaBoltManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_PROJECTILE_DATA * dataProj = (BT_PROJECTILE_DATA *)_data;
		switch (dataProj->wpnClass)
		{
		case WPN_PLASMABOLT:
			{
				BT_PLASMABOLT_DATA * data = (BT_PLASMABOLT_DATA *)_data;

				PlasmaBoltArchetype *newguy = new PlasmaBoltArchetype;
			
				newguy->name = szArchname;
				newguy->data = data;
				newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
				if (data->blastType[0])
				{
					if ((newguy->PBlastType = ARCHLIST->LoadArchetype(data->blastType)) != 0)
						ARCHLIST->AddRef(newguy->PBlastType, OBJREFNAME);
				}
/*				if (data->sparkType[0])
				{
					if ((newguy->pSparkType = ARCHLIST->LoadArchetype(data->sparkType)) != 0)
						ARCHLIST->AddRef(newguy->pSparkType);
				}*/
				if (data->engineTrailType[0])
				{
					if ((newguy->pEngineTrailType = ARCHLIST->LoadArchetype(data->engineTrailType)) != 0)
						ARCHLIST->AddRef(newguy->pEngineTrailType, OBJREFNAME);
				}
				//
				// force preload of sound effect
				// 
				SFXMANAGER->Preload(data->launchSfx);
				
				if (data->animName[0])
				{
					DAFILEDESC fdesc;
					COMPTR<IFileSystem> objFile;
					fdesc.lpFileName = data->animName;
					if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
					{
						newguy->animArch = ANIM2D->create_archetype(objFile);
					}
				}
				else
				{
					if (data->textureName[0])
						newguy->textureID = TMANAGER->CreateTextureFromFile(data->textureName, TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
					else
						newguy->textureID = 0;
				}


				DAFILEDESC fdesc = data->fileName;
				COMPTR<IFileSystem> file;
				if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
					TEXLIB->load_library(file, 0);
				
				if (file != 0 && (newguy->archIndex = ENGINE->create_archetype(((BT_PROJECTILE_DATA *)data)->fileName, file)) != INVALID_ARCHETYPE_INDEX)
				{
				}
				else
				{
					delete newguy;
					newguy = 0;
				}

				return newguy;
			}
			break;
			
		default:
			break;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 PlasmaBoltManager::DestroyArchetype(HANDLE hArchetype)
{
	PlasmaBoltArchetype *deadguy = (PlasmaBoltArchetype *)hArchetype;	

	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * PlasmaBoltManager::CreateInstance(HANDLE hArchetype)
{
	PlasmaBoltArchetype *pWeapon = (PlasmaBoltArchetype *)hArchetype;
	BT_PLASMABOLT_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_PLASMABOLT:
			{
				obj = CreatePlasmaBolt(pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void PlasmaBoltManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _plasmaBolts : GlobalComponent
{
	PlasmaBoltManager *plasmaBoltMng;

	virtual void Startup (void)
	{
		plasmaBoltMng = new DAComponent<PlasmaBoltManager>;
		AddToGlobalCleanupList((IDAComponent **) &plasmaBoltMng);
	}

	virtual void Initialize (void)
	{
		plasmaBoltMng->init();
	}
};

static _plasmaBolts plasmaBolts;

//--------------------------------------------------------------------------//
//------------------------------END Projectile.cpp--------------------------------//
//--------------------------------------------------------------------------//
