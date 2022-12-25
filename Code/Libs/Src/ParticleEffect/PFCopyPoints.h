#ifndef PFCOPYPOINTS_H
#define PFCOPYPOINTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFCopyPonts.h                               //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct CopyPointsProgramer : public ParticleProgramer
{
	CopyPointsProgramer();
	~CopyPointsProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

CopyPointsProgramer::CopyPointsProgramer()
{
}

CopyPointsProgramer::~CopyPointsProgramer()
{
}

//IParticleProgramer
U32 CopyPointsProgramer::GetNumOutput()
{
	return 2;
}

const char * CopyPointsProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CopyPointsProgramer::GetNumInput()
{
	return 1;
}

const char * CopyPointsProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CopyPointsProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader);
}

void CopyPointsProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_COPY_POINTS;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	header->size = offset;
}

void CopyPointsProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

struct CopyPointsEffect : public IParticleEffect
{
	IParticleEffect * outputOrg;
	IParticleEffect * outputCopy;

	char effectName[32];

	U32 numInst;
	struct InstStruct
	{
		U32 particleRegionStart;
	};
	InstStruct * instStruct;

	CopyPointsEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~CopyPointsEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual U32 GetAllocationStart(IParticleEffectInstance * target,U32 instance);
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

CopyPointsEffect::CopyPointsEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	outputOrg = NULL;
	outputCopy = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

CopyPointsEffect::~CopyPointsEffect()
{
	delete [] instStruct;
	if(outputOrg)
		delete outputOrg;
	if(outputCopy)
		delete outputCopy;
}

IParticleEffectInstance * CopyPointsEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(outputOrg)
			delete outputOrg;
		outputOrg = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return outputOrg;
	}
	else if(outputID == 1)
	{
		if(outputCopy)
			delete outputCopy;
		outputCopy = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return outputCopy;
	}
	return NULL;
}

void CopyPointsEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(outputOrg)
			delete outputOrg;
		outputOrg = (IParticleEffect*)target;
		outputOrg->parentEffect[inputID] = this;
	}
	else if(outputID == 1)
	{
		if(outputCopy)
			delete outputCopy;
		outputCopy = (IParticleEffect*)target;
		outputCopy->parentEffect[inputID] = this;
	}
};

U32 CopyPointsEffect::GetAllocationStart(IParticleEffectInstance * target,U32 instance)
{
	if(outputOrg == target)
	{
		if(parentEffect[0])
			return parentEffect[0]->GetAllocationStart(this,instance);
		return 0;
	}
	return instStruct[instance].particleRegionStart;
}

void CopyPointsEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(outputOrg)
		outputOrg->Render(t,inputStart,numInput,instance,parentTrans);
	if(outputCopy)
		outputCopy->Render(t,instStruct[instance].particleRegionStart,numInput,instance,parentTrans);
}

bool CopyPointsEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Particle * org1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * org2 = &(parentSystem->myParticles->particles2[inputStart]);
	Particle * cpy1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
	Particle * cpy2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
	memcpy(cpy1,org1,sizeof(Particle)*numInput);
	memcpy(cpy2,org2,sizeof(Particle)*numInput);
	if(outputOrg)
		return outputOrg->Update(inputStart,numInput, bShutdown,instance);
	if(outputCopy)
		return outputCopy->Update(instStruct[instance].particleRegionStart,numInput, bShutdown,instance);
	return true;
}

U32 CopyPointsEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = 0;
	}
	U32 number = 0;
	if(outputOrg)
		number += outputOrg->ParticlesUsed();
	if(outputCopy)
		number += outputCopy->ParticlesUsed();
	return number+(OutputRange()*numInst);
}

void CopyPointsEffect::FindAllocation(U32 & startPos)
{
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = startPos;
		startPos += OutputRange();
	}
	if(outputOrg)
		outputOrg->FindAllocation(startPos);
	if(outputCopy)
		outputCopy->FindAllocation(startPos);
}

void CopyPointsEffect::DeleteOutput()
{
	if(outputOrg)
	{
		delete outputOrg;
		outputOrg = NULL;
	}
	if(outputCopy)
	{
		delete outputCopy;
		outputCopy = NULL;
	}
}

void CopyPointsEffect::NullOutput(IParticleEffect * target)
{
	if(outputOrg == target)
		outputOrg = NULL;
	if(outputCopy == target)
		outputCopy = NULL;
}

IParticleEffect * CopyPointsEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * found = NULL;
	if(outputOrg)
		found = outputOrg->FindFilter(searchName);
	if(found)
		return found;
	if(outputCopy)
		return outputCopy->FindFilter(searchName);
	return NULL;
}

void CopyPointsEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(outputOrg)
		outputOrg->SetInstanceNumber(numInst);
	if(outputCopy)
		outputCopy->SetInstanceNumber(numInst);
}

bool CopyPointsEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	bool bFound = false;
	if(outputOrg)
		bFound = outputOrg->GetParentPosition(index,postion,lastIndex);
	if(bFound)
		return true;
	if(outputCopy)
		return outputCopy->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
