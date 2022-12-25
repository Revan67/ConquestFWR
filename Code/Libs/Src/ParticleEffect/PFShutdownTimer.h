#ifndef PFSHUTDOWNTIMER_H
#define PFSHUTDOWNTIMER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFShutdownTimer.h                           //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct ShutdownTimerProgramer : public ParticleProgramer
{
	FloatType * lifetime;

	ShutdownTimerProgramer();
	~ShutdownTimerProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

ShutdownTimerProgramer::ShutdownTimerProgramer()
{
	lifetime = MakeDefaultFloat(0);
}

ShutdownTimerProgramer::~ShutdownTimerProgramer()
{
	delete lifetime;
}

//IParticleProgramer
U32 ShutdownTimerProgramer::GetNumOutput()
{
	return 1;
}

const char * ShutdownTimerProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 ShutdownTimerProgramer::GetNumInput()
{
	return 1;
}

const char * ShutdownTimerProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 ShutdownTimerProgramer::GetNumFloatParams()
{
	return 1;
}

const char * ShutdownTimerProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Lifetime";
	}
	return NULL;
}

const FloatType * ShutdownTimerProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return lifetime;
	}
	return NULL;
}

void ShutdownTimerProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(lifetime)
				delete lifetime;
			if(param)
				lifetime = param->CreateCopy();
			else
				lifetime = NULL;
			break;
		}
	}
}

U32 ShutdownTimerProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(lifetime);
}

void ShutdownTimerProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SHUTDOWN_TIMER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lifetimeHeader ,lifetime);
	offset +=lifetimeHeader->size;

	header->size = offset;
}

void ShutdownTimerProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	lifetime = EncodeParam::DecodeFloat(lifetimeHeader);
	offset += lifetimeHeader->size;

}

struct ShutdownTimerEffect : public IParticleEffect
{
	IParticleEffect * output;
	EncodedFloatTypeHeader *lifetime;

	U8 * data;

	U32 numInst;

	struct InstStruct
	{
		SINGLE time;
	};
	InstStruct * instStruct;

	ShutdownTimerEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer, U32 inputID);
	virtual ~ShutdownTimerEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInpu, U32 instancet, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

ShutdownTimerEffect::ShutdownTimerEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	lifetime = (EncodedFloatTypeHeader *)(data+sizeof(ParticleHeader));
}

ShutdownTimerEffect::~ShutdownTimerEffect()
{
	delete [] instStruct;
}

IParticleEffectInstance * ShutdownTimerEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(output)
		delete output;
	output = createParticalEffect(parentSystem,type,buffer,this,inputID);
	return output;
}

void ShutdownTimerEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void ShutdownTimerEffect::Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool ShutdownTimerEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	instStruct[instance].time += DELTA_TIME;
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(instStruct[instance].time > EncodeParam::GetFloat(lifetime,&(p2[0]),parentSystem))
	{
		parentSystem->Shutdown();
		return false;
	}
	if(output)
		output->Update(inputStart,numInput,bShutdown,instance);
	return true;
}

U32 ShutdownTimerEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].time = 0;
	}
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void ShutdownTimerEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void ShutdownTimerEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void ShutdownTimerEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * ShutdownTimerEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void ShutdownTimerEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool ShutdownTimerEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
