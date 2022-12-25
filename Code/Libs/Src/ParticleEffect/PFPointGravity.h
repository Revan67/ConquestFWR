#ifndef PFPOINTGRAVIITY_H
#define PFPOINTGRAVIITY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFPointGravity.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

enum PGravType
{
	PGE_DISTANCE,
	PGE_NO_DISTANCE,
};

struct PointGravityProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * strength;
	PGravType gravType;

	PointGravityProgramer();
	~PointGravityProgramer();

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

PointGravityProgramer::PointGravityProgramer()
{
	orientation = MakeDefaultTrans();
	strength = MakeDefaultFloat(0);
	gravType = PGE_DISTANCE;
}

PointGravityProgramer::~PointGravityProgramer()
{
	delete orientation;
	delete strength;
}

//IParticleProgramer
U32 PointGravityProgramer::GetNumOutput()
{
	return 1;
}

const char * PointGravityProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 PointGravityProgramer::GetNumInput()
{
	return 1;
}

const char * PointGravityProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 PointGravityProgramer::GetNumFloatParams()
{
	return 1;
}

const char * PointGravityProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Strength";
	}
	return NULL;
}

const FloatType * PointGravityProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return strength;
	}
	return NULL;
}

void PointGravityProgramer::SetFloatParam(U32 index,const FloatType * param)
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

U32 PointGravityProgramer::GetNumTransformParams()
{
	return 1;
}

const char * PointGravityProgramer::GetTransformParamName(U32 index)
{
	return "Location";
}

const TransformType * PointGravityProgramer::GetTransformParam(U32 index)
{
	return orientation;
}


void PointGravityProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 PointGravityProgramer::GetNumEnumParams()
{
	return 1;
}

const char * PointGravityProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Gravity Type";
	}
	return NULL;
}

U32 PointGravityProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * PointGravityProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Use Distance";
			case 1:
				return "No Distance";
			}
		}
		break;
	}
	return NULL;
}

U32 PointGravityProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return gravType;
	}
	return 0;
}

void PointGravityProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		gravType = (PGravType)value;
		break;
	}
}

U32 PointGravityProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(strength)
		+ sizeof(S32);
}

void PointGravityProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_POINT_GRAVITY;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(strengthHeader,strength);
	offset +=strengthHeader->size;

	U32 * oGravType = (U32*)(buffer+offset);
	(*oGravType) = gravType;
	offset += sizeof(U32);

	header->size = offset;
}

void PointGravityProgramer::SetDataChunk(U8 * buffer)
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

	gravType = *((PGravType*)(buffer+offset));
	offset += sizeof(U32);
}

struct PointGravityEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * strength;
	PGravType gravType;

	U8 * data;

	PointGravityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~PointGravityEffect();
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

PointGravityEffect::PointGravityEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
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

	gravType = *((PGravType*)(data+offset));
	offset += sizeof(U32);
}

PointGravityEffect::~PointGravityEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * PointGravityEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void PointGravityEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void PointGravityEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool PointGravityEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Transform trans;
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,orientation,trans,instance))
	{
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive)
			{
				Vector direction = trans.translation-p2[i].pos;
				if(gravType == PGE_NO_DISTANCE)
				{
					SINGLE dirMag = direction.fast_magnitude();
					if(dirMag)
					{
						direction /= dirMag;
						SINGLE str = EncodeParam::GetFloat(strength,&(p2[i]),parentSystem)*FEET_TO_WORLD;
						p2[i].vel = p2[i].vel+(direction*str);
					}
				}
				else
				{
					SINGLE dirMag = direction.fast_magnitude();
					if(dirMag)
					{
						direction /= dirMag;
						dirMag = dirMag/4000;//get it to a "good scale"
						SINGLE str = EncodeParam::GetFloat(strength,&(p2[i]),parentSystem)*FEET_TO_WORLD;
						p2[i].vel = p2[i].vel+(direction*(str/(dirMag*dirMag)));
					}
				}
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 PointGravityEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void PointGravityEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void PointGravityEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void PointGravityEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * PointGravityEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void PointGravityEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool PointGravityEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
