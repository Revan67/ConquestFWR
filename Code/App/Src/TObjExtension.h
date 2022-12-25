#ifndef TOBJEXTENSION_H
#define TOBJEXTENSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 TObjExtansion.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjExtension.h 35    10/04/00 8:35p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif

#ifndef ENGINE_H
#include <Engine.h>
#endif

#ifndef PHYSICS_H
#include <Physics.h>
#endif

#ifndef _3DMATH_H
#include <3DMath.h>
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef DBASEDATA_H
#include <DBaseData.h>
#endif

#ifndef DEXTENSION_H
#include <DExtension.h>
#endif

#ifndef IUPGRADE_H
#include "IUpgrade.h"
#endif

#ifndef MESHRENDER_H
#include "MeshRender.h"
#endif

#ifndef IBUILD_H
#include "IBuild.h"
#endif

#ifndef OBJWATCH_H
#include "ObjWatch.h"
#endif


struct ExtensionInfo
{
#ifdef _DEBUG
	char *fileName;
#endif
	OBJPTR<IBuildEffect> addEffect;
	U32 addIndex;
	JointInfo addJointInfo;
	IMeshInfoTree *add_mesh_info;
	IMeshInfoTree *remove_mesh_info;
	OBJPTR<IBuildEffect> removeEffect;
	U32 removeIndex;
	JointInfo removeJointInfo;
	bool bAttached:1;
	bool isArch:1;
	bool bDuplicateAdd:1;
	bool bDuplicateRemove:1;
};

