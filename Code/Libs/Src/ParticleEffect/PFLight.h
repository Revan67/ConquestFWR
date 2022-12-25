#ifndef PFLIGHT_H
#define PFLIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFLight.h                                   //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct LightProgramer : public ParticleProgramer
{
	FloatType * fadeInTime;
	FloatType * fadeOutTime;

	FloatType * diffuseRed;
	FloatType * diffuseGreen;
	FloatType * diffuseBlue;
	FloatType * ambientRed;
	FloatType * ambientGreen;
	FloatType * ambientBlue;
	
	FloatType * linearAttenuation;
	FloatType * squaredAttenuation;
	FloatType * exponentialAttenuation;

	LightProgramer();
	~LightProgramer();

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

LightProgramer::LightProgramer()
{
	fadeInTime = MakeDefaultFloat(0);
	fadeOutTime = MakeDefaultFloat(0);

	diffuseRed = MakeDefaultFloat(1);
	diffuseGreen = MakeDefaultFloat(1);
	diffuseBlue = MakeDefaultFloat(1);
	ambientRed = MakeDefaultFloat(0);
	ambientGreen = MakeDefaultFloat(0);
	ambientBlue = MakeDefaultFloat(0);
	
	linearAttenuation = MakeDefaultFloat(0);
	squaredAttenuation = MakeDefaultFloat(1.0);
	exponentialAttenuation = MakeDefaultFloat(0);
}

LightProgramer::~LightProgramer()
{
	delete fadeInTime;
	delete fadeOutTime;

	delete diffuseRed;
	delete diffuseGreen;
	delete diffuseBlue;
	delete ambientRed;
	delete ambientGreen;
	delete ambientBlue;
	
	delete linearAttenuation;
	delete squaredAttenuation;
	delete exponentialAttenuation;
}

//IParticleProgramer
U32 LightProgramer::GetNumOutput()
{
	return 1;
}

const char * LightProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 LightProgramer::GetNumFloatParams()
{
	return 11;
}

const char * LightProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Fade In Time";
	case 1:
		return "Fade Out Time";
	case 2:
		return "Diffuse Red";
	case 3:
		return "Diffuse Green";
	case 4:
		return "Diffuse Blue";
	case 5:
		return "Ambient Red";
	case 6:
		return "Ambient Green";
	case 7:
		return "Ambient Blue";
	case 8:
		return "Linear Attenuation";
	case 9:
		return "Squared Attenuation";
	case 10:
		return "Exponential Attenuation";
	}
	return NULL;
}

const FloatType * LightProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return fadeInTime;
	case 1:
		return fadeOutTime;
	case 2:
		return diffuseRed;
	case 3:
		return diffuseGreen;
	case 4:
		return diffuseBlue;
	case 5:
		return ambientRed;
	case 6:
		return ambientGreen;
	case 7:
		return ambientBlue;
	case 8:
		return linearAttenuation;
	case 9:
		return squaredAttenuation;
	case 10:
		return exponentialAttenuation;
	}
	return NULL;
}

void LightProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(fadeInTime)
				delete fadeInTime;
			if(param)
				fadeInTime = param->CreateCopy();
			else
				fadeInTime = NULL;
			break;
		}
	case 1:
		{
			if(fadeOutTime)
				delete fadeOutTime;
			if(param)
				fadeOutTime = param->CreateCopy();
			else
				fadeOutTime = NULL;
			break;
		}
	case 2:
		{
			if(diffuseRed)
				delete diffuseRed;
			if(param)
				diffuseRed = param->CreateCopy();
			else
				diffuseRed = NULL;
			break;
		}
	case 3:
		{
			if(diffuseGreen)
				delete diffuseGreen;
			if(param)
				diffuseGreen = param->CreateCopy();
			else
				diffuseGreen = NULL;
			break;
		}
	case 4:
		{
			if(diffuseBlue)
				delete diffuseBlue;
			if(param)
				diffuseBlue = param->CreateCopy();
			else
				diffuseBlue = NULL;
			break;
		}
	case 5:
		{
			if(ambientRed)
				delete ambientRed;
			if(param)
				ambientRed = param->CreateCopy();
			else
				ambientRed = NULL;
			break;
		}
	case 6:
		{
			if(ambientGreen)
				delete ambientGreen;
			if(param)
				ambientGreen = param->CreateCopy();
			else
				ambientGreen = NULL;
			break;
		}
	case 7:
		{
			if(ambientBlue)
				delete ambientBlue;
			if(param)
				ambientBlue = param->CreateCopy();
			else
				ambientBlue = NULL;
			break;
		}
	case 8:
		{
			if(linearAttenuation)
				delete linearAttenuation;
			if(param)
				linearAttenuation = param->CreateCopy();
			else
				linearAttenuation = NULL;
			break;
		}
	case 9:
		{
			if(squaredAttenuation)
				delete squaredAttenuation;
			if(param)
				squaredAttenuation = param->CreateCopy();
			else
				squaredAttenuation = NULL;
			break;
		}
	case 10:
		{
			if(exponentialAttenuation)
				delete exponentialAttenuation;
			if(param)
				exponentialAttenuation = param->CreateCopy();
			else
				exponentialAttenuation = NULL;
			break;
		}
	}
}

