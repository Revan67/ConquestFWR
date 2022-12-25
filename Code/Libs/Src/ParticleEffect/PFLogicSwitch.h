#ifndef PFLOGICSWITCH_H
#define PFLOGICSWITCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFLogicSwitch.h                           //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

enum LogicSwitchType
{
	LST_EQUAL,
	LST_NOTEQUAL,
	LST_GREATERTHAN,
	LST_GREATEREQTHAN,
	LST_LESSTHAN,
	LST_LESSEQTHAN,
};

struct LogicSwitchProgramer : public ParticleProgramer
{
	FloatType * value1;
	FloatType * value2;
	LogicSwitchType type;

	LogicSwitchProgramer();
	~LogicSwitchProgramer();

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

LogicSwitchProgramer::LogicSwitchProgramer()
{
	value1 = MakeDefaultFloat(0);
	value2 = MakeDefaultFloat(0);
	type = LST_EQUAL;
}

LogicSwitchProgramer::~LogicSwitchProgramer()
{
	delete value1;
	delete value2;
}

//IParticleProgramer
U32 LogicSwitchProgramer::GetNumOutput()
{
	return 2;
}

const char * LogicSwitchProgramer::GetOutputName(U32 index)
{
	switch(index)
	{
	case 0:
		return "True Output";
	case 1:
		return "False Output";
	}
	return "Error";
}

U32 LogicSwitchProgramer::GetNumFloatParams()
{
	return 2;
}

const char * LogicSwitchProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Value 1";
	case 1:
		return "Value 2";
	}
	return NULL;
}

const FloatType * LogicSwitchProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return value1;
	case 1:
		return value2;
	}
	return NULL;
}

void LogicSwitchProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(value1)
				delete value1;
			if(param)
				value1 = param->CreateCopy();
			else
				value1 = NULL;
			break;
		}
	case 1:
		{
			if(value2)
				delete value2;
			if(param)
				value2 = param->CreateCopy();
			else
				value2 = NULL;
			break;
		}
	}
}

U32 LogicSwitchProgramer::GetNumEnumParams()
{
	return 1;
}

const char * LogicSwitchProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Comparison Type";
	}
	return NULL;
}

U32 LogicSwitchProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 6;
	}
	return 0;
}

const char * LogicSwitchProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Equal";
			case 1:
				return "Not Equal";
			case 2:
				return "Greater Than";
			case 3:
				return "Greater Than or Equal";
			case 4:
				return "Less Than";
			case 5:
				return "Less Than or Equal";
			}
		}
		break;
	}
	return NULL;
}

U32 LogicSwitchProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return type;
	}
	return 0;
}

void LogicSwitchProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		type = (LogicSwitchType)value;
		break;
	}
}

U32 LogicSwitchProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)
		+ EncodeParam::EncodedFloatSize(value1)
		+ EncodeParam::EncodedFloatSize(value2)
		+sizeof(U32);

}

void LogicSwitchProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_LOGIC_SWITCH;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * value1Header = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(value1Header ,value1);
	offset +=value1Header->size;

	EncodedFloatTypeHeader * value2Header = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(value2Header ,value2);
	offset +=value2Header->size;

	U32 * typeHeader = (U32*)(buffer+offset);
	(*typeHeader) = type;
	offset += sizeof(U32);

	header->size = offset;
}

void LogicSwitchProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	U32 offset = sizeof(ParticleHeader);
	strcpy(effectName,header->effectName);

	EncodedFloatTypeHeader * value1Header = (EncodedFloatTypeHeader *)(buffer+offset);
	value1 = EncodeParam::DecodeFloat(value1Header);
	offset += value1Header->size;

	EncodedFloatTypeHeader * value2Header = (EncodedFloatTypeHeader *)(buffer+offset);
	value2 = EncodeParam::DecodeFloat(value2Header);
	offset += value2Header->size;

	type = *((LogicSwitchType*)(buffer+offset));
	offset += sizeof(U32);
}

struct LogicSwitchEffect : public IParticleEffect
{
	IParticleEffect * output1;//0
	IParticleEffect * output2;//1

