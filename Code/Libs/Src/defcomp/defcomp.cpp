//
// defcomp.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "dacom.h"
#include "tcomponent.h"
#include "da_heap_utility.h"
#include "SysConsumerDesc.h"
#include "lightman.h"
#include "ITextureLibrary.h"
#include "ianim.h"
#include "ihardpoint.h"
#include "physics.h"
#include "collision.h"

#include "defcomp.h"

//

static const C8 interface_name[]		= "IDeformable";	// Interface name used for registration
static const C8 implementation_name[]	= "default";		// The implementation we want

//

DEFORM::DEFORM(void)
{
	system		= NULL;
	BATCH		= NULL;
	ENG			= NULL;
	PHYSICS		= NULL;
	COLLIDE		= NULL;
	HARDPOINT	= NULL;
	LIGHT		= NULL;
	TXMLIB		= NULL;
	ANIM		= NULL;
	CHANNEL		= NULL;
}

//

//
// DeformOpen() and DeformClose() handle references to
// system components and engine components BUT not to the 
// system and engine specifically.  The system and engine 
// references are handled by the deform component.
//
bool DeformOpen(IDAComponent *, IEngine *);
void DeformClose(void);

//

DEFORM::~DEFORM(void)
{
	DeformClose();
	
	DACOM_RELEASE(system);
	if( stand_alone ) {
		DACOM_RELEASE(ENG);
	}
}

//

