#ifndef DEFORM_H
#define DEFORM_H

// NOTE: the app must link w/ deform.lib

/*
 Sample code for creating a deformable object:

	DeformableObject TheCharacter;		// instantiate DeformableObject.

	DeformPartDesc parts[2];			// describe parts. 
	parts[0].mesh_parent = NULL;		// *_parent fields are IFileSystem pointers to parent file system, if any.
	parts[0].mesh_name = "head.3db";
	parts[0].skeleton_parent = NULL;	// Mesh, skeleton, and animation files can all live in different file systems.
	parts[0].anim_parent = NULL;
	parts[0].anim_name = "head.anm";

	parts[1].mesh_parent = NULL;
	parts[1].mesh_name = "body.3db";
	parts[1].skeleton_parent = NULL;
	parts[1].anim_parent = NULL;
	parts[1].anim_name = "body.anm";

	CharEventHandler event_handler;		// CharEventHandler implements Channel::IEventHandler interface.

	DeformDesc desc;					// describe character.
	desc.num_parts = 2;
	desc.parts = parts;

	char user[] = "Sample user data. Can be anything.";

	TheCharacter.create(desc, &event_handler, user);

	int num_scripts = TheCharacter.get_script_count();

	const char ** script_names = new const char *[num_scripts];
	TheCharacter.get_scripts(script_names);

// Start the 0th script with a transition time of 0.5 seconds and no looping.
	TheCharacter.start_motion(script_names[0], 0.5, false);

*/

#include <stdlib.h>

#include "FileSys.h"
#include "Engine.h"
#include "IAnim.h"
#include "StdDAT.h"
#include "system.h"
#include "FaceProp.h"
#include "FaceGroup.h"
#include "PatchGroup.h"
#include "TextureCoord.h"
#include "matrix4.h"

//

struct BoneDescriptor
{
	char *	object_name;
	char *	file_name;
	int		index;

	int		num_vertices;
	Vector *vertices;
	Vector *normals;

	static char * mesh_name;

	int		vertex_counter;
	bool	extra:1;

	BoneDescriptor(void)
	{
		memset(this, 0, sizeof(*this));
		index = -1;
	}

	~BoneDescriptor(void)
	{
		free();
	}

	void free(void)
	{
		if (object_name)
		{
			delete [] object_name;
			object_name = NULL;
		}
		if (file_name)
		{
			delete [] file_name;
			file_name = NULL;
		}
		if (vertices)
		{
			delete [] vertices;
			vertices = NULL;
		}
		if (normals)
		{
			delete [] normals;
			normals = NULL;
		}

		if (mesh_name)
		{
			delete [] mesh_name;
			mesh_name = NULL;
		}
		
		memset(this, 0, sizeof(*this));     
	}

	void read(IFileSystem * file)
	{
		U32 bytes_read;

	// Name of object, for use with IModel::is_named().
		DAFILEDESC desc("Object name");
		desc.dwDesiredAccess = GENERIC_READ;
		desc.dwCreationDistribution = OPEN_EXISTING;
		HANDLE h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			int size = file->GetFileSize(h, NULL);
			if (size)
			{
				object_name = new char[size];
				file->ReadFile(h, object_name, size, &bytes_read);
			}
			file->CloseHandle(h);
		}

	// Name of file, to be read below.
		desc = "File name";
		h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			int size = file->GetFileSize(h, NULL);
			if (size)
			{
				file_name = new char[size];
				file->ReadFile(h, file_name, size, &bytes_read);
			}
			file->CloseHandle(h);
		}

	// Index.
		desc = "Index";
		h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			file->ReadFile(h, &index, sizeof(index), &bytes_read);
			file->CloseHandle(h);
		}
	}
};

//

struct BoneArchetype
{
// DEBUG
	char name[64];
// DEBUG

	int				id;

	int				num_vertices;
	Vector *		vertices;
	Vector *		normals;

	int				num_faces;
	int *			faces;

	bool			extra:1;

	BoneArchetype(void)
	{
		memset(this, 0, sizeof(BoneArchetype));
	}

	~BoneArchetype(void)
	{
		delete [] vertices;
		delete [] normals;
		delete [] faces;
	}

	void init(const BoneDescriptor & desc)
	{
	// DEBUG
		if (desc.object_name)
		{
			strcpy(name, desc.object_name);
		}
	// DEBUG

		id = desc.index;

		num_vertices = desc.num_vertices;
		if (num_vertices)
		{
			vertices = new Vector[num_vertices];
			memcpy(vertices, desc.vertices, sizeof(Vector) * num_vertices);

			if(desc.normals) // patches don't have normals
			{
				normals = new Vector[num_vertices];
				memcpy(normals, desc.normals, sizeof(Vector) * num_vertices);
			}
		}

		extra = desc.extra;
	}
};

//

struct BoneInstance
{
	const BoneArchetype *	arch;
	INSTANCE_INDEX			instance;
	Vector *				transformed_vertices;
	Vector *				transformed_normals;
	U32						vertex_counter;

