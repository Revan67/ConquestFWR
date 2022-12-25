//
// MODEL component.
//

#pragma warning (disable : 4786 4530 )

//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>
#include <map>

//

#include "dacom.h"
#include "tcomponent.h"
#include "TSmartPointer.h"
#include "SysConsumerDesc.h"
#include "3dmath.h"
#include "fdump.h"
#include "da_heap_utility.h"
#include "stddat.h"
#include "engine.h"
#include "engcomp.h"
#include "model.h"
#include "FileSys.h"
#include "ICamera.h"

//

#include "handlemap.h"
#include "tfuncs.h"

//

#include "Compound.h"

//

#define LAZY_UPDATE 1

//

// #define DEBUG_VERBOSE for lots of debug output.
//#define DEBUG_VERBOSE
//#define DEBUG_ARCHETYPE_REF_CNT

void DEBUG_printf (char *fmt, ...)			
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		GENERAL_TRACE_1 (work);
	}
}

//

ICOManager *DACOM = NULL;		// Handle to component manager


//

struct Tree;

//

struct Instance
{
// These pointers keep the Model tree linkage information.
	INSTANCE_INDEX	parent;
	INSTANCE_INDEX	child;
	INSTANCE_INDEX	sibling;

	//when deleting an instance call archetype->release ()
	//and then use 'root_archetype' to see if the entire
	//compound archetype can be deleted.

	ARCHETYPE_INDEX root_archetype;
	Compound::Archetype* archetype;

	//EMAURER all instances in a particular heirarchy point to the same tree.

	Tree *		tree;

	int			index;

	float		centered_cmp_radius;	// radius relative to a bounding sphere center below
	Vector		center;					// center (in local coord system) about which radius is computed

#if LAZY_UPDATE
	Transform	last_xform;				// used to cache the last xform (only for the root instance)
	bool		dirty_xform;			// if true then we need updating relative to parent
#endif

	bool		active;


	void reset (void)
	{
		root_archetype = INVALID_ARCHETYPE_INDEX;
		parent	=
		child	=
		sibling	= INVALID_INSTANCE_INDEX;

		archetype = NULL;
		tree	= NULL;

		index	= -1;
		active	= false;

		centered_cmp_radius = -1.0f;
		center.zero();
#if LAZY_UPDATE
		dirty_xform = true;
		last_xform.set_identity();
#endif
	}

	Instance(void)
#if LAZY_UPDATE
		: last_xform(false)
#endif
	{
		reset ();
	}

	~Instance(void)
	{
		archetype = NULL;
	}

	inline bool is_connected(void) const
	{
		return ((parent != INVALID_INSTANCE_INDEX) || (child != INVALID_INSTANCE_INDEX));
	}
};

//

//
// JointNode is used by HashPool.
//
struct JointNode
{
	Joint		joint;

// HashPool stuff.
	U32			hash_key;
	JointNode *	hash_next;   
	JointNode *	hash_prev;  
	JointNode *	next;          
	JointNode *	prev;       
	S32			index;

	static U32 hash(const void * obj)
	{
		Joint * j = (Joint *) obj;
		int small, large;
		if (j->parent < j->child)
		{
			small = j->parent;
			large = j->child;
		}
		else
		{
			small = j->child;
			large = j->parent;
		}

		// 5 bits each = 1024 elements
		return ((small & 0x1f) << 5) + (large & 0x1f);
	}

	BOOL32 compare(const void * obj)
	{
		Joint * j = (Joint *) obj;
		const int in1		= j->parent;
		const int out1	= j->child;

		const int in2		= joint.parent;
		const int out2	= joint.child;

		BOOL32 result = ( (in1 == in2)  && (out1 == out2)) ||
						 ((in1 == out2) && (out1 == in2));

		return result;
	}

	void initialize(const void * obj)
	{
		Joint * j = (Joint *) obj;

		memcpy(&joint, j, sizeof(joint));

		joint.type		= j->type;
		joint.parent	= j->parent;
		joint.child		= j->child;
	}

	void shutdown(void) 
	{
	#ifndef NDEBUG
		memset (&joint, 0xCD, sizeof (joint));
	#endif
	}
	void display(void) {}
};

//

typedef HashPool<JointNode, 48, 1024>	JointPool;

//

struct JointLink
{
	//EMAURER references to HashPool elements cannot be ptrs as
	//HashPool elements may be moved without notification.
	//JointNode *	joint;
	unsigned int joint;

	JointLink *	prev;
	JointLink *	next;
};

//

typedef LList<JointLink> JointList;

//

struct Tree
{
	INSTANCE_INDEX	root;
	JointList		joints;

	Tree *			prev;
	Tree *			next;


	~Tree(void)
	{
		joints.free();
	}
};

//

typedef LList<Tree> TreeList;

typedef std::vector<INSTANCE_INDEX> INST_VECTOR;

typedef inst_handlemap< Instance >				CompoundInstanceMap;
typedef arch_handlemap< Compound::Archetype* >	CompoundArchetypeMap;

//

struct DACOM_NO_VTABLE MODEL : public IEngineComponent, public IModel
{
	//emaurer:  the order of these statements matters.  The template
	//code that implements CreateInstance () expects the name of the object
	//that is being created to be the first in the list.

	BEGIN_DACOM_MAP_INBOUND(MODEL)
	DACOM_INTERFACE_ENTRY(IModel)
	DACOM_INTERFACE_ENTRY2(IID_IModel, IModel)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent, IEngineComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	END_DACOM_MAP()

//
// Implementation-specific members.
//
	IEngine *	engine;
	JointPool	joints;
	TreeList	trees;

	mutable CompoundArchetypeMap Archetypes;
	mutable CompoundInstanceMap Instances;	

	void init_tree(Tree * tree, INSTANCE_INDEX obj);
	void order_joints(unsigned int* dst, INSTANCE_INDEX root, int & index, JointPool & joints);
	void set_tree(INSTANCE_INDEX obj, Tree * tree);
	void update_tree(Tree * tree);
	void update_joint_child(const Joint * j);

	void add_joints(Tree * tree, INSTANCE_INDEX root);
	void rebuild_tree(Tree * tree, INSTANCE_INDEX member);

	MODEL(void);
	~MODEL(void);

	DA_HEAP_DEFINE_NEW_OPERATOR(MODEL);

	GENRESULT init (SYSCONSUMERDESC * info) 
	{ 
		return GR_OK; 
	}

	inline IDAComponent * get_base(void)
	{
		return (IEngineComponent *) this;
	}

	virtual GENRESULT	COMAPI Initialize(void);

	// IEngineComponent
	BOOL32 COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void COMAPI	duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	BOOL32 COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
	void COMAPI update(SINGLE dt) ;

// Model-specific functions.
	virtual void			COMAPI update_tree(INSTANCE_INDEX root);
	virtual void			COMAPI sync_instance (INSTANCE_INDEX idx);

	virtual bool			COMAPI create_compound_archetype (ARCHETYPE_INDEX idx, IFileSystem * fs);

