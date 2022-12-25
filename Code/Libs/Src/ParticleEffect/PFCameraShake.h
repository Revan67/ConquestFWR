#ifndef PFCAMERASHAKE_H
#define PFCAMERASHAKE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFCameraShake.h                             //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct CameraShakeProgramer : public ParticleProgramer
{
	FloatType * durration;
	FloatType * power;

	CameraShakeProgramer();
	~CameraShakeProgramer();

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

CameraShakeProgramer::CameraShakeProgramer()
{
	durration = MakeDefaultFloat(1);
	power = MakeDefaultFloat(1);
}

CameraShakeProgramer::~CameraShakeProgramer()
{
	delete durration;
	delete power;
}

//IParticleProgramer
U32 CameraShakeProgramer::GetNumOutput()
{
	return 1;
}

const char * CameraShakeProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CameraShakeProgramer::GetNumInput()
{
	return 1;
}

const char * CameraShakeProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CameraShakeProgramer::GetNumFloatParams()
{
	return 2;
}

const char * CameraShakeProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Durration";
	case 1:
		return "Power";
	}
	return NULL;
}

const FloatType * CameraShakeProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return durration;
	case 1:
		return power;
	}
	return NULL;
}

void CameraShakeProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(durration)
				delete durration;
			if(param)
				durration = param->CreateCopy();
			else
				durration = NULL;
			break;
		}
	case 1:
		{
			if(power)
				delete power;
			if(param)
				power = param->CreateCopy();
			else
				power = NULL;
			break;
		}
	}
}

U32 CameraShakeProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(durration)
		+ EncodeParam::EncodedFloatSize(power);
}

void CameraShakeProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_CAMERA_SHAKE;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * durrationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(durrationHeader,durration);
	offset += durrationHeader->size;

	EncodedFloatTypeHeader * powerHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(powerHeader,power);
	offset += powerHeader->size;

	header->size = offset;
}

void CameraShakeProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * durrationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	durration = 	EncodeParam::DecodeFloat(durrationHeader);
	offset += durrationHeader->size;

	EncodedFloatTypeHeader * powerHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	power = 	EncodeParam::DecodeFloat(powerHeader);
	offset += powerHeader->size;

}

struct CameraShakeEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedFloatTypeHeader * durration;
	EncodedFloatTypeHeader * power;

	U8 * data;

	CameraShakeEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer, U32 inputID);
	virtual ~CameraShakeEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

CameraShakeEffect::CameraShakeEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	durration = (EncodedFloatTypeHeader *)(data+offset);
	offset += durration->size;

	power = (EncodedFloatTypeHeader *)(data+offset);
	offset += power->size;

	output = 0;
}

CameraShakeEffect::~CameraShakeEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * CameraShakeEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void CameraShakeEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
}

void CameraShakeEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool CameraShakeEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(!bShutdown)
	{
		Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
		SINGLE durTime = EncodeParam::GetFloat(durration,p2,parentSystem);
		SINGLE powerValue = EncodeParam::GetFloat(power,p2,parentSystem);
		parentSystem->GetOwner()->GetCallback()->ShakeCamera(durTime,powerValue);
	}
	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	if(bShutdown)
		return false;
	return true;
}

U32 CameraShakeEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void CameraShakeEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void CameraShakeEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * CameraShakeEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void CameraShakeEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool CameraShakeEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