	BoneInstance(const BoneArchetype * arch, const bool need_normals);
	~BoneInstance(void)
	{
		delete [] transformed_vertices;
		delete [] transformed_normals;
	}
};

//


//EMAURER declare here so as to avoid including 
//IDumpText.h which redefines 'assert'  
struct IDumpText;

//
// YOU MUST CALL DeformOpen() before using DeformableObjects.
//
// DeformOpen() returns false if it cannot get pointers to all the interfaces
// it needs from "engine." These include IModel, IAnimation, ITXMLib, and
// ILightManager.
//
bool DeformOpen(IDAComponent *system, IEngine * engine, IDumpText * dump = NULL);

//
// Call DeformClose() before shutting down to release all references
// to DACOM components.
//
void DeformClose(void);

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
	SCRIPT_SET_ARCH			anim_script_set;
};

//

struct DeformDesc
{
	int						num_parts;
	DeformPartDesc *		parts;
};

//

struct DefScriptInfo
{
	char *					name;
	struct DeformablePart *	part;
	bool					overlay;

	DefScriptInfo(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~DefScriptInfo(void)
	{
		if (name)
		{
			free(name);
			name = NULL;
		}
	}

// assignment operator needed since HashPool will happily copy pointers around,
// then destroy the original data.
	const DefScriptInfo & operator = (const DefScriptInfo & copy)
	{
		name	= strdup(copy.name);
		part	= copy.part;
		overlay	= copy.overlay;
		return *this;
	}

	SCRIPT_SET_ARCH get_script_set(void) const;
	float get_scale(void) const;
};

//

struct DefScriptNode
{
	DefScriptInfo		info;
// HashPool stuff...
	U32					hash_key;    // For internal use only
	DefScriptNode *		hash_next;   
	DefScriptNode *		hash_prev;  
	DefScriptNode *		next;          
	DefScriptNode *		prev;       
                          
	S32					index;       // Index of this entry in linear array

// SCRIPT NAMES WILL TEND TO HAVE A COMMON PREFIX, so using the 
// first 2 characters for hashing will result in a linear search.
// Use variable-string addition instead.
	static inline U32 hash(const void * obj)
	{
		const DefScriptInfo * src = (const DefScriptInfo *) obj;
		const char * str = src->name;
		U32 result = 0;
		while (*str)
		{
			result += *str++;
		}
		return result & 0xff;
	}

	BOOL32 compare(const void * obj)
	{
		const DefScriptInfo * sinfo = (const DefScriptInfo *) obj;
    	return !strcmp(info.name, sinfo->name);
	}

	void initialize(const void * obj)
	{
		DefScriptInfo * src = (DefScriptInfo *) obj;
		info.name			= strdup(src->name);
		info.part			= src->part;
	}

	void shutdown(void);
	void display(void) {}

};

//

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
		AD_IGNORE_LIMITS=0x80
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

struct IKJoint
{
	JOINT_INDEX				idx;
	CHANNEL_INSTANCE_INDEX	channel;
	Quaternion				joint_data;		// joint data.
	Matrix					R;				// global orientation
	Vector					r;				// offset in R.
	Vector					p;				// global position.
	Quaternion				qmid;			// midpoint (relative) orientation.

	IKJoint(void) {}
	void init(JOINT_INDEX jnt, CHANNEL_INSTANCE_INDEX chan);
};

//

struct IKScriptLink : public Channel::IVirtualChannel
{
	HANDLE					handle;
	U32						num_joints;
	JOINT_INDEX *			joints;
	Matrix *				Rmid;
	INSTANCE_INDEX			root;
	INSTANCE_INDEX			end_effector;
	CHANNEL_INSTANCE_INDEX *channels;
	Quaternion *			data;
	const Vector &			point;
	const Matrix &			orient;
	U32						flags;
	bool *					locked;
	int						num_locked_joints;
	bool					done:1;
	bool					child_offset:1;
	float					damping_factor;

	IKScriptLink(const Vector & pt, const Matrix & _orient) : point(pt), orient(_orient), done(false)
	{
	}

	~IKScriptLink(void);
	virtual int update(void * dst, U32 channel_idx, const Channel::Target & target, float time);

	void compute_forward_kinematics(Vector & p_eff, Matrix & R_eff, Vector * p_new, Matrix * R_new) const;

	void build_jacobian(void) const;
	bool solve(void);
	void solve_sr(void);
	void solve_sr_child_offset(void);

};

//

struct ScriptLink
{
	S32				index;

	bool			ik_script;
	union
	{
		SCRIPT_INST		instance;
		IKScriptLink *	ik;
	};

	float			start_height;

	ScriptLink *	prev;
	ScriptLink *	next;

	ScriptLink(void)
	{
		ik_script = false;
		instance = -1;
	}

	void release(void);
};

//

struct DeformablePartArchetype
{
	U32							ref_cnt;
	bool						valid;

	char *						name;

	float						scale;

	int							num_bones;
	BoneArchetype *				bones;

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
	int							patch_group_cnt;		// # of patch groups
	PatchGroup *				patch_groups;			// Bezier patch groups

								
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
	BoneInstance **				bones;
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
	