U32 LightProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(fadeInTime)
		+ EncodeParam::EncodedFloatSize(fadeOutTime)
		+ EncodeParam::EncodedFloatSize(diffuseRed)
		+ EncodeParam::EncodedFloatSize(diffuseGreen)
		+ EncodeParam::EncodedFloatSize(diffuseBlue)
		+ EncodeParam::EncodedFloatSize(ambientRed)
		+ EncodeParam::EncodedFloatSize(ambientGreen)
		+ EncodeParam::EncodedFloatSize(ambientBlue)
		+ EncodeParam::EncodedFloatSize(linearAttenuation)
		+ EncodeParam::EncodedFloatSize(squaredAttenuation)
		+ EncodeParam::EncodedFloatSize(exponentialAttenuation);
}

void LightProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_LIGHT;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * fadeInTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(fadeInTimeHeader ,fadeInTime);
	offset +=fadeInTimeHeader->size;

	EncodedFloatTypeHeader * fadeOutTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(fadeOutTimeHeader ,fadeOutTime);
	offset +=fadeOutTimeHeader->size;

	EncodedFloatTypeHeader * diffuseRedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(diffuseRedHeader ,diffuseRed);
	offset +=diffuseRedHeader->size;

	EncodedFloatTypeHeader * diffuseGreenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(diffuseGreenHeader ,diffuseGreen);
	offset +=diffuseGreenHeader->size;

	EncodedFloatTypeHeader * diffuseBlueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(diffuseBlueHeader ,diffuseBlue);
	offset +=diffuseBlueHeader->size;

	EncodedFloatTypeHeader * ambientRedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(ambientRedHeader ,ambientRed);
	offset +=ambientRedHeader->size;

	EncodedFloatTypeHeader * ambientGreenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(ambientGreenHeader ,ambientGreen);
	offset +=ambientGreenHeader->size;

	EncodedFloatTypeHeader * ambientBlueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(ambientBlueHeader ,ambientBlue);
	offset +=ambientBlueHeader->size;

	EncodedFloatTypeHeader * linearAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(linearAttenuationHeader ,linearAttenuation);
	offset +=linearAttenuationHeader->size;

	EncodedFloatTypeHeader * squaredAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(squaredAttenuationHeader ,squaredAttenuation);
	offset +=squaredAttenuationHeader->size;

	EncodedFloatTypeHeader * exponentialAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(exponentialAttenuationHeader ,exponentialAttenuation);
	offset +=exponentialAttenuationHeader->size;

	header->size = offset;
}

void LightProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * fadeInTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	fadeInTime = EncodeParam::DecodeFloat(fadeInTimeHeader);
	offset += fadeInTimeHeader->size;

	EncodedFloatTypeHeader * fadeOutTimeHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	fadeOutTime = EncodeParam::DecodeFloat(fadeOutTimeHeader);
	offset += fadeOutTimeHeader->size;

	EncodedFloatTypeHeader * diffuseRedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	diffuseRed = EncodeParam::DecodeFloat(diffuseRedHeader);
	offset += diffuseRedHeader->size;

	EncodedFloatTypeHeader * diffuseGreenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	diffuseGreen = EncodeParam::DecodeFloat(diffuseGreenHeader);
	offset += diffuseGreenHeader->size;

	EncodedFloatTypeHeader * diffuseBlueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	diffuseBlue = EncodeParam::DecodeFloat(diffuseBlueHeader);
	offset += diffuseBlueHeader->size;

	EncodedFloatTypeHeader * ambientRedHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	ambientRed = EncodeParam::DecodeFloat(ambientRedHeader);
	offset += ambientRedHeader->size;

	EncodedFloatTypeHeader * ambientGreenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	ambientGreen = EncodeParam::DecodeFloat(ambientGreenHeader);
	offset += ambientGreenHeader->size;

	EncodedFloatTypeHeader * ambientBlueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	ambientBlue = EncodeParam::DecodeFloat(ambientBlueHeader);
	offset += ambientBlueHeader->size;

	EncodedFloatTypeHeader * linearAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	linearAttenuation = EncodeParam::DecodeFloat(linearAttenuationHeader);
	offset += linearAttenuationHeader->size;

	EncodedFloatTypeHeader * squaredAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	squaredAttenuation = EncodeParam::DecodeFloat(squaredAttenuationHeader);
	offset += squaredAttenuationHeader->size;

	EncodedFloatTypeHeader * exponentialAttenuationHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	exponentialAttenuation = EncodeParam::DecodeFloat(exponentialAttenuationHeader);
	offset += exponentialAttenuationHeader->size;
}

struct LightEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedFloatTypeHeader * fadeInTime;
	EncodedFloatTypeHeader * fadeOutTime;

	EncodedFloatTypeHeader * diffuseRed;
	EncodedFloatTypeHeader * diffuseGreen;
	EncodedFloatTypeHeader * diffuseBlue;
	EncodedFloatTypeHeader * ambientRed;
	EncodedFloatTypeHeader * ambientGreen;
	EncodedFloatTypeHeader * ambientBlue;
	
	EncodedFloatTypeHeader * linearAttenuation;
	EncodedFloatTypeHeader * squaredAttenuation;
	EncodedFloatTypeHeader * exponentialAttenuation;

	U8 * data;

	U32 numInst;

	struct InstStruct
	{
		IRenderLight * light;
		U32 fadeOutTime;
	};
	InstStruct * instStruct;

	LightEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~LightEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer);
	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 inst, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 inst);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
	virtual void SetInstanceNumber(U32 numInstances);
};

LightEffect::LightEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer) : IParticleEffect(_parent,_parentEffect)
{
	numInst = 1;
	instStruct = NULL;
	output = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);
	data = new U8[header->size];
	memcpy(data,buffer,header->size);

	fadeInTime = (EncodedFloatTypeHeader *)(data+offset);
	offset += fadeInTime->size;

	fadeOutTime = (EncodedFloatTypeHeader *)(data+offset);
	offset += fadeOutTime->size;

	diffuseRed = (EncodedFloatTypeHeader *)(data+offset);
	offset += diffuseRed->size;

	diffuseGreen = (EncodedFloatTypeHeader *)(data+offset);
	offset += diffuseGreen->size;

	diffuseBlue = (EncodedFloatTypeHeader *)(data+offset);
	offset += diffuseBlue->size;

	ambientRed = (EncodedFloatTypeHeader *)(data+offset);
	offset += ambientRed->size;

	ambientGreen = (EncodedFloatTypeHeader *)(data+offset);
	offset += ambientGreen->size;

	ambientBlue = (EncodedFloatTypeHeader *)(data+offset);
	offset += ambientBlue->size;

	linearAttenuation = (EncodedFloatTypeHeader *)(data+offset);
	offset += linearAttenuation->size;

	squaredAttenuation = (EncodedFloatTypeHeader *)(data+offset);
	offset += squaredAttenuation->size;

	exponentialAttenuation = (EncodedFloatTypeHeader *)(data+offset);
	offset += exponentialAttenuation->size;

}

