//--------------------------------------------------------------------------//
//                                                                          //
//                              MeshArch.CPP                                //
//                                                                          //
//               COPYRIGHT (C) 2004 By Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//

#include "stdafx.h"
#include <IMaterialManager.h>
#include "IInternalMeshManager.h"
#include "MeshArch.h"
#include <Renderer.h>
#include <rendpipeline.h>
#include <mesh.h>
#include <IHardpoint.h>
#include <IChannel2.h>
#include <ChannelEventTypes.h>
#include "MeshInstance.h"

MeshArch::MeshArch(ARCHETYPE_INDEX _archIndex, SCRIPT_SET_ARCH _animArchIndex)
{
	owner = 0;
	next = NULL;
	refCount = 1;

	archIndex = _archIndex ;
	animArchIndex = _animArchIndex;

	vertexBuffers = NULL;
	num_verts = 0;
	allocatedVerts = 0;
	vertexList = NULL;
	
	faceArray = 0;
	numFaceGroups = 0;

	bDynamic = false;
	dynamicMaterial = NULL;

	bRef = false;
	bRefOverrideVerts = false;
	bRefOverrideFaces = false;
	refArch = NULL;
}

MeshArch::MeshArch(IMaterial * mat, U32 numVerts)//dynamic mesh contructor
{
	owner = 0;
	next = NULL;
	refCount = 1;

	archIndex = INVALID_ARCHETYPE_INDEX ;
	animArchIndex = INVALID_SCRIPT_SET_ARCH;

	vertexBuffers = NULL;
	num_verts = numVerts;
	vertexList = NULL;
	
	faceArray = 0;
	numFaceGroups = 0;

	bDynamic = true;
	dynamicMaterial = mat;

	bRef = false;
	bRefOverrideVerts = false;
	bRefOverrideFaces = false;
	refArch = NULL;
}

MeshArch::MeshArch(MeshArch * refSource)//reference arch
{
	owner = 0;
	next = NULL;
	refCount = 1;

	archIndex = INVALID_ARCHETYPE_INDEX ;
	animArchIndex = INVALID_SCRIPT_SET_ARCH;

	vertexBuffers = NULL;
	num_verts = 0;
	vertexList = NULL;
	
	faceArray = 0;
	numFaceGroups = 0;

	bDynamic = false;
	dynamicMaterial = NULL;

	bRef = true;
	bRefOverrideVerts = false;
	bRefOverrideFaces = false;
	refArch = refSource;
	refArch->AddRef();
}

MeshArch::~MeshArch()
{
	if(!bRef || bRefOverrideVerts)
	{
		while(vertexBuffers)
		{
			VertexBufferNode * tmp = vertexBuffers;
			vertexBuffers = vertexBuffers->next;
			owner->GetPipe()->destroy_vertex_buffer(tmp->vertexBuffer);
			delete tmp;
		}

		if(vertexList)
		{
			delete [] vertexList;
			vertexList = NULL;
		}
	}

	if(!bRef || bRefOverrideFaces)
	{
		for(U32 i = 0; i < numFaceGroups; ++i)
		{
			if(faceArray[i].indexBuffer)
			{
				faceArray[i].indexBuffer->Release();
				faceArray[i].indexBuffer = NULL;
			}
		}

		if(faceArray)
		{
			delete [] faceArray;
		}
	}
	if(refArch)
	{
		refArch->Release();
		refArch = NULL;
	}
}

ARCHETYPE_INDEX MeshArch::GetEngineArchtype()
{
	if(bRef)
		return refArch->GetEngineArchtype();
	return archIndex;
}

SCRIPT_SET_ARCH MeshArch::GetAnimArchtype()
{
	if(bRef)
		return refArch->GetAnimArchtype();
	return animArchIndex;
}

struct HpSearch
{
	bool bNextHpEnum:1;
	bool bFoundHpEnum:1;
	bool bFindExactEnumFromIndex:1;
	bool bFindExactEnumFromName:1;
	const char * hpEnumName;
	U32 currentHpIndex;
	U32 lastHpIndex;

	ARCHETYPE_INDEX foundArch;

	MeshArch * arch;

	HpSearch(MeshArch * _arch)
	{
		arch = _arch;
		bNextHpEnum = false;
		bFoundHpEnum = false;
		bFindExactEnumFromIndex = false;
		bFindExactEnumFromName = false;
		hpEnumName = NULL;
		currentHpIndex = 0;
		lastHpIndex = 0;
		foundArch = INVALID_ARCHETYPE_INDEX;
	}
};

