#ifndef PFMESHRENDERER_H
#define PFMESHRENDERER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMeshRender.h                            //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
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

struct MeshRenderProgramer : public ParticleProgramer
{
	FloatType * widthScale;
	FloatType * heightScale;
	FloatType * lengthScale;
	FloatType * orientationFloat;

	TransformType * orientTrans;

	GRE_Orientation orientType;

	MeshRenderProgramer();
	~MeshRenderProgramer();

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

MeshRenderProgramer::MeshRenderProgramer()
{
	widthScale = MakeDefaultFloat(1);
	heightScale = MakeDefaultFloat(1);
	lengthScale = MakeDefaultFloat(1);
	orientationFloat = MakeDefaultFloat(0);

	orientTrans = MakeDefaultTrans();

	orientType = GRE_PARTICLE_ALIGNED_I;
}

MeshRenderProgramer::~MeshRenderProgramer()
{
	delete widthScale;
	delete heightScale;
	delete lengthScale;
	delete orientationFloat;
	delete orientTrans;
}

//IParticleProgramer
U32 MeshRenderProgramer::GetNumOutput()
{
	return 1;
}

const char * MeshRenderProgramer::GetOutputName(U32 index)
{
	return FI_MAT_MOD;
}

U32 MeshRenderProgramer::GetNumInput()
{
	return 2;
}

const char * MeshRenderProgramer::GetInputName(U32 index)
{
	switch(index)
	{
	case 0:
		return FI_POINT_LIST;
	case 1:
		return FI_MESH_INST;
	}

	return "Error";
}

U32 MeshRenderProgramer::GetNumFloatParams()
{
	return 4;
}

const char * MeshRenderProgramer::GetFloatParamName(U32 index)
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
	}
	return NULL;
}

const FloatType * MeshRenderProgramer::GetFloatParam(U32 index)
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
	}
	return NULL;
}

void MeshRenderProgramer::SetFloatParam(U32 index,const FloatType * param)
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
	}
}

U32 MeshRenderProgramer::GetNumTransformParams()
{
	return 1;
}

const char * MeshRenderProgramer::GetTransformParamName(U32 index)
{
	return "Orientation Transform";
}

const TransformType * MeshRenderProgramer::GetTransformParam(U32 index)
{
	return orientTrans;
}

void MeshRenderProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientTrans)
		delete orientTrans;
	if(param)
		orientTrans = param->CreateCopy();
	else
		orientTrans = NULL;
}

U32 MeshRenderProgramer::GetNumEnumParams()
{
	return 1;
}

const char * MeshRenderProgramer::GetEnumParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Orientation Type";
	}
	return NULL;
}

U32 MeshRenderProgramer::GetNumEnumValues(U32 index)
{
	switch(index)
	{
	case 0:
		return 8;
	}
	return 0;
}

const char * MeshRenderProgramer::GetEnumValueName(U32 index, U32 value)
{
	switch(index)
	{
	case 0:
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

U32 MeshRenderProgramer::GetEnumParam(U32 index)
{
	switch(index)
	{
	case 0:
		return orientType;
	}
	return 0;
}

void MeshRenderProgramer::SetEnumParam(U32 index,U32 value)
{
	switch(index)
	{
	case 0:
		orientType = (GRE_Orientation)value;
		break;
	}
}

U32 MeshRenderProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader) 
		+ EncodeParam::EncodedFloatSize(widthScale) 
		+ EncodeParam::EncodedFloatSize(heightScale) 
		+ EncodeParam::EncodedFloatSize(lengthScale) 
		+ EncodeParam::EncodedFloatSize(orientationFloat) 
		+ EncodeParam::EncodedTransformSize(orientTrans)
		+ sizeof(U32)
		+ sizeof(U32);
}

void MeshRenderProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_RENDER_MESH;
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

	EncodedTransformTypeHeader * orientTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientTransHeader ,orientTrans);
	offset +=orientTransHeader->size;

	U32 * oTypeHeader = (U32*)(buffer+offset);
	(*oTypeHeader) = orientType;
	offset += sizeof(U32);

	header->size = offset;
}

