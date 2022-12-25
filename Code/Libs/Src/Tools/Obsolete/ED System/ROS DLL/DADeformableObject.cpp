// Author: Shaival Varma
// --------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include "PCH.h"
#include <windows.h>
//

#include <stdio.h>
#include <stdlib.h>
#include <vector>

/*****NOTE: Explicit include to avoid confusion with Model.h in ROS dll******/
#include <Model.h>
#include "DACom.h"
#include "3DMath.h"
#include "GameSys.h"
#include "HeapObj.h"
#include "IAnim.h"
#include "ITXMLib.h"
#include "LightMan.h"
#include "FileSys.h"
//#include "text.h"
//#include "bitmap.h"
#include "TimeTagData.h"
#include "deform.h"
#include "PersistChannel.h"
#include "MatrixUtil.h"
#include "Matrix4x4.h"
#include "Misc.h"
#include "DABaseCamera.h"
#include "BaseCam.h"
#include "DADeformableObject.h"
#include "CodeMsg.h"
#include "IntersectInfo.h"
#include "StringList.h"
#include "StringType.h"
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "IHardPoint.h"
#include "IKState.h"
// --------------------------------------------------------------------------
//
// Globals.
//

CHANNEL_ARCHETYPE_INDEX g_channel_arch_idx = INVALID_CHANNEL_ARCHETYPE_INDEX;
// --------------------------------------------------------------------------
struct EventHandler: public Channel::IEventHandler
{
	virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator)
	{
		((ROS::EventHandler)user_supplied)(channel_id, event_iterator);
	}

	virtual void COMAPI on_finished(unsigned int channel_id, void * user_supplied)
	{
	}

	virtual void COMAPI on_loop(unsigned int channel_id, Transform & xform, void * user_supplied)
	{
	}
};

EventHandler	gEventHandler;

#if 0
// --------------------------------------------------------------------------
unsigned int LoadTexture(const char * filename)
{
	unsigned int result = 0;

	_BITMAP bmp;
	if (BMP_load(bmp, filename))
	{
		glGenTextures(1, &result);

		glBindTexture(GL_TEXTURE_2D, result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Choose nearest mipmap, bilinear filter within mipmap.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if 1
 		glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 16, GL_RGB, GL_UNSIGNED_BYTE, (GLubyte *) bmp.palette);
#else
		glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, (GLubyte *) bmp.palette);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, bmp.width, bmp.height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bmp.pixels);

		free(bmp.pixels);
		free(bmp.palette);
	}

	return result;
}
#endif
// --------------------------------------------------------------------------
const ROS::DAIK* GetDAIK(HANDLE handle)
{
	return reinterpret_cast<const ROS::DAIK*>(handle);
}
// --------------------------------------------------------------------------
HANDLE GetHandle(const ROS::DAIK* dAIK)
{
	return reinterpret_cast<HANDLE>(const_cast<ROS::DAIK*>(dAIK));
}
// --------------------------------------------------------------------------
const INSTANCE_INDEX GetPart(INSTANCE_INDEX root, const char* partName)
{
	ASSERT(MODEL);

	if(root == INVALID_INSTANCE_INDEX)
	{
		return INVALID_INSTANCE_INDEX;
	}

	const char* name = MODEL->get_name(root);

	if(name != NULL)
	{
		if(!strcmp(partName, name))
		{
			// Found it!
			return root;
		}
	}

	// Traverse the children
	INSTANCE_INDEX	child = MODEL->get_child(root);
	while (child != INVALID_INSTANCE_INDEX)
	{
		const INSTANCE_INDEX	partIndex = GetPart(child, partName);

		if(partIndex != INVALID_INSTANCE_INDEX)
		{
			// Found it!
			return partIndex;
		}

		child = MODEL->get_child(root, child);
	}

	// Not found!
	return INVALID_INSTANCE_INDEX;
}
// --------------------------------------------------------------------------
const INSTANCE_INDEX GetAncestor(INSTANCE_INDEX part, unsigned int ancestorGenerations)
{
	while(ancestorGenerations > 0 && INVALID_INSTANCE_INDEX != (part = MODEL->get_parent(part)))
	{
		--ancestorGenerations;
	}

	return part;
}
// --------------------------------------------------------------------------
class ScriptArchetypeInfo
{
	public:
		ScriptArchetypeInfo(const ROS::ROSString& filename, SCRIPT_SET_ARCH scriptSetArchetype)
		:mFilename(filename), mScriptSetArchetype(scriptSetArchetype), mReferenceCount(0)
		{
		}

