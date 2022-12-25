
#include "stdafx.h"
#include <windows.h>
#include "IParticleManager.h"
#include <FDUMP.h>

#include <ICAMERA.h>
#include <FileSys.h>
#include <tsmartpointer.h>

#include "IMaterialManager.h"
#include "IMeshManager.h"

#include "TComponent.h"
#include "da_heap_utility.h"

#include "stdio.h"
#include "windows.h"
#include <stdlib.h>

#include "Particlesystem.h"
#include "ParticlesaveStructs.h"
#include "ParticleParamFunctions.h"

#define DELTA_TIME (0.1f) //tenth of a second
#define MAX_UPDATE 10

#define FEET_TO_WORLD 1.0f//0.0078125    // scale conversion for granny objects in the world
#define WORLD_TO_FEET 1.0f//(1.0/0.0078125)    // scale conversion for granny objects in the world

struct ParticleSystem;
struct IParticleEffect;

IParticleEffect * createParticalEffect(ParticleSystem * parentSystem,ParticleEffectType type,U8 * buffer, 
									   IParticleEffect * parentEffect, U32 inputID);

#define FI_POINT_LIST "Point List"
#define FI_MAT_MOD "MatMod List"
#define FI_MESH_INST "Mesh Inst"
#define FI_EVENT "Event"

enum BooleanEnum
{
	BE_FALSE,
	BE_TRUE,
};

enum AxisEnum
{
	AXIS_I,
	AXIS_J,
	AXIS_K,
};

enum PEE_Type
{
	PEE_WORLDSPACE,
	PEE_PARENTED,
};

FloatType * MakeDefaultFloat(SINGLE value)
{
	FloatType * retVal = new FloatType;
	retVal->type = FloatType::CONSTANT;
	retVal->constant = value;
	return retVal;
}

FloatType * MakeDefaultRangeFloat(SINGLE minVal,SINGLE maxVal)
{
	FloatType * const1 = new FloatType;
	const1->type = FloatType::CONSTANT;
	const1->constant = minVal;
	FloatType * const2 = new FloatType;
	const2->type = FloatType::CONSTANT;
	const2->constant = maxVal;
	FloatType * retVal = new FloatType;
	retVal->type = FloatType::RANGE;
	retVal->range.min = const1;
	retVal->range.max = const2;
	return retVal;
}

TransformType * MakeDefaultTrans()
{
	TransformType * retVal = new TransformType;
	retVal->type = TransformType::UP;
	return retVal;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//IParticleEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct IParticleEffect * LPIParticleEffect; 
struct IParticleEffect : public IParticleEffectInstance
{
	IParticleEffect * next;//only used by root effects
	IParticleEffect ** parentEffect;
	ParticleSystem * parentSystem;

	U32 numInputs;

	char effectName[32];

	IParticleEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U32 _numInputs, U32 parentInputID)
	{
		numInputs = _numInputs;
		effectName[0] = 0;
		if(numInputs)
		{
			parentEffect = new LPIParticleEffect[numInputs];
			memset(parentEffect,0,sizeof(LPIParticleEffect)*numInputs);
			parentEffect[parentInputID] = _parentEffect;
		}
		else
			parentEffect = NULL;
		parentSystem = _parent;
		next = NULL;
	};

	virtual ~IParticleEffect();

	//IParticleEffectInstance

	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 intputID, ParticleEffectType type, U8 * buffer)
	{
		return NULL;
	};

	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
	{
	}

	virtual U32 GetAllocationStart(IParticleEffectInstance * target,U32 instance)
	{
		if(parentEffect[0])
			return parentEffect[0]->GetAllocationStart(this,instance);
		return 0;
	}

	virtual void GetTransform(SINGLE t,Transform & trans,U32 instance)
	{
		if(parentEffect[0])
			parentEffect[0]->GetTransform(t,trans,instance);
	};

	//IParticleEffect


	virtual void DeleteOutput()
	{};

	virtual void NullOutput(IParticleEffect * target)
	{};

	virtual void Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
	{};

	virtual IModifier * UpdateMatMod(IMaterial * mat, IModifier * inMod, SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
	{
		return NULL;
	}
	
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
	{
		return true;
	}

	virtual U32 ParticlesUsed()
	{
		return 0;
	}

	virtual void FindAllocation(U32 & startPos)
	{
	};

	virtual IParticleEffect * FindFilter(const char * searchName) = 0;

	virtual void SetInstanceNumber(U32 numInstances)
	{
	}

	virtual U32 OutputRange()
	{
		if(parentEffect[0])
			return parentEffect[0]->OutputRange();
		return 0;
	};

	virtual bool ParticleEvent(const Vector & position, const Vector &velocity,U32 instance)
	{
		return false;
	}

	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
	{
		return false;
	}

	virtual IMeshInstance * GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance)
	{
		return NULL;
	}
};