LightEffect::~LightEffect()
{
	delete [] data;
	for(U32 i = 0; i < numInst; ++i)
	{
		if(instStruct[i].light)
		{
			instStruct[i].light->deactivate();
			GMRENDERER->destroyLight(instStruct[i].light);
			instStruct[i].light = NULL;
		}
	}
	delete [] instStruct;
}

IParticleEffectInstance * LightEffect::AddFilter(U32 outputID, ParticleEffectType type, U8 * buffer)
{
	if(output)
		delete output;
	output = createParticalEffect(parentSystem,type,buffer,this);
	return output;
}

void LightEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(p1->bLive && p2->bLive)
	{
		if(p1->birthTime < currentTimeMS)
		{
			instStruct[instance].fadeOutTime = currentTimeMS;
			LIGHT_DATA lightInfo;
			if(!(instStruct[instance].light))
			{
				//make a new light
				lightInfo.lightProps.overbright = 1;
				lightInfo.bDirectionalLight = false;
				lightInfo.bPulseLight = false;
				lightInfo.position = (t*(p2->pos - p1->pos))+p1->pos;
				lightInfo.lightProps.ambient.red = 0;
				lightInfo.lightProps.ambient.green = 0;
				lightInfo.lightProps.ambient.blue = 0;
				lightInfo.lightProps.diffuse.red = 0;
				lightInfo.lightProps.diffuse.green = 0;
				lightInfo.lightProps.diffuse.blue = 0;
				lightInfo.lightProps.attenuation = 0;
				lightInfo.bStaticLight = 0;
				instStruct[instance].light = GMRENDERER->makeLight(&lightInfo);
				instStruct[instance].light->activate();
				
			}
			instStruct[instance].light->getData(&lightInfo);
			SINGLE fadeIn = 1.0;
			SINGLE lifeTime = (F2LONG(currentTimeMS-p1->birthTime)/1000.0f);
			SINGLE fadeInValue = EncodeParam::GetFloat(fadeInTime,&(p2[0]),parentSystem);
			if(fadeInValue > lifeTime)
			{
				fadeIn = lifeTime/fadeInValue;
			}
			SINGLE fadeOut = 1.0;
			SINGLE lifeLeft = (p1->lifeTime/1000.0f)-lifeTime;
			SINGLE fadeOutValue = EncodeParam::GetFloat(fadeOutTime,&(p2[0]),parentSystem);
			if(fadeOutValue > lifeLeft)
			{
				fadeOut = __max(0,lifeLeft/fadeOutValue);
			}
			SINGLE fade = fadeIn*fadeOut;
		
			lightInfo.position = (t*(p2->pos - p1->pos))+p1->pos;
			lightInfo.lightProps.ambient.red = EncodeParam::GetFloat(ambientRed,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.ambient.green = EncodeParam::GetFloat(ambientGreen,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.ambient.blue = EncodeParam::GetFloat(ambientBlue,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.diffuse.red = EncodeParam::GetFloat(diffuseRed,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.diffuse.green = EncodeParam::GetFloat(diffuseGreen,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.diffuse.blue = EncodeParam::GetFloat(diffuseBlue,&(p2[0]),parentSystem)*255;
			lightInfo.lightProps.attenuation = EncodeParam::GetFloat(exponentialAttenuation,&(p2[0]),parentSystem);

			lightInfo.lightProps.attenuation += sqrt(1.0 - fade) * 3000;

			instStruct[instance].light->setData(&lightInfo);
		}
	}
	else if(instStruct[instance].light)
	{
		instStruct[instance].light->deactivate();
		GMRENDERER->destroyLight(instStruct[instance].light);
		instStruct[instance].light = NULL;
	}
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool LightEffect::Update(U32 inputStart, U32 numInput, bool bShutdown,U32 instance)
{
	bool retVal = false;
	if(output)
		 retVal = output->Update(inputStart,numInput,bShutdown,instance);
	if(instStruct[instance].light)
		return true;
	return retVal;
}

U32 LightEffect::ParticlesUsed()
{
	instStruct = new InstStruct[numInst];
	for(U32 i = 0; i < numInst; ++i)
	{
		instStruct[i].light = NULL;
		instStruct[i].fadeOutTime = 0;
	}
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void LightEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void LightEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

IParticleEffect * LightEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void LightEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool LightEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
