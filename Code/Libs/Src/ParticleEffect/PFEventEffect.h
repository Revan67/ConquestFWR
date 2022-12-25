#ifndef PFEVENTEFFECT_H
#define PFEVENTEFFECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFEventEffect.h                             //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct EventProgramer : public ParticleProgramer
{
	char eventName[32];

	EventProgramer();
	~EventProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumStringParams();

	virtual const char * GetStringParamName(U32 index);

	virtual const char * GetStringParam(U32 index);

	virtual void SetStringParam(U32 index,const char * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

EventProgramer::EventProgramer()
{
	eventName[0] = 0;
}

EventProgramer::~EventProgramer()
{
}

//IParticleProgramer
U32 EventProgramer::GetNumOutput()
{
	return 1;
}

const char * EventProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 EventProgramer::GetNumStringParams()
{
	return 1;
}

const char * EventProgramer::GetStringParamName(U32 index)
{
	return "Event Name";
}

const char * EventProgramer::GetStringParam(U32 index)
{
	return eventName;
}

void EventProgramer::SetStringParam(U32 index,const char * param)
{
	strncpy(eventName,param,31);
	eventName[31] = 0;
}

U32 EventProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ (sizeof(char)*32);
}

void EventProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_EVENT;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	char * eventHeader = (char *)(buffer+offset);
	strcpy(eventHeader,eventName);
	offset += sizeof(char)*32;

	header->size = offset;
}

void EventProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	char * eventHeader = (char *)(buffer+offset);
	strcpy(eventName,eventHeader);
}

struct EventEffect : public IParticleEffect
{
	IParticleEffect * output;

	char eventString[32];
	
	U32 numInst;

	struct InstStruct
	{
		bool bSent;
	};
	InstStruct * instStruct;

	EventEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~EventEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 inst, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 inst);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

EventEffect::EventEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	instStruct = NULL;
	numInst = 1;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	strcpy(eventString,(char *)(buffer+sizeof(ParticleHeader)));
}

EventEffect::~EventEffect()
{
	delete instStruct;
}

IParticleEffectInstance * EventEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(output)
		delete output;
	output = createParticalEffect(parentSystem,type,buffer,this);
	return output;
}

void EventEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool EventEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if((!instStruct[instance].bSent) && (!bShutdown))
	{
		parentSystem->Event(eventString);
		instStruct[instance].bSent = true;
	}
	if(output)
		 return output->Update(inputStart,numInput,bShutdown,instance);
	return false;
}

U32 EventEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].bSent = false;
	}
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void EventEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void EventEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * EventEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void EventEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool EventEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
