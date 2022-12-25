// Author: Shaival Varma
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include "PCH.h"
#include <windows.h>

#include <Model.h>	// using <Model.h> instead of "Model.h" to keep it from confusing the file with the ROS version
#include "Engine.h"
#include "ITXMLib.h"
#include "FileSys.h"
#include "MatrixUtil.h"
#include "Misc.h"
#include "BaseCam.h"
#include "CodeMsg.h"
#include "DACompoundObject.h"
#include "RayMeshCollision.h"
#include "IntersectInfo.h"
#include "StringList.h"
#include "DABaseCamera.h"
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
INSTANCE_INDEX FindCameraInCompound(INSTANCE_INDEX instance, unsigned int& cameraIndex);
// --------------------------------------------------------------------------
bool IsCamera(INSTANCE_INDEX instance);
// --------------------------------------------------------------------------
const ROS::DAMotionObject* GetDAMotionObject(SCRIPT_INST scriptIdx)
{
	return reinterpret_cast<const ROS::DAMotionObject*>(scriptIdx + 1);		// Adding 1 since 0 is a valid SCRIPT_INST, but don't want to return a NULL pointer (indicating failure)
}
// --------------------------------------------------------------------------
INSTANCE_INDEX GetMotionObjectIndex(const ROS::DAMotionObject* dAMotionObject)
{
	return reinterpret_cast<SCRIPT_INST>(dAMotionObject) - 1;	// Subtracting 1: See GetDAScriptObject()
}
// --------------------------------------------------------------------------
DXDEF_ROS const ROS::DACompoundObject* __cdecl CompoundObjectCreate(const ROS::StringList& descriptionStrings)
{
	// Create character hierarchy and deformable mesh.
	ASSERT(descriptionStrings.GetStringCount() == 1);

	ROS::ROSString	filenameString = descriptionStrings.GetString(0);

	const char*	filename = filenameString.c_str();

	COMPTR<IFileSystem> fileSys;

	if (GR_OK != ENG->create_file_system (filename, fileSys))
	{	return GetDACompoundObject(INVALID_INSTANCE_INDEX);
	}
	

	INSTANCE_INDEX compoundObject = INVALID_INSTANCE_INDEX;

 	TXMLIB->load_library (fileSys);
	compoundObject = ENG->create_instance (filename, fileSys);

	if(compoundObject != INVALID_INSTANCE_INDEX)
	{	MODEL->update_tree(compoundObject);

		ENG->set_position(compoundObject, Vector(0, 0, 0));
	#if 0
		ENG->set_orientation(compoundObject, Quaternion(Vector(0, 1, 0), 3.14159 / 2));
	#else
		ENG->set_orientation(compoundObject, Quaternion(Vector(0, 1, 0), 0));
	#endif

	#if 0
		// The following 2 lines were moved to CharAppMain2() since they need a glRenderContext, 
		// and need to be set up for each context that the app creates
		GLuint txm = LoadTexture("C:\\Develop\\Projects\\ROS System\\shademap.bmp");
		glBindTexture(GL_TEXTURE_2D, txm);
	#endif

		SCRIPT_SET_ARCH	scripts = ANIM->create_script_set_arch(fileSys);

		ENG->set_user_data(compoundObject, scripts);
	}
	
	return GetDACompoundObject(compoundObject);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectGetPosition(const ROS::DACompoundObject* dACompoundObject, Vector& position)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if (objIdx != INVALID_INSTANCE_INDEX)
	{
		position = ENG->get_position(objIdx);
	}
	else
	{
		position = Vector(0, 0, 0);
		return;
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectGetOrientation(const ROS::DACompoundObject* dACompoundObject, ROS::Matrix& orientation)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if (objIdx != INVALID_INSTANCE_INDEX)
	{
		Matrix orient = ENG->get_orientation(objIdx);

		orientation.SetI(orient.get_i());
		orientation.SetJ(orient.get_j());
		orientation.SetK(orient.get_k());
	}
	else
	{
		orientation.SetIdentity();	// Identity
	}

}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectSetPosition(const ROS::DACompoundObject* dACompoundObject, const Vector& position)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if (objIdx != INVALID_INSTANCE_INDEX)
	{
		ENG->set_position(objIdx, position);
		MODEL->update_tree(objIdx);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectSetOrientation(const ROS::DACompoundObject* dACompoundObject, const ROS::Matrix& orientation)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if (objIdx != INVALID_INSTANCE_INDEX)
	{
		Matrix orient(orientation.GetI(), orientation.GetJ(), orientation.GetK());
		
		ENG->set_orientation(objIdx, Quaternion(orient));
		MODEL->update_tree(objIdx);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectSetTransform(const ROS::DACompoundObject* dACompoundObject, const Transform& transform)
{	
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if (objIdx != INVALID_INSTANCE_INDEX)
	{
		ENG->set_transform(objIdx, transform);
		MODEL->update_tree(objIdx);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectRenderObject(const ROS::DACompoundObject* dACompoundObject, const ROS::DABaseCamera* camera)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	ENG->render_instance (GetBaseCamera(camera), objIdx);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CompoundObjectDestroy(const ROS::DACompoundObject* dACompoundObject)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);
	ASSERT(objIdx != INVALID_INSTANCE_INDEX);

	SCRIPT_SET_ARCH	scripts = ENG->get_user_data(objIdx);

	if(scripts != INVALID_SCRIPT_SET_ARCH)
	{
		ANIM->release_script_set_arch (scripts);
	}

	ENG->destroy_instance(objIdx);
}
// --------------------------------------------------------------------------
void EnumerationCallback(const char* script_name, void* motionList)
{
	ROS::StringList*	strings = reinterpret_cast<ROS::StringList*>(motionList);
	ASSERT(strings);

	strings->Add(script_name);
}
// --------------------------------------------------------------------------
DXDEF void CompoundObjectGetMotionNames(const ROS::DACompoundObject* dACompoundObject, ROS::StringList& motionNames)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	SCRIPT_SET_ARCH	scripts = ENG->get_user_data(objIdx);

	if(scripts != INVALID_SCRIPT_SET_ARCH)
	{
		ANIM->enumerate_scripts(EnumerationCallback, scripts, &motionNames);
	}
}
// --------------------------------------------------------------------------
DXDEF const ROS::DAMotionObject* __cdecl CompoundObjectCreateMotionObject(const ROS::DACompoundObject* dACompoundObject, const ROS::ROSString& motionName)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	SCRIPT_SET_ARCH	scripts = ENG->get_user_data(objIdx);

	if(scripts != INVALID_SCRIPT_SET_ARCH)
	{
		SCRIPT_INST	scriptInstance = ANIM->create_script_inst (scripts, objIdx, motionName.c_str());
	
		return GetDAMotionObject(scriptInstance);
	}
	else
	{
		return NULL;
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl MotionObjectDestroy(const ROS::DAMotionObject* dAMotionObject)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		ANIM->release_script_inst (scriptInstance);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl MotionObjectStart(const ROS::DAMotionObject* dAMotionObject, bool loop, float startTime, float transition)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		unsigned int	flags = Animation::FORWARD | Animation::NO_XLAT_OFFSET;

		if(loop)
		{
			flags |= Animation::LOOP;
		}

		ANIM->script_start(scriptInstance, flags, startTime, 1.0, transition);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl MotionObjectStop(const ROS::DAMotionObject* dAMotionObject)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		ANIM->script_stop (scriptInstance);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl MotionObjectPause(const ROS::DAMotionObject* dAMotionObject)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		ANIM->script_stop (scriptInstance);
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl MotionObjectResume(const ROS::DAMotionObject* dAMotionObject)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		ANIM->script_start (scriptInstance);
	}
}
// --------------------------------------------------------------------------
DXDEF float __cdecl MotionObjectGetDuration(const ROS::DAMotionObject* dAMotionObject)
{
	SCRIPT_INST	scriptInstance = GetMotionObjectIndex(dAMotionObject);

	if(scriptInstance != INVALID_SCRIPT_INST)
	{
		return ANIM->get_duration(scriptInstance);
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CompoundObjectLoadTextures(const char * filename)
{
	// Create character hierarchy and deformable mesh.
	COMPTR<IFileSystem> fileSys;

	if (GR_OK == ENG->create_file_system (filename, fileSys))
	{ 	TXMLIB->load_library (fileSys);
	}
}
// --------------------------------------------------------------------------
bool MeshIntersect(INSTANCE_INDEX instIndex, const Vector& rayStart, const Vector& rayDirection, float* distance)
{
	if(instIndex != INVALID_INSTANCE_INDEX)
	{	bool	collided = false;
		float	currDistance, shortestDistance;
		Mesh*	mesh = RENDERER->get_instance_mesh(instIndex);
		
		if(mesh)
		{	Vector	position = ENG->get_position(instIndex);
			Matrix	orientation = ENG->get_orientation(instIndex);
			
			if(TRUE == collide_ray_with_mesh(rayStart, rayDirection, position, orientation, mesh, NULL, NULL, &currDistance))
			{	if(currDistance > 0)
				{	collided = true;
					shortestDistance = currDistance;
				}
			}
		}

		INSTANCE_INDEX	childIndex = MODEL->get_child (instIndex, INVALID_INSTANCE_INDEX);

		while(childIndex != INVALID_INSTANCE_INDEX)
		{	if(MeshIntersect(childIndex, rayStart, rayDirection, &currDistance))
			{	if(collided)
				{	if(currDistance < shortestDistance)
					{	shortestDistance = currDistance;
					}
				}
				else
				{	collided = true;
					shortestDistance = currDistance;						
				}
			}

			childIndex = MODEL->get_child (instIndex, childIndex);
		}

		if(collided)
		{	*distance = shortestDistance;
			return true;
		}
		else
		{	return false;
		}
	}
	else
	{	return false;
	}
}
// --------------------------------------------------------------------------
DXDEF bool __cdecl CompoundObjectIntersect(const ROS::DACompoundObject* dACompoundObject, const ROS::IntersectInfo& intersectInfo, float* distance)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	return MeshIntersect(objIdx, intersectInfo.GetRayStart(), intersectInfo.GetRayDirection(), distance);
}
// --------------------------------------------------------------------------
DXDEF unsigned int CamerasGetCount(const ROS::DACompoundObject* dACompoundObject)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	unsigned int cameraCount = 0;
		unsigned int cameraIndex = 0;

		while(FindCameraInCompound(objIdx, cameraIndex) != INVALID_INSTANCE_INDEX)
		{	++cameraCount;
			cameraIndex = cameraCount;
		}
		
		return cameraCount;
	}
	else
	{	return 0;
	}
}
// --------------------------------------------------------------------------
DXDEF ROS::ROSString CamerasGetCameraName(const ROS::DACompoundObject* dACompoundObject, unsigned int idx)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	ASSERT(idx <= CamerasGetCount(dACompoundObject));

		unsigned int	index = idx;
		INSTANCE_INDEX	cameraIdx = FindCameraInCompound(objIdx, index);
		ASSERT(cameraIdx != INVALID_INSTANCE_INDEX);
		
		COMPTR<ICamera> camera;
		GENRESULT		result;

		result = ENG->query_interface(cameraIdx, "ICamera", camera);
		ASSERT(result == GR_OK);

