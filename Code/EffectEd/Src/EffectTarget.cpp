//EffectTarget.cpp

#include "stdafx.h"
#include "globals.h"
#include "IEffectTarget.h"
#include "ITargetHp.h"
#include "ITargetAnim.h"
#include "SuperTrans.h"
#include "ITargetCue.h"
#include "IEffectAction.h"
#include "IEffectFile.h"
#include "Camera.h"
#include "Archlist.h"
#include "InfoArea.h"
#include "MyVertex.h"
#include "Previewwin.h"
#include <IMeshManager.h>

#include <DACOM.h>
#include <DACOM.h>
#include <filesys.h>
#include <TSmartPointer.h>
#include <DLight.h>

#include <stdio.h>

#define DEFAULT_SIZE (1000.0*FEET_TO_WORLD)

#define EFFECT_TARGET_SAVE_VERSION 1

struct EffectTargetSave
{
	U32 version;
	char name[64];
	char meshfile[256];
	U32 instID;
	U32 numHardpoints;
	U32 numAnim;
	Transform trans;
};

struct MovieLight
{
	MovieLight * next;
//	IRenderLight * light;
	U32 bone;
};

struct ActionNode
{
	ActionNode * next;
	IEffectAction * action;
};

struct EffectTarget: public IEffectTarget, IMeshCallback
{
	IEffectTarget * next;
	char name[64];

	ITargetHp * firstHardPoint;
	ITargetAnim * firstAnim;

	IMeshInstance * mesh;

	char meshFile[256];

	bool bSelected:1;
	bool bHide:1;

	IEffectAction * currentAction;
	ActionNode * updateList;

	U32 instID;

	//preview data
	TRANSFORM trans;

	MovieLight * lightList;

	EffectTarget();

	~EffectTarget();

	//IEffectTarget
	virtual IEffectTarget * GetNextTarget();

	virtual void SetNextTarget(IEffectTarget * target);

	virtual const char * GetName();

	virtual void SetName(const char * newName);

	virtual struct ITargetHp * GetFirstHardpoint();

	virtual struct ITargetHp * AddHardpoint(const char * newName);

	virtual void RemoveTargetHp(ITargetHp * target);

	virtual struct ITargetAnim * GetFirstAnim();

	virtual struct ITargetAnim * AddAnim(const char * newName);

	virtual void RemoveAnim(struct ITargetAnim * target);

	virtual void Update();

	virtual void Render();

	virtual void LoadMesh(const char * meshName);

	virtual char * GetMeshName();

	virtual IMeshInstance * GetMesh();

	virtual bool LoadAnimFile(const char * filename,const char * animName);

	virtual SINGLE GetAnimPlaytime(const char * animName);

	virtual void UnloadAnimation(const char * animName);

	virtual void Deselect();
	
	virtual void Select();

	virtual bool HitTest(Vector origin, Vector dir, SINGLE & dist);

	virtual TRANSFORM GetTransform();

	virtual void SetTransform(const TRANSFORM & trans);

	virtual Vector GetPosition();

	virtual void SetPosition(Vector pos);

	virtual void PlayAnimation(ITargetAnim * anim, bool bLooping);

	virtual void PlayNamedAnimation(const char * animName, bool bLooping);

	virtual void StopAnimation();

	virtual void SetCurrentAction(struct IEffectAction * action);

	virtual U32 GetEffectID();

	virtual U32 GetHardPointIndex(struct ITargetHp * hp);

	virtual U32 GetHardPointIndex(const char * hpName);

	virtual void GetHardPointTransform(U32 index, Transform & hpTrans);

	virtual U32 GetTargetID();

	virtual void SaveTest(struct IFileSystem * outfile);

	virtual void LoadTest(struct IFileSystem * inFile);

//	virtual IGrannyInstance * GetMesh();

	virtual void InitMovieLights();

	virtual void DeleteMovieLights();

	virtual void Hide(bool bSetting);

	virtual void AddToUpdateList(struct IEffectAction * action);

	virtual void ClearUpdateList();

	//IMeshCallback
	virtual void AnimationCue(struct IMeshInstance * meshInstance, const char * cueName);
};

