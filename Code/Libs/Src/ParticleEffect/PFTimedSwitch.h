#ifndef PFTIMEDSWITCH_H
#define PFTIMEDSWITCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFTimedSwitch.h                           //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

enum TimedSwitchType
{
	TST_ONCE,
	TST_OSCILATE,
};

enum TimedSwitchUpdate
{
	TSU_HARD,
	TSU_SOFT,
};

struct TimedSwitchProgramer : public ParticleProgramer
{
	FloatType * switchTime;
	TimedSwitchType type;
	TimedSwitchUpdate updateType;

	TimedSwitchProgramer();
	~TimedSwitchProgramer();

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

TimedSwitchProgramer::TimedSwitchProgramer()
{
	switchTime = MakeDefaultFloat(0);
	type = TST_ONCE;
	updateType = TSU_HARD;
}

TimedSwitchProgramer::~TimedSwitchProgramer()
{
	delete switchTime;
}

//IParticleProgramer
U32 TimedSwitchProgramer::GetNumOutput()
{
	return 2;
}

const char * TimedSwitchProgramer::GetOutputName(U32 index)
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

U32 TimedSwitchProgramer::GetNumFloatParams()
{
	return 1;
}

const char * TimedSwitchProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Timer";
	}
	return NULL;
}

const FloatType * TimedSwitchProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return switchTime;
	}
	return NULL;
}

void TimedSwitchProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(switchTime)
				delete switchTime;
			if(param)
				switchTime = param->CreateCopy();
			else
				switchTime = NULL;
			break;
		}
	}
}

U32 TimedSwitchProgramer::GetNumEnumParams()
{
	return 2;
}

const char * TimedSwitchProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Switch Type";
	case 1:
		return "Update Type";
	}
	return NULL;
}

U32 TimedSwitchProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	case 1:
		return 2;
	}
	return 0;
}

const char * TimedSwitchProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Once";
			case 1:
				return "Oscilate";
			}
		}
		break;
	case 1:
		{
			switch(value)
			{
			case 0:
				return "Hard";
			case 1:
				return "Soft";
			}
		}
		break;
	}
	return NULL;
}

U32 TimedSwitchProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return type;
	case 1:
		return updateType;
	}
	return 0;
}

void TimedSwitchProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		type = (TimedSwitchType)value;
		break;
	case 1:
		updateType = (TimedSwitchUpdate)value;
		break;
	}
}

U32 TimedSwitchProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)
		+ EncodeParam::EncodedFloatSize(switchTime)
		+sizeof(U32)
		+sizeof(U32);

}

void TimedSwitchProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_TIMED_SWITCH;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * switchTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(switchTimeHeader ,switchTime);
	offset +=switchTimeHeader->size;

	U32 * typeHeader = (U32*)(buffer+offset);
	(*typeHeader) = type;
	offset += sizeof(U32);

	U32 * updateHeader = (U32*)(buffer+offset);
	(*updateHeader) = updateType;
	offset += sizeof(U32);

	header->size = offset;
}

void TimedSwitchProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedFloatTypeHeader * switchTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	switchTime = EncodeParam::DecodeFloat(switchTimeHeader);
	offset += switchTimeHeader->size;

	type = *((TimedSwitchType*)(buffer+offset));
	offset += sizeof(U32);

	updateType = *((TimedSwitchUpdate*)(buffer+offset));
	offset += sizeof(U32);
}

struct TimedSwitchEffect : public IParticleEffect
{
	IParticleEffect * output1;//0
	IParticleEffect * output2;//1

	EncodedFloatTypeHeader * switchTime;
	TimedSwitchType type;
	TimedSwitchUpdate updateType;

	U32 numInst;
	struct InstStruct
	{
		SINGLE time;
		bool bState;
	};

	InstStruct * instStruct;

	U8 * data;

	TimedSwitchEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~TimedSwitchEffect();
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

TimedSwitchEffect::TimedSwitchEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	numInst = 1;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = new U8[header->size];
	U32 offset = sizeof(ParticleHeader);
	memcpy(data,buffer,header->size);

	switchTime = (EncodedFloatTypeHeader *)(data+offset);
	offset += switchTime->size;

	type = *((TimedSwitchType *)(data+offset));
	offset += sizeof(U32);

	updateType = *((TimedSwitchUpdate *)(data+offset));
	offset += sizeof(U32);

	output1 = NULL;//0
	output2 = NULL;//1
}

TimedSwitchEffect::~TimedSwitchEffect()
{
	delete [] instStruct;
	delete [] data;
	if(output1)
		delete output1;
	if(output2)
		delete output2;
}

IParticleEffectInstance * TimedSwitchEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
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

void TimedSwitchEffect::Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(updateType == TSU_HARD)
	{
		if(instStruct[instance].bState)
		{
			if(output1)
				output1->Render(t,inputStart,numInput,instance,parentTrans);
		}
		else
		{
			if(output2)
				output2->Render(t,inputStart,numInput,instance,parentTrans);
		}
	}
	else
	{
		if(output1)
			output1->Render(t,inputStart,numInput,instance,parentTrans);
		if(output2)
			output2->Render(t,inputStart,numInput,instance,parentTrans);
	}
}

bool TimedSwitchEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	if(type == TST_ONCE && instStruct[instance].bState)
	{
		Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
		SINGLE swTime = EncodeParam::GetFloat(switchTime,p2,parentSystem);
		instStruct[instance].time += DELTA_TIME;
		if(instStruct[instance].time > swTime)
		{
			instStruct[instance].bState = false;
		}
	}
	else if(type == TST_OSCILATE)
	{
		Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
		SINGLE swTime = EncodeParam::GetFloat(switchTime,p2,parentSystem);
		instStruct[instance].time += DELTA_TIME;
		if(instStruct[instance].time > swTime)
		{
			instStruct[instance].time -= swTime;
			instStruct[instance].bState = !instStruct[instance].bState;
		}
	}
	bool bRet1 = false;
	bool bRet2 = false;
	if(updateType == TSU_HARD)
	{
		if(instStruct[instance].bState)
		{
			if(output1)
				bRet1 = output1->Update(inputStart,numInput,bShutdown,instance);
		}
		else
		{
			if(output2)
				bRet2 = output2->Update(inputStart,numInput,bShutdown,instance);
		}
	}
	else
	{
		if(instStruct[instance].bState)
		{
			if(output1)
				bRet1 = output1->Update(inputStart,numInput,bShutdown,instance);
			if(output2)
				bRet2 = output2->Update(inputStart,numInput,true,instance);
		}
		else
		{
			if(output1)
				bRet1 = output1->Update(inputStart,numInput,true,instance);
			if(output2)
				bRet2 = output2->Update(inputStart,numInput,bShutdown,instance);
		}
	}

	return (bRet1 || bRet2);
}

U32 TimedSwitchEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].bState = true;
		instStruct[i].time = 0;
	}
	U32 count = 0;
	if(output1)
		count += output1->ParticlesUsed();
	if(output2)
		count += output2->ParticlesUsed();
	return count;
}

void TimedSwitchEffect::FindAllocation(U32 & startPos)
{
	if(output1)
		output1->FindAllocation(startPos);
	if(output2)
		output2->FindAllocation(startPos);
}

void TimedSwitchEffect::DeleteOutput()
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

IParticleEffect * TimedSwitchEffect::FindFilter(const char * searchName)
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

void TimedSwitchEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
	if(output1)
		output1->SetInstanceNumber(numInstances);
	if(output2)
		output2->SetInstanceNumber(numInstances);
}

bool TimedSwitchEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output1)
		if(output1->GetParentPosition(index,postion,lastIndex))
			return true;
	if(output2)
		return output2->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
