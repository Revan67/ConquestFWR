#ifndef IDEFORMABLE_H
#define IDEFORMABLE_H

//

#ifndef DACOM_H
#include "dacom.h"
#endif

#include "typedefs.h"
#include "Engine.h"
#include "IAnim.h"
#include "FaceGroup.h"
#include "PatchGroup.h"
#include "TextureCoord.h"

//

#define IID_IDeformable MAKE_IID("IDeformable", 2)

//
// To create a stand-alone (non-Engine-aggregate) instance of IDeformable, you must provide
// both system and engine services from which necessary component interfaces can be queried.
//
struct DADEFORMDESC : public DACOMDESC
{
	struct IDAComponent *	system_services;
	struct IEngine *		engine_services;

	DADEFORMDESC(IDAComponent * sys, IEngine * eng)
	{
		size = sizeof(*this);
		interface_name = "IDeformable";
		system_services = sys;
		engine_services = eng;
	}
};

//

struct IDeformable : public IDAComponent
{
	typedef enum {POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z} Axis;

// General create() for complicated deformables, i.e. multiple parts and/or multiple meshes per part.
	DEFMETHOD(create)		(
							HANDLE & dh,								// resulting deformable handle.
							const struct DeformDesc & desc,				// creation description
							Animation::IEventHandler * callback = NULL,	// optional event callback.
							void * user_data = NULL,					// optional user data passed into callback.
							IEngineInstance * user_instance = NULL		// option user instance for skeleton root.
							) = 0;

// Simple create() for simple deformables, i.e. single part, single mesh.
	DEFMETHOD(create)		(
							HANDLE & dh,								// resulting Deformable handle.
							IFileSystem * mesh_parent,					// parent IFileSystem where mesh is located.
							const char * mesh_name,						// name of mesh file (DFM).
							IFileSystem * skeleton_parent,				// parent file system where skeleton CMP is located.
							Animation::IEventHandler * callback = NULL,	// optional event callback.
							void * user_data = NULL,					// optional user data passed to callback.
							IEngineInstance * user_instance = NULL		// optional IEngineInstance for skeleton root.
							) = 0;

	DEFMETHOD(destroy)		(HANDLE dh) = 0;

	DEFMETHOD(start_motion)	(	
							HANDLE dh,

						// return parameter. Fills in the SCRIPT_INST of the motion
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

						//per motion user data that will overide this->user_data						
							void * user_data = NULL
							) = 0;

	DEFMETHOD(start_motion_locked)	(	HANDLE dh, 
										SCRIPT_INST & inst, // return parameter.
										SCRIPT_SET_ARCH set, const char * script_name, 
										float start_time,
										float transition_duration,
										float time_scale,
										float weight,
										unsigned int flags,
										float heading,
										INSTANCE_INDEX locked_bone) = 0;

	DEFMETHOD(stop_motion)	(HANDLE dh) = 0;
	DEFMETHOD(stop_single_motion)	(HANDLE dh, const SCRIPT_INST inst) = 0;

// Start IK aim. aim_handle is return parameter.
	DEFMETHOD(start_aim)	(HANDLE dh, HANDLE & aim_handle, const struct AimDesc & desc, float transition_duration) = 0;

// stop IK aim, using handles of deformable & aim.
	DEFMETHOD(end_aim)		(HANDLE dh, HANDLE aim_handle) = 0;

	DEFMETHOD(pause)		(HANDLE dh) = 0;
	DEFMETHOD(resume)		(HANDLE dh) = 0;

	DEFMETHOD(update)		(HANDLE dh, float dt, const int * mesh_index) = 0;
	DEFMETHOD(render)		(HANDLE dh, struct ICamera * camera, const int * mesh_index) = 0;
	DEFMETHOD(render_alpha)	(HANDLE dh, struct ICamera * camera, const int * mesh_index, U8 alpha) = 0;

	DEFMETHOD(get_root)		(HANDLE dh, INSTANCE_INDEX & index) const = 0;

