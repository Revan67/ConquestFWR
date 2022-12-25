#ifndef PFPOSITIONCALLBACK_H
#define PFPOSITIONCALLBACK_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFPositionCallback.h                        //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct PositionCallbackProgramer : public ParticleProgramer
{
	char positionName[32];
	U32 targetID;

	PositionCallbackProgramer();
	~PositionCallbackProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumTargetParams();

	virtual const char * GetTargetParamName(U32 index);

	virtual const U32 GetTargetParam(U32 index);

	virtual void SetTargetParam(U32 index,U32 param);

	virtual U32 GetNumStringParams();

	virtual const char * GetStringParamName(U32 index);

	virtual const char * GetStringParam(U32 index);

	virtual void SetStringParam(U32 index,const char * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

PositionCallbackProgramer::PositionCallbackProgramer()
{
	positionName[0] = 0;
	targetID = 0;
}

PositionCallbackProgramer::~PositionCallbackProgramer()
{
}

//IParticleProgramer
U32 PositionCallbackProgramer::GetNumOutput()
{
	return 1;
}

const char * PositionCallbackProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 PositionCallbackProgramer::GetNumTargetParams()
{
	return 1;
}

const char * PositionCallbackProgramer::GetTargetParamName(U32 index)
{
	return "Callback Target";
}

const U32 PositionCallbackProgramer::GetTargetParam(U32 index)
{
	return targetID;
}

void PositionCallbackProgramer::SetTargetParam(U32 index,U32 param)
{
	targetID = param;
}

U32 PositionCallbackProgramer::GetNumStringParams()
{
	return 1;
}

const char * PositionCallbackProgramer::GetStringParamName(U32 index)
{
	return "Position Name";
}

const char * PositionCallbackProgramer::GetStringParam(U32 index)
{
	return positionName;
}

void PositionCallbackProgramer::SetStringParam(U32 index,const char * param)
{
	strncpy(positionName,param,31);
	positionName[31] = 0;
}

U32 PositionCallbackProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ sizeof(U32)
		+ (sizeof(char)*32);
}

void PositionCallbackProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_POSITION_CALLBACK;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	*((U32*)(buffer+offset)) = targetID;
	offset += sizeof(U32);

	char * positionNameHeader = (char *)(buffer+offset);
	strcpy(positionNameHeader,positionName);
	offset += sizeof(char)*32;

	header->size = offset;
}

void PositionCallbackProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	targetID = *((U32*)(buffer+offset));
	offset += sizeof(U32);

	char * positionNameHeader = (char *)(buffer+offset);
	strcpy(positionName,positionNameHeader);
}

struct PositionCallbackEffect : public IParticleEffect
{
	IParticleEffect * output;

	U32 targetID;
	char positionName[32];
	
	PositionCallbackEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~PositionCallbackEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 inst, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 inst);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

PositionCallbackEffect::PositionCallbackEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);
	targetID = *((U32*)(buffer+offset));
	offset += sizeof(U32);
	strcpy(positionName,(char *)(buffer+offset));
}

PositionCallbackEffect::~PositionCallbackEffect()
{
}

IParticleEffectInstance * PositionCallbackEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(output)
		delete output;
	output = createParticalEffect(parentSystem,type,buffer,this);
	return output;
}

void PositionCallbackEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	for(U32 i = inputStart; i <numInput;++i)
	{
		if(p1[i].bLive && p2[i].bLive && (p1[i].birthTime < currentTimeMS))
		{
			Vector vI,vJ,vK;
			if(p2[i].vel.x == 0 && p2[i].vel.y == 0 && p2[i].vel.z == 0)
				vI = Vector(1,0,0);
			else
			{
				vI = p2[i].vel;
				vI.fast_normalize();
			}
			if(vI.x == 0 && vI.y == 0 && vI.z == 1)
			{
				vJ = Vector(0,1.0,0);
			}
			else
			{
				vJ = Vector(0,0,1.0);
			}
			vK = cross_product(vI,vJ);
			vK.fast_normalize();
			vJ = cross_product(vK,vI);
			vJ.fast_magnitude();

			Vector epos = (t*(p2[i].pos - p1[i].pos))+p1[i].pos;

			Transform trans;
			trans.set_i(vI);
			trans.set_j(vJ);
			trans.set_k(vK);
			trans.translation = epos;

			POSCALLBACK->ObjectPostionCallback(targetID,parentSystem->GetContext(),positionName,trans);
		}
	}
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool PositionCallbackEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(output)
		 return output->Update(inputStart,numInput,bShutdown,instance);
	return false;
}

U32 PositionCallbackEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void PositionCallbackEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void PositionCallbackEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * PositionCallbackEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void PositionCallbackEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool PositionCallbackEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
