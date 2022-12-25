// ParticleEffect.cpp : Defines the entry point for the dll application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <IParticleManager.h>
#include "ParticleSystem.h"
#include "ParticleParamFunctions.h"
#include <tcomponent.h>
#include <da_heap_utility.h>

#define CLSID_ParticleManager "IParticleManager"


//Particles.cpp
IParticleInstance* makeBaseParticleInstance(IInternalParticleManager * owner);
IParticleProgramer * makeParticleProgramer(ParticleEffectType type);
void destroyParticleInstance(IParticleInstance * inst);
void destroyParticleProgramer(IParticleProgramer * inst);

//

struct ParticleManager : IParticleManager, public IAggregateComponent, IInternalParticleManager
{
	BEGIN_DACOM_MAP_INBOUND(ParticleManager)
		DACOM_INTERFACE_ENTRY(IParticleManager)
		DACOM_INTERFACE_ENTRY2(IID_IParticleManager,IParticleManager)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT COMAPI init( AGGDESC *desc );

	// IParticleManager

	virtual void InitParticles(InitInfo & info);

	virtual IParticleInstance * CreateBaseParticleInstance();

	virtual IParticleProgramer * CreateParticleProgramer(ParticleEffectType type);

	virtual void ReleaseInstance(IParticleInstance * inst);

	virtual void ReleaseInstance(IParticleProgramer * inst);

	virtual void Update(SINGLE time);

	virtual void Close();

	virtual const char * GetFilterName(ParticleEffectType pType);

	virtual U32 GetFilterNumber();

	virtual ParticleEffectType GetFilterEnum(U32 number);

	virtual void SaveFloatType(IFileSystem * outFile,FloatType * target);

	virtual FloatType * LoadFloatType(IFileSystem * inFile);

	virtual void SaveTransType(IFileSystem * outFile,TransformType * target);

	virtual TransformType * LoadTransType(IFileSystem * inFile);

	//IInternalParticleManager
	virtual S32 GetRealTimeMS();

	virtual S32 GetLastUpdateTimeMS();

	virtual ICamera* GetCamera();

	virtual IGamePositionCallback * GetCallback();

	virtual struct IMaterialManager * GetMaterialManager();

	virtual struct IMeshManager * GetMeshManager();

	virtual void AddToParticleFreeList(ParticleBatch * old);

	virtual ParticleBatch * AllocParticles(U32 allocateNumber);

	//ParticleManager
	ParticleManager();

	~ParticleManager();


	void CleanUpParticles();

	//data
	ICamera* GMCAMERA;
	IGamePositionCallback * POSCALLBACK;
	IMaterialManager * MATMAN;
	IMeshManager * MESHMAN;

	S32 realTimeMS;
	S32 lastUpdateTimeMS;

	ParticleBatch * freeParticleList;
};

GENRESULT COMAPI ParticleManager::Initialize(void) 
{ 
	return GR_OK; 
};

GENRESULT COMAPI ParticleManager::init( AGGDESC *desc )
{
	return GR_OK;
};

void ParticleManager::InitParticles(InitInfo & info)
{
	GMCAMERA = info.camera;
	POSCALLBACK = info.posCallback;
	MATMAN = info.MATMAN;
	MESHMAN = info.MESHMAN;
}

IParticleInstance * ParticleManager::CreateBaseParticleInstance()
{
	return makeBaseParticleInstance(this);
}

IParticleProgramer * ParticleManager::CreateParticleProgramer(ParticleEffectType type)
{
	return makeParticleProgramer(type);
}

void ParticleManager::ReleaseInstance(IParticleInstance * inst)
{
	destroyParticleInstance(inst);
}

void ParticleManager::ReleaseInstance(IParticleProgramer * inst)
{
	destroyParticleProgramer(inst);
}

void ParticleManager::Update(SINGLE time)
{
	S32 TimeMS = (S32)(time*1000);
	if(realTimeMS > (S32)TimeMS)
		lastUpdateTimeMS = (S32)TimeMS;
	else
		lastUpdateTimeMS = realTimeMS;
	realTimeMS = (S32)TimeMS;
}

void ParticleManager::Close()
{
	CleanUpParticles();
}

