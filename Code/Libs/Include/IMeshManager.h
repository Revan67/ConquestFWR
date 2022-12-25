#ifndef IMESHMANAGER_H
#define IMESHMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IMeshManager.H                              //
//                                                                          //
//               COPYRIGHT (C) 2004 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Tmauer $
*/


/*

*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "DACOM.h"
#endif

#include <FileSys.h>

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "ENGINE.h"

#include "IANIM.h"

#ifndef _NO_VTABLE	
  #if (_MSC_VER < 1100)      // 1100 = VC++ 5.0
    #define _NO_VTABLE
  #else
    #define _NO_VTABLE __declspec(novtable)
  #endif
#endif

struct HitDef
{
	U32 faceGroup;
	Vector hitPosition;
	SINGLE hitDist;
};

#define INVALID_HARD_POINT (-1)

struct HardPointDef
{
	char name[32];
	U32 index;
	U32 archIndex;
};

struct AnimScriptEntry
{
	char name[32];
	U32 index;
	SINGLE durration;
};

struct AnimCueEntry
{
	char name[32];
	U32 index;
	SINGLE time;
};

struct IMeshCallback
{
	virtual void AnimationCue(struct IMeshInstance * meshInstance, const char * cueName) = 0;
};

struct IMaterial;

struct IMeshInstance;

struct _NO_VTABLE IMeshArchetype
{
	virtual ARCHETYPE_INDEX GetEngineArchtype() = 0;

	virtual SCRIPT_SET_ARCH GetAnimArchtype() = 0;

	virtual void AddRef() = 0;

	virtual void Release() = 0;

	virtual bool FindFirstHardpoint(HardPointDef & hp) = 0;

	virtual bool FindNextHardpoint(HardPointDef & hp) = 0;

	virtual U32 FindHardPointIndex(const char * name) = 0;

	virtual bool FindHardPontFromIndex(U32 index,HardPointDef & hp) = 0;

	virtual bool GetFirstAnimScript(AnimScriptEntry & entry) = 0;

	virtual bool GetNextAnimScript(AnimScriptEntry & entry) = 0;

	virtual bool GetFirstAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue) = 0;

	virtual bool GetNextAnimCue(AnimScriptEntry & entry, AnimCueEntry & cue) = 0;

	virtual SINGLE GetAnimationDurration(const char * animName) = 0;

	virtual U32 GetNumFaceGroups() = 0;

	virtual IMaterial * GetFaceGroupMaterial(U32 fgIndex) = 0;

	virtual void ResetRef() = 0;

	virtual void MeshOperationSlice(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh) = 0;

	virtual void MeshOperationCut(SINGLE sliceDelta, Vector sliceDir, IMeshInstance * targetMesh) = 0;
};

struct _NO_VTABLE IMeshInstance
{
	virtual IMeshArchetype * GetArchtype() = 0;

	virtual void Update(SINGLE dt) = 0;

	virtual void SetTransform(Transform & trans) = 0;

	virtual const Transform & GetTransform() = 0;

	virtual void GetHardPointTransform(U32 index, Transform & trans) =0;

	virtual void Render() = 0;

	virtual void BeginDynamicDef() = 0;

	virtual void EndDynamicDef() = 0;

	virtual void DynDef_SetTex1(const SINGLE & u, const SINGLE & v) =0;

	virtual void DynDef_SetTex2(const SINGLE & u, const SINGLE & v) =0;

	virtual void DynDef_SetNormal(const Vector & normal) =0;

	virtual void DynDef_SetColor(const U8 & red, const U8 & green,const U8 & blue,const U8 & alpha) = 0;

	virtual void DynDef_SetPos(const Vector & position) =0;

	virtual bool ComputeHitTest(Vector * start, Vector * dir, HitDef & hitDef) = 0;

	virtual void PlayAnimation(const char * animName, bool bLooping) = 0;

	virtual void KillAnimations() = 0;

	virtual void SetCallback(IMeshCallback * newCallback) = 0;

	virtual void SetModifierList(struct IModifier * modList) = 0;

	virtual SINGLE GetBoundingRadius() = 0;

	virtual Vector GetRandomSurfacePos(U32 seed, struct IMaterial * filterMat) = 0;

	virtual INSTANCE_INDEX FindChild (const char * pathname) = 0;
};

//--------------------------------------------------------------------------//
//------------------------IParticleManager Interface------------------------//
//--------------------------------------------------------------------------//

#define IID_IMeshManager MAKE_IID("IMeshManager",1)

struct DACOM_NO_VTABLE IMeshManager : public IDAComponent
{
	struct InitInfo
	{
		struct IEngine * ENGINE;
		struct IAnimation * ANIM;
		struct IFileSystem * OBJDIR;
		struct IRenderer * REND;
		struct IRenderPipeline * PIPE;
		struct ICamera * CAMERA;
		struct IHardpoint * HARDPOINT;
		struct IMaterialManager * MATMAN;
	};

	virtual void InitManager(InitInfo & info) = 0;

	virtual IMeshArchetype * CreateMeshArch(const char * filename) = 0;

	virtual IMeshArchetype * FindMeshArch(const char * filename) = 0;

	virtual IMeshInstance * CreateMesh(IMeshArchetype * arch, IEngineInstance * engInst = NULL) = 0;

	virtual IMeshInstance * CreateMesh(const char * filename, IEngineInstance * engInst = NULL) = 0;

	virtual IMeshInstance * CreateDynamicMesh(U32 numVerts, struct IMaterial * mat) = 0;

	virtual IMeshInstance * CreateRefMesh(IMeshInstance * source) = 0;

	virtual void DestroyMesh(IMeshInstance * mesh) = 0;

	virtual void ReleaseArch(IMeshArchetype * arch) = 0;
};

#endif
