#ifndef PFSPHERESHAPER_H
#define PFSPHERESHAPER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFSphereShaper.h                               //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct SphereShaperProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * radius;
	FloatType * angle1;
	FloatType * angle2;
	AxisEnum primaryAxis;

	SphereShaperProgramer();
	~SphereShaperProgramer();

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

SphereShaperProgramer::SphereShaperProgramer()
{
	orientation = MakeDefaultTrans();
	radius = MakeDefaultRangeFloat(0,4000);
	angle1 = MakeDefaultRangeFloat(0,360);
	angle2 = MakeDefaultRangeFloat(0,180);
	primaryAxis = AXIS_I;
}

SphereShaperProgramer::~SphereShaperProgramer()
{
	delete orientation;
	delete radius;
	delete angle1;
	delete angle2;
}

//IParticleProgramer
U32 SphereShaperProgramer::GetNumOutput()
{
	return 1;
}

const char * SphereShaperProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 SphereShaperProgramer::GetNumInput()
{
	return 1;
}

const char * SphereShaperProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 SphereShaperProgramer::GetNumFloatParams()
{
	return 3;
}

const char * SphereShaperProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Radius";
	case 1:
		return "Angle1";
	case 2:
		return "Angle2";
	}
	return NULL;
}

const FloatType * SphereShaperProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return radius;
	case 1:
		return angle1;
	case 2:
		return angle2;
	}
	return NULL;
}

void SphereShaperProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(radius)
				delete radius;
			if(param)
				radius = param->CreateCopy();
			else
				radius = NULL;
			break;
		}
	case 1:
		{
			if(angle1)
				delete angle1;
			if(param)
				angle1 = param->CreateCopy();
			else
				angle1 = NULL;
			break;
		}
	case 2:
		{
			if(angle2)
				delete angle2;
			if(param)
				angle2 = param->CreateCopy();
			else
				angle2 = NULL;
			break;
		}
	}
}

U32 SphereShaperProgramer::GetNumTransformParams()
{
	return 1;
}

const char * SphereShaperProgramer::GetTransformParamName(U32 index)
{
	return "Orientation";
}

const TransformType * SphereShaperProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void SphereShaperProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 SphereShaperProgramer::GetNumEnumParams()
{
	return 1;
}

const char * SphereShaperProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Primary Axis";
	}
	return NULL;
}

U32 SphereShaperProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 3;
	}
	return 0;
}

const char * SphereShaperProgramer::GetEnumValueName(U32 index, U32 value)
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

U32 SphereShaperProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return primaryAxis;
	}
	return 0;
}

void SphereShaperProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		primaryAxis = (AxisEnum)value;
		break;
	}
}

U32 SphereShaperProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(radius) 
		+ EncodeParam::EncodedFloatSize(angle1) 
		+ EncodeParam::EncodedFloatSize(angle2) 
		+ sizeof(U32);
}

void SphereShaperProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SPHERE_SHAPER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * radiusHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(radiusHeader,radius);
	offset +=radiusHeader->size;

	EncodedFloatTypeHeader * angle1Header = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angle1Header ,angle1);
	offset +=angle1Header->size;

	EncodedFloatTypeHeader * angle2Header = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angle2Header ,angle2);
	offset +=angle2Header->size;

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = primaryAxis;
	offset += sizeof(U32);

	header->size = offset;
}

void SphereShaperProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * radiusHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	radius = EncodeParam::DecodeFloat(radiusHeader);
	offset += radiusHeader->size;

	EncodedFloatTypeHeader * angle1Header = (EncodedFloatTypeHeader *)(buffer+offset);
	angle1 = EncodeParam::DecodeFloat(angle1Header );
	offset += angle1Header ->size;

	EncodedFloatTypeHeader * angle2Header = (EncodedFloatTypeHeader *)(buffer+offset);
	angle2 = EncodeParam::DecodeFloat(angle2Header);
	offset += angle2Header->size;

	primaryAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);
}

struct SphereShaperEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * radius;
	EncodedFloatTypeHeader * angle1;
	EncodedFloatTypeHeader * angle2;
	AxisEnum primaryAxis;

	U8 * data;

	SphereShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~SphereShaperEffect();
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

SphereShaperEffect::SphereShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	radius = (EncodedFloatTypeHeader *)(data+offset);
	offset += radius->size;

	angle1 = (EncodedFloatTypeHeader *)(data+offset);
	offset += angle1->size;

	angle2 = (EncodedFloatTypeHeader *)(data+offset);
	offset += angle2->size;

	primaryAxis= *((AxisEnum *)(data+offset));
}

SphereShaperEffect::~SphereShaperEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * SphereShaperEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void SphereShaperEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void SphereShaperEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool SphereShaperEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
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
				trans.make_orthogonal();
				SINGLE r = EncodeParam::GetFloat(radius,&(p2[i]),parentSystem)*FEET_TO_WORLD;
				SINGLE a1 = EncodeParam::GetFloat(angle1,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				SINGLE a2 = EncodeParam::GetFloat(angle2,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
				SINGLE cosA1 = cos(a1);
				SINGLE sinA1 = sin(a1);
				SINGLE cosA2 = cos(a2);
				SINGLE sinA2 = sin(a2);

				Vector offset;
				if(primaryAxis == 0)//I axis
				{
					offset = Vector(r*cosA1*sinA2,r*sinA1*sinA2,r*cosA2);
				}	
				else if(primaryAxis == 1)//J axis
				{
					offset = Vector(r*cosA2,r*cosA1*sinA2,r*sinA1*sinA2);
				}
				else//K axis
				{
					offset = Vector(r*sinA1*sinA2,r*cosA2,r*cosA1*sinA2);
				}

				p2[i].pos = p2[i].pos+trans.rotate_translate(offset);
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 SphereShaperEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void SphereShaperEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void SphereShaperEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void SphereShaperEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * SphereShaperEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void SphereShaperEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool SphereShaperEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
