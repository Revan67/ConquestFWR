#ifndef PFANIMBILLBOARDRENDERER_H
#define PFANIMBILLBOARDRENDERER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFAnimBillboardRender.h                     //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

#include "PFBillboardRender.h"

enum ABR_AnimType
{
	ABR_NORMAL,
	ABR_SMOOTH,
};

struct AnimBillboardRenderProgramer : public ParticleProgramer
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
	FloatType * animColums;
	FloatType * animRows;
	FloatType * fps;
	TransformType * orientationTrans;

	BRE_BlendMode blendMode;
	BRE_Orientation orientType;
	AxisEnum orientAxis;
	ABR_AnimType animType;

	AnimBillboardRenderProgramer();
	~AnimBillboardRenderProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

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

AnimBillboardRenderProgramer::AnimBillboardRenderProgramer()
{
	materialName[0] = 0;

	widthScale = MakeDefaultFloat(1);
	heightScale = MakeDefaultFloat(1);
	orientationFloat = MakeDefaultFloat(0);
	redColor = MakeDefaultFloat(1);
	greenColor = MakeDefaultFloat(1);
	blueColor = MakeDefaultFloat(1);
	alphaColor = MakeDefaultFloat(1);
	offsetX = MakeDefaultFloat(0);
	offsetY = MakeDefaultFloat(0);
	animColums = MakeDefaultFloat(1);
	animRows = MakeDefaultFloat(1);
	fps = MakeDefaultFloat(1);

	orientationTrans = MakeDefaultTrans();

	blendMode = BRE_ADDITIVE;
	orientType = BRE_SPIN;
	orientAxis = AXIS_I;
	animType = ABR_NORMAL;
}

AnimBillboardRenderProgramer::~AnimBillboardRenderProgramer()
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
	delete animColums;
	delete animRows;
	delete fps;
	delete orientationTrans;
}

//IParticleProgramer
U32 AnimBillboardRenderProgramer::GetNumOutput()
{
	return 0;
}

const char * AnimBillboardRenderProgramer::GetOutputName(U32 index)
{
	return "Error";
}

U32 AnimBillboardRenderProgramer::GetNumFloatParams()
{
	return 12;
}

const char * AnimBillboardRenderProgramer::GetFloatParamName(U32 index)
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
	case 9:
		return "Frame Columns";
	case 10:
		return "Frame Rows";
	case 11:
		return "FPS";
	}
	return NULL;
}

const FloatType * AnimBillboardRenderProgramer::GetFloatParam(U32 index)
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
	case 9:
		return animColums;
	case 10:
		return animRows;
	case 11:
		return fps;
	}
	return NULL;
}

void AnimBillboardRenderProgramer::SetFloatParam(U32 index,const FloatType * param)
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
	case 9:
		{
			if(animColums)
				delete animColums;
			if(param)
				animColums = param->CreateCopy();
			else
				animColums = NULL;
			break;
		}
	case 10:
		{
			if(animRows)
				delete animRows;
			if(param)
				animRows = param->CreateCopy();
			else
				animRows = NULL;
			break;
		}
	case 11:
		{
			if(fps)
				delete fps;
			if(param)
				fps = param->CreateCopy();
			else
				fps = NULL;
			break;
		}
	}
}

U32 AnimBillboardRenderProgramer::GetNumTransformParams()
{
	return 1;
}

const char * AnimBillboardRenderProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * AnimBillboardRenderProgramer::GetTransformParam(U32 index)
{
	return orientationTrans;
}

void AnimBillboardRenderProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientationTrans)
		delete orientationTrans;
	if(param)
		orientationTrans = param->CreateCopy();
	else
		orientationTrans = NULL;
}

U32 AnimBillboardRenderProgramer::GetNumStringParams()
{
	return 1;
}

const char * AnimBillboardRenderProgramer::GetStringParamName(U32 index)
{
	return "Material Name";
}

const char * AnimBillboardRenderProgramer::GetStringParam(U32 index)
{
	return materialName;
}

void AnimBillboardRenderProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(materialName,param);
}

U32 AnimBillboardRenderProgramer::GetNumEnumParams()
{
	return 4;
}

const char * AnimBillboardRenderProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Blend Mode";
	case 1:
		return "Orientation Type";
	case 2:
		return "Orientation Axis";
	case 3:
		return "Animation Type";
	}
	return NULL;
}