void MeshRenderProgramer::SetDataChunk(U8 * buffer)
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

	EncodedTransformTypeHeader * orientTransHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientTrans = EncodeParam::DecodeTransform(orientTransHeader);
	offset += orientTransHeader->size;

	orientType = *((GRE_Orientation*)(buffer+offset));
	offset += sizeof(U32);

}

struct MeshRenderEffect : public IParticleEffect
{
	char effectName[32];

	IParticleEffect * modEffect;

	EncodedFloatTypeHeader * widthScale;
	EncodedFloatTypeHeader * heightScale;
	EncodedFloatTypeHeader * lengthScale;
	EncodedFloatTypeHeader * orientationFloat;

	EncodedTransformTypeHeader * orientTrans;

	GRE_Orientation orientType;

	U8 * data;

	IModifier * modList;

	MeshRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MeshRenderEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);
	//IParticleEffect
	virtual void Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans);

	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual bool GetParentPosition(U32 index, Vector & postion,U32 & lastIndex);
};

MeshRenderEffect::MeshRenderEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID)  : IParticleEffect(_parent,_parentEffect,2, inputID)
{
	modList = NULL;
	modEffect = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);
	data = buffer;

	U32 offset = sizeof(ParticleHeader);
	widthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += widthScale->size;
	heightScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += heightScale->size;
	lengthScale = (EncodedFloatTypeHeader *)(data+offset);
	offset += lengthScale->size;
	orientationFloat = (EncodedFloatTypeHeader *)(data+offset);
	offset += orientationFloat->size;

	orientTrans = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientTrans->size;

	orientType= *((GRE_Orientation*)(data+offset));
	offset += sizeof(U32);
}

MeshRenderEffect::~MeshRenderEffect()
{
	if(modList)
	{
		modList->Release();
		modList = NULL;
	}
	delete data;
}

IParticleEffectInstance * MeshRenderEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void MeshRenderEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(modEffect)
			delete modEffect;
		modEffect = (IParticleEffect*)target;
		modEffect->parentEffect[inputID] = this;
	}
};