#if 0
		return camera->get_name();	// NOTE: this is not available at this point
#else
		ROS::ROSString	name("Camera");

		char	indexStr[20];

		_itoa(idx, indexStr, 10);

		name = name + " " + indexStr;

		return name;
#endif


	}
	else
	{	return "Invalid Camera";
	}
}
// --------------------------------------------------------------------------
DXDEF void CamerasGetCameraPosition(const ROS::DACompoundObject* dACompoundObject, unsigned int idx, Vector& position)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	ASSERT(idx <= CamerasGetCount(dACompoundObject));

		unsigned int	index = idx;
		INSTANCE_INDEX	cameraIdx = FindCameraInCompound(objIdx, index);
		ASSERT(cameraIdx != INVALID_INSTANCE_INDEX);
		
		COMPTR<ICamera> camera;
		GENRESULT		result;

		result = ENG->query_interface(cameraIdx, "ICamera", camera);
		ASSERT(result == GR_OK);

		position = camera->get_position();
	}
	else
	{	position.zero();
	}
}
// --------------------------------------------------------------------------
DXDEF void CamerasGetCameraOrientation(const ROS::DACompoundObject* dACompoundObject, unsigned int idx, ROS::Matrix& orientation)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	ASSERT(idx <= CamerasGetCount(dACompoundObject));

		unsigned int	index = idx;
		INSTANCE_INDEX	cameraIdx = FindCameraInCompound(objIdx, index);
		ASSERT(cameraIdx != INVALID_INSTANCE_INDEX);
	
		COMPTR<ICamera> camera;
		GENRESULT		result;

		result = ENG->query_interface(cameraIdx, "ICamera", camera);
		ASSERT(result == GR_OK);

		Matrix orient = camera->get_transform().get_orientation();

		orientation.SetI(orient.get_i());
		orientation.SetJ(orient.get_j());
		orientation.SetK(orient.get_k());
	}
	else
	{	orientation.SetIdentity();
	}
}
// --------------------------------------------------------------------------
DXDEF float CamerasGetCameraHorizontalFOV(const ROS::DACompoundObject* dACompoundObject, unsigned int idx)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	ASSERT(idx <= CamerasGetCount(dACompoundObject));

		unsigned int	index = idx;
		INSTANCE_INDEX	cameraIdx = FindCameraInCompound(objIdx, index);
		ASSERT(cameraIdx != INVALID_INSTANCE_INDEX);
		
		COMPTR<ICamera> camera;
		GENRESULT		result;

		result = ENG->query_interface(cameraIdx, "ICamera", camera);
		ASSERT(result == GR_OK);

		return camera->get_fovx();
	}
	else
	{	return 35.0;
	}
}
// --------------------------------------------------------------------------
DXDEF float CamerasGetCameraVerticalFOV(const ROS::DACompoundObject* dACompoundObject, unsigned int idx)
{
	INSTANCE_INDEX	objIdx = GetCompoundObjectIndex(dACompoundObject);

	if(objIdx != INVALID_INSTANCE_INDEX)
	{	ASSERT(idx <= CamerasGetCount(dACompoundObject));

		unsigned int	index = idx;
		INSTANCE_INDEX	cameraIdx = FindCameraInCompound(objIdx, index);
		ASSERT(cameraIdx != INVALID_INSTANCE_INDEX);
		
		COMPTR<ICamera> camera;
		GENRESULT		result;

		result = ENG->query_interface(cameraIdx, "ICamera", camera);
		ASSERT(result == GR_OK);

		return camera->get_fovy();
	}
	else
	{	return 35.0;
	}
}
// --------------------------------------------------------------------------
INSTANCE_INDEX FindCameraInCompound(INSTANCE_INDEX instance, unsigned int& cameraIndex)
{
	while(instance != INVALID_INSTANCE_INDEX) 
	{	if(IsCamera(instance))
		{	if(cameraIndex == 0)
			{	return instance;
			}
			else
			{	--cameraIndex;
			}			
		}
		
		INSTANCE_INDEX child = MODEL->get_child(instance, INVALID_INSTANCE_INDEX);

		while(child != INVALID_INSTANCE_INDEX)
		{	INSTANCE_INDEX	cameraInstIdx = FindCameraInCompound(child, cameraIndex);
			
			if(cameraInstIdx != INVALID_INSTANCE_INDEX)
			{	if(cameraIndex == 0)
				{	return cameraInstIdx;
				}
				else
				{	--cameraIndex;
				}
			}

			child = MODEL->get_child(instance, child);
		}
			
		return INVALID_INSTANCE_INDEX;
	}

	return INVALID_INSTANCE_INDEX;
}
// --------------------------------------------------------------------------
bool IsCamera(INSTANCE_INDEX instance)
{
	COMPTR<ICamera> camera;

	return (GR_OK == ENG->query_interface (instance, "ICamera", camera));
}
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