U32 AnimBillboardRenderProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	case 1:
		return 6;
	case 2:
		return 3;
	case 3:
		return 2;
	}
	return 0;
}

const char * AnimBillboardRenderProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
		{
			switch(value)
			{
			case 0:
				return "Additive";
			case 1:
				return "Mutiply";
			}
		}
		break;
	case 1:
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
	case 2:
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
	case 3:
		{
			switch(value)
			{
			case 0:
				return "Normal";
			case 1:
				return "Smooth";
			}
		}
		break;
	}
	return NULL;
}

U32 AnimBillboardRenderProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return blendMode;
	case 1:
		return orientType;
	case 2:
		return orientAxis;
	case 3:
		return animType;
	}
	return 0;
}

void AnimBillboardRenderProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		blendMode = (BRE_BlendMode)value;
		break;
	case 1:
		orientType = (BRE_Orientation)value;
		break;
	case 2:
		orientAxis = (AxisEnum)value;
		break;
	case 3:
		animType = (ABR_AnimType)value;
		break;
	}
}

U32 AnimBillboardRenderProgramer::GetDataChunkSize()
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
		+ EncodeParam::EncodedFloatSize(animColums) 
		+ EncodeParam::EncodedFloatSize(animRows) 
		+ EncodeParam::EncodedFloatSize(fps) 
		+ EncodeParam::EncodedTransformSize(orientationTrans)
		+ sizeof(U32)
		+ sizeof(U32)
		+ sizeof(U32)
		+ sizeof(U32)
		+ (sizeof(char)*256);
}

void AnimBillboardRenderProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_RENDER_BILBOARD_2;
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

	EncodedFloatTypeHeader * animColumsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(animColumsHeader ,animColums);
	offset +=animColumsHeader->size;

	EncodedFloatTypeHeader * animRowsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(animRowsHeader ,animRows);
	offset +=animRowsHeader->size;

	EncodedFloatTypeHeader * fpsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(fpsHeader ,fps);
	offset +=fpsHeader->size;

	EncodedTransformTypeHeader * orientationTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationTransHeader ,orientationTrans);
	offset +=orientationTransHeader->size;

	U32 * blendHeader = (U32*)(buffer+offset);
	(*blendHeader) = blendMode;
	offset += sizeof(U32);

	U32 * oTypeHeader = (U32*)(buffer+offset);
	(*oTypeHeader) = orientType;
	offset += sizeof(U32);

	U32 * oAxisHeader = (U32*)(buffer+offset);
	(*oAxisHeader) = orientAxis;
	offset += sizeof(U32);

	U32 * animTypeHeader = (U32*)(buffer+offset);
	(*animTypeHeader) = animType;
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,materialName);
	offset += sizeof(char)*256;

	header->size = offset;
}

void AnimBillboardRenderProgramer::SetDataChunk(U8 * buffer)
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

	EncodedFloatTypeHeader * animColumsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	animColums = EncodeParam::DecodeFloat(animColumsHeader);
	offset += animColumsHeader->size;

	EncodedFloatTypeHeader * animRowsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	animRows = EncodeParam::DecodeFloat(animRowsHeader);
	offset += animRowsHeader->size;

	EncodedFloatTypeHeader * fpsHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	fps = EncodeParam::DecodeFloat(fpsHeader);
	offset += fpsHeader->size;
	
	EncodedTransformTypeHeader * orientationTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientationTrans = EncodeParam::DecodeTransform(orientationTransHeader);
	offset += orientationTransHeader->size;

	blendMode = *((BRE_BlendMode*)(buffer+offset));
	offset += sizeof(U32);

	orientType = *((BRE_Orientation*)(buffer+offset));
	offset += sizeof(U32);

	orientAxis = *((AxisEnum*)(buffer+offset));
	offset += sizeof(U32);

	animType = *((ABR_AnimType*)(buffer+offset));
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(materialName,matHeader);
}

struct AnimBillboardRenderEffect : public IParticleEffect
{
	IRenderMaterial * mat;

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
	EncodedFloatTypeHeader * animColums;
	EncodedFloatTypeHeader * animRows;
	EncodedFloatTypeHeader * fps;
	EncodedTransformTypeHeader * orientationTrans;

	BRE_BlendMode blendMode;
	BRE_Orientation orientType;
	AxisEnum orientAxis;
	ABR_AnimType animType;

	U8 * data;

	U32 numInst;

	SINGLE interp;

