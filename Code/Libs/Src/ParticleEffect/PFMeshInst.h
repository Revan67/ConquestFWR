#ifndef PFMESHINST_H
#define PFMESHINST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              PFMeshInst.h                                //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//////////////////////////////////////////////////////////////////////////////
//This header is only intended to be included into particles.cpp, use at your own peril.
//////////////////////////////////////////////////////////////////////////////

struct MeshInstProgramer : public ParticleProgramer
{
	char meshName[256];

	MeshInstProgramer();
	~MeshInstProgramer();

	//IParticleProgramer
	virtual U32 GetNumOutput();

	virtual const char * GetOutputName(U32 index);

	virtual U32 GetNumInput();

	virtual const char * GetInputName(U32 index);

	virtual U32 GetNumStringParams();

	virtual const char * GetStringParamName(U32 index);

	virtual const char * GetStringParam(U32 index);

	virtual void SetStringParam(U32 index,const char * param);

	virtual U32 GetDataChunkSize();

	virtual void GetDataChunk(U8 * buffer);

	virtual void SetDataChunk(U8 * buffer);
};

MeshInstProgramer::MeshInstProgramer()
{
	meshName[0] = 0;
}

MeshInstProgramer::~MeshInstProgramer()
{
}

//IParticleProgramer
U32 MeshInstProgramer::GetNumOutput()
{
	return 1;
}

const char * MeshInstProgramer::GetOutputName(U32 index)
{
	return FI_MESH_INST;
}

U32 MeshInstProgramer::GetNumInput()
{
	return 0;
}

const char * MeshInstProgramer::GetInputName(U32 index)
{
	return "Error";
}

U32 MeshInstProgramer::GetNumStringParams()
{
	return 1;
}

const char * MeshInstProgramer::GetStringParamName(U32 index)
{
	return "Mesh Name";
}

const char * MeshInstProgramer::GetStringParam(U32 index)
{
	return meshName;
}

void MeshInstProgramer::SetStringParam(U32 index,const char * param)
{
	strcpy(meshName,param);
}

U32 MeshInstProgramer::GetDataChunkSize()
{
	return sizeof(ParticleHeader)
		+ (sizeof(char)*256);
}

void MeshInstProgramer::GetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	header->version = PARTICLE_SAVE_VERSION;
	header->type = PE_MESH_INST;
	strcpy(header->effectName,effectName);

	U32 offset = sizeof(ParticleHeader);

	char * meshHeader = (char *)(buffer+offset);
	strcpy(meshHeader,meshName);
	offset += sizeof(char)*256;

	header->size = offset;
}

void MeshInstProgramer::SetDataChunk(U8 * buffer)
{
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	U32 offset = sizeof(ParticleHeader);

	char * meshHeader = (char *)(buffer+offset);
	strcpy(meshName,meshHeader);
}

struct MeshInstEffect : public IParticleEffect
{
	IParticleEffect * user;

	char effectName[32];

	U8 * data;

	IMeshInstance * mesh;

	MeshInstEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID);
	virtual ~MeshInstEffect();
	//IParticleEffectInstance
	virtual IParticleEffectInstance * AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer);
	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target);

	//IParticleEffect
	virtual void DeleteOutput();
	virtual void NullOutput(IParticleEffect * target);
	virtual IParticleEffect * FindFilter(const char * searchName);
	virtual IMeshInstance * GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance);

};

MeshInstEffect::MeshInstEffect(ParticleSystem * _parent,IParticleEffect * _parentEffect, U8 * buffer, U32 inputID) : IParticleEffect(_parent,_parentEffect,1,inputID)
{
	user = NULL;
	ParticleHeader * header = (ParticleHeader *)buffer;
	strcpy(effectName,header->effectName);

	data = buffer;

	U32 offset = sizeof(ParticleHeader);

	char * meshName = (char*)(data+offset);
	offset += sizeof(char)*256;

	mesh = parentSystem->GetOwner()->GetMeshManager()->CreateMesh(meshName);
}

MeshInstEffect::~MeshInstEffect()
{
	if(mesh)
	{
		parentSystem->GetOwner()->GetMeshManager()->DestroyMesh(mesh);
		mesh = NULL;
	}
	if(user)
		delete user;
}

IParticleEffectInstance * MeshInstEffect::AddFilter(U32 outputID, U32 inputID, ParticleEffectType type, U8 * buffer)
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

void MeshInstEffect::LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target)
{
	if(outputID == 0)
	{
		if(user)
			delete user;
		user = (IParticleEffect*)target;
		user->parentEffect[inputID] = this;
	}
};

void MeshInstEffect::DeleteOutput()
{
	if(user)
	{
		delete user;
		user = NULL;
	}
}

void MeshInstEffect::NullOutput(IParticleEffect * target)
{
	if(user == target)
		user = NULL;
}

IParticleEffect * MeshInstEffect::FindFilter(const char * searchName)
{
	if(strcmp(searchName,effectName) == 0)
		return this;
	else if(user)
		return user->FindFilter(searchName);
	return NULL;
}

IMeshInstance * MeshInstEffect::GetMeshInstance(Particle * particle,IParticleEffect * outputID,U32 instance)
{
	return mesh;
}



#endif
