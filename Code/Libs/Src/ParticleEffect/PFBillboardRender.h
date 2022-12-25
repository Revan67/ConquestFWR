#ifndef PFBILLBOARDRENDERER_H
#define PFBILLBOARDRENDERER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFBillboardRender.h                         //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

enum BRE_Orientation
{
	BRE_FIXED,
	BRE_SPIN,
	BRE_ALIGNED,
	BRE_PARTICLE_ALIGNED,
	BRE_PINBOARD_PARTICLE_ALIGNED,
	BRE_PINBOARD_AXIS_ALIGNED,
};

struct BillboardRenderProgramer : public ParticleProgramer
{
	char materialName[256];

	FloatType * widthScale;
	FloatType * heightScale;
	FloatType * orientationFloat;
	FloatType * redColor;
	FloatType * greenColor;
	FloatType * blueColor;
	FloatType * alphaColor;
	FloatType * offsetX;
	FloatType * offsetY;
	TransformType * orientationTrans;

	BRE_Orientation orientType;
	AxisEnum orientAxis;

	BillboardRenderProgramer();
	~BillboardRenderProgramer();

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

BillboardRenderProgramer::BillboardRenderProgramer()
{
	materialName[0] = 0;

	widthScale = MakeDefaultFloat(1000);
	heightScale = MakeDefaultFloat(1000);
	orientationFloat = MakeDefaultFloat(0);
	redColor = MakeDefaultFloat(1);
	greenColor = MakeDefaultFloat(1);
	blueColor = MakeDefaultFloat(1);
	alphaColor = MakeDefaultFloat(1);
	offsetX = MakeDefaultFloat(0);
	offsetY = MakeDefaultFloat(0);

	orientationTrans = MakeDefaultTrans();

	orientType = BRE_SPIN;
	orientAxis = AXIS_I;
}

BillboardRenderProgramer::~BillboardRenderProgramer()
{
	delete widthScale;
	delete heightScale;
	delete orientationFloat;
	delete redColor;
	delete greenColor;
	delete blueColor;
	delete alphaColor;
	delete offsetX;
	delete offsetY;
	delete orientationTrans;
}

//IParticleProgramer
U32 BillboardRenderProgramer::GetNumOutput()
{
	return 1;
}

const char * BillboardRenderProgramer::GetOutputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 BillboardRenderProgramer::GetNumInput()
{
	return 1;
}

const char * BillboardRenderProgramer::GetInputName(U32 index)
{
	return FI_POINT_LIST;
}

U32 BillboardRenderProgramer::GetNumFloatParams()
{
	return 9;
}

const char * BillboardRenderProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Width Scale";
	case 1:
		return "Height Scale";
	case 2:
		return "Orientation Parameter";
	case 3:
		return "Red";
	case 4:
		return "Green";
	case 5:
		return "Blue";
	case 6:
		return "Alpha";
	case 7:
		return "OffsetX";
	case 8:
		return "OffsetY";
	}
	return NULL;
}

const FloatType * BillboardRenderProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return widthScale;
	case 1:
		return heightScale;
	case 2:
		return orientationFloat;
	case 3:
		return redColor;
	case 4:
		return greenColor;
	case 5:
		return blueColor;
	case 6:
		return alphaColor;
	case 7:
		return offsetX;
	case 8:
		return offsetY;
	}
	return NULL;
}

void BillboardRenderProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(widthScale)
				delete widthScale;
			if(param)
				widthScale = param->CreateCopy();
			else
				widthScale = NULL;
			break;
		}
	case 1:
		{
			if(heightScale)
				delete heightScale;
			if(param)
				heightScale = param->CreateCopy();
			else
				heightScale = NULL;
			break;
		}
	case 2:
		{
			if(orientationFloat)
				delete orientationFloat;
			if(param)
				orientationFloat = param->CreateCopy();
			else
				orientationFloat = NULL;
			break;
		}
	case 3:
		{
			if(redColor)
				delete redColor;
			if(param)
				redColor = param->CreateCopy();
			else
				redColor = NULL;
			break;
		}
	case 4:
		{
			if(greenColor)
				delete greenColor;
			if(param)
				greenColor = param->CreateCopy();
			else
				greenColor = NULL;
			break;
		}
	case 5:
		{
			if(blueColor)
				delete blueColor;
			if(param)
				blueColor = param->CreateCopy();
			else
				blueColor = NULL;
			break;
		}
	case 6:
		{
			if(alphaColor)
				delete alphaColor;
			if(param)
				alphaColor = param->CreateCopy();
			else
				alphaColor = NULL;
			break;
		}
	case 7:
		{
			if(offsetX)
				delete offsetX;
			if(param)
				offsetX = param->CreateCopy();
			else
				offsetX = NULL;
			break;
		}
	case 8:
		{
			if(offsetY)
				delete offsetY;
			if(param)
				offsetY = param->CreateCopy();
			else
				offsetY = NULL;
			break;
		}
	}
}

U32 BillboardRenderProgramer::GetNumTransformParams()
{
	return 1;
}

const char * BillboardRenderProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * BillboardRenderProgramer::GetTransformParam(U32 index)
{
	return orientationTrans;
}

void BillboardRenderProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientationTrans)
		delete orientationTrans;
	if(param)
		orientationTrans = param->CreateCopy();
	else
		orientationTrans = NULL;
}

U32 BillboardRenderProgramer::GetNumStringParams()
{
	return 1;
}

const char * BillboardRenderProgramer::GetStringParamName(U32 index)
{
	return "Material Name";
}

const char * BillboardRenderProgramer::GetStringParam(U32 index)
{
	return materialName;
}

void BillboardRenderProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(materialName,param);
}

U32 BillboardRenderProgramer::GetNumEnumParams()
{
	return 2;
}

const char * BillboardRenderProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Orientation Type";
	case 1:
		return "Orientation Axis";
	}
	return NULL;
}

U32 BillboardRenderProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 6;
	case 1:
		return 3;
	}
	return 0;
}

const char * BillboardRenderProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Fixed";
			case 1:
				return "Spin";
			case 2:
				return "Aligned";
			case 3:
				return "Particle Aligned";
			case 4:
				return "Pinboard Particle Aligned";
			case 5:
				return "Pinboard";
			}
		}
		break;
	case 1:
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

U32 BillboardRenderProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return orientType;
	case 1:
		return orientAxis;
	}
	return 0;
}

void BillboardRenderProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		orientType = (BRE_Orientation)value;
		break;
	case 1:
		orientAxis = (AxisEnum)value;
		break;
	}
}

U32 BillboardRenderProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(widthScale) 
		+ EncodeParam::EncodedFloatSize(heightScale) 
		+ EncodeParam::EncodedFloatSize(orientationFloat) 
		+ EncodeParam::EncodedFloatSize(redColor) 
		+ EncodeParam::EncodedFloatSize(greenColor) 
		+ EncodeParam::EncodedFloatSize(blueColor) 
		+ EncodeParam::EncodedFloatSize(alphaColor) 
		+ EncodeParam::EncodedFloatSize(offsetX) 
		+ EncodeParam::EncodedFloatSize(offsetY) 
		+ EncodeParam::EncodedTransformSize(orientationTrans)
		+ sizeof(U32)
		+ sizeof(U32)
		+ (sizeof(char)*256);
}

void BillboardRenderProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_RENDER_BILBOARD;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * widthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(widthScaleHeader,widthScale);
	offset +=widthScaleHeader->size;

	EncodedFloatTypeHeader * heightScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(heightScaleHeader ,heightScale);
	offset +=heightScaleHeader->size;

	EncodedFloatTypeHeader * orientationFloatHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(orientationFloatHeader ,orientationFloat);
	offset +=orientationFloatHeader->size;

	EncodedFloatTypeHeader * redColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(redColorHeader ,redColor);
	offset +=redColorHeader->size;

	EncodedFloatTypeHeader * greenColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(greenColorHeader ,greenColor);
	offset +=greenColorHeader->size;

	EncodedFloatTypeHeader * blueColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(blueColorHeader ,blueColor);
	offset +=blueColorHeader->size;

	EncodedFloatTypeHeader * alphaColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(alphaColorHeader ,alphaColor);
	offset +=alphaColorHeader->size;

	EncodedFloatTypeHeader * offsetXHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(offsetXHeader ,offsetX);
	offset +=offsetXHeader->size;

	EncodedFloatTypeHeader * offsetYHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(offsetYHeader ,offsetY);
	offset +=offsetYHeader->size;

	EncodedTransformTypeHeader * orientationTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationTransHeader ,orientationTrans);
	offset +=orientationTransHeader->size;

	U32 * oTypeHeader = (U32*)(buffer+offset);
	(*oTypeHeader) = orientType;
	offset += sizeof(U32);

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = orientAxis;
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,materialName);
	offset += sizeof(char)*256;

	header->size = offset;
}

void BillboardRenderProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * widthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	widthScale = EncodeParam::DecodeFloat(widthScaleHeader);
	offset += widthScaleHeader->size;

	EncodedFloatTypeHeader * heightScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	heightScale = EncodeParam::DecodeFloat(heightScaleHeader );
	offset += heightScaleHeader ->size;

	EncodedFloatTypeHeader * orientationFloatHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	orientationFloat = EncodeParam::DecodeFloat(orientationFloatHeader);
	offset += orientationFloatHeader->size;

	EncodedFloatTypeHeader * redColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	redColor = EncodeParam::DecodeFloat(redColorHeader);
	offset += redColorHeader->size;

	EncodedFloatTypeHeader * greenColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	greenColor = EncodeParam::DecodeFloat(greenColorHeader);
	offset += greenColorHeader->size;

	EncodedFloatTypeHeader * blueColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	blueColor = EncodeParam::DecodeFloat(blueColorHeader);
	offset += blueColorHeader->size;

	EncodedFloatTypeHeader * alphaColorHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	alphaColor = EncodeParam::DecodeFloat(alphaColorHeader);
	offset += alphaColorHeader->size;

	EncodedFloatTypeHeader * offsetXHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	offsetX = EncodeParam::DecodeFloat(offsetXHeader);
	offset += offsetXHeader->size;

	EncodedFloatTypeHeader * offsetYHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	offsetY = EncodeParam::DecodeFloat(offsetYHeader);
	offset += offsetYHeader->size;

	EncodedTransformTypeHeader * orientationTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientationTrans = EncodeParam::DecodeTransform(orientationTransHeader);
	offset += orientationTransHeader->size;

	orientType = *((BRE_Orientation*)(buffer+offset));
	offset += sizeof(U32);

	orientAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(materialName,matHeader);
}

struct BillboardRenderEffect : public IParticleEffect
{
	IMaterial * mat;
	IParticleEffect * modEffect;
	IModifier * modList;

	char effectName[32];

	EncodedFloatTypeHeader * widthScale;
	EncodedFloatTypeHeader * heightScale;
	EncodedFloatTypeHeader * orientationFloat;
	EncodedFloatTypeHeader * redColor;
	EncodedFloatTypeHeader * greenColor;
	EncodedFloatTypeHeader * blueColor;
	EncodedFloatTypeHeader * alphaColor;
	EncodedFloatTypeHeader * offsetX;
	EncodedFloatTypeHeader * offsetY;
	EncodedTransformTypeHeader * orientationTrans;

	BRE_Orientation orientType;
	AxisEnum orientAxis;

	U8 * data;

	U32 numInst;

	SINGLE interp;

	IMeshInstance ** myMeshes; 

	BillboardRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~BillboardRenderEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);

	virtual bool UpdateMesh(U32 inputStart, U32 numInput, bool bShutdown,U32 instance,Transform & parentTrans);

	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);

	virtual void CalcVertColor(	U32 i, 
								Particle * p1, 
								Particle * p2, 
								U8 & red,
								U8 & green,
								U8 & blue,
								U8 & alpha);

	virtual void CalcVertA(	U32 i, 
							Particle * p1, 
							Particle * p2, 
							IMeshInstance * mesh,
							const Vector& xBB,
							const Vector& yBB,
							const Vector & normal,
							Transform & parentTrans);

	virtual void CalcVertB(	U32 i, 
							Particle * p1, 
							Particle * p2, 
							IMeshInstance * mesh,
							const Vector& xBB,
							const Vector& yBB,
							const Vector & normal,
							float angle,
							Transform & parentTrans);

	virtual void FindAllocation(U32 & startPos);
	virtual void SetInstanceNumber(U32 numInstance);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

void BillboardRenderEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
}

bool BillboardRenderEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	return false;
}

void BillboardRenderEffect::FindAllocation(U32 & startPos)
{
	U32 OutputRange = parentEffect[0]->OutputRange();
	myMeshes = new IMeshInstance * [numInst];

	for(U32 i = 0; i < numInst; ++i)
	{
		myMeshes[i] = parentSystem->GetOwner()->GetMeshManager()->CreateDynamicMesh(OutputRange * 6,mat);
	}
}

BillboardRenderEffect::BillboardRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID)  : IParticleEffect(_parent,_parentEffect,1, inputID)
{
	modList = NULL;
	modEffect = NULL;
	numInst = 1;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = buffer;
	U32 offset = sizeof(ParticleHeader);
	widthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += widthScale->size;
	heightScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += heightScale->size;
	orientationFloat = (EncodedFloatTypeHeader *)(data+offset);
	offset += orientationFloat->size;
	redColor = (EncodedFloatTypeHeader *)(data+offset);
	offset += redColor->size;
	greenColor = (EncodedFloatTypeHeader *)(data+offset);
	offset += greenColor->size;
	blueColor = (EncodedFloatTypeHeader *)(data+offset);
	offset += blueColor->size;
	alphaColor = (EncodedFloatTypeHeader *)(data+offset);
	offset += alphaColor->size;

	offsetX = (EncodedFloatTypeHeader *)(data+offset);
	offset += offsetX->size;
	offsetY = (EncodedFloatTypeHeader *)(data+offset);
	offset += offsetY->size;

	orientationTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientationTrans->size;
	orientType= *((BRE_Orientation*)(data+offset));
	offset += sizeof(U32);
	orientAxis= *((AxisEnum*)(data+offset));
	offset += sizeof(U32);

	char * matHeader = (char *)(data+offset);

	mat = parentSystem->GetOwner()->GetMaterialManager()->FindMaterial(matHeader);
}

BillboardRenderEffect::~BillboardRenderEffect()
{
	if (myMeshes)
	{
		for (U32 i = 0; i < numInst; i++)
		{
			parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(myMeshes[i]);
		}
		delete [] myMeshes;
	}
	if(modList)
	{
		modList->Release();
		modList = NULL;
	}
	if(mat)
	{
//not currently releasing properly.....
//		parentSystem->GetOwner()->GetMaterialManager()-(mat);
		mat = NULL;
	}
}

IParticleEffectInstance * BillboardRenderEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(modEffect)
			delete modEffect;
		modEffect = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return modEffect;
	}
	return NULL;
}

void BillboardRenderEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(modEffect)
			delete modEffect;
		modEffect = (IParticleEffect*)target;
		modEffect->parentEffect[inputID] = this;
	}
};

void BillboardRenderEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	interp = t;
	if(modEffect)
	{
		modList = modEffect->UpdateMatMod(mat,modList,t, inputStart, numInput,instance, parentTrans);
	}
	UpdateMesh(inputStart, numInput, false, instance,parentTrans);
	if(myMeshes[instance])
	{
		myMeshes[instance]->SetModifierList(modList);
		myMeshes[instance]->Update(parentSystem->GetRenderTime());
		myMeshes[instance]->Render();
	}
}

void BillboardRenderEffect::CalcVertColor(U32 i, 
										 Particle * p1, 
										 Particle * p2,  
										U8 & red,
										U8 & green,
										U8 & blue,
										U8 & alpha)
{
	SINGLE r1 = EncodeParam::GetFloat(redColor,&(p1[i]),parentSystem);
	SINGLE g1 = EncodeParam::GetFloat(greenColor,&(p1[i]),parentSystem);
	SINGLE b1 = EncodeParam::GetFloat(blueColor,&(p1[i]),parentSystem);
	SINGLE a1 = EncodeParam::GetFloat(alphaColor,&(p1[i]),parentSystem);

	SINGLE r2 = EncodeParam::GetFloat(redColor,&(p2[i]),parentSystem);
	SINGLE g2 = EncodeParam::GetFloat(greenColor,&(p2[i]),parentSystem);
	SINGLE b2 = EncodeParam::GetFloat(blueColor,&(p2[i]),parentSystem);
	SINGLE a2 = EncodeParam::GetFloat(alphaColor,&(p2[i]),parentSystem);

	SINGLE r = r1 + interp*(r2-r1);
	SINGLE g = g1 + interp*(g2-g1);
	SINGLE b = b1 + interp*(b2-b1);
	SINGLE a = a1 + interp*(a2-a1);

	red = (U8)(r*255);
	green = (U8)(g*255);
	blue = (U8)(b*255);
	alpha = (U8)(a*255);
}

