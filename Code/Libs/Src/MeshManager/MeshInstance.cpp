//--------------------------------------------------------------------------//
//                                                                          //
//                              MeshInstance.CPP                            //
//                                                                          //
//               COPYRIGHT (C) 2004 By Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//

#include "stdafx.h"
#include "MeshArch.h"
#include "IMeshManager.h"
#include "MeshInstance.h"
#include "IInternalMeshManager.h"
#include <Mesh.h>
#include <ICamera.h>
#include <rendpipeline.h>
#include <IHardpoint.h>
#include <IChannel2.h>
#include <ChannelEventTypes.h>

MeshInstance::MeshInstance()
{
	owner = NULL;
	arch= NULL;
	instanceIndex = INVALID_INSTANCE_INDEX;
	bInDynDef = false;
	dynPos = 0;
	faceInstance = NULL;
	currentAnimation = INVALID_SCRIPT_INST;
	callback = NULL;
	modList = NULL;
	timer = 0.0;
}

MeshInstance::~MeshInstance()
{
	if(instanceIndex != INVALID_INSTANCE_INDEX)
		owner->GetEngine()->destroy_instance(instanceIndex);
	if(arch)
		arch->Release();
	if(faceInstance)
	{
		delete [] faceInstance;
		faceInstance = NULL;
	}
}

void MeshInstance::Initialize(MeshArch * meshArch, struct IEngineInstance * engInst)
{
	timer = 0.0;
	arch = meshArch;
	arch->AddRef();

	faceInstance = new INSTANCE_INDEX[arch->GetNumFaceGroups()];
	for(U32 i = 0; i < arch->GetNumFaceGroups(); ++i)
	{
		faceInstance[i] = INVALID_INSTANCE_INDEX;
	}
	if(!(arch->bDynamic))
	{
		instanceIndex = owner->GetEngine()->create_instance2(arch->GetEngineArchtype(),engInst);

		if(instanceIndex != INVALID_INSTANCE_INDEX)
		{
			ARCHETYPE_INDEX archIndex = owner->GetEngine()->get_instance_archetype(instanceIndex);
			for(U32 i = 0;i < arch->GetNumFaceGroups(); ++i)
			{
				if(arch->GetFaceArray()[i].archIndex == archIndex)
				{
					faceInstance[i] = instanceIndex;
				}
			}
			owner->GetEngine()->release_archetype(archIndex);

			U32 childInst = owner->GetEngine()->get_instance_child_next(instanceIndex,0,INVALID_INSTANCE_INDEX);
			while(childInst != INVALID_INSTANCE_INDEX)
			{
				archIndex = owner->GetEngine()->get_instance_archetype(childInst);
				for(U32 i = 0;i < arch->GetNumFaceGroups(); ++i)
				{
					if(arch->GetFaceArray()[i].archIndex == archIndex)
					{
						faceInstance[i] = childInst;
					}
				}
				owner->GetEngine()->release_archetype(archIndex);
				childInst = owner->GetEngine()->get_instance_child_next(instanceIndex,0,childInst);
			}
		}
	}
	{
		static int diagN = 0;
		if(diagN < 20)
		{
			++diagN;
			U32 matched = 0;
			for(U32 i = 0; i < arch->GetNumFaceGroups(); ++i)
				if(faceInstance[i] != INVALID_INSTANCE_INDEX) ++matched;
			FILE* df = fopen("Debug\\mesh_diag.txt", "a");
			if(df)
			{
				fprintf(df, "MeshInst::Init[%d] bDynamic=%d instanceIdx=%d(INVALID=%d) numFG=%d matched=%d\n",
					diagN, (int)arch->bDynamic, (int)instanceIndex, (int)INVALID_INSTANCE_INDEX,
					(int)arch->GetNumFaceGroups(), (int)matched);
				fflush(df); fclose(df);
			}
		}
	}
}

INSTANCE_INDEX MeshInstance::findInstanceFromArch(ARCHETYPE_INDEX baseArch)
{
	if(arch)
	{
		for(U32 i = 0; i < arch->GetNumFaceGroups() ; ++i)
		{
			if(arch->GetFaceArray()[i].archIndex == baseArch)
			{
				return faceInstance[i];
			}
		}
	}
	return INVALID_INSTANCE_INDEX;
}