U32 nextInstID = 1;

EffectTarget::EffectTarget()
{
	strcpy(name,"newName");
	next = NULL;
	firstHardPoint = NULL;
	firstAnim = NULL;
	trans.set_identity();
	meshFile[0] = 0;
	mesh = NULL;
	updateList = NULL;

	bSelected = false;
	bHide = false;
	currentAction = NULL;
	instID = nextInstID;
	nextInstID++;
	lightList = NULL;
}

EffectTarget::~EffectTarget()
{
	while(updateList)
	{
		ActionNode * tmp = updateList;
		updateList = updateList->next;
		delete tmp;
	}
	if(mesh)
	{
		MESHMAN->DestroyMesh(mesh);
		mesh = NULL;
	}
	if(bSelected)
		InfoArea::SelectTarget(NULL);
	while(lightList)
	{
		MovieLight * tmp = lightList;
		lightList = lightList->next;
		delete tmp;
	}
	EFFECTFILE->NullTarget(this);
}

IEffectTarget * EffectTarget::GetNextTarget()
{
	return next;
}

void EffectTarget::SetNextTarget(IEffectTarget * target)
{
	next = target;
}

const char * EffectTarget::GetName()
{
	return name;
}

void EffectTarget::SetName(const char * newName)
{
	strncpy(name,newName,63);
	name[63] = 0;
}

ITargetHp * EffectTarget::GetFirstHardpoint()
{
	return firstHardPoint;
}

ITargetHp * EffectTarget::AddHardpoint(const char * newName)
{
	ITargetHp * newHp = MakeTargetHp();
	newHp->SetName(newName);
	newHp->SetNextHP(firstHardPoint);
	firstHardPoint = newHp;
	return newHp;
}

void EffectTarget::RemoveTargetHp(ITargetHp * target)
{
	ITargetHp * search = firstHardPoint;
	ITargetHp * prev = NULL;
	while(search)
	{
		if(search == target)
		{
			if(prev)
				prev->SetNextHP(search->GetNextHP());
			else
				firstHardPoint = search->GetNextHP();
			DeleteTargetHp(search);
			return;
		}
		prev = search;
		search = search->GetNextHP();
	}
}

ITargetAnim * EffectTarget::GetFirstAnim()
{
	return firstAnim;
}

ITargetAnim * EffectTarget::AddAnim(const char * newName)
{
	ITargetAnim * newAnim = MakeTargetAnim();
	newAnim->SetName(newName);
	newAnim->SetNextAnim(firstAnim);
	newAnim->SetFileDependant(false);
	firstAnim = newAnim;

	if(mesh)
	{
		AnimScriptEntry scEntry;
		if(mesh->GetArchtype()->GetFirstAnimScript(scEntry))
		{
			do
			{
				if(strcmp(newAnim->GetName(),scEntry.name) == 0)
				{
					newAnim->SetScripted(true);
					AnimCueEntry cueDesc;
					if(	mesh->GetArchtype()->GetFirstAnimCue(scEntry,cueDesc))
					{
						do
						{
							ITargetCue * newCue = MakeTargetCue();
							newCue->SetName(cueDesc.name);
							newCue->SetTime(cueDesc.time);
							newCue->SetNextCue(newAnim->GetFirstCue());
							newAnim->SetFirstCue(newCue);
						}
						while(mesh->GetArchtype()->GetNextAnimCue(scEntry,cueDesc));
					}
					break;
				}
			}while(mesh->GetArchtype()->GetNextAnimScript(scEntry));
		}
	}

	return newAnim;
}

void EffectTarget::RemoveAnim(struct ITargetAnim * target)
{
	ITargetAnim * search = firstAnim;
	ITargetAnim * prev = NULL;
	while(search)
	{
		if(search == target)
		{
			if(prev)
				prev->SetNextAnim(search->GetNextAnim());
			else
				firstAnim = search->GetNextAnim();
			if(search->GetFileName()[0])
				UnloadAnimation(search->GetName());
			DeleteTargetAnim(search);
			return;
		}
		prev = search;
		search = search->GetNextAnim();
	}
}