const char * ParticleManager::GetFilterName(ParticleEffectType pType)
{
	switch(pType)
	{
	case PE_POINT_EMMITER:
		{
			return "Point Emmiter";
		}
	case PE_INITIAL_VELOCITY:
		{
			return "Initial Velocity";
		}
	case PE_RENDER_BILBOARD:
		{
			return "Render Billboard";
		}
	case PE_SPHERE_SHAPER:
		{
			return "Sphere Shaper";
		}
	case PE_CYLINDER_SHAPER:
		{
			return "Cylinder Shaper";
		}
		break;
	case PE_BOX_SHAPER:
		{
			return "Box Shaper";
		}
		break;		
	case PE_FIXED_EMMITER:
		{
			return "Fixed Emmiter";
		}
		break;	
	case PE_INITIAL_DIRECTED_VELOCITY:
		{
			return "Directed Initial Velocity";
		}
		break;	
	case PE_MOTION_INTERP:
		{
			return "Motion Interp";
		}
		break;	
	case PE_ACCELERATION:
		{
			return "Acceleration";
		}
		break;	
	case PE_DIRECTIONAL_GRAVITY:
		{
			return "Directional Gravity";
		}
		break;	
	case PE_POINT_GRAVITY:
		{
			return "Point Gravity";
		}
		break;	
	case PE_SHUTDOWN_TIMER:
		{
			return "Shutdown Timer";
		}
		break;	
	case PE_MATMOD_COLOR:
		{
			return "MatMod Color";
		}
		break;	
	case PE_MATMOD_FLOAT:
		{
			return "MatMod Float";
		}
		break;	
	case PE_RENDER_MESH:
		{
			return "Mesh Render";
		}
		break;	
	case PE_MESH_INST:
		{
			return "Mesh Inst";
		}
		break;	
	case PE_MESH_SLICE:
		{
			return "Mesh Slice";
		}
		break;	
	case PE_MESH_CUT:
		{
			return "Mesh Cut";
		}
		break;	
	case PE_MESH_REF:
		{
			return "Mesh Ref";
		}
		break;	
	case PE_SPHERE_HITTER:
		{
			return "Sphere Hitter";
		}
		break;
	case PE_PEVENT_KILLER:
		{
			return "Event Killer";
		}
		break;
	case PE_PEVENT_EMMITER:
		{
			return "Event Emmiter";
		}
		break;
	case PE_DISTANCE_DELTA_HITTER:
		{
			return "Distance Hitter";
		}
		break;
	case PE_CAMERA_SHAKE:
		{
			return "Camera Shake";
		}
		break;
	case PE_SPAWNER:
		{
			return "Spawner";
		}
		break;
	case PE_TARGET_SHAPER:
		{
			return "Target Shaper";
		}
		break;
	case PE_COPY_POINTS:
		{
			return "Copy Points";
		}
		break;
	case PE_CAMERA_SORT:
		{
			return "Camera Sort";
		}
		break;
		
	}
	return "Unknown";
}

U32 ParticleManager::GetFilterNumber()
{
	return 29;
}

ParticleEffectType ParticleManager::GetFilterEnum(U32 number)
{
	switch(number)
	{
	case 0:
		return PE_POINT_EMMITER;
	case 1:
		return PE_INITIAL_VELOCITY;
	case 2:
		return PE_RENDER_BILBOARD;
	case 3:
		return PE_SPHERE_SHAPER;
	case 4:
		return PE_CYLINDER_SHAPER;
	case 5:
		return PE_BOX_SHAPER;
	case 6:
		return PE_FIXED_EMMITER;
	case 7:
		return PE_INITIAL_DIRECTED_VELOCITY;
	case 8:
		return PE_MOTION_INTERP;
	case 9:
		return PE_ACCELERATION;
	case 10:
		return PE_DIRECTIONAL_GRAVITY;
	case 11:
		return PE_POINT_GRAVITY;
	case 12:
		return PE_SHUTDOWN_TIMER;
	case 13:
		return PE_MATMOD_COLOR;
	case 14:
		return PE_MATMOD_FLOAT;
	case 15:
		return PE_RENDER_MESH;
	case 16:
		return PE_MESH_INST;
	case 17:
		return PE_MESH_SLICE;
	case 18:
		return PE_MESH_CUT;
	case 19:
		return PE_MESH_REF;
	case 20:
		return PE_SPHERE_HITTER;
	case 21:
		return PE_PEVENT_KILLER;
	case 22:
		return PE_PEVENT_EMMITER;
	case 23:
		return PE_DISTANCE_DELTA_HITTER;
	case 24:
		return PE_CAMERA_SHAKE;
	case 25:
		return PE_SPAWNER;
	case 26:
		return PE_TARGET_SHAPER;
	case 27:
		return PE_COPY_POINTS;
	case 28:
		return PE_CAMERA_SORT;
		
	}
	return PE_POINT_EMMITER;
}

void ParticleManager::SaveFloatType(IFileSystem * outFile,FloatType * target)
{
	if(target)
	{
		U32 dataSize = EncodeParam::EncodedFloatSize(target);
		U8 * data = new U8[dataSize];
		EncodeParam::EncodeFloat((EncodedFloatTypeHeader*)data,target);

		U32 dwWritten = 0;
		outFile->WriteFile(0,data ,dataSize,&dwWritten);
	}
}

