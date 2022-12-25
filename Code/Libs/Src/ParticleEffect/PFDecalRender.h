#ifndef PFDECALRENDERER_H
#define PFDECALRENDERER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFDecalRender.h                             //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

#include "PFBillboardRender.h"

struct DecalProgramer : public ParticleProgramer
{
	char materialName[256];

	FloatType * widthScale;
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

	DecalProgramer();
	~DecalProgramer();

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

DecalProgramer::DecalProgramer()
{
	materialName[0] = 0;

	widthScale = MakeDefaultFloat(1);
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

DecalProgramer::~DecalProgramer()
{
	delete widthScale;
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
U32 DecalProgramer::GetNumOutput()
{
	return 0;
}

const char * DecalProgramer::GetOutputName(U32 index)
{
	return "Error";
}

U32 DecalProgramer::GetNumFloatParams()
{
	return 8;
}

const char * DecalProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Width Scale";
	case 1:
		return "Orientation Parameter";
	case 2:
		return "Red";
	case 3:
		return "Green";
	case 4:
		return "Blue";
	case 5:
		return "Alpha";
	case 6:
		return "OffsetX";
	case 7:
		return "OffsetY";
	}
	return NULL;
}

const FloatType * DecalProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return widthScale;
	case 1:
		return orientationFloat;
	case 2:
		return redColor;
	case 3:
		return greenColor;
	case 4:
		return blueColor;
	case 5:
		return alphaColor;
	case 6:
		return offsetX;
	case 7:
		return offsetY;
	}
	return NULL;
}

void DecalProgramer::SetFloatParam(U32 index,const FloatType * param)
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
			if(orientationFloat)
				delete orientationFloat;
			if(param)
				orientationFloat = param->CreateCopy();
			else
				orientationFloat = NULL;
			break;
		}
	case 2:
		{
			if(redColor)
				delete redColor;
			if(param)
				redColor = param->CreateCopy();
			else
				redColor = NULL;
			break;
		}
	case 3:
		{
			if(greenColor)
				delete greenColor;
			if(param)
				greenColor = param->CreateCopy();
			else
				greenColor = NULL;
			break;
		}
	case 4:
		{
			if(blueColor)
				delete blueColor;
			if(param)
				blueColor = param->CreateCopy();
			else
				blueColor = NULL;
			break;
		}
	case 5:
		{
			if(alphaColor)
				delete alphaColor;
			if(param)
				alphaColor = param->CreateCopy();
			else
				alphaColor = NULL;
			break;
		}
	case 6:
		{
			if(offsetX)
				delete offsetX;
			if(param)
				offsetX = param->CreateCopy();
			else
				offsetX = NULL;
			break;
		}
	case 7:
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

U32 DecalProgramer::GetNumTransformParams()
{
	return 1;
}

const char * DecalProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * DecalProgramer::GetTransformParam(U32 index)
{
	return orientationTrans;
}

void DecalProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientationTrans)
		delete orientationTrans;
	if(param)
		orientationTrans = param->CreateCopy();
	else
		orientationTrans = NULL;
}

U32 DecalProgramer::GetNumStringParams()
{
	return 1;
}

const char * DecalProgramer::GetStringParamName(U32 index)
{
	return "Material Name";
}

const char * DecalProgramer::GetStringParam(U32 index)
{
	return materialName;
}

void DecalProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(materialName,param);
}

U32 DecalProgramer::GetNumEnumParams()
{
	return 2;
}

const char * DecalProgramer::GetEnumParamName(U32 index)
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

U32 DecalProgramer::GetNumEnumValues(U32 index)
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

const char * DecalProgramer::GetEnumValueName(U32 index, U32 value)
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

U32 DecalProgramer::GetEnumParam(U32 index)
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

void DecalProgramer::SetEnumParam(U32 index,U32 value)
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

U32 DecalProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(widthScale) 
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

void DecalProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_DECAL_RENDER;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * widthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(widthScaleHeader,widthScale);
	offset +=widthScaleHeader->size;

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

void DecalProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * widthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	widthScale = EncodeParam::DecodeFloat(widthScaleHeader);
	offset += widthScaleHeader->size;

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

struct DecalEffect : public IParticleEffect
{
	ISOMTex * texture;

	char effectName[32];

	EncodedFloatTypeHeader * widthScale;
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

	DecalEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~DecalEffect();
	//IParticleEffectInstance
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);

	virtual IParticleEffect * FindFilter(const char * searchName);

	virtual void FindAllocation(U32 & startPos);
	virtual void SetInstanceNumber(U32 numInstance);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);

	//DecalEffect
	void renderDecal(U32 i, Particle * p1, Particle * p2, SINGLE t, Transform decalTrans,SINGLE angle,Transform & parentTrans);
};

void DecalEffect::SetInstanceNumber(U32 numInstances)
{
}

bool DecalEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	return false;
}

void DecalEffect::FindAllocation(U32 & startPos)
{
}

DecalEffect::DecalEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer)  : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = new U8[header->size];
	memcpy(data,buffer,header->size);
	U32 offset = sizeof(ParticleHeader);
	widthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += widthScale->size;
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
	char fileName[512];
	sprintf(fileName, "particles\\%s", matHeader);

	texture = GMRENDERER->CreateTextureFromFile(fileName);
	if (texture) texture->AddRef();
}

DecalEffect::~DecalEffect()
{
	delete data;
	if(texture)
	{
		texture->ReleaseRef();
		texture = NULL;
	}
}


void DecalEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	
	switch(orientType)
	{
	case BRE_ALIGNED:
		{
			Transform trans;
			Transform decalTrans;
			if(EncodeParam::FindObjectTransform(parentSystem,p2,t,orientationTrans,trans,instance))
			{
				if(orientAxis == AXIS_I)
				{
					decalTrans.set_i(trans.get_j());
					decalTrans.set_j(trans.get_k());
					decalTrans.set_k(trans.get_i());
				}
				else if(orientAxis == AXIS_J)
				{
					decalTrans.set_i(trans.get_k());
					decalTrans.set_j(trans.get_i());
					decalTrans.set_k(trans.get_j());
				}
				else
				{
					decalTrans.set_i(trans.get_i());
					decalTrans.set_j(trans.get_j());
					decalTrans.set_k(trans.get_k());
				}
			}
			else
			{
				Vector camDir;
				camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
				Vector upVect;
				camDir.fast_normalize();
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
				xBB.fast_normalize();
				yBB.fast_normalize();
				decalTrans.set_i(xBB);
				decalTrans.set_j(yBB);
				decalTrans.set_k(camDir);
			}
			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					renderDecal(i,p1,p2,t,decalTrans,0,parentTrans);
				}
			}
		}
		break;
	case BRE_FIXED:
		{
			Transform decalTrans;
			Vector camDir;
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
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
			
			xBB.fast_normalize();
			yBB.fast_normalize();
			decalTrans.set_i(xBB);
			decalTrans.set_j(yBB);
			decalTrans.set_k(camDir);
			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					SINGLE angle = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					renderDecal(i,p1,p2,t,decalTrans,angle,parentTrans);
				}
			}		
		}
		break;
	case BRE_SPIN:
		{
			Transform decalTrans;
			Vector camDir;
			camDir = GMCAMERA->get_position() - GMCAMERA->GetLookAtPosition();
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
			
			xBB.fast_normalize();
			yBB.fast_normalize();
			decalTrans.set_i(xBB);
			decalTrans.set_j(yBB);
			decalTrans.set_k(camDir);
			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					SINGLE startAngle = (((SINGLE)(((i+p2[i].birthTime)*1234)%1000))*6.28)/1000.0;
					SINGLE rate = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem)*MUL_DEG_TO_RAD;
					SINGLE age = (currentTimeMS-p2[i].birthTime)/1000.0;
					SINGLE angle = startAngle + rate*age;
					renderDecal(i,p1,p2,t,decalTrans,angle,parentTrans);
				}
			}		
		}
		break;
	case BRE_PARTICLE_ALIGNED:
		{			
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					Transform decalTrans;
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
					if(orientAxis == AXIS_I)
					{
						decalTrans.set_i(vJ);
						decalTrans.set_j(vK);
						decalTrans.set_k(vI);
					}
					else if(orientAxis == AXIS_J)
					{
						decalTrans.set_i(vK);
						decalTrans.set_j(vI);
						decalTrans.set_k(vJ);
					}
					else
					{
						decalTrans.set_i(vI);
						decalTrans.set_j(vJ);
						decalTrans.set_k(vK);
					}
					renderDecal(i,p1,p2,t,decalTrans,0,parentTrans);
				}
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
					Transform decalTrans;
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
					decalTrans.set_i(xBB);
					decalTrans.set_j(yBB);
					decalTrans.set_k(camDir);
					renderDecal(i,p1,p2,t,decalTrans,0,parentTrans);
				}
			}
		}
		break;
	case BRE_PINBOARD_AXIS_ALIGNED:
		{			
			Transform decalTrans;
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

			decalTrans.set_i(xBB);
			decalTrans.set_j(yBB);
			decalTrans.set_k(camDir);

			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					renderDecal(i,p1,p2,t,decalTrans,0,parentTrans);
				}
			}
		}
		break;
	}
}