IParticleEffect::~IParticleEffect()
{
	for(U32 i = 0; i < numInputs; ++i)
	{
		if(parentEffect[i])
		{
			parentEffect[i]->NullOutput(this);
		}
	}
	if(parentEffect)
	{
		delete parentEffect;
		parentEffect = NULL;
	}
}

struct ParticleProgramer : public IParticleProgramer
{
	char effectName[32];

	ParticleProgramer()
	{
		effectName[0] = 0;
	};

	virtual ~ParticleProgramer();

	//IParticleEffectInstance

	virtual void SetEffectName(const char * name)
	{
		strncpy(effectName,name,31);
		effectName[31] = 0;
	}

	virtual const char * GetEffectName()
	{
		return effectName;
	}

	virtual U32 GetNumOutput()
	{
		return 0;
	}

	virtual const char * GetOutputName(U32 index)
	{
		return NULL;
	}

	virtual U32 GetNumInput()
	{
		return 0;
	}

	virtual const char * GetInputName(U32 index)
	{
		return "Unknown";
	}

	virtual U32 GetNumFloatParams()
	{
		return 0;
	};

	virtual const char * GetFloatParamName(U32 index)
	{
		return NULL;
	}

	virtual const FloatType * GetFloatParam(U32 index)
	{
		return NULL;
	}

	virtual void SetFloatParam(U32 index,const FloatType * param)
	{
	}

	virtual U32 GetNumTransformParams()
	{
		return 0;
	}

	virtual const char * GetTransformParamName(U32 index)
	{
		return NULL;
	}

	virtual const TransformType * GetTransformParam(U32 index)
	{
		return NULL;
	}

	virtual void SetTransformParam(U32 index,const TransformType * param)
	{
	}

	virtual U32 GetNumStringParams()
	{
		return 0;
	}

	virtual const char * GetStringParamName(U32 index)
	{
		return NULL;
	}

	virtual const char * GetStringParam(U32 index)
	{
		return NULL;
	}

	virtual void SetStringParam(U32 index,const char * param)
	{
	}

	virtual U32 GetNumEnumParams()
	{
		return 0;
	}

	virtual const char * GetEnumParamName(U32 index)
	{
		return NULL;
	}

	virtual U32 GetNumEnumValues(U32 index)
	{
		return 0;
	}

	virtual const char * GetEnumValueName(U32 index, U32 value)
	{
		return NULL;
	}

	virtual U32 GetEnumParam(U32 index)
	{
		return 0;
	}

	virtual void SetEnumParam(U32 index,U32 value)
	{
	}

	virtual U32 GetNumTargetParams()
	{
		return 0;
	}

	virtual const char * GetTargetParamName(U32 index)
	{
		return NULL;
	}

	virtual const U32 GetTargetParam(U32 index)
	{
		return 0;
	}

	virtual void SetTargetParam(U32 index,U32 param)
	{
	}

	virtual U32 GetDataChunkSize()
	{
		return 0;
	}

	virtual void GetDataChunk(U8 * buffer)
	{
	}

	virtual void SetDataChunk(U8 * buffer)
	{
	}
};

ParticleProgramer::~ParticleProgramer()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//ParticleSystem
//////////////////////////////////////////////////////////////////////////////////////////////////////
struct ParticleSystem : public IParticleInstance ,IParticleSystem
{
	IInternalParticleManager * owner;
	IParticleEffect * firstEffect;
	ParticleBatch * myParticles;

