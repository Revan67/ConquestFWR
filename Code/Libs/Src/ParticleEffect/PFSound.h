#ifndef PFSOUND_H
#define PFSOUND_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFSound.h                                   //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

//not currently functional - tom
struct SoundProgramer : public ParticleProgramer
{
	char contextName[32];
	char soundName[64];

	BooleanEnum looping;

	SoundProgramer();
	~SoundProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumStringParams();

	virtual const char * GetStringParamName(U32 index);

	virtual const char * GetStringParam(U32 index);

	virtual void SetStringParam(U32 index,const char * param);

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

SoundProgramer::SoundProgramer()
{
	contextName[0] = 0;
	soundName[0] = 0;
	looping = BE_FALSE;
}

SoundProgramer::~SoundProgramer()
{
}

//IParticleProgramer
U32 SoundProgramer::GetNumOutput()
{
	return 1;
}

const char * SoundProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 SoundProgramer::GetNumStringParams()
{
	return 2;
}

const char * SoundProgramer::GetStringParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Context Name";
	case 1:
		return "BT_SOUND";
	}
	return "Error";
}

const char * SoundProgramer::GetStringParam(U32 index)
{
	switch(index)
	{
	case 0:
		return contextName;
	case 1:
		return soundName;
	}
	return NULL;
}

void SoundProgramer::SetStringParam(U32 index,const char * param)
{
	switch(index)
	{
	case 0:
		{
			strncpy(contextName,param,31);
			contextName[31] = 0;
			break;
		}
	case 1:
		{
			strncpy(soundName,param,63);
			soundName[63] = 0;
			break;
		}
	}
}

U32 SoundProgramer::GetNumEnumParams()
{
	return 1;
}

const char * SoundProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Looping";
	}
	return NULL;
}

U32 SoundProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * SoundProgramer::GetEnumValueName(U32 index, U32 value)
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
	}
	return NULL;
}

U32 SoundProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return looping;
	}
	return 0;
}

void SoundProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		looping = (BooleanEnum)value;
		break;
	}
}

U32 SoundProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ (sizeof(char)*32)
		+ (sizeof(char)*64)
		+ sizeof(U32);
}

void SoundProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SOUND;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	char * contextNameHeader = (char *)(buffer+offset);
	strcpy(contextNameHeader,contextName);
	offset += sizeof(char)*32;

	char * soundNameHeader = (char *)(buffer+offset);
	strcpy(soundNameHeader,soundName);
	offset += sizeof(char)*64;

	header->size = offset;
}

void SoundProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	char * contextNameHeader = (char *)(buffer+offset);
	strcpy(contextName,contextNameHeader);
	offset += sizeof(char)*32;

	char * soundNameHeader = (char *)(buffer+offset);
	strcpy(soundName,soundNameHeader);
	offset += sizeof(char)*64;
}

struct SoundEffect : public IParticleEffect
{
	IParticleEffect * output;

	char contextName[32];
	char soundName[64];
	
	SoundEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~SoundEffect();
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

SoundEffect::SoundEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);
	strcpy(contextName,(char *)(buffer+offset));
	offset += sizeof(char)*32;
	strcpy(soundName,(char *)(buffer+offset));
	offset += sizeof(char)*64;
}

SoundEffect::~SoundEffect()
{
}

IParticleEffectInstance * SoundEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(output)
		delete output;
	output = createParticalEffect(parentSystem,type,buffer,this);
	return output;
}

void SoundEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(p1[0].bLive && p2[0].bLive && (p1[0].birthTime < currentTimeMS))
	{
		Vector vI,vJ,vK;
		if(p2[0].vel.x == 0 && p2[0].vel.y == 0 && p2[0].vel.z == 0)
			vI = Vector(1,0,0);
		else
		{
			vI = p2[0].vel;
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

		Vector epos = (t*(p2[0].pos - p1[0].pos))+p1[0].pos;

		Transform trans;
		trans.set_i(vI);
		trans.set_j(vJ);
		trans.set_k(vK);
		trans.translation = epos;

		POSCALLBACK->ObjectPostionCallback(0,parentSystem->GetContext(),contextName,trans);
	}
	if(output)
		output->Render(t,inputStart,numInput,instance, parentTrans);
}

bool SoundEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(output)
		 return output->Update(inputStart,numInput,bShutdown,instance);
	return false;
}

U32 SoundEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void SoundEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void SoundEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * SoundEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void SoundEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool SoundEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
