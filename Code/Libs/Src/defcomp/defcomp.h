#ifndef DEFCOMP_H
#define DEFCOMP_H

//

#include "IDeformable.h"
#include "deform.h"

#include "tcomponent.h"
#include "RPUL\primitivebuilder.h"

//

struct DEFORM : public IDeformable, public IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(DEFORM)
	DACOM_INTERFACE_ENTRY(IDeformable)
	DACOM_INTERFACE_ENTRY2(IID_IDeformable, IDeformable)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	struct IDAComponent *		system;
	struct IRenderPrimitive *	BATCH;
	struct IEngine *			ENG;
	struct IPhysics *			PHYSICS;
	struct ICollision *			COLLIDE;
	struct IHardpoint *			HARDPOINT;
	struct ILightManager *		LIGHT;
	struct ITextureLibrary *	TXMLIB;
	struct IAnimation *			ANIM;
	struct IChannel *			CHANNEL;

	bool						stand_alone;

	DEFORM(void);
	~DEFORM(void);

	GENRESULT init(struct SYSCONSUMERDESC * desc);
	GENRESULT init(struct DADEFORMDESC * desc);

	GENRESULT COMAPI Initialize(void);

// old-style create() for complicated deformables, i.e. multiple parts and/or multiple meshes per part.
	virtual GENRESULT COMAPI create(HANDLE & dh, const DeformDesc & desc, Animation::IEventHandler * callback = NULL, void * user_data = NULL, IEngineInstance * user_instance = NULL);

// simple create() for simple deformables, i.e. single part, single mesh.
	virtual GENRESULT COMAPI create(HANDLE & dh, IFileSystem * mesh_parent, const char * mesh_name, IFileSystem * skel_parent, Animation::IEventHandler * callback = NULL, void * user_data = NULL, IEngineInstance * user_instance = NULL);

	virtual GENRESULT COMAPI destroy(HANDLE dh);

	virtual GENRESULT COMAPI start_motion(	HANDLE dh,

										// return parameter.
											SCRIPT_INST & inst,	

										// animation script set & name.
											SCRIPT_SET_ARCH script_set, const char * script_name, 

										// start_time: Animation::BEGIN, Animation::CUR, Animation::END, or positive number 
										// indicating time from beginning of motion. 
											float start_time,

										// transition_duration: length of transition from previous motion/pose to new motion.
											float transition_duration, 

										// time_scale: scalar applied to time passage for this motion.
											float time_scale = 1.0f, 

										// weight: scalar applied to motion's effect on objects/joints.
											float weight = 1.0f, 

										// flags: can be any valid combination of Animation flags. See AnimTypes.h.
											unsigned int flags = Animation::FORWARD,

										// heading: start heading for motion. MUST BE POSITIVE, 0 <= heading <= 2PI.
										// negative number means use current heading.
											float heading = -1.0f,

										// per motion user data overrides user_data passed in for create											
											void * user_data = NULL);

	virtual GENRESULT COMAPI start_motion_locked(	HANDLE dh,
													SCRIPT_INST & inst,
													SCRIPT_SET_ARCH set, const char * script_name, 
													float start_time,
													float transition_duration,
													float time_scale,
													float weight,
													unsigned int flags,
													float heading,
													INSTANCE_INDEX locked_bone);

	virtual GENRESULT COMAPI stop_motion(HANDLE dh);
	virtual	GENRESULT COMAPI stop_single_motion(HANDLE dh, const SCRIPT_INST inst);

// Start IK aim. aim_handle is return parameter.
	virtual GENRESULT COMAPI start_aim(HANDLE dh, HANDLE & aim_handle, const AimDesc & desc, float transition_duration);

// stop IK aim, using handles of deformable & aim.
	virtual GENRESULT COMAPI end_aim(HANDLE dh, HANDLE aim_handle);

	virtual GENRESULT COMAPI pause(HANDLE dh);
	virtual GENRESULT COMAPI resume(HANDLE dh);

	virtual GENRESULT COMAPI update(HANDLE dh, float dt, const int * mesh_index);
	virtual GENRESULT COMAPI render(HANDLE dh, struct ICamera * camera, const int * mesh_index);
	virtual GENRESULT COMAPI render_alpha(HANDLE dh, struct ICamera * camera, const int * mesh_index, U8 alpha);

	virtual GENRESULT COMAPI get_root(HANDLE dh, INSTANCE_INDEX & index) const;

	virtual GENRESULT COMAPI set_position(HANDLE dh, const Vector & p);
	virtual GENRESULT COMAPI set_orientation(HANDLE dh, const Matrix & R);

	virtual GENRESULT COMAPI set_heading_axis(HANDLE dh, Axis ax);
	virtual GENRESULT COMAPI set_up_axis(HANDLE dh, Axis ax);

	virtual GENRESULT COMAPI get_heading_quat(HANDLE dh, Quaternion & q) const;

	virtual GENRESULT COMAPI set_floor_height(HANDLE dh, float height);

	virtual GENRESULT COMAPI visible_rect(HANDLE dh, BOOL32 & vis, RECT & rect, struct ICamera * camera);

	virtual GENRESULT COMAPI get_num_active_scripts(HANDLE dh, U32 & num_active_scripts) const;
	virtual GENRESULT COMAPI describe_active_scripts(HANDLE dh, ActiveScriptDesc * desc) const;

	virtual GENRESULT COMAPI get_num_bones(HANDLE dh, U32 & num_bones) const;

	virtual GENRESULT COMAPI intersect_ray(HANDLE dh, BOOL32 & hit, Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, const int * mesh_index) const;

	virtual GENRESULT COMAPI get_DeformableObject(HANDLE dh, const DeformableObject *& do_ptr) const;

	virtual GENRESULT COMAPI get_num_hardpoints(HANDLE dh, U32 & num_hardpoint) const;
	virtual GENRESULT COMAPI get_hardpoints(HANDLE dh, const HardpointDesc *& hardpoints) const;

	virtual GENRESULT COMAPI get_bone_indices(HANDLE dh, INSTANCE_INDEX * bone_indices, int array_length) const;

	virtual GENRESULT COMAPI get_face_vertex_counts(HANDLE dh, const int * mesh_index, int ** faces, int ** vertices) const;

	virtual GENRESULT COMAPI get_surface_pixel_error(HANDLE dh, const int * mesh_index, float ** error) const;
	virtual GENRESULT COMAPI set_surface_pixel_error(HANDLE dh, const int * mesh_index, const float * error);
};

//

#endif