//--------------------------------------------------------------------------//
//                                                                          //
//                                Damage.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Damage.cpp 25    10/13/00 12:04p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Damage.h"
#include "Objlist.h"
#include "TObjExtension.h"
#include "TObjDamage.h"
#include "TObjExtent.h"
#include "TObjWarp.h"
#include "TObjMove.h"
#include "TObjControl.h"
#include "TObjPhys.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjSelect.h"
#include "TObjMission.h"
#include "TObjBuild.h"
#include "Field.h"

#include <DSpaceship.h>
#include <DShipSave.h>
#include <DBaseData.h>
#include <DPlatform.h>
#include <DPlatSave.h>

bool SphereTriangle  (Vector v[3],Vector pt,SINGLE rad);

struct ShieldMap shieldMapBank[NUM_SHIELD_MAPS];

ShieldMap * GetShieldHitMap()
{
	for (int i=0;i<NUM_SHIELD_MAPS;i++)
	{
		if (shieldMapBank[i].poly_cnt == 0)
			return &shieldMapBank[i];
	}

	return 0;
}

typedef SPACESHIP_INIT<BASE_SPACESHIP_DATA> BASESHIPINIT;

//--------------------------------------------------------------------------//
//
template ObjectDamage
			<ObjectBuild
		        <ObjectExtent
		         <ObjectWarp
			      <ObjectMove
				   <ObjectSelection
					<ObjectMission
					 <ObjectPhysics
					  <ObjectControl
					   <ObjectTransform
					    <ObjectFrame<struct IBaseObject,struct SPACESHIP_SAVELOAD, BASESHIPINIT> >
					   >
					  > 
					 > 
					> 
				   >
				  >
				 >
				 >
				>;

typedef PLATFORM_INIT<BASE_PLATFORM_DATA> BASEPLATINIT;

template ObjectDamage
			<ObjectBuild
			<ObjectExtension
			 <ObjectExtent
			  <ObjectSelection
			   <ObjectMission
			    <ObjectTransform
			     <ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT> >
				>
			   >
			  >
			 >
			 >
			>;

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectDamage< Base >::ObjectDamage (void) :
					saveNode(this, SaveLoadProc(&ObjectDamage::saveDamage)),
					loadNode(this, SaveLoadProc(&ObjectDamage::loadDamage)),
					postRenderNode(this, RenderProc(&ObjectDamage::damagePostRender)),
					updateNode(this, UpdateProc(&ObjectDamage::updateDamage)),
					initNode(this, InitProc(&ObjectDamage::initDamage)),
					physUpdateNode(this, PhysUpdateProc(&ObjectDamage::physUpdateDamage)),
					resolveNode(this, ResolveProc(&ObjectDamage::resolveDamage))
{
	threshold = 0.5;
	U8 i;
	for (i=0;i<NUM_FIRES;i++)
	{
		fire[i].index = INVALID_INSTANCE_INDEX;
	}
	bShieldsUp = TRUE;
}

template <class Base>
U32 ObjectDamage< Base >::GetNumDamageSpots ()
{
	return numDamages;
}

template <class Base>
void ObjectDamage< Base >::GetNextDamageSpot (Vector & vect, Vector & dir)
{
	lastRepairSlot=-1;
	if(numDamages)
	{
		for (int i=0;i<DAMAGE_RECORDS;i++)
		{
			if (damageSave[i].bActive)
			{
				lastRepairSlot = i;
			}
		}

		if (lastRepairSlot != -1)
		{
			vect = damageSave[lastRepairSlot].pos;
			dir = (GetPosition()-vect).normalize();
		}
	}
	if(lastRepairSlot == -1)
	{
		OBJBOX box;
		GetObjectBox(box);

		vect = Vector(0,box[2],0);
		dir = Vector(0,0,1);
	}
}

template <class Base>
void ObjectDamage< Base >::FixDamageSpot ()
{
	if(numDamages)
	{
		if (lastRepairSlot != -1)
		{
			CQASSERT(damageSave[lastRepairSlot].bActive);
			lastDamage += 0.12;
			damageSave[lastRepairSlot].damage = 0;
			damageSave[lastRepairSlot].bActive = 0;
			delete damageRecord[lastRepairSlot].ec;
			damageRecord[lastRepairSlot].ec = 0;
			numDamages--;
			lastRepairSlot = -1;
		}
	}
}

