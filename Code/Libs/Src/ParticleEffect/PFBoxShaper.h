#ifndef PFBOXSHAPER_H
#define PFBOXSHAPER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFBoxShaper.h                               //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct BoxShaperProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * width;
	FloatType * length;
	FloatType * height;

	BoxShaperProgramer();
	~BoxShaperProgramer();

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

BoxShaperProgramer::BoxShaperProgramer()
{
	orientation = MakeDefaultTrans();
	width = MakeDefaultRangeFloat(-2000,2000);
	length = MakeDefaultRangeFloat(-2000,2000);
	height = MakeDefaultRangeFloat(-2000,2000);
}

BoxShaperProgramer::~BoxShaperProgramer()
{
	delete orientation;
	delete width;
	delete length;
	delete height;
}

//IParticleProgramer
U32 BoxShaperProgramer::GetNumOutput()
{
	return 1;
}

const char * BoxShaperProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 BoxShaperProgramer::GetNumInput()
{
	return 1;
}

const char * BoxShaperProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 BoxShaperProgramer::GetNumFloatParams()
{
	return 3;
}

const char * BoxShaperProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Width";
	case 1:
		return "Length";
	case 2:
		return "Height";
	}
	return NULL;
}

const FloatType * BoxShaperProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return width;
	case 1:
		return length;
	case 2:
		return height;
	}
	return NULL;
}

void BoxShaperProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(width)
				delete width;
			if(param)
				width = param->CreateCopy();
			else
				width = NULL;
			break;
		}
	case 1:
		{
			if(length)
				delete length;
			if(param)
				length = param->CreateCopy();
			else
				length = NULL;
			break;
		}
	case 2:
		{
			if(height)
				delete height;
			if(param)
				height = param->CreateCopy();
			else
				height= NULL;
			break;
		}
	}
}

U32 BoxShaperProgramer::GetNumTransformParams()
{
	return 1;
}

const char * BoxShaperProgramer::GetTransformParamName(U32 index)
{
	return "Orientation";
}

const TransformType * BoxShaperProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void BoxShaperProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 BoxShaperProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(width) 
		+ EncodeParam::EncodedFloatSize(length) 
		+ EncodeParam::EncodedFloatSize(height);
}

void BoxShaperProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_BOX_SHAPER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * widthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(widthHeader ,width);
	offset +=widthHeader->size;

	EncodedFloatTypeHeader * lengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lengthHeader,length);
	offset +=lengthHeader->size;

	EncodedFloatTypeHeader * heightHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(heightHeader ,height);
	offset +=heightHeader->size;

	header->size = offset;
}

void BoxShaperProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * widthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	width = EncodeParam::DecodeFloat(widthHeader);
	offset += widthHeader->size;

	EncodedFloatTypeHeader * lengthHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	length = EncodeParam::DecodeFloat(lengthHeader);
	offset += lengthHeader->size;

	EncodedFloatTypeHeader * heightHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	height = EncodeParam::DecodeFloat(heightHeader );
	offset += heightHeader->size;
}

struct BoxShaperEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * width;
	EncodedFloatTypeHeader * length;
	EncodedFloatTypeHeader * height;

	U8 * data;

	BoxShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~BoxShaperEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void Render(float t, U32 inputStart, U32 numInput, U32 uinstance, Transform & parentTrans);
	virtual bool Update(U32 inputStart, U32 numInput, bool bShutdown, U32 uinstance);
	virtual U32 ParticlesUsed();
	virtual void FindAllocation(U32 & startPos);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void SetInstanceNumber(U32 numInstances);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

BoxShaperEffect::BoxShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;
	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	width = (EncodedFloatTypeHeader *)(data+offset);
	offset += width->size;

	length = (EncodedFloatTypeHeader *)(data+offset);
	offset += length->size;

	height = (EncodedFloatTypeHeader *)(data+offset);
	offset += height->size;
}

BoxShaperEffect::~BoxShaperEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * BoxShaperEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void BoxShaperEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void BoxShaperEffect::Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool BoxShaperEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
{
	Transform trans;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	if(EncodeParam::FindObjectTransform(parentSystem,p2,0.0,orientation,trans,instance))
	{
		for (U16 i = 0; i < numInput; i++)
		{
			if(p2[i].bLive && p2[i].bNew)
			{
				trans.make_orthogonal();
				SINGLE lenPos = EncodeParam::GetFloat(length,&(p2[i]),parentSystem)*FEET_TO_WORLD;
				SINGLE widthPos = EncodeParam::GetFloat(width,&(p2[i]),parentSystem)*FEET_TO_WORLD;
				SINGLE heightPos = EncodeParam::GetFloat(height,&(p2[i]),parentSystem)*FEET_TO_WORLD;

				Vector offset(widthPos,lenPos,heightPos);

				p2[i].pos = p2[i].pos+trans.rotate_translate(offset);
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 BoxShaperEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void BoxShaperEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void BoxShaperEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void BoxShaperEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * BoxShaperEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void BoxShaperEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool BoxShaperEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