void EffectTarget::Update()
{
	ActionNode * search = updateList;
	while(search)
	{
		search->action->UpdateAction();
		search = search->next;
	}
	if(mesh)
	{
		TRANSFORM t = trans;
		t.scale(FEET_TO_WORLD);
		mesh->SetTransform(t);
		mesh->Update(PreviewWin::GetRenderTime());
	}
	
/*	MovieLight * search = lightList;
	while(search)
	{
		TRANSFORM trans;
		mesh->GetHardPointTransform(search->bone,trans);
		LIGHT_DATA data;
		if(search->light)
		{
			search->light->getData(&data);
			data.position = trans.translation;
			search->light->setData(&data);
		}
		search = search->next;
	}
*/
}

void EffectTarget::Render()
{
	if(bHide)
		return;
	if(mesh)
	{
		PIPE->set_texture_stage_texture(0,0);
		PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

		PIPE->set_texture_stage_texture(1,0);
		PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_LIGHTING,TRUE);
		PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);

		mesh->Render();

/*		if(bSelected)
		{
			Vector center, vMin, vMax;
			mesh->GetExtents(center, vMin, vMax);

			PB.Begin(RENDERER,PB_LINES);
			PB.Color4ub(0,255,255, 255);

			//top
			PB.Vertex3f(vMax.x, vMax.y, vMax.z);
			PB.Vertex3f(vMin.x, vMax.y, vMax.z);

			PB.Vertex3f(vMin.x, vMin.y, vMax.z);
			PB.Vertex3f(vMin.x, vMax.y, vMax.z);

			PB.Vertex3f(vMax.x, vMax.y, vMax.z);
			PB.Vertex3f(vMax.x, vMin.y, vMax.z);

			PB.Vertex3f(vMin.x, vMin.y, vMax.z);
			PB.Vertex3f(vMax.x, vMin.y, vMax.z);

			//bottom
			PB.Vertex3f(vMin.x, vMin.y, vMin.z);
			PB.Vertex3f(vMin.x, vMax.y, vMin.z);

			PB.Vertex3f(vMin.x, vMin.y, vMin.z);
			PB.Vertex3f(vMax.x, vMin.y, vMin.z);

			PB.Vertex3f(vMax.x, vMin.y, vMin.z);
			PB.Vertex3f(vMax.x, vMax.y, vMin.z);

			PB.Vertex3f(vMax.x, vMax.y, vMin.z);
			PB.Vertex3f(vMin.x, vMax.y, vMin.z);
			
			//corners
			PB.Vertex3f(vMin.x, vMax.y, vMin.z);
			PB.Vertex3f(vMin.x, vMax.y, vMax.z);

			PB.Vertex3f(vMin.x, vMin.y, vMax.z);
			PB.Vertex3f(vMin.x, vMin.y, vMin.z);

			PB.Vertex3f(vMax.x, vMax.y, vMax.z);
			PB.Vertex3f(vMax.x, vMax.y, vMin.z);

			PB.Vertex3f(vMax.x, vMin.y, vMax.z);
			PB.Vertex3f(vMax.x, vMin.y, vMin.z);

			PB.End();
		}*/
	}
	else
	{
		PIPE->set_texture_stage_texture(0,0);
		PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

		PIPE->set_texture_stage_texture(1,0);
		PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		CAMERA->SetModelView();
		PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_LIGHTING,FALSE);

		PB.Begin(PB_LINES);
		if(bSelected)
			PB.Color3ub(128,128,255);
		else
			PB.Color3ub(255,255,255);
		PB.Vertex3fv((float*)(&(trans.translation+Vector(0,0,-DEFAULT_SIZE))));
		PB.Vertex3fv((float*)(&(trans.translation+Vector(0,0,DEFAULT_SIZE))));
		PB.Vertex3fv((float*)(&(trans.translation+Vector(0,-DEFAULT_SIZE,0))));
		PB.Vertex3fv((float*)(&(trans.translation+Vector(0,DEFAULT_SIZE,0))));
		PB.Vertex3fv((float*)(&(trans.translation+Vector(-DEFAULT_SIZE,0,0))));
		PB.Vertex3fv((float*)(&(trans.translation+Vector(DEFAULT_SIZE,0,0))));

		PB.End();
	}
}