	SINGLE framePerSecond;
	U32 frameCount;
	U32 numRows;
	U32 numColumns;

	IRenderMesh ** myMeshes; 

	AnimBillboardRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~AnimBillboardRenderEffect();
	//IParticleEffectInstance
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);

	virtual bool UpdateMesh(U32 inputStart, U32 numInput, bool bShutdown,U32 instance, Transform & parentTrans);

	virtual IParticleEffect * FindFilter(const char * searchName);

	virtual void CalcVertColor(	U32 i, 
								Particle * p1, 
								Particle * p2, 
								FVFColoredTexturedVertex* verts);

	virtual void CalcVertA(	U32 i, 
							Particle * p1, 
							Particle * p2, 
							FVFColoredTexturedVertex* verts,
							const Vector& xBB,
							const Vector& yBB,
							Transform & parentTrans);

	virtual void CalcVertB(	U32 i, 
							Particle * p1, 
							Particle * p2, 
							FVFColoredTexturedVertex* verts,
							const Vector& xBB,
							const Vector& yBB,
							float angle,
							Transform & parentTrans);

	virtual void HideVert(	U32 i, 
							Particle * p1, 
							Particle * p2, 
							FVFColoredTexturedVertex* verts);

	virtual void FindAllocation(U32 & startPos);
	virtual void SetInstanceNumber(U32 numInstance);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

void AnimBillboardRenderEffect::SetInstanceNumber(U32 numInstances)
{
	numInst = numInstances;
}

bool AnimBillboardRenderEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	return false;
}

void AnimBillboardRenderEffect::FindAllocation(U32 & startPos)
{

	U32 OutputRange = parentEffect->OutputRange();
	myMeshes = new IRenderMesh * [numInst];

	for(U32 i = 0; i < numInst; ++i)
	{
		myMeshes[i] = GMRENDERER->CreateFVFMesh(OutputRange * 4,FVFColoredTexturedVertex::format(), sizeof(FVFColoredTexturedVertex), true);
		myMeshes[i]->NewFaceGroup(OutputRange * 6);
		myMeshes[i]->GetFaceGroup(0)->setMaterial(mat);
		
		IRenderFaceGroup* fg = myMeshes[i]->GetFaceGroup(0);
		fg->setRenderPriority(77);
		
		U16* inds = fg->lockIndices();
		const int anVertexIndices[6] = { 0, 1, 2, 0, 2, 3 };
		for (U16 i = 0; i < OutputRange; i++)
		{
			for (U32 k = 0; k < 6; k++)
			{
				inds[i*6+k] = i*4 + anVertexIndices[k];
			}
		}
		fg->unlockIndices();

	}
}



AnimBillboardRenderEffect::AnimBillboardRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer)  : IParticleEffect(_parent,_parentEffect)
{
	numInst = 1;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = new U8[header->size];
	memcpy(data,buffer,header->size);
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
	animColums = (EncodedFloatTypeHeader *)(data+offset);
	offset += animColums->size;
	animRows = (EncodedFloatTypeHeader *)(data+offset);
	offset += animRows->size;
	fps = (EncodedFloatTypeHeader *)(data+offset);
	offset += fps->size;		

	orientationTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientationTrans->size;
	blendMode = *((BRE_BlendMode*)(data+offset));
	offset += sizeof(U32);
	orientType= *((BRE_Orientation*)(data+offset));
	offset += sizeof(U32);
	orientAxis= *((AxisEnum*)(data+offset));
	offset += sizeof(U32);
	animType= *((ABR_AnimType*)(data+offset));
	offset += sizeof(U32);

	char * matHeader = (char *)(data+offset);
	char fileName[512];
	sprintf(fileName, "particles\\%s", matHeader);

	const char * type = "PartMult";
	if(blendMode ==	BRE_ADDITIVE)
	{
		type = "PartAdd";
	}

	mat = GMRENDERER->LoadMaterialByName(fileName,type);
	mat->SetIgnoreLighting(true);
	mat->SetCullMode(D3DCULL_NONE);
	if(blendMode ==	BRE_ADDITIVE)
	{
		mat->SetZWrite(false);
		mat->SetSourceBlend(D3DBLEND_ONE);
		mat->SetDestBlend(D3DBLEND_ONE);
	}
	//else //BRE_MULTIPLY
	mat->SetZWrite(false);

	framePerSecond = EncodeParam::GetFloat(fps,NULL,parentSystem);
	numRows = EncodeParam::GetFloat(animRows,NULL,parentSystem);
	numColumns = EncodeParam::GetFloat(animColums,NULL,parentSystem);
	frameCount = numRows*numColumns;
}