	DEFMETHOD(set_position)		(HANDLE dh, const Vector & p) = 0;
	DEFMETHOD(set_orientation)	(HANDLE dh, const Matrix & R) = 0;

	DEFMETHOD(set_heading_axis)	(HANDLE dh, Axis ax) = 0;
	DEFMETHOD(set_up_axis)		(HANDLE dh, Axis ax) = 0;

// returns a quaternion representing the deformable's heading angle about the up axis.
	DEFMETHOD(get_heading_quat)	(HANDLE dh, Quaternion & q) const = 0;

	DEFMETHOD(set_floor_height)	(HANDLE dh, float height) = 0;

	DEFMETHOD(visible_rect)		(HANDLE dh, BOOL32 & vis, RECT & rect, struct ICamera * camera) = 0;

	DEFMETHOD(get_num_active_scripts)	(HANDLE dh, U32 & num_active_scripts) const = 0;

// Fills in user-owned array of ActiveScriptDesc structs. Use get_num_active_scripts() immediately before
// calling to make sure your array is long enough.
	DEFMETHOD(describe_active_scripts)	(HANDLE dh, struct ActiveScriptDesc * desc) const = 0;

	DEFMETHOD(get_num_bones)	(HANDLE dh, U32 & num_bones) const = 0;

	DEFMETHOD(intersect_ray)	(
								HANDLE dh, 
								BOOL32 & hit,				// TRUE if ray intersects deformable, FALSE if not.
								Vector & intersection,		// point of intersection, if any.
								Vector & normal,			// surface normal at intersection.

								const Vector & origin,		// ray origin.
								const Vector & direction,	// ray direction.

								int & num_bones_hit,		// number of bones hit.
								INSTANCE_INDEX * bones_hit,	// list of bones hit. Can be NULL, but if not, list 
															// must be long enough to hold number of bones hit by ray,
															// use get_num_bones().

								const int * mesh_index		// list of integer indices of mesh to test for each part,
															// just like update() & render().
								) const = 0;

// Gives you a const pointer to an IDeformable-owned DeformableObject struct. Modify at your own risk.
	DEFMETHOD(get_DeformableObject)	(HANDLE dh, const struct DeformableObject *& do_ptr) const = 0;

	DEFMETHOD(get_num_hardpoints)	(HANDLE dh, U32 & num_hardpoints) const = 0;

// Gives you a const pointer to an IDeformable-owned list of hardpoints. Modify at your own risk.
	DEFMETHOD(get_hardpoints)		(HANDLE dh, const struct HardpointDesc *& hardpoints) const = 0;

//
// Fills in list of bone indices. Stops at array_length. Use get_num_bones() to make sure the array is long enough.
//
	DEFMETHOD(get_bone_indices)		(HANDLE dh, INSTANCE_INDEX * bone_indices, int array_length) const = 0;

// get number of faces and vertices last rendered
	DEFMETHOD(get_face_vertex_counts)	(HANDLE dh, const int * mesh_index, int ** faces, int ** vertices) const = 0;

// gets the pixel error used for patch tessellation
// -1.0f is returned if mesh is not a patch
	DEFMETHOD(get_surface_pixel_error)	(HANDLE dh, const int * mesh_index, float ** error) const = 0;
// sets the pixel error used for patch tessellation
// no effect id mesh is not a patch
	DEFMETHOD(set_surface_pixel_error)	(HANDLE dh, const int * mesh_index, const float * error) = 0;
};
				
//

struct DeformPartMeshDesc
{
	IFileSystem *	mesh_parent;
	const char *	mesh_name;
};

//

struct DeformPartDesc
{
	int						num_meshes;
	DeformPartMeshDesc *	meshes;
	IFileSystem *			skeleton_parent;
};

//

struct DeformDesc
{
	int						num_parts;
	DeformPartDesc *		parts;
};

//

struct ActiveScriptDesc
{
	const char *name;
	float		duration;
	float		current_time;
	SCRIPT_INST	instance;
};

//

struct DeformablePartArchetype
{
	U32							ref_cnt;
	bool						valid;

