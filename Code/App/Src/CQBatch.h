#ifndef CQBATCH_H
#define CQBATCH_H

#define CQ_RPRSTATE(n) ((RPRSTATE)(RPR_MAX_STATE+n))

#define EXTRA_STATES 1
#define RPR_DELAY 	CQ_RPRSTATE(0)

struct BATCHDESC
{
	struct BATCHEDMATERIAL *material;
	void *verts;
	U16 *indices;
	int num_verts,num_indices;
	D3DPRIMITIVETYPE type;
	U32 vertex_format;
	bool bHasRef:1;
	IRP_VERTEXBUFFERHANDLE vb_handle;
	U32 handle;
};

#define IID_ICQBatch MAKE_IID("ICQBatch",1)

struct ICQBatch : IDAComponent
{
	virtual unsigned __int64 GetTicks () = 0;

	virtual void ClearTicks () = 0;

	virtual void Reset () = 0;

	virtual U32 CreateMaterial (U32 stateID,D3DPRIMITIVETYPE _type,U32 _vertex_format) = 0;

	//true means the material was successfully loaded
	//false means user must recreate it
	virtual bool ActivateMaterial (U32 stateID,U32 hintID) = 0;

	virtual void InvalidateMaterial (U32 stateID) = 0;

	//set the "material state" and grab a hint!
	virtual void SetMStateID (U32 stateID,U32 &_hintID,D3DPRIMITIVETYPE _type,U32 _vertex_format) = 0;

	virtual void SetIdentityModelview () = 0;

	//failure means we ran out of buffer space and all locked buffers must be relocked
	virtual bool GetPrimBuffer (BATCHDESC *desc, bool bAllowFailure=0 ) = 0;

	virtual void ReleasePrimBuffer (BATCHDESC *desc) = 0;

	virtual void Startup () = 0;
	
	virtual void Shutdown () = 0;

	virtual void CommitState () = 0;

	virtual void RestoreSurfaces () = 0;
};

#endif