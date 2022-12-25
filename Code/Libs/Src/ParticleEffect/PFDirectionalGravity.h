#ifndef PFDIRECTIONALGRAVIITY_H
#define PFDIRECTIONALGRAVIITY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFDirectionalGravity.h                      //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct DirectionalGravityProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * strength;
	AxisEnum primaryAxis;

	DirectionalGravityProgramer();
	~DirectionalGravityProgramer();

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

DirectionalGravityProgramer::DirectionalGravityProgramer()
{
	orientation = MakeDefaultTrans();
	strength = MakeDefaultFloat(0);
	primaryAxis = AXIS_I;
}

DirectionalGravityProgramer::~DirectionalGravityProgramer()
{
	delete orientation;
	delete strength;
}

//IParticleProgramer
U32 DirectionalGravityProgramer::GetNumOutput()
{
	return 1;
}

const char * DirectionalGravityProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 DirectionalGravityProgramer::GetNumInput()
{
	return 1;
}

const char * DirectionalGravityProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 DirectionalGravityProgramer::GetNumFloatParams()
{
	return 1;
}

const char * DirectionalGravityProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Strength";
	}
	return NULL;
}

const FloatType * DirectionalGravityProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return strength;
	}
	return NULL;
}

void DirectionalGravityProgramer::SetFloatParam(U32 index,const FloatType * param)
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

	}
}

U32 DirectionalGravityProgramer::GetNumTransformParams()
{
	return 1;
}

const char * DirectionalGravityProgramer::GetTransformParamName(U32 index)
{
	return "Orientation";
}

const TransformType * DirectionalGravityProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void DirectionalGravityProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 DirectionalGravityProgramer::GetNumEnumParams()
{
	return 1;
}

const char * DirectionalGravityProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Primary Axis";
	}
	return NULL;
}

U32 DirectionalGravityProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 3;
	}
	return 0;
}

const char * DirectionalGravityProgramer::GetEnumValueName(U32 index, U32 value)
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

U32 DirectionalGravityProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return primaryAxis;
	}
	return 0;
}

void DirectionalGravityProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		primaryAxis = (AxisEnum)value;
		break;
	}
}

U32 DirectionalGravityProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(strength) 
		+ sizeof(U32);
}

void DirectionalGravityProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_DIRECTIONAL_GRAVITY;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(strengthHeader,strength);
	offset +=strengthHeader->size;

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = primaryAxis;
	offset += sizeof(U32);

	header->size = offset;
}

void DirectionalGravityProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	strength = EncodeParam::DecodeFloat(strengthHeader);
	offset += strengthHeader->size;

	primaryAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);
}

struct DirectionalGravityEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * strength;
	AxisEnum primaryAxis;

	U8 * data;

	DirectionalGravityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~DirectionalGravityEffect();
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

DirectionalGravityEffect::DirectionalGravityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	strength = (EncodedFloatTypeHeader *)(data+offset);
	offset += strength->size;

	primaryAxis= *((AxisEnum *)(data+offset));
}

DirectionalGravityEffect::~DirectionalGravityEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * DirectionalGravityEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void DirectionalGravityEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void DirectionalGravityEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool DirectionalGravityEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Transform trans;
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,orientation,trans,instance))
	{
		Vector direction;
		if(primaryAxis == 0)//I axis
		{
			direction = trans.get_i();
		}	
		else if(primaryAxis == 1)//J axis
		{
			direction = trans.get_j();
		}
		else//K axis
		{
			direction = trans.get_k();
		}
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive)
			{
				trans.make_orthogonal();
				SINGLE str = EncodeParam::GetFloat(strength,&(p2[i]),parentSystem)*FEET_TO_WORLD;


				p2[i].vel = p2[i].vel+(direction*str);
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 DirectionalGravityEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void DirectionalGravityEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void DirectionalGravityEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void DirectionalGravityEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * DirectionalGravityEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void DirectionalGravityEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool DirectionalGravityEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