IParticleEffect * DecalEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	return NULL;
}

void DecalEffect::renderDecal(U32 i, Particle * p1, Particle * p2, SINGLE t, Transform decalTrans,SINGLE angle,Transform & parentTrans)
{
	float offX1 = EncodeParam::GetFloat(offsetX,&(p1[i]),parentSystem)*FEET_TO_WORLD;
	float offY1 = EncodeParam::GetFloat(offsetY,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float offX2 = EncodeParam::GetFloat(offsetX,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	float offY2 = EncodeParam::GetFloat(offsetY,&(p2[i]),parentSystem)*FEET_TO_WORLD;
	
	float offX = offX1 + t*(offX2-offX1);	
	float offY = offY1 + t*(offY2-offY1);	

	float scale1 = EncodeParam::GetFloat(widthScale,&(p1[i]),parentSystem)*FEET_TO_WORLD;

	float scale2 = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem)*FEET_TO_WORLD;

	float scale = (scale1 + t*(scale2-scale1));	

	decalTrans.rotate_around_k(angle);

	Vector epos = (t*(p2[i].pos - p1[i].pos))+p1[i].pos + decalTrans.get_i()*offX-decalTrans.get_j()*offY;
	if(p1[i].bParented)
		epos = parentTrans.rotate_translate(epos);

	decalTrans.scale(scale*10);
	decalTrans.translation = epos;

	SINGLE r1 = EncodeParam::GetFloat(redColor,&(p1[i]),parentSystem);
	SINGLE g1 = EncodeParam::GetFloat(greenColor,&(p1[i]),parentSystem);
	SINGLE b1 = EncodeParam::GetFloat(blueColor,&(p1[i]),parentSystem);
	SINGLE a1 = EncodeParam::GetFloat(alphaColor,&(p1[i]),parentSystem);

	SINGLE r2 = EncodeParam::GetFloat(redColor,&(p2[i]),parentSystem);
	SINGLE g2 = EncodeParam::GetFloat(greenColor,&(p2[i]),parentSystem);
	SINGLE b2 = EncodeParam::GetFloat(blueColor,&(p2[i]),parentSystem);
	SINGLE a2 = EncodeParam::GetFloat(alphaColor,&(p2[i]),parentSystem);

	SINGLE r = r1 + t*(r2-r1);
	SINGLE g = g1 + t*(g2-g1);
	SINGLE b = b1 + t*(b2-b1);
	SINGLE a = a1 + t*(a2-a1);

	D3DXCOLOR color = D3DXCOLOR(r,g,b,a);

	GMRENDERER->DecalBegin();
	POSCALLBACK->AddGeometryToDecal(decalTrans.translation, scale,true, true);
	GMRENDERER->RealTimeDecalFinish(texture, decalTrans,color);

}

#endif
