#ifndef PFMATMODFLOAT_H
#define PFMATMODFLOAT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMatModFloat.h                             //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MatModFloatProgramer : public ParticleProgramer
{
	FloatType * value;

	char floatName[256];
	char material[256];

	MatModFloatProgramer();
	~MatModFloatProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumFloatParams();

	virtual const char * GetFloatParamName(U32 index);

	virtual const FloatType * GetFloatParam(U32 index);

	virtual void SetFloatParam(U32 index,const FloatType * param);

	virtual U32 GetNumStringParams();

	virtual const char * GetStringParamName(U32 index);

	virtual const char * GetStringParam(U32 index);

	virtual void SetStringParam(U32 index,const char * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

MatModFloatProgramer::MatModFloatProgramer()
{
	material[0] = 0;
	floatName[0] = 0;
	value = MakeDefaultFloat(1.0f);
}

MatModFloatProgramer::~MatModFloatProgramer()
{
	delete value;
}

//IParticleProgramer
U32 MatModFloatProgramer::GetNumOutput()
{
	return 1;
}

const char * MatModFloatProgramer::GetOutputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 MatModFloatProgramer::GetNumInput()
{
	return 1;
}

const char * MatModFloatProgramer::GetInputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 MatModFloatProgramer::GetNumFloatParams()
{
	return 1;
}

const char * MatModFloatProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Value";
	}
	return NULL;
}

const FloatType * MatModFloatProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return value;
	}
	return NULL;
}

void MatModFloatProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(value)
				delete value;
			if(param)
				value = param->CreateCopy();
			else
				value = NULL;
			break;
		}

	}
}

U32 MatModFloatProgramer::GetNumStringParams()
{
	return 2;
}

const char * MatModFloatProgramer::GetStringParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Float Name";
	case 1:
		return "Material Name";
	}
	return "Error";
}

const char * MatModFloatProgramer::GetStringParam(U32 index)
{
	switch(index)
	{
	case 0:
		return floatName;
	case 1:
		return material;
	}
	return NULL;
}

void MatModFloatProgramer::SetStringParam(U32 index,const char * param)
{
	switch(index)
	{
	case 0:
		strcpy(floatName,param);
		break;
	case 1:
		strcpy(material,param);
		break;
	}
}

U32 MatModFloatProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) + EncodeParam::EncodedFloatSize(value) + 
		+ (sizeof(char)*256)
		+ (sizeof(char)*256);
}

void MatModFloatProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MATMOD_COLOR;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * valueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(valueHeader,value);
	offset += valueHeader->size;

	char * floatHeader = (char *)(buffer+offset);
	strcpy(floatHeader,floatName);
	offset += sizeof(char)*256;

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,material);
	offset += sizeof(char)*256;

	header->size = offset;
}

void MatModFloatProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * valueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	value = 	EncodeParam::DecodeFloat(valueHeader);
	offset += valueHeader->size;

	char * floatHeader = (char *)(buffer+offset);
	strcpy(floatName,floatHeader);
	offset += sizeof(char)*256;

	char * matHeader = (char *)(buffer+offset);
	strcpy(material,matHeader);
	offset += sizeof(char)*256;
}


struct MatModFloatEffect : public IParticleEffect
{
	IParticleEffect * modList;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * value;
	char * floatName;
	IMaterial * material;

	MatModFloatEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MatModFloatEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual IModifier * UpdateMatMod(IMaterial * mat, IModifier * inMod, SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
};

MatModFloatEffect::MatModFloatEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	material = NULL;
	modList = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	value = (EncodedFloatTypeHeader *)(data+offset);
	offset += value->size;

	floatName = (char*)(data+offset);
	offset += sizeof(char)*256;

	char * matName = (char*)(data+offset);
	offset += sizeof(char)*256;

	if(matName[0])
		material = parentSystem->GetOwner()->GetMaterialManager()->FindMaterial(matName);
	else
		material = NULL;
}

MatModFloatEffect::~MatModFloatEffect()
{
	if(modList)
		delete modList;
}

IParticleEffectInstance * MatModFloatEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(modList)
			delete modList;
		modList = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return modList;
	}
	return NULL;
}

void MatModFloatEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(modList)
			delete modList;
		modList = (IParticleEffect*)target;
		modList->parentEffect[inputID] = this;
	}
};

IModifier * MatModFloatEffect::UpdateMatMod(IMaterial * mat, IModifier * inMod, SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(mat)
	{
		if(material == NULL || mat == material)
		{
			Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
			SINGLE valueValue = EncodeParam::GetFloat(value,&(p1[0]),parentSystem);
			inMod = mat->CreateModifierFloat(floatName,valueValue,inMod);
			if(modList && inMod)
				inMod->SetNextModifier(modList->UpdateMatMod(mat,inMod->GetNextModifier(),t, inputStart, numInput, instance, parentTrans));
		}
		else
		{
			if(modList)
				inMod = modList->UpdateMatMod(mat,inMod,t, inputStart, numInput, instance, parentTrans);
		}
	}
	return inMod;
}

void MatModFloatEffect::DeleteOutput()
{
	if(modList)
	{
		delete modList;
		modList = NULL;
	}
}

void MatModFloatEffect::NullOutput(IParticleEffect * target)
{
	if(modList == target)
		modList = NULL;
}

IParticleEffect * MatModFloatEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(modList)
		return modList->FindFilter(searchName);
	return NULL;
}

#endif
