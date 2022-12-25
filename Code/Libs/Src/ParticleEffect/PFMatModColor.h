#ifndef PFMATMODCOLOR_H
#define PFMATMODCOLOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMatModColor.h                             //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MatModColorProgramer : public ParticleProgramer
{
	FloatType * red;
	FloatType * green;
	FloatType * blue;

	char colorName[256];
	char material[256];

	MatModColorProgramer();
	~MatModColorProgramer();

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

MatModColorProgramer::MatModColorProgramer()
{
	colorName[0] = 0;
	material[0] = 0;
	red = MakeDefaultFloat(1.0f);
	green = MakeDefaultFloat(1.0f);
	blue = MakeDefaultFloat(1.0f);
}

MatModColorProgramer::~MatModColorProgramer()
{
	delete red;
	delete green;
	delete blue;
}

//IParticleProgramer
U32 MatModColorProgramer::GetNumOutput()
{
	return 1;
}

const char * MatModColorProgramer::GetOutputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 MatModColorProgramer::GetNumInput()
{
	return 1;
}

const char * MatModColorProgramer::GetInputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 MatModColorProgramer::GetNumFloatParams()
{
	return 3;
}

const char * MatModColorProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Red";
	case 1:
		return "Green";
	case 2:
		return "Blue";
	}
	return NULL;
}

const FloatType * MatModColorProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return red;
	case 1:
		return green;
	case 2:
		return blue;
	}
	return NULL;
}

void MatModColorProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(red)
				delete red;
			if(param)
				red = param->CreateCopy();
			else
				red = NULL;
			break;
		}
	case 1:
		{
			if(green)
				delete green;
			if(param)
				green = param->CreateCopy();
			else
				green = NULL;
			break;
		}
	case 2:
		{
			if(blue)
				delete blue;
			if(param)
				blue = param->CreateCopy();
			else
				blue = NULL;
			break;
		}
	}
}

U32 MatModColorProgramer::GetNumStringParams()
{
	return 2;
}

const char * MatModColorProgramer::GetStringParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Color Name";
	case 1:
		return "Material Name";
	}
	return "Error";
}

const char * MatModColorProgramer::GetStringParam(U32 index)
{
	switch(index)
	{
	case 0:
		return colorName;
	case 1:
		return material;
	}
	return NULL;
}

void MatModColorProgramer::SetStringParam(U32 index,const char * param)
{
	switch(index)
	{
	case 0:
		strcpy(colorName,param);
		break;
	case 1:
		strcpy(material,param);
		break;
	}
}

U32 MatModColorProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) + EncodeParam::EncodedFloatSize(red) + 
		EncodeParam::EncodedFloatSize(green) 
		+ EncodeParam::EncodedFloatSize(blue)
		+ (sizeof(char)*256)
		+ (sizeof(char)*256);
}

void MatModColorProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MATMOD_COLOR;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * redHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(redHeader,red);
	offset += redHeader->size;

	EncodedFloatTypeHeader * greenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(greenHeader ,green);
	offset += greenHeader->size;

	EncodedFloatTypeHeader * blueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(blueHeader ,blue);
	offset += blueHeader->size;

	char * colorHeader = (char *)(buffer+offset);
	strcpy(colorHeader,colorName);
	offset += sizeof(char)*256;

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,material);
	offset += sizeof(char)*256;

	header->size = offset;
}

void MatModColorProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * redHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	red = 	EncodeParam::DecodeFloat(redHeader);
	offset += redHeader->size;

	EncodedFloatTypeHeader * greenHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	green = EncodeParam::DecodeFloat(greenHeader);
	offset += greenHeader->size;

	EncodedFloatTypeHeader * blueHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	blue = EncodeParam::DecodeFloat(blueHeader);
	offset += blueHeader->size;

	char * colorHeader = (char *)(buffer+offset);
	strcpy(colorName,colorHeader);
	offset += sizeof(char)*256;

	char * matHeader = (char *)(buffer+offset);
	strcpy(material,matHeader);
	offset += sizeof(char)*256;
}


struct MatModColorEffect : public IParticleEffect
{
	IParticleEffect * modList;

	char effectName[32];

	U8 * data;
	EncodedFloatTypeHeader * red;
	EncodedFloatTypeHeader * green;
	EncodedFloatTypeHeader * blue;
	char * colorName;
	IMaterial * material;

	MatModColorEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MatModColorEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual IModifier * UpdateMatMod(IMaterial * mat, IModifier * inMod, SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
};

MatModColorEffect::MatModColorEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	modList = NULL;
	material = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	red = (EncodedFloatTypeHeader *)(data+offset);
	offset += red->size;

	green = (EncodedFloatTypeHeader *)(data+offset);
	offset += green->size;

	blue = (EncodedFloatTypeHeader *)(data+offset);
	offset += blue->size;

	colorName = (char*)(data+offset);
	offset += sizeof(char)*256;

	char * matName = (char*)(data+offset);
	offset += sizeof(char)*256;

	if(matName[0])
		material = parentSystem->GetOwner()->GetMaterialManager()->FindMaterial(matName);
	else
		material = NULL;
}

MatModColorEffect::~MatModColorEffect()
{
	if(modList)
		delete modList;
}

IParticleEffectInstance * MatModColorEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void MatModColorEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(modList)
			delete modList;
		modList = (IParticleEffect*)target;
		modList->parentEffect[inputID] = this;
	}
};

IModifier * MatModColorEffect::UpdateMatMod(IMaterial * mat, IModifier * inMod, SINGLE t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(mat)
	{
		if(material == NULL || mat == material)
		{
			Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
			SINGLE redValue = EncodeParam::GetFloat(red,&(p1[0]),parentSystem);
			SINGLE greenValue = EncodeParam::GetFloat(green,&(p1[0]),parentSystem);
			SINGLE blueValue = EncodeParam::GetFloat(blue,&(p1[0]),parentSystem);
			inMod = mat->CreateModifierColor(colorName,redValue*255.0f,greenValue*255.0f,blueValue*255.0f,inMod);
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

void MatModColorEffect::DeleteOutput()
{
	if(modList)
	{
		delete modList;
		modList = NULL;
	}
}

void MatModColorEffect::NullOutput(IParticleEffect * target)
{
	if(modList == target)
		modList = NULL;
}

IParticleEffect * MatModColorEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(modList)
		return modList->FindFilter(searchName);
	return NULL;
}

#endif