void EffectTarget::LoadMesh(const char * meshName)
{
	if(mesh)
	{
		MESHMAN->DestroyMesh(mesh);
		mesh = NULL;
		ITargetHp * search = firstHardPoint;
		ITargetHp * prev = NULL;
		while(search)
		{
			if(search->GetIndex() != INVALID_HARD_POINT)//remove all old loaded hardpoints
			{
				if(prev)
					prev->SetNextHP(search->GetNextHP());
				else
					firstHardPoint = search->GetNextHP();
				ITargetHp * tmp = search;
				search = search->GetNextHP();
				DeleteTargetHp(tmp);
			}
			else
			{
				prev = search;
				search = search->GetNextHP();
			}
		}
	}

	strncpy(meshFile,meshName,255);
	meshFile[255] = 0;
	if(meshName)
	{
		mesh = MESHMAN->CreateMesh(meshFile);
		if(!mesh)
		{
			//the mesh has been lost.  Attempt to find it.
			char * str = strstr(meshFile,"Objects");
			if(str)
			{
				char buffer[255];
				str += 8;
				strncpy(buffer,str,255);
				buffer[255] = 0;
				strncpy(meshFile,buffer,255);
				meshFile[255] = 0;
				mesh = MESHMAN->CreateMesh(meshFile);
			}
		}
		if(mesh)
			mesh->SetCallback(this);
		
		//find all included animations and add them
		AnimScriptEntry animEntry;
		if(mesh && mesh->GetArchtype()->GetFirstAnimScript(animEntry))
		{
			do
			{
				bool bFound = false;
				ITargetAnim * search = firstAnim;
				while(search)
				{
					if(strcmp(search->GetName(),animEntry.name) == 0)
					{
						bFound = true;
					}
					search = search->GetNextAnim();
				}
				if(!bFound)
				{
					AddAnim(animEntry.name);
				}

			}while(mesh->GetArchtype()->GetNextAnimScript(animEntry));
		}

		HardPointDef hp;
		if(mesh && mesh->GetArchtype()->FindFirstHardpoint(hp))
		{
			do
			{
				ITargetHp * newHp = AddHardpoint(hp.name);
				newHp->SetIndex(hp.index);
			}
			while(mesh->GetArchtype()->FindNextHardpoint(hp));
		}
	}

}

char * EffectTarget::GetMeshName()
{
	return meshFile;
}

IMeshInstance * EffectTarget::GetMesh()
{
	return mesh;
}

bool EffectTarget::LoadAnimFile(const char * filename,const char * animName)
{
/*	if(mesh)
	{
		IGrannyAnimation * anim = GRANNY->CreateAnimation(filename);
		if(anim)
		{
			mesh->GetMesh()->AddAnimation(anim,animName,0);
			return true;
		}
	}
*/	return false;
}

SINGLE EffectTarget::GetAnimPlaytime(const char * animName)
{
	if(mesh)
	{
		return mesh->GetArchtype()->GetAnimationDurration(animName);
	}
	return 0;
}

void EffectTarget::UnloadAnimation(const char * animName)
{
/*	if(mesh)
	{
		mesh->GetMesh()->RemoveAnimation(animName);
	}
*/}

void EffectTarget::Deselect()
{
	bSelected = false;
}
	
void EffectTarget::Select()
{
	bSelected = true;
}

bool EffectTarget::HitTest(Vector origin, Vector dir, SINGLE & dist) 
{
	if(mesh)
	{
		HitDef hitDef;
		if(mesh->ComputeHitTest(&origin,&dir,hitDef))
		{
			dist = hitDef.hitDist;
			return true;
		}
		return false;
	}
	else
	{
		Vector lookDir = trans.translation-origin;
		lookDir.fast_normalize();
		if(dot_product(dir,lookDir) > 0.9999)
			return true;
	}
	return false;
}

TRANSFORM EffectTarget::GetTransform()
{
	return trans;
}

void EffectTarget::SetTransform(const TRANSFORM & _trans)
{
	trans = _trans;
}