	Vector minVect;
	Vector maxVect;
	bool bHasExtents;

	DWORD context;

	bool bShutdown;//true if a shutdown has been called in any way
	bool bAllDone;//true if there are no active particles

	U32 leftOverTimeMS;
	U32 startTimeMS;

	U32 currentTimeMS;

	SINGLE renderTime;

	struct ListenerNode 
	{
		IParticleListener * listener;
		ListenerNode * next;
	};

	ListenerNode * listenerList;

	ParticleSystem(IInternalParticleManager * _owner)
	{
		owner = _owner;
		startTimeMS = 0;
		context = 0;
		leftOverTimeMS = 0;
		renderTime = 0.0;
		bAllDone = false;
		bShutdown = false;
		firstEffect = NULL;
		myParticles = NULL;
		listenerList = NULL;
		bHasExtents = false;
		currentTimeMS = 0;
	}

	~ParticleSystem()
	{
		while(listenerList)
		{
			ListenerNode * tmp = listenerList;
			listenerList = listenerList->next;
			delete tmp;
		}
		if(myParticles)
		{
			owner->AddToParticleFreeList(myParticles);
			myParticles= NULL;
		}
		while(firstEffect)
		{
			IParticleEffect * tmp = firstEffect;
			firstEffect = firstEffect->next;
			delete tmp;
		}
	}

	//IParticleSystem

	virtual IParticleEffectInstance * FindFilter(const char * name);

	virtual Particle * GetParticle(U32 i);

	virtual DWORD GetContext();

	virtual U32 StartTime();

	virtual struct IInternalParticleManager * GetOwner();

	virtual S32 GetCurrentTimeMS();

	virtual SINGLE GetRenderTime();

	//IParticleInstance
	virtual void SetContext(DWORD _context);

	virtual void Initialize(DWORD _context);

	virtual void Render(bool bVisible);

	virtual bool GetVisInfo(Vector & center, SINGLE & radius);

	virtual IParticleEffectInstance * AddFilter(ParticleEffectType type, U8 * buffer);

	virtual void Shutdown();

	virtual bool IsFinished();

	virtual void Event(const char * eventName);

	virtual void AddParticalListener(IParticleListener * listener);

	virtual void RemoveParticalListener(IParticleListener * listener);
};

IParticleEffectInstance * ParticleSystem::FindFilter(const char * name)
{
	IParticleEffect * search = firstEffect;
	while(search)
	{
		IParticleEffect * found = search->FindFilter(name);
		if(found)
			return found;
		search = search->next;
	}
	return NULL;
}

Particle * ParticleSystem::GetParticle(U32 i)
{
	return &(myParticles->particles2[i]);
}

DWORD ParticleSystem::GetContext()
{
	return context;
}

U32 ParticleSystem::StartTime()
{
	return startTimeMS;
}

IInternalParticleManager * ParticleSystem::GetOwner()
{
	return owner;
}

S32 ParticleSystem::GetCurrentTimeMS()
{
	return currentTimeMS;
}

SINGLE ParticleSystem::GetRenderTime()
{
	return renderTime;
}

void ParticleSystem::SetContext(DWORD _context)
{
	context = _context;
}

void ParticleSystem::Initialize(DWORD _context)
{
	startTimeMS = owner->GetRealTimeMS();
	context = _context;
	U32 allocateNumber = 0;
	if(myParticles)
	{
		owner->AddToParticleFreeList(myParticles);
		myParticles= NULL;
	}
	IParticleEffect * search = firstEffect;
	while(search)
	{
		allocateNumber += search->ParticlesUsed();
		search = search->next;
	}
	if(!myParticles)
	{
		myParticles = owner->AllocParticles(allocateNumber);
	}

	memset(myParticles->particles1,0,sizeof(Particle)*myParticles->numAllocated);
	memset(myParticles->particles2,0,sizeof(Particle)*myParticles->numAllocated);

	search = firstEffect;
	U32 allocStart = 0;
	while(search)
	{
		search->FindAllocation(allocStart);
		search = search->next;
	}
}