	char *						name;

	float						scale;

	int							num_bones;
	struct BoneArchetype *		bones;

// NEW-STYLE STUFF:
	int							face_cnt;					// total face count of all groups
	int							face_group_cnt;
	FaceGroup *					face_groups;

	int *						face_group_lookup;
	int *						face_group_index_lookup;
	int							vertex_batch_cnt;			// numer of unique vertices based on x,y,z,u,v,m
	int	*						vertex_batch_list;			// indices into object_vertex_list
	int *						texture_batch_list;			// indices into texture_vertex_list
	int *						texture_batch_list2;		// 2nd (optional) indices into texture_vertex_list

	int							object_vertex_cnt;
	Vector *					object_vertex_list;

	int							texture_vertex_cnt;
	TexCoord *					texture_vertex_list;

	int							vertex_reference_cnt;

	int							material_cnt;
	Material *					material_list;
	
	int *						vertex_bone_cnt;
	int *						vertex_bone_index;		// index into bone_*_list of vertex's first entry.
	int							bone_array_length;
	int *						bone_id_list;
	float *						bone_weight_list;
	Vector *					bone_vertex_list;
	Vector *					bone_normal_list;


	// uv's controlled by bones
	int							uv_bone_count;			// number of controlling bones	
	int *						uv_bone_id;				// bone id's
	int	*						uv_vertex_count;		// number of uv vertices affected per bone
	float *						uv_plane_distance;		// plane distance in -Z
	float *						x_to_u_scale;			
	float *						y_to_v_scale;			
	float *						min_du;					// uv limits
	float *						max_du;
	float *						min_dv;
	float *						max_dv;

	int							uv_list_length;			// sum of all uv_vertex_count[]
	int *						uv_vertex_id;			// indices into texture_vertex_list
	TexCoord *					uv_default_list;		// rest state uv coordinates

// Patch stuff
	int							patch_cnt;				// total count in all groups
	int							tri_patch_cnt;			// total number of triangular patches
	int							patch_group_cnt;		// # of patch groups
	PatchGroup *				patch_groups;			// Bezier patch groups

	int							edge_cnt;
	BezierEdge *				edges;

								
	ARCHETYPE_INDEX				arch_index;

	int							num_extras;

#define DPA_MAX_EXTRAS			8
	ARCHETYPE_INDEX				extras[DPA_MAX_EXTRAS];
	SCRIPT_SET_ARCH				script_set;

	DeformablePartArchetype *	prev;
	DeformablePartArchetype *	next;

// Default constructor for people too lazy to install compiler service packs.
	DeformablePartArchetype(void)
	{
		memset(this, 0, sizeof(*this));
		arch_index = INVALID_ARCHETYPE_INDEX;

		for (int i = 0; i < DPA_MAX_EXTRAS; i++)
		{
			extras[i] = INVALID_ARCHETYPE_INDEX;
		}
		script_set = INVALID_SCRIPT_SET_ARCH;
	}

	DeformablePartArchetype(const DeformPartDesc & desc, int mesh_number = 0);
	~DeformablePartArchetype(void);

	bool is_valid(void) const
	{
		return valid;
	}

	bool load_skeleton(IFileSystem * file, struct BoneDescriptor * bdesc, int & num_bones);
	bool load_mesh(IFileSystem * file);

	void setup(IFileSystem * sk_parent, const char * sk_name, BoneDescriptor * bdesc, int nbones);

	INSTANCE_INDEX create_instance(const char * filename, IFileSystem * file);

	INSTANCE_INDEX create_instance(void);

	void add_ref(void);
	U32 release(void);

	inline bool is_patch(void) const { return (patch_cnt > 0); }
};

//

struct DeformablePartMesh
{
	DeformablePartArchetype *	arch;
	struct BoneInstance *		bones;
};

//

struct DeformablePart
{
	int								num_meshes;
	DeformablePartMesh *			meshes;
	INSTANCE_INDEX					root;
	INSTANCE_INDEX					extras[DPA_MAX_EXTRAS];

