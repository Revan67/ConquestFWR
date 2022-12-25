#ifndef PFCYLINDERSHAPER_H
#define PFCYLINDERSHAPER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFCylinderShaper.h                               //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct CylinderShaperProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * height;
	FloatType * topRad;
	FloatType * bottomRad;
	FloatType * angle;

	AxisEnum primaryAxis;

	CylinderShaperProgramer();
	~CylinderShaperProgramer();

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

CylinderShaperProgramer::CylinderShaperProgramer()
{
	orientation = MakeDefaultTrans();
	height = MakeDefaultRangeFloat(-2000,2000);
	topRad = MakeDefaultRangeFloat(0,2000);
	bottomRad = MakeDefaultRangeFloat(0,2000);
	angle = MakeDefaultRangeFloat(0,360);
	primaryAxis = AXIS_I;
}

CylinderShaperProgramer::~CylinderShaperProgramer()
{
	delete orientation;
	delete height;
	delete topRad;
	delete bottomRad;
	delete angle;
}

//IParticleProgramer
U32 CylinderShaperProgramer::GetNumOutput()
{
	return 1;
}

const char * CylinderShaperProgramer::GetOutputName(U32 index)
{
	return "Default Output";
}

U32 CylinderShaperProgramer::GetNumInput()
{
	return 1;
}

const char * CylinderShaperProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 CylinderShaperProgramer::GetNumFloatParams()
{
	return 4;
}

const char * CylinderShaperProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Height";
	case 1:
		return "Top Radius";
	case 2:
		return "Bottom Radius";
	case 3:
		return "Slice Angle";
	}
	return NULL;
}

const FloatType * CylinderShaperProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return height;
	case 1:
		return topRad;
	case 2:
		return bottomRad;
	case 3:
		return angle;
	}
	return NULL;
}

void CylinderShaperProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(height)
				delete height;
			if(param)
				height = param->CreateCopy();
			else
				height = NULL;
			break;
		}
	case 1:
		{
			if(topRad)
				delete topRad;
			if(param)
				topRad = param->CreateCopy();
			else
				topRad = NULL;
			break;
		}
	case 2:
		{
			if(bottomRad)
				delete bottomRad;
			if(param)
				bottomRad = param->CreateCopy();
			else
				bottomRad = NULL;
			break;
		}
	case 3:
		{
			if(angle)
				delete angle;
			if(param)
				angle = param->CreateCopy();
			else
				angle = NULL;
			break;
		}
	}
}

U32 CylinderShaperProgramer::GetNumTransformParams()
{
	return 1;
}

const char * CylinderShaperProgramer::GetTransformParamName(U32 index)
{
	return "Orientation";
}

const TransformType * CylinderShaperProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void CylinderShaperProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 CylinderShaperProgramer::GetNumEnumParams()
{
	return 1;
}

const char * CylinderShaperProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Primary Axis";
	}
	return NULL;
}

U32 CylinderShaperProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 3;
	}
	return 0;
}

const char * CylinderShaperProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "I";
			case 1:
				return "J";
			case 2:
				return "K";
			}
		}
		break;
	}
	return NULL;
}

U32 CylinderShaperProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return primaryAxis;
	}
	return 0;
}

void CylinderShaperProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		primaryAxis = (AxisEnum)value;
		break;
	}
}

U32 CylinderShaperProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(height) 
		+ EncodeParam::EncodedFloatSize(topRad) 
		+ EncodeParam::EncodedFloatSize(bottomRad) 
		+ EncodeParam::EncodedFloatSize(angle) 
		+ sizeof(U32);
}

void CylinderShaperProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_CYLINDER_SHAPER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * heightHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(heightHeader,height);
	offset +=heightHeader->size;

	EncodedFloatTypeHeader * topRadHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(topRadHeader ,topRad);
	offset +=topRadHeader->size;

	EncodedFloatTypeHeader * bottomRadHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(bottomRadHeader ,bottomRad);
	offset +=bottomRadHeader->size;

	EncodedFloatTypeHeader * angleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(angleHeader ,angle);
	offset +=angleHeader->size;

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = primaryAxis;
	offset += sizeof(U32);

	header->size = offset;
}

void CylinderShaperProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * heightHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	height = EncodeParam::DecodeFloat(heightHeader);
	offset += heightHeader->size;

	EncodedFloatTypeHeader * topRadHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	topRad = EncodeParam::DecodeFloat(topRadHeader );
	offset += topRadHeader ->size;

	EncodedFloatTypeHeader * bottomRadHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	bottomRad = EncodeParam::DecodeFloat(bottomRadHeader);
	offset += bottomRadHeader->size;

	EncodedFloatTypeHeader * angleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	angle = EncodeParam::DecodeFloat(angleHeader);
	offset += angleHeader->size;

	primaryAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);
}