	virtual bool			COMAPI archetype_is_compound(ARCHETYPE_INDEX idx) const;
	virtual INSTANCE_INDEX	COMAPI create_compound_instance(ARCHETYPE_INDEX idx);
	virtual INSTANCE_INDEX	COMAPI create_compound_instance2(ARCHETYPE_INDEX idx, IEngineInstance * userInstance);

	virtual BOOL32 COMAPI connect(const Joint * info);
	virtual BOOL32 COMAPI disconnect(INSTANCE_INDEX obj1, INSTANCE_INDEX obj2);

	virtual INSTANCE_INDEX COMAPI get_next_root_object(INSTANCE_INDEX prev_root = -1);

	virtual INSTANCE_INDEX	COMAPI get_root(INSTANCE_INDEX index) const;
	virtual INSTANCE_INDEX	COMAPI get_parent(INSTANCE_INDEX child) const;
	virtual INSTANCE_INDEX	COMAPI get_child(INSTANCE_INDEX parent, INSTANCE_INDEX prev_child = -1) const;

	virtual GENRESULT COMAPI enumerate_arch_connections( ARCHETYPE_INDEX index,
		void (__cdecl *callback)(ARCHETYPE_INDEX parent, ARCHETYPE_INDEX child, void *user_data),
		void *user_data) const;
	virtual GENRESULT COMAPI enumerate_arch_children( ARCHETYPE_INDEX index,
		void (__cdecl *callback)(ARCHETYPE_INDEX child, void *user_data),
		void *user_data) const;

	virtual INSTANCE_INDEX	COMAPI traverse_roots(INSTANCE_INDEX prev_root = -1) const;
	virtual JOINT_INDEX COMAPI traverse_joints(INSTANCE_INDEX root, JOINT_INDEX prev_joint = -1);


	virtual JOINT_INDEX COMAPI find_joint(INSTANCE_INDEX obj1, INSTANCE_INDEX obj2) const;
	virtual JointType	COMAPI get_joint_type(JOINT_INDEX index) const;

	virtual S32		COMAPI get_joint_data_size(JointType type) const;

	virtual void	COMAPI get_joint_data(JOINT_INDEX index, SINGLE * dst) const;
	virtual void	COMAPI set_joint_data(JOINT_INDEX index, const SINGLE * src);

	virtual const Joint * COMAPI get_joint(JOINT_INDEX index) const;

	virtual BOOL32 COMAPI connected(INSTANCE_INDEX i1, INSTANCE_INDEX i2) const;

	virtual const C8* COMAPI get_name (INSTANCE_INDEX obj) const;

	virtual INSTANCE_INDEX get_joint_parent(JOINT_INDEX jnt) const;
	virtual INSTANCE_INDEX get_joint_child(JOINT_INDEX jnt) const;

	void DEBUG_dump_state(const char * where = NULL);

	bool UnlinkChild(INSTANCE_INDEX parent, INSTANCE_INDEX child);

	void dump_tree(INSTANCE_INDEX root, int level);

	void dirty_compound_radius(INSTANCE_INDEX idx);
	void get_compound_radius(INSTANCE_INDEX idx, float * radius, Vector * center);

	inline void dirty_xform(INSTANCE_INDEX index);

	inline Instance * GetInstance (const INSTANCE_INDEX idx) const;
	
	protected:

	inline JointNode* get_joint (unsigned int i) const
	{
		return joints.list + i;
	}

	int find_joint (const Tree* t, INSTANCE_INDEX parent, INSTANCE_INDEX child) const;
	void destroy_joints (Tree* tree, INSTANCE_INDEX root);
	//EMAURER remove all child/sibling connections for this index.
	void unhook (INSTANCE_INDEX idx, INST_VECTOR& dead);
	void destroy_joints_and_tree (Tree* tree);
};

//
// DLL stuff.
//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	IComponentFactory *server;

	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<MODEL>, SYSCONSUMERDESC> ("IModel");

			if (server == NULL)
			{
				break;
			}

			DACOM = DACOM_Acquire();

		// EMAURER can't imagine that priority matters anymore.

			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, "IModel", DACOM_NORMAL_PRIORITY);
			}

			server->Release();
			break;

	//
	// DLL_PROCESS_DETACH: Release DACOM manager instance
	//
		case DLL_PROCESS_DETACH:

			if (DACOM != NULL)
			{
				DACOM->Release();
				DACOM = NULL;
			}
			break;
	}

	return TRUE;
}

//

MODEL::MODEL(void) 
{
}

//

MODEL::~MODEL(void)
{
	engine = NULL;	// can not call back into ENGINE during destructor!!

	CompoundInstanceMap::iterator inst;
	CompoundArchetypeMap::iterator arch;

	while( (inst = Instances.begin()) != Instances.end() ) {
		destroy_instance( (*inst).first );
	}

	while( (arch = Archetypes.begin()) != Archetypes.end() ) {
		destroy_archetype( (*arch).first );
	}
}

//
// this is used very often; so if dynamic array goes away it should be replaced w/ something 
// that keeps the access time constant
inline Instance * MODEL::GetInstance(const INSTANCE_INDEX idx) const
{
	if( idx == INVALID_INSTANCE_INDEX ) {
		return NULL;
	}

	CompoundInstanceMap::iterator inst;

	if( (inst = Instances.find( idx )) == Instances.end() ) {
		return NULL;
	}

	Instance *inst_ptr = &inst->second;

	return (inst_ptr->active)? inst_ptr : NULL;
}

//

GENRESULT COMAPI MODEL::Initialize(void)
{
	GENRESULT result;

	if (get_base()->QueryInterface(IID_IEngine, (void **) &engine) == GR_OK)
	{
		get_base()->Release();
		result = GR_OK;
	}
	else
	{
		result = GR_INTERFACE_UNSUPPORTED;
	}

	return result;
}

//
// Convert all relative coordinates to absolute Cartesian coordinates.
//
void COMAPI MODEL::update(SINGLE dt)
{
#if LAZY_UPDATE
	INSTANCE_INDEX idx = -1;
	do
	{
		idx = get_next_root_object(idx);
		update_instance(idx, dt);

	} while (idx != -1);
#else
	INSTANCE_INDEX idx = -1;
	do
	{
		idx = get_next_root_object(idx);

		Instance * obj = GetInstance(idx);
		if (obj && obj->is_connected())
		{
		// Update object's tree.
			update_tree(obj->tree);
		}

	} while (idx != -1);
#endif
}

// void COMAPI MODEL::update_tree(INSTANCE_INDEX root) should now go away
void COMAPI MODEL::update_instance (INSTANCE_INDEX idx, SINGLE dt)
{
#if LAZY_UPDATE
	Instance* i = GetInstance (idx);

	if( i && i->parent == INVALID_INSTANCE_INDEX ) // root
	{
		const Transform & current = engine->get_transform( idx );

		if( !i->last_xform.equal( current, .00001f ) )
		{
			i->last_xform = current;
			dirty_xform( i->child );
		}
	}
#else
	// NOTE: dt is ignored here, but required by the interface.
	Instance * obj = GetInstance(idx);
	if (obj && obj->is_connected())
	{
		// Update object's tree.
		update_tree(obj->tree);
	}	
#endif
}