void hpEnum(const char* hardpoint_name, void* misc)
{
	HpSearch * search = (HpSearch*) misc;
	if(!search->bFoundHpEnum)
	{
		if(search->bNextHpEnum)
		{
			search->bFoundHpEnum = true;
			search->hpEnumName = hardpoint_name;
			return;
		}
		else if(search->bFindExactEnumFromIndex)
		{
			if(search->lastHpIndex == search->currentHpIndex)
			{
				search->bFoundHpEnum = true;
				search->hpEnumName = hardpoint_name;
				return;
			}
		}
		else if(search->bFindExactEnumFromName)
		{
			if(strcmp(search->hpEnumName,hardpoint_name) == 0)
			{
				search->bFoundHpEnum = true;
				search->hpEnumName = hardpoint_name;
				return;
			}
		}
		else
		{
			if(search->lastHpIndex == search->currentHpIndex)
			{
				search->bNextHpEnum = true;
			}
		}
		++(search->currentHpIndex);
	}
}

bool enumArchForHP( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data )
{
	HpSearch * search = (HpSearch*) user_data;
	if(search->arch->findHardpoint(search,child_arch_index))
		return false;
	return true;
}

bool MeshArch::findHardpoint(HpSearch * search, ARCHETYPE_INDEX baseIndex)
{
	search->foundArch = baseIndex;
	owner->GetHardpoint()->enumerate_hardpoints(hpEnum,baseIndex,search);
	if(search->bFoundHpEnum)
        return true;

	owner->GetEngine()->enumerate_archetype_parts(baseIndex,enumArchForHP,(void*)(search));
	return search->bFoundHpEnum;
};

bool MeshArch::FindFirstHardpoint(HardPointDef & hp)
{
	if(bRef)
		return refArch->FindFirstHardpoint(hp);
	HpSearch search(this);
	search.bNextHpEnum = true;

	if(findHardpoint(&search,archIndex))
	{
		strncpy(hp.name,search.hpEnumName,31);
		hp.name[31] = 0;
		hp.index = search.currentHpIndex;
		return true;
	}
	return false;
}

bool MeshArch::FindNextHardpoint(HardPointDef & hp)
{
	if(bRef)
		return refArch->FindNextHardpoint(hp);
	HpSearch search(this);
	search.lastHpIndex = hp.index;

	if(findHardpoint(&search,archIndex))
	{
		strncpy(hp.name,search.hpEnumName,31);
		hp.name[31] = 0;
		hp.index = search.currentHpIndex;
		return true;
	}
	return false;
}

U32 MeshArch::FindHardPointIndex(const char * name)
{
	if(bRef)
		return refArch->FindHardPointIndex(name);
	HpSearch search(this);
	search.bFindExactEnumFromName = true;
	search.hpEnumName = name;
	if(findHardpoint(&search,archIndex))
	{
		return search.currentHpIndex;
	}
	return INVALID_HARD_POINT;
}

bool MeshArch::FindHardPontFromIndex(U32 index,HardPointDef & hp)
{
	if(bRef)
		return refArch->FindHardPontFromIndex(index,hp);
	HpSearch search(this);
	search.bFindExactEnumFromIndex = true;
	search.lastHpIndex = index;
	if(findHardpoint(&search,archIndex))
	{
		strncpy(hp.name,search.hpEnumName,31);
		hp.name[31] = 0;
		hp.index = search.currentHpIndex;
		hp.archIndex = search.foundArch;
		return true;
	}
	return false;
}

bool bNextAnimEnum = false;
bool bFoundAnimEnum = false;
const char * animEnumName = NULL;
U32 currentAnimIndex = 0;
U32 lastAnimIndex = 0;

void enumAnimScripts(const char* script_name, void* misc)
{
	if(!bFoundAnimEnum)
	{
		if(bNextAnimEnum)
		{
			bFoundAnimEnum = true;
			animEnumName = script_name;
			return;
		}
		else
		{
			if(lastAnimIndex == currentAnimIndex)
			{
				bNextAnimEnum = true;
			}
		}
		++currentAnimIndex;
	}
}

bool MeshArch::GetFirstAnimScript(AnimScriptEntry & entry)
{
	if(bRef)
		return refArch->GetFirstAnimScript(entry);
	if(animArchIndex == INVALID_SCRIPT_SET_ARCH)
		return false;
	bNextAnimEnum = true;
	bFoundAnimEnum = false;
	currentAnimIndex = 0;
	lastAnimIndex = 0;
	owner->GetAnim()->enumerate_scripts(enumAnimScripts,animArchIndex,NULL);
	if(bFoundAnimEnum)
	{
		strncpy(entry.name,animEnumName,31);
		entry.index = currentAnimIndex;
		entry.name[31] = 0;
	}
	return bFoundAnimEnum;
}

