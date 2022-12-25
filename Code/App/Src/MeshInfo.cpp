//--------------------------------------------------------------------------//
//                                                                          //
//                              Meshinfo->cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MeshInfo.cpp 30    10/03/00 1:31a Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "MeshRender.h"

#include <renderer.h>
	
struct MeshInfoTree : IMeshInfoTree
{
	MeshInfo *info;

	MeshInfoTree *child;
	MeshInfoTree *next;

	U32 num_children;

	MeshInfoTree(INSTANCE_INDEX id);
	~MeshInfoTree();

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual MeshInfo *GetMeshInfo();

	virtual void DetachChild(INSTANCE_INDEX childID,IMeshInfoTree **child_info);

	virtual void AttachChild(INSTANCE_INDEX parentID,IMeshInfoTree *child_info);

	virtual void GetChildInfo(INSTANCE_INDEX childID,IMeshInfoTree **child_info);
	
	virtual U32 ListChildren(MeshInfo **mesh_info);

	virtual U32 ListFirstChildren(IMeshInfoTree **mesh_info);

	void listChildren(MeshInfo **mesh_info,const MeshInfoTree *parent);

	void MakeChildren (INSTANCE_INDEX parentIdx);

	void makeChildren (INSTANCE_INDEX parentIdx,MeshInfoTree *parent);

	void findChild(INSTANCE_INDEX childID,MeshInfoTree *parent,MeshInfoTree **child,MeshInfoTree **prev);

	virtual void LoseOwnershipOfMeshInfo()
	{
		info = NULL;
	}
};
	
MeshInfoTree::MeshInfoTree(INSTANCE_INDEX id)
{
	info = new MeshInfo;
	info->parent = this;
	info->instanceIndex = id;
//	info->buildMesh = REND->get_unique_instance_mesh(info->instanceIndex);
	Mesh *m = REND->get_instance_mesh(info->instanceIndex);
	if (m)
	{
		info->radius = m->radius;
		info->bHasMesh = true;
		m->set_lod(1.0f);
//		info->myFaceArray = new MyFace[m->face_cnt];
//		info->hiddenArray = new FACE_STATE[m->face_cnt];
//		info->face_props = new FACE_PROPERTY[m->face_cnt];
		
/*		S32 i,j;
		//bubble sort faces on x coord
		
		//setup array
		S32 total=0;
		for (i=0;i<m->face_group_cnt;i++)
		{
			memcpy(&info->face_props[total],m->face_groups[i].face_properties,m->face_groups[i].face_cnt*sizeof(FACE_PROPERTY));
			for (j=0;j<m->face_groups[i].face_cnt;j++)
			{
				info->myFaceArray[total].groupID = i;
				info->myFaceArray[total].index = j;
				//	info->hiddenArray[total] = FS_HIDDEN;
				total++;
			}
		}*/

		//here??
//		info->Sort(true);
	}
	else
		info->radius=1000.0f;
}



MeshInfoTree::~MeshInfoTree()
{
	if (info)
		delete info;
	//recursive deleting?
	if (child)
		delete child;
	if (next)
		delete next;
}

MeshInfo *MeshInfoTree::GetMeshInfo()
{
	return info;
}

void MeshInfoTree::DetachChild(INSTANCE_INDEX childID,IMeshInfoTree **child_info)
{
	MeshInfoTree *child=0,*prev=0;
	findChild(childID,this,&child,&prev);
	CQASSERT(prev && "can't detach parent object from mesh tree");
	if (prev->child && prev->child->info->instanceIndex == childID)
	{
		*child_info = prev->child;
		prev->child = prev->child->next;
		child->next = NULL;
	}
	else
	{
		CQASSERT(prev->next->info->instanceIndex == childID);
		*child_info = prev->next;
		child = prev->next;
		prev->next = prev->next->next;
		child->next = NULL;
	}
}

