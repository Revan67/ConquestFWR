#ifndef PFFIXEDEMMITER_H
#define PFFIXEDEMMITER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFFixedEmmiter.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct FixedEmmiterProgramer : public ParticleProgramer
{
	TransformType * location;
	FloatType * number;
	FloatType * lifetime;

	PEE_Type spaceType;

	FixedEmmiterProgramer();
	~FixedEmmiterProgramer();

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

FixedEmmiterProgramer::FixedEmmiterProgramer()
{
	location = MakeDefaultTrans();
	number = MakeDefaultFloat(10);
	lifetime = MakeDefaultFloat(10);
	spaceType = PEE_WORLDSPACE;
}

FixedEmmiterProgramer::~FixedEmmiterProgramer()
{
	delete location;
	delete number;
	delete lifetime;
}

//IParticleProgramer
U32 FixedEmmiterProgramer::GetNumOutput()
{
	return 1;
}

const char * FixedEmmiterProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 FixedEmmiterProgramer::GetNumInput()
{
	return 1;
}

const char * FixedEmmiterProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 FixedEmmiterProgramer::GetNumFloatParams()
{
	return 2;
}

const char * FixedEmmiterProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Number";
	case 1:
		return "Lifetime";
	}
	return NULL;
}

const FloatType * FixedEmmiterProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return number;
	case 1:
		return lifetime;
	}
	return NULL;
}

void FixedEmmiterProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(number)
				delete number;
			if(param)
				number = param->CreateCopy();
			else
				number = NULL;
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

U32 FixedEmmiterProgramer::GetNumTransformParams()
{
	return 1;
}

const char * FixedEmmiterProgramer::GetTransformParamName(U32 index)
{
	return "Location";
}

const TransformType * FixedEmmiterProgramer::GetTransformParam(U32 index)
{
	return location;
}

void FixedEmmiterProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(location)
		delete location;
	if(param)
		location = param->CreateCopy();
	else
		location = NULL;
}

U32 FixedEmmiterProgramer::GetNumEnumParams()
{
	return 1;
}

const char * FixedEmmiterProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Space Type";
	}
	return NULL;
}

U32 FixedEmmiterProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * FixedEmmiterProgramer::GetEnumValueName(U32 index, U32 value)
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

U32 FixedEmmiterProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return spaceType;
	}
	return 0;
}

void FixedEmmiterProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		spaceType = (PEE_Type)value;
		break;

	}
}

U32 FixedEmmiterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) + EncodeParam::EncodedFloatSize(lifetime) + EncodeParam::EncodedFloatSize(number) + 
		EncodeParam::EncodedTransformSize(location) + sizeof(U32);
}

void FixedEmmiterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_FIXED_EMMITER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * numberHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(numberHeader,number);
	offset += numberHeader->size;

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lifetimeHeader ,lifetime);
	offset += lifetimeHeader->size;

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(locationHeader ,location);
	offset += locationHeader->size;

	U32 * sTypeHeader = (U32*)(buffer+offset);
	(*sTypeHeader) = spaceType;
	offset += sizeof(U32);

	header->size = offset;
}

void FixedEmmiterProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * numberHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	number = 	EncodeParam::DecodeFloat(numberHeader);
	offset += numberHeader->size;

	EncodedFloatTypeHeader * lifetimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	lifetime = EncodeParam::DecodeFloat(lifetimeHeader);
	offset += lifetimeHeader->size;

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	location = EncodeParam::DecodeTransform(locationHeader);
	offset += locationHeader->size;

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

struct FixedEmmiterEffect : public IParticleEffect
{
	IParticleEffect * output;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * number;
	EncodedFloatTypeHeader * lifetime;
	EncodedTransformTypeHeader * location;
	PEE_Type spaceType;

	U32 numInst;
	struct InstStruct
	{
		U32 particleRegionStart;
		bool bCreated;
	};
	InstStruct * instStruct;

	U32 maxParticles;

	FixedEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~FixedEmmiterEffect();
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

FixedEmmiterEffect::FixedEmmiterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);
	
	number = (EncodedFloatTypeHeader *)(data+offset);
	offset += number->size;

	lifetime = (EncodedFloatTypeHeader *)(data+offset);
	offset+= lifetime->size;

	location = (EncodedTransformTypeHeader *)(data+offset);
	offset+= location->size;

	if(header->version <=3)//no space type
	{
		spaceType = PEE_WORLDSPACE;
	}
	else
	{
		spaceType= *((PEE_Type*)(data+offset));
		offset += sizeof(U32);
	}
	
	maxParticles = EncodeParam::GetMaxFloat(number,parentSystem);
}

FixedEmmiterEffect::~FixedEmmiterEffect()
{
	delete [] instStruct;
	if(output)
		delete output;
}

IParticleEffectInstance * FixedEmmiterEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void FixedEmmiterEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

U32 FixedEmmiterEffect::GetAllocationStart(IParticleEffectInstance * target,U32 instance)
{
	return instStruct[instance].particleRegionStart;
}

void FixedEmmiterEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
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

bool FixedEmmiterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	//bool bStillAround = false;
	if((!(instStruct[instance].bCreated))&& (!bShutdown))
	{
		instStruct[instance].bCreated = true;
		//bStillAround = true;
		Transform trans;
		Transform trans2;
		Particle * pInput = &(parentSystem->myParticles->particles2[inputStart]);
		Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
		if(EncodeParam::FindObjectTransform(parentSystem,pInput,0.0,location,trans,instance) && EncodeParam::FindObjectTransform(parentSystem,pInput,1.0,location,trans2,instance))
		{

			for(U32 i = 0; i < maxParticles; ++i)
			{
				if((!(p1[i].bLive)) )
				{
					//bStillAround = true;
					p2[i].bLive = true;
					p2[i].bNew = true;
					p2[i].bNoComputeVelocity = false;
					p2[i].birthTime = parentSystem->GetCurrentTimeMS()+(DELTA_TIME*1000*2);
					if(spaceType == PEE_WORLDSPACE)
					{
						p2[i].pos = trans.translation;
						p2[i].bParented = false;
					}
					else
					{
						p2[i].pos = Vector(0,0,0);
						p2[i].bParented = true;
					}
					p2[i].vel = Vector(0,0,0);
					p2[i].lifeTime = EncodeParam::GetFloat(lifetime,&(p2[i]),parentSystem)*1000;
				}
			}
		}
	}
	else
	{
		Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
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
					//bStillAround = true;
				}
			}
		}
	}
	if(instStruct[instance].bCreated && bShutdown)//reset the emmiter in case it will be used again.
		instStruct[instance].bCreated = false;
//	if(!bStillAround)
//		return false;
	if(output)
		return output->Update(instStruct[instance].particleRegionStart,maxParticles, bShutdown,instance);
	return true;
}

U32 FixedEmmiterEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = 0;
		instStruct[i].bCreated= false;
	}
	if(output)
		return output->ParticlesUsed()+maxParticles*numInst;
	return maxParticles*numInst;
}

void FixedEmmiterEffect::FindAllocation(U32 & startPos)
{
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = startPos;
		startPos += maxParticles;
	}
	if(output)
		output->FindAllocation(startPos);
}

void FixedEmmiterEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void FixedEmmiterEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * FixedEmmiterEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void FixedEmmiterEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInst);
}

bool FixedEmmiterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
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
			return false;
		}
	}
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

U32 FixedEmmiterEffect::OutputRange()
{
	return maxParticles;
};

#endif