bool MeshArch::GetNextAnimScript(AnimScriptEntry & entry)
{
	if(bRef)
		return refArch->GetNextAnimScript(entry);
	if(animArchIndex == INVALID_SCRIPT_SET_ARCH)
		return false;
	bNextAnimEnum = false;
	bFoundAnimEnum = false;
	lastAnimIndex = entry.index;
	currentAnimIndex = 0;
	owner->GetAnim()->enumerate_scripts(enumAnimScripts,animArchIndex,NULL);
	if(bFoundAnimEnum)
	{
		strncpy(entry.name,animEnumName,31);
		entry.name[31] = 0;
		entry.index = currentAnimIndex;
	}
	return bFoundAnimEnum;
}

bool MeshArch::GetFirstAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue)
{
	if(bRef)
		return refArch->GetFirstAnimCue(entry,cue);
	if(animArchIndex == INVALID_SCRIPT_SET_ARCH)
		return false;

	IChannel2 * channel = findEventChannel(entry.name);
	if(channel)
	{
		EventIterator::Event events[1];
		U32 numEvents = 0;
		SINGLE durration = GetAnimationDurration(entry.name);
		channel->get_events_at_time(0,events,1,numEvents);
		if(!numEvents)
			channel->get_events(0,durration,events,1,numEvents);
		if(!numEvents)
			channel->get_events_at_time(durration,events,1,numEvents);

		channel->Release();
		if(numEvents > 0)
		{
			cue.time = events[0].time;
			cue.index = 0;
			strcpy(cue.name,"NoName");
			switch(*((int*)(events[0].data)))
			{
			case NAMED_EVENT:
				strncpy(cue.name,(((char *)(events[0].data))+sizeof(int)),31); 
				cue.name[31] = 0;		
				break;
			case CHANNEL_END:
				strcpy(cue.name,"End Tag");
				break;
			case CHANNEL_BEGIN:
				strcpy(cue.name,"Begin Tag");
				break;
			}
			return true;
		}
	}
	return false;
}

bool MeshArch::GetNextAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue)
{
	if(bRef)
		return refArch->GetNextAnimCue(entry,cue);
	if(animArchIndex == INVALID_SCRIPT_SET_ARCH)
		return false;

	IChannel2 * channel = findEventChannel(entry.name);
	if(channel)
	{
		EventIterator::Event events[20];
		U32 maxEvents = 20;
		U32 numEvents = 0;
		SINGLE durration = 0;
		channel->get_duration(durration);
		channel->get_events_at_time(0,events,maxEvents,numEvents);
		U32 totalEvents = numEvents;
		channel->get_events(0,durration,events+totalEvents,maxEvents-totalEvents,numEvents);
		totalEvents+=numEvents;
		channel->get_events_at_time(durration,events+totalEvents,maxEvents-totalEvents,numEvents);
		totalEvents+=numEvents;

		channel->Release();
		if(totalEvents > cue.index+1)
		{
			cue.time = events[cue.index+1].time;
			strcpy(cue.name,"NoName");
			switch(*((int*)(events[cue.index+1].data)))
			{
			case NAMED_EVENT:
				strncpy(cue.name,(((char *)(events[cue.index+1].data))+sizeof(int)),31); 
				cue.name[31] = 0;		
				break;
			case CHANNEL_END:
				strcpy(cue.name,"End Tag");
				break;
			case CHANNEL_BEGIN:
				strcpy(cue.name,"Begin Tag");
				break;
			}
			cue.index += 1;
			return true;
		}
	}
	return false;
}

SINGLE MeshArch::GetAnimationDurration(const char * animName)
{
	if(bRef)
		return refArch->GetAnimationDurration(animName);
	if(animArchIndex == INVALID_SCRIPT_SET_ARCH)
		return 0;
	return owner->GetAnim()->get_duration(animArchIndex,animName);
}

U32 MeshArch::GetNumFaceGroups()
{
	if(bRef && (!bRefOverrideFaces))
		return refArch->GetNumFaceGroups();
	return numFaceGroups;
}

IMaterial * MeshArch::GetFaceGroupMaterial(U32 fgIndex)
{
	if(bRef && (!bRefOverrideFaces))
		return refArch->GetFaceGroupMaterial(fgIndex);
	return faceArray[fgIndex].mat;
}

void MeshArch::ResetRef()
{
	if(bRefOverrideFaces)
	{
		bRefOverrideFaces = false;
		for(U32 i = 0; i < numFaceGroups; ++i)
		{
			if(faceArray[i].indexBuffer)
			{
				faceArray[i].indexBuffer->Release();
				faceArray[i].indexBuffer = NULL;
			}
		}
		if(faceArray)
		{
			delete [] faceArray;
			faceArray = NULL;
		}
	}
	if(bRefOverrideVerts)
	{
		bRefOverrideVerts = false;
		while(vertexBuffers)
		{
			VertexBufferNode * tmp = vertexBuffers;
			vertexBuffers = vertexBuffers->next;
			owner->GetPipe()->destroy_vertex_buffer(tmp->vertexBuffer);
			delete tmp;
		}

		if(vertexList)
		{
			delete [] vertexList;
			vertexList = NULL;
		}
	}
	vertexList = refArch->vertexList;
	faceArray = refArch->faceArray;
	num_verts = refArch->num_verts;
	numFaceGroups = refArch->numFaceGroups;
}

