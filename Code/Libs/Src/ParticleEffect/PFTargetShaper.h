#ifndef PFTARGETSHAPER_H
#define PFTARGETSHAPER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFTargetShaper.h                               //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

#define TSP_VERSION_1_SIZE (sizeof(ParticleHeader) + sizeof(U32)+ sizeof(U32))

enum TSE_MODE
{
	TSE_NORMAL,
	TSE_FOLLOW,
};

struct TargetShaperProgramer : public ParticleProgramer
{
	U32 targetID;

	TSE_MODE mode;

	char filterName[256];

	TargetShaperProgramer();
	~TargetShaperProgramer();

	//IParticleProgramer

	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumTargetParams();

	virtual const char * GetTargetParamName(U32 index);

	virtual const U32 GetTargetParam(U32 index);

	virtual void SetTargetParam(U32 index,U32 param);

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

TargetShaperProgramer::TargetShaperProgramer()
{
	filterName[0] = 0;

	targetID = 0;
	mode = TSE_NORMAL;
}

TargetShaperProgramer::~TargetShaperProgramer()
{
}

//IParticleProgramer
U32 TargetShaperProgramer::GetNumOutput()
{
	return 1;
}

const char * TargetShaperProgramer::GetOutputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 TargetShaperProgramer::GetNumInput()
{
	return 1;
}

const char * TargetShaperProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 TargetShaperProgramer::GetNumTargetParams()
{
	return 1;
}

const char * TargetShaperProgramer::GetTargetParamName(U32 index)
{
	return "Shape Target";
}

const U32 TargetShaperProgramer::GetTargetParam(U32 index)
{
	return targetID;
}

void TargetShaperProgramer::SetTargetParam(U32 index,U32 param)
{
	targetID = param;
}

U32 TargetShaperProgramer::GetNumStringParams()
{
	return 1;
}

const char * TargetShaperProgramer::GetStringParamName(U32 index)
{
	return "Material Name";
}

const char * TargetShaperProgramer::GetStringParam(U32 index)
{
	return filterName;
}

void TargetShaperProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(filterName,param);
}

U32 TargetShaperProgramer::GetNumEnumParams()
{
	return 1;
}

const char * TargetShaperProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Mode";
	}
	return NULL;
}

U32 TargetShaperProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	}
	return 0;
}

const char * TargetShaperProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Normal";
			case 1:
				return "Follow";
			}
		}
		break;
	}
	return NULL;
}

U32 TargetShaperProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return mode;
	}
	return 0;
}

void TargetShaperProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		mode = (TSE_MODE)value;
		break;
	}
}

U32 TargetShaperProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ sizeof(U32)
		+ sizeof(U32)
		+ sizeof(U32)
		+ sizeof(char)*256;
}

void TargetShaperProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_TARGET_SHAPER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	*((U32*)(buffer+offset)) = targetID;
	offset += sizeof(U32);

	*((TSE_MODE*)(buffer+offset)) = mode;
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,filterName);
	offset += sizeof(char)*256;

	header->size = offset;
}

void TargetShaperProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	targetID = *((U32*)(buffer+offset));
	offset += sizeof(U32);

	mode = *((TSE_MODE*)(buffer+offset));
	offset += sizeof(U32);

	if(header->size == TSP_VERSION_1_SIZE)
	{
		filterName[0] = 0;
	}
	else
	{
		char * matHeader = (char *)(buffer+offset);
		strcpy(filterName,matHeader);
		offset += sizeof(char)*256;
	}
}

struct TargetShaperEffect : public IParticleEffect
{
	IParticleEffect * output;

	U8 * data;

	U32 targetID;
	TSE_MODE mode;
	IMaterial * filterMat;

	U32 numInst;
	struct InstStruct
	{
		U32 baseRand;
	};

	InstStruct * instStruct;

	TargetShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~TargetShaperEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 uinstance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 uinstance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);

	//TargetShaperEffect
};

TargetShaperEffect::TargetShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	numInst = 1;
	instStruct = NULL;
	output = NULL;
	filterMat = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	targetID = *((U32 *)(data+offset));
	offset += sizeof(U32);

	mode = *((TSE_MODE *)(data+offset));
	offset += sizeof(U32);

	if(header->size == TSP_VERSION_1_SIZE)
	{
		filterMat = NULL;
	}
	else
	{
		char * matName = ((char *)(data+offset));
		if(matName[0])
			filterMat = _parent->GetOwner()->GetMaterialManager()->FindMaterial(matName);
		offset += sizeof(char)*256;
	}
}

TargetShaperEffect::~TargetShaperEffect()
{
	delete [] instStruct;
	if(output)
		delete output;
}

IParticleEffectInstance * TargetShaperEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void TargetShaperEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void TargetShaperEffect::Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(mode == TSE_FOLLOW)
	{
		IMeshInstance * mesh = parentSystem->GetOwner()->GetCallback()->GetObjectMesh(targetID,parentSystem->GetContext());
		if(mesh)
		{
			Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
			Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive)
				{
					p2[i].pos = mesh->GetRandomSurfacePos(p2[i].birthTime+(i*456),filterMat);
					p1[i].pos = p2[i].pos;
				}
			}
		}
		if(output)
			output->Render(t,inputStart,numInput,instance,parentTrans);
	}
	else
	{
		if(output)
			output->Render(t,inputStart,numInput,instance,parentTrans);
	}
}

bool TargetShaperEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	if(mode == TSE_NORMAL)
	{
		IMeshInstance * mesh = parentSystem->GetOwner()->GetCallback()->GetObjectMesh(targetID,parentSystem->GetContext());
		if(mesh)
		{
			Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
			Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive)
				{
					if(p2[i].bNew)
					{
						p2[i].pos = mesh->GetRandomSurfacePos(p2[i].birthTime+(i*456),filterMat);
					}			
				}
			}
		}
	}
	else if(mode == TSE_FOLLOW)
	{
		IMeshInstance * mesh = parentSystem->GetOwner()->GetCallback()->GetObjectMesh(targetID,parentSystem->GetContext());
		if(mesh)
		{
			Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
			Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive)
				{
					p2[i].pos = mesh->GetRandomSurfacePos(p2[i].birthTime+(i*456),filterMat);
					p1[i].pos = p2[i].pos;
				}
			}
		}
	}

	bool bRetVal = true;
	if(output)
		bRetVal = output->Update(inputStart,numInput, bShutdown,instance);

	return bRetVal;
}

U32 TargetShaperEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].baseRand = rand();
	}

	if(output)
		return output->ParticlesUsed();
	return 0;
}

void TargetShaperEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void TargetShaperEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * TargetShaperEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void TargetShaperEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool TargetShaperEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