void MeshInfoTree::AttachChild(INSTANCE_INDEX parentID,IMeshInfoTree *child_info)
{
	MeshInfoTree *child=0,*prev=0;
	findChild(parentID,0,&child,&prev);
	CQASSERT(child);
	if (child->child == 0)
		child->child = static_cast<MeshInfoTree *>(child_info);
	else
	{
		child = child->child;
		while (child->next)
			child = child->next;
		child->next = static_cast<MeshInfoTree *>(child_info);
	}
}

void MeshInfoTree::GetChildInfo(INSTANCE_INDEX childID,IMeshInfoTree **child_info)
{
	MeshInfoTree *child=0,*prev=0;
	findChild(childID,this,&child,&prev);
	CQASSERT(child);
	if (prev->child && prev->child->info->instanceIndex == childID)
	{
		*child_info = prev->child;
	}
	else
	{
		CQASSERT(prev->next->info->instanceIndex == childID);
		*child_info = prev->next;
	}
}

void MeshInfoTree::findChild(INSTANCE_INDEX childID,MeshInfoTree *parent,MeshInfoTree **child,MeshInfoTree **prev)
{
	MeshInfoTree *my_prev,*my_child;
	my_prev = parent;
	if (parent)
		my_child = parent->child;
	else
		my_child = this;

	while (*child == 0 && my_child && my_child->info->instanceIndex != childID)
	{
		my_prev = my_child;
		findChild(childID,my_child,child,prev);
		if (*child == 0)
			my_child = my_child->next;
	}

	if (my_child && my_child->info->instanceIndex == childID)
	{
		*child = my_child;
		*prev = my_prev;
	}
}
	
//compiles children in this part of the tree into a linear list
U32 MeshInfoTree::ListChildren(MeshInfo **mesh_info)
{
	num_children = 1;
	mesh_info[0] = info;
	listChildren(mesh_info,this);

	return num_children;
}

U32 MeshInfoTree::ListFirstChildren(IMeshInfoTree **mesh_info)
{
	int cnt=0;
	MeshInfoTree *pos = child;
	while (pos)
	{
		mesh_info[cnt++] = pos;
		pos = pos->next;
	}

	return cnt;
}

void MeshInfoTree::listChildren(MeshInfo **mesh_info,const MeshInfoTree *parent)
{
	MeshInfoTree *child = parent->child;
	while (child)
	{
		mesh_info[num_children++] = child->info;
		listChildren(mesh_info,child);
		child = child->next;
	}
}


void MeshInfoTree::MakeChildren (INSTANCE_INDEX parentIdx)
{
	num_children = 1;
	//info->instanceIndex = parentIdx;
	info->bWhole = TRUE;
	makeChildren (parentIdx,this);
}

void MeshInfoTree::makeChildren (INSTANCE_INDEX parentIdx,MeshInfoTree *parent)
{
	INSTANCE_INDEX childIndex = ENGINE->get_instance_child_next(parentIdx,0,INVALID_INSTANCE_INDEX);
	MeshInfoTree *child=0;
	if (childIndex != INVALID_INSTANCE_INDEX)
	{
		parent->child = new MeshInfoTree(childIndex);
		child = parent->child;
	}

	while ((childIndex != INVALID_INSTANCE_INDEX))// && (numChildren < MAX_CHILDS))
	{
	//	child->info->instanceIndex = childIndex;
		child->info->bWhole = TRUE;
		num_children++;
		CQASSERT(num_children <= MAX_CHILDS && "See Rob");
		makeChildren(childIndex,child);
		childIndex = ENGINE->get_instance_child_next(parentIdx, EN_DONT_RECURSE, childIndex);
		if (childIndex != INVALID_INSTANCE_INDEX)
		{
			child->next = new MeshInfoTree(childIndex);
			child = child->next;
		}
	}
}
//-----------------------------------------------------------------------------