void ParticleSystem::Render(bool bVisible)
{
	static bool bSkipParticleUpdate = false;
	static bool bSkipParticleRender = false;
	if (bSkipParticleUpdate) return;
	renderTime = owner->GetRealTimeMS() - owner->GetLastUpdateTimeMS();
	renderTime /= 1000.0f;
	float dt = (float)((owner->GetRealTimeMS() - owner->GetLastUpdateTimeMS())+leftOverTimeMS);
	dt /= 1000.0;//into seconds
	U32 numUpdate = (U32)(floor(dt/DELTA_TIME));
	if(numUpdate > MAX_UPDATE)
	{
		leftOverTimeMS = 0;
		numUpdate = MAX_UPDATE;
	}
	else
	{
		leftOverTimeMS = (U32)((dt*1000.0f)-((numUpdate*DELTA_TIME)*1000.0f));
	}
	for(U32 i = 0; i < numUpdate; ++i)
	{
		currentTimeMS = (U32)(owner->GetRealTimeMS()-((DELTA_TIME*1000)*(numUpdate-(i+1)) ));
		Particle * pTmp = myParticles->particles1;
		myParticles->particles1 = myParticles->particles2;
		myParticles->particles2 = pTmp;

		bHasExtents = false;

		Vector parentPos(0,0,0);
		U32 lastIndex = myParticles->numAllocated;
		IParticleEffect * tmp = firstEffect;
		while (tmp)
		{
			if(tmp->GetParentPosition(0,parentPos,lastIndex))
				break;
			tmp = tmp->next;
		}
	
		for(U32 pCount = 0;pCount < myParticles->numAllocated; ++pCount)
		{
			if(pCount >= lastIndex)
			{
				IParticleEffect * tmp = firstEffect;
				parentPos = Vector(0,0,0);
				while (tmp)
				{
					if(tmp->GetParentPosition(pCount,parentPos,lastIndex))
						break;
					tmp = tmp->next;
				}
			}
			if(myParticles->particles1[pCount].bLive)
			{
				myParticles->particles2[pCount].bLive = true;
				myParticles->particles2[pCount].bNew = false;
				myParticles->particles2[pCount].bParented = myParticles->particles1[pCount].bParented;
				myParticles->particles2[pCount].bNoComputeVelocity = myParticles->particles1[pCount].bNoComputeVelocity;
				myParticles->particles2[pCount].birthTime = myParticles->particles1[pCount].birthTime;
				myParticles->particles2[pCount].lifeTime = myParticles->particles1[pCount].lifeTime;
				myParticles->particles2[pCount].vel = myParticles->particles1[pCount].vel;
				if(myParticles->particles2[pCount].bNoComputeVelocity)
					myParticles->particles2[pCount].pos = myParticles->particles1[pCount].pos;
				else
					myParticles->particles2[pCount].pos = myParticles->particles1[pCount].pos +(myParticles->particles1[pCount].vel*DELTA_TIME);
				if(bHasExtents)
				{
					SINGLE val = myParticles->particles2[pCount].pos.x+parentPos.x;
					if(val < minVect.x)
						minVect.x = val;
					else if(val > maxVect.x)
						maxVect.x = val;
					val = myParticles->particles2[pCount].pos.y+parentPos.y;
					if(val < minVect.y)
						minVect.y = val;
					else if(val > maxVect.y)
						maxVect.y = val;
					val = myParticles->particles2[pCount].pos.z+parentPos.z;
					if(val < minVect.z)
						minVect.z = val;
					else if(val > maxVect.z)
						maxVect.z = val;
				}
				else
				{
					bHasExtents = true;
					minVect = maxVect = myParticles->particles2[pCount].pos+parentPos;
				}
			}
			else
			{
				myParticles->particles2[pCount].bLive = false;
			}
		}

		bAllDone = true;
		tmp = firstEffect;
		while (tmp)
		{
			bool bStillActive = tmp->Update(0,0,bShutdown,0);
			if(bStillActive)
				bAllDone = false;
			tmp = tmp->next;
		}
	}
	if(bVisible)
	{
		if (bSkipParticleRender) return;
		currentTimeMS = owner->GetRealTimeMS();
		SINGLE t = (leftOverTimeMS/1000.0f)/DELTA_TIME;
		IParticleEffect * tmp = firstEffect;
		while (tmp)
		{
			Transform baseTrans;
			baseTrans.set_identity();
			tmp->Render(t,0,0,0,baseTrans);
			tmp = tmp->next;
		}
	}
}

