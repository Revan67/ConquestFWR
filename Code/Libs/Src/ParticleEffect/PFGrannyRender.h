#ifndef PFGRANNYRENDERER_H
#define PFGRANNYRENDERER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFGrannyRender.h                            //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

enum GRE_Orientation
{
	GRE_PARTICLE_ALIGNED_I,
	GRE_STRECH_TO,
	GRE_PARTICLE_ALIGNED_J,
	GRE_PARTICLE_ALIGNED_K,
	GRE_PARTICLE_ALIGNED_NEG_I,
	GRE_PARTICLE_ALIGNED_NEG_J,
	GRE_PARTICLE_ALIGNED_NEG_K,
	GRE_PARENT_ALIGNED,
};

struct GrannyRenderProgramer : public ParticleProgramer
{
	char fileName[256];

	FloatType * widthScale;
	FloatType * heightScale;
	FloatType * lengthScale;
	FloatType * orientationFloat;
	FloatType * redColor;
	FloatType * greenColor;
	FloatType * blueColor;
	FloatType * alphaColor;
	FloatType * uCrawl;
	FloatType * vCrawl;

	TransformType * orientTrans;

	BRE_BlendMode blendMode;
	GRE_Orientation orientType;

	GrannyRenderProgramer();
	~GrannyRenderProgramer();

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

GrannyRenderProgramer::GrannyRenderProgramer()
{
	fileName[0] = 0;

	widthScale = MakeDefaultFloat(1);
	heightScale = MakeDefaultFloat(1);
	lengthScale = MakeDefaultFloat(1);
	orientationFloat = MakeDefaultFloat(0);
	redColor = MakeDefaultFloat(1);
	greenColor = MakeDefaultFloat(1);
	blueColor = MakeDefaultFloat(1);
	alphaColor = MakeDefaultFloat(1);
	uCrawl = MakeDefaultFloat(0);
	vCrawl = MakeDefaultFloat(0);

	orientTrans = MakeDefaultTrans();

	blendMode = BRE_MULTIPLY;
	orientType = GRE_PARTICLE_ALIGNED_I;
}

GrannyRenderProgramer::~GrannyRenderProgramer()
{
	delete widthScale;
	delete heightScale;
	delete lengthScale;
	delete orientationFloat;
	delete redColor;
	delete greenColor;
	delete blueColor;
	delete alphaColor;
	delete uCrawl;
	delete vCrawl;
	delete orientTrans;
}

//IParticleProgramer
U32 GrannyRenderProgramer::GetNumOutput()
{
	return 0;
}

const char * GrannyRenderProgramer::GetOutputName(U32 index)
{
	return "Error";
}

U32 GrannyRenderProgramer::GetNumFloatParams()
{
	return 10;
}

const char * GrannyRenderProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Width Scale";
	case 1:
		return "Height Scale";
	case 2:
		return "Length Scale";
	case 3:
		return "Orientation Parameter";
	case 4:
		return "Red";
	case 5:
		return "Green";
	case 6:
		return "Blue";
	case 7:
		return "Alpha";
	case 8:
		return "U Crawl";
	case 9:
		return "V Crawl";
	}
	return NULL;
}

const FloatType * GrannyRenderProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return widthScale;
	case 1:
		return heightScale;
	case 2:
		return lengthScale;
	case 3:
		return orientationFloat;
	case 4:
		return redColor;
	case 5:
		return greenColor;
	case 6:
		return blueColor;
	case 7:
		return alphaColor;
	case 8:
		return uCrawl;
	case 9:
		return vCrawl;
	}
	return NULL;
}