void MeshRenderEffect::Render(SINGLE t, U32 inputStart, U32 numInput,U32 instance, Transform & parentTrans)
{
	if(!(parentEffect[1]))
		return;
	Particle * p1 = &(parentSystem->myParticles->particles1[inputStart]);
	Particle * p2 = &(parentSystem->myParticles->particles2[inputStart]);
	switch(orientType)
	{
	case GRE_PARENT_ALIGNED:
		{
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					IMeshInstance * mesh = parentEffect[1]->GetMeshInstance(&(p2[i]),this,instance);
					if(mesh)
					{
						if(modEffect)
						{
							U32 fgCount = mesh->GetArchtype()->GetNumFaceGroups();
							for(U32 fg = 0; fg < fgCount; ++fg)
							{
								IMaterial * mat = mesh->GetArchtype()->GetFaceGroupMaterial(fg);
								if(mat)
								{
									IModifier * modSearch = modList;
									while(modSearch)
									{
										if(modSearch->GetMaterial() == mat)
											break;
										modSearch = modSearch->GetNextModifier();
									}
									modSearch = modEffect->UpdateMatMod(mat,modSearch,t, inputStart+i, 1,instance, parentTrans);
									if(!modList)
										modList = modSearch;
								}
							}
						}
						Transform rotTrans;
						if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,rotTrans,instance))
						{
							float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
							float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
							float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);

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

							mesh->SetTransform(trans);
							mesh->SetModifierList(modList);
							mesh->Update(0);
							mesh->Render();
						}
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
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					IMeshInstance * mesh = parentEffect[1]->GetMeshInstance(&(p2[i]),this,instance);
					if(mesh)
					{
						if(modEffect)
						{
							bool bNewList = (modList == NULL);
							U32 fgCount = mesh->GetArchtype()->GetNumFaceGroups();
							for(U32 fg = 0; fg < fgCount; ++fg)
							{
								if(bNewList)
								{
									IMaterial * mat = mesh->GetArchtype()->GetFaceGroupMaterial(fg);
									if(mat)
									{
										IModifier * modSearch = modList;
										while(modSearch)
										{
											if(modSearch->GetMaterial() == mat)
												break;
											modSearch = modSearch->GetNextModifier();
										}
										//modSearch is now the first materal with this material as a reference if it exists
										if(!modSearch)
										{
											modSearch = modEffect->UpdateMatMod(mat,modSearch,t, inputStart+i, 1,instance, parentTrans);
											if(modList)
											{
												IModifier * endMod = modList;
												while(endMod->GetNextModifier())
												{
													endMod = endMod->GetNextModifier();
												}
												endMod->SetNextModifier(modSearch);
											}
											else
											{
												modList = modSearch;
											}
										}
									}
								}
								else
								{
									IModifier * modSearch = modList;
									while(modSearch)
									{
										IMaterial * mat = modSearch->GetMaterial();
										modEffect->UpdateMatMod(mat,modSearch,t, inputStart+i, 1,instance, parentTrans);
										while(modSearch)
										{
											if(modSearch->GetMaterial() != mat)
												break;
											modSearch = modSearch->GetNextModifier();
										}
									}
								}
							}
						}
						Transform rotTrans;
						if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,rotTrans,instance))
						{
							float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
							float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
							float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);

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
							
							mesh->SetTransform(trans);
							mesh->SetModifierList(modList);
							mesh->Update(0);
							mesh->Render();
						}
					}
				}
			}
		}
		break;
	case GRE_STRECH_TO:
		{
			for (U16 i = 0; i < numInput; i++)
			{
				if(p2[i].bLive && p1[i].bLive && (p1[i].birthTime < parentSystem->GetCurrentTimeMS()))
				{
					IMeshInstance * mesh = parentEffect[1]->GetMeshInstance(&(p2[i]),this,instance);
					SINGLE boundingRad = mesh->GetBoundingRadius();
					if(mesh)
					{
						if(modEffect)
						{
							U32 fgCount = mesh->GetArchtype()->GetNumFaceGroups();
							for(U32 fg = 0; fg < fgCount; ++fg)
							{
								IMaterial * mat = mesh->GetArchtype()->GetFaceGroupMaterial(fg);
								if(mat)
								{
									IModifier * modSearch = modList;
									while(modSearch)
									{
										if(modSearch->GetMaterial() == mat)
											break;
										modSearch = modSearch->GetNextModifier();
									}
									modSearch = modEffect->UpdateMatMod(mat,modSearch,t, inputStart+i, 1,instance, parentTrans);
									if(!modList)
										modList = modSearch;
								}
							}
						}
						Transform endTrans;
						if(EncodeParam::FindObjectTransform(parentSystem,&(p2[i]),0.0,orientTrans,endTrans,instance))
						{
							float strech = EncodeParam::GetFloat(orientationFloat,&(p2[i]),parentSystem);
							float xScale = EncodeParam::GetFloat(widthScale,&(p2[i]),parentSystem);
							float yScale = EncodeParam::GetFloat(heightScale,&(p2[i]),parentSystem);
							float zScale = EncodeParam::GetFloat(lengthScale,&(p2[i]),parentSystem);

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

							mesh->SetTransform(trans);
							mesh->SetModifierList(modList);
							mesh->Update(0);
							mesh->Render();
						}
					}
				}
			}
		}
		break;
	}
}

IParticleEffect * MeshRenderEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	return NULL;
}

void MeshRenderEffect::DeleteOutput()
{
	if(modEffect)
	{
		delete modEffect;
		modEffect = NULL;
	}
}

void MeshRenderEffect::NullOutput(IParticleEffect * target)
{
	if(modEffect == target)
		modEffect = NULL;
}

bool MeshRenderEffect::GetParentPosition(U32 index, Vector & postion,U32 & lastIndex)
{
	return false;
}


#endif
