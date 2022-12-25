#ifndef PFWIND_H
#define PFWIND_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFWind.h                                    //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct WindProgramer : public ParticleProgramer
{
	FloatType * strength;

	WindProgramer();
	~WindProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

WindProgramer::WindProgramer()
{
	strength = MakeDefaultFloat(1);
}

WindProgramer::~WindProgramer()
{
	delete strength;
}

//IParticleProgramer
U32 WindProgramer::GetNumOutput()
{
	return 1;
}

const char * WindProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 WindProgramer::GetNumFloatParams()
{
	return 1;
}

const char * WindProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Strength";
	}
	return NULL;
}

const FloatType * WindProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return strength;
	}
	return NULL;
}

void WindProgramer::SetFloatParam(U32 index,const FloatType * param)
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

U32 WindProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(strength);
}

void WindProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_WIND;
	strcpy(header->effectName,effectName);

	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+sizeof(ParticleHeader));
	EncodeParam::EncodeFloat(strengthHeader,strength);

	header->size = sizeof(ParticleHeader)+strengthHeader->size;
}

void WindProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	EncodedFloatTypeHeader * strengthHeader = (EncodedFloatTypeHeader *)(buffer+sizeof(ParticleHeader));
	strength = 	EncodeParam::DecodeFloat(strengthHeader);
}

struct WindEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedFloatTypeHeader * strength;

	U8 * data;

	WindEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer);
	virtual ~WindEffect();
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

WindEffect::WindEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = new U8[header->size];
	memcpy(data,buffer,header->size);
	strength = (EncodedFloatTypeHeader *)(data+sizeof(ParticleHeader));

	output = 0;
}

WindEffect::~WindEffect()
{
	delete [] data;
	if(output)
		delete output;
}

IParticleEffectInstance * WindEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = createParticalEffect(parentSystem,type,buffer,this);
		return output;
	}
	return NULL;
}

void WindEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool WindEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	for (U16 i = 0; i < numInput; i++)
	{
		if(p2[i].bLive)
		{
			SINGLE str = EncodeParam::GetFloat(strength,&(p2[i]),parentSystem);
			Vector wind = GMGRANNY->GetWind(p2[i].pos,false)*(str*DELTA_TIME);
			p2[i].vel = p2[i].vel+wind;			
		}
	}
	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 WindEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void WindEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void WindEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * WindEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void WindEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool WindEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