void GrannyRenderProgramer::SetFloatParam(U32 index,const FloatType * param)
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
			if(lengthScale)
				delete lengthScale;
			if(param)
				lengthScale = param->CreateCopy();
			else
				lengthScale = NULL;
			break;
		}
	case 3:
		{
			if(orientationFloat)
				delete orientationFloat;
			if(param)
				orientationFloat = param->CreateCopy();
			else
				orientationFloat = NULL;
			break;
		}
	case 4:
		{
			if(redColor)
				delete redColor;
			if(param)
				redColor = param->CreateCopy();
			else
				redColor = NULL;
			break;
		}
	case 5:
		{
			if(greenColor)
				delete greenColor;
			if(param)
				greenColor = param->CreateCopy();
			else
				greenColor = NULL;
			break;
		}
	case 6:
		{
			if(blueColor)
				delete blueColor;
			if(param)
				blueColor = param->CreateCopy();
			else
				blueColor = NULL;
			break;
		}
	case 7:
		{
			if(alphaColor)
				delete alphaColor;
			if(param)
				alphaColor = param->CreateCopy();
			else
				alphaColor = NULL;
			break;
		}
	case 8:
		{
			if(uCrawl)
				delete uCrawl;
			if(param)
				uCrawl = param->CreateCopy();
			else
				uCrawl = NULL;
			break;
		}
	case 9:
		{
			if(vCrawl)
				delete vCrawl;
			if(param)
				vCrawl = param->CreateCopy();
			else
				vCrawl = NULL;
			break;
		}
	}
}

U32 GrannyRenderProgramer::GetNumTransformParams()
{
	return 1;
}

const char * GrannyRenderProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * GrannyRenderProgramer::GetTransformParam(U32 index)
{
	return orientTrans;
}

void GrannyRenderProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientTrans)
		delete orientTrans;
	if(param)
		orientTrans = param->CreateCopy();
	else
		orientTrans = NULL;
}

U32 GrannyRenderProgramer::GetNumStringParams()
{
	return 1;
}

const char * GrannyRenderProgramer::GetStringParamName(U32 index)
{
	return "File Name";
}

const char * GrannyRenderProgramer::GetStringParam(U32 index)
{
	return fileName;
}

void GrannyRenderProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(fileName,param);
}

U32 GrannyRenderProgramer::GetNumEnumParams()
{
	return 2;
}

const char * GrannyRenderProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Blend Mode";
	case 1:
		return "Orientation Type";
	}
	return NULL;
}

U32 GrannyRenderProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 2;
	case 1:
		return 8;
	}
	return 0;
}

const char * GrannyRenderProgramer::GetEnumValueName(U32 index, U32 value)
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
				return "Particle Aligned I";
			case 1:
				return "Strech To";
			case 2:
				return "Particle Aligned J";
			case 3:
				return "Particle Aligned K";
			case 4:
				return "Particle Aligned Neg I";
			case 5:
				return "Particle Aligned Neg J";
			case 6:
				return "Particle Aligned Neg K";
			case 7:
				return "Parent Aligned";
			}
		}
		break;
	}
	return NULL;
}

U32 GrannyRenderProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return blendMode;
	case 1:
		return orientType;
	}
	return 0;
}

void GrannyRenderProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		blendMode = (BRE_BlendMode)value;
		break;
	case 1:
		orientType = (GRE_Orientation)value;
		break;
	}
}

U32 GrannyRenderProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(widthScale) 
		+ EncodeParam::EncodedFloatSize(heightScale) 
		+ EncodeParam::EncodedFloatSize(lengthScale) 
		+ EncodeParam::EncodedFloatSize(orientationFloat) 
		+ EncodeParam::EncodedFloatSize(redColor) 
		+ EncodeParam::EncodedFloatSize(greenColor) 
		+ EncodeParam::EncodedFloatSize(blueColor) 
		+ EncodeParam::EncodedFloatSize(alphaColor) 
		+ EncodeParam::EncodedFloatSize(uCrawl) 
		+ EncodeParam::EncodedFloatSize(vCrawl) 
		+ EncodeParam::EncodedTransformSize(orientTrans)
		+ sizeof(U32)
		+ sizeof(U32)
		+ (sizeof(char)*256);
}

void GrannyRenderProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_RENDER_GRANNY;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedFloatTypeHeader * widthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(widthScaleHeader,widthScale);
	offset +=widthScaleHeader->size;

	EncodedFloatTypeHeader * heightScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(heightScaleHeader ,heightScale);
	offset +=heightScaleHeader->size;

	EncodedFloatTypeHeader * lengthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(lengthScaleHeader ,lengthScale);
	offset +=lengthScaleHeader->size;

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

	EncodedFloatTypeHeader * uCrawlHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(uCrawlHeader ,uCrawl);
	offset +=uCrawlHeader->size;

	EncodedFloatTypeHeader * vCrawlHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(vCrawlHeader ,vCrawl);
	offset +=vCrawlHeader->size;

	EncodedTransformTypeHeader * orientTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientTransHeader ,orientTrans);
	offset +=orientTransHeader->size;

	U32 * blendHeader = (U32*)(buffer+offset);
	(*blendHeader) = blendMode;
	offset += sizeof(U32);

	U32 * oTypeHeader = (U32*)(buffer+offset);
	(*oTypeHeader) = orientType;
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(matHeader,fileName);
	offset += sizeof(char)*256;

	header->size = offset;
}

