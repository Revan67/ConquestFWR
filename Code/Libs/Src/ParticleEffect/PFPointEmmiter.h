#ifndef PFPOINTEMMITER_H
#define PFPOINTEMMITER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFPointEmmiter.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct PointEmmiterProgramer : public ParticleProgramer
{
	TransformType * location;
	FloatType * rate;
	FloatType * lifetime;

	FloatType * maxParticles;

	PEE_Type spaceType;

	PointEmmiterProgramer();
	~PointEmmiterProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

	virtual U32 GetNumTransformParams();

	virtual const char * GetTransformParamName(U32 index);

	virtual const TransformType * GetTransformParam(U32 index);

	virtual void SetTransformParam(U32 index,const TransformType * param);

	virtual U32 GetNumEnumParams();

	virtual const char * GetEnumParamName(U32 index);

	virtual U32 GetNumEnumValues(U32 index);

	virtual const char * GetEnumValueName(U32 index, U32 value);

	virtual U32 GetEnumParam(U32 index);

	virtual void SetEnumParam(U32 index,U32 value);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

PointEmmiterProgramer::PointEmmiterProgramer()
{
	location = MakeDefaultTrans();
	rate = MakeDefaultFloat(10);
	lifetime = MakeDefaultFloat(10);
	maxParticles = MakeDefaultFloat(110);
	spaceType = PEE_WORLDSPACE;
}

PointEmmiterProgramer::~PointEmmiterProgramer()
{
	delete location;
	delete rate;
	delete lifetime;
	delete maxParticles;
}

//IParticleProgramer
U32 PointEmmiterProgramer::GetNumOutput()
{
	return 1;
}

const char * PointEmmiterProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 PointEmmiterProgramer::GetNumInput()
{
	return 1;
}

const char * PointEmmiterProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 PointEmmiterProgramer::GetNumFloatParams()
{
	return 3;
}

const char * PointEmmiterProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Rate";
	case 1:
		return "Lifetime";
	case 2:
		return "Max Particles";
	}
	return NULL;
}

const FloatType * PointEmmiterProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return rate;
	case 1:
		return lifetime;
	case 2:
		return maxParticles;
	}
	return NULL;
}

void PointEmmiterProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(rate)
				delete rate;
			if(param)
				rate = param->CreateCopy();
			else
				rate = NULL;
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
	case 2:
		{
			if(maxParticles)
				delete maxParticles;
			if(param)
				maxParticles = param->CreateCopy();
			else
				maxParticles = NULL;
			break;
		}
	}
}

U32 PointEmmiterProgramer::GetNumTransformParams()
{
	return 1;
}

const char * PointEmmiterProgramer::GetTransformParamName(U32 index)
{
	return "Location";
}

const TransformType * PointEmmiterProgramer::GetTransformParam(U32 index)
{
	return location;
}

void PointEmmiterProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(location)
		delete location;
	if(param)
		location = param->CreateCopy();
	else
		location = NULL;
}

U32 PointEmmiterProgramer::GetNumEnumParams()
{
	return 1;
}

const char * PointEmmiterProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Space Type";
	}
	return NULL;
}

U32 PointEmmiterProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * PointEmmiterProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "World";
			case 1:
				return "Parented";
			}
		}
		break;

	}
	return NULL;
}

U32 PointEmmiterProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return spaceType;
	}
	return 0;
}

void PointEmmiterProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		spaceType = (PEE_Type)value;
		break;

	}
}

U32 PointEmmiterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) + EncodeParam::EncodedFloatSize(lifetime) + 
		EncodeParam::EncodedFloatSize(rate) + EncodeParam::EncodedTransformSize(location) +
		+ EncodeParam::EncodedFloatSize(maxParticles) + sizeof(U32);
}

void PointEmmiterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_POINT_EMMITER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * rateHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(rateHeader,rate);
	offset += rateHeader->size;

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lifetimeHeader ,lifetime);
	offset += lifetimeHeader->size;

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(locationHeader ,location);
	offset += locationHeader->size;

	EncodedFloatTypeHeader * maxParticlesHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(maxParticlesHeader ,maxParticles);
	offset += maxParticlesHeader->size;

	U32 * sTypeHeader = (U32*)(buffer+offset);
	(*sTypeHeader) = spaceType;
	offset += sizeof(U32);

	header->size = offset;
}

void PointEmmiterProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * rateHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	rate = 	EncodeParam::DecodeFloat(rateHeader);
	offset += rateHeader->size;

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	lifetime = EncodeParam::DecodeFloat(lifetimeHeader);
	offset += lifetimeHeader->size;

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	location = EncodeParam::DecodeTransform(locationHeader);
	offset += locationHeader->size;

	if(header->version <= 2) //no maxParticles
	{
		maxParticles = MakeDefaultFloat(110);
	}
	else
	{
		EncodedFloatTypeHeader * maxParticlesHeader = (EncodedFloatTypeHeader *)(buffer+offset);
		maxParticles = EncodeParam::DecodeFloat(maxParticlesHeader);
		offset += maxParticlesHeader->size;
	}

	if(header->version <= 3)
	{
		spaceType = PEE_WORLDSPACE;
	}
	else
	{
		spaceType = *((PEE_Type*)(buffer+offset));
		offset += sizeof(U32);
	}
}


struct PointEmmiterEffect : public IParticleEffect
{
	IParticleEffect * output;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * rate;
	EncodedFloatTypeHeader * lifetime;
	EncodedTransformTypeHeader * location;

	PEE_Type spaceType;

	U32 numInst;
	struct InstStruct
	{
		U32 particleRegionStart;
		SINGLE time;
		Transform lastTrans;
		bool bHasTrans;
	};
	InstStruct * instStruct;