//

GENRESULT COMAPI MODEL::query_archetype_interface( ARCHETYPE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

GENRESULT COMAPI MODEL::query_instance_interface( INSTANCE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

enum vis_state COMAPI MODEL::render_instance( struct ICamera *, INSTANCE_INDEX , float , U32 , const Transform * ) 
{
	return VS_UNKNOWN;
}


//

BOOL32 COMAPI MODEL::connect(const Joint * info)
{
	ASSERT( info );

	BOOL32 result = 0;

	ASSERT(info->parent != info->child);

	Instance * obj1 = GetInstance(info->parent);
	Instance * obj2 = GetInstance(info->child);

// IF EITHER of the instance pointers is NULL, create a NULL OBJECT
// in order to deal with connections correctly.

	if (obj1 && obj2)
	{
		ASSERT(obj2->parent == INVALID_INSTANCE_INDEX);	// An object can have at most one parent.

		if (obj1->child != INVALID_INSTANCE_INDEX)
		{
		// obj1 has previous children. Add obj2 to list.
			obj2->sibling = obj1->child;
			obj1->child = info->child;
		}
		else
		{
		// obj1 has no previous children.
			obj1->child = info->child;
			obj2->sibling = -1;
		}

		obj2->parent = info->parent;

		int joint_index;

		switch (info->type)
		{
			case JT_FIXED:
			case JT_REVOLUTE:
			case JT_PRISMATIC:
			case JT_DAMPED_SPRING:
			case JT_CYLINDRICAL:
			case JT_SPHERICAL:
			case JT_TRANSLATIONAL:
			case JT_LOOSE:
				joint_index = joints.allocate(info);
				result = 1;
				break;

			default:
				break;
		}

	//
	// Create or update kinematic tree.
	//
		Tree * tree;

		if (obj1->tree)
		{
			if (obj2->tree)
			{
			// Make sure this connection doesn't create a kinematic loop.
				ASSERT(obj1->tree != obj2->tree);

			// Merge trees.
				tree = trees.alloc();

			// Add all joints from the 2 existing trees to the new tree.
				Tree * src_tree = obj1->tree;
				JointLink * node = src_tree->joints.first();
				while (node)
				{
					src_tree->joints.unlink(node);

					JointLink * next = node->next;
					tree->joints.link(node);
					node = next;
				}

				src_tree = obj2->tree;
				node = src_tree->joints.first();
				while (node)
				{
					src_tree->joints.unlink(node);

					JointLink * next = node->next;
					tree->joints.link(node);
					node = next;
				}

			// Delete old trees.
				trees.free(obj1->tree);
				trees.free(obj2->tree);

				obj1->tree = tree;
				obj2->tree = tree;

			// Find root, recursively set tree pointers.
				Instance * rt = obj1;
				while (rt->parent != INVALID_INSTANCE_INDEX)
				{
					rt = GetInstance(rt->parent);
				}

				set_tree(rt->index, tree);
			}
			else
			{
			// Use obj1's tree.
				tree = obj1->tree;
				obj2->tree = tree;
			}
		}
		else if (obj2->tree)
		{
		// Use obj2's tree.
			tree = obj2->tree;
			obj1->tree = tree;
		}
		else
		{
		// Create new tree
			tree = trees.alloc();
			obj1->tree = tree;
			obj2->tree = tree;
		}

	// Add joint to tree.
		JointLink * jn = tree->joints.alloc();
		jn->joint = joint_index;

		init_tree(tree, info->parent);

#ifdef DEBUG_VERBOSE
DEBUG_dump_state("CONNECT");
#endif
		if (result)
		{
			dirty_compound_radius (info->parent);
			dirty_xform(info->child);
			update_tree(info->parent);
		}
	}

	return result;
}

//

bool MODEL::UnlinkChild(INSTANCE_INDEX parent, INSTANCE_INDEX child)
{
	bool result;

	INSTANCE_INDEX c = GetInstance(parent)->child;
	INSTANCE_INDEX prev = INVALID_INSTANCE_INDEX;
	while ((c != INVALID_INSTANCE_INDEX) && (c != child))
	{
		prev = c;
		c = GetInstance(c)->sibling;
	}

	if (c != INVALID_INSTANCE_INDEX)
	{
		if (prev != INVALID_INSTANCE_INDEX)
		{
			GetInstance(prev)->sibling = GetInstance(c)->sibling;
		}
		else
		{
			GetInstance(parent)->child = GetInstance(c)->sibling;
		}

		GetInstance(c)->parent = INVALID_INSTANCE_INDEX;
		GetInstance(c)->sibling = INVALID_INSTANCE_INDEX;
		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}

//

int MODEL::find_joint (const Tree* t, INSTANCE_INDEX parent, INSTANCE_INDEX child) const
{
	int result = -1;

	Joint test(false); // avoid init constructor
	test.parent = parent;
	test.child	= child;

	JointLink * node = t->joints.first();

	while (node)
	{
		if (joints.list[node->joint].compare (&test))
		{
			result = node->joint;
			break;
		}
		node = node->next;
	}

	return result;
}

//

//EMAURER destroys all joints below this index. Assumes child/sibling links correct.
void MODEL::destroy_joints (Tree* tree, INSTANCE_INDEX root)
{
	INSTANCE_INDEX child = GetInstance(root)->child;
	while (child != INVALID_INSTANCE_INDEX)
	{
		JOINT_INDEX idx = find_joint(tree, root, child);
		ASSERT (idx != -1);
		joints.unlink (idx);

		destroy_joints (tree, child);

		child = GetInstance(child)->sibling;
	}
}

//

//EMAURER remove all child/sibling connections for this index.
void MODEL::unhook (INSTANCE_INDEX idx, INST_VECTOR& dead)
{
	INSTANCE_INDEX c = GetInstance (idx)->child;
	while (c != INVALID_INSTANCE_INDEX)
	{
		INSTANCE_INDEX next = GetInstance(c)->sibling;

		UnlinkChild (idx, c);

		dead.push_back (c);

		unhook (c, dead);

		c = next;
	}
}

//

void MODEL::destroy_joints_and_tree (Tree* tree)
{
	JointLink * node = tree->joints.first();
	while (node)
	{
		JointLink* next = node->next;
		joints.unlink(node->joint);
		node = next;
	}

// Tree goes away.
	trees.free(tree);
}

//

void MODEL::destroy_instance (INSTANCE_INDEX index)
{
	//EMAURER could be done with recursion and disconnect (), but ends up doing 
	//substantially more work than necessary rebuilding trees that are immediately
	//destroyed. This attempts to destroy all of the model's linkage info for
	//the entire subtree of 'index' and then call Engine::destroy_instance () for all
	//of the disconnected children.

// Destroy instance and all its children.
	if (index != INVALID_INSTANCE_INDEX)
	{
		Instance * inst = GetInstance(index);

		if (inst && inst->active)
		{
			//EMAURER set of children of this index.
			INST_VECTOR to_die;

			//EMAURER destroy joints in a manner that minimizes searching joint database.

			//EMAURER is object connected at all?
			if (inst->tree)
			{
			// If object has parent, disconnect from parent.

				Tree* tree = inst->tree;

				if (inst->parent != INVALID_INSTANCE_INDEX)
				{
					INSTANCE_INDEX parent_idx = inst->parent;

					UnlinkChild(parent_idx, index);

					Instance* parent = GetInstance (parent_idx);

					//EMAURER recompute parent's tree if necessary.

					if (parent->is_connected())
					{
						//EMAURER the tree contains joints other than those that are to
						//be destroyed.

						//break parent/index joint

						int jnt = find_joint (tree, parent_idx, index);
						ASSERT (jnt != -1);
						joints.unlink (jnt);

						//EMAURER find all of the child joints that must be destroyed.
						//They shoud be in the tree's list. avoid searching entire joint database.

						if (inst->is_connected())
							destroy_joints (tree, index);

						//EMAURER connection tree must be rebuilt for parent, as it is still connected
						rebuild_tree(tree, parent_idx);
					}
					else
					{
						//EMAURER just severed only sub-branch from parent.
						//Perfect. Tree contains exactly the joints that 
						//must be removed from the master joint list.
						ASSERT (parent_idx == parent->tree->root);
						destroy_joints_and_tree (tree);
						parent->tree = NULL;
					}

					dirty_compound_radius (parent_idx);
				}
				else
				{
					//EMAURER best case. this is a root, destroy all joints in the tree.
					ASSERT (inst->tree->root == index);
					destroy_joints_and_tree (tree);
				}

//EMAURER assume that all joints connecting the branch are destroyed and that the 
//remaining tree, if necessary, has been rebuilt.

				set_tree (index, NULL);

//EMAURER destroy child/sibling ptrs for this index and all below.
//simultaneously build a list of children of this index for destroying later.

				unhook (index, to_die);
			}

			//emaurer
			//reduce reference count if this instance refers to a compound archetype

			if (inst->archetype)
			{
				//emaurer
				//now that this instance no longer refers to the compound archetype
				//check to see if it should be deleted.

				ASSERT (INVALID_ARCHETYPE_INDEX != inst->root_archetype);

			#ifdef DEBUG_ARCHETYPE_REF_CNT
				DEBUG_printf ("Model: releasing root archetype %d\n", inst->root_archetype);
			#endif

				if (engine)
					engine->release_archetype (inst->root_archetype);
			}

			inst->reset ();

			//EMAURER destroy list of children. all Model calls by other components
			//should behave correctly. all children are disconnected. 

			for (INST_VECTOR::const_iterator it = to_die.begin ();
				it != to_die.end ();
				it++)
			{
				if (engine)
					engine->destroy_instance (*it);
			}

			Instances.erase( index );
		}
	}
}

BOOL32 COMAPI MODEL::disconnect(INSTANCE_INDEX obj1, INSTANCE_INDEX obj2)
{
	BOOL32 result;

	Instance * object1 = GetInstance(obj1);
	Instance * object2 = GetInstance(obj2);

	if (object1 && object2)
	{
		if (object2->parent == obj1)
		{
			result = UnlinkChild(obj1, obj2);
			if(result)
			{
				dirty_compound_radius (obj1);
			}
		}
		else if (object1->parent == obj2)
		{
			result = UnlinkChild(obj2, obj1);
			if(result)
			{
				dirty_compound_radius (obj2);
			}
		}
		else
		{
		// Objects are not directly connected.
			result = false;
		}
	//
	// Need to remove any joints connecting the 2 objects as well.
	//
		if (result)
		{
			Tree * tree = object1->tree;

			if (object1->is_connected())
			{
				rebuild_tree(tree, obj1);

				if (object2->is_connected())
				{
				// Now have 2 trees instead of 1.
				// Create new tree and update everyone appropriately.
					Tree * new_tree = trees.alloc();
					rebuild_tree(new_tree, obj2);
				}
				else
				{
					object2->tree = NULL;
				}
			}
			else if (object2->is_connected())
			{
				rebuild_tree(tree, obj2);
				object1->tree = NULL;
			}
			else
			{
			// Tree goes away.
				trees.free(tree);
				object1->tree = 
				object2->tree = NULL;
			}

		// Find joint connecting two objects.
			Joint test(false);
			test.parent = obj1;
			test.child	= obj2;
			int index = joints.search(&test);

			ASSERT(index != -1);

		// Now remove joint from the master pool.
			joints.unlink(index);
		}
	}
	else
	{
		result = false;
	}

#ifdef DEBUG_VERBOSE
DEBUG_dump_state("DISCONNECT");
#endif

	return result;
}

//

INSTANCE_INDEX COMAPI MODEL::get_next_root_object( INSTANCE_INDEX prev_root )
{
	CompoundInstanceMap::iterator ibeg;
	CompoundInstanceMap::iterator iend = Instances.end();
	CompoundInstanceMap::iterator inst;

	ibeg = Instances.begin();

	if( prev_root != INVALID_INSTANCE_INDEX ) {
		if( (inst = Instances.find( prev_root )) != iend ) {
			inst++;
			ibeg = inst;
		}
	}

	for( inst = ibeg; inst != iend; inst++ ) {
		if( inst->second.tree && inst->second.tree->root == inst->first ) {
			return inst->first;
		}
	}

	return INVALID_INSTANCE_INDEX;
}

//

GENRESULT COMAPI MODEL::enumerate_arch_connections( ARCHETYPE_INDEX arch_index,
					void (__cdecl *callback)(ARCHETYPE_INDEX parent, ARCHETYPE_INDEX child, void *user_data),
					void *user_data) const
{
	ASSERT( callback );
	GENRESULT result = GR_INVALID_PARMS;

	CompoundArchetypeMap::iterator arch;

	if( arch_index != INVALID_ARCHETYPE_INDEX ) {
		
		if( (arch = Archetypes.find( arch_index )) != Archetypes.end() ) {

			const Compound::Archetype *arch_ptr = arch->second;
			const Compound::CNXNDEF_LIST & cons = arch_ptr->get_connections();

			for(Compound::CNXNDEF_LIST::const_iterator con_it = cons.begin ();
				con_it != cons.end ();
				con_it++)
			{					
					Compound::Archetype * parent = con_it->parent;
					Compound::Archetype * child = con_it->child;

					callback( parent->get_archetype_index(), child->get_archetype_index(), user_data );
			}

			result = GR_OK;
		}
	}

	return result;
}

//

GENRESULT COMAPI MODEL::enumerate_arch_children( ARCHETYPE_INDEX index,
					void (__cdecl *callback)(ARCHETYPE_INDEX child, void *user_data),
					void *user_data) const
{
	ASSERT( callback );
	GENRESULT result = GR_INVALID_PARMS;

	if (index != INVALID_ARCHETYPE_INDEX)
	{
		CompoundArchetypeMap::iterator iter = Archetypes.find (index);

		if( iter != Archetypes.end () ) // compound
		{
			const Compound::Archetype *arch = iter->second;

			const Compound::ARCHETYPE_LIST & arch_list = arch->get_children();

			for (Compound::ARCHETYPE_LIST::const_iterator child_it = arch_list.begin ();
					child_it != arch_list.end ();
					child_it++)
			{
				ARCHETYPE_INDEX child = (*child_it)->get_archetype_index ();

				callback( child, user_data );
			}

			result = GR_OK;
		}
	}

	return result;
}

//

INSTANCE_INDEX COMAPI MODEL::get_root(INSTANCE_INDEX index) const
{
	INSTANCE_INDEX root;
	const Instance * inst = GetInstance(index);
	if (inst)
	{
		if (inst->tree)
		{
			root = inst->tree->root;
		}
		else
		{
			root = index;
		}
	}
	else
	{
		root = -1;
	}
	return root;
}

//

INSTANCE_INDEX COMAPI MODEL::get_parent(INSTANCE_INDEX child) const
{
	INSTANCE_INDEX result;
	const Instance * inst = GetInstance(child);
	if (inst)
	{
		result = inst->parent;
	}
	else
	{
		result = -1;
	}

	return result;
}

//

INSTANCE_INDEX COMAPI MODEL::get_child(INSTANCE_INDEX parent, INSTANCE_INDEX prev_child) const
{
	INSTANCE_INDEX result;

	Instance * p = GetInstance(parent);
	if (p)
	{
		Instance * prev = GetInstance(prev_child);

		result = (prev == NULL) ? p->child : prev->sibling;
	}
	else
	{
		result = -1;
	}

	return result;
}

//

const C8* COMAPI MODEL::get_name (INSTANCE_INDEX obj) const
{
	const C8* result = NULL;

	const Instance * p = GetInstance(obj);

	if (p && p->archetype)
	{
		result = p->archetype->get_label();
	}

	return result;
}

//

INSTANCE_INDEX COMAPI MODEL::traverse_roots( INSTANCE_INDEX prev_root ) const
{
	CompoundInstanceMap::iterator ibeg;
	CompoundInstanceMap::iterator iend = Instances.end();
	CompoundInstanceMap::iterator inst;

	ibeg = Instances.begin();

	if( (prev_root != INVALID_INSTANCE_INDEX) ) {
		if( (inst = Instances.find( prev_root )) != iend ) {
			inst++;
			ibeg = inst;
		}
	}

	for( inst = ibeg; inst != iend; inst++ ) {
		if( inst->second.tree ) {
			if( inst->second.tree->root == inst->first ) {
				return inst->first;
			}
		}
		else {
			return inst->first;
		}
	}

	return INVALID_INSTANCE_INDEX;
}

//

JOINT_INDEX COMAPI MODEL::traverse_joints(INSTANCE_INDEX root, JOINT_INDEX prev_joint)
{
	JOINT_INDEX result = -1;

	Instance * obj = GetInstance(root);
	if (obj && obj->tree)
	{
		Tree * tree = obj->tree;
		JointLink * link = tree->joints.first();
		JOINT_INDEX ji = get_joint (link->joint)->index;

		if (prev_joint == -1)
		{
			result = ji;
		}
		else
		{
			while (link)
			{
				ji = get_joint (link->joint)->index;
				if (ji == prev_joint)
				{
					JointLink * next = link->next;
					if (next)
					{
						result = get_joint (next->joint)->index;
					}
					else
					{
						result = -1;
					}

					break;
				}

				link = link->next;
			}
		}
	}

	return result;
}

//

JOINT_INDEX COMAPI MODEL::find_joint(INSTANCE_INDEX obj1, INSTANCE_INDEX obj2) const
{
	ASSERT(obj1 != -1);
	ASSERT(obj2 != -1);

	Joint jnt(false);
	jnt.parent = obj1;
	jnt.child = obj2;
	S32 idx = joints.search(&jnt);
	return idx;
}

//

JointType COMAPI MODEL::get_joint_type(JOINT_INDEX index) const
{
	ASSERT(index != -1);
	JointNode * node = joints.list + index;
	return node->joint.type;
}

//

const Joint * COMAPI MODEL::get_joint(JOINT_INDEX index) const
{
	ASSERT(index != -1);
	JointNode * node = joints.list + index;
	return &node->joint;
}

//

S32	COMAPI MODEL::get_joint_data_size(JointType type) const
{
	S32 result;
	switch (type)
	{
		case JT_REVOLUTE:
		case JT_PRISMATIC:
			result = 1;
			break;

		case JT_CYLINDRICAL:
			result = 2;
			break;

		case JT_SPHERICAL:
			result = 4;
			break;

		case JT_TRANSLATIONAL:
			result = 3;
			break;

		case JT_LOOSE:
			result = 7;
			break;

		default:
			result = 0;
			break;
	}

	return result;
}

//

void COMAPI MODEL::get_joint_data(JOINT_INDEX index, SINGLE * dst) const
{
	ASSERT( dst );
	if (index != -1)
	{
		JointNode * node = joints.list + index;
		Joint * j = &node->joint;

		switch (j->type)
		{
			case JT_REVOLUTE:
			case JT_PRISMATIC:
				*dst = j->q;
				break;

			case JT_CYLINDRICAL:
				*dst = j->p;
				*(dst + 1) = j->r;
				break;

			case JT_SPHERICAL:
				*(dst++) = j->w;
				*(dst++) = j->x;
				*(dst++) = j->y;
				*(dst++) = j->z;
				break;

			case JT_TRANSLATIONAL:
				*(dst++) = j->px;
				*(dst++) = j->py;
				*(dst++) = j->pz;
				break;

			case JT_LOOSE:
				*(dst++) = j->px;
				*(dst++) = j->py;
				*(dst++) = j->pz;
				*(dst++) = j->w;
				*(dst++) = j->x;
				*(dst++) = j->y;
				*(dst++) = j->z;
				break;

			default:
				break;
		}
	}
}

//

inline void MODEL::dirty_xform(INSTANCE_INDEX index)
{
#if LAZY_UPDATE
	Instance* i = GetInstance (index);

	if( i )
	{
		if( !i->dirty_xform )
		{
			i->dirty_xform = true;

			dirty_xform( i->child );
		}

		i = GetInstance( i->sibling );
		while( i )
		{
			if( !i->dirty_xform )
			{
				i->dirty_xform = true;
				dirty_xform( i->child );
			}

			i = GetInstance( i->sibling );
		}
	}
#endif
}

//

inline void MODEL::dirty_compound_radius(INSTANCE_INDEX index)
{
	Instance* i = GetInstance (index);

	while( i && (i->centered_cmp_radius >= 0.0f) )
	{
		i->centered_cmp_radius = -1.0f;

		i = GetInstance( i->parent );
	}
}

//

void COMAPI MODEL::set_joint_data(JOINT_INDEX index, const SINGLE * src)
{
	ASSERT( src );

	if (index != -1)
	{
		JointNode * node = joints.list + index;
		Joint * j = &node->joint;

		switch (j->type)
		{
			case JT_FIXED:
				ASSERT( "set_joint_data() called on a JT_FIXED joint!" );
				break;
			case JT_REVOLUTE:
			case JT_PRISMATIC:
				j->q = Tmin (j->max0, Tmax (j->min0, *src));
				dirty_compound_radius(j->parent);
				dirty_xform(j->child);
				break;

			case JT_CYLINDRICAL:
				j->p = *src;
				j->r = *(src + 1);
				dirty_compound_radius(j->parent);
				dirty_xform(j->child);
				break;

			case JT_SPHERICAL:
				j->w = *(src++);
				j->x = *(src++);
				j->y = *(src++);
				j->z = *(src++);
				dirty_compound_radius(j->parent);
				dirty_xform(j->child);
				break;

			case JT_TRANSLATIONAL:
				j->px = *(src++);
				j->py = *(src++);
				j->pz = *(src++);
				dirty_compound_radius(j->parent);
				dirty_xform(j->child);
				break;

			case JT_LOOSE:
				j->px = *(src++);
				j->py = *(src++);
				j->pz = *(src++);
				j->w = *(src++);
				j->x = *(src++);
				j->y = *(src++);
				j->z = *(src++);
				dirty_compound_radius(j->parent);
				dirty_xform(j->child);
				break;

			default:
				break;
		}
	}
}

//
// Put joints in order guaranteeing that each parent will be encountered before
// any of its children.
//
// WARNING: This function calls itself.
//
void MODEL::order_joints(unsigned int* dst, INSTANCE_INDEX root, int & index, JointPool & joints)
{
	INSTANCE_INDEX c = GetInstance(root)->child;
	while (c != INVALID_INSTANCE_INDEX)
	{
		Joint jnt(false);
		jnt.parent	= root;
		jnt.child	= c;

		int jnt_idx = joints.search(&jnt);
		dst[index++] = jnt_idx;

		order_joints(dst, c, index, joints);

		c = GetInstance(c)->sibling;
	}
}

//

void MODEL::set_tree(INSTANCE_INDEX obj, Tree * tree)
{
	Instance * object = GetInstance(obj);
	object->tree = tree;
	INSTANCE_INDEX c = object->child;
	while (c != INVALID_INSTANCE_INDEX)
	{
		set_tree(c, tree);
		c = GetInstance(c)->sibling;
	}
}

//

void MODEL::init_tree(Tree * tree, INSTANCE_INDEX obj)
{
	Instance * object = GetInstance(obj);

// Find the root of the tree.
	Instance * rt = object;
	while (rt->parent != INVALID_INSTANCE_INDEX)
	{
		rt = GetInstance(rt->parent);
	}

	tree->root = rt->index;

// Order joints.
	unsigned int num_joints = 0;
	for (int i = 0; i < joints.list_size; i++)
	{
		if (joints.list[i].index != -1)
		{
			num_joints++;
		}
	}

	if (num_joints)
	{
		unsigned int* joint_order = new unsigned int[num_joints];

		int start_idx = 0;
		order_joints(joint_order, tree->root, start_idx, joints);

	// Now rebuild joint list in correct order.
		unsigned int* fp = joint_order;
		JointLink * node = tree->joints.first();
		while (node)
		{
			node->joint = *(fp++);
			node = node->next;
		}

		delete [] joint_order;
	}
}

//

// redundant now that update_instance() is in the interface
void COMAPI MODEL::update_tree(INSTANCE_INDEX root)
{
#if LAZY_UPDATE
	update_instance(root, 0.0f);
#else
	Instance * inst = GetInstance(root);
	if (inst && inst->is_connected())
	{
		update_tree(inst->tree);
	}
#endif
}

//

void MODEL::sync_instance(INSTANCE_INDEX idx)
{
#if LAZY_UPDATE
	ASSERT( idx != INVALID_INSTANCE_INDEX );

	Instance *i = GetInstance ( idx );

	if( i && i->dirty_xform )
	{
		if( i->parent != INVALID_INSTANCE_INDEX )
		{
			JOINT_INDEX ji = find_joint( i->parent, idx );

			const Joint* j = get_joint( ji );

			update_joint_child( j ); // sets i->dirty_xform to false
		}
		else // root
		{
			i->dirty_xform = false;
		}
	}
#endif
}

//

void MODEL::update_joint_child(const Joint * j)
{
#if LAZY_UPDATE
	// make sure parents are correct first
	sync_instance( j->parent );
#endif

	Instance* i = GetInstance (j->child);
	
#if LAZY_UPDATE
	if( i->dirty_xform )
#endif
	{
		// make a few less call to the engine
		Transform t ( engine->get_transform(j->parent) );
		const Matrix pR ( t );
		const Vector px ( t.translation );
		Matrix & R = t;
		Vector & x = t.translation;

		switch (j->type)
		{
			case JT_FIXED:
			{
				R *= j->rel_orientation;
				x += pR * j->rel_position;

				engine->set_transform(j->child, t);
				break;
			}

			case JT_REVOLUTE:
			{
				const Matrix Rrot ( Quaternion( j->axis, j->q)  );

				R *= Rrot * j->rel_orientation;
				x += pR * j->parent_point - R * j->child_point;

				engine->set_transform(j->child, t);
				break;

			}

			case JT_PRISMATIC:
			{
				R *= j->rel_orientation;
				x += pR * (j->parent_point + (j->axis * j->q)) - R * j->child_point;

				engine->set_transform(j->child, t);
				break;
			}

			case JT_CYLINDRICAL:
			{
			// Combine revolute and prismatic along same axis.
				const Matrix Rrot ( Quaternion(j->axis, j->r) );

				R *= j->rel_orientation * Rrot;
				x += pR * (j->parent_point + (j->axis * j->p)) - R * j->child_point;

				engine->set_transform(j->child, t);
				break;
			}

			case JT_SPHERICAL:
			{
				const Matrix Rrot ( Quaternion( j->w, j->x, j->y, j->z ) );

				R *= Rrot * j->rel_orientation;
				x += pR * j->parent_point - R * j->child_point;

				engine->set_transform(j->child, t);
				break;
			}

			case JT_TRANSLATIONAL:
			{
				R *= j->rel_orientation;
				x += pR * (j->rel_position + Vector(j->px, j->py, j->pz));
				
				engine->set_transform(j->child, t);
				break;
			}

			case JT_LOOSE:
			{
				const Matrix Rrot ( Quaternion( j->w, j->x, j->y, j->z ) );

				R *= Rrot * j->rel_orientation;
				x += pR * (j->rel_position + Vector(j->px, j->py, j->pz));
			
				engine->set_transform(j->child, t);
				break;
			}

			default:
				break;
		}
#if LAZY_UPDATE
		i->dirty_xform = false;
#endif
	}
}

void MODEL::update_tree(Tree * tree)
{
#if !LAZY_UPDATE
// Compute positions.

	const JointLink * node = tree->joints.first();
	while (node)
	{
		const Joint * j = &(get_joint (node->joint)->joint);

		update_joint_child( j );

		node = node->next;
	}
#endif
}

//

void MODEL::add_joints(Tree * tree, INSTANCE_INDEX root)
{
	INSTANCE_INDEX child = GetInstance(root)->child;
	while (child != INVALID_INSTANCE_INDEX)
	{
		JOINT_INDEX idx = find_joint(root, child);
	// Add joint to tree.
		JointLink * jn = tree->joints.alloc();
		jn->joint = idx;

		add_joints(tree, child);

		child = GetInstance(child)->sibling;
	}
}

//
// Given a single known member of the tree, rebuild it from scratch.
// Best not to do this if you can be a little more clever about which
// joints are being added/removed, but in some cases you've just got to
// punt and rebuild the thing.
//
void MODEL::rebuild_tree(Tree * tree, INSTANCE_INDEX member)
{
	tree->joints.free();

	Instance * m = GetInstance(member);

// Find the root of the tree.
	Instance * rt = m;
	while (rt->parent != INVALID_INSTANCE_INDEX)
	{
		rt = GetInstance(rt->parent);
	}

	tree->root = rt->index;

// Recursively add connected joints to tree.
	add_joints(tree, tree->root);

	init_tree(tree, tree->root);
	set_tree(tree->root, tree);
}


//

#define COMPOUND "Cmpnd"

bool COMAPI MODEL::create_compound_archetype(ARCHETYPE_INDEX index, IFileSystem * file)
{
	ASSERT( file );
	bool result = false;

	if (index != INVALID_ARCHETYPE_INDEX)
	{
		COMPTR<IFileSystem> compound_root;

		char cur_directory[MAX_PATH] = "\\";
		file->GetCurrentDirectory (sizeof (cur_directory), cur_directory);

		DAFILEDESC desc (cur_directory);

		GENRESULT e = file->CreateInstance (&desc, compound_root);

		if (file->SetCurrentDirectory(COMPOUND))
		{
			Compound::Archetype *archetype;
			
			if( (archetype = Compound::Archetype::Build (file, engine, compound_root)) != NULL ) {
				Archetypes.insert( index, archetype );
				result = true;
			}
			else {
				result = false;
			}


			file->SetCurrentDirectory ("..");
		}
	}

	return result;
}

//

BOOL32 COMAPI MODEL::create_archetype( ARCHETYPE_INDEX, IFileSystem * )
{
	return FALSE ;
}

//

void COMAPI	MODEL::duplicate_archetype( ARCHETYPE_INDEX , ARCHETYPE_INDEX ) 
{
	return ;
}

//

void COMAPI MODEL::destroy_archetype(ARCHETYPE_INDEX index)
{
	CompoundArchetypeMap::iterator it = Archetypes.find (index);

	if (it != Archetypes.end ())
	{
		it->second->decouple (engine);
		delete it->second;
		Archetypes.erase (it);
	}
}

//

bool COMAPI MODEL::archetype_is_compound(ARCHETYPE_INDEX index) const
{
	bool result = false;

	if (index != INVALID_ARCHETYPE_INDEX)
		result = (Archetypes.end () != Archetypes.find (index));

	return result;
}

//

INSTANCE_INDEX COMAPI MODEL::create_compound_instance(ARCHETYPE_INDEX index)
{
	return create_compound_instance2 (index, NULL);
}

//

INSTANCE_INDEX COMAPI MODEL::create_compound_instance2(ARCHETYPE_INDEX index, IEngineInstance * userInstance)
{
	INSTANCE_INDEX root = INVALID_INSTANCE_INDEX;

#ifdef DEBUG_ARCHETYPE_REF_CNT
	DEBUG_printf ("Model: enter create_compound_instance\n");
#endif

	if (index != INVALID_ARCHETYPE_INDEX)
	{
		CompoundArchetypeMap::iterator arch_it = Archetypes.find (index);

		if (Archetypes.end () != arch_it)
		{
			root = engine->create_instance2 (arch_it->second->get_archetype_index (), userInstance);
			ASSERT (root != INVALID_INSTANCE_INDEX);

			{
				Instance* i = GetInstance(root);
				ASSERT (i);

				i->archetype = arch_it->second;

				//emaurer
				//stash the root of this archetype tree.  Upon destroy_instance () the archetype 
				//that the instance refers to has it's reference count decremented.  Then it is
				//checked to determine if it can be deleted

				i->root_archetype = index;

			#ifdef DEBUG_ARCHETYPE_REF_CNT
				DEBUG_printf("Model: holding parent archetype\n");
			#endif

				engine->hold_archetype (index);
			}

			const Compound::ARCHETYPE_LIST& child_list = arch_it->second->get_children ();

			unsigned int num_children = child_list.size ();
			
			if (num_children)
			{
				INSTANCE_INDEX* parts = new INSTANCE_INDEX[num_children + 1];	//+1 see below
				ASSERT (parts);

			#ifndef NDEBUG
				int* obj_used = new int[num_children + 1];
				memset (obj_used, 0, sizeof (int) * (num_children + 1));
			#endif

				unsigned int cur = 0;

				for (Compound::ARCHETYPE_LIST::const_iterator it = child_list.begin ();
					it != child_list.end ();
					it++)
				{
					parts[cur] = engine->create_instance2( (*it)->get_archetype_index (), NULL );
					ASSERT (parts[cur] != INVALID_INSTANCE_INDEX);

					Instance* i = GetInstance(parts[cur]);
					ASSERT (i);

					i->archetype = *it;

					//see comment above
					i->root_archetype = index;

				#ifdef DEBUG_ARCHETYPE_REF_CNT
					DEBUG_printf("Model: holding parent archetype\n");
				#endif

					engine->hold_archetype (index);

					cur++;
				}

				//the child list now is used as a list of possible connection targets.
				//since connections are not assumed to be between 'root' and a child, but
				//may be between two children, the root is treated as another connection
				//target.  add the root to the list of possible connection targets.
				parts[num_children] = root;

				//child and root instances concocted. now connect them

				{
					const Compound::CNXNDEF_LIST& connections = arch_it->second->get_connections ();

					for (Compound::CNXNDEF_LIST::const_iterator it = connections.begin ();
						it != connections.end ();
						it++)
					{
						INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
						INSTANCE_INDEX parent = INVALID_INSTANCE_INDEX;

						//find child part

						unsigned int i;

						for (i = 0; i < num_children + 1; i++)
						{
							Instance* instance = GetInstance (parts[i]);
							ASSERT (instance);

							if (!strcmp (it->child->get_label (), instance->archetype->get_label ()))
							{
								child = parts[i];

							#ifndef NDEBUG
								obj_used[i] = !0;
							#endif

								break;
							}	
						}

						for (i = 0; i < num_children + 1; i++)
						{
							Instance* instance = GetInstance (parts[i]);
							ASSERT (instance);

							if (!strcmp (it->parent->get_label (), instance->archetype->get_label ()))
							{
								parent = parts[i];

							#ifndef NDEBUG
								obj_used[i] = !0;
							#endif

								break;
							}	
						}

						ASSERT (child != INVALID_INSTANCE_INDEX);
						ASSERT (parent != INVALID_INSTANCE_INDEX);

						{
							Joint j(it->connection, parent, child);
							BOOL32 con_result = connect(&j);
							ASSERT (con_result);
						}
					}
				}

				delete [] parts;

				//emaurer
				//verify that all instances that were created were connected to something
				//the entire tree may not be connected, but this is a start.

			#ifndef NDEBUG
				{
					for (unsigned int i = 0; i < num_children + 1; i++)
						ASSERT (obj_used[i]);
				}

				delete [] obj_used;
			#endif
			}
		}
	}

#ifdef DEBUG_ARCHETYPE_REF_CNT
	DEBUG_printf ("Model: exit create_compound_instance\n");
#endif

	return root;
}

//
// WE ALSO HAVE TO IMPLEMENT THE NORMAL IEngineComponent::create_instance()
// and destroy_instance() functions.
//
// We don't care about archetypes here. This is just a placeholder used for storing
// connection information.
//

BOOL32 COMAPI MODEL::create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	Instance *instance;
	CompoundInstanceMap::iterator inst;

	if( (instance = GetInstance( inst_index )) == NULL ) {
		if( (inst = Instances.insert( inst_index )) == Instances.end() ) {
			return FALSE;
		}

		instance = &inst->second;
	}

	instance->active = true;
	instance->index = inst_index;

	instance->parent =
	instance->child =
	instance->sibling = INVALID_INSTANCE_INDEX;

	instance->tree = NULL;

	instance->root_archetype = INVALID_ARCHETYPE_INDEX;

	instance->centered_cmp_radius = -1.0f;
	instance->center.zero();

#if LAZY_UPDATE
	instance->dirty_xform = true;
	instance->last_xform.set_identity();
#endif

	return TRUE;
}

//

BOOL32 COMAPI MODEL::connected(INSTANCE_INDEX i1, INSTANCE_INDEX i2) const
{
	BOOL32 result = FALSE;
	if ((i1 != INVALID_INSTANCE_INDEX) && (i2 != INVALID_INSTANCE_INDEX))
	{
		Instance * obj1 = GetInstance(i1);
		Instance * obj2 = GetInstance(i2);

		if (obj1 && obj2)
		{
			if (obj1->tree && (obj1->tree == obj2->tree))
			{
				result = TRUE;
			}
		}
	}

	return result;
}


// center is in local coord system (otherwise it would get invalidated anytime an object moved)
void MODEL::get_compound_radius(INSTANCE_INDEX idx, float * radius, Vector * center)
{
	ASSERT( radius );
	ASSERT( center );

	Instance * root_obj = GetInstance(idx);

	if(root_obj)
	{
		if(root_obj->centered_cmp_radius > 0.0f) // cached
		{
			*radius = root_obj->centered_cmp_radius;
			*center = root_obj->center;
		}
		else // compute
		{
			const Transform & root_trans = engine->get_transform(idx);
			float root_radius;
			Vector root_center;
			engine->get_centered_radius(idx, &root_radius, &root_center);
			root_center = root_trans * root_center;
			
			float min_x = root_center.x - root_radius;
			float max_x = root_center.x + root_radius;
			float min_y = root_center.y - root_radius;
			float max_y = root_center.y + root_radius;
			float min_z = root_center.z - root_radius;
			float max_z = root_center.z + root_radius;
			
			// find a good center
			INSTANCE_INDEX child = root_obj->child;
			while (child != INVALID_INSTANCE_INDEX)
			{
				float c_radius;
				Vector c_center;
				get_compound_radius(child, &c_radius, &c_center);
				ASSERT( c_radius >= 0.0f );
				
				c_center = engine->get_transform(child) * c_center;
				
				min_x = Tmin( min_x, c_center.x - c_radius );
				max_x = Tmax( max_x, c_center.x + c_radius );

				min_y = Tmin( min_y, c_center.y - c_radius );
				max_y = Tmax( max_y, c_center.y + c_radius );

				min_z = Tmin( min_z, c_center.z - c_radius );
				max_z = Tmax( max_z, c_center.z + c_radius );

				Instance * child_obj = GetInstance(child);
				ASSERT(child_obj);
				child = child_obj->sibling;
			}

			*center = .5f * Vector( max_x + min_x, max_y + min_y, max_z + min_z );

			*radius = .5f * Tmax(Tmax(max_x - min_x, max_y - min_y), max_z - min_z);

			// check root radius
			float r_rad = (*center - root_center).magnitude() + root_radius;
			if(r_rad > *radius )
			{
				*radius = r_rad;
			}

			// check child radii
			child = root_obj->child;
			while (child != INVALID_INSTANCE_INDEX)
			{
				float c_radius;
				Vector c_center;
				get_compound_radius(child, &c_radius, &c_center);

				c_radius += ((engine->get_transform(child) * c_center) - *center).magnitude();

				if(c_radius > *radius)
				{
					*radius = c_radius;
				}

				Instance * child_obj = GetInstance(child);
				ASSERT(child_obj);
				child = child_obj->sibling;
			}

			// back to root local
			*center = *center * root_trans;

			// update cache
			root_obj->centered_cmp_radius = *radius;
			root_obj->center = *center;
			
			ASSERT(*radius >= 0.0f);
		}
	}
	else
	{
		*radius = -1.0f;
	}
}

//

void MODEL::dump_tree(INSTANCE_INDEX root, int level)
{
	Instance * root_obj = GetInstance(root);

	INSTANCE_INDEX child = root_obj->child;
	if (child != INVALID_INSTANCE_INDEX)
	{
		char spacer[80];
		int spaces = level << 1;
		memset(spacer, ' ', spaces);
		spacer[spaces] = 0;

		DEBUG_printf("\n%sroot: %d\n", spacer, root);
		DEBUG_printf("%schildren: ", spacer);

		while (child != INVALID_INSTANCE_INDEX)
		{
			DEBUG_printf("%d ", child);

			Instance * child_obj = GetInstance(child);
			child = child_obj->sibling;
		}

		DEBUG_printf("\n");

		child = root_obj->child;
		while (child != INVALID_INSTANCE_INDEX)
		{
			dump_tree(child, level + 1);

			Instance * child_obj = GetInstance(child);
			child = child_obj->sibling;
		}
	}
}

//

void MODEL::DEBUG_dump_state(const char * where)
{
	if (where != NULL)
	{
		DEBUG_printf("\n%s\n", where);
	}

	Tree * tree = trees.first();
	while (tree)
	{
		DEBUG_printf("\nTREE: root %d, %d joints\n", tree->root, tree->joints.count());

		dump_tree(tree->root, 0);

		tree = tree->next;
	}
}

//

INSTANCE_INDEX MODEL::get_joint_parent(JOINT_INDEX jnt) const
{
	INSTANCE_INDEX result = INVALID_INSTANCE_INDEX;
	if (jnt != INVALID_JOINT_INDEX)
	{
		JointNode * node = joints.list + jnt;
		result = node->joint.parent;
	}
	return result;
}

//

INSTANCE_INDEX MODEL::get_joint_child(JOINT_INDEX jnt) const
{
	INSTANCE_INDEX result = INVALID_INSTANCE_INDEX;
	if (jnt != INVALID_JOINT_INDEX)
	{
		JointNode * node = joints.list + jnt;
		result = node->joint.child;
	}
	return result;
}

//

