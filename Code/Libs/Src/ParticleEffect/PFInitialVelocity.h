#ifndef PFINITIALVELOCITY_H
#define PFINITIALVELOCITY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFInitialVelocity.h                         //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct InitialVelocityProgramer : public ParticleProgramer
{
	TransformType * orientation;

	FloatType * speed;
	FloatType * angleNoiseI;
	FloatType * angleNoiseJ;
	FloatType * angleNoiseK;

	AxisEnum primaryAxis;

	InitialVelocityProgramer();
	~InitialVelocityProgramer();

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

InitialVelocityProgramer::InitialVelocityProgramer()
{
	orientation = MakeDefaultTrans();
	speed = MakeDefaultFloat(0);
	angleNoiseI = MakeDefaultFloat(0);
	angleNoiseJ = MakeDefaultFloat(0);
	angleNoiseK = MakeDefaultFloat(0);

	primaryAxis = AXIS_I;
}

InitialVelocityProgramer::~InitialVelocityProgramer()
{
	delete orientation;
	delete speed;
	delete angleNoiseI;
	delete angleNoiseJ;
	delete angleNoiseK;
}

//IParticleProgramer
U32 InitialVelocityProgramer::GetNumOutput()
{
	return 1;
}

const char * InitialVelocityProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 InitialVelocityProgramer::GetNumInput()
{
	return 1;
}

const char * InitialVelocityProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 InitialVelocityProgramer::GetNumFloatParams()
{
	return 4;
}

const char * InitialVelocityProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Speed";
	case 1:
		return "Angle Noise I";
	case 2:
		return "Angle Noise J";
	case 3:
		return "Angle Noise K";
	}
	return NULL;
}

const FloatType * InitialVelocityProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return speed;
	case 1:
		return angleNoiseI;
	case 2:
		return angleNoiseJ;
	case 3:
		return angleNoiseK;
	}
	return NULL;
}

void InitialVelocityProgramer::SetFloatParam(U32 index,const FloatType * param)
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
			if(angleNoiseI)
				delete angleNoiseI;
			if(param)
				angleNoiseI = param->CreateCopy();
			else
				angleNoiseI = NULL;
			break;
		}
	case 2:
		{
			if(angleNoiseJ)
				delete angleNoiseJ;
			if(param)
				angleNoiseJ = param->CreateCopy();
			else
				angleNoiseJ = NULL;
			break;
		}
	case 3:
		{
			if(angleNoiseK)
				delete angleNoiseK;
			if(param)
				angleNoiseK = param->CreateCopy();
			else
				angleNoiseK = NULL;
			break;
		}
	}
}

U32 InitialVelocityProgramer::GetNumTransformParams()
{
	return 1;
}

const char * InitialVelocityProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * InitialVelocityProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void InitialVelocityProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 InitialVelocityProgramer::GetNumEnumParams()
{
	return 1;
}

const char * InitialVelocityProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Primary Axis";
	}
	return NULL;
}

U32 InitialVelocityProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 3;
	}
	return 0;
}

const char * InitialVelocityProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "I";
			case 1:
				return "J";
			case 2:
				return "K";
			}
		}
		break;
	}
	return NULL;
}

U32 InitialVelocityProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return primaryAxis;
	}
	return 0;
}

void InitialVelocityProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		primaryAxis = (AxisEnum)value;
		break;
	}
}

U32 InitialVelocityProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(speed) 
		+ EncodeParam::EncodedFloatSize(angleNoiseI) 
		+ EncodeParam::EncodedFloatSize(angleNoiseJ) 
		+ EncodeParam::EncodedFloatSize(angleNoiseK) 
		+ sizeof(U32);
}

void InitialVelocityProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_INITIAL_VELOCITY;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(speedHeader,speed);
	offset +=speedHeader->size;

	EncodedFloatTypeHeader * angleNoiseIHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseIHeader ,angleNoiseI);
	offset +=angleNoiseIHeader->size;

	EncodedFloatTypeHeader * angleNoiseJHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseJHeader ,angleNoiseJ);
	offset +=angleNoiseJHeader->size;

	EncodedFloatTypeHeader * angleNoiseKHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleNoiseKHeader ,angleNoiseK);
	offset +=angleNoiseKHeader->size;

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = primaryAxis;
	offset += sizeof(U32);

	header->size = offset;
}

void InitialVelocityProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	speed = EncodeParam::DecodeFloat(speedHeader);
	offset += speedHeader->size;

	EncodedFloatTypeHeader * angleNoiseIHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseI = EncodeParam::DecodeFloat(angleNoiseIHeader );
	offset += angleNoiseIHeader ->size;

	EncodedFloatTypeHeader * angleNoiseJHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseJ = EncodeParam::DecodeFloat(angleNoiseJHeader);
	offset += angleNoiseJHeader->size;

	EncodedFloatTypeHeader * angleNoiseKHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angleNoiseK = EncodeParam::DecodeFloat(angleNoiseKHeader);
	offset += angleNoiseKHeader->size;

	primaryAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);
}

struct InitialVelocityEffect : public IParticleEffect
{
	IParticleEffect * output;
	char effectName[32];

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * speed;
	EncodedFloatTypeHeader * angleNoiseI;
	EncodedFloatTypeHeader * angleNoiseJ;
	EncodedFloatTypeHeader * angleNoiseK;

	AxisEnum primaryAxis;

	U8 * data;

	InitialVelocityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~InitialVelocityEffect();
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

InitialVelocityEffect::InitialVelocityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	speed = (EncodedFloatTypeHeader *)(data+offset);
	offset += speed->size;

	angleNoiseI = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseI->size;

	angleNoiseJ = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseJ->size;

	angleNoiseK = (EncodedFloatTypeHeader *)(data+offset);
	offset += angleNoiseK->size;

	primaryAxis= *((AxisEnum*)(data+offset));
	offset += sizeof(U32);
}

InitialVelocityEffect::~InitialVelocityEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * InitialVelocityEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void InitialVelocityEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void InitialVelocityEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool InitialVelocityEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Transform trans;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,orientation,trans,instance))
	{
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive && p2[i].bNew)
			{
				Transform noise = trans;
				SINGLE angleNoise = EncodeParam::GetFloat(angleNoiseI,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.x_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				angleNoise = EncodeParam::GetFloat(angleNoiseJ,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.y_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				angleNoise = EncodeParam::GetFloat(angleNoiseK,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				noise.z_rotate_right(getRandRange(0,angleNoise)-(angleNoise*0.5));
				Vector baseVel;
				noise.make_orthogonal();
				if(primaryAxis == AXIS_I)//I axis
					 baseVel= noise.get_i();
				else if(primaryAxis == AXIS_J)//J axis
					 baseVel= noise.get_j();
				else//K axis
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

U32 InitialVelocityEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void InitialVelocityEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void InitialVelocityEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void InitialVelocityEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * InitialVelocityEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void InitialVelocityEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool InitialVelocityEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