		ROS::ROSString	GetFilename() const { return mFilename; }
		SCRIPT_SET_ARCH	GetScriptSetArchetype() const { return mScriptSetArchetype; }

		void			IncrementReferenceCount() { ++mReferenceCount; }
		void			DecrementReferenceCount() { --mReferenceCount; }

		int				GetReferenceCount() const { return mReferenceCount; }

	private:
		ROS::ROSString	mFilename;
		SCRIPT_SET_ARCH	mScriptSetArchetype;
		int				mReferenceCount;
};

typedef std::vector<ScriptArchetypeInfo>	ArchetypeInfoCollection;
// --------------------------------------------------------------------------
static ArchetypeInfoCollection gScriptArchetypeInfo;
// --------------------------------------------------------------------------
static SCRIPT_SET_ARCH GetAnimationScriptSet(const ROS::ROSString& filename)
{	
	// First check if an archetype is available for the file
	ArchetypeInfoCollection::iterator				begin = gScriptArchetypeInfo.begin();
	const ArchetypeInfoCollection::const_iterator	end = gScriptArchetypeInfo.end();

	while(begin != end)
	{
		if(begin->GetFilename() == filename)
		{
			// Found a match!
			begin->IncrementReferenceCount();

			return begin->GetScriptSetArchetype();
		}

		++begin;
	}

	// No archetype for the specified file.
	// Create a new archetype and store it.
	COMPTR<IFileSystem> fileSys;

	if(ENG->create_file_system(filename.c_str(), fileSys) != GR_OK)
	{
		throw ExAnimationArchetypeCreationFailure(filename);
	}

	SCRIPT_SET_ARCH	archetype = ANIM->create_script_set_arch(fileSys);
	
	if(archetype == INVALID_SCRIPT_SET_ARCH)
	{
		throw ExAnimationArchetypeCreationFailure(filename);
	}

	// We have a valid archetype. Time to store it.
	try
	{
		gScriptArchetypeInfo.push_back(ScriptArchetypeInfo(filename, archetype));
	}
	catch(...)
	{
		ANIM->release_script_set_arch(archetype);
		throw;
	}
	
	gScriptArchetypeInfo.back().IncrementReferenceCount();

	return archetype;
}
// --------------------------------------------------------------------------
static void RemoveAnimationScriptSet(const ROS::ROSString& filename)
{	
	// First check if an archetype is available for the file
	ArchetypeInfoCollection::iterator				begin = gScriptArchetypeInfo.begin();
	const ArchetypeInfoCollection::const_iterator	end = gScriptArchetypeInfo.end();

	while(begin != end)
	{
		if(begin->GetFilename() == filename)
		{
			// Found a match!
			begin->DecrementReferenceCount();

			if(begin->GetReferenceCount() == 0)
			{
				// No more users!
				// Release the script and remove the entry from the collection
				ANIM->release_script_set_arch(begin->GetScriptSetArchetype());

				gScriptArchetypeInfo.erase(begin);
			}

			return;
		}

		++begin;
	}

	ASSERT(0 && "Removing an animation script set that is not in the collection");
}
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
bool __cdecl ROSSystemStartup(IDAComponent* system, IEngine* engine, HWND wndH)
{
	ASSERT(system && engine && wndH && !ENG && !MODEL && !LIGHT && !ANIM);

	engine->QueryInterface(IID_IEngine,			(void **) &ENG);
//	engine->QueryInterface(IID_IModel,			(void **) &MODEL);
	engine->QueryInterface("ILightManager",		(void **) &LIGHT);
	engine->QueryInterface("IAnimation",		(void **) &ANIM);
	engine->QueryInterface(IID_IHardpoint,		(void **) &HARDPOINT);
	engine->QueryInterface(IID_IModel,			(void **) &MODEL);

	const bool	deformOpened = DeformOpen(system, ENG, NULL);

	if(ENG && MODEL && LIGHT && deformOpened && ANIM && HARDPOINT && MODEL)
	{
		return true;
	}
	else
	{
		ROSSystemShutdown();

		return false;
	}
}
// --------------------------------------------------------------------------
void __cdecl ROSSystemShutdown()
{
	DeformClose();

	if(MODEL)
	{
		MODEL->Release();
		MODEL = NULL;
	}

	if(HARDPOINT)
	{
		HARDPOINT->Release();
		HARDPOINT = NULL;
	}

	if(ANIM)
	{
		ANIM->Release();
		ANIM = NULL;
	}

	if(LIGHT)
	{
		LIGHT->Release();
		LIGHT = NULL;
	}

	if(ENG)
	{
		ENG->Release();
		ENG = NULL;
	}
}
// --------------------------------------------------------------------------
CPP_DEFN const ROS::DADeformableObject* __cdecl DeformableObjectCreate(const ROS::StringList& descriptionStrings, ROS::EventHandler EventHandlerFunction)
{
	const unsigned int	stringCount = descriptionStrings.GetStringCount();
	ASSERT((stringCount % 4) == 0);

	const unsigned int numParts = stringCount / 4;

	const char**			entityDescriptionStrings = NULL;
	ROS::ROSString*			descriptionROSStrings = NULL;
	COMPTR<IFileSystem>*	skeletonFileSystem = NULL;
	DeformPartDesc*			defPartDescs = NULL;
	DeformPartMeshDesc*		defPartMeshDescs = NULL;

	try
	{
		entityDescriptionStrings = new const char*[stringCount];
		descriptionROSStrings = new ROS::ROSString[stringCount];	

		for(unsigned int stringIdx = 0; stringIdx < stringCount; ++stringIdx)
		{
			descriptionROSStrings[stringIdx] = descriptionStrings.GetString(stringIdx);
			entityDescriptionStrings[stringIdx] = descriptionROSStrings[stringIdx].c_str();
		}

		skeletonFileSystem = new COMPTR<IFileSystem>[numParts];
		defPartDescs = new DeformPartDesc[numParts];
		defPartMeshDescs = new DeformPartMeshDesc[numParts];

		for(unsigned int partIdx = 0; partIdx < numParts; ++partIdx)
		{
			GENRESULT	result = ENG->create_file_system(entityDescriptionStrings[(partIdx * 4) + 2], skeletonFileSystem[partIdx], NULL);

			if(result != GR_OK)
			{
				delete[] entityDescriptionStrings;
				delete[] descriptionROSStrings;
				delete[] skeletonFileSystem;
				delete[] defPartDescs;
				delete[] defPartMeshDescs;
			
				return NULL;
			}

			defPartMeshDescs[partIdx].mesh_parent = NULL;
			defPartMeshDescs[partIdx].mesh_name = entityDescriptionStrings[(partIdx * 4) + 1];
		
			defPartDescs[partIdx].num_meshes = 1;
			defPartDescs[partIdx].meshes = &(defPartMeshDescs[partIdx]);
			defPartDescs[partIdx].skeleton_parent = skeletonFileSystem[partIdx];
			defPartDescs[partIdx].anim_script_set = GetAnimationScriptSet(entityDescriptionStrings[(partIdx * 4) + 3]);
		}
	}
	catch(...)
	{
		delete[] entityDescriptionStrings;
		delete[] descriptionROSStrings;
		delete[] skeletonFileSystem;
		delete[] defPartDescs;
		delete[] defPartMeshDescs;

		return NULL;
	}

	DeformDesc defDesc;
	defDesc.num_parts = numParts;
	defDesc.parts = defPartDescs;

	DeformableObject*	object;

	try 
	{
		object = new DeformableObject;

		if(!object->create(defDesc, &gEventHandler, EventHandlerFunction))
		{
			delete object;
			delete[] defPartDescs;
			delete[] entityDescriptionStrings;
			delete[] descriptionROSStrings;
			delete[] skeletonFileSystem;
			delete[] defPartMeshDescs;

			return NULL;
		}

		delete[] defPartDescs;	
		delete[] defPartMeshDescs;
		delete[] entityDescriptionStrings;
		delete[] descriptionROSStrings;
		delete[] skeletonFileSystem;
	}
	catch(...)
	{
		delete[] defPartDescs;
		delete[] defPartMeshDescs;
		delete[] entityDescriptionStrings;
		delete[] descriptionROSStrings;
		delete[] skeletonFileSystem;

		return NULL;
	}

	object->set_position(Vector(0, 0, 0));
#if 0
	object->set_orientation(Quaternion(Vector(0, 1, 0), 3.14159 / 2));
#else
	object->set_orientation(Quaternion(Vector(0, 1, 0), 0));
#endif
	object->set_floor_height(0);
	
#if 0
	// The following 2 lines were moved to CharAppMain2() since they need a glRenderContext, 
	// and need to be set up for each context that the app creates
	GLuint txm = LoadTexture("C:\\Develop\\Projects\\ROS System\\shademap.bmp");
	glBindTexture(GL_TEXTURE_2D, txm);
#endif

	return GetDADeformableObject(object);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectLoadTextures(const char * filename)
{
	// Create character hierarchy and deformable mesh.
	COMPTR<IFileSystem> fileSys;

	if (GR_OK == ENG->create_file_system (filename, fileSys))
	{
		TXMLIB->load_library (fileSys);
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectAddArchetypeTimeTagChannel(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName)
{
	if(!motionName.empty())
	{
		DeformableObject* object = GetDeformableObject(defObj);

		g_channel_arch_idx = object->add_channel(motionName.c_str(), "TimeTag Channel");
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectReplaceArchetypeTimeTagChannelData(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName, float time[], int tag[], unsigned int count)
{
	if(!motionName.empty())
	{
		DeformableObject*	object = GetDeformableObject(defObj);
		IChannel::Header	header;

		header.frames = count;
		header.capture_rate = -1;
		header.type = PersistDT_EVENT;

		void* data = TimeTagData::create_time_tag_data(time, tag, count);

		float duration = count > 0 ? time[count - 1] : 0.0;

		object->replace_channel_data(motionName.c_str(), g_channel_arch_idx, header, data, 8, duration);
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectRender(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera)
{
	DeformableObject* object = GetDeformableObject(defObj);

	int*	meshList = new int[object->num_parts];

	for(unsigned int idx = 0; idx < object->num_parts; ++idx)
	{
		meshList[idx] = 0;
	}
	
	try
	{
		object->deform(meshList);

		object->render(GetBaseCamera(camera), meshList);
	}
	catch(...)
	{
		delete[] meshList;
		throw;
	}

	delete[] meshList;
}
// --------------------------------------------------------------------------
CPP_DEFN void __cdecl DeformableObjectDestroy(const ROS::DADeformableObject* defObj, const ROS::StringList& descriptionStrings)
{
	// Important to delete the DeformableObject before freeing the script archetypes
	// it uses.
	DeformableObject* object = GetDeformableObject(defObj);
	delete object;

	const unsigned int	stringCount = descriptionStrings.GetStringCount();
	ASSERT((stringCount % 4) == 0);

	const unsigned int numParts = stringCount / 4;

	for(unsigned int partIdx = 0; partIdx < numParts; ++partIdx)
	{
		RemoveAnimationScriptSet(descriptionStrings.GetString((partIdx * 4) + 3));
	}
}
// --------------------------------------------------------------------------
unsigned int __cdecl DeformableObjectGetMotionCount(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	return object->get_script_count();
}
// --------------------------------------------------------------------------
ROS::ROSString __cdecl DeformableObjectGetMotionName(const ROS::DADeformableObject* defObj, int motionIdx)
{
	DeformableObject* object = GetDeformableObject(defObj);

	unsigned int	count = object->get_script_count();
	const char*		name = NULL;

	if(count > 0)
	{
		const char**	names = new const char*[count];

		if(object->get_scripts(names))
		{
			name = names[motionIdx];
		}

		delete[] names;
	}

	return ROS::ROSString(name);
}
// --------------------------------------------------------------------------
float __cdecl DeformableObjectGetMotionLength(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName)
{
	float		duration = 0;

	if(!motionName.empty())
	{
		DeformableObject* object = GetDeformableObject(defObj);

		duration = object->get_script_duration(motionName.c_str());
	}

	return duration;
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectStartMotion(const ROS::DADeformableObject* defObj, const ROS::ROSString& motionName, bool loop, float startTime, float transition)
{
	if(!motionName.empty())
	{
		DeformableObject* object = GetDeformableObject(defObj);
	
		unsigned int	flags = Animation::FORWARD | Animation::NO_XLAT_OFFSET;

		if(loop)
		{
			flags |= Animation::LOOP;
		}

		object->start_motion(motionName.c_str(), startTime, transition, 1.0, 1.0, flags);
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectStopMotion(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);
	
	object->stop_motion();
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectPauseMotion(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	object->pause();
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectResumeMotion(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	object->resume();
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectUpdate(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	ENG->update_instance(object->get_root(), 0.001);
	ENG->update_instance(object->get_root(), 0.001);
}
#if 0
// --------------------------------------------------------------------------
float	__cdecl DeformableObjectGetCurrentMotionTime(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	return object->get_current_time();
}
// --------------------------------------------------------------------------
bool __cdecl DeformableObjectIsMotionOver(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	return object->motion_is_done();
}
#endif
// --------------------------------------------------------------------------
void __cdecl DeformableObjectSetPosition(const ROS::DADeformableObject* defObj, const Vector& position)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		object->set_position(position);
		object->set_floor_height(position.y);
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectSetPositionViaEngine(const ROS::DADeformableObject* defObj, const Vector& position)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		ENG->set_position(object->get_root(), position);
		object->set_floor_height(position.y);
	}
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectSetPositionOnly(const ROS::DADeformableObject* defObj, const Vector& position)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		object->set_position(position);
	}
}
#if 0
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetPosition(const ROS::DADeformableObject* defObj, Vector& position)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		position = object->get_position();
	}
	else
	{
		position = Vector(0, 0, 0);
		return;
	}
}
#else
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetPosition(const ROS::DADeformableObject* defObj, Vector& position)
{
	DeformableObject* object = GetDeformableObject(defObj);
	INSTANCE_INDEX character = object->get_root();

	if (character == INVALID_INSTANCE_INDEX)
	{
		position = Vector(0, 0, 0);
		return;
	}

	position = ENG->get_position(character);
}
#endif
// --------------------------------------------------------------------------
void __cdecl DeformableObjectSetOrientation(const ROS::DADeformableObject* defObj, const ROS::Matrix& orientation)
{
	DeformableObject* object = GetDeformableObject(defObj);
	
	if(object)
	{
		Matrix orient(orientation.GetI(), orientation.GetJ(), orientation.GetK());
	
		object->set_orientation(Quaternion(orient));
	}
}
#if 0
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetOrientation(const ROS::DADeformableObject* defObj, ROS::Matrix& orientation)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		Matrix orient = object->get_orientation();

		orientation.SetI(orient.get_i());
		orientation.SetJ(orient.get_j());
		orientation.SetK(orient.get_k());
	}
	else
	{
		orientation.SetIdentity();	// Identity
	}
}
#else
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetOrientation(const ROS::DADeformableObject* defObj, ROS::Matrix& orientation)
{
	DeformableObject* object = GetDeformableObject(defObj);
	INSTANCE_INDEX character = object->get_root();

	if (character == INVALID_INSTANCE_INDEX)
	{
		orientation.SetIdentity();	// Identity

		return;
	}

	Matrix orient = ENG->get_orientation(character);

	orientation.SetI(orient.get_i());
	orientation.SetJ(orient.get_j());
	orientation.SetK(orient.get_k());
}
#endif
// --------------------------------------------------------------------------
float __cdecl DeformableObjectGetFloorHeight(const ROS::DADeformableObject* defObj)
{
	DeformableObject* object = GetDeformableObject(defObj);

	if (object)
	{
		return object->floor_height;
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
bool __cdecl DeformableObjectIntersect(const ROS::DADeformableObject* defObj, const ROS::IntersectInfo& intersectInfo, float* distance)
{
	DeformableObject*	object = GetDeformableObject(defObj);
	RECT				rect;
	/*******NOTE: the following stripping of const should go away*******/
	BaseCamera*			baseCam = const_cast<BaseCamera*>(intersectInfo.GetCamera());

	if(object->visible_rect(rect, baseCam))
	{
		const int	winX = intersectInfo.GetWindowX();
		const int	winY = intersectInfo.GetWindowY();

		if(rect.left <= winX && winX <= rect.right && rect.top <= winY && winY <= rect.bottom)
		{
			Vector	deformPosition;
			DeformableObjectGetPosition(defObj, deformPosition);

			const Vector	displacement(deformPosition - intersectInfo.GetRayStart());

			*distance = displacement.magnitude();

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
// --------------------------------------------------------------------------
unsigned int __cdecl DeformableObjectGetHardpointCount(const ROS::DADeformableObject* defObj)
{
	DeformableObject*	object = GetDeformableObject(defObj);
	ASSERT(object);

	return object->get_num_hardpoints();
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetHardPointName(const ROS::DADeformableObject* defObj, unsigned int index, ROS::ROSString& hardPointName)
{
	ASSERT(index < DeformableObjectGetHardpointCount(defObj));

	const DeformableObject*	object = GetDeformableObject(defObj);
	ASSERT(object);
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);

	hardPointName = ROS::ROSString(hardpoints[index].name);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetHardPointPosition(const ROS::DADeformableObject* defObj, unsigned int index, Vector& position)
{
	ASSERT(index < DeformableObjectGetHardpointCount(defObj));
	ASSERT(ENG && HARDPOINT);

	// Obtain the hardpoint position which is in the frame of the host instance (not necessarily the deformable object's root)
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);
	const INSTANCE_INDEX	instance = hardpoints[index].object;
	ASSERT(instance != INVALID_INSTANCE_INDEX);

	HardpointInfo	hardPointInfo;

	ARCHETYPE_INDEX arch = ENG->get_archetype(instance);
	const bool	hardPointInfoFound = HARDPOINT->retrieve_hardpoint_info(arch, hardpoints[index].name, hardPointInfo);
	ASSERT(hardPointInfoFound);
	ENG->release_archetype(arch);

	// Convert that position into the world frame
	const Transform	instanceTransform = ENG->get_transform(instance);
	
	position = instanceTransform * hardPointInfo.point;

	// Finally, convert to the position in the deformable objects frame
	const Transform	invDeformTransform = ENG->get_transform(object->get_root()).get_inverse();

	position = invDeformTransform * position;
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetHardPointOrientation(const ROS::DADeformableObject* defObj, unsigned int index, ROS::Matrix& orientation)
{
	ASSERT(index < DeformableObjectGetHardpointCount(defObj));
	ASSERT(ENG && HARDPOINT);

	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);
	const INSTANCE_INDEX	instance = hardpoints[index].object;
	ASSERT(instance != INVALID_INSTANCE_INDEX);

	HardpointInfo	hardPointInfo;

	ARCHETYPE_INDEX arch = ENG->get_archetype(instance);
	const bool	hardPointInfoFound = HARDPOINT->retrieve_hardpoint_info(arch, hardpoints[index].name, hardPointInfo);
	ASSERT(hardPointInfoFound);
	ENG->release_archetype(arch);

	// Convert the orientation into the world frame
	const Transform	instanceTransform = ENG->get_transform(instance);

	Matrix orient = instanceTransform.get_orientation() * hardPointInfo.orientation;	

	// Finally, convert to the position in the deformable objects frame
	const Transform	invDeformTransform = ENG->get_transform(object->get_root()).get_inverse();

	orient = invDeformTransform.get_orientation() * orient;

	orientation.SetI(orient.get_i());
	orientation.SetJ(orient.get_j());
	orientation.SetK(orient.get_k());
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectRenderHardpoints(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera)
{
	ASSERT(camera);
	ASSERT(PIPE);
		
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const unsigned int		count = object->get_num_hardpoints();
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);
	
	PrimitiveBuilder	pb(PIPE);

	pb.Begin(PB_LINES);

	Transform	oldModelView;

	PIPE->get_modelview(oldModelView);
	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

	for(unsigned int index = 0; index < count; ++index)
	{
		Vector		position;
		ROS::Matrix	orient;
		
		DeformableObjectGetHardPointPosition(defObj, index, position);
		DeformableObjectGetHardPointOrientation(defObj, index, orient);

		Matrix	orientation(orient.GetI(), orient.GetJ(), orient.GetK());

		const Transform	tr(orientation, position);
		const Transform	deformTransform = ENG->get_transform(object->get_root());
		const Transform currModelView = oldModelView * deformTransform * tr;

		PIPE->set_modelview(currModelView);

		pb.Color3f(1, 0, 0);
		position = orientation.get_i();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	
		pb.Color3f(0, 1, 0);
		position = orientation.get_j();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	
		pb.Color3f(0, 0, 0);
		position = orientation.get_k();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	}

	pb.End();

	PIPE->set_modelview(oldModelView);
}
// --------------------------------------------------------------------------
static void render_bone_tree (const Transform &viewXform, PrimitiveBuilder &pb, IModel *model, INSTANCE_INDEX root, float boneLength)
{
	// If the root is not invalid, add an item for it, then add each of its child trees
	if (root != INVALID_INSTANCE_INDEX)
	{
		// Render this bone's coordinate system
		const Transform tr = ENG->get_transform(root);
		const Transform currModelView = viewXform * tr;

		PIPE->set_modelview(currModelView);
		PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

		pb.Begin(PB_LINES);

		float scale = boneLength;

		pb.Color3f(1, 0, 0);
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(scale, 0, 0);
	
		pb.Color3f(0, 1, 0);
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(0, scale, 0);
	
		pb.Color3f(0, 0, 1);
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(0, 0, scale);

		pb.End();

		// Render a tree for each of the children of the root.
		assert (model != NULL);

		INSTANCE_INDEX child = model->get_child (root);
		while (child != INVALID_INSTANCE_INDEX)
		{
			render_bone_tree (viewXform, pb, model, child, boneLength);
			child = model->get_child (root, child);
		}
	}
}

void __cdecl DeformableObjectRenderSkeleton(const ROS::DADeformableObject* defObj, const ROS::DABaseCamera* camera, float boneLength)
{
	ASSERT(camera);
	ASSERT(PIPE);
		
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	
	PrimitiveBuilder	pb(PIPE);

	Transform	oldModelView;

	PIPE->get_modelview(oldModelView);
	PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

	Transform worldToView = CameraGetTransform(camera).get_inverse();

	render_bone_tree (worldToView, pb, MODEL, object->get_root(), boneLength);

	PIPE->set_modelview(oldModelView);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectAttachHardPointToParent(const ROS::DADeformableObject* defObj, unsigned int index, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName)
{
	ASSERT(defObj && parentHardPointHost);

	const INSTANCE_INDEX	parentInstance = GetHardPointHostInstanceIndex(parentHardPointHost);
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);
	const INSTANCE_INDEX	childInstance = hardpoints[index].object;
	ASSERT(childInstance != INVALID_INSTANCE_INDEX);
	ROS::ROSString			childHardPointName;

	DeformableObjectGetHardPointName(defObj, index, childHardPointName);

	const int	result = HARDPOINT->connect(parentInstance, parentHardPointName.c_str(), childInstance, childHardPointName.c_str());

	ASSERT(result == 0);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectDetachHardPointFromParent(const ROS::DADeformableObject* defObj, unsigned int index, const ROS::HardPointHost* parentHardPointHost, const ROS::ROSString& parentHardPointName)
{
	ASSERT(defObj && parentHardPointHost);

	const INSTANCE_INDEX	parentInstance = GetHardPointHostInstanceIndex(parentHardPointHost);
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);
	const INSTANCE_INDEX	childInstance = hardpoints[index].object;
	ASSERT(childInstance != INVALID_INSTANCE_INDEX);

	BOOL32	result = MODEL->disconnect(parentInstance, childInstance);
}
// --------------------------------------------------------------------------
const ROS::HardPointHost* __cdecl DeformableObjectGetHardPointHost(const ROS::DADeformableObject* defObj, unsigned int index)
{
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	const unsigned int		count = object->get_num_hardpoints();
	const HardpointDesc*	hardpoints = object->get_hardpoints();
	ASSERT(hardpoints);

	return GetHardPointHost(hardpoints[index].object);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectGetEndEffectorPosition (const ROS::DADeformableObject* defObj, const ROS::IKState& iKState, Vector &position)
{
	DeformableObject*	object = GetDeformableObject(defObj);
	ASSERT(object);

	const INSTANCE_INDEX idx = GetPart(object->get_root(), iKState.GetEndEffectorName().c_str());

	if (idx != INVALID_INSTANCE_INDEX)
	{
		position = ENG->get_position(idx);
	}
	else
	{
		position = Vector(0, 0, 0);
	}
}

// --------------------------------------------------------------------------
const ROS::DAIK* __cdecl DeformableObjectStartIK(const ROS::DADeformableObject* defObj, const ROS::IKState& iKState, const Vector& location, const Matrix& orient, float transition)
{
	DeformableObject*	object = GetDeformableObject(defObj);
	ASSERT(object);

	const INSTANCE_INDEX	endEffector = GetPart(object->get_root(), iKState.GetEndEffectorName().c_str());
	const INSTANCE_INDEX	rootEffector = GetAncestor(endEffector, iKState.GetCountToRootEffector());

	if(endEffector == INVALID_INSTANCE_INDEX || rootEffector == INVALID_INSTANCE_INDEX)
	{
		return NULL;
	}
	
	AimDesc	aimDesc("", rootEffector, endEffector, &location, &orient);

	aimDesc.set_damping(iKState.GetDampingFactor());

#if 0
	// *** This doesn't happen here. The axis is used when calculating the orientation matrix
	U32	axis;

	switch(iKState.GetEndEffectorAxis())
	{
		case ROS::IKState::kXAxis:
			axis = AimDesc::AD_AIM_I;
			break;

		case ROS::IKState::kYAxis:
			axis = AimDesc::AD_AIM_J;
			break;

		case ROS::IKState::kZAxis:
			axis = AimDesc::AD_AIM_K;
			break;
	
		default:
			ASSERT(0 && "Unknown case");
			axis = AimDesc::AD_AIM_K;
	}
#endif

	aimDesc.flags = 0;
	if (iKState.GetPointAtFlag())
	{
		aimDesc.flags |= (AimDesc::AD_EE_ORIENT | AimDesc::AD_IGNORE_LIMITS);
	}
	if (iKState.GetMoveToFlag())
	{
		aimDesc.flags |= AimDesc::AD_EE_POS;
	}

	ASSERT(aimDesc.flags != 0 && "Invalid IK flags");
		
	const HANDLE	handle = object->start_aim(aimDesc, transition);

	return GetDAIK(handle);
}
// --------------------------------------------------------------------------
void __cdecl DeformableObjectStopIK(const ROS::DADeformableObject* defObj, const ROS::DAIK* dAIK)
{
	DeformableObject*	object = GetDeformableObject(defObj);
	ASSERT(object);

	const HANDLE	handle = GetHandle(dAIK);

	if(handle)
	{
		object->end_aim(handle);
	}
}
// --------------------------------------------------------------------------
long __cdecl DeformableObjectGetRoot(const ROS::DADeformableObject* defObj)
{
	DeformableObject*		object = GetDeformableObject(defObj);
	ASSERT(object);
	return (unsigned long) object->get_root();
}// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
