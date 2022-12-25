#ifndef TOBJEXTENT_H
#define TOBJEXTENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjCloak.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjExtent.h 44    10/20/00 9:09p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include <Mesh.h>
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef IEXPLOSION_H
#include "IExplosion.h"
#endif

#ifndef SECTOR_H
#include "Sector.h"
#endif

#ifndef MESHRENDER_H
#include "MeshRender.h"
#endif

#ifndef CQEXTENT_H
#include "CQExtent.h"
#endif

#ifndef IWEAPON_H
#include "IWeapon.h"
#endif

#ifndef OBJMAP_H
#include "ObjMap.h"
#endif

#ifndef DSPACESHIP_H
#include <DSpaceship.h>
#endif
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//





#define ObjectExtent _Coe

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectExtent : public Base, IWeaponTarget, IExplosionOwner, IExtent
{
/*	RECT extents[SLICES];
	SINGLE _step;
	SINGLE _min;
	SINGLE min_slice,max_slice;
	BOOL32 bZ;*/
	const struct EXTENT_DATA *extentData;

	//S32 numChildren;

	typename typedef Base::INITINFO EXTENTINITINFO;
	struct InitNode			initNode;
	struct PostRenderNode	postRenderNode;

	//child blasts
	IBaseObject *childBlastList;
	
	ObjectExtent (void) : initNode(this, InitProc(&ObjectExtent::initExtents)),
		postRenderNode(this,RenderProc(&ObjectExtent::postRenderExtent))
	{
	}

	~ObjectExtent();

	void initExtents (const EXTENTINITINFO & data);

	virtual BOOL32 GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual BOOL32 GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);


	//IExtent

	virtual void AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir);

	virtual void AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans);

	typedef const RECT * cr;

	virtual void GetExtentInfo (cr &extents,SINGLE *_step,SINGLE *_min,U8 *slices);

	virtual void SetAliasData (U32 _aliasArch,U8 aliasPlayer)
	{
		aliasArchetypeID = _aliasArch;
		aliasPlayerID = aliasPlayer;
		if (_aliasArch != -1)
		{
			if (objMapNode)
				objMapNode->flags |= OM_MIMIC;
//			mc->mi[0]->GetBoundingBox(box);
			ComputeCorners(box, instanceIndex);
		}
		else
		{
			ComputeCorners(box, instanceIndex);
//			mc.mi[0]->GetBoundingBox(box);
			if (objMapNode)
				objMapNode->flags &= ~OM_MIMIC;
		}
		boxRadius = __max(box[0], -box[1]);
		boxRadius = __max(boxRadius, box[2]);
		boxRadius = __max(boxRadius, -box[3]);
	}

	virtual void GetAliasData (U32 & _aliasArch,U8 &aliasPlayer)
	{
		_aliasArch = aliasArchetypeID;
		aliasPlayer = aliasPlayerID;
	}

	virtual void CalculateRect (bool bEnableMimic)
	{
		if (bEnableMimic && (aliasArchetypeID != -1))
		{
			ComputeCorners(box, instanceIndex);
		}
		else
			ComputeCorners(box, instanceIndex);
		
		boxRadius = __max(box[0], -box[1]);
		boxRadius = __max(boxRadius, box[2]);
		boxRadius = __max(boxRadius, -box[3]);
	}

	virtual const RECT *GetExtentRect (SINGLE _val);

	virtual BOOL32 IsX () {return extentData->bX;}

/*	virtual void ListChildren (INSTANCE_INDEX parentIdx);

	virtual void listChildren (INSTANCE_INDEX parentIdx,IMeshInfo *parent);*/

//	virtual U32 GetNumChildren ();

	void postRenderExtent();

	void AddChildBlast (IBaseObject *blast);
};



inline void ExtentDummy()
{
	U32 dummy=0;
	dummy++;
}

