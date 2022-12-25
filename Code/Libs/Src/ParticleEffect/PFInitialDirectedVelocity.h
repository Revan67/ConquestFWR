#ifndef PFINITIALDIRECTEDVELOCITY_H
#define PFINITIALDIRECTEDVELOCITY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                      PFInitialDirectedVelocity.h                         //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct InitialDirectedVelocityProgramer : public ParticleProgramer
{
	TransformType * target;

	FloatType * speed;
	FloatType * angleNoiseUp;
	FloatType * angleNoiseSide;
	FloatType * angleNoiseSpin;

	InitialDirectedVelocityProgramer();
	~InitialDirectedVelocityProgramer();

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

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

InitialDirectedVelocityProgramer::InitialDirectedVelocityProgramer()
{
	target = MakeDefaultTrans();
	speed = MakeDefaultFloat(0);
	angleNoiseUp = MakeDefaultFloat(0);
	angleNoiseSide = MakeDefaultFloat(0);
	angleNoiseSpin = MakeDefaultFloat(0);
}

InitialDirectedVelocityProgramer::~InitialDirectedVelocityProgramer()
{
	delete target;
	delete speed;
	delete angleNoiseUp;
	delete angleNoiseSide;
	delete angleNoiseSpin;
}

//IParticleProgramer
U32 InitialDirectedVelocityProgramer::GetNumOutput()
{
	return 1;
}

const char * InitialDirectedVelocityProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 InitialDirectedVelocityProgramer::GetNumInput()
{
	return 1;
}

const char * InitialDirectedVelocityProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 InitialDirectedVelocityProgramer::GetNumFloatParams()
{
	return 4;
}

const char * InitialDirectedVelocityProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Speed";
	case 1:
		return "Angle Noise Up";
	case 2:
		return "Angle Noise Side";
	case 3:
		return "Angle Noise Spin";
	}
	return NULL;
}

const FloatType * InitialDirectedVelocityProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return speed;
	case 1:
		return angleNoiseUp;
	case 2:
		return angleNoiseSide;
	case 3:
		return angleNoiseSpin;
	}
	return NULL;
}

void InitialDirectedVelocityProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(speed)
				delete speed;
			if(param)
				speed = param->CreateCopy();
			else
				speed = NULL;
			break;
		}
	case 1:
		{
			if(angleNoiseUp)
				delete angleNoiseUp;
			if(param)
				angleNoiseUp = param->CreateCopy();
			else
				angleNoiseUp = NULL;
			break;
		}
	case 2:
		{
			if(angleNoiseSide)
				delete angleNoiseSide;
			if(param)
				angleNoiseSide = param->CreateCopy();
			else
				angleNoiseSide = NULL;
			break;
		}
	case 3:
		{
			if(angleNoiseSpin)
				delete angleNoiseSpin;
			if(param)
				angleNoiseSpin = param->CreateCopy();
			else
				angleNoiseSpin = NULL;
			break;
		}
	}
}

U32 InitialDirectedVelocityProgramer::GetNumTransformParams()
{
	return 1;
}

const char * InitialDirectedVelocityProgramer::GetTransformParamName(U32 index)
{
	return "Target Transform";
}

const TransformType * InitialDirectedVelocityProgramer::GetTransformParam(U32 index)
{
	return target;
}

void InitialDirectedVelocityProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(target)
		delete target;
	if(param)
		target = param->CreateCopy();
	else
		target = NULL;
}

U32 InitialDirectedVelocityProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(target)
		+ EncodeParam::EncodedFloatSize(speed) 
		+ EncodeParam::EncodedFloatSize(angleNoiseUp) 
		+ EncodeParam::EncodedFloatSize(angleNoiseSide) 
		+ EncodeParam::EncodedFloatSize(angleNoiseSpin);
}

void InitialDirectedVelocityProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_INITIAL_DIRECTED_VELOCITY;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * targetHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(targetHeader ,target);
	offset +=targetHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(speedHeader,speed);
	offset +=speedHeader->size;

	EncodedFloatTypeHeader * angleNoiseUpHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseUpHeader ,angleNoiseUp);
	offset +=angleNoiseUpHeader->size;

	EncodedFloatTypeHeader * angleNoiseSideHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseSideHeader ,angleNoiseSide);
	offset +=angleNoiseSideHeader->size;

	EncodedFloatTypeHeader * angleNoiseSpinHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseSpinHeader ,angleNoiseSpin);
	offset +=angleNoiseSpinHeader->size;

	header->size = offset;
}

void InitialDirectedVelocityProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedTransformTypeHeader * targetHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	target = EncodeParam::DecodeTransform(targetHeader);
	offset += targetHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	speed = EncodeParam::DecodeFloat(speedHeader);
	offset += speedHeader->size;

	EncodedFloatTypeHeader * angleNoiseUpHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseUp = EncodeParam::DecodeFloat(angleNoiseUpHeader );
	offset += angleNoiseUpHeader ->size;

	EncodedFloatTypeHeader * angleNoiseSideHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseSide = EncodeParam::DecodeFloat(angleNoiseSideHeader);
	offset += angleNoiseSideHeader->size;

	EncodedFloatTypeHeader * angleNoiseSpinHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseSpin = EncodeParam::DecodeFloat(angleNoiseSpinHeader);
	offset += angleNoiseSpinHeader->size;
}

struct InitialDirectedVelocityEffect : public IParticleEffect
{
	IParticleEffect * output;
	char effectName[32];

	EncodedTransformTypeHeader * target;
	EncodedFloatTypeHeader * speed;
	EncodedFloatTypeHeader * angleNoiseUp;
	EncodedFloatTypeHeader * angleNoiseSide;
	EncodedFloatTypeHeader * angleNoiseSpin;

	U8 * data;

	InitialDirectedVelocityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~InitialDirectedVelocityEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual void SetInstanceNumber(U32 numInstances);
};

InitialDirectedVelocityEffect::InitialDirectedVelocityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	target = (EncodedTransformTypeHeader *)(data+offset);
	offset += target->size;

	speed = (EncodedFloatTypeHeader *)(data+offset);
	offset += speed->size;

	angleNoiseUp = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseUp->size;

	angleNoiseSide = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseSide->size;

	angleNoiseSpin = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseSpin->size;
}

InitialDirectedVelocityEffect::~InitialDirectedVelocityEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * InitialDirectedVelocityEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void InitialDirectedVelocityEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void InitialDirectedVelocityEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool InitialDirectedVelocityEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Transform trans;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,target,trans,instance))
	{
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive && p2[i].bNew)
			{
				Transform noise;
				Vector vk = trans.translation-(p2[i].pos);
				vk.fast_normalize();
				Vector vj = Vector(0,0,1);
				Vector vi = cross_product(vj,vk);
				vi.fast_normalize();
				vj = cross_product(vk,vi);
				vj.fast_normalize();
				noise.set_i(vi);
				noise.set_j(vj);
				noise.set_k(vk);
				noise.translation = Vector(0,0,0);

				SINGLE angleNoise = EncodeParam::GetFloat(angleNoiseUp,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.x_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				angleNoise = EncodeParam::GetFloat(angleNoiseSide,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.y_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				angleNoise = EncodeParam::GetFloat(angleNoiseSpin,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.z_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				Vector baseVel;
				noise.make_orthogonal();
				baseVel= noise.get_k();
				p2[i].vel= baseVel*(EncodeParam::GetFloat(speed,&(p2[i]),parentSystem)*FEET_TO_WORLD);
				SINGLE primeTime = ( (parentSystem->GetCurrentTimeMS()+(DELTA_TIME*1000)) -p2[i].birthTime)/1000.0;
				p2[i].pos = p2[i].pos+(p2[i].vel*( primeTime));
			}
		}
	}
	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 InitialDirectedVelocityEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void InitialDirectedVelocityEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void InitialDirectedVelocityEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void InitialDirectedVelocityEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * InitialDirectedVelocityEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void InitialDirectedVelocityEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool InitialDirectedVelocityEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