	int								last_face_cnt;
	int								last_vertex_cnt;
	bool							need_normals;

	void build_tree(INSTANCE_INDEX * tree, INSTANCE_INDEX root, int & n);

	bool visible_rect(RECT & rect, const struct ICamera * camera) const;

	void deform(int mesh_index);
	void render(struct ICamera * camera, int mesh_index, int tessellation_cnt = 1);

	void render_mesh(const struct ICamera * camera, DeformablePartMesh * mesh);
	void mtl_render_indexed_primitive_list(Material *mat, int uv_ch_num, U32 rwm_flags);

	void render_patches(struct ICamera * camera, DeformablePartMesh * mesh, int tessellation_cnt);

	void sort_faces_by_area(int mesh_index);
	void compute_face_areas(DeformablePartArchetype *arch);

	BOOL32 intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, int mesh_index) const;

	DeformablePart(int num_archs, DeformablePartArchetype * archs[], IEngineInstance * user_instance);

	~DeformablePart(void);
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



//

struct DeformableObject : public Channel::IEventHandler
{
// Data members.

	static LList<DeformablePartArchetype> archetypes;

	int							num_parts;
	DeformablePart **			parts;
	INSTANCE_INDEX				root;

	int							script_count;
	HashPool<DefScriptNode, 32>	scripts;

	LList<ScriptLink>			active_scripts;

	Channel::IEventHandler *	callback;
	void *						user_data;

	float						floor_height;

	U32							num_hardpoints;
	HardpointDesc *				hardpoints;

	float						last_height;

// Methods.
	DeformableObject(void);
	~DeformableObject(void);

	void update(void);

	bool load_skeleton(IFileSystem * file, struct BoneDescriptor * bdesc, int & num_bones);

	S32 get_script_index(const char * script_name) const;
	DefScriptInfo * get_script_info(const char * script_name) const;
	DefScriptInfo * get_script_info(S32 idx) const;

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
	bool create(const DeformDesc & desc, Channel::IEventHandler * callback = NULL, void * user_data = NULL, IEngineInstance * user_instance = NULL);
	void destroy(void);

	U32 get_script_count(void) const
	{
		return script_count;
	}

	BOOL32 get_scripts(const char * script_names[]) const;

	float get_script_duration(const char * script_name) const;
	BOOL32 get_script_events(const char * script_name, EventIterator & events) const;

	bool start_motion(	const char * script_name, 

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
						float heading = -1.0f);

// 
// start_motion2() is IDENTICAL to start_motion() except it returns the SCRIPT_INST of the motion.
//
	SCRIPT_INST start_motion2(const char * script_name, float start_time, float transition_duration, float time_scale, float weight, unsigned int flags, float heading);

//
// start_motion3() is the same as start_motion2() EXCEPT that it takes a part_index and SCRIPT_SET_ARCH as well as a script name.
// The script_name requested MUST BE IN THE SET.
//
	SCRIPT_INST start_motion3(	SCRIPT_SET_ARCH set, const char * script_name, float start_time, float transition_duration,
								float time_scale = 1.0f, float weight = 1.0f, 
								unsigned int flags = Animation::FORWARD, float heading = -1.0f);

	SCRIPT_INST start_motion_locked(	SCRIPT_SET_ARCH set, const char * script_name,
										float start_time,
										float transition_duration,
										float time_scale,
										float weight,
										unsigned int flags,
										float heading,
										INSTANCE_INDEX locked_bone);

	void stop_motion(void);

	HANDLE	start_aim(const AimDesc & desc, float transition_duration);
	void	end_aim(HANDLE aim);

	void pause(void);
	void resume(void);

// You MUST call deform() to compute the current deformed vertex positions before rendering.
	void deform(const int * mesh_index);

// render() DOESN'T UPDATE VERTEX POSITIONS ANY MORE. Also, it's in your interest to call deform() 
// followed by render() using the SAME list of mesh indices.
// the tessellation_cnt is just a temporary hack; valid range is 1 and up
	void render(struct ICamera * camera, const int * mesh_index, int tessellation_cnt = 1);

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

	CHANNEL_ARCHETYPE_INDEX add_channel(const char * script_name, const char * channel_name);
	void replace_channel_data(const char * script_name, CHANNEL_ARCHETYPE_INDEX channel_arch, const IChannel::Header& header_replacement, const void* data_replacement, size_t frame_size_replacement, float duration_replacement);

	U32 get_num_active_scripts(void);
	void describe_active_scripts(ActiveScriptDesc * desc);

	U32 get_num_hardpoints(void) const;
	const HardpointDesc * get_hardpoints(void) const;

	void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator);
    void COMAPI on_finished(unsigned int channel_id, void * user_supplied);
	void COMAPI on_loop(unsigned int channel_id, Transform & T, void * user_supplied);

	BOOL32 COMAPI intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, const int * mesh_index) const;

	void sort_faces_by_area(const int part_index, const int mesh_index);
};
				
//

#endif