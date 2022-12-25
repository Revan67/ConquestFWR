#ifndef IPARTICLEMANAGER_H
#define IPARTICLEMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IParticleManager.H                          //
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

#include "xform.h"
#include <FileSys.h>

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "ParticleParameters.h"

#ifndef _NO_VTABLE	
  #if (_MSC_VER < 1100)      // 1100 = VC++ 5.0
    #define _NO_VTABLE
  #else
    #define _NO_VTABLE __declspec(novtable)
  #endif
#endif

enum ParticleEffectType
{
	//emmiters
	PE_POINT_EMMITER,
	PE_FIXED_EMMITER,

	//initizers
	PE_INITIAL_VELOCITY,
	PE_INITIAL_DIRECTED_VELOCITY,

	PE_CYLINDER_SHAPER,
	PE_BOX_SHAPER,
	PE_SPHERE_SHAPER,
	PE_TARGET_SHAPER,

	//motion controlers
	PE_MOTION_INTERP,
	PE_ACCELERATION,
	PE_DIRECTIONAL_GRAVITY,
	PE_POINT_GRAVITY,
	PE_WIND,//not supported yet

	//particle output
	PE_RENDER_BILBOARD,
	PE_RENDER_MESH,
	PE_SOUND,//not supported yet
	PE_LIGHT,//not supported yet

	//mesh instance
	PE_MESH_INST,
	PE_MESH_REF,
	PE_MESH_SLICE,
	PE_MESH_CUT,
	PE_MESH_CHUNK,//not supported yet
	PE_MESH_DISTORT,//not supported yet

	//utility
	PE_SHUTDOWN_TIMER,
	PE_SPLITTER,//not supported yet
	PE_EVENT,//not supported yet
	PE_TIMED_SWITCH,//not supported yet
	PE_SPAWNER,
	PE_LOGIC_SWITCH,//not supported yet
	PE_POSITION_CALLBACK,//not supported yet

	//event gentators
	PE_OBJECT_COLLISION,//not supported yet
	PE_SPHERE_HITTER,
	PE_DISTANCE_DELTA_HITTER,

	PE_PEVENT_EMMITER,
	PE_PEVENT_KILLER,
	PE_PEVENT_EXPLOSION,//not supported yet
	PE_CAMERA_SHAKE,

	//material mods
	PE_MATMOD_COLOR,
	PE_MATMOD_FLOAT,

	PE_COPY_POINTS,
	PE_CAMERA_SORT,

};

struct IMeshInstance;
///////////////////////////////////////////////////////////////
struct _NO_VTABLE IGamePositionCallback
{
	virtual bool GetObjectTransform(U32 objectID, U32 hardpoint, DWORD context, Transform & trans) = 0;

	virtual bool GetObjectTransform(U32 objectID, const char * hpName, DWORD context, Transform & trans) = 0;

	virtual SINGLE GetParameter(const char * name, U32 objectID,DWORD context) = 0;

	virtual void ObjectPostionCallback(U32 objectID,DWORD context, char * string, Transform & trans) = 0;

	virtual bool TestCollision(Vector p1,Vector p2,Vector & collisionPoint,Vector & finalPoint,bool bTerrain,bool bTinkerToys,bool bWater,bool bUnits) = 0;

	virtual void ShakeCamera(SINGLE durration, SINGLE power) = 0;

	virtual IMeshInstance * GetObjectMesh(U32 objectID, DWORD context) = 0;
};

struct _NO_VTABLE IParticleProgramer
{
	virtual void SetEffectName(const char * name) = 0;

	virtual const char * GetEffectName() = 0;

	virtual U32 GetNumOutput() = 0;

	virtual const char * GetOutputName(U32 index) = 0;

	virtual U32 GetNumInput() = 0;

	virtual const char * GetInputName(U32 index) = 0;

	virtual U32 GetNumFloatParams() = 0;

	virtual const char * GetFloatParamName(U32 index) = 0;

	virtual const FloatType * GetFloatParam(U32 index) = 0;

	virtual void SetFloatParam(U32 index,const FloatType * param) = 0;

	virtual U32 GetNumTransformParams() = 0;