void MeshArch::MeshOperationSlice(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh)
{
	//this operation is intended to be used on a reference object with it's own instance

	//a Slice Operation will cut up the mesh allong an object space axis
		//The cut will make new face groups
		//The cut will fix up edge polygons to be straight.
	Transform worldTrans = targetMesh->GetTransform();
	Transform ident;
	ident.set_identity();
	targetMesh->SetTransform(ident);
	targetMesh->Update(0);

	sliceDir.fast_normalize();
	SINGLE radius = targetMesh->GetBoundingRadius();
	Vector maxPoint = sliceDir*radius;
	Vector cutPoint = sliceDelta*(maxPoint+maxPoint)-maxPoint;

	//first split the face group up into keep and no keep
	if(bRef && (!bRefOverrideFaces))
		overrideFaces();

	if(bRef && (!bRefOverrideVerts))
		overrideVerts();

	MeshVertex * verts = vertexList;

	MeshInstance * tMesh = (MeshInstance *)targetMesh;
	
	for(U32 fg = 0; fg < numFaceGroups;++fg)
	{
		U16* readInd;
		faceArray[fg].indexBuffer->Lock(0,0,(void**)&readInd,0);
		U32 writeIndex = 0;
		Transform faceTrans = owner->GetEngine()->get_transform(tMesh->faceInstance[fg]);
		Transform faceInv = faceTrans.get_inverse();
		for(U32 index = 0; index < faceArray[fg].numIndex; index = index+3)
		{
			Vector pos1 = verts[readInd[index]].pos;
			Vector pos2 = verts[readInd[index+1]].pos;
			Vector pos3 = verts[readInd[index+2]].pos;

			pos1 = faceTrans.rotate_translate(pos1);
			pos2 = faceTrans.rotate_translate(pos2);
			pos3 = faceTrans.rotate_translate(pos3);

			bool bPointGood1 = true;
			bool bPointGood2 = true;
			bool bPointGood3 = true;

			Vector dirTest = pos1-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				bPointGood1 = false;

			dirTest = pos2-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				bPointGood2 = false;
			
			dirTest = pos3-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				bPointGood3 = false;

			if(bPointGood1 || bPointGood2 || bPointGood3)
			{
				//at least one point is good
				if(writeIndex != index)
				{
					readInd[writeIndex] = readInd[index];
					readInd[writeIndex+1] = readInd[index+1];
					readInd[writeIndex+2] = readInd[index+2];
				}
				writeIndex += 3;

				//smoth out points
				if(!bPointGood1)
				{
					Vector point;
					if(bPointGood2)
					{
						// 1 to 2
						SINGLE s = dot_product(sliceDir,(cutPoint-pos1)/dot_product(sliceDir,pos2-pos1));
						point = (s*(pos2-pos1))+pos1+(sliceDir*0.01);
					}
					else
					{
						//1 to 3
						SINGLE s = dot_product(sliceDir,(cutPoint-pos1)/dot_product(sliceDir,pos3-pos1));
						point = (s*(pos3-pos1))+pos1+(sliceDir*0.01);
					}
					verts[readInd[index]].pos = faceInv.rotate_translate(point);
				}
				if(!bPointGood2)
				{
					Vector point;
					if(bPointGood3)
					{
						// 2 to 3
						SINGLE s = dot_product(sliceDir,(cutPoint-pos2)/dot_product(sliceDir,pos3-pos2));
						point = (s*(pos3-pos2))+pos2+(sliceDir*0.01);
					}
					else
					{
						//2 to 1
						SINGLE s = dot_product(sliceDir,(cutPoint-pos2)/dot_product(sliceDir,pos1-pos2));
						point = (s*(pos1-pos2))+pos2+(sliceDir*0.01);
					}
					verts[readInd[index+1]].pos = faceInv.rotate_translate(point);
				}
				if(!bPointGood3)
				{
					Vector point;
					if(bPointGood1)
					{
						// 3 to 1
						SINGLE s = dot_product(sliceDir,(cutPoint-pos3)/dot_product(sliceDir,pos1-pos3));
						point = (s*(pos1-pos3))+pos3+(sliceDir*0.01);
					}
					else
					{
						//3 to 2
						SINGLE s = dot_product(sliceDir,(cutPoint-pos3)/dot_product(sliceDir,pos2-pos3));
						point = (s*(pos2-pos3))+pos3+(sliceDir*0.01);
					}
					verts[readInd[index+2]].pos = faceInv.rotate_translate(point);
				}
			}
		}
		faceArray[fg].numIndex = writeIndex;
		faceArray[fg].indexBuffer->Unlock();
	}

	InvalidateBuffers();

	targetMesh->SetTransform(worldTrans);
	targetMesh->Update(0);
}