template <class Base>
ObjectExtent< Base >::~ObjectExtent()
{
	IBaseObject *pos = childBlastList;
	while (childBlastList)
	{
		childBlastList = pos->next;
		//delete pos->obj;
		delete pos;
		pos = childBlastList;
	}

	ExtentDummy();

}
//----------------------------------------------------------------------------------
//
template <class Base>
void ObjectExtent< Base >::initExtents (const EXTENTINITINFO & data)
{
	EXTENT_DATA *extentDataRW;
	extentDataRW = (EXTENT_DATA *)&data.extent;
	extentData = extentDataRW;

	if (extentData->_step == 0.0f)
	{
	//	SINGLE box[6];
		SINGLE dy,dx;
		S32 i;
		bool hit[SLICES];
		SINGLE weight[SLICES];
		for (i=0;i<SLICES;i++)
		{
			weight[i] =0;
		}
		
		HARCH archIndex = instanceIndex;
		Mesh * mesh = REND->get_archetype_mesh(archIndex);
		
		for (i=0;i<SLICES;i++)
		{
			hit[i] =0;
		}
		
		memset(extentDataRW->extents,0,sizeof(RECT)*SLICES);
		
		// Rob said to do this.... -sbarton
		SINGLE local_box[6];

		REND->get_archetype_bounding_box(archIndex,LODPERCENT, local_box);
		dy = local_box[BBOX_MAX_Y]-local_box[BBOX_MIN_Y]+1;
		dx = local_box[BBOX_MAX_X]-local_box[BBOX_MIN_X]+1;
		
		if (dx > dy)
		{
			extentDataRW->_step = dx/SLICES;
			extentDataRW->_min = local_box[BBOX_MIN_X];
			extentDataRW->bX = TRUE;
		}
		else
		{
			extentDataRW->_step = dy/SLICES;
			extentDataRW->_min = local_box[BBOX_MIN_Y];
			extentDataRW->bX = FALSE;
		}
		
		FaceGroup *fg = mesh->face_groups;
		for (int g=0;g<mesh->face_group_cnt;g++,fg++)
		{
			int *n = fg->face_normal;
			for (int f=0;f<fg->face_cnt;f++,n++)
			{
				//I don't know what this check does - seems to work fine without it
				//	if (fabs(mesh->normal_ABC[*n].y) > 0.6)
				//	{
				Vector v0 = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f]]];
				Vector v1 = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f+1]]];
				Vector v2 = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f+2]]];
				
				Vector a=v0-v1;
				Vector b=v1-v2;
				Vector x=cross_product(a,b);
				SINGLE tri_weight = x.magnitude();
				S8 low= 100,high = -4;
				Vector v[3];
				v[0] = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f]]];
				v[1] = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f+1]]];
				v[2] = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f+2]]];
				
				//check all the edges to see which extent slices they touch
				for (i=0;i<3;i++)
				{
					Vector vA,vB;
					vA = v[i];
					vB = v[(i+1)%3];
					int sliceA,sliceB;
					if (extentDataRW->bX)
					{
						sliceA=floor(SLICES*(vA.x-local_box[BBOX_MIN_X])/dx);
						sliceB=floor(SLICES*(vB.x-local_box[BBOX_MIN_X])/dx);
						
						CQASSERT(sliceA>=0 && sliceA<SLICES);
					}
					else
					{
						sliceA=floor(SLICES*(vA.y-local_box[BBOX_MIN_Y])/dy);
						sliceB=floor(SLICES*(vB.y-local_box[BBOX_MIN_Y])/dy);
						
						CQASSERT(sliceA>=0 && sliceA<SLICES);
					}
					
					int walk = (sliceA > sliceB) ? -1 : 1;
					
					for (int slice=sliceA;slice<=sliceB;slice+=walk)
					{
						Vector vv;
						if (sliceA == sliceB)
							vv.set(max(vA.x,vB.x),max(vA.y,vB.y),max(vA.z,vB.z));
						else
							vv = (vA*abs(sliceB-slice)+vB*abs(sliceA-slice))/abs(sliceB-sliceA);
						
						if (extentDataRW->bX)
						{
							CQASSERT(slice>=0 && slice<SLICES);
							if (vv.y < extentDataRW->extents[slice].left)
								extentDataRW->extents[slice].left = vv.y;
							if (vv.y > extentDataRW->extents[slice].right)
								extentDataRW->extents[slice].right = vv.y;
						}
						else
						{
							CQASSERT(slice>=0 && slice<SLICES);
							if (vv.x < extentDataRW->extents[slice].left)
								extentDataRW->extents[slice].left = vv.x;
							if (vv.x > extentDataRW->extents[slice].right)
								extentDataRW->extents[slice].right = vv.x;
						}
						if (vv.z > extentDataRW->extents[slice].top)
							extentDataRW->extents[slice].top = vv.z;
						if (vv.z < extentDataRW->extents[slice].bottom)
							extentDataRW->extents[slice].bottom = vv.z;
						
						hit[slice] = TRUE;
						
						if (slice > high)
							high = slice;
						if (slice < low)
							low = slice;
					}
				}
				
				for (i=low;i<=high;i++)
				{
					weight[i] += tri_weight;
				}
				
				//	}
				/*else
				{
				Vector v = mesh->object_vertex_list[mesh->vertex_batch_list[fg->face_vertex_chain[3*f+i]]];
				
				  S8 slice = floor(SLICES*(v.z-box[BBOX_MIN_Z])/dz);
				  if (slice==0)
				  Dummy();
			}*/
			}
		}
		
		CQASSERT(hit[0] && hit[SLICES-1]);
		extentDataRW->min_slice = SLICES;
		extentDataRW->max_slice = 0;
		//fill in missing slices
		for (i=0;i<SLICES;i++)
		{
			
			S32 j,last=-1,next=-1;
			if (!hit[i])
			{
				for (j=0;j<i;j++)
				{
					if (hit[j])
					{
						last = j;
					}
				}
				for (j=SLICES-1;j>i;j--)
				{
					if (hit[j])
					{
						next=j;
					}
				}
				//CQASSERT(i>=0 && j >= 0);
				if (last != -1 && next != -1)
				{
					extentDataRW->extents[i].left   = (extentDataRW->extents[next].left  *(next-i)+extentDataRW->extents[last].left  *(i-last))/(next-last);
					extentDataRW->extents[i].right  = (extentDataRW->extents[next].right *(next-i)+extentDataRW->extents[last].right *(i-last))/(next-last);
					extentDataRW->extents[i].top    = (extentDataRW->extents[next].top   *(next-i)+extentDataRW->extents[last].top   *(i-last))/(next-last);
					extentDataRW->extents[i].bottom = (extentDataRW->extents[next].bottom*(next-i)+extentDataRW->extents[last].bottom*(i-last))/(next-last);
				}
			}
			else
			{
				if (extentDataRW->min_slice > i)
					extentDataRW->min_slice = i;
				if (extentDataRW->max_slice < i)
					extentDataRW->max_slice = i;
			}
		}
		
		//exclude low density areas on ends
		SINGLE totalWeight = 0;
		for (i=0;i<SLICES;i++)
		{
			totalWeight += weight[i];
		}
		for (i=0;i<SLICES;i++)
		{
			if (extentDataRW->min_slice == i)
			{
				ExtentDummy();
				if (weight[i]/totalWeight < 4e-2)
				{
					extentDataRW->min_slice++;
				}
			}
		}
		for (i=SLICES-1;i>=0;i--)
		{
			if (extentDataRW->max_slice == i)
			{
				if (weight[i]/totalWeight < 1e-2)
				{
					extentDataRW->max_slice--;
				}
			}
		}
		
		//cut off pointy things
		for (i=1;i<SLICES-1;i++)
		{
			if (extentDataRW->extents[i].top > extentDataRW->extents[i-1].top+200 && extentDataRW->extents[i].top > extentDataRW->extents[i+1].top+200)
			{
				extentDataRW->extents[i].top = (extentDataRW->extents[i-1].top+extentDataRW->extents[i+1].top)*0.5;
			}
		}
	}
	
	CQASSERT(extentData->_step);
}

