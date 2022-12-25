#ifndef PFDISTANCEDELTAHITTER_H
#define PFDISTANCEDELTAHITTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFDistanceDeltaHitter.h                     //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////
enum DDHE_CheckType
{
	DDHE_Closer,
	DDHE_Farther,
};

struct DistanceDeltaHitterProgramer : public ParticleProgramer
{
	FloatType * radius;
	TransformType * location;

	DDHE_CheckType checkType;

	DistanceDeltaHitterProgramer();
	~DistanceDeltaHitterProgramer();

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

DistanceDeltaHitterProgramer::DistanceDeltaHitterProgramer()
{
	radius = MakeDefaultFloat(0);
	location = MakeDefaultTrans();
	checkType = DDHE_Farther;
}

DistanceDeltaHitterProgramer::~DistanceDeltaHitterProgramer()
{
	delete radius;
	delete location;
}

//IParticleProgramer
U32 DistanceDeltaHitterProgramer::GetNumOutput()
{
	return 2;
}

const char * DistanceDeltaHitterProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return FI_POINT_LIST;
	case 1:
		return FI_EVENT;
	}
	return "Error";
}

U32 DistanceDeltaHitterProgramer::GetNumInput()
{
	return 1;
}

const char * DistanceDeltaHitterProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 DistanceDeltaHitterProgramer::GetNumFloatParams()
{
	return 1;
}

const char * DistanceDeltaHitterProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Radius";
	}
	return NULL;
}

const FloatType * DistanceDeltaHitterProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return radius;
	}
	return NULL;
}

void DistanceDeltaHitterProgramer::SetFloatParam(U32 index,const FloatType * param)
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
	}
}

U32 DistanceDeltaHitterProgramer::GetNumTransformParams()
{
	return 1;
}

const char * DistanceDeltaHitterProgramer::GetTransformParamName(U32 index)
{
	return "Location";
}

const TransformType * DistanceDeltaHitterProgramer::GetTransformParam(U32 index)
{
	return location;
}

void DistanceDeltaHitterProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(location)
		delete location;
	if(param)
		location = param->CreateCopy();
	else
		location = NULL;
}

U32 DistanceDeltaHitterProgramer::GetNumEnumParams()
{
	return 1;
}

const char * DistanceDeltaHitterProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Check Type";
	}
	return NULL;
}

U32 DistanceDeltaHitterProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * DistanceDeltaHitterProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Closer";
			case 1:
				return "Farther";
			}
		}
		break;
	}
	return NULL;
}

U32 DistanceDeltaHitterProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return checkType;
	}
	return 0;
}

void DistanceDeltaHitterProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		checkType = (DDHE_CheckType)value;
		break;
	}
}

U32 DistanceDeltaHitterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(location)
		+ EncodeParam::EncodedFloatSize(radius)
		+ sizeof(U32);
}

void DistanceDeltaHitterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_DISTANCE_DELTA_HITTER;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(locationHeader ,location);
	offset +=locationHeader->size;

	EncodedFloatTypeHeader * radiusHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(radiusHeader,radius);
	offset = offset + radiusHeader->size;

	U32 * checkTypeHeader = (U32*)(buffer+offset);
	(*checkTypeHeader) = checkType;
	offset += sizeof(U32);

	header->size = offset;
}

void DistanceDeltaHitterProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	location = EncodeParam::DecodeTransform(locationHeader);
	offset += locationHeader->size;

	EncodedFloatTypeHeader * radiusHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	radius = EncodeParam::DecodeFloat(radiusHeader);
	offset+= radiusHeader->size;

	checkType = *((DDHE_CheckType*)(buffer+offset));
	offset += sizeof(U32);
}

struct DistanceDeltaHitterEffect : public IParticleEffect
{
	IParticleEffect * output;
	IParticleEffect * collisionEvent;

	EncodedTransformTypeHeader * location;
	EncodedFloatTypeHeader * radius;

	U32 checkType;

	U8 * data;

	DistanceDeltaHitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer, U32 inputID);
	virtual ~DistanceDeltaHitterEffect();
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

DistanceDeltaHitterEffect::DistanceDeltaHitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	data = buffer;

	location = (EncodedTransformTypeHeader *)(data+offset);
	offset += location->size;

	radius = (EncodedFloatTypeHeader *)(data+offset);
	offset += radius->size; 

	checkType = *((DDHE_CheckType*)(buffer+offset));
	offset += sizeof(U32);

	output = 0;
	collisionEvent = 0;
}

DistanceDeltaHitterEffect::~DistanceDeltaHitterEffect()
{
	if(output)
		delete output;
	if(collisionEvent)
		delete collisionEvent;
}

IParticleEffectInstance * DistanceDeltaHitterEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return output;
	}
	if(outputID == 1)
	{
		if(collisionEvent)
			delete collisionEvent;
		collisionEvent = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return collisionEvent;
	}
	return NULL;
}

void DistanceDeltaHitterEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
	if(outputID == 1)
	{
		if(collisionEvent)
			delete collisionEvent;
		collisionEvent = (IParticleEffect*)target;
		collisionEvent->parentEffect[inputID] = this;
	}
}

void DistanceDeltaHitterEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
	if(collisionEvent)
		collisionEvent->Render(t,inputStart,numInput,instance,parentTrans);
}

bool DistanceDeltaHitterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(collisionEvent)
	{
		Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
			{
				if(!(p1[i].pos == p2[i].pos))
				{
					Transform trans;
					if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0,location,trans,instance))
					{
						SINGLE rad = EncodeParam::GetFloat(radius,&(p2[i]),parentSystem)*FEET_TO_WORLD;
						rad = rad*rad;

						SINGLE dist1 = (p1[i].pos-trans.translation).magnitude_squared();
						SINGLE dist2 = (p2[i].pos-trans.translation).magnitude_squared();
						if(dist1 < rad || dist2 < rad)
						{
							if(checkType == DDHE_Closer)
							{
								if(dist2 < dist1)
								{
									if(collisionEvent->ParticleEvent(p2[i].pos,p2[i].vel,instance))
									{
										p1[i].bLive = false;
										p2[i].bLive = false;
									}							
								}
							}
							else //DDHE_Farther
							{
								if(dist2 > dist1)
								{
									if(collisionEvent->ParticleEvent(p2[i].pos,p2[i].vel,instance))
									{
										p1[i].bLive = false;
										p2[i].bLive = false;
									}							
								}
							}
						}
					}				
				}
			}
		}
	}
	bool bRetVal1 = true;
	bool bRetVal2 = false;
	if(output)
		bRetVal1 = output->Update(inputStart,numInput, bShutdown,instance);
	if(collisionEvent)
		bRetVal2 = collisionEvent->Update(inputStart,numInput, bShutdown,instance);
	return bRetVal1 | bRetVal2;
}

U32 DistanceDeltaHitterEffect::ParticlesUsed()
{
	U32 used = 0;
	if(output)
		used += output->ParticlesUsed();
	if(collisionEvent)
		used += collisionEvent->ParticlesUsed();
	return used;
}

void DistanceDeltaHitterEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
	if(collisionEvent)
		collisionEvent->FindAllocation(startPos);
}

void DistanceDeltaHitterEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
	if(collisionEvent)
	{
		delete collisionEvent;
		collisionEvent = NULL;
	}
}

IParticleEffect * DistanceDeltaHitterEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * posible = NULL;
	if(output)
		posible = output->FindFilter(searchName);
	if(posible)
		return posible;
	if(collisionEvent)
		return collisionEvent->FindFilter(searchName);
	return NULL;
}

void DistanceDeltaHitterEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
	if(collisionEvent)
		collisionEvent->SetInstanceNumber(numInstances);
}

bool DistanceDeltaHitterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		if(output->GetParentPosition(index,postion,lastIndex))
			return true;
	if(collisionEvent)
		return collisionEvent->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