IMeshArchetype * MeshInstance::GetArchtype()
{
	return arch;
}

void MeshInstance::Update(SINGLE dt)
{
	timer += dt;
	if(instanceIndex != INVALID_INSTANCE_INDEX)
	{
		owner->GetEngine()->update_instance(instanceIndex,0,dt);
		owner->GetAnim()->update_instance(instanceIndex,dt);
	}
}

void MeshInstance::SetTransform(Transform & trans)
{
	if(instanceIndex != INVALID_INSTANCE_INDEX)
		owner->GetEngine()->set_transform(instanceIndex,trans);
}

Transform ident;

const Transform & MeshInstance::GetTransform()
{
	if(!(arch->bDynamic))
		return owner->GetEngine()->get_transform(instanceIndex);
	ident.set_identity();
	return ident;
}

void MeshInstance::GetHardPointTransform(U32 index, Transform & trans)
{
	if(arch)
	{
		HardPointDef hpDef;
		if(arch->FindHardPontFromIndex(index,hpDef))
		{
			HardpointInfo info;
			if(owner->GetHardpoint()->retrieve_hardpoint_info(hpDef.archIndex,hpDef.name,info))
			{
				INSTANCE_INDEX parentIndex = findInstanceFromArch(hpDef.archIndex);
				if(parentIndex != INVALID_INSTANCE_INDEX)
				{
					trans.set_orientation(info.orientation);
					trans.set_position(info.point);
					trans = owner->GetEngine()->get_transform(parentIndex)*trans;
					return;
				}
			}
		}
	}
	trans = GetTransform();
}

void MeshInstance::Render()
{
	//this is just a sample
	//  does not handle children and child transforms
	{
		static int diagN = 0;
		if(diagN < 5)
		{
			++diagN;
			FILE* df = fopen("Debug\\mesh_diag.txt", "a");
			if(df)
			{
				fprintf(df, "MeshInst::Render[%d] numFG=%d\n", diagN, (int)arch->GetNumFaceGroups());
				for(U32 dface = 0; dface < arch->GetNumFaceGroups(); ++dface)
				{
					IMaterial* dmat = arch->GetFaceArray()[dface].mat;
					IMaterial* dfb = !dmat ? owner->GetMatManager()->FindMaterial("Default.txt") : (IMaterial*)0;
					fprintf(df, "  fg[%d] mat=%s fallback=%s faceInst=%d numIdx=%d\n",
						(int)dface, dmat ? "ok" : "NULL", dmat ? "n/a" : (dfb ? "ok" : "NULL"),
						(int)faceInstance[dface], (int)arch->GetFaceArray()[dface].numIndex);
				}
				fflush(df); fclose(df);
			}
		}
	}
	for(U32 face = 0; face < arch->GetNumFaceGroups(); ++face)
	{
		IMaterial * mat = arch->GetFaceArray()[face].mat;
		if(!mat)
		{
			mat = owner->GetMatManager()->FindMaterial("Default.txt");
			if(mat)
			{
				if(!(mat->IsRealized()))
					mat->Realize();
			}
		}
		if(mat)
		{
			//set up transform
			Transform trans;
			if(faceInstance[face] != INVALID_INSTANCE_INDEX)
			{
				trans = owner->GetEngine()->get_transform(faceInstance[face]);
			}
			else
			{
				trans.set_identity();
			}

			Transform to_view = owner->GetCamera()->get_inverse_transform();
			
			to_view = to_view.multiply(trans);

			owner->GetPipe()->set_modelview(to_view);


			Transform inv(false);
		
			inv = trans.get_inverse();

			Vector cam_pos_in_object_space = inv.rotate_translate(owner->GetCamera()->get_position());
		
			owner->GetPipe()->set_default_constants(trans,cam_pos_in_object_space,4);

			U32 vertexBuffer = arch->GetVertexBufferForType(mat->GetVertexType());
			mat->DrawVB((IDirect3DVertexBuffer9 *)vertexBuffer,arch->GetFaceArray()[face].indexBuffer,arch->GetFaceArray()[face].startVertex,
				arch->GetFaceArray()[face].numVerts,arch->GetFaceArray()[face].startIndex,arch->GetFaceArray()[face].numIndex,modList,timer);
		}
	}
}