AnimBillboardRenderEffect::~AnimBillboardRenderEffect()
{
	delete data;
	if (myMeshes)
	{
		for (U32 i = 0; i < numInst; i++)
		{
			GMRENDERER->destroyMesh(myMeshes[i]);
		}
		delete [] myMeshes;
	}
	if(mat)
	{
		GMRENDERER->ReleaseMaterialRef(mat);
		mat = NULL;
	}
}


void AnimBillboardRenderEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	interp = t;
	UpdateMesh(inputStart, numInput, false, instance,parentTrans);
	myMeshes[instance]->Draw(NULL, Vector(0,0,0));
}


void AnimBillboardRenderEffect::HideVert(	U32 i, 
										Particle * p1, 
										Particle * p2, 
										FVFColoredTexturedVertex* verts)
{
	verts[i*4].a = 0;
	verts[i*4+1].a = 0;
	verts[i*4+2].a = 0;
	verts[i*4+3].a = 0;
}

void AnimBillboardRenderEffect::CalcVertColor(U32 i, 
										 Particle * p1, 
										 Particle * p2, 
										 FVFColoredTexturedVertex* verts)
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

	verts[i*4].r = 255 * r;
	verts[i*4+1].r = 255 * r;
	verts[i*4+2].r = 255 * r;
	verts[i*4+3].r = 255 * r;

	verts[i*4].g = 255 * g;
	verts[i*4+1].g = 255 * g;
	verts[i*4+2].g = 255 * g;
	verts[i*4+3].g = 255 * g;
	verts[i*4].b = 255 * b;
	verts[i*4+1].b = 255 * b;
	verts[i*4+2].b = 255 * b;
	verts[i*4+3].b = 255 * b;
	verts[i*4].a = 255 * a;
	verts[i*4+1].a = 255 * a;
	verts[i*4+2].a = 255 * a;
	verts[i*4+3].a = 255 * a;

	U32 timeAliveMS = (currentTimeMS-p2[i].birthTime);

	U32 currentFrame = (F2LONG((timeAliveMS*framePerSecond)/1000.0f))%frameCount;
	U32 currentRow = currentFrame%numRows;
	U32 currentColumn = currentFrame/numRows;

	SINGLE columnSize = 1.0f/numColumns;
	SINGLE rowSize = 1.0f/numRows;
	
	verts[i*4].uv =D3DXVECTOR2(currentColumn*columnSize,currentRow*rowSize);
	verts[i*4+1].uv =D3DXVECTOR2((currentColumn+1)*columnSize,currentRow*rowSize);
	verts[i*4+2].uv =D3DXVECTOR2((currentColumn+1)*columnSize,(currentRow+1)*rowSize);
	verts[i*4+3].uv =D3DXVECTOR2(currentColumn*columnSize,(currentRow+1)*rowSize);
}

void AnimBillboardRenderEffect::CalcVertA(	U32 i, 
										Particle * p1, 
										Particle * p2, 
										FVFColoredTexturedVertex* verts,
										const Vector& xBB,
										const Vector& yBB,
										Transform & parentTrans)
{
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
	//Vector epos = p2[i].pos+xBB*offX+yBB*offY;
	
	verts[i*4].p = *(D3DXVECTOR3*)&(epos+xBB*(0-pivx)-yBB*(0-pivy));
	verts[i*4+1].p =*(D3DXVECTOR3*)&(epos+xBB*(0-pivx)-yBB*(scaleY-pivy));
	verts[i*4+2].p =*(D3DXVECTOR3*)&(epos+xBB*(scaleX-pivx)-yBB*(scaleY-pivy));
	verts[i*4+3].p =*(D3DXVECTOR3*)&(epos+xBB*(scaleX-pivx)-yBB*(0-pivy));
	CalcVertColor(i,p1,p2,verts);
}