template <class Base> 
ObjectDamage< Base >::~ObjectDamage (void) 
{
	int i;
	for (i=0;i<NUM_FIRES;i++)
	{
	//	MODEL->disconnect(fire[i],instanceIndex);
		ENGINE->destroy_instance(fire[i].index);
		fire[i].index = INVALID_INSTANCE_INDEX;
	}
	/*if (spark_archID != INVALID_ARCHETYPE_INDEX)
	{
		ENGINE->release_archetype(spark_archID);
		spark_archID = INVALID_ARCHETYPE_INDEX;
	}*/

//	freeArrays();

	//TEMP
	if (vertex_normals)
		delete [] vertex_normals;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectDamage< Base >::saveDamage (DMGSAVEINFO & save)
{
	save.damage_SL = *static_cast<DAMAGE_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectDamage< Base >::loadDamage (DMGSAVEINFO & load)
{
	*static_cast<DAMAGE_SAVELOAD *>(this) = load.damage_SL;

	for (int i=0;i<DAMAGE_RECORDS;i++)
	{
		// use "load" variable to prevent optimizer error (jy)
		if (load.damage_SL.damageSave[i].bActive)
			DamageSpot(i);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::initDamage (const DAMAGEINITINFO & data)
{
	//CURRENT CODE
	smoke_archID=data.smoke_archID;

//	ListChildren(instanceIndex);
//	numChildren = GetNumChildren();

	damageTexID = data.damageTexID;
	pDamageBlast = data.pDamageBlast;
	pSparkBlast = data.pSparkBlast;

	// SHIELD STUFF
/*	if (shieldTexID == INVALID_TXM_ID)
	{
		shieldTexID = CreateTextureFromFile("shield_blob.tga", TEXTURESDIR,DA::TGA,PF_4CC_DAA4);
	}*/
	/*if (shieldArch == 0)
	{
		fdesc.lpFileName = "SH_blue.anm";
		if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
		{
			shieldArch = ANIM2D->create_archetype(file);
		}
	}*/

	shieldAnimArch = data.shieldAnimArch;
	shieldFizzAnimArch = data.shieldFizzAnimArch;
	damageAnimArch = data.damageAnimArch;

/*	for (int c=0;c<numChildren;c++)
	{
		CQASSERT (mc[c].instanceIndex != INVALID_INSTANCE_INDEX);
		
	}*/

	if (extentData->bX)
		sizeFactor = (box[BBOX_MAX_X]-box[BBOX_MIN_X])*0.00025;
	else
		sizeFactor = (box[BBOX_MAX_Y]-box[BBOX_MIN_Y])*0.00025;

	lastDamage = 0.72;


	smesh = data.smesh;
//	m_extent = data.m_extent;
	if (data.pData->objClass == OC_SPACESHIP)
		bUseSMeshAsShield = true;
	else
		bUseSMeshAsShield = false;
	getScale();

	shieldSoundID = data.pData->shield.sfx;
	fizzSoundID = data.pData->shield.fizzOut;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectDamage< Base >::damagePreRender (void)
{

}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectDamage< Base >::damagePostRender (void)
{

	
	/*if (!bExploding && instanceIndex != INVALID_INSTANCE_INDEX)
	{
		renderDamageSpots();
	}*/

		//desirable? -no?
	//CAMERA->SetModelView();

	if (shieldAnimArch && smesh)
	{
		renderShieldHits();

		if (shieldDownTimer > 0 && shieldFizzAnimArch)
			renderShieldDown();
	//	else if (shieldDownTimer <= 0 && bShieldsUp == 0 && shieldFizzAnimArch)
	//		renderShieldUp(shieldFizzAnimArch->frames[rand()%4].texture);
	//	renderShield();
	}

	int i=0;
	int cnt=0;
	while (cnt < numFires)
	{
		if (fire[i].index != INVALID_INSTANCE_INDEX)
		{
			cnt++;
			ENGINE->set_transform(fire[i].index,transform.multiply(fire[i].trans));
			ENGINE->render_instance(MAINCAM,fire[i].index,0,LODPERCENT,0,0);
		}
		i++;
	}


}

//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectDamage< Base >::renderDamageSpots ()
{
/*	BATCH->set_state(RPR_BATCH,TRUE);
	CAMERA->SetModelView(&transform);
	if (numDamages > 0)
	{
		for (int i=0;i<DAMAGE_RECORDS;i++)
		{
			if (damageSave[i].bActive)
			{
				val = 255*(0.7+0.3*cos(2*damageRecord[i].dmgTimer));

				val += 30;
				if (val > 510)
					val -= 510;
				S32 mag=val;
				if (mag > 255)
					mag = 510-mag;
				CQASSERT( mag >= 0 && mag <= 255);
				U32 color = 0xff000000 | (mag<<16) | (mag<<8) | mag;
				damageRecord[i].ec->RenderWithTexture(damageTexID,color);
			}
		}
	}*/
}
//---------------------------------------------------------------------------
//
static bool bUpdateDamage = true;

template <class Base>
BOOL32 ObjectDamage< Base >::updateDamage (void)
{
	if(!bUpdateDamage)
		return 1;

	if (bExploding || instanceIndex == INVALID_INSTANCE_INDEX)
		goto UpdateOnly;

	SINGLE hull;

	if (hullPointsMax)
		hull = (SINGLE)hullPoints / (SINGLE)hullPointsMax;
	else 
		hull = 1.0F;


//repair stuff
	if (building && hull > lastDamage && lastDamage <= 0.6f)
	{
		lastDamage += 0.12f;
		Vector dummy,dummy2;
		GetNextDamageSpot(dummy,dummy2);
		FixDamageSpot();

		/*S32 slot=-1;
		SINGLE slotDamage=0;
		for (int i=0;i<DAMAGE_RECORDS;i++)
		{
			if (damageSave[i].bActive && damageSave[i].damage > slotDamage)
			{
				slot = i;
				slotDamage = damageSave[i].damage;
			}
		}

		if (slot != -1)
		{
			damageSave[slot].damage = 0;
			damageSave[slot].bActive = 0;
			numDamages--;
		}*/
	}

	timer++;
	U8 i;
	if ((timer%8 == 0 && rand()%5==0) && hull < 0.25 && bReady)
	{
		S8 slot=-1;
		int i=0;
		while (i<NUM_FIRES && slot == -1)
		{
			if (fire[i].index == INVALID_INSTANCE_INDEX)
			{
				slot = i;
			}
			i++;
		}
		if (slot >= 0)
		{
			numFires++;
			CQASSERT(numFires <= NUM_FIRES);
			fire[slot].index = ENGINE->create_instance2(smoke_archID,NULL);
			fire[slot].trans.set_identity();
			fire[slot].trans.rotate_about_i(PI/2);
			fire[slot].trans.rotate_about_j(PI);
			ENGINE->set_transform(fire[slot].index,fire[slot].trans);
//			SINGLE pos = _min+min_slice*_step+(BBOX_MAX__slice-min_slice)*_step*rand()/RAND_BBOX_MAX_;
			SINGLE pos;
			S32 choice = rand()%DAMAGE_RECORDS;
			S32 c=0;
			while (!damageSave[choice].damage && c < DAMAGE_RECORDS)
			{
				c++;
				choice = (choice+1)%DAMAGE_RECORDS;
			}
			

			Vector src = damageSave[choice].pos;
			if (extentData->bX)
			{
			//	pos = (src.z-_min)/(box[BBOX_MAX_Z]-_min);
				pos = src.x;
			}
			else
			{
				//pos = (src.x-_min)/(box[BBOX_MAX_X]-_min);
				pos = src.y;
			}

			const RECT *rect = GetExtentRect(pos);
			if (extentData->bX)
			{
				fire[slot].trans.translation.x = pos;
				if (src.y < 0.8*rect->left)
					src.y = 0.8*rect->left;
				if (src.y > 0.8*rect->right)
					src.y = 0.8*rect->right;
				fire[slot].trans.translation.y = src.y;//0.8*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}
			else
			{
				fire[slot].trans.translation.y = pos;
				if (src.x < 0.8*rect->left)
					src.x = 0.8*rect->left;
				if (src.x > 0.8*rect->right)
					src.x = 0.8*rect->right;
				fire[slot].trans.translation.x = src.x;//0.8*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}


			fire[slot].trans.translation.z = rect->top;
			fire[slot].timer = timer+140;
		}

	}
	if ((timer%6 == 4 && rand()%2 == 0) && hull < 0.5 && bReady)
	{
		/*S8 slot=-1;

		for (i=0;i<NUM_FIRES;i++)
		{
			if (fire[i].index == INVALID_INSTANCE_INDEX)
			{
				slot = i;
			}
		}
		if (slot >= 0)
		{*/
			/*numFires++;
			CQASSERT(numFires <= NUM_FIRES);*/
			IBaseObject *obj = ARCHLIST->CreateInstance(pSparkBlast);
			AddChildBlast(obj);

			Transform trans;
			//SINGLE pos = _min+(min_slice+1)*_step+(BBOX_MAX__slice-min_slice-2)*_step*rand()/RAND_BBOX_MAX_;

			SINGLE pos;
			S32 choice = rand()%DAMAGE_RECORDS;
			S32 c=0;
			while (!damageSave[choice].damage && c < DAMAGE_RECORDS)
			{
				c++;
				choice = (choice+1)%DAMAGE_RECORDS;
			}
			

			Vector src = damageSave[choice].pos;
			if (extentData->bX)
			{
		//		pos = (src.z-_min)/(box[BBOX_MAX_Z]-_min);
				pos = src.x;
			}
			else
			{
		//		pos = (src.x-_min)/(box[BBOX_MAX_X]-_min);
				pos = src.y;
			}

			const RECT *rect = GetExtentRect(pos);
			if (extentData->bX)
			{
				trans.translation.x = pos;
				if (src.y < 0.8*rect->left)
					src.y = 0.8*rect->left;
				if (src.y > 0.8*rect->right)
					src.y = 0.8*rect->right;
				trans.translation.y = src.y;//0.8*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}
			else
			{
				trans.translation.y = pos;
				if (src.x < 0.8*rect->left)
					src.x = 0.8*rect->left;
				if (src.x > 0.8*rect->right)
					src.x = 0.8*rect->right;
				trans.translation.x = src.x;//0.7*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}

	/*		if (bZ)
			{
				trans.translation.z = pos;
				trans.translation.x = 0.5*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}
			else
			{
				trans.translation.x = pos;
				trans.translation.z = 0.5*(rect->left+(rect->right-rect->left)*rand()/RAND_BBOX_MAX_);
			}*/

			trans.translation.z = rect->top+50;
			OBJPTR<IBlast> blast;

			if (obj->QueryInterface(IBlastID,blast))
			{
				blast->InitBlast(trans,systemID,this,0.2+box[BBOX_MAX_Z]/4500.0);
			}
		//}
	}

UpdateOnly:

	i=0;
	int cnt=0;
	while (cnt < numFires)
	{	
		if (fire[i].index != INVALID_INSTANCE_INDEX)
		{
			if (timer == fire[i].timer)
			{
				ENGINE->destroy_instance(fire[i].index);
				fire[i].index = INVALID_INSTANCE_INDEX;
				numFires--;
			}
			else
				cnt++;
		}
		i++;
	}

	IBaseObject *pos = childBlastList,*last=NULL;
	while (pos)
	{
		if (pos->Update()==0)
		{
			if (last)
			{
				last->next = pos->next;
				pos->next = 0;		// so objlist wont do bad things (jy)
				delete pos;
				pos = last->next;
			}
			else
			{
				childBlastList = pos->next;
				pos->next = 0;		// so objlist wont do bad things (jy)
				delete pos;
				pos = childBlastList;
			}
		}
		else
		{
			last = pos;
			pos = pos->next;
		}
	}

	//SHIELD STUFF
	if (fieldFlags.shieldsInoperable() == bShieldsUp)
	{
		bShieldsUp = !bShieldsUp;
		if (fizzSound==0)
			fizzSound = SFXMANAGER->Open(fizzSoundID);
		SFXMANAGER->Play(fizzSound, systemID, &transform.translation);
		shieldDownTimer = 1.0;
	}

	if (fieldFlags.bHades && bVisible)
	{
		if (timer%20 == 0) //BAD
		{
			FIELDMGR->CreateFieldBlast(this,Vector(0,0,0),systemID);
		}
	}
	
	//apply damage each second, based on the really lame timer mechanism that now infects every ship
	if (fieldFlags.damagePerTwentySeconds)
	{
		S32 quantumTime1 = floor((coarseTimer*fieldFlags.damagePerTwentySeconds)/20.0f);
		coarseTimer += ELAPSED_TIME;
		S32 quantumTime2 = floor((coarseTimer*fieldFlags.damagePerTwentySeconds)/20.0f);
		if (coarseTimer > 100.0)
			coarseTimer -= 100.0;
		if (quantumTime2-quantumTime1)
			ApplyAOEDamage(0,quantumTime2-quantumTime1);
	}

	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::physUpdateDamage(SINGLE dt)
{
	/*int i;
	for (i=0;i<DAMAGE_RECORDS;i++)
	{
		if (damageSave[i].bActive)
		{
			damageRecord[i].dmgTimer += dt;
		}
	}*/

	int i=0;
	int cnt=0;
	while (cnt < numFires)
	{
		if (fire[i].index != INVALID_INSTANCE_INDEX)
		{
			cnt++;
			ENGINE->update_instance(fire[i].index,0,dt);
		}
		i++;
	}

	IBaseObject *pos = childBlastList;
	while (pos)
	{
		pos->PhysicalUpdate(dt);
		pos = pos->next;
	}

	for (i=0;i<NUM_SHIELD_HITS;i++)
	{
		if (shieldHitPolys[i])
		{
			shieldHitPolys[i]->timer -= dt*2;
			if (shieldHitPolys[i]->timer < 0)
			{
				shieldHitPolys[i]->timer = 0;
				shieldHitPolys[i]->poly_cnt =0;
				shieldHitPolys[i] = 0;
				numShieldHits--;
			}
		}
	}

	if (shieldDownTimer > 0)
		shieldDownTimer -= 0.7*dt;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::resolveDamage()
{
	bShieldsUp = (!fieldFlags.shieldsInoperable());
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::CreateShieldHit(const Vector & pos, const Vector &dir,Vector &collide_point,U32 damage)
{
#if 1
	if (bUseSMeshAsShield && bShieldsUp && hullPoints >= 0.3*hullPointsMax)
	{
		if (smesh && shieldAnimArch && !fieldFlags.shieldsInoperable())
		{
			GenerateShieldHit(pos,dir,collide_point,damage);
			numShieldHits++;
		}
	}
#else
	IBaseObject *obj;
	if ((obj = ARCHLIST->CreateInstance(pShieldHitType)) != 0)
	{
		OBJPTR<IBlast> blast;
		TRANSFORM trans;// = collider->GetTransform();
		{
			Vector i,j,k;
			k = dir;
			k.normalize();
			j.set(k.z*10,-k.x*20,k.y);
			i = cross_product(j,k);
			i.normalize();
			j = cross_product(k,i);
			trans.set_i(i);
			trans.set_j(j);
			trans.set_k(k);
		}
		
		trans.set_position(pos);
		TRANSFORM inv = transform.get_inverse();
		trans = inv.multiply(trans);
		if (obj->QueryInterface(IBlastID, blast))
		{
			blast->InitBlast(trans, systemID,this);
		}
		IBaseObject *blastPos = childBlastList;
		if (!blastPos)
		{
			childBlastList = obj;//blastPos;
		}
		else
		{
			while (blastPos->next)
			{
				blastPos = blastPos->next;
			}
			blastPos->next = obj;//new ChildBlast;
		}
	}
#endif
}

#if 0
template <class Base>
void ObjectDamage< Base >::GenerateShieldHit(const Vector & pos,const Vector &dir,U32 damage)
{

	const BaseExtent *extent,*e_pos,*cm_extent=0;
	CollisionMesh *cmesh=0;
	PHYSICS->get_extent(&extent,instanceIndex);
	if (extent)
	{
		//find first convex mesh for now
		while (extent && cmesh==0)
		{
			e_pos = extent;
			while (e_pos && cmesh==0)
			{
				if (e_pos->type == ET_CONVEX_MESH)
				{
					cmesh = (CollisionMesh *)e_pos->get_primitive();
					cm_extent = e_pos;
				}
				e_pos = e_pos->next;
			}
			extent = extent->child;
		}
		
		if (cmesh)
		{
			
			if (vertex_normals == 0)
				GenerateNormals(cmesh);
			
			S32 slot=-1;
			for (int cc=0;cc<NUM_SHIELD_HITS;cc++)
			{
				if (shieldHitPolys[cc] == 0)
					slot = cc;
			}
			if (slot != -1)
			{
				shieldHitPolys[slot] = GetShieldHitMap();
				if (shieldHitPolys[slot])
				{
					Vector norm;
					Vector collide_point;
					//	TRANSFORM scaleTrans;
					//	scaleTrans.d[0][0] = SHIELD_SCALE;
					//	scaleTrans.d[1][1] = SHIELD_SCALE;
					//	scaleTrans.d[2][2] = SHIELD_SCALE;
					
					//TRANSFORM trans = scaleTrans*transform;
					Vector opos = transform.inverse_rotate_translate(pos);
					Vector odir = transform.inverse_rotate(dir);
					opos = opos/SHIELD_SCALE;
					odir.normalize();
					if (target->GetModelCollisionPosition(collide_point,norm, pos-dir*1000, dir))
					{
						//collide_point = trans.inverse_rotate_translate(collide_point);
						//	norm = trans.inverse_rotate(norm);
						
						Vector base_i,base_j;
						base_i.set(-norm.y,norm.x,0);
						if (base_i.x == 0 && base_i.y ==0)
							base_i.set(0,0,1);
						else
							base_i.normalize();
						
						base_j = cross_product(norm,base_i);
						
						//TEST CODE START
						/*	shieldHitPolys.poly_cnt = 1;
						shieldHitPolys.v[0][0] = collide_point-base_j*400;
						shieldHitPolys.v[0][1] = collide_point+base_j*200-base_i*300;
						shieldHitPolys.v[0][2] = collide_point+base_j*200+base_i*300;
						shieldHitPolys.t[0][0].u = 0.5;
						shieldHitPolys.t[0][0].v = -0.333333;
						shieldHitPolys.t[0][1].u = -0.333333;
						shieldHitPolys.t[0][1].v = 1.0;
						shieldHitPolys.t[0][2].u = 1.333333;
						shieldHitPolys.t[0][2].v = 1.0;
						
						return;*/
						
						//TEST CODE END
						U32 next_poly = 0;
						
						for (int t=0;t<cmesh->num_triangles;t++)
						{
							TexCoord tc[3];
							Vector v[3];
							Vector n[3];
							Vector i,j;
							
							v[0] = cmesh->vertices[cmesh->triangles[t].v[0]].p;
							v[1] = cmesh->vertices[cmesh->triangles[t].v[1]].p;
							v[2] = cmesh->vertices[cmesh->triangles[t].v[2]].p;
							
							/*	n = cmesh->normals[cmesh->triangles[t].normal];
							
							  SINGLE angle = acos(dot_product(norm,n));
							  if (angle)
							  {
							  Vector rot = cross_product(norm,n);
							  Quaternion quat(rot,angle);
							  Vector test = quat.transform(norm);
							  i = quat.transform(base_i);
							  j = quat.transform(base_j);
							  }
							  else
							  {
							  i = base_i;
							  j = base_j;
						}*/
							
							SINGLE dist[3],minDist=99999;
							int minVert=4;
							
							for (int c=0;c<3;c++)
							{
								dist[c] = (v[c]-collide_point).magnitude();
								if (dist[c] < minDist)
								{
									minDist = dist[c];
									minVert = c;
								}
							}
							
							//define BBOX_MAX__DIST 800.0
							
							SINGLE BBOX_MAX__DIST = 400+box[BBOX_MAX_Z]*0.15;
							
							n[0] = vertex_normals[cmesh->triangles[t].v[0]];
							n[1] = vertex_normals[cmesh->triangles[t].v[1]];
							n[2] = vertex_normals[cmesh->triangles[t].v[2]];
							
							SINGLE n_dot[3];
							
							n_dot[0] = dot_product(n[0],norm);
							n_dot[1] = dot_product(n[1],norm);
							n_dot[2] = dot_product(n[2],norm);
							
							CQASSERT(minVert < 3);
							
							if (dist[minVert] < BBOX_MAX__DIST*1.4 && (n_dot[0] > 0 && n_dot[1] > 0 && n_dot[2] > 0))
							{
								if (next_poly == POLYS_PER_HOLE)
									next_poly--;
									/*	shieldHitPolys.v[next_poly][minVert] = v[minVert];
									for (c=0;c<3;c++)
									{
									if (minVert != c)
									{
									if (dist[c] < BBOX_MAX__DIST)
									shieldHitPolys.v[next_poly][c] = v[c];
									else
									shieldHitPolys.v[next_poly][c] = v[minVert]+(v[c]-v[minVert])*(1-((dist[c]-BBOX_MAX__DIST)/(dist[c]-dist[minVert])));
									}
							}*/
								
								for (c=0;c<3;c++)
								{
									
									
									SINGLE angle = acos(dot_product(norm,n[c]));
									if (angle)
									{
										Vector rot = cross_product(norm,n[c]);
										Quaternion quat(rot,angle);
										Vector test = quat.transform(norm);
										i = quat.transform(base_i);
										j = quat.transform(base_j);
									}
									else
									{
										i = base_i;
										j = base_j;
									}
									
									shieldHitPolys[slot]->v[next_poly][c] = v[c];
									//	shieldHitPolys.t[next_poly][c].u = 0.5 + dot_product(v[c]-collide_point,i) / BBOX_MAX__DIST;
									//	shieldHitPolys.t[next_poly][c].v = 0.5 + dot_product(v[c]-collide_point,j) / BBOX_MAX__DIST;
									
									Vector offset = v[c]-collide_point;
									Vector offset_2d;
									SINGLE nor_mag;
									nor_mag = dot_product(offset,n[c]);
									offset_2d = offset-nor_mag*n[c];
									offset_2d.normalize();
									SINGLE scale = damage/12.0;
									shieldHitPolys[slot]->t[next_poly][c].u = 0.5 + scale*dist[c]*dot_product(i,offset_2d) / BBOX_MAX__DIST;
									shieldHitPolys[slot]->t[next_poly][c].v = 0.5 + scale*dist[c]*dot_product(j,offset_2d) / BBOX_MAX__DIST;
									
								}
								next_poly++;
							}
						}
						shieldHitPolys[slot]->poly_cnt = next_poly;
						shieldHitPolys[slot]->timer = 1.0;
					}
				}
			}
		}
	}
}

template <class Base>
void ObjectDamage< Base >::GenerateNormals(CollisionMesh *cmesh)
{
	vertex_normals = new Vector[cmesh->num_vertices];
	for (int c=0;c<cmesh->num_vertices;c++)
	{
		Vector norm;
		Triangle * t = cmesh->triangles;
		for (int f=0;f<cmesh->num_triangles;f++)
		{
			if (t[f].v[0] == c || t[f].v[1] == c || t[f].v[2] == c)
			{
				norm += cmesh->normals[t[f].normal];
			}
		}
		norm.normalize();
		vertex_normals[c] = norm;
	}
}
#endif

#if 1
template <class Base>
void ObjectDamage< Base >::GenerateShieldHit(const Vector & pos,const Vector &dir,Vector &collide_point,U32 damage)
{
	S32 slot=-1;
	for (int cc=0;cc<NUM_SHIELD_HITS;cc++)
	{
		if (shieldHitPolys[cc] == 0)
		{
			slot = cc;
			break;
		}
	}

	if (slot != -1)
	{
		shieldHitPolys[slot] = GetShieldHitMap();
		if (shieldHitPolys[slot])
		{
			
			Vector norm;
			//	TRANSFORM scaleTrans;
			//	scaleTrans.d[0][0] = SHIELD_SCALE;
			//	scaleTrans.d[1][1] = SHIELD_SCALE;
			//	scaleTrans.d[2][2] = SHIELD_SCALE;
			
			//TRANSFORM trans = scaleTrans*transform;
			Vector opos = transform.inverse_rotate_translate(pos);
			Vector odir = transform.inverse_rotate(dir);
			opos = opos/SHIELD_SCALE;
			odir.normalize();
			if (GetModelCollisionPosition(collide_point,norm,pos-dir*1000, dir))
			{
				
				//the following two lines are necessary if GetModelCollisionPosition returns world space
				collide_point = transform.inverse_rotate_translate(collide_point);
				norm = transform.inverse_rotate(norm);
				
				Vector base_i,base_j;
				base_i.set(-norm.y,norm.x,0);
				if (base_i.x == 0 && base_i.y ==0)
					base_i.set(0,0,1);
				else
					base_i.normalize();
				
				base_j = cross_product(norm,base_i);
				
				//TEST CODE START
				/*	shieldHitPolys.poly_cnt = 1;
				shieldHitPolys.v[0][0] = collide_point-base_j*400;
				shieldHitPolys.v[0][1] = collide_point+base_j*200-base_i*300;
				shieldHitPolys.v[0][2] = collide_point+base_j*200+base_i*300;
				shieldHitPolys.t[0][0].u = 0.5;
				shieldHitPolys.t[0][0].v = -0.333333;
				shieldHitPolys.t[0][1].u = -0.333333;
				shieldHitPolys.t[0][1].v = 1.0;
				shieldHitPolys.t[0][2].u = 1.333333;
				shieldHitPolys.t[0][2].v = 1.0;
				
				return;*/
				
				//TEST CODE END
				U32 next_poly = 0;
				
				for (int t=0;t<smesh->f_cnt;t++)
				{
					TexCoord tc[3];
					Vector v[3];
					Vector n[3];
					Vector i,j;
					
					v[0] = smesh->v_list[smesh->f_list[t].v[0]].pt;
					v[1] = smesh->v_list[smesh->f_list[t].v[1]].pt;
					v[2] = smesh->v_list[smesh->f_list[t].v[2]].pt;
					
					/*	n = cmesh->normals[cmesh->triangles[t].normal];
					
					  SINGLE angle = acos(dot_product(norm,n));
					  if (angle)
					  {
					  Vector rot = cross_product(norm,n);
					  Quaternion quat(rot,angle);
					  Vector test = quat.transform(norm);
					  i = quat.transform(base_i);
					  j = quat.transform(base_j);
					  }
					  else
					  {
					  i = base_i;
					  j = base_j;
				}*/
					
					SINGLE dist[3],minDist=99999;
					int minVert=4;
					
					int c;
					for (c=0;c<3;c++)
					{
						dist[c] = (v[c]-collide_point).magnitude();
						if (dist[c] < minDist)
						{
							minDist = dist[c];
							minVert = c;
						}
					}
					
					//define BBOX_MAX__DIST 800.0
					
					SINGLE BBOX_MAX__DIST = 400+box[BBOX_MAX_Z]*0.15;
					
					n[0] = smesh->v_list[smesh->f_list[t].v[0]].n;
					n[1] = smesh->v_list[smesh->f_list[t].v[1]].n;
					n[2] = smesh->v_list[smesh->f_list[t].v[2]].n;
					
					SINGLE n_dot[3];
					
					n_dot[0] = dot_product(n[0],norm);
					n_dot[1] = dot_product(n[1],norm);
					n_dot[2] = dot_product(n[2],norm);
					
					CQASSERT(minVert < 3);
					
					if (dist[minVert] < BBOX_MAX__DIST*1.4 && (n_dot[0] > 0 && n_dot[1] > 0 && n_dot[2] > 0))
					{
						if (next_poly == POLYS_PER_HIT)
							next_poly--;
							/*	shieldHitPolys.v[next_poly][minVert] = v[minVert];
							for (c=0;c<3;c++)
							{
							if (minVert != c)
							{
							if (dist[c] < BBOX_MAX__DIST)
							shieldHitPolys.v[next_poly][c] = v[c];
							else
							shieldHitPolys.v[next_poly][c] = v[minVert]+(v[c]-v[minVert])*(1-((dist[c]-BBOX_MAX__DIST)/(dist[c]-dist[minVert])));
							}
					}*/
						
						for (c=0;c<3;c++)
						{
							
							
							SINGLE angle = acos(dot_product(norm,n[c]));
							if (angle)
							{
								Vector rot = cross_product(norm,n[c]);
								Quaternion quat(rot,angle);
								Vector test = quat.transform(norm);
								i = quat.transform(base_i);
								j = quat.transform(base_j);
							}
							else
							{
								i = base_i;
								j = base_j;
							}
							
							shieldHitPolys[slot]->v[next_poly][c] = v[c];
							//	shieldHitPolys.t[next_poly][c].u = 0.5 + dot_product(v[c]-collide_point,i) / BBOX_MAX__DIST;
							//	shieldHitPolys.t[next_poly][c].v = 0.5 + dot_product(v[c]-collide_point,j) / BBOX_MAX__DIST;
							
							Vector offset = v[c]-collide_point;
							Vector offset_2d;
							SINGLE nor_mag;
							nor_mag = dot_product(offset,n[c]);
							offset_2d = offset-nor_mag*n[c];
							offset_2d.normalize();
							SINGLE scale = 0.2+0.9*damage/12.0;
							shieldHitPolys[slot]->t[next_poly][c].u = 0.5 + dist[c]*dot_product(i,offset_2d) / (400*scale);
							shieldHitPolys[slot]->t[next_poly][c].v = 0.5 + dist[c]*dot_product(j,offset_2d) / (400*scale);
							
						}
						next_poly++;
					}
				}
				shieldHitPolys[slot]->poly_cnt = next_poly;
				shieldHitPolys[slot]->timer = 1.0;
				Vector cp_global = transform*collide_point;
				if (shieldSound[slot]==0)
					shieldSound[slot] = SFXMANAGER->Open(shieldSoundID);
				SFXMANAGER->Play(shieldSound[slot], systemID, &cp_global);
			}
		}
	}
}
#endif
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::renderShieldHits()
{
	if (numShieldHits > 0)
	{
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);

	Vector v[3];
	TexCoord t[3];
	
	int tech = MGlobals::GetUpgradeLevel(playerID,UG_SHIELDS,race);


	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x*(1+0.1*tech);
	scaleTrans.d[1][1] = shieldScale.y*(1+0.1*tech);
	scaleTrans.d[2][2] = shieldScale.z*(1+0.1*tech);

/*	BATCH->set_texture_stage_texture( 0, shieldTexID );
	BATCH->set_texture_stage_texture( 1, shieldTexID );

	// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	// addressing - clamped
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );

	BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
	// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
	BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );*/

	// addressing - clamped
//	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
//	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);

	TRANSFORM trans = transform*scaleTrans;
//	trans.rotate_about_j(PI);
	CAMERA->SetModelView(&trans);
	SetupDiffuseBlend(shieldAnimArch->frames[0].texture,TRUE);
	if (CQFLAGS.bCQBatcher)
		BATCH->set_state(RPR_DELAY,1);
	//BATCH->set_state(RPR_STATE_ID,shieldAnimArch->frames[0].texture);
	//	DisableTextures();
	
	for (int s=0;s<NUM_SHIELD_HITS;s++)
	{
		if (shieldHitPolys[s] && shieldHitPolys[s]->timer)
		{
			SINGLE s_timer = shieldHitPolys[s]->timer;
			
			AnimFrame *frame = &shieldAnimArch->frames[int((1-s_timer)*shieldAnimArch->frame_cnt)];
			
			PB.Color4ub(255,255,255,255);//s_timer*255);
			
			
			for (int f=0;f<shieldHitPolys[s]->poly_cnt;f++)
			{
				v[0] = shieldHitPolys[s]->v[f][0];
				v[1] = shieldHitPolys[s]->v[f][1];
				v[2] = shieldHitPolys[s]->v[f][2];
				
				t[0] = shieldHitPolys[s]->t[f][0];
				t[1] = shieldHitPolys[s]->t[f][1];
				t[2] = shieldHitPolys[s]->t[f][2];
				
				/*	t[0].u = 0.5+(t[0].u-0.5)/(1.2-s_timer);
				t[0].v = 0.5+(t[0].v-0.5)/(1.2-s_timer);
				t[1].u = 0.5+(t[1].u-0.5)/(1.2-s_timer);
				t[1].v = 0.5+(t[1].v-0.5)/(1.2-s_timer);
				t[2].u = 0.5+(t[2].u-0.5)/(1.2-s_timer);
				t[2].v = 0.5+(t[2].v-0.5)/(1.2-s_timer);*/
				
				t[0].u = frame->x0+(0.5+(t[0].u-0.5))*(frame->x1-frame->x0);
				t[0].v = frame->y0+(0.5+(t[0].v-0.5))*(frame->y1-frame->y0);
				t[1].u = frame->x0+(0.5+(t[1].u-0.5))*(frame->x1-frame->x0);
				t[1].v = frame->y0+(0.5+(t[1].v-0.5))*(frame->y1-frame->y0);
				t[2].u = frame->x0+(0.5+(t[2].u-0.5))*(frame->x1-frame->x0);
				t[2].v = frame->y0+(0.5+(t[2].v-0.5))*(frame->y1-frame->y0);
				
				if ((t[0].u > 0 || t[1].u > 0 || t[2].u > 0) && (t[0].v > 0 || t[1].v > 0 || t[2].v > 0) &&
					(t[0].u < 1 || t[1].u < 1 || t[2].u < 1) && (t[0].v < 1 || t[1].v < 1 || t[2].v < 1))
				{
					
					//	COLORREF color = polyColor[f];
					//	PB.Color4ub(GetRValue(color),GetGValue(color),GetBValue(color),255);
					
					//			SINGLE a = 0.3*cos(timer*0.1);
					//			SINGLE b = 0.3*sin(timer*0.1);
					
					BATCH->set_texture_stage_texture(0,frame->texture);
					BATCH->set_state(RPR_STATE_ID,frame->texture);
					PB.Begin(PB_TRIANGLES);
					//	PB.TexCoord2f(a,b);
					//	PB.MulCoord2f(t[0].u,t[0].v);
					PB.TexCoord2f(t[0].u,t[0].v);
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					//	PB.TexCoord2f(a,b+1);
					//	PB.MulCoord2f(t[1].u,t[1].v);
					PB.TexCoord2f(t[1].u,t[1].v);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					//	PB.TexCoord2f(a+1,b+1);
					//	PB.MulCoord2f(t[2].u,t[2].v);
					PB.TexCoord2f(t[2].u,t[2].v);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					PB.End();
				}
			}
		}
	}
	BATCH->set_state(RPR_STATE_ID,0);
	if (CQFLAGS.bCQBatcher)
		BATCH->set_state(RPR_DELAY,0);
	}

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::renderShield()
{
	int tech = MGlobals::GetUpgradeLevel(playerID,UG_SHIELDS,race);

	Vector v[3];
	
	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x*(1+0.1*tech);
	scaleTrans.d[1][1] = shieldScale.y*(1+0.1*tech);
	scaleTrans.d[2][2] = shieldScale.z*(1+0.1*tech);

	DisableTextures();

	TRANSFORM trans = transform*scaleTrans;
	//trans.rotate_about_j(PI);
	CAMERA->SetModelView(&trans);

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	PB.Color4ub(255,255,255,100);

	BATCH->set_state(RPR_STATE_ID,2);
	PB.Begin(PB_TRIANGLES);
	for (int f=0;f<smesh->f_cnt;f++)
	{
		PB.Color4ub((f*12341234)%255,(f*233432)%255,(f*7777777)%255,100);

		v[0] = smesh->v_list[smesh->f_list[f].v[0]].pt;
		v[1] = smesh->v_list[smesh->f_list[f].v[1]].pt;
		v[2] = smesh->v_list[smesh->f_list[f].v[2]].pt;

		PB.Vertex3f(v[0].x,v[0].y,v[0].z);
		PB.Vertex3f(v[1].x,v[1].y,v[1].z);
		PB.Vertex3f(v[2].x,v[2].y,v[2].z);
	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::renderShieldDown()
{
	int tech = MGlobals::GetUpgradeLevel(playerID,UG_SHIELDS,race);

	Vector v[3];
	TexCoord t[3];
	
	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x*(1+0.1*tech);
	scaleTrans.d[1][1] = shieldScale.y*(1+0.1*tech);
	scaleTrans.d[2][2] = shieldScale.z*(1+0.1*tech);

	TRANSFORM trans = transform*scaleTrans;
	//trans.rotate_about_j(PI);

	BATCH->set_state(RPR_BATCH,TRUE);
	CAMERA->SetModelView(&trans);

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);//SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
	PB.Color4ub(255,255,255,255);

	AnimFrame *frame = &shieldFizzAnimArch->frames[int((1-shieldDownTimer)*shieldFizzAnimArch->frame_cnt)];
	BATCH->set_state(RPR_STATE_ID,frame->texture);
	SetupDiffuseBlend(frame->texture,TRUE);
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	PB.Begin(PB_TRIANGLES);
	for (int f=0;f<smesh->f_cnt;f++)
	{
		v[0] = smesh->v_list[smesh->f_list[f].v[0]].pt;
		v[1] = smesh->v_list[smesh->f_list[f].v[1]].pt;
		v[2] = smesh->v_list[smesh->f_list[f].v[2]].pt;

		t[0].u = v[0].z*0.0006+shieldDownTimer*4-1.0;
		t[0].v = atan2(v[0].x,v[0].y)/PI+0.5;
		t[1].u = v[1].z*0.0006+shieldDownTimer*4-1.0;
		t[1].v = atan2(v[1].x,v[1].y)/PI+0.5;
		t[2].u = v[2].z*0.0006+shieldDownTimer*4-1.0;
		t[2].v = atan2(v[2].x,v[2].y)/PI+0.5;

		if (t[0].v-t[1].v > 2)
			t[1].v += 2*PI;
		if (t[0].v-t[1].v < -2)
			t[1].v -= 2*PI;
		if (t[0].v-t[2].v > 2)
			t[2].v += 2*PI;
		if (t[0].v-t[2].v < -2)
			t[2].v -= 2*PI;
		
		
		bool bIsGood = true;
		if (t[0].u > 1)
		{
			if (t[1].u > 1 && t[2].u > 1)
				bIsGood = false;
		}
		else if (t[0].u < 0)
		{
			if (t[1].u < 0 && t[2].u < 0)
				bIsGood = false;
		}
		if (bIsGood)
		{
			PB.TexCoord2f(t[0].u,t[0].v);
			PB.Vertex3f(v[0].x,v[0].y,v[0].z);
			PB.TexCoord2f(t[1].u,t[1].v);
			PB.Vertex3f(v[1].x,v[1].y,v[1].z);
			PB.TexCoord2f(t[2].u,t[2].v);
			PB.Vertex3f(v[2].x,v[2].y,v[2].z);
		}
	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::renderShieldUp(int texID)
{	
	int tech = MGlobals::GetUpgradeLevel(playerID,UG_SHIELDS,race);

	Vector v[3];
	TexCoord t[3];
	
	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x*(1+0.1*tech);
	scaleTrans.d[1][1] = shieldScale.y*(1+0.1*tech);
	scaleTrans.d[2][2] = shieldScale.z*(1+0.1*tech);

	TRANSFORM trans = transform*scaleTrans;
	//trans.rotate_about_j(PI);

	BATCH->set_state(RPR_BATCH,TRUE);
	CAMERA->SetModelView(&trans);

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);//SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);//INVSRCALPHA);
	PB.Color4ub(255,255,255,255);

//	AnimFrame *frame = &shieldFizzAnimArch->frames[int((1-shieldDownTimer)*shieldFizzAnimArch->frame_cnt)];
	BATCH->set_state(RPR_STATE_ID,texID);
	SetupDiffuseBlend(texID,TRUE);
	BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	PB.Begin(PB_TRIANGLES);
	for (int f=0;f<smesh->f_cnt;f++)
	{
		v[0] = smesh->v_list[smesh->f_list[f].v[0]].pt;
		v[1] = smesh->v_list[smesh->f_list[f].v[1]].pt;
		v[2] = smesh->v_list[smesh->f_list[f].v[2]].pt;

		t[0].u = v[0].z*0.0003+0.5;
		t[0].v = atan2(v[0].x,v[0].y)/PI+0.5;
		t[1].u = v[1].z*0.0003+0.5;
		t[1].v = atan2(v[1].x,v[1].y)/PI+0.5;
		t[2].u = v[2].z*0.0003+0.5;
		t[2].v = atan2(v[2].x,v[2].y)/PI+0.5;

		if (t[0].v-t[1].v > 2)
			t[1].v += 2*PI;
		if (t[0].v-t[1].v < -2)
			t[1].v -= 2*PI;
		if (t[0].v-t[2].v > 2)
			t[2].v += 2*PI;
		if (t[0].v-t[2].v < -2)
			t[2].v -= 2*PI;
		
		
	/*	bool bIsGood = true;
		if (t[0].u > 1)
		{
			if (t[1].u > 1 && t[2].u > 1)
				bIsGood = false;
		}
		else if (t[0].u < 0)
		{
			if (t[1].u < 0 && t[2].u < 0)
				bIsGood = false;
		}
		if (bIsGood)
		{*/
			PB.TexCoord2f(t[0].u,t[0].v);
			PB.Vertex3f(v[0].x,v[0].y,v[0].z);
			PB.TexCoord2f(t[1].u,t[1].v);
			PB.Vertex3f(v[1].x,v[1].y,v[1].z);
			PB.TexCoord2f(t[2].u,t[2].v);
			PB.Vertex3f(v[2].x,v[2].y,v[2].z);
		//}
	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::getScale()
{
	shieldScale.x = 1/(box[BBOX_MAX_X]-box[BBOX_MIN_X]);
	shieldScale.y = 1/(box[BBOX_MAX_Y]-box[BBOX_MIN_Y]);
	shieldScale.z = 1/(box[BBOX_MAX_Z]-box[BBOX_MIN_Z]);

	shieldScale.normalize();

//	shieldScale *= SHIELD_SCALE;
	shieldScale.x = 1+(shieldScale.x*0.5);
	shieldScale.y = 1+(shieldScale.y*0.5);
	shieldScale.z = 1+(shieldScale.z*0.5);
}

//---------------------------------------------------------------------------
//
/*template <class Base>
void ObjectDamage< Base >::DamageFace (S32 childID,S32 faceID)
{
	mc[childID].hiddenArray[faceID] = FS_DAMAGED;
//	MyFace *face = &mc[childID].myFaceArray[faceID];
//	FACE_PROPERTY *prop = &mc[childID].buildMesh->face_groups[face->groupID].face_properties[face->index];
//	*prop |= HIDDEN;
}*/
void Dummy()
{
};

#define DMG_TEXFACTOR 0.0012
#define BLAST_SIZE 500.0

template <class Base>
void ObjectDamage< Base >::DamageSpot (U32 spot)
{
/*
	U32 nextFace=0;

//	Mesh *m = mc[0]->buildMesh;
	SINGLE blastRadius = sizeFactor*BLAST_SIZE;
	SINGLE texFactor = 0.5/blastRadius;

	Vector c = damageSave[spot].pos;
	TexCoord temp_tc[240];
	U16 temp_tc_refs[240];
	U16 temp_refs[240];
	U16 temp_face_refs[81];
	U16 tc_cnt=0,vert_cnt=0;
	U16 tc_re_index[240];
//	U16 re_index_list[240];

	//memset(tc_re_index,0xff,sizeof(U16)*1024);
	//memset(re_index_list,0xff,sizeof(U16)*240);
	Vector *pos_list = mc.mi[0]->mr->pos_list;

	U16 pos_offset=0;
	U16 face_offset=0;

	IRenderMaterial *irm=0;
	
	while((irm = mc.mi[0]->mr->GetNextFaceGroup(irm)) != 0)
	{
		
		U16 *index_list;
		void *src_verts;
		Vector *src_norms;
		ArchetypeFaceInfo *faces;
		S32 svert_cnt;
		S32 face_cnt;
		irm->GetBuffers(&src_verts,&index_list,&src_norms,&faces,&svert_cnt,&face_cnt);
		U8 *faceRenders = &mc.mi[0]->faceRenders[irm->face_offset];
		
		if (!(mc.mi[0]->fgi[irm->fg_idx].texture_flags & TF_F_HAS_ALPHA))
		{
			int i=0;
			while (i<face_cnt && nextFace < 80)
			{
				Vector v[3];
				v[0]=pos_list[pos_offset+index_list[i*3]];
				v[1]=pos_list[pos_offset+index_list[i*3+1]];
				v[2]=pos_list[pos_offset+index_list[i*3+2]];
				
				CQASSERT(i != 0 || v[0].y == ((Vector *)src_verts)[0].y);
				//v[0]=m->object_vertex_list[m->vertex_batch_list[m->face_groups[mc[0]->myFaceArray[i].groupID].face_vertex_chain[mc[0]->myFaceArray[i].index*3]]];
				//v[1]=m->object_vertex_list[m->vertex_batch_list[m->face_groups[mc[0]->myFaceArray[i].groupID].face_vertex_chain[mc[0]->myFaceArray[i].index*3+1]]];
				//v[2]=m->object_vertex_list[m->vertex_batch_list[m->face_groups[mc[0]->myFaceArray[i].groupID].face_vertex_chain[mc[0]->myFaceArray[i].index*3+2]]];
				
				Vector n = faces[i].norm;// = m->normal_ABC[m->face_groups[mc[0]->myFaceArray[i].groupID].face_normal[mc[0]->myFaceArray[i].index]];
				
				TexCoord tx[3];
				
				for (int vv=0;vv<3;vv++)
				{
					v[vv] -= c;
				}
				
				if ((faceRenders[i] & FS__HIDDEN) == 0)
				{
					if (SphereTriangle(v,Vector(0,0,0),blastRadius))
					{
						int tc_set;  //this variable will allow me to reindex optimally
						if (fabs(n.x) > fabs(n.y))
						{
							if (fabs(n.z) > fabs(n.x))
							{
								tc_set = 0;
								for (vv=0;vv<3;vv++)
								{
									tx[vv].u = v[vv].x*texFactor+0.5;
									tx[vv].v = v[vv].y*texFactor+0.5;
								}
							}
							else
							{
								tc_set = 1;
								for (vv=0;vv<3;vv++)
								{
									tx[vv].u = v[vv].z*texFactor+0.5;
									tx[vv].v = v[vv].y*texFactor+0.5;
								}
							}
						}
						else 
						{
							if (fabs(n.z) > fabs(n.y))
							{
								tc_set = 0;
								for (vv=0;vv<3;vv++)
								{
									tx[vv].u = v[vv].x*texFactor+0.5;
									tx[vv].v = v[vv].y*texFactor+0.5;
								}
							}
							else
							{
								tc_set = 2;
								for (vv=0;vv<3;vv++)
								{
									tx[vv].u = v[vv].z*texFactor+0.5;
									tx[vv].v = -v[vv].x*texFactor+0.5;
								}
							}
						}
						
						
						//damageRecord[spot].SetPolyCnt(nextFace+1);
						//damageRecord[spot].polyList[nextFace] = i;
						
						for (vv=0;vv<3;vv++)
						{
							//	damageRecord[spot].texCoord[nextFace*3+vv].u = tx[vv].u;
							//	damageRecord[spot].texCoord[nextFace*3+vv].v = tx[vv].v;
							U16 src_ref = pos_offset+index_list[i*3+vv];
							//this is a slow method but for LOW output poly counts should be ok
							U16 found_ref = 0xffff;
							for (int i=0;i<tc_cnt;i++)
							{
								if (tc_re_index[i] == (tc_set<<14)+src_ref)
								{
									found_ref = i;
									break;
								}
							}
							
							if (found_ref == 0xffff)
							{
								temp_tc[tc_cnt].u = tx[vv].u;
								temp_tc[tc_cnt].v = tx[vv].v;
								tc_re_index[tc_cnt] = (tc_set<<14)+src_ref;
								found_ref = tc_cnt;
								tc_cnt++;
							}
							temp_tc_refs[nextFace*3+vv] = found_ref;
							CQASSERT(tx[vv].u == temp_tc[found_ref].u);
						temp_refs[nextFace*3+vv] = src_ref;
						}
						temp_face_refs[nextFace] = face_offset+i;
						
						nextFace++;
					}
				}
				
				i++;
			}
		}
		pos_offset += svert_cnt;
		face_offset += face_cnt;
	}

	Dummy();

	if (nextFace)
	{
		IEffectChannel *ec = mc.mi[0]->GetNewEffectChannel();
		ec->idx_list = new U16[nextFace*3];
		memcpy(ec->idx_list,temp_refs,sizeof(U16)*nextFace*3);
		ec->idx_cnt = nextFace*3;
		ec->tc = new TexCoord[tc_cnt];
		memcpy(ec->tc,temp_tc,sizeof(TexCoord)*tc_cnt);
		ec->tc_cnt = tc_cnt;
		ec->tc_idx_list = new U16[nextFace*3];
		memcpy(ec->tc_idx_list,temp_tc_refs,sizeof(U16)*nextFace*3);
		ec->vert_cnt = vert_cnt;
		ec->face_ref_list = new U16[nextFace+1];
		//make sure the "last" face has a really huge index for splitting
		temp_face_refs[nextFace] = 0xffff;
		memcpy(ec->face_ref_list,temp_face_refs,sizeof(U16)*(nextFace+1));
		DamageRender *new_dr = new DamageRender;
		ec->irc = new_dr;
		new_dr->damageTexID = damageTexID;
		new_dr->ec = ec;

		damageRecord[spot].ec = ec;
		damageSave[spot].bActive = true;
		numDamages++;
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectDamage< Base >::RegisterDamage(Vector pos,U32 amount)
{
	if (bExploding)
		return;

	S32 newRecord=-1;
	S32 appliedSlot=-1;
	bool bApplied = 0;
	Vector pos2 = pos;//transform.inverse_rotate_translate(pos);

	if (extentData->bX)
	{
		if (pos2.x < box[BBOX_MIN_X])
			pos2.x = box[BBOX_MIN_X];
	
		if (pos2.x > box[BBOX_MAX_X])
			pos2.x = box[BBOX_MAX_X];
	}
	else
	{
		if (pos2.y < box[BBOX_MIN_Y])
			pos2.y = box[BBOX_MIN_Y];
	
		if (pos2.y > box[BBOX_MAX_Y])
			pos2.y = box[BBOX_MAX_Y];
	}

	SINGLE dist[DAMAGE_RECORDS];
	for (int c=0;c<DAMAGE_RECORDS;c++)
	{
		if (damageSave[c].damage)
		{
			if ((dist[c] = (damageSave[c].pos-pos2).magnitude()) < 400*sizeFactor)
			{
				damageSave[c].damage += amount;
				newRecord = -1;

				appliedSlot = c;

				bApplied = 1;

			//	if (damageSave[c].damage > 15 && damageSave[c].bActive == 0)
			//		DamageSpot(c);
				break;
			}
		}
		else
		{
			newRecord = c;
		}
	}

	if (newRecord != -1)
	{
		damageSave[newRecord].pos = pos2;
		damageSave[newRecord].damage = amount;

		appliedSlot = newRecord;

		bApplied = 1;

	//	if (amount > 15 && damageRecord[newRecord].bActive == 0)
		//	DamageSpot(newRecord);
	}
	else if (bApplied == 0)
	{
		SINGLE leastDist=1e9;
		S32 leastDistSlot=-1;
		for (int c=0;c<DAMAGE_RECORDS;c++)
		{
			if (dist[c] < leastDist)
			{
				leastDist = dist[c];
				leastDistSlot = c;
			}
		}
		
		damageSave[leastDistSlot].damage += amount;
		newRecord = -1;
		
		appliedSlot = leastDistSlot;
	}

	if (appliedSlot != -1)
	{
		SINGLE hull;
		if (hullPointsMax)
			hull = (SINGLE)hullPoints / (SINGLE)hullPointsMax;
		else 
			hull = 1.0F;
		
		if (hull <= lastDamage - 0.12 && hull <= 0.60)
		{
			if (damageSave[appliedSlot].bActive == 0)
			{
				DamageSpot(appliedSlot);
				lastDamage -= 0.12;
				
				//Create blast effect for creation of damage
				IBaseObject *obj;
				if ((obj = ARCHLIST->CreateInstance(pDamageBlast)) !=0)
				{
					OBJPTR<IBlast> blast;
					if (obj->QueryInterface(IBlastID,blast))
					{
						blast->InitBlast(Transform(pos2), systemID,this);
					}
					IBaseObject *blastPos = childBlastList;
					if (!blastPos)
					{
						childBlastList = obj;
					}
					else
					{
						while (blastPos->next)
						{
							blastPos = blastPos->next;
						}
						blastPos->next = obj;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
bool SphereTriangle  (Vector v[3],Vector pt,SINGLE rad)
{
	bool result;
	
	float r_squared = (rad*rad);
	
	float min = FLT_MAX;
//	int num_tri_cols = 0;
	
	// Sphere is in mesh frame. Compute distance from sphere center to each triangle in mesh.
	
	const Vector & b0 = v[0];
	const Vector & v1 = v[1];
	const Vector & v2 = v[2];
	
	Vector e0 = v1 - b0;
	Vector e1 = v2 - b0;
	
	Vector diff = b0 - pt;
	
	float a = dot_product(e0, e0);
	float b = dot_product(e0, e1);
	float c = dot_product(e1, e1);
	float d = dot_product(e0, diff);
	float e = dot_product(e1, diff);
//	float f = dot_product(diff, diff);

	// This determinant will never be zero for a non-degenerate triangle.
	float det = a * c - b * b;
	
	float s = (b * e - c * d) / det;
	float t = (b * d - a * e) / det;
	
	if (s+t <= 1.0f)
	{
		if (s < 0.0f)
		{
			if (t < 0.0f)  // region 4
			{
				if (d < 0)
				{
					t = 0.0f;
					s = -d / a;
					if (s > 1.0f) 
					{
						s = 1.0f;
					}
				}
				else if (e < 0)
				{
					s = 0.0f;
					t = - e / c;
					if (t > 1.0f) 
					{
						t = 1.0f;
					}
				}
				else
				{
					s = 0.0f;
					t = 0.0f;
				}
			}
			else  // region 3
			{
				s = 0.0f;
				t = - e / c;
				if (t < 0.0f)
				{
					t = 0.0f;
				}
				else if (t > 1.0f)
				{
					t = 1.0f;
				}
			}
		}
		else if (t < 0.0f)  // region 5
		{
			t = 0.0f;
			s = -d / a;
			if (s < 0.0f)
			{
				s = 0.0f;
			}
			else if (s > 1.0f)
			{
				s = 1.0f;
			}
		}
		else  // region 0
		{
			// minimum at interior point
		}
	}
	else
	{
		if (s < 0.0f)  // region 2
		{
			if (b - c + d - e < 0.0f)
			{
				s = -(b - c + d - e) / (a - 2 * b + c);
				if (s < 0.0f) 
				{
					s = 0.0f; 
				}
				else if (s > 1.0f)
				{
					s = 1.0f;
				}
				t = 1.0f - s;
			}
			else if (c + e > 0.0f)
			{
				s = 0.0f;
				t = - e / c;
				if (t < 0.0f)
				{
					t = 0.0f;
				}
				else if (t > 1.0f)
				{
					t = 1.0f;
				}
			}
			else
			{
				s = 0.0f;
				t = 1.0f;
			}
		}
		else if (t < 0.0f)  // region 6
		{
			if (a - b + d - e > 0.0f)
			{
				t = (a - b + d - e) / (a - 2 * b + c);
				if (t < 0.0f) 
				{
					t = 0.0f;
				}
				else if (t > 1.0f)
				{
					t = 1.0f;
				}
				s = 1.0f - t;
			}
			else if (a + d > 0.0f)
			{
				t = 0.0f;
				s = -d / a;
				if (s < 0.0f) 
				{
					s = 0.0f;
				}
				else if (s > 1.0f)
				{
					s = 1.0f;
				}
			}
			else
			{
				s = 1.0f;
				t = 0.0f;
			}
		}
		else  // region 1
		{
			s = -(b - c + d - e) / (a - 2 * b + c);
			if (s < 0.0f )
			{
				s = 0.0f; 
			}
			else if (s > 1.0f) 
			{
				s = 1.0f;
			}
			t = 1.0f - s;
		}
	}

	
	// Alternate computation of min distance.
	//float dist_squared = fabs(s * (a * s + b * t + 2 * d) + t * (b * s + c * t + 2 * e) + f);
	
	Vector closest = b0 + s * e0 + t * e1;
	Vector dc = closest - pt;
	float dc_squared = dot_product(dc, dc);
	/*float dd = fabs(dc_squared - min);

	if (dd <= 1e-3)
	{
		tricol[num_tri_cols].tri1 = tri;
		tricol[num_tri_cols].pt = closest;
		num_tri_cols++;
	}
	else if (dc_squared < min)
	{*/
		min = dc_squared;
	/*	tricol[0].tri1 = tri;
		tricol[0].pt = closest;
		num_tri_cols = 1;
	}*/

	result = (min <= r_squared);

	return result;
}

struct DamageRender : IRenderChannel
{
	SINGLE dmgTimer;
	U32 damageTexID;

	DamageRender();

	virtual void Render(SINGLE dt);

	virtual IRenderChannel *Clone();
};

DamageRender::DamageRender()
{
	dmgTimer = 0;
}

void DamageRender::Render(SINGLE dt)
{
	dmgTimer += dt;
	if (dmgTimer > 314.16f)
		dmgTimer -= 314.16f;

	BATCH->set_state(RPR_BATCH,TRUE);
	const Transform &trans = ENGINE->get_transform(ec->mi->instanceIndex);
	CAMERA->SetModelView(&trans);
	
	int val = 255*(0.7+0.3*cos(2*dmgTimer));
	
	val += 30;
	if (val > 510)
		val -= 510;
	S32 mag=val;
	if (mag > 255)
		mag = 510-mag;
	CQASSERT( mag >= 0 && mag <= 255);
	U32 color = 0xff000000 | (mag<<16) | (mag<<8) | mag;
	BATCH->set_state(RPR_DELAY,1);
	BATCH->set_render_state(D3DRS_DEPTHBIAS ,1);
	ec->RenderWithTexture(damageTexID,color,true); //true for clamp
	BATCH->set_render_state(D3DRS_DEPTHBIAS ,0);
	BATCH->set_state(RPR_DELAY,0);
}

IRenderChannel * DamageRender::Clone()
{
	DamageRender *new_dr = new DamageRender;
	new_dr->damageTexID = damageTexID;
	new_dr->dmgTimer = dmgTimer;

	return new_dr;
}
//---------------------------------------------------------------------------
//-------------------------End Damage.cpp------------------------------------
//---------------------------------------------------------------------------