void MeshArch::MeshOperationCut(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh)
{
	//this operation is intended to be used on a reference object with it's own instance

	//a cut Operation will cut up the mesh allong an object space axis
		//The cut will make new face groups

	sliceDir.fast_normalize();
	SINGLE radius = targetMesh->GetBoundingRadius();
	Vector maxPoint = sliceDir*radius;
	Vector cutPoint = sliceDelta*(maxPoint+maxPoint)-maxPoint;

	//first split the face group up into keep and no keep
	if(bRef && (!bRefOverrideFaces))
		overrideFaces();

	MeshVertex * verts = vertexList;
	if(bRef && (!bRefOverrideVerts))
	{
		verts = refArch->vertexList;
	}

	MeshInstance * tMesh = (MeshInstance *)targetMesh;
	
	for(U32 fg = 0; fg < numFaceGroups;++fg)
	{
		U16* readInd;
		faceArray[fg].indexBuffer->Lock(0,0,(void**)&readInd,0);
		U32 writeIndex = 0;
		Transform faceTrans = owner->GetEngine()->get_transform(tMesh->faceInstance[fg]);
		for(U32 index = 0; index < faceArray[fg].numIndex; index = index+3)
		{
			Vector pos1 = verts[readInd[index]].pos;
			Vector pos2 = verts[readInd[index+1]].pos;
			Vector pos3 = verts[readInd[index+2]].pos;

			pos1 = faceTrans.rotate_translate(pos1);
			pos2 = faceTrans.rotate_translate(pos2);
			pos3 = faceTrans.rotate_translate(pos3);

			U32 outCount = 0;

			Vector dirTest = pos1-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				++outCount;

			dirTest = pos2-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				++outCount;
			
			dirTest = pos3-cutPoint;
			dirTest.fast_normalize();
			if(dot_product(dirTest,sliceDir) > 0)
				++outCount;

			if(outCount < 2)
			{
				//at least two points are good will give us a good break
				if(writeIndex != index)
				{
					readInd[writeIndex] = readInd[index];
					readInd[writeIndex+1] = readInd[index+1];
					readInd[writeIndex+2] = readInd[index+2];
				}
				writeIndex += 3;
			}
		}
		faceArray[fg].numIndex = writeIndex;
		faceArray[fg].indexBuffer->Unlock();
	}
}

void MeshArch::AddRef()
{
	++refCount;
}

void MeshArch::Release()
{
	owner->ReleaseArch(this);//can cause deletion, carfull
}

void MeshArch::ReinitDynamic(IMaterial * mat, U32 numVerts)
{
	num_verts = numVerts;
	dynamicMaterial = mat;

	faceArray[0].mat = dynamicMaterial;
	faceArray[0].numVerts= num_verts;
}

MeshFace * MeshArch::GetFaceArray()
{
	if(bRef && (!bRefOverrideFaces))
	{
		return refArch->GetFaceArray();
	}
	return faceArray;
}

U32 MeshArch::GetNumVerts()
{
	if(bRef && (!bRefOverrideVerts))
	{
		return refArch->GetNumVerts();
	}
	return num_verts;
}

MeshVertex * MeshArch::GetVertexList()
{
	if(bRef && (!bRefOverrideVerts))
	{
		return refArch->GetVertexList();
	}
	return vertexList;
}

U32 MeshArch::GetArchIndex()
{
	if(bRef)
	{
		return refArch->GetArchIndex();
	}
	return archIndex;
}

void MeshArch::Realize()
{
	if(bDynamic)
	{
		if(!vertexList)
		{
			allocatedVerts = num_verts;
			vertexList = new MeshVertex[allocatedVerts];
		}

		if(!faceArray)
		{
			numFaceGroups = 1;
			faceArray = new MeshFace[1];
		}

		faceArray[0].indexBuffer = NULL;
		faceArray[0].mat = dynamicMaterial;
		faceArray[0].numIndex = 0;
		faceArray[0].startIndex = 0;
		faceArray[0].startVertex = 0;
		faceArray[0].numVerts= num_verts;
		if(dynamicMaterial)
			dynamicMaterial->Realize();
	}
	else if(bRef)
	{
		vertexList = refArch->vertexList;
		faceArray = refArch->faceArray;
		num_verts = refArch->num_verts;
		numFaceGroups = refArch->numFaceGroups;
	}
	else
	{
		buildFullMesh();
	}
}