Vector EffectTarget::GetPosition()
{
	return trans.translation;
}

void EffectTarget::SetPosition(Vector pos)
{
	trans.translation = pos;
}

void EffectTarget::PlayAnimation(ITargetAnim * anim, bool bLooping)
{
	if(mesh)
		mesh->PlayAnimation(anim->GetName(),bLooping);
}

void EffectTarget::PlayNamedAnimation(const char * animName, bool bLooping)
{
	if(mesh)
		mesh->PlayAnimation(animName,bLooping);
}

void EffectTarget::StopAnimation()
{
	if(mesh)
	{
		mesh->KillAnimations();
		mesh->Update(0);
	}
}

void EffectTarget::SetCurrentAction(struct IEffectAction * action)
{
	currentAction = action;
}

U32 EffectTarget::GetEffectID()
{
	return instID;
}

U32 EffectTarget::GetHardPointIndex(struct ITargetHp * hp)
{
	if(mesh && hp)
	{
		return mesh->GetArchtype()->FindHardPointIndex(hp->GetName());
	}
	return INVALID_HARD_POINT;
}

U32 EffectTarget::GetHardPointIndex(const char * hpName)
{
	if(mesh)
	{
		return mesh->GetArchtype()->FindHardPointIndex(hpName);
	}
	return INVALID_HARD_POINT;
}

void EffectTarget::GetHardPointTransform(U32 index, Transform & hpTrans)
{
	if(index == INVALID_HARD_POINT)
	{
		hpTrans = trans;
	}
	if(mesh)
	{
		mesh->GetHardPointTransform(index,hpTrans);
	}
	else
	{
		hpTrans = trans;
	}
}

U32 EffectTarget::GetTargetID()
{
	return EFFECTFILE->FindTargetPosition(this);
}

void EffectTarget::SaveTest(struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "EFFECTTARGET";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_NEW;

	U32 numAnim = 0;
	ITargetAnim * animCount = GetFirstAnim();
	while(animCount)
	{
		if(!(animCount->IsFileDependant()))
			++numAnim;
		animCount = animCount->GetNextAnim();
	}
	
	COMPTR<IFileSystem> baseData;
	if (outFile->CreateInstance(&fdesc, baseData) == GR_OK)
	{
		EffectTargetSave save;
		save.instID = instID;
		save.version = EFFECT_TARGET_SAVE_VERSION;
		save.numAnim = numAnim;
		strcpy(save.name,name);
		strcpy(save.meshfile,meshFile);
		save.trans = trans;
		U32 dwWritten;
		baseData->WriteFile(0,&save ,sizeof(EffectTargetSave),&dwWritten);
	}

	animCount = GetFirstAnim();
	U32 fileCount = 0;
	while(animCount)
	{
//		if(!(animCount->IsFileDependant()))//removed to allow saving animations with cues.
		{
			char dirName[256];
			sprintf(dirName,"Anim%d",fileCount);
			outFile->CreateDirectory(dirName);
			if (outFile->SetCurrentDirectory(dirName) == 0)
				return;
			animCount->SaveTest(outFile);
			if (outFile->SetCurrentDirectory("..") == 0)
				return;

			++fileCount;
		}
		animCount = animCount->GetNextAnim();
	}
}