	virtual const char * GetTransformParamName(U32 index) = 0;

	virtual const TransformType * GetTransformParam(U32 index) = 0;

	virtual void SetTransformParam(U32 index,const TransformType * param) = 0;

	virtual U32 GetNumStringParams() = 0;

	virtual const char * GetStringParamName(U32 index) = 0;

	virtual const char * GetStringParam(U32 index) = 0;

	virtual void SetStringParam(U32 index,const char * param) = 0;

	virtual U32 GetNumEnumParams() = 0;

	virtual const char * GetEnumParamName(U32 index) = 0;

	virtual U32 GetNumEnumValues(U32 index) = 0;

	virtual const char * GetEnumValueName(U32 index, U32 value) = 0;

	virtual U32 GetEnumParam(U32 index) = 0;

	virtual void SetEnumParam(U32 index,U32 value) = 0;

	virtual U32 GetNumTargetParams() = 0;

	virtual const char * GetTargetParamName(U32 index) = 0;

	virtual const U32 GetTargetParam(U32 index) = 0;

	virtual void SetTargetParam(U32 index,U32 param) = 0;

	virtual U32 GetDataChunkSize() = 0;

	virtual void GetDataChunk(U8 * buffer) = 0;

	virtual void SetDataChunk(U8 * buffer) = 0;
};

struct _NO_VTABLE IParticleEffectInstance
{
	virtual IParticleEffectInstance * AddFilter(U32 outputID,U32 inputID, ParticleEffectType type, U8 * buffer) = 0;

	virtual void LinkFilter(U32 outputID,U32 inputID, IParticleEffectInstance * target) = 0;

	virtual U32 GetAllocationStart(IParticleEffectInstance * target, U32 instance) = 0;

	virtual void GetTransform(SINGLE t,Transform & trans,U32 instance) = 0;
};

struct _NO_VTABLE IParticleListener
{
	virtual void ParticalEvent(const char * eventName) = 0;
};

struct _NO_VTABLE IParticleInstance
{
	virtual void SetContext(DWORD _context);

	virtual void Initialize(DWORD _context) = 0;

	virtual void Render(bool bVisible=true) = 0;

	virtual bool GetVisInfo(Vector & center, SINGLE & radius) =0;

	virtual IParticleEffectInstance * AddFilter(ParticleEffectType type, U8 * buffer) = 0;

	virtual void Shutdown() = 0;

	virtual bool IsFinished() = 0;

	virtual void Event(const char * eventName) = 0;

	virtual void AddParticalListener(IParticleListener * listener) = 0;

	virtual void RemoveParticalListener(IParticleListener * listener) = 0;
};

//--------------------------------------------------------------------------//
//------------------------IParticleManager Interface------------------------//
//--------------------------------------------------------------------------//

#define IID_IParticleManager MAKE_IID("IParticleManager",1)

struct DACOM_NO_VTABLE IParticleManager : public IDAComponent
{
	struct InitInfo
	{
		struct ICamera* camera;
		struct IGamePositionCallback * posCallback;
		struct IMaterialManager * MATMAN;
		struct IMeshManager * MESHMAN;
	};

	virtual void InitParticles(InitInfo & info) = 0;

	virtual IParticleInstance * CreateBaseParticleInstance() = 0;

	virtual IParticleProgramer * CreateParticleProgramer(ParticleEffectType type) = 0;

	virtual void ReleaseInstance(IParticleInstance * inst) = 0;

	virtual void ReleaseInstance(IParticleProgramer * inst) = 0;

	virtual void Update(SINGLE time) = 0;

	virtual void Close() = 0;

	virtual const char * GetFilterName(ParticleEffectType pType) = 0;

	virtual U32 GetFilterNumber() = 0;

	virtual ParticleEffectType GetFilterEnum(U32 number) = 0;

	virtual void SaveFloatType(IFileSystem * outFile,FloatType * target) = 0;

	virtual FloatType * LoadFloatType(IFileSystem * inFile) = 0;

	virtual void SaveTransType(IFileSystem * outFile,TransformType * target) = 0;

	virtual TransformType * LoadTransType(IFileSystem * inFile) = 0;
};

#endif