void GrannyRenderProgramer::SetDataChunk(U8 * buffer)
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

	EncodedFloatTypeHeader * lengthScaleHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	lengthScale = EncodeParam::DecodeFloat(lengthScaleHeader );
	offset += lengthScaleHeader ->size;

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

	EncodedFloatTypeHeader * uCrawlHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	uCrawl = EncodeParam::DecodeFloat(uCrawlHeader);
	offset += uCrawlHeader->size;

	EncodedFloatTypeHeader * vCrawlHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	vCrawl = EncodeParam::DecodeFloat(vCrawlHeader);
	offset += vCrawlHeader->size;

	EncodedTransformTypeHeader * orientTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientTrans = EncodeParam::DecodeTransform(orientTransHeader);
	offset += orientTransHeader->size;

	blendMode = *((BRE_BlendMode*)(buffer+offset));
	offset += sizeof(U32);

	orientType = *((GRE_Orientation*)(buffer+offset));
	offset += sizeof(U32);

	char * matHeader = (char *)(buffer+offset);
	strcpy(fileName,matHeader);
}

struct GrannyRenderEffect : public IParticleEffect
{
	IGrannyInstance * mesh;

	char effectName[32];

	EncodedFloatTypeHeader * widthScale;
	EncodedFloatTypeHeader * heightScale;
	EncodedFloatTypeHeader * lengthScale;
	EncodedFloatTypeHeader * orientationFloat;
	EncodedFloatTypeHeader * redColor;
	EncodedFloatTypeHeader * greenColor;
	EncodedFloatTypeHeader * blueColor;
	EncodedFloatTypeHeader * alphaColor;
	EncodedFloatTypeHeader * uCrawl;
	EncodedFloatTypeHeader * vCrawl;

	EncodedTransformTypeHeader * orientTrans;

	BRE_BlendMode blendMode;
	GRE_Orientation orientType;

	U8 * data;

	GrannyRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer);
	virtual ~GrannyRenderEffect();
	//IParticleEffectInstance
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);

	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

GrannyRenderEffect::GrannyRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer)  : IParticleEffect(_parent,_parentEffect)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = new U8[header->size];
	memcpy(data,buffer,header->size);
	U32 offset = sizeof(ParticleHeader);
	widthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += widthScale->size;
	heightScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += heightScale->size;
	lengthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += lengthScale->size;
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
	uCrawl = (EncodedFloatTypeHeader *)(data+offset);
	offset += uCrawl->size;
	vCrawl = (EncodedFloatTypeHeader *)(data+offset);
	offset += vCrawl->size;

	orientTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientTrans->size;

	blendMode = *((BRE_BlendMode*)(data+offset));
	offset += sizeof(U32);
	orientType= *((GRE_Orientation*)(data+offset));
	offset += sizeof(U32);

	char * fileName = (char *)(data+offset);
	mesh = GMGRANNY->CreateInstance(fileName);
	if(mesh && blendMode == BRE_ADDITIVE)
	{
		IGrannyMesh * gMesh = mesh->GetMesh();
		U32 i = 1;
		IRenderMaterial * mat =	gMesh->GetRenderMaterial(i);
		while(mat)
		{
			mat->SetDestBlend(D3DBLEND_ONE);
			mat->SetSourceBlend(D3DBLEND_ONE);
			mat->SetIgnoreLighting(true);
			mat->SetZWrite(false);
			++i;
			mat = gMesh->GetRenderMaterial(i);
		}
	}
}

GrannyRenderEffect::~GrannyRenderEffect()
{
	GMGRANNY->ReleaseInstance(mesh);
	delete data;
}

void GrannyRenderEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(!mesh)
		return;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	switch(orientType)
	{
	case GRE_PARENT_ALIGNED:
		{
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					Transform rotTrans;
					if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,rotTrans,instance))
					{
						float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
						float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
						float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);
						SINGLE r = EncodeParam::GetFloat(redColor,&(p2[i]),parentSystem);
						SINGLE g = EncodeParam::GetFloat(greenColor,&(p2[i]),parentSystem);
						SINGLE b = EncodeParam::GetFloat(blueColor,&(p2[i]),parentSystem);
						SINGLE a = EncodeParam::GetFloat(alphaColor,&(p2[i]),parentSystem);

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

						Transform trans;
						trans.set_i(vJ);
						trans.set_j(vK);
						trans.set_k(vI);

						Quaternion q1(rotTrans);
						Quaternion q2(trans);

						q1 = q1*q2;

						if(p1[i].bParented)
						{
							Transform temp = parentTrans;
							temp.make_orthogonal();
							Quaternion q3(temp);
							q1 = q3*q1;
						}

						trans = Matrix(q1);

						trans.d[0][0] *= xScale;
						trans.d[0][1] *= xScale;
						trans.d[0][2] *= xScale;
						trans.d[1][0] *= yScale;
						trans.d[1][1] *= yScale;
						trans.d[1][2] *= yScale;
						trans.d[2][0] *= zScale;
						trans.d[2][1] *= zScale;
						trans.d[2][2] *= zScale;

						trans.scale(FEET_TO_WORLD);
						trans.translation =	(t*(p2[i].pos - p1[i].pos))+p1[i].pos+rotTrans.translation;
						if(p1[i].bParented)
						{
							trans.translation += parentTrans.translation;
						}



						Transform texTrans;
						texTrans.d[0][2] = EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem);
						texTrans.d[1][2] = EncodeParam::GetFloat(vCrawl,&(p2[i]),parentSystem);
						
						mesh->SetTransform(trans);
						mesh->Update();
						mesh->Render(D3DXCOLOR(r,g,b,a),&texTrans);
					}
				}
			}
		}
		break;
	case GRE_PARTICLE_ALIGNED_I:
	case GRE_PARTICLE_ALIGNED_J:
	case GRE_PARTICLE_ALIGNED_K:
	case GRE_PARTICLE_ALIGNED_NEG_I:
	case GRE_PARTICLE_ALIGNED_NEG_J:
	case GRE_PARTICLE_ALIGNED_NEG_K:
		{
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					Transform rotTrans;
					if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,rotTrans,instance))
					{
						float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
						float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
						float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);
						SINGLE r = EncodeParam::GetFloat(redColor,&(p2[i]),parentSystem);
						SINGLE g = EncodeParam::GetFloat(greenColor,&(p2[i]),parentSystem);
						SINGLE b = EncodeParam::GetFloat(blueColor,&(p2[i]),parentSystem);
						SINGLE a = EncodeParam::GetFloat(alphaColor,&(p2[i]),parentSystem);

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

						Transform trans;
						switch(orientType)
						{
							case GRE_PARTICLE_ALIGNED_I:
								trans.set_i(vI);
								trans.set_j(vJ);
								trans.set_k(vK);
								break;
							case GRE_PARTICLE_ALIGNED_J:
								trans.set_i(vK);
								trans.set_j(vI);
								trans.set_k(vJ);
								break;
							case GRE_PARTICLE_ALIGNED_K:
								trans.set_i(vJ);
								trans.set_j(vK);
								trans.set_k(vI);
								break;
							case GRE_PARTICLE_ALIGNED_NEG_I:
								trans.set_i(-vI);
								trans.set_j(-vJ);
								trans.set_k(vK);
								break;
							case GRE_PARTICLE_ALIGNED_NEG_J:
								trans.set_i(vK);
								trans.set_j(-vI);
								trans.set_k(-vJ);
								break;
							case GRE_PARTICLE_ALIGNED_NEG_K:
								trans.set_i(-vJ);
								trans.set_j(vK);
								trans.set_k(-vI);
								break;
						}
						Quaternion q1(rotTrans);
						Quaternion q2(trans);

						q1 = q1*q2;

						if(p1[i].bParented)
						{
							Transform temp = parentTrans;
							temp.make_orthogonal();
							Quaternion q3(temp);
							q1 = q1*q3;
						}

						trans = Matrix(q1);

						trans.d[0][0] *= xScale;
						trans.d[0][1] *= xScale;
						trans.d[0][2] *= xScale;
						trans.d[1][0] *= yScale;
						trans.d[1][1] *= yScale;
						trans.d[1][2] *= yScale;
						trans.d[2][0] *= zScale;
						trans.d[2][1] *= zScale;
						trans.d[2][2] *= zScale;

						trans.scale(FEET_TO_WORLD);
						trans.translation =	(t*(p2[i].pos - p1[i].pos))+p1[i].pos+rotTrans.translation;
						if(p1[i].bParented)
						{
							trans.translation += parentTrans.translation;
						}



						Transform texTrans;
						//texTrans.scale(EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem));
	//					texTrans.translation.x = EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem);
	//					texTrans.translation.y = EncodeParam::GetFloat(vCrawl,&(p2[i]),parentSystem);
						texTrans.d[0][2] = EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem);
						texTrans.d[1][2] = EncodeParam::GetFloat(vCrawl,&(p2[i]),parentSystem);
						
						mesh->SetTransform(trans);
						mesh->Update();
						mesh->Render(D3DXCOLOR(r,g,b,a),&texTrans);
					}
				}
			}
		}
		break;
	case GRE_STRECH_TO:
		{
			Transform trans;
			mesh->SetTransform(trans);
			mesh->Update();
			SINGLE boundingRad = mesh->GetBoundingRadius()*FEET_TO_WORLD;
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < currentTimeMS))
				{
					Transform endTrans;
					if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,endTrans,instance))
					{
						float strech = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem);
						float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
						float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
						float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);
						SINGLE r = EncodeParam::GetFloat(redColor,&(p2[i]),parentSystem);
						SINGLE g = EncodeParam::GetFloat(greenColor,&(p2[i]),parentSystem);
						SINGLE b = EncodeParam::GetFloat(blueColor,&(p2[i]),parentSystem);
						SINGLE a = EncodeParam::GetFloat(alphaColor,&(p2[i]),parentSystem);

						Vector vI,vJ,vK;
						Vector startPos = (t*(p2[i].pos - p1[i].pos))+p1[i].pos;
						Vector endPos = (strech*(endTrans.translation - startPos))+startPos;
						SINGLE motionMag = (startPos-endPos).fast_magnitude();
						vK = endTrans.translation - startPos;
						if(vK.x == 0 && vK.y == 0 && vK.z == 0)
							vK = Vector(1,0,0);
						else
						{
							vK.fast_normalize();
						}
						if(vK.x == 0 && vK.y == 0 && vK.z == 1)
						{
							vI = Vector(0,1.0,0);
						}
						else
						{
							vI = Vector(0,0,1.0);
						}
						vJ = cross_product(vK,vI);
						vJ.fast_normalize();
						vI = cross_product(vJ,vK);
						vI.fast_magnitude();
						
						Transform strechTrans;
						strechTrans.d[0][0] *= xScale;
						strechTrans.d[1][1] *= yScale;
						strechTrans.d[2][2] = zScale*(motionMag/(boundingRad*2));
						Transform trans;
						trans.set_i(vI);
						trans.set_j(vJ);
						trans.set_k(vK);
						trans.scale(FEET_TO_WORLD);
						trans.translation =	(startPos+endPos)*0.5;

						trans = trans*strechTrans;

						Transform texTrans;
						//texTrans.scale(EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem));
	//					texTrans.translation.x = EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem);
	//					texTrans.translation.y = EncodeParam::GetFloat(vCrawl,&(p2[i]),parentSystem);
						texTrans.d[0][2] = EncodeParam::GetFloat(uCrawl,&(p2[i]),parentSystem);
						texTrans.d[1][2] = EncodeParam::GetFloat(vCrawl,&(p2[i]),parentSystem);
						
						mesh->SetTransform(trans);
						mesh->Update();
						mesh->Render(D3DXCOLOR(r,g,b,a),&texTrans);
					}
				}
			}
		}
		break;
	}
}

IParticleEffect * GrannyRenderEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	return NULL;
}

bool GrannyRenderEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	return false;
}


#endif