	EncodedFloatTypeHeader * value1;
	EncodedFloatTypeHeader * value2;
	LogicSwitchType type;

	U8 * data;

	LogicSwitchEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~LogicSwitchEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual void SetInstanceNumber(U32 numInstances);
};

LogicSwitchEffect::LogicSwitchEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = new U8[header->size];
	U32 offset = sizeof(ParticleHeader);
	memcpy(data,buffer,header->size);

	value1 = (EncodedFloatTypeHeader *)(data+offset);
	offset += value1->size;

	value2 = (EncodedFloatTypeHeader *)(data+offset);
	offset += value2->size;

	type = *((LogicSwitchType *)(data+offset));
	offset += sizeof(U32);

	output1 = NULL;//0
	output2 = NULL;//1
}

LogicSwitchEffect::~LogicSwitchEffect()
{
	delete [] data;
	if(output1)
		delete output1;
	if(output2)
		delete output2;
}

IParticleEffectInstance * LogicSwitchEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
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

void LogicSwitchEffect::Render(SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	SINGLE val1 = EncodeParam::GetFloat(value1,p2,parentSystem);
	SINGLE val2 = EncodeParam::GetFloat(value2,p2,parentSystem);
	bool bTrue = true;
	switch(type)
	{
	case LST_EQUAL:
		{
			bTrue = (val1 == val2);
		}
		break;
	case LST_NOTEQUAL:
		{
			bTrue = (val1 != val2);
		}
		break;
	case LST_GREATERTHAN:
		{
			bTrue = (val1 > val2);
		}
		break;
	case LST_GREATEREQTHAN:
		{
			bTrue = (val1 >= val2);
		}
		break;
	case LST_LESSTHAN:
		{
			bTrue = (val1 < val2);
		}
		break;
	case LST_LESSEQTHAN:
		{
			bTrue = (val1 <= val2);
		}
		break;
	}

	if(bTrue)
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

bool LogicSwitchEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	SINGLE val1 = EncodeParam::GetFloat(value1,p2,parentSystem);
	SINGLE val2 = EncodeParam::GetFloat(value2,p2,parentSystem);
	bool bTrue = true;
	switch(type)
	{
	case LST_EQUAL:
		{
			bTrue = (val1 == val2);
		}
		break;
	case LST_NOTEQUAL:
		{
			bTrue = (val1 != val2);
		}
		break;
	case LST_GREATERTHAN:
		{
			bTrue = (val1 > val2);
		}
		break;
	case LST_GREATEREQTHAN:
		{
			bTrue = (val1 >= val2);
		}
		break;
	case LST_LESSTHAN:
		{
			bTrue = (val1 < val2);
		}
		break;
	case LST_LESSEQTHAN:
		{
			bTrue = (val1 <= val2);
		}
		break;
	}

	if(bTrue)
	{
		if(output1)
			return output1->Update(inputStart,numInput,bShutdown,instance);
	}
	else
	{
		if(output2)
			return output2->Update(inputStart,numInput,bShutdown,instance);
	}
	return false;
}

U32 LogicSwitchEffect::ParticlesUsed()
{
	U32 count = 0;
	if(output1)
		count += output1->ParticlesUsed();
	if(output2)
		count += output2->ParticlesUsed();
	return count;
}

void LogicSwitchEffect::FindAllocation(U32 & startPos)
{
	if(output1)
		output1->FindAllocation(startPos);
	if(output2)
		output2->FindAllocation(startPos);
}

void LogicSwitchEffect::DeleteOutput()
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

IParticleEffect * LogicSwitchEffect::FindFilter(const char * searchName)
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

void LogicSwitchEffect::SetInstanceNumber(U32 numInstances)
{
	if(output1)
		output1->SetInstanceNumber(numInstances);
	if(output2)
		output2->SetInstanceNumber(numInstances);
}

bool LogicSwitchEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output1)
		if(output1->GetParentPosition(index,postion,lastIndex))
			return true;
	if(output2)
		return output2->GetParentPosition(index,postion,lastIndex);
	return false;
}


#endif
