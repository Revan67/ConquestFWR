#ifndef PFMESHREF_H
#define PFMESHREF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMeshRef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MeshRefProgramer : public ParticleProgramer
{
	U32 target;

	MeshRefProgramer();
	~MeshRefProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumTargetParams();

	virtual const char * GetTargetParamName(U32 index);

	virtual const U32 GetTargetParam(U32 index);

	virtual void SetTargetParam(U32 index,U32 param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

MeshRefProgramer::MeshRefProgramer()
{
	target = 0;
}

MeshRefProgramer::~MeshRefProgramer()
{
}

//IParticleProgramer
U32 MeshRefProgramer::GetNumOutput()
{
	return 1;
}

const char * MeshRefProgramer::GetOutputName(U32 index)
{
	return FI_MESH_INST;
}

U32 MeshRefProgramer::GetNumInput()
{
	return 0;
}

const char * MeshRefProgramer::GetInputName(U32 index)
{
	return "Error";
}

U32 MeshRefProgramer::GetNumTargetParams()
{
	return 1;
}

const char * MeshRefProgramer::GetTargetParamName(U32 index)
{
	return "Target";
}

const U32 MeshRefProgramer::GetTargetParam(U32 index)
{
	return target;
}

void MeshRefProgramer::SetTargetParam(U32 index,U32 param)
{
	target = param;
}


U32 MeshRefProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)
		+ (sizeof(U32));
}

void MeshRefProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MESH_REF;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	U32 * targetHeader = (U32 *)(buffer+offset);
	*targetHeader = target;
	offset += sizeof(U32);

	header->size = offset;
}

void MeshRefProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	U32 * targetHeader = (U32 *)(buffer+offset);
	target = (*targetHeader);
}

struct MeshRefEffect : public IParticleEffect
{
	IParticleEffect * user;

	char effectName[32];

	U8 * data;

	IMeshInstance * mesh;
	IMeshInstance * cachedBaseMesh;
	U32 target;

	MeshRefEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MeshRefEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual IMeshInstance * GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance);

};

MeshRefEffect::MeshRefEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	user = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	U32 * targetHeader = (U32*)(data+offset);
	target = *targetHeader;
	offset += sizeof(U32);

	cachedBaseMesh = NULL;
	mesh = NULL;
}

MeshRefEffect::~MeshRefEffect()
{
	if(mesh)
	{
		parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(mesh);
		mesh = NULL;
	}
	if(user)
		delete user;
}

IParticleEffectInstance * MeshRefEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
{
	if(outputID == 0)
	{
		if(user)
			delete user;
		user = createParticalEffect(parentSystem,type,buffer,this,inputID);
		return user;
	}
	return NULL;
}

void MeshRefEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(user)
			delete user;
		user = (IParticleEffect*)target;
		user->parentEffect[inputID] = this;
	}
};

void MeshRefEffect::DeleteOutput()
{
	if(user)
	{
		delete user;
		user = NULL;
	}
}

void MeshRefEffect::NullOutput(IParticleEffect * target)
{
	if(user == target)
		user = NULL;
}

IParticleEffect * MeshRefEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(user)
		return user->FindFilter(searchName);
	return NULL;
}

IMeshInstance * MeshRefEffect::GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance)
{
	IMeshInstance * baseMesh = parentSystem->GetOwner()->GetCallback()->GetObjectMesh(target,parentSystem->GetContext());
	if(baseMesh)
	{
		if(baseMesh != cachedBaseMesh || (!mesh))
		{
			if(mesh)
			{
				parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(mesh);
				mesh = NULL;
			}
			cachedBaseMesh = baseMesh;
			mesh = parentSystem->GetOwner()->GetMeshManager()->CreateRefMesh(baseMesh);
		}
		return mesh;
	}
	return NULL;
}



#endif