#define ObjectExtension _CoE
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectExtension : public Base, IUpgrade, EXTENSION_SAVELOAD
{
	ExtensionInfo extension[MAX_EXTENSIONS];

	typename typedef Base::SAVEINFO EXTENDSAVEINFO;
	typename typedef Base::INITINFO EXTENDINITINFO;

//	struct InitNode			initNode;
	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct PostRenderNode   postRenderNode;
	struct PhysUpdateNode   physUpdateNode;

	PARCHETYPE buildEffectArch;

	//----------------------------------
	
	ObjectExtension (void);

	~ObjectExtension (void);

	void initExtension (const EXTENDINITINFO & data);

	void saveTransform (EXTENDSAVEINFO & saveStruct);
	
	void loadTransform (EXTENDSAVEINFO & saveStruct);

	void extendPhysUpdate (SINGLE dt);

	void extendPostRender (void);

	/* IUpgrade */

	virtual void SetUpgrade(U32 level);

	virtual S8 GetUpgrade();

	virtual S8 GetWorkingUpgrade();

	virtual U8 GetUpgradeFlags();

	virtual SINGLE GetUpgradePercent();
	
	virtual void StartUpgrade(U32 level,U32 time);

	virtual void SetUpgradePercent(SINGLE percent);

	virtual void CancelUpgrade();
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectExtension< Base >::ObjectExtension (void) :
				//	initNode(this, InitProc(initExtension)),
					saveNode(this, SaveLoadProc(&ObjectExtension::saveTransform)),
					loadNode(this, SaveLoadProc(&ObjectExtension::loadTransform)),
					physUpdateNode(this, PhysUpdateProc(&ObjectExtension::extendPhysUpdate)),
					postRenderNode(this, RenderProc(&ObjectExtension::extendPostRender))
{
}
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectExtension< Base >::~ObjectExtension (void) 
{
	if(workingExtLevel != -1)
	{
		if(extension[workingExtLevel].addEffect)
		{
			delete extension[workingExtLevel].addEffect;
		}
		if(extension[workingExtLevel].removeEffect)
		{
			delete extension[workingExtLevel].removeEffect;
		}
		if(extension[workingExtLevel].removeIndex != -1)
		{
			CQASSERT(!extension[workingExtLevel].bDuplicateRemove);// I know this can happen but currently it does not in our game and it is late.
			ENGINE->destroy_instance(extension[workingExtLevel].removeIndex);
			DestroyMeshInfoTree(extension[workingExtLevel].remove_mesh_info);
		}
	}
	for(U32 exCount = 0; exCount < MAX_EXTENSIONS; ++exCount)
	{
		if(extension[exCount].bAttached)
		{
			if(extension[exCount].addIndex != -1 && extension[exCount].bDuplicateAdd)
			{
				for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
				{
					if(extension[i].addIndex == extension[exCount].addIndex)
					{
						extension[i].addIndex = -1;
					}
				}
			}
			if(extension[exCount].removeIndex != -1)
			{
				if(extension[exCount].bDuplicateRemove)
				{
					for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
					{
						if(extension[i].removeIndex == extension[exCount].removeIndex)
						{
							extension[i].removeIndex = -1;
						}
					}
				}
				ENGINE->destroy_instance(extension[exCount].removeIndex);
				DestroyMeshInfoTree(extension[exCount].remove_mesh_info);
			}
		}
		else
		{
			if(extension[exCount].addIndex != -1)
			{
				if(!extension[exCount].bDuplicateAdd)
				{
					ENGINE->destroy_instance(extension[exCount].addIndex);
					DestroyMeshInfoTree(extension[exCount].add_mesh_info);
				}
				else
				{
					bool deleteNeeded = true;
					for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
					{
						if(extension[i].addIndex == extension[exCount].addIndex)
						{
							if(extension[i].bAttached)
							{
								deleteNeeded = false;
							}
							extension[i].addIndex = -1;
						}
					}
					if(deleteNeeded)
					{
						ENGINE->destroy_instance(extension[exCount].addIndex);
						DestroyMeshInfoTree(extension[exCount].add_mesh_info);
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::initExtension (const EXTENDINITINFO & data)
{
	CQASSERT(instanceIndex != -1);

	buildEffectArch = data.pExtBuildEffect;

	//find the child objects used for extensions
	for(U32 exCount = 0; exCount < MAX_EXTENSIONS; ++exCount)
	{
		extension[exCount].bAttached = false;
		if(data.pData->extension[exCount].extensionName[0])
		{
			BT_EXTENSION_INFO * exData = (BT_EXTENSION_INFO *) ARCHLIST->GetArchetypeData(data.pData->extension[exCount].extensionName);

#ifdef _DEBUG
			BASE_PLATFORM_DATA * bdata = (BASE_PLATFORM_DATA *) ARCHLIST->GetArchetypeData(exData->archetypeName);
			extension[exCount].fileName = bdata->fileName;
#endif

			if(exData->archetypeName[0])
			{
				extension[exCount].isArch = true;
			}
			else
				extension[exCount].isArch = false;
			if(exData->addChildName[0])
			{	
				bool found = false;
				for(U32 i = 0; i < exCount && (!found); ++i)
				{
					if(extension[i].addIndex != INVALID_INSTANCE_INDEX)
					{
						if(strcmp(exData->addChildName,ENGINE->get_instance_part_name(extension[i].addIndex)) == 0)
						{
							found = true;
							extension[exCount].bDuplicateAdd = true;
							extension[i].bDuplicateAdd = true;
							extension[exCount].addIndex = extension[i].addIndex;
							memcpy (&(extension[exCount].addJointInfo),&(extension[i].addJointInfo),sizeof(JointInfo));
							extension[exCount].add_mesh_info = extension[i].add_mesh_info;
						}
					}
				}
				if(!found)
				{
					if((extension[exCount].addIndex = ENGINE->get_instance_child_next(instanceIndex,0,INVALID_INSTANCE_INDEX)) != INVALID_ARCHETYPE_INDEX)
					{
						while(strcmp(exData->addChildName,ENGINE->get_instance_part_name(extension[exCount].addIndex))!=0)
						{
							extension[exCount].addIndex = ENGINE->get_instance_child_next(instanceIndex,0,extension[exCount].addIndex);
							if(extension[exCount].addIndex == INVALID_ARCHETYPE_INDEX)
								break;
						}
					}
					if(extension[exCount].addIndex != INVALID_INSTANCE_INDEX)
					{
						extension[exCount].bDuplicateAdd = false;
						const JointInfo *temp = ENGINE->get_joint_info(extension[exCount].addIndex);
						memcpy (&(extension[exCount].addJointInfo),temp,sizeof(JointInfo));
						ENGINE->destroy_joint(instanceIndex,extension[exCount].addIndex);
//						mesh_info->DetachChild(extension[exCount].addIndex,&extension[exCount].add_mesh_info);
					}
				}
			}
			else
			{
				extension[exCount].addIndex = INVALID_ARCHETYPE_INDEX;
			}
			if(exData->removeChildName[0])
			{	
				bool found = false;
				for(U32 i = 0; i < exCount && (!found); ++i)
				{
					if(extension[i].removeIndex != INVALID_INSTANCE_INDEX)
					{
						if(strcmp(exData->removeChildName,ENGINE->get_instance_part_name(extension[i].removeIndex)) == 0)
						{
							extension[exCount].bDuplicateRemove = true;
							extension[i].bDuplicateRemove = true;
							found = true;
							extension[exCount].removeIndex = extension[i].removeIndex;
							memcpy (&(extension[exCount].removeJointInfo),&(extension[i].removeJointInfo),sizeof(JointInfo));
							extension[exCount].remove_mesh_info = extension[i].remove_mesh_info;
						}
					}
				}
				if(!found)
				{
					if((extension[exCount].removeIndex = ENGINE->get_instance_child_next(instanceIndex,0,INVALID_INSTANCE_INDEX)) != INVALID_ARCHETYPE_INDEX)
					{
						while(strcmp(exData->removeChildName,ENGINE->get_instance_part_name(extension[exCount].removeIndex))!=0)
						{
							extension[exCount].removeIndex = ENGINE->get_instance_child_next(instanceIndex,0,extension[exCount].removeIndex);
							if(extension[exCount].removeIndex == INVALID_ARCHETYPE_INDEX)
								break;
						}
					}
					if(extension[exCount].removeIndex != INVALID_INSTANCE_INDEX)
					{
						extension[exCount].bDuplicateRemove = false;
						const JointInfo *temp = ENGINE->get_joint_info(extension[exCount].removeIndex);
						memcpy (&(extension[exCount].removeJointInfo),temp,sizeof(JointInfo));
					}
				}
			}
			else
			{
				extension[exCount].removeIndex = INVALID_ARCHETYPE_INDEX;
			}
		}
		else
		{
			extension[exCount].addIndex = INVALID_ARCHETYPE_INDEX;
			extension[exCount].removeIndex = INVALID_ARCHETYPE_INDEX;
		}
	}
	extensionLevel = data.pData->extensionLevel-1;//0 should be nothing so this is  1 based in data and zero based in code.
	workingExtLevel = -1;
	levelsAdded = data.pData->extensionBits;

//	mc.numChildren = mesh_info->ListChildren(mc.mi);
	for(U32 i = 0 ; i < MAX_EXTENSIONS; ++i)
	{
		if(levelsAdded & (0x01 << i))
		{
			SetUpgrade(i);
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::saveTransform (EXTENDSAVEINFO & save)
{
	save.extend_SL = *static_cast<EXTENSION_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::loadTransform (EXTENDSAVEINFO & load)
{
	U8 oldLevel = levelsAdded;
	*static_cast<EXTENSION_SAVELOAD *>(this) = load.extend_SL;
	// use "load" variable here to prevent optimizer error (jy)
	if(load.extend_SL.workingExtLevel != -1)
	{
		//FIX!!!!!!!!!!!
		StartUpgrade(workingExtLevel,30);
		SetUpgradePercent(percentExt);
	}
	for(U32 i = 0 ; i < MAX_EXTENSIONS; ++i)
	{
		if((levelsAdded & (0x01 << i)) && (!(oldLevel & (0x01 << i))))
		{
			SetUpgrade(i);
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::extendPhysUpdate (SINGLE dt)
{
	if(bVisible && workingExtLevel != -1)
	{
		bool bPause = !SECTOR->SystemInSupply(systemID,playerID);
		if (extension[workingExtLevel].addEffect)
		{
			extension[workingExtLevel].addEffect->PauseBuildEffect(bPause);
			extension[workingExtLevel].addEffect.Ptr()->PhysicalUpdate(dt);
		}
		if (extension[workingExtLevel].removeEffect)
		{
			extension[workingExtLevel].removeEffect->PauseBuildEffect(bPause);
			extension[workingExtLevel].removeEffect.Ptr()->PhysicalUpdate(dt);
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::extendPostRender (void)
{
	if(workingExtLevel != -1)
	{
		if (extension[workingExtLevel].addEffect)
		{
			//ENGINE->render_instance(MAINCAM, extension[workingExtLevel].addIndex, 0,LODPERCENT, RF_CLAMP_COLOR, NULL);
			extension[workingExtLevel].addEffect.Ptr()->Render();
		}
		if (extension[workingExtLevel].removeEffect)
		{
			//ENGINE->render_instance(MAINCAM, extension[workingExtLevel].removeIndex, 0,LODPERCENT, RF_CLAMP_COLOR, NULL);
			extension[workingExtLevel].removeEffect.Ptr()->Render();
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::SetUpgrade(U32 level)
{
	CQASSERT(workingExtLevel == (S32)level || workingExtLevel == -1);
	SetUpgradePercent(1.0);
	levelsAdded |= (0x1 << level);
	workingExtLevel = -1;
	extensionLevel = level;
	if(!(extension[extensionLevel].bAttached))
	{
		extension[extensionLevel].bAttached = true;
		if(extension[extensionLevel].addIndex != INVALID_INSTANCE_INDEX)
		{	
			if(extension[extensionLevel].addEffect)
				delete extension[extensionLevel].addEffect;
			ENGINE->create_joint(instanceIndex,extension[extensionLevel].addIndex,&(extension[extensionLevel].addJointInfo));
			ENGINE->update_instance(instanceIndex,0,0);
//			mesh_info->AttachChild(instanceIndex,extension[extensionLevel].add_mesh_info);

		}
		if(extension[extensionLevel].removeIndex != INVALID_INSTANCE_INDEX)
		{
			if(extension[extensionLevel].removeEffect)
				delete extension[extensionLevel].removeEffect;
			else //must still be connected
			{
				ENGINE->destroy_joint(instanceIndex,extension[extensionLevel].removeIndex);
//				mesh_info->DetachChild(extension[extensionLevel].removeIndex,&extension[extensionLevel].remove_mesh_info);
			}
		}

		//TobjExtent::SetColors
//		mc.numChildren = mesh_info->ListChildren(mc.mi);
		if(extension[extensionLevel].isArch)
		{
			if(pArchetype)
			{
//				U32 oldHull = hullPointsMax;
				BT_EXTENSION_INFO * exData = (BT_EXTENSION_INFO *) ARCHLIST->GetArchetypeData(
					((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->extension[extensionLevel].extensionName);
				ARCHLIST->Release(pArchetype, OBJREFNAME);
				pArchetype = ARCHLIST->LoadArchetype(exData->archetypeName);
				CQASSERT(pArchetype);
				ARCHLIST->AddRef(pArchetype, OBJREFNAME);
				pInitData = &(((BASE_PLATFORM_DATA *) ARCHLIST->GetArchetypeData(pArchetype))->missionData);
				MGlobals::UpgradeMissionObj(this);
//				hullPoints += hullPointsMax-oldHull;
				if(supplies > supplyPointsMax)
					supplies = supplyPointsMax;
				mObjClass = pInitData->mObjClass;

				EXTENDINITINFO * iInfo = static_cast<EXTENDINITINFO *>(ARCHLIST->GetArchetypeHandle(pArchetype));
				FRAME_upgrade(*iInfo);
			}
		}
		SetColors();
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
S8 ObjectExtension< Base >::GetUpgrade()
{
	return extensionLevel;
}
//---------------------------------------------------------------------------
//
template <class Base> 
S8 ObjectExtension< Base >::GetWorkingUpgrade()
{
	return workingExtLevel;
}
//---------------------------------------------------------------------------
//
template <class Base> 
U8 ObjectExtension< Base >::GetUpgradeFlags()
{
	return levelsAdded;
}
//---------------------------------------------------------------------------
//
template <class Base> 
SINGLE ObjectExtension< Base >::GetUpgradePercent()
{
	return percentExt;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::StartUpgrade(U32 level,U32 time)
{
#ifdef _DEBUG
	BASE_PLATFORM_DATA * bdata = (BASE_PLATFORM_DATA *) ARCHLIST->GetArchetypeData(pArchetype);
	CQASSERT(stricmp(bdata->fileName,extension[level].fileName) == 0);
#endif

	CQASSERT((workingExtLevel == -1) || (workingExtLevel == (S32)level));
	workingExtLevel = level;
	percentExt = 0;
	if(!(extension[workingExtLevel].bAttached))
	{
		if(extension[workingExtLevel].addIndex != INVALID_INSTANCE_INDEX)
		{
			if (extension[workingExtLevel].addJointInfo.type == JT_FIXED)
			{
				TRANSFORM trans(extension[workingExtLevel].addJointInfo.rel_orientation,extension[workingExtLevel].addJointInfo.rel_position);
				trans = transform*trans;
				ENGINE->set_transform(extension[workingExtLevel].addIndex,trans);
				ENGINE->update_instance(extension[workingExtLevel].addIndex,0,0);
			}
			else
			{
				if (extension[workingExtLevel].addJointInfo.type != JT_REVOLUTE)
					CQTRACE10("upgrading unsupported joint");
				
				JointInfo *j = &extension[workingExtLevel].addJointInfo;

				TRANSFORM trans = transform;
				const Matrix & pR = transform;

				Matrix & cR = trans;
				Vector & cT = trans.translation;
				const Matrix Rrot ( Quaternion( j->axis, 0.0f) );
				cR *= Rrot * j->rel_orientation;
				cT += pR * j->parent_point - cR * j->child_point;
				
				ENGINE->set_transform(extension[workingExtLevel].addIndex,trans);
				ENGINE->update_instance(extension[workingExtLevel].addIndex,0,0);
			}

			IBaseObject * obj = ARCHLIST->CreateInstance(buildEffectArch);
			CQASSERT(obj);
			obj->QueryInterface(IBuildEffectID,extension[workingExtLevel].addEffect,NONSYSVOLATILEPTR);
			CQASSERT(extension[workingExtLevel].addEffect);

			extension[workingExtLevel].addEffect->SetupMesh(this,extension[workingExtLevel].add_mesh_info,time);
			extension[workingExtLevel].addEffect->SetBuildPercent(0);
			extension[workingExtLevel].addEffect->SetBuildRate(1.0/time);
		}
		if(extension[workingExtLevel].removeIndex != INVALID_INSTANCE_INDEX)
		{
			ENGINE->destroy_joint(instanceIndex,extension[workingExtLevel].removeIndex);
//			mesh_info->DetachChild(extension[workingExtLevel].removeIndex,&extension[workingExtLevel].remove_mesh_info);

		//	mesh_info->GetChildInfo(extension[workingExtLevel].removeIndex,&extension[workingExtLevel].remove_mesh_info);

		/*	TRANSFORM trans(extension[workingExtLevel].removeJointInfo.rel_orientation,extension[workingExtLevel].removeJointInfo.rel_position);
			trans = transform*trans;
			ENGINE->set_transform(extension[workingExtLevel].removeIndex,trans);*/

			IBaseObject * obj = ARCHLIST->CreateInstance(buildEffectArch);
			CQASSERT(obj);
			obj->QueryInterface(IBuildEffectID,extension[workingExtLevel].removeEffect,NONSYSVOLATILEPTR);
			CQASSERT(extension[workingExtLevel].removeEffect);


			extension[workingExtLevel].removeEffect->SetupMesh(this,extension[workingExtLevel].remove_mesh_info,time);
			extension[workingExtLevel].removeEffect->SetBuildPercent(1.0);
			extension[workingExtLevel].removeEffect->SetBuildRate(-1.0/time);
		}
//		mc.numChildren = mesh_info->ListChildren(mc.mi);
	}	
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::SetUpgradePercent(SINGLE percent)
{
	percentExt = percent;
	if(workingExtLevel != -1)
	{
		if(extension[workingExtLevel].addEffect)
			extension[workingExtLevel].addEffect->SetBuildPercent(percent);
		if(extension[workingExtLevel].removeEffect)
			extension[workingExtLevel].removeEffect->SetBuildPercent(1.0-percent);
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectExtension< Base >::CancelUpgrade()
{
	CQASSERT(workingExtLevel != -1);
	if(!(extension[workingExtLevel].bAttached))
	{
		if(extension[workingExtLevel].addIndex != INVALID_INSTANCE_INDEX)
		{
			if(extension[workingExtLevel].addEffect)
				delete extension[workingExtLevel].addEffect;
		}
		if(extension[workingExtLevel].removeIndex != INVALID_INSTANCE_INDEX)
		{
			if(extension[workingExtLevel].removeEffect)
			{
				extension[workingExtLevel].removeEffect->Done();
				delete extension[workingExtLevel].removeEffect;
			}
			ENGINE->create_joint(instanceIndex,extension[workingExtLevel].removeIndex,&(extension[workingExtLevel].removeJointInfo));
			ENGINE->update_instance(instanceIndex,0,0);
//			mesh_info->AttachChild(instanceIndex,extension[workingExtLevel].remove_mesh_info);
		}
	}	
	workingExtLevel = -1;

}
//---------------------------------------------------------------------------
//------------------------End TObjExtansion.h----------------------------------
//---------------------------------------------------------------------------
#endif