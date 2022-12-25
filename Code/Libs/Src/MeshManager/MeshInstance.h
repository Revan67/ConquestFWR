#ifndef MESHINSTANCE_H
#define MESHINSTANCE_H

//--------------------------------------------------------------------------//
//                                                                          //
//                              MeshInstance.H                              //
//                                                                          //
//               COPYRIGHT (C) 2004 By Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//

struct MeshInstance : public IMeshInstance, Animation::IEventHandler
{
	MeshInstance * next;
	struct MeshArch * arch;
	struct IInternalMeshManager * owner;

	INSTANCE_INDEX instanceIndex;

	INSTANCE_INDEX *faceInstance;

	SCRIPT_INST currentAnimation;

	bool bInDynDef;
	U32 dynPos;
	SINGLE timer;

	IMeshCallback * callback;

	struct IModifier * modList;

	//MeshInstance
	MeshInstance();

	~MeshInstance();

	void Initialize(MeshArch * meshArch, struct IEngineInstance * engInst = NULL);

	INSTANCE_INDEX findInstanceFromArch(ARCHETYPE_INDEX baseArch);

	//IMeshInstance

	virtual IMeshArchetype * GetArchtype();

	virtual void Update(SINGLE dt);

	virtual void SetTransform(Transform & trans);

	virtual const Transform & GetTransform();

	virtual void GetHardPointTransform(U32 index, Transform & trans);

	virtual void Render();

	virtual void BeginDynamicDef();

	virtual void EndDynamicDef();

	virtual void DynDef_SetTex1(const SINGLE & u, const SINGLE & v);

	virtual void DynDef_SetTex2(const SINGLE & u, const SINGLE & v);

	virtual void DynDef_SetNormal(const Vector & normal);

	virtual void DynDef_SetColor(const U8 & red, const U8 & green,const U8 & blue,const U8 & alpha);

	virtual void DynDef_SetPos(const Vector & position);

	virtual bool ComputeHitTest(Vector * start, Vector * dir, HitDef & hitDef);

	virtual void PlayAnimation(const char * animName, bool bLooping);

	virtual void KillAnimations();

	virtual void SetCallback(IMeshCallback * newCallback);

	virtual void SetModifierList(struct IModifier * modList);

	virtual SINGLE GetBoundingRadius();

	virtual Vector GetRandomSurfacePos(U32 seed, struct IMaterial * filterMat);

	virtual INSTANCE_INDEX FindChild (const char * pathname);

	//Animation::IEventHandler
	virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator&);

	virtual void COMAPI on_finished(unsigned int channel_id, void * user_supplied);

	virtual void COMAPI on_loop(unsigned int channel_id, Transform & xform, void * user_supplied);
};

#endif