//---------------------------------------------------------------------------
//
/*void TransformRayToObjectSpace(Vector & xorg, Vector & xdir, const Vector & org, const Vector & dir, const Vector & p, const Matrix & R)
{
Matrix RT = R.get_transpose();
xorg = RT * (org - p);
xdir = RT * dir;
}*/
//---------------------------------------------------------------------------
//
template <class Base>
BOOL32 ObjectExtent< Base >::GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction)
{
	return GetCollisionPosition (collide_point,dir,start,direction);
	/*
	BOOL32 result = FALSE;
	if (instanceIndex != INVALID_INSTANCE_INDEX)
	{
		
		const BaseExtent *extent;
		if (PHYSICS->get_extent(&extent,instanceIndex))
		{
			if (//COLLISION->intersect_ray_with_extent_hierarchy(collide_point,dir,start,direction,*extent,transform,true))
				result = TRUE;
		}
		
	}
	
	return result;*/
}
//---------------------------------------------------------------------------
//
template <class Base>
BOOL32 ObjectExtent< Base >::GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction)
{
	BOOL32 result = FALSE;
	if (instanceIndex != INVALID_INSTANCE_INDEX)
	{
		
		struct Ellipse ellipse,tran_ellipse;
		OBJBOX box;
		
		GetObjectBox(box);
		
		if (REND->get_archetype_centroid(HARCH(instanceIndex), LODPERCENT, ellipse.center) == 0)
			CQTRACE10("Centroid not found");
		
		ellipse.radius = 0.5*(box[BBOX_MAX_Z]-box[BBOX_MIN_Z]);
		ellipse.scale.x = (ellipse.radius/(0.5*(box[BBOX_MAX_X]-box[BBOX_MIN_X])));
		ellipse.scale.y = (ellipse.radius/(0.5*(box[BBOX_MAX_Y]-box[BBOX_MIN_Y])));
		ellipse.scale.z = 1;
		ellipse.radius *= 1.2;
		ellipse.center.set(0,0,0);
		ellipse.R.set_identity();
		//	ellipse.R = transform.get_inverse();
		
		ellipse.transform(&tran_ellipse, transform.translation, transform);
		
		Vector normdir = direction;
		normdir.normalize();
		result = RayEllipse(collide_point,dir,start,normdir,tran_ellipse);
		
		/*	dir = collide_point-tran_ellipse.center;
		dir.x *= fabs(tran_ellipse.scale.x);
		dir.y *= fabs(tran_ellipse.scale.y);
		dir.z *= fabs(tran_ellipse.scale.z);*/
		dir.normalize();
		
	}
	
	return result;

/*	S8 slice=0;
	BOOL32 hit=0;

	S32 cnt,lastSlice;
	
	Transform world_to_object = transform.get_inverse();
	Vector obj_start = world_to_object.rotate_translate(start);

	if (obj_start.z < z_min)
	{
		cnt = 1;
		lastSlice = SLICES;
		slice = 0;
	}
	else
	{
		cnt = -1;
		lastSlice = -1;
		slice = SLICES-1;
	}


	while (slice != lastSlice && !hit)
	{
		Box box;
	
		box.center.x = (extents[slice].left+extents[slice].right)*0.5;
		box.center.y = (extents[slice].top+extents[slice].bottom)*0.5;
		box.center.z = z_min+(slice+0.5)*z_step;
		box.half_x = fabs((extents[slice].left-extents[slice].right)*0.5);
		box.half_y = fabs((extents[slice].top-extents[slice].bottom)*0.5);
		box.half_z = z_step*0.5;

		BoxExtent extent(box);

		//my extents are geo-based, not center-of-mass based
		if (COLLISION->intersect_ray_with_extent(collide_point, start, direction, extent,
			ENGINE->get_position(instanceIndex), transform))
		{
			hit=1;
		}

		slice+=cnt;
	}

	return hit;*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectExtent< Base >::AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir)
{
	if (bVisible)
	{
		TRANSFORM trans(pos);
		trans = transform.get_inverse().multiply(trans);
		IBaseObject *obj;
		if ((obj = ARCHLIST->CreateInstance(pBlast)) != 0)
		{
			OBJPTR<IBlast> blast;

			if (obj->QueryInterface(IBlastID, blast))
			{
				blast->InitBlast(trans, systemID,this);
			}
		}

		AddChildBlast(obj);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectExtent< Base >::AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans)
{
	if (bVisible)
	{
		TRANSFORM trans(baseTrans);
		trans = transform.get_inverse().multiply(trans);
		IBaseObject *obj;
		if ((obj = ARCHLIST->CreateInstance(pBlast)) != 0)
		{
			OBJPTR<IBlast> blast;

			if (obj->QueryInterface(IBlastID, blast))
			{
				blast->InitBlast(trans, systemID,this);
			}
		}

		AddChildBlast(obj);
	}
}


//---------------------------------------------------------------------------
//
template <class Base>
void ObjectExtent< Base >::GetExtentInfo (cr &_extents,SINGLE *__step,SINGLE *__min,U8 *_slices)
{
	_extents = extentData->extents;
	*__step = extentData->_step;
	*__min = extentData->_min;
	*_slices = SLICES;
}
//---------------------------------------------------------------------------
//
template <class Base>
const RECT *ObjectExtent< Base >::GetExtentRect (SINGLE _val)
{
	return &extentData->extents[(S32)floor((_val-extentData->_min)/extentData->_step)];
}


template <class Base>
void ObjectExtent< Base >::postRenderExtent()
{
	IBaseObject *pos = childBlastList;
	const U32 currentSystem = SECTOR->GetCurrentSystem();
	const U32 currentPlayer = MGlobals::GetThisPlayer();
	const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();

	while (pos)
	{
		pos->TestVisible(defaults, currentSystem, currentPlayer);
		pos->Render();
		pos = pos->next;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectExtent< Base >::AddChildBlast (IBaseObject *obj)
{
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//


#endif