FloatType * ParticleManager::LoadFloatType(IFileSystem * inFile)
{
	EncodedFloatTypeHeader header;
	U32 dwWritten = 0;
	inFile->ReadFile(0,&(header) ,sizeof(EncodedFloatTypeHeader),&dwWritten);
	U8 *data = new U8[header.size];
	memcpy(data,&header,sizeof(EncodedFloatTypeHeader));
	inFile->ReadFile(0,data+sizeof(EncodedFloatTypeHeader) ,header.size-sizeof(EncodedFloatTypeHeader),&dwWritten);
	FloatType * floatType = EncodeParam::DecodeFloat((EncodedFloatTypeHeader*)data);
	delete data;
	return floatType;
}

void ParticleManager::SaveTransType(IFileSystem * outFile,TransformType * target)
{
	if(target)
	{
		U32 dataSize = EncodeParam::EncodedTransformSize(target);
		U8 * data = new U8[dataSize];
		EncodeParam::EncodeTransform((EncodedTransformTypeHeader*)data,target);

		U32 dwWritten = 0;
		outFile->WriteFile(0,data ,dataSize,&dwWritten);
	}
}

TransformType * ParticleManager::LoadTransType(IFileSystem * inFile)
{
	EncodedTransformTypeHeader header;
	U32 dwWritten = 0;
	inFile->ReadFile(0,&(header) ,sizeof(EncodedTransformTypeHeader),&dwWritten);
	U8 *data = new U8[header.size];
	memcpy(data,&header,sizeof(EncodedTransformTypeHeader));
	inFile->ReadFile(0,data+sizeof(EncodedTransformTypeHeader) ,header.size-sizeof(EncodedTransformTypeHeader),&dwWritten);
	TransformType * transType = EncodeParam::DecodeTransform((EncodedTransformTypeHeader*)data);
	delete data;
	return transType;
}

S32 ParticleManager::GetRealTimeMS()
{
	return realTimeMS;
}

S32 ParticleManager::GetLastUpdateTimeMS()
{
	return lastUpdateTimeMS;
}

ICamera* ParticleManager::GetCamera()
{
	return GMCAMERA;
}

IGamePositionCallback * ParticleManager::GetCallback()
{
	return POSCALLBACK;
}

IMaterialManager * ParticleManager::GetMaterialManager()
{
	return MATMAN;
}

IMeshManager * ParticleManager::GetMeshManager()
{
	return MESHMAN;
}

ParticleManager::ParticleManager()
{
	GMCAMERA = NULL;
	POSCALLBACK = NULL;

	realTimeMS = NULL;
	lastUpdateTimeMS = NULL;

	freeParticleList = NULL;
}

ParticleManager::~ParticleManager()
{
	CleanUpParticles();
}

void ParticleManager::AddToParticleFreeList(ParticleBatch * old)
{
	old->next = freeParticleList;
	freeParticleList = old;
}

ParticleBatch * ParticleManager::AllocParticles(U32 allocateNumber)
{
	ParticleBatch * best = NULL;
	ParticleBatch * bestPrev = NULL;
	ParticleBatch * search = freeParticleList;
	ParticleBatch * prev = NULL;
	while(search)
	{
		if(search->numAllocated >= allocateNumber)
		{
			if(best)
			{
				if(best->numAllocated > search->numAllocated)
				{
					bestPrev = prev;
					best = search;
				}
			}
			else
			{
				bestPrev = prev;
				best = search;
			}
		}
		prev = search;
		search = search->next;
	}
	if(best)
	{
		if(bestPrev)
			bestPrev->next = best->next;
		else
			freeParticleList = best->next;
		best->next = NULL;
		return best;
	}
	
	best = new ParticleBatch;
	best->next = 0;
	best->numAllocated = (U16)allocateNumber;
	best->particles1 = new Particle[allocateNumber];
	best->particles2 = new Particle[allocateNumber];
	return best;
}

void ParticleManager::CleanUpParticles()
{
	while(freeParticleList)
	{
		ParticleBatch * tmp = freeParticleList;
		freeParticleList = freeParticleList->next;
		delete [](tmp->particles1);
		delete [](tmp->particles2);
		delete tmp;
	}
}


BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		//
		// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
		//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hModule );

			ICOManager * DACOM = DACOM_Acquire(); 
			IComponentFactory * server;

			// Register System aggragate factory
			if( DACOM && (server = new DAComponentFactory2<DAComponentAggregate<ParticleManager>, AGGDESC>(CLSID_ParticleManager)) != NULL ) 
			{
				DACOM->RegisterComponent( server, CLSID_ParticleManager, DACOM_NORMAL_PRIORITY );
				server->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

