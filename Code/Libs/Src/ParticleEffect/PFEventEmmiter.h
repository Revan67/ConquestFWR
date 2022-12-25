#ifndef PFEVENTEMMITER_H
#define PFEVENTEMMITER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFEventEmmiter.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct PEventEmmiterProgramer : public ParticleProgramer
{
	FloatType * maxParticles;
	FloatType * lifetime;

	PEventEmmiterProgramer();
	~PEventEmmiterProgramer();

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

PEventEmmiterProgramer::PEventEmmiterProgramer()
{
	maxParticles = MakeDefaultFloat(10);
	lifetime = MakeDefaultFloat(10);
}

PEventEmmiterProgramer::~PEventEmmiterProgramer()
{
	delete maxParticles;
	delete lifetime;
}

//IParticleProgramer
U32 PEventEmmiterProgramer::GetNumOutput()
{
	return 2;
}

const char * PEventEmmiterProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return FI_POINT_LIST;
	case 1:
		return FI_EVENT;
	}
	return NULL;
}

U32 PEventEmmiterProgramer::GetNumInput()
{
	return 1;
}

const char * PEventEmmiterProgramer::GetInputName(U32 index)
{
	return FI_EVENT;
}

U32 PEventEmmiterProgramer::GetNumFloatParams()
{
	return 2;
}

const char * PEventEmmiterProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Max Particles";
	case 1:
		return "Lifetime";
	}
	return NULL;
}

const FloatType * PEventEmmiterProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return maxParticles;
	case 1:
		return lifetime;
	}
	return NULL;
}

void PEventEmmiterProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(maxParticles)
				delete maxParticles;
			if(param)
				maxParticles = param->CreateCopy();
			else
				maxParticles = NULL;
			break;
		}
	case 1:
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

U32 PEventEmmiterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) + EncodeParam::EncodedFloatSize(maxParticles) + EncodeParam::EncodedFloatSize(lifetime);
}

void PEventEmmiterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_PEVENT_EMMITER;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * maxParticlesHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(maxParticlesHeader,maxParticles);
	offset += maxParticlesHeader->size;

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lifetimeHeader,lifetime);
	offset += lifetimeHeader->size;

	header->size = offset;
}

void PEventEmmiterProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * maxParticlesHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	maxParticles = EncodeParam::DecodeFloat(maxParticlesHeader);
	offset += maxParticlesHeader->size;

	EncodedFloatTypeHeader * lifeTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	lifetime = EncodeParam::DecodeFloat(lifeTimeHeader);
	offset += lifeTimeHeader->size;

}


struct PEventEmmiterEffect : public IParticleEffect
{
	IParticleEffect * output;
	IParticleEffect * pEventPassThrough;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * maxParticles;
	EncodedFloatTypeHeader * lifetime;

	U32 numInst;
	struct InstStruct
	{
		U32 particleRegionStart;
	};
	InstStruct * instStruct;

	U32 maxParticleAlloc;

	PEventEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~PEventEmmiterEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual U32 GetAllocationStart(IParticleEffectInstance * target,U32 instance);
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual U32 OutputRange();
	virtual bool ParticleEvent(const Vector & position, const Vector &velocity,U32 instance);

};

PEventEmmiterEffect::PEventEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	maxParticles = (EncodedFloatTypeHeader *)(data+offset);
	offset += maxParticles->size;

	lifetime = (EncodedFloatTypeHeader *)(data+offset);
	offset += lifetime->size;

	maxParticleAlloc = EncodeParam::GetMaxFloat(maxParticles,parentSystem);

	output = NULL;
	pEventPassThrough = NULL;
}

PEventEmmiterEffect::~PEventEmmiterEffect()
{
	delete [] instStruct;
	if(output)
		delete output;
	if(pEventPassThrough)
		delete pEventPassThrough;
}

IParticleEffectInstance * PEventEmmiterEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return output;
	}
	if(outputID == 1)
	{
		if(pEventPassThrough)
			delete pEventPassThrough;
		pEventPassThrough = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return pEventPassThrough;
	}
	return NULL;
}

void PEventEmmiterEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
	if(outputID == 1)
	{
		if(pEventPassThrough)
			delete pEventPassThrough;
		pEventPassThrough = (IParticleEffect*)target;
		pEventPassThrough->parentEffect[inputID] = this;
	}
}

U32 PEventEmmiterEffect::GetAllocationStart(IParticleEffectInstance * target,U32 instance)
{
	return instStruct[instance].particleRegionStart;
}

void PEventEmmiterEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,instStruct[instance].particleRegionStart,maxParticleAlloc,instance,parentTrans);
	if(pEventPassThrough)
		pEventPassThrough->Render(t,instStruct[instance].particleRegionStart,maxParticleAlloc,instance,parentTrans);
}

bool PEventEmmiterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	bool bStillAround = false;
	Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);

	for(U32 i = 0; i < maxParticleAlloc; ++i)
	{
		if(p1[i].bLive)
		{
			if((parentSystem->GetCurrentTimeMS() > p1[i].birthTime) && (parentSystem->GetCurrentTimeMS()- p1[i].birthTime > p1[i].lifeTime))
			{
				p2[i].bLive = false;
			}
			else
			{
				bStillAround = true;
			}
		}
	}
	if(!bStillAround)
		return false;
	bool bRetVal1 = true;
	bool bRetVal2 = false;
	if(output)
		bRetVal1 = output->Update(instStruct[instance].particleRegionStart,maxParticleAlloc, bShutdown,instance);
	if(pEventPassThrough)
		bRetVal2 = pEventPassThrough->Update(instStruct[instance].particleRegionStart,maxParticleAlloc, bShutdown,instance);
	return bRetVal1||bRetVal2;
}

U32 PEventEmmiterEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = 0;
	}
	U32 used = maxParticleAlloc*numInst;
	if(output)
		used += output->ParticlesUsed();
	if(pEventPassThrough)
		used += pEventPassThrough->ParticlesUsed();
	return used;
}

void PEventEmmiterEffect::FindAllocation(U32 & startPos)
{
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = startPos;
		startPos += maxParticleAlloc;
	}
	if(output)
		output->FindAllocation(startPos);
	if(pEventPassThrough)
		pEventPassThrough->FindAllocation(startPos);
}

void PEventEmmiterEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
	if(pEventPassThrough)
	{
		delete pEventPassThrough;
		pEventPassThrough = NULL;
	}
}

IParticleEffect * PEventEmmiterEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * posible = NULL;
	if(output)
		posible = output->FindFilter(searchName);
	if(posible)
		return posible;
	if(pEventPassThrough)
		return pEventPassThrough->FindFilter(searchName);
	return NULL;
}

void PEventEmmiterEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInst);
	if(pEventPassThrough)
		pEventPassThrough->SetInstanceNumber(numInst);
}

bool PEventEmmiterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		if(output->GetParentPosition(index,postion,lastIndex))
			return true;
	if(pEventPassThrough)
		return pEventPassThrough->GetParentPosition(index,postion,lastIndex);
	return false;
}

U32 PEventEmmiterEffect::OutputRange()
{
	return maxParticleAlloc;
};

bool PEventEmmiterEffect::ParticleEvent(const Vector & position, const Vector &velocity,U32 instance)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
	for(U32 i = 0; i < maxParticleAlloc; ++i)
	{
		if((!(p1[i].bLive)) && (!(p2[i].bLive)))
		{
			p2[i].bLive = true;
			p2[i].bNew = true;
			p2[i].bNoComputeVelocity = false;
			p2[i].birthTime = parentSystem->GetCurrentTimeMS();
			p2[i].pos = position;
			p2[i].vel = Vector(0,0,0);
			p2[i].lifeTime = EncodeParam::GetFloat(lifetime,&(p2[i]),parentSystem)*1000;
			break;
		}
	}
	if(pEventPassThrough)
		return pEventPassThrough->ParticleEvent(position,velocity,instance);
	return false;
}

#endif
