#ifndef PFSPLITTER_H
#define PFSPLITTER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFSplitter.h                           //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct SplitterProgramer : public ParticleProgramer
{
	SplitterProgramer();
	~SplitterProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

SplitterProgramer::SplitterProgramer()
{
}

SplitterProgramer::~SplitterProgramer()
{
}

//IParticleProgramer
U32 SplitterProgramer::GetNumOutput()
{
	return 2;
}

const char * SplitterProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Output 1";
	case 1:
		return "Output 2";
	}
	return "Error";
}

U32 SplitterProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader);
}

void SplitterProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_SPLITTER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	header->size = offset;
}

void SplitterProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

struct SplitterEffect : public IParticleEffect
{
	IParticleEffect * output1;//0
	IParticleEffect * output2;//1

	SplitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~SplitterEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

SplitterEffect::SplitterEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	output1 = NULL;//0
	output2 = NULL;//1
}

SplitterEffect::~SplitterEffect()
{
	if(output1)
		delete output1;
	if(output2)
		delete output2;
}

IParticleEffectInstance * SplitterEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	switch(outputID)
	{
	case 0:
		if(output1)
			delete output1;
		output1 = createParticalEffect(parentSystem,type,buffer,this);
		return output1;
	case 1:
		if(output2)
			delete output2;
		output2 = createParticalEffect(parentSystem,type,buffer,this);
		return output2;
	}
	return NULL;
}

void SplitterEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output1)
		output1->Render(t,inputStart,numInput,instance,parentTrans);
	if(output2)
		output2->Render(t,inputStart,numInput,instance,parentTrans);
}

bool SplitterEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	bool bRet1 = false;
	if(output1)
		bRet1 = output1->Update(inputStart,numInput,bShutdown,instance);
	bool bRet2 = false;
	if(output2)
		bRet2 = output2->Update(inputStart,numInput,bShutdown,instance);

	return (bRet1 || bRet2);
}

U32 SplitterEffect::ParticlesUsed()
{
	U32 count = 0;
	if(output1)
		count += output1->ParticlesUsed();
	if(output2)
		count += output2->ParticlesUsed();
	return count;
}

void SplitterEffect::FindAllocation(U32 & startPos)
{
	if(output1)
		output1->FindAllocation(startPos);
	if(output2)
		output2->FindAllocation(startPos);
}

void SplitterEffect::DeleteOutput()
{
	if(output1)
	{
		delete output1;
		output1 = NULL;
	}
	if(output2)
	{
		delete output2;
		output2 = NULL;
	}
}

IParticleEffect * SplitterEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * retVal = NULL;
	if(output1)
		retVal = output1->FindFilter(searchName);
	if(retVal)
		return retVal;
	else if(output2)
		return output2->FindFilter(searchName);
	return NULL;
}

void SplitterEffect::SetInstanceNumber(U32 numInstances)
{
	if(output1)
		output1->SetInstanceNumber(numInstances);
	if(output2)
		output2->SetInstanceNumber(numInstances);
}

bool SplitterEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output1)
		if(output1->GetParentPosition(index,postion,lastIndex))
			return true;
	if(output2)
		return output2->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