	U32 maxParticles;

	PointEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~PointEmmiterEffect();
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
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual U32 OutputRange();
};

PointEmmiterEffect::PointEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	rate = (EncodedFloatTypeHeader *)(data+offset);
	offset += rate->size;

	lifetime = (EncodedFloatTypeHeader *)(data+offset);
	offset += lifetime->size;

	location = (EncodedTransformTypeHeader *)(data+offset);
	offset += location->size;

	if(header->version <= 2)//no maxParticlesFloat
	{
		maxParticles = (U32)(EncodeParam::GetMaxFloat(rate,parentSystem)*(EncodeParam::GetMaxFloat(lifetime,parentSystem)+(3*DELTA_TIME)));
	}
	else
	{
		EncodedFloatTypeHeader * maxParticlesFloat;
		maxParticlesFloat = (EncodedFloatTypeHeader *)(data+offset);
		offset += maxParticlesFloat->size;

		maxParticles = (U32)(EncodeParam::GetMaxFloat(maxParticlesFloat,parentSystem));
	}

	if(header->version <=3)//no space type
	{
		spaceType = PEE_WORLDSPACE;
	}
	else
	{
		spaceType= *((PEE_Type*)(data+offset));
		offset += sizeof(U32);
	}
}

PointEmmiterEffect::~PointEmmiterEffect()
{
	delete [] instStruct;
	if(output)
		delete output;
}

IParticleEffectInstance * PointEmmiterEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return output;
	}
	return NULL;
}

void PointEmmiterEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

U32 PointEmmiterEffect::GetAllocationStart(IParticleEffectInstance * target,U32 instance)
{
	return instStruct[instance].particleRegionStart;
}

void PointEmmiterEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(spaceType == PEE_PARENTED)
	{
		Transform thisParent;
		Particle * pInput2 = &(parentSystem->myParticles->particles2[inputStart]);
		EncodeParam::FindObjectTransform(parentSystem,pInput2,t,location,thisParent,instance);
		if(output)
			output->Render(t,instStruct[instance].particleRegionStart,maxParticles,instance,thisParent);
	}
	else
	{
		if(output)
			output->Render(t,instStruct[instance].particleRegionStart,maxParticles,instance,parentTrans);
	}
}

bool PointEmmiterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	bool bStillAround = false;
	Transform trans;
	Transform trans2;
	Particle * pInput1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * pInput2 = &(parentSystem->myParticles->particles2[inputStart]);
	Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,pInput2,1.0,location,trans2,instance))
	{
		if(instStruct[instance].bHasTrans)
			trans = instStruct[instance].lastTrans;
		else
			trans = trans2;
		instStruct[instance].bHasTrans = true;
		instStruct[instance].lastTrans = trans2;
		instStruct[instance].time += DELTA_TIME;
		SINGLE respawnRate = EncodeParam::GetFloat(rate,&(p1[0]),parentSystem);
		if(respawnRate == 0)
			respawnRate = 1;
		SINGLE respawnFreq = 1/respawnRate;
		for(U32 i = 0; i < maxParticles; ++i)
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
			if((instStruct[instance].time >= respawnFreq) && (!(p1[i].bLive)) && (!bShutdown))
			{
				instStruct[instance].time -= respawnFreq;
				SINGLE t = 1.0f-(instStruct[instance].time/DELTA_TIME);
				bStillAround = true;
				p2[i].bLive = true;
				p2[i].bNew = true;
				p2[i].bNoComputeVelocity = false;
				p2[i].birthTime = (U32)(parentSystem->GetCurrentTimeMS()-((S32)(instStruct[instance].time*1000))+(DELTA_TIME*1000*2));
				if(spaceType == PEE_WORLDSPACE)
				{
					p2[i].bParented = false;
					p2[i].pos = (t+1.0f)*(trans2.translation-trans.translation)+trans.translation;
				}
				else
				{
					p2[i].bParented = true;
					p2[i].pos = Vector(0,0,0);
				}
				p2[i].vel = Vector(0,0,0);
				p2[i].lifeTime = (S32)EncodeParam::GetFloat(lifetime,&(p2[i]),parentSystem)*1000;
			}
		}
		U32 stepsLost = (U32)(instStruct[instance].time/respawnFreq);
		instStruct[instance].time -= (stepsLost*respawnFreq);
	}
	else
	{
		for(U32 i = 0; i < maxParticles; ++i)
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
	}
	if(!bStillAround)
		return false;
	if(output)
		return output->Update(instStruct[instance].particleRegionStart,maxParticles, bShutdown,instance);
	return true;
}

U32 PointEmmiterEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = 0;
		instStruct[i].time = 0;
		instStruct[i].bHasTrans = false;
	}
	if(output)
		return output->ParticlesUsed()+maxParticles*numInst;
	return maxParticles*numInst;
}

void PointEmmiterEffect::FindAllocation(U32 & startPos)
{
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = startPos;
		startPos += maxParticles;
	}
	if(output)
		output->FindAllocation(startPos);
}

void PointEmmiterEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void PointEmmiterEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * PointEmmiterEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void PointEmmiterEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInst);
}

bool PointEmmiterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	for(U32 instance = 0; instance < numInst; ++instance)
	{
		U32 last = instStruct[instance].particleRegionStart+maxParticles;
		if(index >= instStruct[instance].particleRegionStart && index < last)
		{
			if(spaceType == PEE_PARENTED)
			{
				lastIndex = last;
				Transform thisParent;
				Particle * pInput2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
				EncodeParam::FindObjectTransform(parentSystem,pInput2,0,location,thisParent,instance);
				postion = thisParent.translation;
				return true;
			}
		}
	}
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

U32 PointEmmiterEffect::OutputRange()
{
	return maxParticles;
};

#endif
