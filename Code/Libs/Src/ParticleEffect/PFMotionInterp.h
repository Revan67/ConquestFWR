#ifndef PFMOTIONINTERP_H
#define PFMOTIONINTERP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMotionInterp.h                           //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MotionInterpProgramer : public ParticleProgramer
{
	FloatType * speed;

	TransformType * startTrans;
	TransformType * endTrans;

	MotionInterpProgramer();
	~MotionInterpProgramer();

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

MotionInterpProgramer::MotionInterpProgramer()
{
	speed = MakeDefaultFloat(10);
	startTrans = MakeDefaultTrans();
	endTrans = MakeDefaultTrans();
}

MotionInterpProgramer::~MotionInterpProgramer()
{
	delete speed;
	delete startTrans;
	delete endTrans;
}

//IParticleProgramer
U32 MotionInterpProgramer::GetNumOutput()
{
	return 2;
}

const char * MotionInterpProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Moving Effect";
	case 1:
		return "On Completed Effect";
	}
	return "Error";
}

U32 MotionInterpProgramer::GetNumInput()
{
	return 1;
}

const char * MotionInterpProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 MotionInterpProgramer::GetNumFloatParams()
{
	return 1;
}

const char * MotionInterpProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Speed";
	}
	return NULL;
}

const FloatType * MotionInterpProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return speed;
	}
	return NULL;
}

void MotionInterpProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(speed)
				delete speed;
			if(param)
				speed = param->CreateCopy();
			else
				speed = NULL;
			break;
		}
	}
}

U32 MotionInterpProgramer::GetNumTransformParams()
{
	return 2;
}

const char * MotionInterpProgramer::GetTransformParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Start Location";
	case 1:
		return "End Location";
	}
	return "Error";
}

const TransformType * MotionInterpProgramer::GetTransformParam(U32 index)
{
	switch(index)
	{
	case 0:
		return startTrans;
	case 1:
		return endTrans;
	}
	return NULL;
}

void MotionInterpProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	switch(index)
	{
	case 0:
		{
			if(startTrans)
				delete startTrans;
			if(param)
				startTrans = param->CreateCopy();
			else
				startTrans = NULL;
			break;
		}
	case 1:
		{
			if(endTrans)
				delete endTrans;
			if(param)
				endTrans = param->CreateCopy();
			else
				endTrans = NULL;
			break;
		}
	}
}

U32 MotionInterpProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(speed)
		+ EncodeParam::EncodedTransformSize(startTrans)
		+ EncodeParam::EncodedTransformSize(endTrans);
}

void MotionInterpProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MOTION_INTERP;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(speedHeader ,speed);
	offset +=speedHeader->size;

	EncodedTransformTypeHeader * startTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(startTransHeader ,startTrans);
	offset +=startTransHeader->size;

	EncodedTransformTypeHeader * endTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(endTransHeader ,endTrans);
	offset +=endTransHeader->size;

	header->size = offset;
}

void MotionInterpProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedFloatTypeHeader * speedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	speed = EncodeParam::DecodeFloat(speedHeader);
	offset += speedHeader->size;

	EncodedTransformTypeHeader * startTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	startTrans = EncodeParam::DecodeTransform(startTransHeader);
	offset += startTransHeader->size;

	EncodedTransformTypeHeader * endTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	endTrans = EncodeParam::DecodeTransform(endTransHeader);
	offset += endTransHeader->size;
}

struct MotionInterpEffect : public IParticleEffect
{
	IParticleEffect * workingOutput;//0
	IParticleEffect * onCompleteOutput;//1


	EncodedFloatTypeHeader * speed;

	EncodedTransformTypeHeader * startTrans;
	EncodedTransformTypeHeader * endTrans;

	U8 * data;

	U32 numInst;

	struct InstStruct
	{
		U32 particleRegionStart;
		bool bStarted;
		bool bCompleted;
	};
	InstStruct * instStruct;
	
	MotionInterpEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MotionInterpEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	virtual U32 GetAllocationStart(IParticleEffectInstance * target,U32 instance);
	virtual void GetTransform(SINGLE t,Transform & trans,U32 instance);

	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual U32 OutputRange();
};

MotionInterpEffect::MotionInterpEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	workingOutput = NULL;//0
	onCompleteOutput = NULL;//1
	numInst = 1;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	speed = (EncodedFloatTypeHeader *)(data+offset);
	offset += speed->size;

	startTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += startTrans->size;
	
	endTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += endTrans->size;
}

MotionInterpEffect::~MotionInterpEffect()
{
	delete [] instStruct;
	if(workingOutput)
	{
		delete workingOutput;
		workingOutput = NULL;
	}
	if(onCompleteOutput)
	{
		delete onCompleteOutput;
		onCompleteOutput = NULL;
	}
}

IParticleEffectInstance * MotionInterpEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type,U8 * buffer)
{
	switch(outputID)
	{
	case 0:
		if(workingOutput)
			delete workingOutput;
		workingOutput = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return workingOutput;
	case 1:
		if(onCompleteOutput)
			delete onCompleteOutput;
		onCompleteOutput = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return onCompleteOutput;
	}
	return NULL;
}

void MotionInterpEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(workingOutput)
			delete workingOutput;
		workingOutput = (IParticleEffect*)target;
		workingOutput->parentEffect[inputID] = this;
	}
	else if(outputID == 1)
	{
		if(onCompleteOutput)
			delete onCompleteOutput;
		onCompleteOutput = (IParticleEffect*)target;
		onCompleteOutput->parentEffect[inputID] = this;
	}
};

U32 MotionInterpEffect::GetAllocationStart(IParticleEffectInstance * target,U32 instance)
{
	return instStruct[instance].particleRegionStart;
}

void MotionInterpEffect::GetTransform(SINGLE t, Transform & trans, U32 instance)
{
	trans.set_identity();
	Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);

	Vector pos = (t*(p2->pos-p1->pos))+p1->pos;
	Vector j = p2->vel;
	if(j.magnitude_squared() == 0)
		j = Vector(0,1,0);
	j.fast_normalize();
	Vector k(0,0,1);
	if((j-k).magnitude_squared() == 0)
		k = Vector(0,1,0);
	Vector i = cross_product(j,k);
	i.fast_normalize();
	k = cross_product(i,j);
	k.fast_normalize();
	j = cross_product(k,i);
	j.fast_normalize();
	trans.set_i(i);
	trans.set_j(j);
	trans.set_k(k);
	trans.translation = pos;
};

void MotionInterpEffect::Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(workingOutput)
		workingOutput->Render(t,instStruct[instance].particleRegionStart,1,instance,parentTrans);
	if(instStruct[instance].bCompleted)
	{
		if(onCompleteOutput)
			onCompleteOutput->Render(t,instStruct[instance].particleRegionStart,1,instance,parentTrans);
	}
}