void MeshArch::InvalidateBuffers()
{
	if(bRef && (!bRefOverrideVerts))
		return refArch->InvalidateBuffers();
	VertexBufferNode * search = vertexBuffers;
	while(search)
	{
		search->bValid = false;
		search = search->next;
	}
}

U32 MeshArch::GetVertexBufferForType(CQ_VertexType vType)
{
	if(bRef && (!bRefOverrideVerts))
		return refArch->GetVertexBufferForType(vType);
	VertexBufferNode * search = vertexBuffers;
	while(search)
	{
		if(search->vType == vType)
		{
			if(search->bValid)
				return search->vertexBuffer;
			else
				break;
		}
		search = search->next;
	}

	VertexBufferNode * newNode = search;
	if(!newNode)
	{
		newNode = new VertexBufferNode;
		newNode->vType = vType;
		newNode->next = vertexBuffers;
		newNode->vertexBuffer = NULL;
		vertexBuffers = newNode;
	}

	switch(vType)
	{
	case VT_STANDARD:
		{
			if(!(newNode->vertexBuffer))
				owner->GetPipe()->create_vertex_buffer(D3DFVF_STANDARD,num_verts,0,&(newNode->vertexBuffer));

			StandardVertex *vbuffer;
			U32 size;
			owner->GetPipe()->lock_vertex_buffer(newNode->vertexBuffer,DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS,(void **)&vbuffer,&size);

			for(U32 i = 0; i < num_verts; ++i)
			{
				vbuffer[i].pos = vertexList[i].pos;
				vbuffer[i].norm = vertexList[i].normal;
				vbuffer[i].r = vertexList[i].red;
				vbuffer[i].g = vertexList[i].green;
				vbuffer[i].b = vertexList[i].blue;
				vbuffer[i].a = vertexList[i].alpha;
				vbuffer[i].u = vertexList[i].u;
				vbuffer[i].v = vertexList[i].v;
				vbuffer[i].u2 = vertexList[i].u2;
				vbuffer[i].v2 = vertexList[i].v2;
				vbuffer[i].binX = vertexList[i].binormal.x;
				vbuffer[i].binY = vertexList[i].binormal.y;
				vbuffer[i].binZ = vertexList[i].binormal.z;
				vbuffer[i].tanX = vertexList[i].tangent.x;
				vbuffer[i].tanY = vertexList[i].tangent.y;
				vbuffer[i].tanZ = vertexList[i].tangent.z;
			}

			owner->GetPipe()->unlock_vertex_buffer(newNode->vertexBuffer);

			newNode->bValid = true;

			return newNode->vertexBuffer;
		}
		break;
	}
	return 0;
}

MeshVertex * decodeArray = 0;
U32 decodeArraySize = 0;
U32 lastDecodeIndex = 0;

bool bHasBox;
Vector minBox;
Vector maxBox;

void initDecodeArray()
{
	if(decodeArraySize == 0)
	{
		decodeArraySize = 2000;
		decodeArray = new MeshVertex[decodeArraySize];
	}

	lastDecodeIndex  = 0;
}

U16 addDecodedVertex(const Vector & pos,const Vector & norm,const SINGLE & u,const SINGLE & v,const SINGLE & u2,const SINGLE & v2)
{
	if(!bHasBox)
	{
		bHasBox = true;
		minBox = pos;
		maxBox = pos;
	}
	else
	{
		minBox.x = __min(minBox.x,pos.x);
		minBox.y = __min(minBox.y,pos.y);
		minBox.z = __min(minBox.z,pos.z);
		maxBox.x = __max(maxBox.x,pos.x);
		maxBox.y = __max(maxBox.y,pos.y);
		maxBox.z = __max(maxBox.z,pos.z);
	}

	//this can be spee up by usingthe fact that the positions all have the same reference number in the calling function
	for(U32 i = 0; i < lastDecodeIndex; ++i)
	{
		MeshVertex & vert = decodeArray[i];
		if(vert.pos == pos && vert.normal == norm && vert.u == u && vert.v == v && vert.u2 == u2 && vert.v2 == v2)
		{
			return i;
		}
	}

	if(lastDecodeIndex == decodeArraySize)
	{
		MeshVertex * newarray = new MeshVertex[decodeArraySize+1000];
		memcpy(newarray,decodeArray,sizeof(MeshVertex)*decodeArraySize);
		decodeArraySize = decodeArraySize+1000;
		delete [] decodeArray;
		decodeArray = newarray;
	}

	decodeArray[lastDecodeIndex].pos = pos;
	decodeArray[lastDecodeIndex].normal = norm;
	decodeArray[lastDecodeIndex].u = u;
	decodeArray[lastDecodeIndex].v = v;
	decodeArray[lastDecodeIndex].u2 = u2;
	decodeArray[lastDecodeIndex].v2 = v2;
	decodeArray[lastDecodeIndex].binormal = Vector(0,0,0);
	decodeArray[lastDecodeIndex].tangent = Vector(0,0,0);
	decodeArray[lastDecodeIndex].red = 255;
	decodeArray[lastDecodeIndex].green = 255;
	decodeArray[lastDecodeIndex].blue = 255;
	decodeArray[lastDecodeIndex].alpha = 255;
	U16 retVal = lastDecodeIndex;
	lastDecodeIndex++;
	return retVal;
}