void MeshInstance::BeginDynamicDef()
{
	if(arch->bDynamic)
	{
		bInDynDef = true;
		dynPos = 0;
	}
}

void MeshInstance::EndDynamicDef()
{
	if(arch->bDynamic && bInDynDef)
	{
		arch->GetFaceArray()[0].numVerts =dynPos;
		arch->InvalidateBuffers();//tells the arch to recompute vertex buffers
		bInDynDef = false;
	}
}

void MeshInstance::DynDef_SetTex1(const SINGLE & u, const SINGLE & v)
{
	if(arch->bDynamic)
	{
		ASSERT(dynPos < arch->GetNumVerts());
		arch->GetVertexList()[dynPos].u = u;
		arch->GetVertexList()[dynPos].v = v;
	}
}

void MeshInstance::DynDef_SetTex2(const SINGLE & u, const SINGLE & v)
{
	if(arch->bDynamic)
	{
		ASSERT(dynPos < arch->GetNumVerts());
		arch->GetVertexList()[dynPos].u2 = u;
		arch->GetVertexList()[dynPos].v2 = v;
	}
}

void MeshInstance::DynDef_SetNormal(const Vector & normal)
{
	if(arch->bDynamic)
	{
		ASSERT(dynPos < arch->GetNumVerts());
		arch->GetVertexList()[dynPos].normal = normal;
	}
}

void MeshInstance::DynDef_SetColor(const U8 & red, const U8 & green,const U8 & blue,const U8 & alpha)
{
	if(arch->bDynamic)
	{
		ASSERT(dynPos < arch->GetNumVerts());
		arch->GetVertexList()[dynPos].red= red;
		arch->GetVertexList()[dynPos].green= green;
		arch->GetVertexList()[dynPos].blue= blue;
		arch->GetVertexList()[dynPos].alpha = alpha;
	}
}

void MeshInstance::DynDef_SetPos(const Vector & position)
{
	if(arch->bDynamic)
	{
		ASSERT(dynPos < arch->GetNumVerts());
		arch->GetVertexList()[dynPos].pos = position;

		//comput tangent and binormal for the last three verts 
		if(dynPos%3 == 2)
		{
			Vector UV1, UV2, UV0;
			Vector du,dv;

			MeshVertex & v0 = arch->GetVertexList()[dynPos-2];
			MeshVertex & v1 = arch->GetVertexList()[dynPos-1];
			MeshVertex & v2 = arch->GetVertexList()[dynPos];

			UV0.x = v0.u;
			UV0.y = v0.v;
			UV1.x = v1.u;
			UV1.y = v1.v;
			UV2.x = v2.u;
			UV2.y = v2.v;

            Vector edge01( v1.pos.x - v0.pos.x, UV1.x - UV0.x, UV1.y - UV0.y );
            Vector edge02( v2.pos.x - v0.pos.x, UV2.x - UV0.x, UV2.y - UV0.y );
            Vector cp = cross_product(edge01, edge02);
			cp.fast_normalize();
            if( fabs(cp.x) > 1e-8 )
            {
                du.x = -cp.y / cp.x;        
                dv.x = -cp.z / cp.x;
            }
            edge01 = Vector( v1.pos.y - v0.pos.y, UV1.x - UV0.x, UV1.y - UV0.y );
            edge02 = Vector( v2.pos.y - v0.pos.y, UV2.x - UV0.x, UV2.y - UV0.y );
            cp = cross_product(edge01, edge02 );
			cp.fast_normalize();
            if( fabs(cp.x) > 1e-8 )
            {
                du.y = -cp.y / cp.x;
                dv.y = -cp.z / cp.x;
            }
			edge01 = Vector( v1.pos.z - v0.pos.z, UV1.x - UV0.x, UV1.y - UV0.y );
            edge02 = Vector( v2.pos.z - v0.pos.z, UV2.x - UV0.x, UV2.y - UV0.y );
            cp = cross_product(edge01, edge02 );
			cp.fast_normalize();
            if( fabs(cp.x) > 1e-8 )
            {
                du.z = -cp.y / cp.x;
                dv.z = -cp.z / cp.x;
            }
			du.fast_normalize();
			dv.fast_normalize();

			v0.tangent = du;
            v1.tangent = du;
            v2.tangent = du;

			v0.binormal = dv;
            v1.binormal = dv;
            v2.binormal = dv;
		}
		++dynPos;
	}
}

