#ifndef PFACCELERATION_H
#define PFACCELERATION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFAcceleration.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct AccelerationProgramer : public ParticleProgramer
{
	FloatType * accel;

	AccelerationProgramer();
	~AccelerationProgramer();

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

AccelerationProgramer::AccelerationProgramer()
{
	accel = MakeDefaultFloat(0);
}

AccelerationProgramer::~AccelerationProgramer()
{
	delete accel;
}

//IParticleProgramer
U32 AccelerationProgramer::GetNumOutput()
{
	return 1;
}

const char * AccelerationProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 AccelerationProgramer::GetNumInput()
{
	return 1;
}

const char * AccelerationProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 AccelerationProgramer::GetNumFloatParams()
{
	return 1;
}

const char * AccelerationProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Acceleration";
	}
	return NULL;
}

const FloatType * AccelerationProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return accel;
	}
	return NULL;
}

void AccelerationProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(accel)
				delete accel;
			if(param)
				accel = param->CreateCopy();
			else
				accel = NULL;
			break;
		}
	}
}

U32 AccelerationProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(accel);
}

void AccelerationProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_ACCELERATION;
	strcpy(header->effectName,effectName);

	EncodedFloatTypeHeader * accelHeader = (EncodedFloatTypeHeader *)(buffer+sizeof(ParticleHeader));
	EncodeParam::EncodeFloat(accelHeader,accel);

	header->size = sizeof(ParticleHeader)+accelHeader->size;
}

void AccelerationProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	EncodedFloatTypeHeader * accelHeader = (EncodedFloatTypeHeader *)(buffer+sizeof(ParticleHeader));
	accel = 	EncodeParam::DecodeFloat(accelHeader);
}

struct AccelerationEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedFloatTypeHeader * accel;

	U8 * data;

	AccelerationEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer, U32 inputID);
	virtual ~AccelerationEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

AccelerationEffect::AccelerationEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	accel = (EncodedFloatTypeHeader *)(data+sizeof(ParticleHeader));

	output = 0;
}

AccelerationEffect::~AccelerationEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * AccelerationEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void AccelerationEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void AccelerationEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool AccelerationEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	for (U16 i = 0; i < numInput; i++)
	{
		if(p2[i].bLive)
		{
			SINGLE vel = p2[i].vel.fast_magnitude();
			if(vel)
			{
				SINGLE newVel = (vel+(EncodeParam::GetFloat(accel,&(p2[i]),parentSystem)*DELTA_TIME));
				if(newVel < 0)
					newVel = 0;
				p2[i].vel = p2[i].vel.fast_normalize() * newVel;			
			}
		}
	}
	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 AccelerationEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void AccelerationEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void AccelerationEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void AccelerationEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * AccelerationEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void AccelerationEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool AccelerationEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