void BillboardRenderEffect::CalcVertA(	U32 i, 
										Particle * p1, 
										Particle * p2, 
										IMeshInstance * mesh,
										const Vector& xBB,
										const Vector& yBB,
										const Vector& normal,
										Transform & parentTrans)
{
	U8 red,green,blue,alpha;
	CalcVertColor(i,p1,p2,red,green,blue,alpha);

	float offX1 = EncodeParam::GetFloat(offsetX,&(p1[i]),parentSystem)*FEET_TO_WORLD;
	float offY1 = EncodeParam::GetFloat(offsetY,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float offX2 = EncodeParam::GetFloat(offsetX,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	float offY2 = EncodeParam::GetFloat(offsetY,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	
	float offX = offX1 + interp*(offX2-offX1);	
	float offY = offY1 + interp*(offY2-offY1);	

	float scaleX1 = EncodeParam::GetFloat(widthScale,&(p1[i]),parentSystem)*FEET_TO_WORLD;
	float scaleY1 = EncodeParam::GetFloat(heightScale,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float scaleX2 = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	float scaleY2 = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem)*FEET_TO_WORLD;

	float scaleX = scaleX1 + interp*(scaleX2-scaleX1);	
	float scaleY = scaleY1 + interp*(scaleY2-scaleY1);	
	
	SINGLE pivx = scaleX/2;
	SINGLE pivy = scaleY/2;
	
	Vector epos = (interp*(p2[i].pos-p1[i].pos))+p1[i].pos +xBB*offX+yBB*offY;
	epos = parentTrans.rotate_translate(epos);

	Vector finalPos[4];
	finalPos[0] = epos+xBB*(0-pivx)-yBB*(0-pivy);
	finalPos[1] =(epos+xBB*(0-pivx)-yBB*(scaleY-pivy));
	finalPos[2] =(epos+xBB*(scaleX-pivx)-yBB*(scaleY-pivy));
	finalPos[3] =(epos+xBB*(scaleX-pivx)-yBB*(0-pivy));

	U32 columns = mat->GetAnimUVColumns();
	SINGLE width = 1.0f/columns;
	SINGLE texU = (p1[i].birthTime%columns)*width;

	U32 rows = mat->GetAnimUVRows();
	SINGLE height = 1.0f/rows;

	mesh->DynDef_SetTex1(texU,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[0]);

	mesh->DynDef_SetTex1(texU+width,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[1]);

	mesh->DynDef_SetTex1(texU+width,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[2]);

	mesh->DynDef_SetTex1(texU,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[0]);

	mesh->DynDef_SetTex1(texU+width,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[2]);

	mesh->DynDef_SetTex1(texU,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[3]);
}

void BillboardRenderEffect::CalcVertB(	U32 i, 
										Particle * p1, 
										Particle * p2, 
										IMeshInstance * mesh,
										const Vector& xBB,
										const Vector& yBB,
										const Vector & normal,
										float angle,
										Transform & parentTrans)
{
	U8 red,green,blue,alpha;
	CalcVertColor(i,p1,p2,red,green,blue,alpha);

	SINGLE cosA = cos(angle);//slow
	SINGLE sinA = sin(angle);

	float offX1 = EncodeParam::GetFloat(offsetX,&(p1[i]),parentSystem)*FEET_TO_WORLD;
	float offY1 = EncodeParam::GetFloat(offsetY,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float offX2 = EncodeParam::GetFloat(offsetX,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	float offY2 = EncodeParam::GetFloat(offsetY,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	
	float offX = offX1 + interp*(offX2-offX1);	
	float offY = offY1 + interp*(offY2-offY1);	

	float scaleX1 = EncodeParam::GetFloat(widthScale,&(p1[i]),parentSystem)*FEET_TO_WORLD;
	float scaleY1 = EncodeParam::GetFloat(heightScale,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float scaleX2 = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	float scaleY2 = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem)*FEET_TO_WORLD;

	float scaleX = scaleX1 + interp*(scaleX2-scaleX1);	
	float scaleY = scaleY1 + interp*(scaleY2-scaleY1);	

	SINGLE pivx = scaleX/2;
	SINGLE pivy = scaleY/2;

	Vector epos = (interp*(p2[i].pos - p1[i].pos))+p1[i].pos + xBB*((offX)*cosA-(offY)*sinA)-yBB*((offX)*sinA+(offY)*cosA);
	epos = parentTrans.rotate_translate(epos);

	Vector finalPos[4];
	finalPos[0] = (epos+xBB*((0-pivx)*cosA-(0-pivy)*sinA)-yBB*((0-pivx)*sinA+(0-pivy)*cosA));
	finalPos[1] =(epos+xBB*((0-pivx)*cosA-(scaleY-pivy)*sinA)-yBB*((0-pivx)*sinA+(scaleY-pivy)*cosA));
	finalPos[2] =(epos+xBB*((scaleX-pivx)*cosA-(scaleY-pivy)*sinA)-yBB*((scaleX-pivx)*sinA+(scaleY-pivy)*cosA));
	finalPos[3] =(epos+xBB*((scaleX-pivx)*cosA-(0-pivy)*sinA)-yBB*((scaleX-pivx)*sinA+(0-pivy)*cosA));

	U32 columns = mat->GetAnimUVColumns();
	SINGLE width = 1.0f/columns;
	SINGLE texU = (p1[i].birthTime%columns)*width;

	U32 rows = mat->GetAnimUVRows();
	SINGLE height = 1.0f/rows;

	mesh->DynDef_SetTex1(texU,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[0]);

	mesh->DynDef_SetTex1(texU+width,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[1]);

	mesh->DynDef_SetTex1(texU+width,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[2]);

	mesh->DynDef_SetTex1(texU,0);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[0]);

	mesh->DynDef_SetTex1(texU+width,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[2]);

	mesh->DynDef_SetTex1(texU,height);
	mesh->DynDef_SetNormal(normal);
	mesh->DynDef_SetColor(red,green,blue,alpha);
	mesh->DynDef_SetPos(finalPos[3]);
}


bool BillboardRenderEffect::UpdateMesh(U32 inputStart, U32 numInput, bool bShutdown,U32 instance,Transform & parentTrans)
{
	IMeshInstance * mesh = myMeshes[instance];
	if(!mesh)
		return true;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	
	float t = 0;
	switch(orientType)
	{
	case BRE_ALIGNED:
		{
			Vector camDir;
			Transform trans;
			Vector upVect;
			if(EncodeParam::FindObjectTransform(parentSystem,p2,t,orientationTrans,trans,instance))
			{
				trans.make_orthogonal();
				if(orientAxis == AXIS_I)
				{
					camDir = trans.get_i();
					upVect = trans.get_j();
				}
				else if(orientAxis == AXIS_J)
				{
					camDir = trans.get_j();
					upVect = trans.get_k();
				}
				else
				{
					camDir = trans.get_k();
					upVect = trans.get_i();
				}
			}
			else
			{
				camDir = parentSystem->GetOwner()->GetCamera()->get_position() - parentSystem->GetOwner()->GetCamera()->get_look_pos();
				camDir.fast_normalize();
				if(camDir.x == 0 && camDir.y == 0 && camDir.z == 1)
				{
					upVect = Vector(0,1.0,0);
				}
				else
				{
					upVect = Vector(0,0,1.0);
				}
			}
			Vector xBB = cross_product(camDir, upVect);
			Vector yBB = cross_product(camDir, xBB);
			
			xBB.normalize();
			yBB.normalize();

			mesh->BeginDynamicDef();


			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					CalcVertA(i,p1,p2,mesh,xBB,yBB,camDir,parentTrans);
				}
			}
			mesh->EndDynamicDef();
		}
		break;
	case BRE_FIXED:
		{
			Vector camDir;
			camDir = parentSystem->GetOwner()->GetCamera()->get_position() - parentSystem->GetOwner()->GetCamera()->get_look_pos();
			camDir.fast_normalize();

			Vector upVect;
			if(camDir.x == 0 && camDir.y == 0 && camDir.z == 1)
			{
				upVect = Vector(0,1.0,0);
			}
			else
			{
				upVect = Vector(0,0,1.0);
			}
			Vector xBB = cross_product(camDir, upVect);
			Vector yBB = cross_product(camDir, xBB);
			
			xBB.normalize();
			yBB.normalize();
			
			mesh->BeginDynamicDef();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					SINGLE angle = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					CalcVertB(i,p1,p2,mesh,xBB,yBB,camDir,angle,parentTrans);
				}
			}		
			mesh->EndDynamicDef();
		}
		break;
	case BRE_SPIN:
		{
			Vector camDir;
			camDir = parentSystem->GetOwner()->GetCamera()->get_position() - parentSystem->GetOwner()->GetCamera()->get_look_pos();
			camDir.fast_normalize();

			Vector upVect;
			if(camDir.x == 0 && camDir.y == 0 && camDir.z == 1)
			{
				upVect = Vector(0,1.0,0);
			}
			else
			{
				upVect = Vector(0,0,1.0);
			}
			Vector xBB = cross_product(camDir, upVect);
			Vector yBB = cross_product(camDir, xBB);
			
			xBB.normalize();
			yBB.normalize();
			
			mesh->BeginDynamicDef();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					SINGLE startAngle = (((SINGLE)(((p2[i].birthTime)*1234)%1000))*6.28)/1000.0;
					SINGLE rate = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					SINGLE age = (parentSystem->GetCurrentTimeMS()-p2[i].birthTime)/1000.0;
					SINGLE angle = startAngle + rate*age;
					CalcVertB(i,p1,p2,mesh,xBB,yBB,camDir,angle,parentTrans);
				}
			}		
			mesh->EndDynamicDef();
		}
		break;
	case BRE_PARTICLE_ALIGNED:
		{			
			mesh->BeginDynamicDef();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					Vector vI,vJ,vK;
					if(p2[i].vel.x == 0 && p2[i].vel.y == 0 && p2[i].vel.z == 0)
						vI = Vector(1,0,0);
					else
					{
						vI = p2[i].vel;
						vI.fast_normalize();
					}
					if(vI.x == 0 && vI.y == 0 && vI.z == 1)
					{
						vJ = Vector(0,1.0,0);
					}
					else
					{
						vJ = Vector(0,0,1.0);
					}
					vK = cross_product(vI,vJ);
					vK.fast_normalize();
					vJ = cross_product(vK,vI);
					vJ.fast_normalize();
					Vector xBB,yBB,normal;
					if(orientAxis == AXIS_I)
					{
						xBB = vJ;
						yBB = vK;
						normal = vI;
					}
					else if(orientAxis == AXIS_J)
					{
						xBB = vK;
						yBB = vI;
						normal = vJ;
					}
					else
					{
						xBB = vI;
						yBB = vJ;
						normal = vK;
					}
					CalcVertA(i,p1,p2,mesh,xBB,yBB,normal,parentTrans);
				}
			}
			mesh->EndDynamicDef();
		}
		break;
	case BRE_PINBOARD_PARTICLE_ALIGNED:
		{			
			Vector camDir;
			camDir = parentSystem->GetOwner()->GetCamera()->get_position() - parentSystem->GetOwner()->GetCamera()->get_look_pos();
			camDir.fast_normalize();
			mesh->BeginDynamicDef();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					Vector vI,vJ,vK;
					if(p2[i].vel.x == 0 && p2[i].vel.y == 0 && p2[i].vel.z == 0)
						vI = Vector(1,0,0);
					else
					{
						vI = p2[i].vel;
						vI.fast_normalize();
					}
					if(vI.x == 0 && vI.y == 0 && vI.z == 1)
					{
						vJ = Vector(0,1.0,0);
					}
					else
					{
						vJ = Vector(0,0,1.0);
					}
					vK = cross_product(vI,vJ);
					vK.fast_normalize();
					vJ = cross_product(vK,vI);
					vJ.fast_magnitude();

					Vector xBB,yBB;
					if(orientAxis == AXIS_I)
						xBB = vI;
					else if(orientAxis == AXIS_J)
						xBB = vJ;
					else
						xBB = vK;

					yBB = -cross_product(xBB,camDir);
					yBB.fast_normalize();

					CalcVertA(i,p1,p2,mesh,xBB,yBB,camDir,parentTrans);
				}
			}
			mesh->EndDynamicDef();
		}
		break;
	case BRE_PINBOARD_AXIS_ALIGNED:
		{			
			Transform trans;
			Vector xBB;
			Vector yBB;
			if(EncodeParam::FindObjectTransform(parentSystem,p2,t,orientationTrans,trans,instance))
			{
				if(orientAxis == AXIS_I)
					xBB = trans.get_i();
				else if(orientAxis == AXIS_J)
					xBB = trans.get_j();
				else
					xBB = trans.get_k();
			}
			else
			{
				xBB = Vector(0,0,1);
			}
			xBB.fast_normalize();

			Vector camDir;
			camDir = parentSystem->GetOwner()->GetCamera()->get_position() - parentSystem->GetOwner()->GetCamera()->get_look_pos();
			camDir.fast_normalize();
			yBB = cross_product(xBB,camDir);
			yBB.fast_normalize();
			mesh->BeginDynamicDef();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					CalcVertA(i,p1,p2,mesh,xBB,yBB,camDir,parentTrans);
				}
			}
			mesh->EndDynamicDef();
		}
		break;
	}
	return true;
}

IParticleEffect * BillboardRenderEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	return NULL;
}

void BillboardRenderEffect::DeleteOutput()
{
	if(modEffect)
	{
		delete modEffect;
		modEffect = NULL;
	}
}

void BillboardRenderEffect::NullOutput(IParticleEffect * target)
{
	if(modEffect == target)
		modEffect = NULL;
}



#endif