bool MeshInstance::ComputeHitTest(Vector * start, Vector * dir, HitDef & hitDef)
{
	if(instanceIndex == INVALID_INSTANCE_INDEX)
		return false;

	bool bHit = false;
	SINGLE bestDist = 0;
	U32 bestFaceGroup = 0;
	for(U32 faceGroup = 0; faceGroup < arch->GetNumFaceGroups(); ++faceGroup)
	{
		Transform trans;
		if(faceInstance[faceGroup] != INVALID_INSTANCE_INDEX)
		{
			trans = owner->GetEngine()->get_transform(faceInstance[faceGroup]);
		}
		else
		{
			trans.set_identity();
		}

		Transform inv(false);
	
		SINGLE w;

		inv = trans.get_general_inverse(w);

		Vector transStart = inv.rotate_translate(*start);
		Vector transDir = inv.rotate(*dir);

		if(D3DXBoxBoundProbe ((D3DXVECTOR3 *)(&(arch->GetFaceArray()[faceGroup].minBox)),
			(D3DXVECTOR3 *)(&(arch->GetFaceArray()[faceGroup].maxBox)),(D3DXVECTOR3 *)(&transStart),(D3DXVECTOR3 *)(&transDir)))
		{
			if(arch->GetFaceArray()[faceGroup].indexBuffer)
			{
				U16 * ind = NULL;
				arch->GetFaceArray()[faceGroup].indexBuffer->Lock(0,arch->GetFaceArray()[faceGroup].numIndex*sizeof(U16),(void **)(&ind),D3DLOCK_READONLY);

				for(U32 face = 0; face*3 < arch->GetFaceArray()[faceGroup].numIndex; ++face)
				{
					SINGLE u,v,dist;
					if(D3DXIntersectTri((D3DXVECTOR3 *)(&(arch->GetVertexList()[ind[face*3]])),
						(D3DXVECTOR3 *)(&(arch->GetVertexList()[ind[face*3+1]])),
						(D3DXVECTOR3 *)(&(arch->GetVertexList()[ind[face*3+2]])),
						(D3DXVECTOR3 *)(&transStart),
						(D3DXVECTOR3 *)(&transDir),
						&u,
						&v,
						&dist))
					{
						if(bHit)
						{
							if(dist < bestDist)
							{
								bestDist = dist;
								bestFaceGroup = faceGroup;
							}
						}
						else
						{
							bHit = true;
							bestDist = dist;
							bestFaceGroup = faceGroup;
						}
					}
				}
				arch->GetFaceArray()[faceGroup].indexBuffer->Unlock();
			}
		}
	}
	if(bHit)
	{
		hitDef.faceGroup = bestFaceGroup;
		hitDef.hitPosition = (*start)+((*dir)*bestDist);
		hitDef.hitDist = bestDist;
		return true;
	}
	return false;
}

void MeshInstance::PlayAnimation(const char * animName, bool bLooping)
{
	KillAnimations();
	currentAnimation = owner->GetAnim()->create_script_inst(arch->GetAnimArchtype(),instanceIndex,animName,this,NULL);
	if(currentAnimation != INVALID_SCRIPT_INST)
		owner->GetAnim()->script_start(currentAnimation,bLooping?(Animation::FORWARD|Animation::LOOP):Animation::FORWARD,Animation::BEGIN);
}

void MeshInstance::KillAnimations()
{
	if(currentAnimation != INVALID_SCRIPT_INST)
	{
		owner->GetAnim()->script_stop(currentAnimation);
		owner->GetAnim()->release_script_inst(currentAnimation);
		currentAnimation = INVALID_SCRIPT_INST;
	}
}

void MeshInstance::SetCallback(IMeshCallback * newCallback)
{
	callback = newCallback;
}

void MeshInstance::SetModifierList(struct IModifier * _modList)
{
	modList = _modList;
}

SINGLE MeshInstance::GetBoundingRadius()
{
	SINGLE rad;
	Vector center;
	owner->GetEngine()->get_instance_bounding_sphere(instanceIndex,0,&rad,&center);
	return rad;
}