GENRESULT DEFORM::init( SYSCONSUMERDESC * desc )
{
	// we are an engine aggregate
	//

	stand_alone = false;

	if( (system = desc->system) == NULL ) {
		return GR_GENERIC;
	}
	system->AddRef();

	if( FAILED( static_cast<IDeformable*>(this)->QueryInterface( IID_IEngine, (void**) &ENG ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IEngine, unable to continue." );
		return GR_GENERIC;
	}
	ENG->Release();

	// don't call Initialize as it will be called for us
	// after we are aggregated.

	return GR_OK;
}

//

GENRESULT DEFORM::init( struct DADEFORMDESC * desc )
{
	stand_alone = true;

	if( (system = desc->system_services) == NULL ) {
		return GR_GENERIC;
	}
	system->AddRef();

	if( (ENG = desc->engine_services) == NULL ) {
		return GR_GENERIC;
	}
	ENG->AddRef();

	return Initialize();
}

//

GENRESULT DEFORM::Initialize(void)
{
	DeformClose();

	if( DeformOpen( system, ENG ) ) {
		return GR_OK;
	}

	return GR_GENERIC;
}

//

GENRESULT COMAPI DEFORM::create(HANDLE & dh, const DeformDesc & desc, Animation::IEventHandler * callback, void * user_data, IEngineInstance * user_instance)
{
	GENRESULT result = GR_GENERIC;

	if( desc.num_parts <= 0 )
	{
		result = GR_INVALID_PARMS;
	}
	else
	{
		DeformableObject * d = new DeformableObject;
		if (d)
		{
			if (d->create(desc,  callback, user_data, user_instance))
			{
				dh = d;
				result = GR_OK;
			}
			else
			{
				delete d;
				dh = NULL;
			}
		}
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::create(HANDLE & dh, IFileSystem * mesh_parent, const char * mesh_name, IFileSystem * skeleton_parent, Animation::IEventHandler * callback, void * user_data, IEngineInstance * user_instance)
{
	GENRESULT result = GR_GENERIC;

	DeformPartMeshDesc mesh;
	mesh.mesh_name		= mesh_name;
	mesh.mesh_parent	= mesh_parent;

	DeformPartDesc part;
	part.num_meshes			= 1;
	part.meshes				= &mesh;
	part.skeleton_parent	= skeleton_parent;

	DeformDesc desc;
	desc.num_parts	= 1;
	desc.parts		= &part;

	DeformableObject * d = new DeformableObject;
	if (d)
	{
		if (d->create(desc, callback, user_data, user_instance))
		{
			dh = d;
			result = GR_OK;
		}
		else
		{
			delete d;
			dh = NULL;
		}
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::destroy(HANDLE dh)
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		d->destroy();
		delete d;
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::start_motion(	HANDLE dh, SCRIPT_INST & inst, SCRIPT_SET_ARCH script_set, const char * script_name, 
								float start_time, float transition_duration, float time_scale, float weight, 
								unsigned int flags, float heading, void *user_data)
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;

		inst = d->start_motion(script_set, script_name, start_time, transition_duration, time_scale, weight, flags, heading, user_data);
		if (inst != INVALID_SCRIPT_INST)
		{
			result = GR_OK;
		}
		else
		{
			result = GR_GENERIC;
		}
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::start_motion_locked(	HANDLE dh,
						 						SCRIPT_INST & inst,
						 						SCRIPT_SET_ARCH set, const char * script_name, 
						 						float start_time,
						 						float transition_duration,
						 						float time_scale,
						 						float weight,
						 						unsigned int flags,
						 						float heading,
						 						INSTANCE_INDEX locked_bone)
{
	GENRESULT result;
	if (dh)
	{
		inst = ((DeformableObject *) dh)->start_motion_locked(set, script_name, start_time, transition_duration, time_scale, weight, flags, heading, locked_bone);
		if (inst != INVALID_SCRIPT_INST)
		{
			result = GR_OK;
		}
		else
		{
			result = GR_GENERIC;
		}
	}
	else
	{
		result = GR_GENERIC;
	}
	return result;
}


//

GENRESULT COMAPI DEFORM::stop_motion(HANDLE dh)
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		d->stop_motion();
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::stop_single_motion(HANDLE dh, const SCRIPT_INST inst)
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		d->stop_single_motion(inst);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::start_aim(HANDLE dh, HANDLE & aim_handle, const AimDesc & desc, float transition_duration)
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		aim_handle = d->start_aim(desc, transition_duration);
		if (aim_handle != INVALID_HANDLE_VALUE)
		{
			result = GR_OK;
		}
		else
		{
			result = GR_GENERIC;
		}
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::end_aim(HANDLE dh, HANDLE aim_handle)
{
	GENRESULT result;

	if (dh && aim_handle)
	{
		DeformableObject * d = (DeformableObject *) dh;
		d->end_aim(aim_handle);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::pause(HANDLE dh)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->pause();
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::resume(HANDLE dh)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->resume();
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::update(HANDLE dh, float dt, const int * mesh_index)
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->deform(mesh_index);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::render(HANDLE dh, struct ICamera * camera, const int * mesh_index)
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->render(camera, mesh_index);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::render_alpha(HANDLE dh, struct ICamera * camera, const int * mesh_index, U8 alpha)
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->render_alpha(camera, mesh_index, alpha);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_face_vertex_counts(HANDLE dh, const int * mesh_index, int ** faces, int ** vertices) const
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->get_face_vertex_counts(mesh_index, faces, vertices);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

GENRESULT COMAPI DEFORM::get_surface_pixel_error(HANDLE dh, const int * mesh_index, float ** error) const
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->get_surface_pixel_error(mesh_index, error);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

GENRESULT COMAPI DEFORM::set_surface_pixel_error(HANDLE dh, const int * mesh_index, const float *error)
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( mesh_index );
		((DeformableObject *) dh)->set_surface_pixel_error(mesh_index, error);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_root(HANDLE dh, INSTANCE_INDEX & index) const
{
	GENRESULT result;

	if (dh)
	{
		index = ((DeformableObject *) dh)->get_root();
		result = GR_OK;
	}
	else
	{
		index = INVALID_INSTANCE_INDEX;
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::set_position(HANDLE dh, const Vector & p)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->set_position(p);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::set_orientation(HANDLE dh, const Matrix & R)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->set_orientation(R);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::set_heading_axis(HANDLE dh, Axis ax)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->set_heading_axis((DeformableObject::Axis) ax);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::set_up_axis(HANDLE dh, Axis ax)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->set_up_axis((DeformableObject::Axis) ax);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_heading_quat(HANDLE dh, Quaternion & q) const
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;

		Matrix R = ENG->get_orientation(d->get_root());

		q = d->get_heading_quaternion(R);

		result = GR_OK;
	}
	else
	{
		q.set_identity();
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::set_floor_height(HANDLE dh, float height)
{
	GENRESULT result;

	if (dh)
	{
		((DeformableObject *) dh)->set_floor_height(height);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::visible_rect(HANDLE dh, BOOL32 & vis, RECT & rect, struct ICamera * camera)
{
	GENRESULT result;

	if (dh)
	{
		vis = ((DeformableObject *) dh)->visible_rect(rect, camera);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_num_active_scripts(HANDLE dh, U32 & num_active_scripts) const
{
	GENRESULT result;

	if (dh)
	{
		num_active_scripts = ((DeformableObject *) dh)->get_num_active_scripts();
		result = GR_OK;
	}
	else
	{
		num_active_scripts = 0;
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::describe_active_scripts(HANDLE dh, ActiveScriptDesc * desc) const
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( desc );
		((DeformableObject *) dh)->describe_active_scripts(desc);
		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}


	return result;
}

//

GENRESULT COMAPI DEFORM::get_num_bones(HANDLE dh, U32 & num_bones) const
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;

	// count up the bones. BONES CAN BE SHARED BETWEEN MULTIPLE PARTS, SO THIS IS AN UPPER BOUND,
	// NOT AN EXACT COUNT.

		num_bones = 0;
		for (int i = 0; i < d->num_parts; i++)
		{
			DeformablePart * part = d->parts[i];
			num_bones += part->meshes[0].arch->num_bones;
		}

		result = GR_OK;
	}
	else
	{
		num_bones = 0;
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_bone_indices(HANDLE dh, INSTANCE_INDEX * bone_indices, int array_length) const
{
	GENRESULT result;

	if (dh)
	{
		ASSERT( bone_indices );
		const DeformableObject * d = (const DeformableObject *) dh;

		int num_bones = 0;
		for (int i = 0; i < d->num_parts; i++)
		{
			DeformablePart * part = d->parts[i];
			num_bones += part->meshes[0].arch->num_bones;
		}

		int num = __min(array_length, num_bones);
		int idx = 0;

		for (i = 0; i < d->num_parts; i++)
		{
			DeformablePart * part = d->parts[i];

			for (int j = 0; j < part->meshes[0].arch->num_bones; j++)
			{
				bone_indices[idx++] = part->meshes[0].bones[j].instance;
			}
		}

		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::intersect_ray(HANDLE dh, BOOL32 & hit, Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, const int * mesh_index) const
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		hit = d->intersect_ray(intersection, normal, origin, direction, num_bones_hit, bones_hit, mesh_index);

		result = GR_OK;
	}
	else
	{
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_DeformableObject(HANDLE dh, const DeformableObject *& ptr) const
{
	GENRESULT result;

	if (dh)
	{
		ptr = (const DeformableObject *) dh;
		result = GR_OK;
	}
	else
	{
		ptr = NULL;
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_num_hardpoints(HANDLE dh, U32 & num_hardpoints) const
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		num_hardpoints = d->get_num_hardpoints();
		result = GR_OK;
	}
	else
	{
		num_hardpoints = 0;
		result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI DEFORM::get_hardpoints(HANDLE dh, const HardpointDesc *& hardpoints) const
{
	GENRESULT result;

	if (dh)
	{
		DeformableObject * d = (DeformableObject *) dh;
		hardpoints = d->get_hardpoints();

		result = GR_OK;
	}
	else
	{
		hardpoints = NULL;
		result = GR_GENERIC;
	}

	return result;
}

//

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hModule);

		// Create and register engine-aggregate component factory.
			IComponentFactory * server = new DAComponentFactory2<DAComponentAggregate<DEFORM>, SYSCONSUMERDESC>(interface_name); 
			if (server)
			{
				ICOManager * DACOM = DACOM_Acquire(); // Handle to component manager
				if (DACOM != NULL)
				{
					DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
					DACOM->RegisterComponent(server, IID_IDeformable, DACOM_NORMAL_PRIORITY);
					DACOM->Release();
				}
				else
				{
					GENERAL_TRACE_1("Deformable (DEFORMABLE.DLL): unable to acquire DACOM.\n");
				}

				server->Release();
			}
			else
			{
				GENERAL_TRACE_1("Deformable (DEFORMABLE.DLL): unable to create instance of Deformable.\n");
			}

		// Create and register stand-alone component factory.
			server = new DAComponentFactory<DAComponent<DEFORM>, DADEFORMDESC>(interface_name);
			if (server)
			{
				ICOManager * DACOM = DACOM_Acquire(); // Handle to component manager
				if (DACOM != NULL)
				{
					DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
					DACOM->RegisterComponent(server, IID_IDeformable, DACOM_NORMAL_PRIORITY);
					DACOM->Release();
				}
				else
				{
					GENERAL_TRACE_1("Deformable (DEFORMABLE.DLL): unable to acquire DACOM.\n");
				}

				server->Release();
			}
			else
			{
				GENERAL_TRACE_1("Deformable (DEFORMABLE.DLL): unable to create instance of Deformable.\n");
			}

			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

//