	Vector *						transformed_vertices;
	Vector *						transformed_normals;
	
	mutable int						last_face_cnt;		// last number of tessellated faces
	mutable int						last_vertex_cnt;	// last number of tessellated vertices
	float							pixel_error;		// used for patch tessellation
	bool							need_normals;

	void build_tree(INSTANCE_INDEX * tree, INSTANCE_INDEX root, int & n);

	bool visible_rect(RECT & rect, const struct ICamera * camera) const;

	void deform(int mesh_index);
	void render(struct ICamera * camera, int mesh_index);

	void render_mesh(const struct ICamera * camera, DeformablePartMesh * mesh);
	void mtl_render_indexed_primitive_list(Material *mat, int uv_ch_num, U32 rwm_flags);

	void render_patches(struct ICamera * camera, DeformablePartMesh * mesh);

	void sort_faces_by_area(int mesh_index);
	void compute_face_areas(DeformablePartArchetype *arch);

	BOOL32 intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, int mesh_index) const;

	void get_face_vertex_counts(int mesh_index, int * faces, int * vertices) const;
	void get_surface_pixel_error(int mesh_index, float * error) const;
	void set_surface_pixel_error(int mesh_index, const float error);

	DeformablePart(int num_archs, DeformablePartArchetype * archs[], IEngineInstance * user_instance);

	~DeformablePart(void);
};

//
// Set position and/or orientation independently.
//
struct AimDesc
{
// "name" is for user reference only; it is completely ignored by DeformableObject code.
	const char *	name;

// root of aim motion, e.g. shoulder or neck, NOT root of skeleton.
	INSTANCE_INDEX	root;
	INSTANCE_INDEX	end_effector;

	enum 
	{
	// For single-joint aiming, use POINT or DIRECTION, then 
	// specify which local axis should point in that direction.
	// Local axes vary by skeleton, so may require some trial and error.
		AD_POINT		=0x01,		// treat target as point.
		AD_DIRECTION	=0x02,		// treat target as direction vector.
		AD_AIM_I		=0x04,		// orient end effector's i-vector toward point.
		AD_AIM_J		=0x08,		// orient end effector's j-vector toward point.
		AD_AIM_K		=0x10,		// orient end effector's k-vector toward point.

	// For multi-joint positioning/aiming, you've got to come up with the position/orientation
	// on the app side. 
		AD_EE_POS		=0x20,
		AD_EE_ORIENT	=0x40,

	// Joint limits are enforced by default. You can disable them with the following flag.
		AD_IGNORE_LIMITS=0x80,

	// Utilize the overlay flag of the anim system
		AD_OVERLAY		=0x100
	};

	U32				flags;
	const Vector &	target;	
	const Matrix &	R;
	float			damping_factor;
	int				num_locked_children;
	INSTANCE_INDEX  locked_children[16];

	AimDesc(const char * _name, INSTANCE_INDEX _root, INSTANCE_INDEX _end_eff, const Vector * _target_pos, const Matrix * _target_R) : target(*_target_pos), R(*_target_R)
	{
		name = _name;
		root = _root;
		end_effector = _end_eff;
		damping_factor = 1.0;
		num_locked_children = 0;
	}

	void set_damping(float damp)
	{
		damping_factor = damp;
	}

// Specify child object that won't move relative to its parent.
	void lock_child(INSTANCE_INDEX child)
	{
		locked_children[num_locked_children++] = child;
	}
};

//

struct DeformableObject : public Animation::IEventHandler
{
// Data members.

	static LList<DeformablePartArchetype> archetypes;

	int							num_parts;
	DeformablePart **			parts;
	INSTANCE_INDEX				root;

	//int							script_count;
	//HashPool<struct DefScriptNode, 32>	scripts;

	LList<struct ScriptLink>			active_scripts;

	Animation::IEventHandler *	callback;
	void *						user_data;

	float						floor_height;

	U32							num_hardpoints;
	struct HardpointDesc *		hardpoints;

	float						last_height;

// Methods.
	DeformableObject(void);
	~DeformableObject(void);