void AnimBillboardRenderEffect::CalcVertB(	U32 i, 
										Particle * p1, 
										Particle * p2, 
										FVFColoredTexturedVertex* verts,
										const Vector& xBB,
										const Vector& yBB,
										float angle,
										Transform & parentTrans)
{
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
	//Vector epos = p2[i].pos + xBB*((offX)*cosA-(offY)*sinA)-yBB*((offX)*sinA+(offY)*cosA);
	verts[i*4].p =*(D3DXVECTOR3*)&(epos+xBB*((0-pivx)*cosA-(0-pivy)*sinA)-yBB*((0-pivx)*sinA+(0-pivy)*cosA));
	verts[i*4+1].p =*(D3DXVECTOR3*)&(epos+xBB*((0-pivx)*cosA-(scaleY-pivy)*sinA)-yBB*((0-pivx)*sinA+(scaleY-pivy)*cosA));
	verts[i*4+2].p =*(D3DXVECTOR3*)&(epos+xBB*((scaleX-pivx)*cosA-(scaleY-pivy)*sinA)-yBB*((scaleX-pivx)*sinA+(scaleY-pivy)*cosA));
	verts[i*4+3].p =*(D3DXVECTOR3*)&(epos+xBB*((scaleX-pivx)*cosA-(0-pivy)*sinA)-yBB*((scaleX-pivx)*sinA+(0-pivy)*cosA));
	CalcVertColor(i,p1,p2,verts);
}


bool AnimBillboardRenderEffect::UpdateMesh(U32 inputStart, U32 numInput, bool bShutdown,U32 instance, Transform & parentTrans)
{
	myMeshes[instance]->GetFaceGroup(0)->SetNumIndicesToUse(numInput * 6);
	FVFColoredTexturedVertex* verts = (FVFColoredTexturedVertex*) myMeshes[instance]->LockVertices();
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	
	float t = 0;
	switch(orientType)
	{
	case BRE_ALIGNED:
		{
			Transform trans;
			Vector xBB;
			Vector yBB;
			if(EncodeParam::FindObjectTransform(parentSystem,p2,t,orientationTrans,trans,instance))
			{
				if(orientAxis == AXIS_I)
				{
					xBB = trans.get_j();
					yBB = trans.get_k();
				}
				else if(orientAxis == AXIS_J)
				{
					xBB = trans.get_k();
					yBB = trans.get_i();
				}
				else
				{
					xBB = trans.get_i();
					yBB = trans.get_j();
				}
			}
			else
			{
				Vector camDir;
				camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
				Vector upVect;
				if(camDir.x == 0 && camDir.y == 0 && camDir.z == 1)
				{
					upVect = Vector(0,1.0,0);
				}
				else
				{
					upVect = Vector(0,0,1.0);
				}
				xBB = cross_product(camDir, upVect);
				yBB = cross_product(camDir, xBB);
			}
			
			xBB.normalize();
			yBB.normalize();

			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					CalcVertA(i,p1,p2,verts,xBB,yBB,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}
		}
		break;
	case BRE_FIXED:
		{
			Vector camDir;
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
			
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
			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					SINGLE angle = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					CalcVertB(i,p1,p2,verts,xBB,yBB,angle,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}		
		}
		break;
	case BRE_SPIN:
		{
			Vector camDir;
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
			
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
			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					SINGLE startAngle = (((SINGLE)(((i+p2[i].birthTime)*1234)%1000))*6.28)/1000.0;
					SINGLE rate = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					SINGLE age = (currentTimeMS-p2[i].birthTime)/1000.0;
					SINGLE angle = startAngle + rate*age;
					CalcVertB(i,p1,p2,verts,xBB,yBB,angle,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}		
		}
		break;
	case BRE_PARTICLE_ALIGNED:
		{			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
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
					{
						xBB = vJ;
						yBB = vK;
					}
					else if(orientAxis == AXIS_J)
					{
						xBB = vK;
						yBB = vI;
					}
					else
					{
						xBB = vI;
						yBB = vJ;
					}
					CalcVertA(i,p1,p2,verts,xBB,yBB,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}
		}
		break;
	case BRE_PINBOARD_PARTICLE_ALIGNED:
		{			
			Vector camDir;
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
			camDir.fast_normalize();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
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

					yBB = cross_product(xBB,camDir);
					yBB.fast_normalize();
					CalcVertA(i,p1,p2,verts,xBB,yBB,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}
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
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
			camDir.fast_normalize();
			yBB = cross_product(xBB,camDir);
			yBB.fast_normalize();
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					CalcVertA(i,p1,p2,verts,xBB,yBB,parentTrans);
				}
				else HideVert(i,p1,p2,verts);
			}
		}
		break;
	}
	myMeshes[instance]->UnlockVertices(false);
	return true;
}

IParticleEffect * AnimBillboardRenderEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	return NULL;
}

#endif
