#ifndef PFMESHCUT_H
#define PFMESHCUT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMeshCut.h                                 //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MeshCutProgramer : public ParticleProgramer
{
	TransformType * orientation;
	FloatType * slice;

	MeshCutProgramer();
	~MeshCutProgramer();

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

MeshCutProgramer::MeshCutProgramer()
{
	orientation = MakeDefaultTrans();
	slice = MakeDefaultFloat(0.5);
}

MeshCutProgramer::~MeshCutProgramer()
{
	delete orientation;
	delete slice;
}

//IParticleProgramer
U32 MeshCutProgramer::GetNumOutput()
{
	return 2;
}

const char * MeshCutProgramer::GetOutputName(U32 index)
{
	return FI_MESH_INST;
}

U32 MeshCutProgramer::GetNumInput()
{
	return 1;
}

const char * MeshCutProgramer::GetInputName(U32 index)
{
	return FI_MESH_INST;
}
U32 MeshCutProgramer::GetNumFloatParams()
{
	return 1;
}

const char * MeshCutProgramer::GetFloatParamName(U32 index)
{
	switch(index)
	{
	case 0:
		return "Slice Percent";
	}
	return NULL;
}

const FloatType * MeshCutProgramer::GetFloatParam(U32 index)
{
	switch(index)
	{
	case 0:
		return slice;
	}
	return NULL;
}

void MeshCutProgramer::SetFloatParam(U32 index,const FloatType * param)
{
	switch(index)
	{
	case 0:
		{
			if(slice)
				delete slice;
			if(param)
				slice = param->CreateCopy();
			else
				slice = NULL;
			break;
		}

	}
}

U32 MeshCutProgramer::GetNumTransformParams()
{
	return 1;
}

const char * MeshCutProgramer::GetTransformParamName(U32 index)
{
	return "Orientation";
}

const TransformType * MeshCutProgramer::GetTransformParam(U32 index)
{
	return orientation;
}

void MeshCutProgramer::SetTransformParam(U32 index,const TransformType * param)
{
	if(orientation)
		delete orientation;
	if(param)
		orientation = param->CreateCopy();
	else
		orientation = NULL;
}

U32 MeshCutProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)
		+ EncodeParam::EncodedTransformSize(orientation)
		+ EncodeParam::EncodedFloatSize(slice);
}

void MeshCutProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MESH_CUT;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	EncodeParam::EncodeTransform(orientationHeader ,orientation);
	offset +=orientationHeader->size;

	EncodedFloatTypeHeader * sliceHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	EncodeParam::EncodeFloat(sliceHeader ,slice);
	offset +=sliceHeader->size;

	header->size = offset;
}

void MeshCutProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	EncodedTransformTypeHeader * orientationHeader = (EncodedTransformTypeHeader *)(buffer+offset);
	orientation = EncodeParam::DecodeTransform(orientationHeader);
	offset += orientationHeader->size;

	EncodedFloatTypeHeader * sliceHeader = (EncodedFloatTypeHeader *)(buffer+offset);
	slice = EncodeParam::DecodeFloat(sliceHeader);
	offset += sliceHeader->size;
}

struct MeshCutEffect : public IParticleEffect
{
	IParticleEffect * user1;
	IParticleEffect * user2;

	char effectName[32];

	U8 * data;

	EncodedTransformTypeHeader * orientation;
	EncodedFloatTypeHeader * slice;

	IMeshInstance * mesh1;
	IMeshInstance * mesh2;
	SINGLE cachedSlice;
	Vector cachedDir;

	MeshCutEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MeshCutEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual IMeshInstance * GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance);

};

MeshCutEffect::MeshCutEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	user1 = NULL;
	user2 = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	orientation = (EncodedTransformTypeHeader *)(data+offset);
	offset += orientation->size;

	slice = (EncodedFloatTypeHeader *)(data+offset);
	offset += slice->size;

	mesh1 = NULL;
	mesh2 = NULL;
}

MeshCutEffect::~MeshCutEffect()
{
	if(mesh1)
	{
		parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(mesh1);
		mesh1= NULL;
	}
	if(mesh2)
	{
		parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(mesh2);
		mesh2 = NULL;
	}
	if(user1)
		delete user1;
	if(user2)
		delete user2;
}

IParticleEffectInstance * MeshCutEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(user1)
			delete user1;
		user1 = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return user1;
	}
	if(outputID == 1)
	{
		if(user2)
			delete user2;
		user2 = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return user2;
	}
	return NULL;
}

void MeshCutEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(user1)
			delete user1;
		user1 = (IParticleEffect*)target;
		user1->parentEffect[inputID] = this;
	}
	else if(outputID == 1)
	{
		if(user2)
			delete user2;
		user2 = (IParticleEffect*)target;
		user2->parentEffect[inputID] = this;
	}
};

void MeshCutEffect::DeleteOutput()
{
	if(user1)
	{
		delete user1;
		user1 = NULL;
	}
	if(user2)
	{
		delete user2;
		user2 = NULL;
	}
}

void MeshCutEffect::NullOutput(IParticleEffect * target)
{
	if(user1 == target)
		user1 = NULL;
	if(user2 == target)
		user2 = NULL;
}

IParticleEffect * MeshCutEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	IParticleEffect * retVal = NULL;
	if(user1)
		retVal = user1->FindFilter(searchName);
	if(retVal)
		return retVal;
	if(user2)
		return user2->FindFilter(searchName);
	return NULL;
}

IMeshInstance * MeshCutEffect::GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance)
{
	Transform trans;
	if(EncodeParam::FindObjectTransform(parentSystem,particle,0.0,orientation,trans,instance))
	{
		SINGLE sliceValue = EncodeParam::GetFloat(slice,particle,parentSystem);
		if((sliceValue != cachedSlice || (!(trans.get_k() == cachedDir))) || (outputID == user1 && mesh1 ==NULL) || (outputID == user2 && mesh2 ==NULL) )//do I need to rebuild the meshes
		{
			cachedDir = trans.get_k();
			cachedSlice = sliceValue;
			trans.make_orthogonal();
			if(parentEffect[0])
			{
				IMeshInstance * source = parentEffect[0]->GetMeshInstance(particle,this,instance);
				if(source)
				{
					if(user1)
					{
						if(!mesh1)
							mesh1 = parentSystem->GetOwner()->GetMeshManager()->CreateRefMesh(source);
						else 
							mesh1->GetArchtype()->ResetRef();
						mesh1->GetArchtype()->MeshOperationCut(sliceValue,trans.get_k(),mesh1);
					}
					if(user2)
					{
						if(!mesh2)
							mesh2 = parentSystem->GetOwner()->GetMeshManager()->CreateRefMesh(source);
						else 
							mesh2->GetArchtype()->ResetRef();
						mesh2->GetArchtype()->MeshOperationCut(1.0-sliceValue,-(trans.get_k()),mesh1);
					}
				}
			}
		}
	}
	if(outputID == user1)
		return mesh1;
	else if(outputID == user2)
		return mesh2;
	return NULL;
}



#endif