void MeshArch::buildFullMesh()
{
	if(archIndex == INVALID_ARCHETYPE_INDEX)
		return;

	num_verts = 0;
	initDecodeArray();

	numFaceGroups = computeNumFaceGroups();
	faceArray = new MeshFace[numFaceGroups];

	U32 faceOffset = 0;
	buildMeshForIndex(archIndex,faceOffset);

	if(lastDecodeIndex)
	{
		//renormalize the tangent and binormal
		for(U32 i = 0; i < lastDecodeIndex; ++i)
		{
			decodeArray[i].binormal.fast_normalize();
			decodeArray[i].tangent.fast_normalize();
		}
		vertexList = new MeshVertex[lastDecodeIndex];
		memcpy(vertexList,decodeArray,sizeof(MeshVertex)*lastDecodeIndex);
		num_verts = lastDecodeIndex;
	}
}

struct EnumBuildInfo
{
	MeshArch * arch;
	U32 & faceOffset;

	EnumBuildInfo(MeshArch * _arch,U32 & _faceOffset):arch(_arch),faceOffset(_faceOffset)
	{
	};
};

bool enumCountFaceGroups( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data )
{
	EnumBuildInfo * info = (EnumBuildInfo *)user_data;

	Mesh * archMesh = info->arch->owner->GetRenderer()->get_archetype_mesh(child_arch_index,0);

	if(archMesh)
	{
		info->faceOffset += archMesh->face_group_cnt;

		info->arch->owner->GetEngine()->enumerate_archetype_parts(child_arch_index,enumCountFaceGroups,user_data);
	}

	return true;
}

U32 MeshArch::computeNumFaceGroups()
{
	U32 count = 0;

	Mesh * archMesh = owner->GetRenderer()->get_archetype_mesh(archIndex,0);
	if(archMesh)
		count += archMesh->face_group_cnt;

	EnumBuildInfo info(this,count);

	owner->GetEngine()->enumerate_archetype_parts(archIndex,enumCountFaceGroups,(void*)(&info));

	return count;
}


bool enumBuildMesh( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data )
{
	EnumBuildInfo * info = (EnumBuildInfo *)user_data;

	info->arch->buildMeshForIndex(child_arch_index,info->faceOffset);

	return true;
}

