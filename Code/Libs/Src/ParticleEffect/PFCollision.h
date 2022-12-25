#ifndef PFCOLLISION_H
#define PFCOLLISION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFCollision.h                                    //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct CollisionProgramer : public ParticleProgramer
{
	FloatType * friction;

	BooleanEnum bTerrain;
	BooleanEnum bTinkerToys;
	BooleanEnum bWater;
	BooleanEnum bUnits;

	CollisionProgramer();
	~CollisionProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

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

CollisionProgramer::CollisionProgramer()
{
	friction = MakeDefaultFloat(0);
	bTerrain = BE_FALSE;
	bTinkerToys = BE_FALSE;
	bWater = BE_FALSE;
	bUnits = BE_FALSE;
}

CollisionProgramer::~CollisionProgramer()
{
	delete friction;
}

//IParticleProgramer
U32 CollisionProgramer::GetNumOutput()
{
	return 2;
}

const char * CollisionProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Default Output";
	case 1:
		return "Collision Event";
	}
	return "Error";
}

U32 CollisionProgramer::GetNumFloatParams()
{
	return 1;
}

const char * CollisionProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Friction";
	}
	return NULL;
}

const FloatType * CollisionProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return friction;
	}
	return NULL;
}

void CollisionProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(friction)
				delete friction;
			if(param)
				friction = param->CreateCopy();
			else
				friction = NULL;
			break;
		}
	}
}

U32 CollisionProgramer::GetNumEnumParams()
{
	return 4;
}

const char * CollisionProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Hit Terrain";
	case 1:
		return "Hit TinkerToys";
	case 2:
		return "Hit Water";
	case 3:
		return "Hit Units";
	}
	return NULL;
}

U32 CollisionProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	case 1:
		return 2;
	case 2:
		return 2;
	case 3:
		return 2;
	}
	return 0;
}

const char * CollisionProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "False";
			case 1:
				return "True";
			}
		}
		break;
	case 1:
		{
			switch(value)
			{
			case 0:
				return "False";
			case 1:
				return "True";
			}
		}
		break;
	case 2:
		{
			switch(value)
			{
			case 0:
				return "False";
			case 1:
				return "True";
			}
		}
		break;
	case 3:
		{
			switch(value)
			{
			case 0:
				return "False";
			case 1:
				return "True";
			}
		}
		break;
	}
	return NULL;
}

U32 CollisionProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return bTerrain;
	case 1:
		return bTinkerToys;
	case 2:
		return bWater;
	case 3:
		return bUnits;
	}
	return 0;
}

void CollisionProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		bTerrain = (BooleanEnum)value;
		break;
	case 1:
		bTinkerToys = (BooleanEnum)value;
		break;
	case 2:
		bWater = (BooleanEnum)value;
		break;
	case 3:
		bUnits = (BooleanEnum)value;
		break;
	}
}

U32 CollisionProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(friction)
		+sizeof(U32)
		+sizeof(U32)
		+sizeof(U32)
		+sizeof(U32);
}

void CollisionProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_COLLISION;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * frictionHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(frictionHeader,friction);
	offset = offset + frictionHeader->size;

	*((U32*)(buffer+offset)) = bTerrain;
	offset += sizeof(U32);

	*((U32*)(buffer+offset)) = bTinkerToys;
	offset += sizeof(U32);

	*((U32*)(buffer+offset)) = bWater;
	offset += sizeof(U32);

	*((U32*)(buffer+offset)) = bUnits;
	offset += sizeof(U32);

	header->size = offset;
}

void CollisionProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * frictionHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	friction = EncodeParam::DecodeFloat(frictionHeader);
	offset+= frictionHeader->size;

	bTerrain = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bTinkerToys = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bWater = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bUnits = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

}

struct CollisionEffect : public IParticleEffect
{
	IParticleEffect * output;
	IParticleEffect * collisionEvent;

	EncodedFloatTypeHeader * friction;

	BooleanEnum bTerrain;
	BooleanEnum bTinkerToys;
	BooleanEnum bWater;
	BooleanEnum bUnits;


	U8 * data;

	CollisionEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer);
	virtual ~CollisionEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
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

CollisionEffect::CollisionEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	data = new U8[header->size];
	memcpy(data,buffer,header->size);

	friction = (EncodedFloatTypeHeader *)(data+offset);
	offset += friction->size; 

	bTerrain = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bTinkerToys = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bWater = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	bUnits = *((BooleanEnum*)(buffer+offset));
	offset+=sizeof(U32);

	output = 0;
	collisionEvent = 0;
}

CollisionEffect::~CollisionEffect()
{
	delete [] data;
	if(output)
		delete output;
	if(collisionEvent)
		delete collisionEvent;
}

IParticleEffectInstance * CollisionEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = createParticalEffect(parentSystem,type,buffer,this);
		return output;
	}
	if(outputID == 1)
	{
		if(collisionEvent)
			delete collisionEvent;
		collisionEvent = createParticalEffect(parentSystem,type,buffer,this);
		return collisionEvent;
	}
	return NULL;
}

void CollisionEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
	if(collisionEvent)
		collisionEvent->Render(t,inputStart,numInput,instance,parentTrans);
}

bool CollisionEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	for (U16 i = 0; i < numInput; i++)
	{
		if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
		{
			if(!(p1[i].pos == p2[i].pos))
			{
				Vector collisionPoint;
				Vector finalPoint;
				if(POSCALLBACK->TestCollision(p1[i].pos,p2[i].pos,collisionPoint,finalPoint,bTerrain!= 0,bTinkerToys!= 0,bWater!= 0,bUnits!= 0))
				{
					SINGLE fric = EncodeParam::GetFloat(friction,&(p2[i]),parentSystem);

					SINGLE travelDist = (p1[i].pos-p2[i].pos).fast_magnitude();
					Vector newDir = finalPoint-collisionPoint;
					SINGLE leg2Dist = newDir.fast_magnitude();

					newDir /= leg2Dist;

					SINGLE startVel = travelDist/DELTA_TIME;
					SINGLE endVel = startVel*(1.0-fric);
					p2[i].vel = newDir*endVel;

					SINGLE travelTime = (leg2Dist/travelDist)*DELTA_TIME;

					p2[i].pos = collisionPoint + (p2[i].vel*travelTime);
					if(collisionEvent)
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
	bool bRetVal1 = true;
	bool bRetVal2 = false;
	if(output)
		bRetVal1 = output->Update(inputStart,numInput, bShutdown,instance);
	if(collisionEvent)
		bRetVal2 = collisionEvent->Update(inputStart,numInput, bShutdown,instance);
	return bRetVal1 | bRetVal2;
}

U32 CollisionEffect::ParticlesUsed()
{
	U32 used = 0;
	if(output)
		used += output->ParticlesUsed();
	if(collisionEvent)
		used += collisionEvent->ParticlesUsed();
	return used;
}

void CollisionEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
	if(collisionEvent)
		collisionEvent->FindAllocation(startPos);
}

void CollisionEffect::DeleteOutput()
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

IParticleEffect * CollisionEffect::FindFilter(const char * searchName)
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

void CollisionEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
	if(collisionEvent)
		collisionEvent->SetInstanceNumber(numInstances);
}

bool CollisionEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		if(output->GetParentPosition(index,postion,lastIndex))
			return true;
	if(collisionEvent)
		return collisionEvent->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
