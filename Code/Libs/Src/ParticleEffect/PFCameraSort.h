#ifndef PFCAMERASORT_H
#define PFCAMERASORT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFCameraSort.h                              //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct CameraSortProgramer : public ParticleProgramer
{
	CameraSortProgramer();
	~CameraSortProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

CameraSortProgramer::CameraSortProgramer()
{
}

CameraSortProgramer::~CameraSortProgramer()
{
}

//IParticleProgramer
U32 CameraSortProgramer::GetNumOutput()
{
	return 1;
}

const char * CameraSortProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 CameraSortProgramer::GetNumInput()
{
	return 1;
}

const char * CameraSortProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CameraSortProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader);
}

void CameraSortProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_CAMERA_SORT;
	strcpy(header->effectName,effectName);

	header->size = sizeof(ParticleHeader);
}

void CameraSortProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
}

struct CameraSortEffect : public IParticleEffect
{
	IParticleEffect * output;

	CameraSortEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect,U8 * buffer, U32 inputID);
	virtual ~CameraSortEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

CameraSortEffect::CameraSortEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	output = 0;
}

CameraSortEffect::~CameraSortEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * CameraSortEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void CameraSortEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void CameraSortEffect::Render(float t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

#define MAX_SORT 500

SINGLE csPointSorter[MAX_SORT];
S32 sortDivisions = 0;
S32 sortStack[MAX_SORT*2];//realy should be log(MAX_SORT)*2

bool CameraSortEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);

	//sort into living and dead
/*	U32 lastLive = 0;
	for (U16 i = 0; i < numInput; i++)
	{
		if(p1[i].bLive)
		{
			if(lastLive != i)
			{
				memcpy(&(p1[lastLive]),&(p1[i]),sizeof(Particle));
				memcpy(&(p2[lastLive]),&(p2[i]),sizeof(Particle));
				p1[i].bLive = false;
				p2[i].bLive = false;
			}
			++lastLive;
		}
	}
*/
	//sort living into camera order
	U32 lastLive = numInput;
	if(lastLive)
	{
		Vector cameraPos = parentSystem->GetOwner()->GetCamera()->get_position();
		Vector lookDir = -(parentSystem->GetOwner()->GetCamera()->get_transform().get_k());
		lastLive = __min(MAX_SORT,lastLive);
		for(U32 i = 0; i < lastLive; ++i)
		{
			Vector partDir = p1[i].pos-cameraPos;
			csPointSorter[i] = dot_product(partDir,lookDir);
//			csPointSorter[i] = (cameraPos-p1[i].pos).magnitude_squared();
		}
		sortDivisions = 1;
		sortStack[0] = 0;
		sortStack[1] = lastLive-1;
		while(sortDivisions)
		{
			sortDivisions--;
			S32 currentDivision = sortDivisions*2;
			S32 sortVal = sortStack[currentDivision];
			S32 setStart = sortVal+1;
			S32 endVal = sortStack[currentDivision+1];
			S32 setEnd = endVal;
			while(setStart < setEnd)
			{
				//find next swap
				while(setStart <= setEnd)
				{
					if(csPointSorter[setStart] < csPointSorter[sortVal])
						break;
					++setStart;
				}
				while(setStart <= setEnd)
				{
					if(csPointSorter[setEnd] > csPointSorter[sortVal])
						break;
					--setEnd;
				}
				if(setStart < setEnd)
				{
					SINGLE tmpFloat = csPointSorter[setStart];
					csPointSorter[setStart] = csPointSorter[setEnd];
					csPointSorter[setEnd] = tmpFloat;

					Particle tmpPart =p1[setStart];
					p1[setStart] = p1[setEnd];
					p1[setEnd] = tmpPart;

					tmpPart =p2[setStart];
					p2[setStart] = p2[setEnd];
					p2[setEnd] = tmpPart;
				}
			}
			if(sortVal != setEnd)
			{
				SINGLE tmpFloat = csPointSorter[sortVal];
				csPointSorter[sortVal] = csPointSorter[setEnd];
				csPointSorter[setEnd] = tmpFloat;

				Particle tmpPart =p1[sortVal];
				p1[sortVal] = p1[setEnd];
				p1[setEnd] = tmpPart;

				tmpPart =p2[sortVal];
				p2[sortVal] = p2[setEnd];
				p2[setEnd] = tmpPart;
			}

			//create new partitions if nessecary
			setEnd--;
			S32 sortSize = setEnd-sortVal;
			if(sortSize == 1)
			{
				if(csPointSorter[sortVal] < csPointSorter[setEnd])
				{
					SINGLE tmpFloat = csPointSorter[sortVal];
					csPointSorter[sortVal] = csPointSorter[setEnd];
					csPointSorter[setEnd] = tmpFloat;

					Particle tmpPart =p1[sortVal];
					p1[sortVal] = p1[setEnd];
					p1[setEnd] = tmpPart;

					tmpPart =p2[sortVal];
					p2[sortVal] = p2[setEnd];
					p2[setEnd] = tmpPart;
				}
			}
			else if(sortSize >= 2)
			{
				sortStack[(sortDivisions*2)] = sortVal;
				sortStack[(sortDivisions*2)+1] = setEnd;
				sortDivisions++;
			}

			sortSize = endVal-setStart;
			if(sortSize == 1)
			{
				if(csPointSorter[setStart] < csPointSorter[endVal])
				{
					SINGLE tmpFloat = csPointSorter[setStart];
					csPointSorter[setStart] = csPointSorter[endVal];
					csPointSorter[endVal] = tmpFloat;

					Particle tmpPart =p1[setStart];
					p1[setStart] = p1[endVal];
					p1[endVal] = tmpPart;

					tmpPart =p2[setStart];
					p2[setStart] = p2[endVal];
					p2[endVal] = tmpPart;
				}
			}
			else if(sortSize >= 2)
			{
				sortStack[(sortDivisions*2)] = setStart;
				sortStack[(sortDivisions*2)+1] = endVal;
				sortDivisions++;
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 CameraSortEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void CameraSortEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void CameraSortEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void CameraSortEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * CameraSortEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void CameraSortEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool CameraSortEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