bool ParticleSystem::GetVisInfo(Vector & center, SINGLE & radius)
{
	if(bHasExtents)
	{
		center = (minVect+maxVect)/2;
		radius = (minVect-center).fast_magnitude() + (1*FEET_TO_WORLD);
		return true;
	}
	return false;
}

IParticleEffectInstance * ParticleSystem::AddFilter(ParticleEffectType type, U8 * buffer)
{
	IParticleEffect * tmp = createParticalEffect(this,type,buffer,NULL,0);
	tmp->next = firstEffect;
	firstEffect = tmp;
	return tmp;
}

void ParticleSystem::Shutdown()
{
	bShutdown = true;
}

bool ParticleSystem::IsFinished()
{
	return (bShutdown && bAllDone);
}

void ParticleSystem::Event(const char * eventName)
{
	ListenerNode * search = listenerList;
	while(search)
	{
		search->listener->ParticalEvent(eventName);
		search = search->next;
	}
}

void ParticleSystem::AddParticalListener(IParticleListener * listener)
{
	ListenerNode * node = new ListenerNode;
	node->listener = listener;
	node->next = listenerList;
	listenerList = node;
}

void ParticleSystem::RemoveParticalListener(IParticleListener * listener)
{
	ListenerNode * search = listenerList;
	ListenerNode * prev = NULL;
	while(search)
	{
		if(search->listener == listener)
		{
			if(prev)
				prev->next = search->next;
			else
				listenerList = search->next;
			delete search;
			return;
		}
		prev = search;
		search = search->next;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//Each include file contains a filter for the particle system.  These headers are only intended to be used
//	in this location.

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PointEmmiterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFPointEmmiter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//CopyPointsEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFCopyPoints.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//InitialVelocityEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFInitialVelocity.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//BillboardRenderEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFBillboardRender.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MeshRenderEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMeshRender.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MeshInstEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMeshInst.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MeshSliceEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMeshSlice.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MeshCutEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMeshCut.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MeshRefEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMeshRef.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//SphereShaperEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFSphereShaper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//BoxShaperEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFBoxShaper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//CylinderShaperEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFCylinderShaper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//EventEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFEventEffect.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PositionCallbackEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFPositionCallback.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//LightEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFLight.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//SoundEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFSound.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//SpawnEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFSpawn.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//ShutdownTimerEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFShutdownTimer.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//SplitterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFSplitter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//TimedSwitchEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFTimedSwitch.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//LogicSwitchEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFLogicSwitch.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//MotionInterpEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMotionInterp.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////
//TargetShaperEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFTargetShaper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//FixedEmmiterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFFixedEmmiter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PEventEmmiterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFEventEmmiter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PEventKillerEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFEventKiller.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PEventExplosionEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFEventExplosion.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//DirectionalGravityEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFDirectionalGravity.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//PointGravityEffect 
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFPointGravity.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//AccelerationEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFAcceleration.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//WindEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFWind.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//CollisionEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFCollision.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//SphereHitterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFSphereHitter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//DistanceDeltaHitterEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFDistanceDeltaHitter.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//AnimBillboardRenderEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFAnimBillboardRender.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//InitialDirectedVelocityEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFInitialDirectedVelocity.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//DecalEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "PFDecalRender.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//CameraShakeEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFCameraShake.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MatModColorEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMatModColor.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//MatModFloatEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFMatModFloat.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//CameraSortEffect
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PFCameraSort.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
IParticleEffect * createParticalEffect(ParticleSystem * parentSystem,ParticleEffectType type,U8 * buffer,IParticleEffect * parentEffect, U32 inputID)
{
	switch(type)
	{
	case PE_POINT_EMMITER:
		{
			return new PointEmmiterEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_FIXED_EMMITER:
		{
			return new FixedEmmiterEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_RENDER_BILBOARD:
		{
			return new BillboardRenderEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_INITIAL_VELOCITY:
		{
			return new InitialVelocityEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_INITIAL_DIRECTED_VELOCITY:
		{
			return new InitialDirectedVelocityEffect(parentSystem,parentEffect,buffer,inputID);
		}
	case PE_SPHERE_SHAPER:
		{
			return new SphereShaperEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_CYLINDER_SHAPER:
		{
			return new CylinderShaperEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_BOX_SHAPER:
		{
			return new BoxShaperEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;

	case PE_MOTION_INTERP:
		{
			return new MotionInterpEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_ACCELERATION:
		{
			return new AccelerationEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_DIRECTIONAL_GRAVITY:
		{
			return new DirectionalGravityEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_POINT_GRAVITY:
		{
			return new PointGravityEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_SHUTDOWN_TIMER:
		{
			return new ShutdownTimerEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MATMOD_COLOR:
		{
			return new MatModColorEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MATMOD_FLOAT:
		{
			return new MatModFloatEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_RENDER_MESH:
		{
			return new MeshRenderEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MESH_INST:
		{
			return new MeshInstEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MESH_SLICE:
		{
			return new MeshSliceEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MESH_CUT:
		{
			return new MeshCutEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_MESH_REF:
		{
			return new MeshRefEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_SPHERE_HITTER:
		{
			return new SphereHitterEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_PEVENT_KILLER:
		{
			return new PEventKillerEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_PEVENT_EMMITER:
		{
			return new PEventEmmiterEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_DISTANCE_DELTA_HITTER:
		{
			return new DistanceDeltaHitterEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_CAMERA_SHAKE:
		{
			return new CameraShakeEffect(parentSystem,parentEffect,buffer,inputID);
		}
	case PE_SPAWNER:
		{
			return new SpawnEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_TARGET_SHAPER:
		{
			return new TargetShaperEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_COPY_POINTS:
		{
			return new CopyPointsEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;
	case PE_CAMERA_SORT:
		{
			return new CameraSortEffect(parentSystem,parentEffect,buffer,inputID);
		}
		break;

/*	case PE_RENDER_BILBOARD_2:
		{
			return new BillboardRenderEffect(parentSystem,parentEffect,buffer, 2);
		}
		break;
	case PE_SPLITTER:
		{
			return new SplitterEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_EVENT:
		{
			return new EventEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_TIMED_SWITCH:
		{
			return new TimedSwitchEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_RENDER_GRANNY:
		{
			return new GrannyRenderEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_LOGIC_SWITCH:
		{
			return new LogicSwitchEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_POSITION_CALLBACK:
		{
			return new PositionCallbackEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_COLLISION:
		{
			return new CollisionEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_SOUND:
		{
			return new SoundEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_PEVENT_EXPLOSION:
		{
			return new PEventExplosionEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_WIND:
		{
			return new WindEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_LIGHT:
		{
			return new LightEffect(parentSystem,parentEffect,buffer);
		}
		break;
	case PE_ANIMBILLBOARDRENDER:
		{
			return new AnimBillboardRenderEffect(parentSystem,parentEffect,buffer);
		}
	case PE_DECAL_RENDER:
		{
			return new DecalEffect(parentSystem,parentEffect,buffer);
		}
*/	}
	return NULL;
}

IParticleInstance* makeBaseParticleInstance(IInternalParticleManager * owner)
{
	return new ParticleSystem(owner);
}

IParticleProgramer * makeParticleProgramer(ParticleEffectType type)
{
	switch(type)
	{
	case PE_INITIAL_VELOCITY:
		{
			return new InitialVelocityProgramer();
		}
		break;
	case PE_POINT_EMMITER:
		{
			return new PointEmmiterProgramer();
		}
		break;
	case PE_FIXED_EMMITER:
		{
			return new FixedEmmiterProgramer();
		}
		break;
	case PE_RENDER_BILBOARD:
		{
			return new BillboardRenderProgramer();
		}
		break;
	case PE_INITIAL_DIRECTED_VELOCITY:
		{
			return new InitialDirectedVelocityProgramer();
		}
		break;
	case PE_SPHERE_SHAPER:
		{
			return new SphereShaperProgramer();
		}
		break;
	case PE_CYLINDER_SHAPER:
		{
			return new CylinderShaperProgramer();
		}
		break;
	case PE_BOX_SHAPER:
		{
			return new BoxShaperProgramer();
		}
		break;
	case PE_MOTION_INTERP:
		{
			return new MotionInterpProgramer();
		}
		break;
	case PE_ACCELERATION:
		{
			return new AccelerationProgramer();
		}
		break;
	case PE_DIRECTIONAL_GRAVITY:
		{
			return new DirectionalGravityProgramer();
		}
		break;
	case PE_POINT_GRAVITY:
		{
			return new PointGravityProgramer();
		}
		break;
	case PE_SHUTDOWN_TIMER:
		{
			return new ShutdownTimerProgramer();
		}
		break;
	case PE_MATMOD_COLOR:
		{
			return new MatModColorProgramer();
		}
		break;
	case PE_MATMOD_FLOAT:
		{
			return new MatModFloatProgramer();
		}
		break;
	case PE_RENDER_MESH:
		{
			return new MeshRenderProgramer();
		}
		break;
	case PE_MESH_INST:
		{
			return new MeshInstProgramer();
		}
		break;
	case PE_MESH_SLICE:
		{
			return new MeshSliceProgramer();
		}
		break;
	case PE_MESH_CUT:
		{
			return new MeshCutProgramer();
		}
		break;
	case PE_MESH_REF:
		{
			return new MeshRefProgramer();
		}
		break;
	case PE_SPHERE_HITTER:
		{
			return new SphereHitterProgramer();
		}
		break;
	case PE_PEVENT_KILLER:
		{
			return new PEventKillerProgramer();
		}
		break;
	case PE_PEVENT_EMMITER:
		{
			return new PEventEmmiterProgramer();
		}
		break;
	case PE_DISTANCE_DELTA_HITTER:
		{
			return new DistanceDeltaHitterProgramer();
		}
		break;
	case PE_CAMERA_SHAKE:
		{
			return new CameraShakeProgramer();
		}
		break;
	case PE_SPAWNER:
		{
			return new SpawnProgramer();
		}
		break;
	case PE_TARGET_SHAPER:
		{
			return new TargetShaperProgramer();
		}
		break;
	case PE_COPY_POINTS:
		{
			return new CopyPointsProgramer();
		}
		break;

	case PE_CAMERA_SORT:
		{
			return new CameraSortProgramer();
		}
		break;

/*	case PE_RENDER_BILBOARD_2:
		{
			return new BillboardRenderProgramer_2();
		}
		break;
	case PE_SPLITTER:
		{
			return new SplitterProgramer();
		}
		break;
	case PE_EVENT:
		{
			return new EventProgramer();
		}
		break;
	case PE_TIMED_SWITCH:
		{
			return new TimedSwitchProgramer();
		}
		break;
	case PE_RENDER_GRANNY:
		{
			return new GrannyRenderProgramer();
		}
		break;
	case PE_LOGIC_SWITCH:
		{
			return new LogicSwitchProgramer();
		}
		break;
	case PE_POSITION_CALLBACK:
		{
			return new PositionCallbackProgramer();
		}
		break;
	case PE_COLLISION:
		{
			return new CollisionProgramer();
		}
		break;
	case PE_SOUND:
		{
			return new SoundProgramer();
		}
		break;
	case PE_PEVENT_EXPLOSION:
		{
			return new PEventExplosionProgramer();
		}
		break;
	case PE_WIND:
		{
			return new WindProgramer();
		}
		break;
	case PE_LIGHT:
		{
			return new LightProgramer();
		}
		break;
	case PE_ANIMBILLBOARDRENDER:
		{
			return new AnimBillboardRenderProgramer();
		}
		break;
	case PE_DECAL_RENDER:
		{
			return new DecalProgramer();
		}
		break;
*/	}
	return NULL;
}

void destroyParticleInstance(IParticleInstance * inst)
{
	delete (ParticleSystem*)inst;
}

void destroyParticleProgramer(IParticleProgramer * inst)
{
	delete (ParticleProgramer*)inst;
}