MeshChain::~MeshChain()
{
	if (bOwnsChildren)
	{
		ENGINE->destroy_instance(mi[0]->instanceIndex);
		for (int i=0;i<numChildren;i++)
		{
			delete mi[i];
			mi[i] = 0;
		}
	}
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

MeshInfo::MeshInfo()
{
	fgi = 0;
	ec_list = 0;
	parent = 0;
	mr = 0;
	sphere_center.zero();
	bHasMesh = bSuppressEmissive = 0;
	faceRenders = 0;
	cameraMoveCnt = 0;
	timer = 0;
}

MeshInfo::~MeshInfo()
{
/*	if (myFaceArray)
	{
		delete [] myFaceArray;
		myFaceArray = 0;
	}
	if (hiddenArray)
	{
		delete [] hiddenArray;
		hiddenArray=0;
	}*/
/*	if (face_props)
	{
		delete [] face_props;
		face_props=0;
	}*/
	if (fgi)
	{
		delete [] fgi;
		fgi = 0;
	}
	if (faceRenders)
	{
		delete [] faceRenders;
		faceRenders = 0;
	}
	while (ec_list)
	{
		IEffectChannel *temp = ec_list->next;
//		DeleteEffectChannel(ec_list);
		delete ec_list;
		ec_list = temp;
	}
	if (bOwnsInstance && instanceIndex != -1)
		ENGINE->destroy_instance(instanceIndex);
	if (mr)
		mr->Release();
}

IEffectChannel * MeshInfo::GetNewEffectChannel()
{
	IEffectChannel *new_ec = CreateEffectChannel();

	new_ec->next = ec_list;
	ec_list = new_ec;
	new_ec->mi = this;
	return new_ec;
}

void MeshInfo::RemoveEffectChannel(IEffectChannel *ec)
{
	IEffectChannel *pos = ec_list;
	IEffectChannel *last = 0;
	while (pos && pos != ec)
	{
		last = pos;
		pos = pos->next;
	}

	CQASSERT(pos);
	if (pos)
	{
		if (last)
			last->next = pos->next;
		else
			ec_list = pos->next;
	}
}

#if 0
void MeshInfo::Sort(Mesh *m,bool bZ)
{
	S32 total = m->face_cnt;
	if (bZ)
	{
		for (int i=0;i<total-1;i++)
		{
			for (int j=i+1;j<total;j++)
			{
				if (m->object_vertex_list[m->vertex_batch_list[ m->face_groups[info->myFaceArray[j].groupID].face_vertex_chain[info->myFaceArray[j].index*3] ]].z < 
					m->object_vertex_list[m->vertex_batch_list[ m->face_groups[info->myFaceArray[i].groupID].face_vertex_chain[info->myFaceArray[i].index*3] ]].z)
					//face_vertices[info->face_array[i]]]].z)
				{
					MyFace temp;
					
					temp = myFaceArray[i];
					myFaceArray[i] = myFaceArray[j];
					myFaceArray[j] = temp;
					
				}
			}
		}
	}
	else
	{
		for (int i=0;i<total-1;i++)
		{
			for (int j=i+1;j<total;j++)
			{
				if (m->object_vertex_list[m->vertex_batch_list[ m->face_groups[info->myFaceArray[j].groupID].face_vertex_chain[info->myFaceArray[j].index*3] ]].x < 
					m->object_vertex_list[m->vertex_batch_list[ m->face_groups[info->myFaceArray[i].groupID].face_vertex_chain[info->myFaceArray[i].index*3] ]].x)
					//face_vertices[info->face_array[i]]]].z)
				{
					MyFace temp;
					
					temp = myFaceArray[i];
					myFaceArray[i] = myFaceArray[j];
					myFaceArray[j] = temp;
					
				}
			}
		}
	}
}
#endif

void MeshInfo::CalculateSphere()
{
	radius = 10;
	sphere_center.set(0,0,0);

	if (mr->face_cnt == 0 || bHasMesh == 0)
		return;
	
	SINGLE max_x,max_y,max_z;
	SINGLE min_x,min_y,min_z;
	
	max_x = max_y = max_z = -99999.0f;
	min_x = min_y = min_z = 99999.0f;
	
	for (int v=0;v<mr->pos_cnt;v++)
	{
		Vector &p = mr->pos_list[v];
		
		if (p.x < min_x)
			min_x = p.x;
		if (p.y < min_y)
			min_y = p.y;
		if (p.z < min_z)
			min_z = p.z;
		
		if (p.x > max_x)
			max_x = p.x;
		if (p.y > max_y)
			max_y = p.y;
		if (p.z > max_z)
			max_z = p.z;
	}
	
	sphere_center.x = (min_x+max_x)*0.5;
	sphere_center.y = (min_y+max_y)*0.5;
	sphere_center.z = (min_z+max_z)*0.5;

	radius = (Vector(max_x,max_y,max_z)-sphere_center).magnitude();
}

void MeshInfo::GetBoundingBox(float *box)
{
	if (mr->face_cnt == 0 || bHasMesh == 0)
		return;
	
	SINGLE max_x,max_y,max_z;
	SINGLE min_x,min_y,min_z;
	
	max_x = max_y = max_z = -99999.0f;
	min_x = min_y = min_z = 99999.0f;
	
	for (int v=0;v<mr->pos_cnt;v++)
	{
		Vector &p = mr->pos_list[v];
		
		if (p.x < min_x)
			min_x = p.x;
		if (p.y < min_y)
			min_y = p.y;
		if (p.z < min_z)
			min_z = p.z;
		
		if (p.x > max_x)
			max_x = p.x;
		if (p.y > max_y)
			max_y = p.y;
		if (p.z > max_z)
			max_z = p.z;
	}
	
	box[BBOX_MIN_X] = min_x;
	box[BBOX_MAX_X] = max_x;
	box[BBOX_MIN_Y] = min_y;
	box[BBOX_MAX_Y] = max_y;
	box[BBOX_MIN_Z] = min_z;
	box[BBOX_MAX_Z] = max_z;
}

IMeshInfoTree * CreateMeshInfoTree(INSTANCE_INDEX index)
{
	MeshInfoTree *mmesh = new MeshInfoTree(index);
	mmesh->MakeChildren(index);
	return mmesh;
}

void DestroyMeshInfoTree(IMeshInfoTree *mesh_info)
{
	MeshInfoTree * bubba = static_cast<MeshInfoTree *>(mesh_info);
	delete bubba;
}

/*void InitMeshInfo(MeshInfo & info,INSTANCE_INDEX id)
{
	info.instanceIndex = id;
	Mesh *m = REND->get_instance_mesh(info.instanceIndex);
	if (m)
	{
		info.radius = m->radius;
		info.bHasMesh = true;
		m->set_lod(1.0f);
//		info.myFaceArray = new MyFace[m->face_cnt];
//		info.hiddenArray = new FACE_STATE[m->face_cnt];
//		info.face_props = new FACE_PROPERTY[m->face_cnt];
		
		//here??
		//Sort(true);
	}
	else
		info.radius = 1000.0f;
}*/

/*void InitMeshInfo(MeshInfo &info,IMeshRender *imr,Mesh *mesh)
{
	info.instanceIndex = -1;
	info.myFaceArray = new MyFace[];

}*/

void CopyMeshInfo(MeshInfo & info,const MeshInfo &src,bool bCopyBuffers)
{
	info.instanceIndex = src.instanceIndex;
	info.radius = src.radius;
	info.bHasMesh = src.bHasMesh;
	//pad for goofy readahead behavior
//	info.lit = new LightRGB_U8[vert_cnt+vert_cnt2+1];
	CQASSERT(info.fgi == 0);
	info.fgi = new FaceGroupInfo[src.faceGroupCnt];
	info.faceGroupCnt = src.faceGroupCnt;
	info.bCacheValid = false;
	info.mr = src.mr;
	info.bHasMesh = src.bHasMesh;

	if (bCopyBuffers)
	{
		//added padding for cache opt....
		info.faceRenders = new U8[src.mr->face_cnt+1];
		memcpy(info.faceRenders,src.faceRenders,sizeof(U8)*(src.mr->face_cnt+1));
	}

	IRenderMaterial *pos=0;

	while ((pos = src.mr->GetNextFaceGroup(pos)) != 0)
	{
	/*	info.fgi[pos->fg_idx].diffuse = src.fgi[pos->fg_idx].diffuse;
		info.fgi[pos->fg_idx].emissive = src.fgi[pos->fg_idx].emissive;
		info.fgi[pos->fg_idx].a = src.fgi[pos->fg_idx].a;
		info.fgi[pos->fg_idx].texture_id = src.fgi[pos->fg_idx].texture_id;
		info.fgi[pos->fg_idx].texture_flags = src.fgi[pos->fg_idx].texture_flags;
		info.fgi[pos->fg_idx].emissive_texture_id = src.fgi[pos->fg_idx].emissive_texture_id;
		info.fgi[pos->fg_idx].emissive_texture_flags = src.fgi[pos->fg_idx].emissive_texture_flags;
		info.fgi[pos->fg_idx].unique_id = src.fgi[pos->fg_idx].unique_id;*/
		info.fgi[pos->fg_idx] = src.fgi[pos->fg_idx];
	}
}
//I hereby define split_n to point in the direction of out1
void SplitMeshInfo(const MeshInfo &src,MeshInfo &out0,MeshInfo &out1,SINGLE split_d,const Vector &split_n)
{
	out0.fgi = new FaceGroupInfo[src.faceGroupCnt];
	out0.faceGroupCnt = src.faceGroupCnt;
	out0.bCacheValid = false;
	out0.radius = src.radius;
	out0.bHasMesh = src.bHasMesh;
	out1.fgi = new FaceGroupInfo[src.faceGroupCnt];
	out1.faceGroupCnt = src.faceGroupCnt;
	out1.bCacheValid = false;
	out1.radius = src.radius;
	out1.bHasMesh = src.bHasMesh;

	IRenderMaterial *pos=0;

	while ((pos = src.mr->GetNextFaceGroup(pos)) != 0)
	{
		/*out0.fgi[pos->fg_idx].diffuse = src.fgi[pos->fg_idx].diffuse;
		out0.fgi[pos->fg_idx].emissive = src.fgi[pos->fg_idx].emissive;
		out0.fgi[pos->fg_idx].a = src.fgi[pos->fg_idx].a;
		out0.fgi[pos->fg_idx].texture_id = src.fgi[pos->fg_idx].texture_id;
		out0.fgi[pos->fg_idx].texture_flags = src.fgi[pos->fg_idx].texture_flags;
		out0.fgi[pos->fg_idx].emissive_texture_id = src.fgi[pos->fg_idx].emissive_texture_id;
		out0.fgi[pos->fg_idx].emissive_texture_flags = src.fgi[pos->fg_idx].emissive_texture_flags;
		out1.fgi[pos->fg_idx].unique_id = src.fgi[pos->fg_idx].unique_id;

		out1.fgi[pos->fg_idx].diffuse = src.fgi[pos->fg_idx].diffuse;
		out1.fgi[pos->fg_idx].emissive = src.fgi[pos->fg_idx].emissive;
		out1.fgi[pos->fg_idx].a = src.fgi[pos->fg_idx].a;
		out1.fgi[pos->fg_idx].texture_id = src.fgi[pos->fg_idx].texture_id;
		out1.fgi[pos->fg_idx].texture_flags = src.fgi[pos->fg_idx].texture_flags;
		out1.fgi[pos->fg_idx].emissive_texture_id = src.fgi[pos->fg_idx].emissive_texture_id;
		out1.fgi[pos->fg_idx].emissive_texture_flags = src.fgi[pos->fg_idx].emissive_texture_flags;
		out1.fgi[pos->fg_idx].unique_id = src.fgi[pos->fg_idx].unique_id;*/
		out0.fgi[pos->fg_idx] = src.fgi[pos->fg_idx];
		out1.fgi[pos->fg_idx] = src.fgi[pos->fg_idx];
	}

	SplitMesh(src,out0,out1,split_d,split_n);
}
//-----------------------------------------------------------------------------
//--------------------------End Meshinfo->cpp-----------------------------------
//-----------------------------------------------------------------------------