void MeshArch::buildMeshForIndex(U32 baseIndex,U32 & faceOffset)
{
	Mesh * archMesh = owner->GetRenderer()->get_archetype_mesh(baseIndex,0);
	if(archMesh)
	{
		for(S32 face_group_index = 0; face_group_index < archMesh->face_group_cnt; ++ face_group_index)
		{
			bHasBox = false;

			U32 fullFaceIndex = faceOffset+face_group_index;
			FaceGroup * fg = &(archMesh->face_groups[face_group_index]);

			if(archMesh->imaterial_list)
				faceArray[fullFaceIndex].mat = archMesh->imaterial_list[fg->material];
			else 
				faceArray[fullFaceIndex].mat = NULL;
			faceArray[fullFaceIndex].archIndex = baseIndex;

			owner->GetPipe()->create_index_buffer(fg->face_cnt*3*sizeof(U16),&(faceArray[fullFaceIndex].indexBuffer));
			U16* lockedInds;
			faceArray[fullFaceIndex].indexBuffer->Lock(0,0,(void**)&lockedInds,0);

			for(S32 face_index = 0; face_index < fg->face_cnt; ++ face_index)
			{
				SINGLE uTex[3];
				SINGLE vTex[3];
				U16 triIndex[3];

				for(U32 vert_index = 0; vert_index < 3; ++vert_index)
				{
					U32 absVertIndex = face_index*3+vert_index;
					U32 vertexRef = archMesh->vertex_batch_list[fg->face_vertex_chain[absVertIndex]];
					Vector &pos = archMesh->object_vertex_list[vertexRef];

					SINGLE u2,v2;
					U32 textureRef = archMesh->texture_batch_list[fg->face_vertex_chain[absVertIndex]];
					SINGLE & u = archMesh->texture_vertex_list[textureRef].u;
					uTex[vert_index] = u;
					SINGLE v = 1.0-archMesh->texture_vertex_list[textureRef].v;
					vTex[vert_index] = v;
					if(archMesh->texture_batch_list2)
					{
						U32 textureRef2 = archMesh->texture_batch_list2[fg->face_vertex_chain[absVertIndex]];
						u2 = archMesh->texture_vertex_list[textureRef2].u;
						v2 = archMesh->texture_vertex_list[textureRef2].v;
					}
					else
					{
						u2 = 0;
						v2 = 0;
					}

					Vector norm;
					if (fg->face_properties[face_index] & FLAT_SHADED)
						norm = archMesh->normal_ABC[fg->face_normal[face_index]];
					else
						norm = archMesh->normal_ABC[archMesh->vertex_normal[vertexRef]];

					U16 ind = addDecodedVertex(pos,norm,u,v,u2,v2);
					triIndex[vert_index] = lockedInds[absVertIndex] = ind;
				}
				
				Vector UV1, UV2, UV0;
				Vector du,dv;

				UV0.x = uTex[0];
				UV0.y = vTex[0];
				UV1.x = uTex[1];
				UV1.y = vTex[1];
				UV2.x = uTex[2];
				UV2.y = vTex[2];

				MeshVertex & v0 = decodeArray[triIndex[0]];
				MeshVertex & v1 = decodeArray[triIndex[1]];
				MeshVertex & v2 = decodeArray[triIndex[2]];

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

				v0.tangent += du;
				v1.tangent += du;
				v2.tangent += du;

				v0.binormal += dv;
				v1.binormal += dv;
				v2.binormal += dv;
			}
			faceArray[fullFaceIndex].indexBuffer->Unlock();
			faceArray[fullFaceIndex].numVerts = lastDecodeIndex;
			faceArray[fullFaceIndex].startVertex = 0;
			faceArray[fullFaceIndex].startIndex = 0;
			faceArray[fullFaceIndex].numIndex = fg->face_cnt*3;
			faceArray[fullFaceIndex].maxBox = maxBox;
			faceArray[fullFaceIndex].minBox = minBox;
		}

		faceOffset += archMesh->face_group_cnt;
	}

	//loop through each child and create it as well

	EnumBuildInfo info(this,faceOffset);
	owner->GetEngine()->enumerate_archetype_parts(baseIndex,enumBuildMesh,(void*)(&info));
}

void channelEnum(IChannel2* obj, void* user)
{
	IChannel2 ** search = ((IChannel2 **)user);
	if(*search)
		return;
	U32 type;
	obj->get_data_type(type);
	if(type == DT_EVENT)
	{
		(*search) = obj;
		return;
	}
	obj->Release();
}

struct IChannel2 * MeshArch::findEventChannel(char * animName)
{
	IChannel2 * search = NULL;
	owner->GetAnim()->enumerate_channels(channelEnum,animArchIndex,animName,(void*)(&search));
	return search;
}

void MeshArch::overrideFaces()
{
	if(bRef && (!bRefOverrideFaces))
	{
		bRefOverrideFaces = true;
		numFaceGroups = refArch->numFaceGroups;
		if(numFaceGroups)
		{
			faceArray = new MeshFace[numFaceGroups];
			for(U32 index = 0; index < numFaceGroups; ++index)
			{
				faceArray[index].archIndex = refArch->faceArray[index].archIndex;
				faceArray[index].mat = refArch->faceArray[index].mat;
				faceArray[index].maxBox = refArch->faceArray[index].maxBox;
				faceArray[index].minBox = refArch->faceArray[index].minBox;
				faceArray[index].numIndex = refArch->faceArray[index].numIndex;
				faceArray[index].numVerts = refArch->faceArray[index].numVerts;
				faceArray[index].startIndex = refArch->faceArray[index].startIndex;
				faceArray[index].startVertex = refArch->faceArray[index].startVertex;

				owner->GetPipe()->create_index_buffer(faceArray[index].numIndex*sizeof(U16),&(faceArray[index].indexBuffer));
				U16* lockedIndsDest;
				U16* lockedIndsSrc;
				faceArray[index].indexBuffer->Lock(0,0,(void**)&lockedIndsDest,0);
				refArch->faceArray[index].indexBuffer->Lock(0,0,(void**)&lockedIndsSrc,0);
				memcpy(lockedIndsDest,lockedIndsSrc,faceArray[index].numIndex*sizeof(U16));
				refArch->faceArray[index].indexBuffer->Unlock();
				faceArray[index].indexBuffer->Unlock();			
			}
		}
	}
}

void MeshArch::overrideVerts()
{
	if(bRef && (!bRefOverrideVerts))
	{
		bRefOverrideVerts = true;
		num_verts = refArch->num_verts;
		vertexList = new MeshVertex[num_verts];
		memcpy(vertexList,refArch->vertexList,num_verts*sizeof(MeshVertex));
	}
}


