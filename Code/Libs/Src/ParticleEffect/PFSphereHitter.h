#ifndef PFSPHEREHITTER_H
#define PFSPHEREHITTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFSphereHitter.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct SphereHitterProgramer : public ParticleProgramer
{
	FloatType * radius;
	TransformType * location;

	SphereHitterProgramer();
	~SphereHitterProgramer();

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

SphereHitterProgramer::SphereHitterProgramer()
{
	radius = MakeDefaultFloat(0);
	location = MakeDefaultTrans();
}

SphereHitterProgramer::~SphereHitterProgramer()
{
	delete radius;
	delete location;
}

//IParticleProgramer
U32 SphereHitterProgramer::GetNumOutput()
{
	return 2;
}

const char * SphereHitterProgramer::GetOutputName(U32 index)
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

U32 SphereHitterProgramer::GetNumInput()
{
	return 1;
}

const char * SphereHitterProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 SphereHitterProgramer::GetNumFloatParams()
{
	return 1;
}

const char * SphereHitterProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Radius";
	}
	return NULL;
}

const FloatType * SphereHitterProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return radius;
	}
	return NULL;
}

void SphereHitterProgramer::SetFloatParam(U32 index,const FloatType * param)
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

U32 SphereHitterProgramer::GetNumTransformParams()
{
	return 1;
}

const char * SphereHitterProgramer::GetTransformParamName(U32 index)
{
	return "Location";
}

const TransformType * SphereHitterProgramer::GetTransformParam(U32 index)
{
	return location;
}

void SphereHitterProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(location)
		delete location;
	if(param)
		location = param->CreateCopy();
	else
		location = NULL;
}

U32 SphereHitterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(location)
		+ EncodeParam::EncodedFloatSize(radius);
}

void SphereHitterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SPHERE_HITTER;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * locationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(locationHeader ,location);
	offset +=locationHeader->size;

	EncodedFloatTypeHeader * radiusHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(radiusHeader,radius);
	offset = offset + radiusHeader->size;

	header->size = offset;
}

void SphereHitterProgramer::SetDataChunk(U8 * buffer)
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
}

struct SphereHitterEffect : public IParticleEffect
{
	IParticleEffect * output;
	IParticleEffect * collisionEvent;

	EncodedTransformTypeHeader * location;
	EncodedFloatTypeHeader * radius;

	U8 * data;

	SphereHitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~SphereHitterEffect();
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

SphereHitterEffect::SphereHitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	data = buffer;

	location = (EncodedTransformTypeHeader *)(data+offset);
	offset += location->size;

	radius = (EncodedFloatTypeHeader *)(data+offset);
	offset += radius->size; 

	output = 0;
	collisionEvent = 0;
}

SphereHitterEffect::~SphereHitterEffect()
{
	if(output)
		delete output;
	if(collisionEvent)
		delete collisionEvent;
}

IParticleEffectInstance * SphereHitterEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void SphereHitterEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
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
		collisionEvent =(IParticleEffect*)target;
		collisionEvent->parentEffect[inputID] = this;
	}
};


void SphereHitterEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
	if(collisionEvent)
		collisionEvent->Render(t,inputStart,numInput,instance,parentTrans);
}

bool SphereHitterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
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
						Vector t1 = p1[i].pos;
						Vector t2 = p2[i].pos;
						
						Vector p1p2 = t2-t1;
						Vector p3p1 = t1-trans.translation;
						SINGLE A = (p1p2).magnitude_squared();
						SINGLE B = (dot_product(p1p2,(p3p1)));
						SINGLE C = (p3p1.magnitude_squared()) - (rad*rad);

						bool bHit = false;	
						Vector collisionPoint;
						SINGLE det = (B*B) - (A*C);
						if(det > 0)
						{
							det = Sqrt(det);
							SINGLE u1 = ((-B)+det)/(2*A);
							if(u1 > 0 && u1 < 1)
								bHit = true;
							SINGLE u2 = ((-B)-det)/(2*A);
							if(u2 > 0 && u2 < 1)
							{
								if(bHit)
								{
									if(u1 < u2)
										u2 = u1;
								}
								else
									bHit = true;
							}
							if(bHit)
								collisionPoint = t1+(u2*p1p2);
						}
						else if(det == 0)
						{
							SINGLE u1 = ((-B)/(2*A)) ;
							if(u1 > 0 && u1 < 1)
							{
								collisionPoint = t1+(u1*p1p2);
								bHit = true;
							}
						}
						if(bHit)
						{
							if(collisionEvent->ParticleEvent(collisionPoint,p2[i].vel,instance))
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
	bool bRetVal1 = true;
	bool bRetVal2 = false;
	if(output)
		bRetVal1 = output->Update(inputStart,numInput, bShutdown,instance);
	if(collisionEvent)
		bRetVal2 = collisionEvent->Update(inputStart,numInput, bShutdown,instance);
	return bRetVal1 | bRetVal2;
}

U32 SphereHitterEffect::ParticlesUsed()
{
	U32 used = 0;
	if(output)
		used += output->ParticlesUsed();
	if(collisionEvent)
		used += collisionEvent->ParticlesUsed();
	return used;
}

void SphereHitterEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
	if(collisionEvent)
		collisionEvent->FindAllocation(startPos);
}

void SphereHitterEffect::DeleteOutput()
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

IParticleEffect * SphereHitterEffect::FindFilter(const char * searchName)
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

void SphereHitterEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
	if(collisionEvent)
		collisionEvent->SetInstanceNumber(numInstances);
}

bool SphereHitterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		if(output->GetParentPosition(index,postion,lastIndex))
			return true;
	if(collisionEvent)
		return collisionEvent->GetParentPosition(index,postion,lastIndex);
	return false;
}


#endif
