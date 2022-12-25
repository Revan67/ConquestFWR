#ifndef PFEVENTEXPLOSION_H
#define PFEVENTEXPLOSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFEventExplosion.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct PEventExplosionProgramer : public ParticleProgramer
{
	FloatType * strength;
	FloatType * speed;
	FloatType * widthInSeconds;

	PEventExplosionProgramer();
	~PEventExplosionProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

PEventExplosionProgramer::PEventExplosionProgramer()
{
	strength = MakeDefaultFloat(1);
	speed = MakeDefaultFloat(10);
	widthInSeconds = MakeDefaultFloat(1);
}

PEventExplosionProgramer::~PEventExplosionProgramer()
{
	delete strength;
	delete speed;
	delete widthInSeconds;
}

//IParticleProgramer
U32 PEventExplosionProgramer::GetNumOutput()
{
	return 1;
}

const char * PEventExplosionProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return "PEvent Passthrough";
	}
	return NULL;
}

U32 PEventExplosionProgramer::GetNumFloatParams()
{
	return 3;
}

const char * PEventExplosionProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Strength";
	case 1:
		return "Speed";
	case 2:
		return "Width In Seconds";
	}
	return NULL;
}

const FloatType * PEventExplosionProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return strength;
	case 1:
		return speed;
	case 2:
		return widthInSeconds;
	}
	return NULL;
}

void PEventExplosionProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(strength)
				delete strength;
			if(param)
				strength = param->CreateCopy();
			else
				strength = NULL;
			break;
		}
	case 1:
		{
			if(speed)
				delete speed;
			if(param)
				speed = param->CreateCopy();
			else
				speed = NULL;
			break;
		}
	case 2:
		{
			if(widthInSeconds)
				delete widthInSeconds;
			if(param)
				widthInSeconds = param->CreateCopy();
			else
				widthInSeconds = NULL;
			break;
		}
	}
}

U32 PEventExplosionProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)+ EncodeParam::EncodedFloatSize(strength) 
		+ EncodeParam::EncodedFloatSize(speed)+ EncodeParam::EncodedFloatSize(widthInSeconds);
}

void PEventExplosionProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_PEVENT_EXPLOSION;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);
	
	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(strengthHeader,strength);
	offset += strengthHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(speedHeader,speed);
	offset += speedHeader->size;

	EncodedFloatTypeHeader * widthInSecondsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(widthInSecondsHeader,widthInSeconds);
	offset += widthInSecondsHeader->size;

	header->size = offset;
}

void PEventExplosionProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	strength = EncodeParam::DecodeFloat(strengthHeader);
	offset += strengthHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	speed = EncodeParam::DecodeFloat(speedHeader);
	offset += speedHeader->size;

	EncodedFloatTypeHeader * widthInSecondsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	widthInSeconds = EncodeParam::DecodeFloat(widthInSecondsHeader);
	offset += widthInSecondsHeader->size;
}


struct PEventExplosionEffect : public IParticleEffect
{
	IParticleEffect * pEventPassThrough;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * strength;
	EncodedFloatTypeHeader * speed;
	EncodedFloatTypeHeader * widthInSeconds;

	PEventExplosionEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~PEventExplosionEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);

	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual bool ParticleEvent(const Vector & position, const Vector &velocity,U32 instance);
};

PEventExplosionEffect::PEventExplosionEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	pEventPassThrough = NULL;

	data = new U8[header->size];
	memcpy(data,buffer,header->size);

	U32 offset = sizeof(ParticleHeader);

	strength = (EncodedFloatTypeHeader *)(data+offset);
	offset += strength->size;

	speed = (EncodedFloatTypeHeader *)(data+offset);
	offset += speed->size;

	widthInSeconds = (EncodedFloatTypeHeader *)(data+offset);
	offset += widthInSeconds->size;
}

PEventExplosionEffect::~PEventExplosionEffect()
{
	delete data;
	if(pEventPassThrough)
		delete pEventPassThrough;
}

IParticleEffectInstance * PEventExplosionEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(pEventPassThrough)
			delete pEventPassThrough;
		pEventPassThrough = createParticalEffect(parentSystem,type,buffer,this);
		return pEventPassThrough;
	}
	return NULL;
}

void PEventExplosionEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(pEventPassThrough)
		pEventPassThrough->Render(t,inputStart,numInput,instance,parentTrans);
}

bool PEventExplosionEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(pEventPassThrough)
		return pEventPassThrough->Update(inputStart,numInput, bShutdown,instance);
	return false;
}

U32 PEventExplosionEffect::ParticlesUsed()
{
	if(pEventPassThrough)
		return pEventPassThrough->ParticlesUsed();
	return 0;
}

void PEventExplosionEffect::FindAllocation(U32 & startPos)
{
	if(pEventPassThrough)
		pEventPassThrough->FindAllocation(startPos);
}

void PEventExplosionEffect::DeleteOutput()
{
	if(pEventPassThrough)
	{
		delete pEventPassThrough;
		pEventPassThrough = NULL;
	}
}

IParticleEffect * PEventExplosionEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	if(pEventPassThrough)
		return pEventPassThrough->FindFilter(searchName);
	return NULL;
}

void PEventExplosionEffect::SetInstanceNumber(U32 numInstances)
{
	if(pEventPassThrough)
		pEventPassThrough->SetInstanceNumber(numInstances);
}

bool PEventExplosionEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(pEventPassThrough)
		return pEventPassThrough->GetParentPosition(index,postion,lastIndex);
	return false;
}

bool PEventExplosionEffect::ParticleEvent(const Vector & position, const Vector &velocity,U32 instance)
{
	SINGLE strengthVal = EncodeParam::GetFloat(strength,NULL,parentSystem);
	SINGLE speedVal= EncodeParam::GetFloat(speed,NULL,parentSystem);
	SINGLE widthInSecondsVal= EncodeParam::GetFloat(widthInSeconds,NULL,parentSystem);
	GMGRANNY->CreateWindExplosion(position,strengthVal,speedVal,widthInSecondsVal);
	if(pEventPassThrough)
		return pEventPassThrough->ParticleEvent(position,velocity,instance);
	return false;
}

#endif