bool MotionInterpEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	if(!instStruct[instance].bStarted)
	{
		Transform transStart;
		Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
		if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,startTrans,transStart,instance))
		{
			Transform transEnd;
			if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,endTrans,transEnd,instance))
			{
				p1->bLive = false;
				p1->pos = p2->pos = transStart.translation;
				p2->bLive = true;
				p2->bNew = true;
				p2->bNoComputeVelocity = false;
				p2->birthTime = parentSystem->GetCurrentTimeMS();
				p2->lifeTime = 0xFFFFFFFF;
				p1->vel = p2->vel = (transEnd.translation-transStart.translation).fast_normalize()*EncodeParam::GetFloat(speed,p2,parentSystem)*FEET_TO_WORLD;
				instStruct[instance].bStarted = true;
			}
		}
	}
	else if(instStruct[instance].bCompleted)
	{
		Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
		p2->vel = Vector(0,0,0);
		if(workingOutput)
			workingOutput->Update(instStruct[instance].particleRegionStart,1,true,instance);
		if(onCompleteOutput)
			return onCompleteOutput->Update(instStruct[instance].particleRegionStart,1,bShutdown,instance);
	}
	else
	{
		Transform transEnd;
		Particle * p1 = &(parentSystem->myParticles->particles1[instStruct[instance].particleRegionStart]);
		Particle * p2 = &(parentSystem->myParticles->particles2[instStruct[instance].particleRegionStart]);
		if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,endTrans,transEnd,instance))
		{
			SINGLE dist = p1->vel.magnitude_squared()*DELTA_TIME;
			if((p1->pos-transEnd.translation).magnitude_squared() < dist)
			{
				p2->bLive = true;
				p2->bNew = false;
				p2->bNoComputeVelocity = false;
				p2->birthTime = p1->birthTime;
				p2->lifeTime = p1->lifeTime;
				p2->vel = (transEnd.translation-p2->pos)/DELTA_TIME;
				if(workingOutput)
					workingOutput->Update(instStruct[instance].particleRegionStart,1,bShutdown,instance);
				instStruct[instance].bCompleted = true;
				return true;
			}
			else
			{
				p2->bLive = true;
				p2->bNew = false;
				p2->bNoComputeVelocity = false;
				p2->birthTime = p1->birthTime;
				p2->lifeTime = p1->lifeTime;
				p2->vel = (transEnd.translation-p1->pos).fast_normalize() * EncodeParam::GetFloat(speed,p2,parentSystem)*FEET_TO_WORLD;
				if(workingOutput)
					return workingOutput->Update(instStruct[instance].particleRegionStart,1,bShutdown,instance);
			}
		}
	}

	return false;
}

U32 MotionInterpEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].bCompleted = false;
		instStruct[i].bStarted = false;
		instStruct[i].particleRegionStart = 0;
	}
	U32 count = numInst;
	if(workingOutput)
		count += workingOutput->ParticlesUsed();
	if(onCompleteOutput)
		count += onCompleteOutput->ParticlesUsed();
	return count;
}

void MotionInterpEffect::FindAllocation(U32 & startPos)
{
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].particleRegionStart = startPos;
		startPos += 1;
	}
	if(workingOutput)
		workingOutput->FindAllocation(startPos);
	if(onCompleteOutput)
		onCompleteOutput->FindAllocation(startPos);
}

void MotionInterpEffect::DeleteOutput()
{
	if(workingOutput)
	{
		delete workingOutput;
		workingOutput = NULL;
	}
	if(onCompleteOutput)
	{
		delete onCompleteOutput;
		onCompleteOutput = NULL;
	}
}

void MotionInterpEffect::NullOutput(IParticleEffect * target)
{
	if(workingOutput == target)
		workingOutput = NULL;
	else if(onCompleteOutput == target)
		onCompleteOutput = NULL;
}

IParticleEffect * MotionInterpEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * retVal = NULL;
	if(workingOutput)
		retVal = workingOutput->FindFilter(searchName);
	if(retVal)
		return retVal;
	else if(onCompleteOutput)
		return onCompleteOutput->FindFilter(searchName);
	return NULL;
}

void MotionInterpEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(workingOutput)
		workingOutput->SetInstanceNumber(numInstances);
	if(onCompleteOutput)
		onCompleteOutput->SetInstanceNumber(numInstances);
}

bool MotionInterpEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	for(U32 instance = 0; instance < numInst; ++instance)
	{
		U32 last = instStruct[instance].particleRegionStart+2;
		if(index >= instStruct[instance].particleRegionStart && index < last)
		{
			return false;
		}
	}
	if(workingOutput)
		if(workingOutput->GetParentPosition(index,postion,lastIndex))
			return true;
	if(onCompleteOutput)
		return onCompleteOutput->GetParentPosition(index,postion,lastIndex);
	return false;
}

U32 MotionInterpEffect::OutputRange()
{
	return 1;
};

#endif