	void update(void);

	bool load_skeleton(IFileSystem * file, struct BoneDescriptor * bdesc, int & num_bones);

	//S32 get_script_index(const char * script_name) const;
	//struct DefScriptInfo * get_script_info(const char * script_name) const;
	//struct DefScriptInfo * get_script_info(S32 idx) const;

	typedef enum {POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z} Axis;

	Axis heading_axis;
	Axis up_axis;
	Axis side_axis;

	void compute_side_axis(void);

	Quaternion get_heading_quaternion(const Matrix & R);
	Quaternion compute_heading_quaternion(float heading);
	Vector adjust_height(const Vector & pos, float height);

//
// USER API. All functions above are for internal use.
//
	bool create(const DeformDesc & desc, Animation::IEventHandler * callback = NULL, void * user_data = NULL, IEngineInstance * user_instance = NULL);
	void destroy(void);

	
	// returns the SCRIPT_INST of the motion
	// Now takes a part_index and SCRIPT_SET_ARCH as well as a script name
	// The script_name requested MUST BE IN THE SET.
	SCRIPT_INST start_motion(	
						SCRIPT_SET_ARCH set,

						const char * script_name, 

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

					//per motion user data that will overide this->user_data						
						void * user_data = NULL);


	SCRIPT_INST start_motion_locked(	SCRIPT_SET_ARCH set, const char * script_name,
										float start_time,
										float transition_duration,
										float time_scale,
										float weight,
										unsigned int flags,
										float heading,
										INSTANCE_INDEX locked_bone);

	void stop_motion(void);
	void stop_single_motion(const SCRIPT_INST);

	HANDLE	start_aim(const AimDesc & desc, float transition_duration);
	void	end_aim(HANDLE aim);

	void pause(void);
	void resume(void);

// You MUST call deform() to compute the current deformed vertex positions before rendering.
	void deform(const int * mesh_index);

// render() DOESN'T UPDATE VERTEX POSITIONS ANY MORE. Also, it's in your interest to call deform() 
// followed by render() using the SAME list of mesh indices.
// the tessellation_cnt is just a temporary hack; valid range is 1 and up
	void render(struct ICamera * camera, const int * mesh_index);

	void render_alpha(struct ICamera * camera, const int * mesh_index, U8 alpha);

	inline INSTANCE_INDEX get_root(void) const
	{
		return root;
	}

	void set_position(const Vector & p);
	void set_orientation(const Matrix & R);

	void set_heading_axis(Axis ax);
	void set_up_axis(Axis ax);

	void set_floor_height(float height);

	bool visible_rect(RECT & rect, struct ICamera * camera);

	U32 get_num_active_scripts(void);
	void describe_active_scripts(ActiveScriptDesc * desc);

	U32 get_num_hardpoints(void) const;
	const struct HardpointDesc * get_hardpoints(void) const;

	void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator);
    void COMAPI on_finished(unsigned int channel_id, void * user_supplied);
	void COMAPI on_loop(unsigned int channel_id, Transform & T, void * user_supplied);

	BOOL32 COMAPI intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, const int * mesh_index) const;

	void get_face_vertex_counts(const int * mesh_index, int ** faces, int ** vertices) const;
	void get_surface_pixel_error(const int * mesh_index, float ** error) const;
	void set_surface_pixel_error(const int * mesh_index, const float * error);
};

//

struct HardpointDesc
{
	char *			name;
	INSTANCE_INDEX	object;

	HardpointDesc(void)
	{
		name = NULL;
		object = INVALID_INSTANCE_INDEX;
	}

	HardpointDesc(const HardpointDesc & desc)
	{
		name = strdup(desc.name);
		object = desc.object;
	}

	HardpointDesc & operator = (const HardpointDesc & desc)
	{
		name = strdup(desc.name);
		object = desc.object;
		return *this;
	}

	~HardpointDesc(void)
	{
		if (name)
		{
			free(name);
			name = NULL;
		}

		object = INVALID_INSTANCE_INDEX;
	}
};

#endif