void EffectTarget::LoadTest(struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "EFFECTTARGET";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
		
	COMPTR<IFileSystem> baseData;
	if (inFile->CreateInstance(&fdesc, baseData) == GR_OK)
	{
		EffectTargetSave save;
		U32 dwWritten;
		baseData->ReadFile(0,&save ,sizeof(EffectTargetSave),&dwWritten);
		instID = save.instID;
		if(instID >= nextInstID)
			nextInstID = instID+1;
		strcpy(name,save.name);
		if(save.meshfile[0])
			LoadMesh(save.meshfile);
		SetTransform(save.trans);
	}

	WIN32_FIND_DATA data;
	HANDLE handle;
	handle = inFile->FindFirstFile("Anim*",&data);
	if(handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (inFile->SetCurrentDirectory(data.cFileName) == 0)
					return;
				ITargetAnim * target = MakeTargetAnim();
				target->LoadTest(inFile);
				if (inFile->SetCurrentDirectory("..") == 0)
					return;

				//look to see if this stuff blongs to a cue.
				ITargetAnim * search = firstAnim;
				while(search)
				{
					if(strcmp(search->GetName(),target->GetName()) == 0)
						break;
					search = search->GetNextAnim();
				}
				if(search)
				{
					if(target->GetFileName()[0])
					{
						LoadAnimFile(target->GetFileName(),search->GetName());
						if(!LoadAnimFile(target->GetFileName(),search->GetName()))
						{
							const char * str = strstr(target->GetFileName(),"Objects");
							if(str)
							{
								str += 8;
								target->SetFileName(str);
								LoadAnimFile(target->GetFileName(),search->GetName());
							}
						}
						search->SetPlayTime(GetAnimPlaytime(search->GetName()));
					}
					delete target;
				}
				else
				{
					target->SetNextAnim(firstAnim);
					firstAnim = target;
					if(target->GetFileName()[0])
					{
						LoadAnimFile(target->GetFileName(),target->GetName());
						if(!LoadAnimFile(target->GetFileName(),target->GetName()))
						{
							const char * str = strstr(target->GetFileName(),"Objects");
							if(str)
							{
								str += 8;
								target->SetFileName(str);
								LoadAnimFile(target->GetFileName(),target->GetName());
							}
						}
						target->SetPlayTime(GetAnimPlaytime(target->GetName()));
					}
				}
			}
		}while(inFile->FindNextFile(handle,&data));
		inFile->CloseHandle(handle);
	}	
}

void EffectTarget::AnimationCue(struct IMeshInstance * meshInstance, const char * cueName)
{
	if(currentAction)
	{
		currentAction->TriggerEvent(cueName);
	}
}

void EffectTarget::InitMovieLights()
{
/*	IGrannyLightEntry lightArray[8];
	U32 lightCount = mesh->GetGrannyLights(lightArray);
	for(U32 i = 0; i < lightCount;++i)
	{
		MovieLight * light = new MovieLight;
		BT_LIGHT * base = (BT_LIGHT *)(ARCHLIST->GetArchetypeData("LIGHT!!PointLight"));
		BT_LIGHT def;
		memcpy(&def,base,sizeof(BT_LIGHT));
		
		def.lightProps.diffuse.red = lightArray[i].red*255;
		def.lightProps.diffuse.green = lightArray[i].green*255;
		def.lightProps.diffuse.blue = lightArray[i].blue*255;
		def.lightProps.ambient.red = 0;
		def.lightProps.ambient.green = 0;
		def.lightProps.ambient.blue = 0;

		//making the light infinite for now
//		def.lightProps.linearAttenuation = 0;
//		def.lightProps.squaredAttenuation = 1.0/(0.01*(lightArray[i].atten*FEET_TO_WORLD));
//		def.lightProps.exponentialAttenuation = 0;

		light->bone = lightArray[i].boneIndex;

		light->light = RENDERER->makeLight(&def);
		if(light->light)
			light->light->activate();
		light->next = lightList;
		lightList = light;
	}
	*/
}

void EffectTarget::DeleteMovieLights()
{
	while(lightList)
	{
		MovieLight * light = lightList;
		lightList = lightList->next;
/*		if(light->light)
		{
			light->light->deactivate();
			RENDERER->destroyLight(light->light);
		}
*/		delete light;
	}
}

void EffectTarget::Hide(bool bSetting)
{
	bHide = bSetting;
}

void EffectTarget::AddToUpdateList(struct IEffectAction * action)
{
	ActionNode * node = new ActionNode;
	node->action = action;
	node->next = updateList;
	updateList = node;
}

void EffectTarget::ClearUpdateList()
{
	while(updateList)
	{
		ActionNode * tmp = updateList;
		updateList = updateList->next;
		delete tmp;
	}
}

//-------------------------------------------------------------------
//
IEffectTarget * MakeEffectTarget()
{
	return new EffectTarget();
}

void DeleteEffectTarget(IEffectTarget *  target)
{
	delete ((EffectTarget*)target);
}


