#ifndef PFEVENTKILLER_H
#define PFEVENTKILLER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFEventKiller.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct PEventKillerProgramer : public ParticleProgramer
{
	PEventKillerProgramer();
	~PEventKillerProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

PEventKillerProgramer::PEventKillerProgramer()
{
}

PEventKillerProgramer::~PEventKillerProgramer()
{
}

//IParticleProgramer
U32 PEventKillerProgramer::GetNumOutput()
{
	return 1;
}

const char * PEventKillerProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return FI_EVENT;
	}
	return NULL;
}

U32 PEventKillerProgramer::GetNumInput()
{
	return 1;
}

const char * PEventKillerProgramer::GetInputName(U32 index)
{
	return FI_EVENT;
}

U32 PEventKillerProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader);
}

void PEventKillerProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_PEVENT_KILLER;
	strcpy(header->effectName,effectName);
	U32 offset = sizeof(ParticleHeader);

	header->size = offset;
}

void PEventKillerProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}


struct PEventKillerEffect : public IParticleEffect
{
	IParticleEffect * pEventPassThrough;

	char effectName[32];

	PEventKillerEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~PEventKillerEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual bool ParticleEvent(const Vector & position, const Vector &velocity,U32 instance);
};

PEventKillerEffect::PEventKillerEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	pEventPassThrough = NULL;
}

PEventKillerEffect::~PEventKillerEffect()
{
	if(pEventPassThrough)
		delete pEventPassThrough;
}

IParticleEffectInstance * PEventKillerEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(pEventPassThrough)
			delete pEventPassThrough;
		pEventPassThrough = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return pEventPassThrough;
	}
	return NULL;
}

void PEventKillerEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(pEventPassThrough)
			delete pEventPassThrough;
		pEventPassThrough = (IParticleEffect*)target;
		pEventPassThrough->parentEffect[inputID] = this;
	}
}

void PEventKillerEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(pEventPassThrough)
		pEventPassThrough->Render(t,inputStart,numInput,instance,parentTrans);
}

bool PEventKillerEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	if(pEventPassThrough)
		return pEventPassThrough->Update(inputStart,numInput, bShutdown,instance);
	return false;
}

U32 PEventKillerEffect::ParticlesUsed()
{
	if(pEventPassThrough)
		return pEventPassThrough->ParticlesUsed();
	return 0;
}

void PEventKillerEffect::FindAllocation(U32 & startPos)
{
	if(pEventPassThrough)
		pEventPassThrough->FindAllocation(startPos);
}

void PEventKillerEffect::DeleteOutput()
{
	if(pEventPassThrough)
	{
		delete pEventPassThrough;
		pEventPassThrough = NULL;
	}
}

IParticleEffect * PEventKillerEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	if(pEventPassThrough)
		return pEventPassThrough->FindFilter(searchName);
	return NULL;
}

void PEventKillerEffect::SetInstanceNumber(U32 numInstances)
{
	if(pEventPassThrough)
		pEventPassThrough->SetInstanceNumber(numInstances);
}

bool PEventKillerEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(pEventPassThrough)
		return pEventPassThrough->GetParentPosition(index,postion,lastIndex);
	return false;
}

bool PEventKillerEffect::ParticleEvent(const Vector & position, const Vector &velocity,U32 instance)
{
	if(pEventPassThrough)
		pEventPassThrough->ParticleEvent(position,velocity,instance);
	return true;
}

#endif