struct CylinderShaperEffect : public IParticleEffect
{
	IParticleEffect * output;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * height;
	EncodedFloatTypeHeader * topRad;
	EncodedFloatTypeHeader * bottomRad;
	EncodedFloatTypeHeader * angle;
	AxisEnum primaryAxis;

	U8 * data;

	CylinderShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~CylinderShaperEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

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
};

CylinderShaperEffect::CylinderShaperEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	output = NULL;

	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	height = (EncodedFloatTypeHeader *)(data+offset);
	offset += height->size;

	topRad = (EncodedFloatTypeHeader *)(data+offset);
	offset += topRad->size;

	bottomRad = (EncodedFloatTypeHeader *)(data+offset);
	offset += bottomRad->size;

	angle = (EncodedFloatTypeHeader *)(data+offset);
	offset += angle->size;

	primaryAxis= *((AxisEnum *)(data+offset));
}

CylinderShaperEffect::~CylinderShaperEffect()
{
	if(output)
		delete output;
}

IParticleEffectInstance * CylinderShaperEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void CylinderShaperEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(output)
			delete output;
		output = (IParticleEffect*)target;
		output->parentEffect[inputID] = this;
	}
};

void CylinderShaperEffect::Render(float t, U32 inputStart, U32 numInput, U32 instance, Transform & parentTrans)
{
	if(output)
		output->Render(t,inputStart,numInput,instance,parentTrans);
}

bool CylinderShaperEffect::Update(U32 inputStart, U32 numInput, bool bShutdown, U32 instance)
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
				SINGLE heightPos = EncodeParam::GetFloat(height,&(p2[i]),parentSystem)*FEET_TO_WORLD;
				SINGLE heightMax = EncodeParam::GetMaxFloat(height,parentSystem)*FEET_TO_WORLD;
				SINGLE heightMin = EncodeParam::GetMinFloat(height,parentSystem)*FEET_TO_WORLD;
				SINGLE heightDiff = (heightMax-heightMin);
				SINGLE heightT = 0;
				if(heightDiff > 0.00001)
				{
					heightT = (heightPos-heightMin)/heightDiff;
				}

				SINGLE anglePos = EncodeParam::GetFloat(angle,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;

				SINGLE vInnerRadTop= EncodeParam::GetMinFloat(topRad,parentSystem)*FEET_TO_WORLD;
				SINGLE vInnerRadBottom= EncodeParam::GetMinFloat(bottomRad,parentSystem)*FEET_TO_WORLD;
				SINGLE minRad = (heightT*(vInnerRadTop-vInnerRadBottom))+vInnerRadBottom;

				SINGLE vOutterRadTop= EncodeParam::GetMaxFloat(topRad,parentSystem)*FEET_TO_WORLD;
				SINGLE vOutterRadBottom= EncodeParam::GetMaxFloat(bottomRad,parentSystem)*FEET_TO_WORLD;
				SINGLE maxRad = (heightT*(vOutterRadTop-vOutterRadBottom))+vOutterRadBottom;
				SINGLE rad = getRandRange(minRad,maxRad);
				Vector offset;
				if(primaryAxis == 0)//I axis
				{
					offset.x = heightPos;
					offset.y = sin(anglePos)*rad;
					offset.z = cos(anglePos)*rad;
				}
				else if(primaryAxis == 1)//J axis
				{
					offset.x = cos(anglePos)*rad;
					offset.y = heightPos;
					offset.z = sin(anglePos)*rad;
				}
				else//K axis
				{
					offset.x = cos(anglePos)*rad;
					offset.y = sin(anglePos)*rad;
					offset.z = heightPos;
				}
				p2[i].pos = p2[i].pos+trans.rotate_translate(offset);
			}
		}
	}

	if(output)
		return output->Update(inputStart,numInput, bShutdown,instance);
	return true;
}

U32 CylinderShaperEffect::ParticlesUsed()
{
	if(output)
		return output->ParticlesUsed();
	return 0;
}

void CylinderShaperEffect::FindAllocation(U32 & startPos)
{
	if(output)
		output->FindAllocation(startPos);
}

void CylinderShaperEffect::DeleteOutput()
{
	if(output)
	{
		delete output;
		output = NULL;
	}
}

void CylinderShaperEffect::NullOutput(IParticleEffect * target)
{
	if(output == target)
		output = NULL;
}

IParticleEffect * CylinderShaperEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(output)
		return output->FindFilter(searchName);
	return NULL;
}

void CylinderShaperEffect::SetInstanceNumber(U32 numInstances)
{
	if(output)
		output->SetInstanceNumber(numInstances);
}

bool CylinderShaperEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	if(output)
		return output->GetParentPosition(index,postion,lastIndex);
	return false;
}

#endif