Vector MeshInstance::GetRandomSurfacePos(U32 seed, IMaterial * filterMat)
{
	if(arch)
	{
		if(!filterMat)
		{
			U32 fgIndex = seed%(arch->GetNumFaceGroups());
			MeshFace & face = arch->GetFaceArray()[fgIndex];
			if(face.numIndex)
			{
				U32 triIndex = ((seed*seed*1231)+seed)%(face.numIndex);
				triIndex = triIndex - (triIndex%3);
				SINGLE bay1 = ((SINGLE)((seed+2355675)%1000))/1000.0f;
				SINGLE bay2 = (((SINGLE)((seed+2355675)%1000))/1000.0f)*(1.0f-bay1);
				U16 * ind = NULL;
				face.indexBuffer->Lock(0,face.numIndex*sizeof(U16),(void **)(&ind),D3DLOCK_READONLY);
				Vector & p1 = arch->GetVertexList()[ind[triIndex]].pos;
				Vector & p2 = arch->GetVertexList()[ind[triIndex+1]].pos;
				Vector & p3 = arch->GetVertexList()[ind[triIndex+2]].pos;
				face.indexBuffer->Unlock();
				Vector final = (bay1*(p2-p1))+(bay2*(p3-p1))+p1;

				Transform trans = owner->GetEngine()->get_transform(faceInstance[fgIndex]);
				final = trans.rotate_translate(final);
				return final;
			}
		}
		else//filter the face groups to ionly use the filter mat
		{
			U32 goodFG = 0;
			U32 maxFG = arch->GetNumFaceGroups();
			for(U32 fgIndex = 0; fgIndex < maxFG; ++ fgIndex)
			{
				MeshFace & face = arch->GetFaceArray()[fgIndex];
				if(face.mat == filterMat)
				{
					++goodFG;
				}
			}
			if(goodFG)
			{
				U32 fgCount = seed%(goodFG);
				U32 fgIndex;
				for(fgIndex = 0; fgIndex < maxFG; ++fgIndex)
				{
					MeshFace & face = arch->GetFaceArray()[fgIndex];
					if(face.mat == filterMat)
					{
						if(!fgCount)
						{
							break;
						}
						--fgCount;
					}
				}
				MeshFace & face = arch->GetFaceArray()[fgIndex];
				if(face.numIndex)
				{
					U32 triIndex = ((seed*seed*1231)+seed)%(face.numIndex);
					triIndex = triIndex - (triIndex%3);
					SINGLE bay1 = ((SINGLE)((seed+2355675)%1000))/1000.0f;
					SINGLE bay2 = (((SINGLE)((seed+2355675)%1000))/1000.0f)*(1.0f-bay1);
					U16 * ind = NULL;
					face.indexBuffer->Lock(0,face.numIndex*sizeof(U16),(void **)(&ind),D3DLOCK_READONLY);
					Vector & p1 = arch->GetVertexList()[ind[triIndex]].pos;
					Vector & p2 = arch->GetVertexList()[ind[triIndex+1]].pos;
					Vector & p3 = arch->GetVertexList()[ind[triIndex+2]].pos;
					face.indexBuffer->Unlock();
					Vector final = (bay1*(p2-p1))+(bay2*(p3-p1))+p1;

					Transform trans = owner->GetEngine()->get_transform(faceInstance[fgIndex]);
					final = trans.rotate_translate(final);
					return final;
				}			
			}
		}
	}
	return Vector(0,0,0);
}

INSTANCE_INDEX MeshInstance::FindChild (const char * pathname)
{
	INSTANCE_INDEX parent = instanceIndex;
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = owner->GetEngine()->get_instance_child_next(parent,EN_DONT_RECURSE, child)) != -1)
	{
		const char *name = owner->GetEngine()->get_instance_part_name(child);
		if (strcmp(name, buffer) == 0)
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}

void COMAPI MeshInstance::on_event(unsigned int channel_id, void * user_supplied, const EventIterator& it)
{
	if(callback)
	{
		for(U32 i = 0; i < it.get_event_count(); ++i)
		{
			if(it.get_event_type(i) == NAMED_EVENT)
			{
				char * data = (char*)(it.get_event_data(i));
				callback->AnimationCue(this,data);
			}
		}
	}
}

void COMAPI MeshInstance::on_finished(unsigned int channel_id, void * user_supplied)
{
}

void COMAPI MeshInstance::on_loop(unsigned int channel_id, Transform & xform, void * user_supplied)
{
}
