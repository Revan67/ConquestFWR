
#include "EngComp.h"
#include "Channel.h"
#include "IChannel.h"
#include "ICamera.h"
#include "StdDAT.h"
#include "TComponent.h"
#include "SysConsumerDesc.h"
#include "da_heap_utility.h"
#include "Pools.h"
#include "MList.h"
#include "MStack.h"
#include "fdump.h"

//

void DebugPrint (char *fmt, ...);

//

using namespace Channel;

//

struct InstanceTarget
{
	INSTANCE_INDEX	instance;
	LList<Target>	target_list;
};

LList<InstanceTarget> InstanceTargetList;

//

struct HashPoolTarget
{
// Channel target: object joint or callback.
	Target			target;

	MotionList		motion_list;

	float			transition_duration;	// keep around duration of last transition.

// Hash bookkeeping stuff.
	U32				hash_key;    // For internal use only
	HashPoolTarget *hash_next;   
	HashPoolTarget *hash_prev;  
	HashPoolTarget *next;          
	HashPoolTarget *prev;       
                          
	S32				index;       // Index of this entry in linear array

	static U32 hash(const void * obj)
	{
		const Target * t = (const Target *) obj;
		U32 result = U32(t->type) ^ U32(t->object);
		result &= 0xff;
		return result;
	}

	BOOL32 compare(const void * obj)
	{
		const Target * t = (const Target *) obj;
		return (target == *t);
	}

	void initialize(const void * obj)
	{
		const Target * t = (const Target *) obj;
		target = *t;
	}

	void shutdown(void) 
	{
		motion_list.free();
		memset(&target, 0, sizeof(target));
		transition_duration = 0;
	}

	void display(void) {}

	HashPoolTarget & operator = (const HashPoolTarget & t)
	{
		target = t.target;

	// Copy motion list.
		MotionStackNode * node = t.motion_list.first();
		while (node)
		{
			switch (node->type)
			{
				case MSN_CHANNEL:
				{
					ChannelNode * chan = (ChannelNode *) node;
					ChannelNode * new_chan = new ChannelNode(chan->object);
					motion_list.link(new_chan);
					break;
				}
				case MSN_TRANSITION:
				{
					TransitionNode * trans = (TransitionNode *) node;
					TransitionNode * new_trans = new TransitionNode(trans->duration);
					new_trans->time = trans->time;
					motion_list.link(new_trans);
					break;
				}
				case MSN_POSE:
				{
					PoseNode * pose = (PoseNode *) node;
					PoseNode * new_pose = new PoseNode(*pose);
					motion_list.link(new_pose);
					break;
				}
			}
			node = t.motion_list.next(node);
		}

		transition_duration = t.transition_duration;

		hash_key = t.hash_key;
		hash_next = t.hash_next;
		hash_prev = t.hash_prev;
		next = t.next;
		prev = t.prev;
		index = t.index;
		return *this;
	}
};

//

typedef HashPool <HashPoolTarget, 256>	target_pool;

#pragma warning (disable : 4786)
#include <map>
typedef std::map<CHANNEL_ARCHETYPE_INDEX, Channel::Archetype>  ARCH_MAP;

struct ObjContainer
{
	bool active:1;

	CHANNEL_ARCHETYPE_INDEX archetype_index;
	Object* object;
};

typedef std::map<CHANNEL_INSTANCE_INDEX, ObjContainer> INST_MAP;


struct TargetContainer
{
// Channel target: object joint or callback.
	Target			target;

	MotionList		motion_list;

	float			transition_duration;	// keep around duration of last transition.
};

#include <set>
//typedef std::set<TargetContainer>;

//

struct ChannelControl : public IEngineComponent, 
						public IChannel
{
	protected:

		unsigned int cur_arch_index;
		ARCH_MAP archetypes;
		unsigned int cur_inst_index;
		INST_MAP instances;

		target_pool						targets;

	public:

	// order of these interfaces no longer matters
		
	BEGIN_DACOM_MAP_INBOUND(ChannelControl)
	DACOM_INTERFACE_ENTRY(IChannel)
	DACOM_INTERFACE_ENTRY2(IID_IChannel,IChannel)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	END_DACOM_MAP()

public: // Interface

	DA_HEAP_DEFINE_NEW_OPERATOR(ChannelControl)

	~ChannelControl (void)
	{
		targets.free();
	}

	GENRESULT init (SYSCONSUMERDESC* info);
	GENRESULT COMAPI Initialize (void);

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

	// IChannel
	CHANNEL_ARCHETYPE_INDEX COMAPI create_channel_archetype (IFileSystem* fs) ;
	int COMAPI release_channel_archetype (CHANNEL_ARCHETYPE_INDEX idx) ;
	float COMAPI get_duration (CHANNEL_ARCHETYPE_INDEX idx) const ;
	unsigned int COMAPI get_data_type (CHANNEL_ARCHETYPE_INDEX idx) const ;
	CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (IFileSystem* fs) ;
	CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (CHANNEL_ARCHETYPE_INDEX idx) ;
	CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (Channel::IVirtualChannel * ctrl, unsigned int data_type) ;
	bool COMAPI destroy_channel_instance (CHANNEL_INSTANCE_INDEX idx) ;
	CHANNEL_ARCHETYPE_INDEX COMAPI get_channel_archetype (CHANNEL_INSTANCE_INDEX idx) ;
	bool COMAPI start (CHANNEL_INSTANCE_INDEX idx, Channel::StartParms* parms) ;
	bool COMAPI stop (CHANNEL_INSTANCE_INDEX idx) ;
	bool COMAPI get_start_parms (CHANNEL_INSTANCE_INDEX idx, Channel::StartParms* parms) const ;
	float COMAPI get_current_time (CHANNEL_INSTANCE_INDEX idx) const ;
	bool COMAPI set_current_time (CHANNEL_INSTANCE_INDEX idx, float time) ;
	void COMAPI set_weight(CHANNEL_INSTANCE_INDEX idx, float weight) ;
	float COMAPI get_weight(CHANNEL_INSTANCE_INDEX idx) const ;
	void COMAPI change_position(CHANNEL_INSTANCE_INDEX idx, const class Vector & p) ;
	void COMAPI change_orientation(CHANNEL_INSTANCE_INDEX idx, const class Matrix & R) ;
	void COMAPI update_instance_channels(INSTANCE_INDEX root, float dt) ;
	bool COMAPI channel_in_use(CHANNEL_INSTANCE_INDEX idx) const ;
	Channel::Target COMAPI get_channel_target(CHANNEL_INSTANCE_INDEX idx) const ;
	BOOL32 COMAPI get_channel_events(CHANNEL_ARCHETYPE_INDEX idx, EventIterator & events) const ;
	void COMAPI get_current_offset(float * absolute, float * relative, CHANNEL_INSTANCE_INDEX idx) const ;
	void COMAPI adjust_start_position(CHANNEL_INSTANCE_INDEX idx, const Vector & dp) ;
	BOOL32 COMAPI get_channel_data_at_time(CHANNEL_INSTANCE_INDEX idx, float time, void * data);

protected: // Interface
	void update_target(HashPoolTarget * t, float dt);
};

//

void SetDllHeapMsg (HINSTANCE hInstance)
{
	DWORD dwLen;
	char buffer[260];

	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));

	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:

			DA_HEAP_ACQUIRE_HEAP( HEAP );
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			IComponentFactory* server = new DAComponentFactory2<DAComponentAggregate<ChannelControl>, SYSCONSUMERDESC> ("IChannel");

			if (server)
			{
				DACOM_Acquire ()->RegisterComponent(server, "IChannel", DACOM_NORMAL_PRIORITY);
			}

			server->Release();	// DACOM has added a reference to the server, 
								// we call Release() since we don't save the pointer
			break;
	}

	return TRUE;
}

//

GENRESULT ChannelControl::Initialize (void)
{
	GENRESULT r;

	if ((r = ((IEngineComponent*)this)->QueryInterface (IID_IEngine, (void**)&Channel::Engine)) != GR_OK)
		return r;
	((IEngineComponent*)this)->Release();		// release the extra reference

	return r;
}

//

GENRESULT ChannelControl::init (SYSCONSUMERDESC * )
{
	return GR_OK;
}

//

BOOL32 ChannelControl::create_archetype (ARCHETYPE_INDEX , IFileSystem* )
{
	return FALSE;
}

//

void COMAPI	ChannelControl::duplicate_archetype( ARCHETYPE_INDEX , ARCHETYPE_INDEX ) 
{
	return ;
}

//

void ChannelControl::destroy_archetype (ARCHETYPE_INDEX )
{
	return ;
}

//

GENRESULT COMAPI ChannelControl::query_archetype_interface( ARCHETYPE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

BOOL32 ChannelControl::create_instance (INSTANCE_INDEX , ARCHETYPE_INDEX )
{
	return FALSE;
}

//

void ChannelControl::destroy_instance (INSTANCE_INDEX )
{
	return ;
}

//

void ChannelControl::update_instance( INSTANCE_INDEX idx, SINGLE dt )
{
	update_instance_channels (idx, dt);
}

//

enum vis_state COMAPI ChannelControl::render_instance( struct ICamera *, INSTANCE_INDEX, float, U32, const Transform * ) 
{
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI ChannelControl::query_instance_interface( INSTANCE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

void ChannelControl::update_target(HashPoolTarget * t, float dt)
{
	MotionStackNode * list[64];
	int list_cnt = 0;

// DO update loop, THEN stack loop. Combining them is too confusing.
	MotionStackNode * node = t->motion_list.first();
	while (node)
	{
		list[list_cnt++] = node;
		node = t->motion_list.next(node);
	}
	if (list_cnt)
	{
		for (int l = 0; l < list_cnt; l++)
		{
			node = list[l];

			if (node->update(dt) == ANIMATION_COMPLETE)
			{
				if (node->type == MSN_TRANSITION)
				{
					TransitionNode * trans = (TransitionNode *) node;
					if (trans->special_blend)
					{
					// need to adjust destination start_pos.
						ChannelNode * chan = (ChannelNode *) node->prev;
						chan->object->change_position(Engine->get_position(t->target.object));
					}
		// ALWAYS REMOVE SOURCE?
					if (trans->remove_source)
					{
					// Recursively remove transition's source node...

						t->motion_list.remove_transition(node, false);
					}
					else
					{
					// Remove the transition, leave everything else.
						t->motion_list.free(node);
					}
				}
				else if (node->type == MSN_CHANNEL)
				{
					ChannelNode * chan = (ChannelNode *) node;

				// Need to deal with motion that's part of a transition ending...
					MotionStackNode * next = node->next;
					if (next)
					{
						MotionStackNode * next_next = next->next;
						if ((next->type == MSN_TRANSITION) || (next_next && next_next->type == MSN_TRANSITION))
						{
						// This channel is part of a transition. Suspend instead of deleting.
						//PoseNode * pose = new PoseNode(chan);
						//t->motion_list.free(node);
						//t->motion_list.link(pose, next);
							chan->object->suspend();
						}
						else
						{
						//
						// Not part of transition, but other motions after this one.
						// Should we suspend?
						//
							//chan->object->suspend();
							t->motion_list.free(node);
						}
					}
					else
					{
					// Nothing comes after. See if there are previous active motions:
						MotionStackNode * prev = node->prev;
						if (prev)
						{
						// Transition back into previous motion.
							TransitionNode * trans = new TransitionNode(t->transition_duration, true);

							//PoseNode * pose = new PoseNode(chan);
							//t->motion_list.free(node);
							//t->motion_list.link(pose, next);
							chan->object->suspend();

						// Swap source/dest motions.
							t->motion_list.unlink(node);
							t->motion_list.link(node, prev);

							t->motion_list.link(trans);
						}
						else
						{
						// Delete this motion from list.
							chan->object->suspend();

//
// THIS IS THE PROBLEM CASE. If we just suspend, it causes some kind of problem in the character animation/transition
// case. If we delete after suspending, it causes a problem where when starting a motion at or past the end, the target
// never gets notified.
//

						// DELETE, OR JUST SUSPEND?
							//t->motion_list.free(node);

							node->delete_me = true;
						}
					}
				}

			// CHECK FOR VALID STATE.
				//t->motion_list.verify();
			}
		}

		MotionStack stack;
		node = t->motion_list.first();
		while (node)
		{
			if (node->type == MSN_CHANNEL)
			{
				ChannelNode * chan = (ChannelNode *) node;
				if (chan->object->is_finished() && !chan->object->is_suspended())
				{
					//DebugPrint("OUCH\n");
				}
			}
			stack.push(node);
			node = t->motion_list.next(node);
		}

		if (!stack.is_empty())
		{
			int data_size;
			switch (t->target.type)
			{
				case JOINT:
				{
					const JointInfo *ji;
					if( (ji = Engine->get_joint_info( t->target.joint )) != NULL ) {
						data_size = ji->get_state_vector_size();
					}
					else {
						data_size = 0;
					}
					break;
				}
				case OBJECT:
					data_size = 7;
					break;
				case EVENT:
					data_size = 0;
					break;
				default:
					GENERAL_FATAL("Unknown data type!\n");
			}

			float u[7];
			stack.evaluate(u, data_size);

		// Notify target.
			switch (t->target.type)
			{
				case JOINT:
					Engine->set_joint_state( t->target.joint, IE_JST_BASIC, u );
					break;
				case OBJECT:
				{
					Engine->set_position(t->target.object, *((Vector *) u));
					Engine->set_orientation(t->target.object, *((Quaternion *) (u+3)));
					break;
				}
			}
		}

		node = t->motion_list.first();
		while (node)
		{
			MotionStackNode * next = t->motion_list.next(node);
			if (node->delete_me)
			{
				t->motion_list.free(node);
			}
			node = next;
		}
	}
}

//

void ChannelControl::update (SINGLE dt)
{
// NEED TO UPDATE ANY CHANNELS THAT AREN'T IN A TARGET'S MOTION LIST
	for (INST_MAP::iterator it = instances.begin ();
		it != instances.end ();
		it++)
	{
		if (it->second.active)
			it->second.object->clear_update_flag ();
	}

	HashPoolTarget * t = targets.list;
	for (int i = 0; i < targets.list_size; i++, t++)
	{
		if (t->index != -1)
		{
			update_target(t, dt);
		}
	}

	for (it = instances.begin ();
		it != instances.end ();
		it++)
	{
		if (it->second.active && !it->second.object->was_updated())
		{
			if (it->second.object->update(dt) == ANIMATION_COMPLETE)
			{
				it->second.active = false;
			}
		}
	}
}
//

CHANNEL_ARCHETYPE_INDEX ChannelControl::create_channel_archetype (IFileSystem* fs)
{
	CHANNEL_ARCHETYPE_INDEX index = INVALID_CHANNEL_ARCHETYPE_INDEX;

	Archetype nu;

	if (Archetype::Create (fs, nu))
	{
		index = cur_arch_index++;
		archetypes.insert (ARCH_MAP::value_type (index, nu));
	}

	return index;
}

int ChannelControl::release_channel_archetype (CHANNEL_ARCHETYPE_INDEX idx)
{
	int result = -1;

	if (INVALID_CHANNEL_ARCHETYPE_INDEX != idx)
	{
		ARCH_MAP::iterator it = archetypes.find (idx);
		
		if (it != archetypes.end ())
		{
			result = --it->second.ref_count;

			if (!result)
			{
				it->second.free ();
				archetypes.erase (it);
			}
		}
	}

	return result;
}

unsigned int ChannelControl::get_data_type (CHANNEL_ARCHETYPE_INDEX idx) const
{
	unsigned int result = 0;

	if (INVALID_CHANNEL_ARCHETYPE_INDEX != idx)
	{
		ARCH_MAP::const_iterator it = archetypes.find (idx);
		
		if (it != archetypes.end ())
			result = it->second.get_data_type ();

	}

	return result;
}

float ChannelControl::get_duration (CHANNEL_ARCHETYPE_INDEX idx) const
{
	float duration = 0;

	if (INVALID_CHANNEL_ARCHETYPE_INDEX != idx)
	{
		ARCH_MAP::const_iterator it = archetypes.find (idx);
		
		if (it != archetypes.end ())
			duration = it->second.get_duration ();
	}

	return duration;
}

CHANNEL_INSTANCE_INDEX ChannelControl::create_channel_instance (IFileSystem* fs)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	CHANNEL_ARCHETYPE_INDEX arch = create_channel_archetype (fs);
	
	if (INVALID_CHANNEL_ARCHETYPE_INDEX != arch)
	{
		result = create_channel_instance (arch);
		release_channel_archetype (arch);
	}

	return result;
}

CHANNEL_INSTANCE_INDEX ChannelControl::create_channel_instance (CHANNEL_ARCHETYPE_INDEX idx)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	if (INVALID_CHANNEL_ARCHETYPE_INDEX != idx)
	{
		ARCH_MAP::iterator it = archetypes.find (idx);
		
		if (it != archetypes.end ())
		{
			it->second.ref_count++;

			result = cur_inst_index++;

			ObjContainer obj;
			obj.object = it->second.create_instance (result);

			ASSERT (obj.object);

			obj.archetype_index = idx;
			obj.active = false;

			instances.insert (INST_MAP::value_type (result, obj));
		}
	}

	return result;
}

//

CHANNEL_INSTANCE_INDEX COMAPI ChannelControl::create_channel_instance (Channel::IVirtualChannel * ctrl, unsigned int data_type)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	if (ctrl)
	{
		result = cur_inst_index++;
		Object * obj;
		switch (data_type)
		{
			case DT_FLOAT:
				obj = new FloatStream(ctrl, result);
				break;
			case DT_VECTOR:
				obj = new VectorStream(ctrl, result);
				break;
			case DT_QUATERNION:
				obj = new QuatStream(ctrl, result);
				break;
			case DT_VECTOR | DT_QUATERNION:
				obj = new FullStream(ctrl, result);
				break;
			case DT_EVENT:
				obj = new Object(ctrl, result);
				break;
			default:
				GENERAL_FATAL("Unknown data type!\n");
		}

		ObjContainer cobj;
		cobj.object = obj;

		ASSERT (cobj.object);

		cobj.archetype_index = INVALID_CHANNEL_ARCHETYPE_INDEX;
		cobj.active = false;

		instances.insert (INST_MAP::value_type (result, cobj));
	}

	return result;
}

//

CHANNEL_ARCHETYPE_INDEX ChannelControl::get_channel_archetype (CHANNEL_INSTANCE_INDEX idx)
{
	CHANNEL_ARCHETYPE_INDEX result = INVALID_CHANNEL_ARCHETYPE_INDEX;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			result = it->second.archetype_index;
	}

	return result;
}

bool ChannelControl::destroy_channel_instance (CHANNEL_INSTANCE_INDEX idx)
{
	bool result = false;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
		{
// TO DO: If channel has never been started, target is invalid, so targets.search()
// will return undefined results.
// Skip this step if channel has no valid target.

		//
		// Remove from targets list.
		//
			const Target * target = &(it->second.object->get_target());
			S32 tidx = targets.search(target);
			if (tidx != -1)
			{
				HashPoolTarget * t = targets.list + tidx;

			// IF GIVEN CHANNEL IS SRC OF A TRANSITION, REPLACE IT WITH POSE NODE.
			// If destination of a transition, remove entire transition.

				MotionStackNode * node = t->motion_list.first();
				while (node)
				{
					if (node->type == MSN_CHANNEL)
					{
						ChannelNode * chan = (ChannelNode *) node;
						if (chan->object == it->second.object)
						{
							break;
						}

					}
					node = t->motion_list.next(node);
				}

				if (node)
				{
					if (node->is_transition_source())
					{
					// replace with pose node.
						PoseNode * pose = new PoseNode(t->target);
						t->motion_list.link(pose, node->next);
					}
					else if (node->is_transition_destination())
					{
					// remove transition.
						TransitionNode * trans = (TransitionNode *) node->next;
						if (trans->is_transition_source())
						{
							PoseNode * pose = new PoseNode(t->target);
							t->motion_list.link(pose, trans->next);
						}

						t->motion_list.remove_transition(trans, false);
					}

					t->motion_list.free(node);
				}

				//t->motion_list.verify();

				if (t->motion_list.count() == 0)
				{
				// No more motions affect this target, so get rid of pool node.
					targets.unlink(t);
				}
			}

			release_channel_archetype (it->second.archetype_index);
			result = true;

			delete it->second.object;
			instances.erase (it);
		}
	}

	return result;
}

bool ChannelControl::start (CHANNEL_INSTANCE_INDEX idx, Channel::StartParms* parms)
{
	bool result = false;

	if ((INVALID_CHANNEL_INSTANCE_INDEX != idx) && parms)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
		{
			result = it->second.active = it->second.object->start(parms);

			if (result)
			{
			//
			// Add target to target hashpool if not already present, add object to target's list.
			//
				Target * target = &parms->target;
				S32 tidx = targets.search(target);
				if (tidx == -1)
				{
					tidx = targets.allocate(target);
				}

				if (tidx != -1)
				{
					HashPoolTarget * t = targets.list + tidx;

				// See if it already exists...
					MotionStackNode * node = t->motion_list.first();
					while (node)
					{
						if (node->type == MSN_CHANNEL)
						{
							ChannelNode * chan = (ChannelNode *) node;
							if (chan->object == it->second.object)
							{
								return true;
							}
						}
						node = t->motion_list.next(node);
					}

					MotionStackNode * transition_source = NULL;

					bool trans = (parms->transition_duration > 0);
					if (trans)
					{
						if (t->motion_list.count() == 0)
						{
						//
						// MOTION LIST IS EMPTY. Need to save current pose as 
						// source channel for transition.
						//
							PoseNode * pose = new PoseNode(t->target);
							ASSERT(pose);
							t->motion_list.link(pose);

							transition_source = pose;
						}
						else
						{
							MotionStackNode * last = t->motion_list.last();
							transition_source = last;
						}
					}
					else
					{
					// No transitions, no point in keeping around previous instances.
					// They'll just gum up the works.
						if (t->motion_list.count() != 0)
						{
							MotionStackNode * n = t->motion_list.first();
							while (n)
							{
								if (n->type == MSN_CHANNEL)
								{
									ChannelNode * c = (ChannelNode *) n;
									c->object->finish();
								}

								n = t->motion_list.next(n);
							}

							t->motion_list.free();
						}
					}

				//
				// Add new motion.
				//
					ChannelNode * chan = new ChannelNode(it->second.object);
					ASSERT(chan);
					t->motion_list.link(chan);

					if (trans)
					{
					//
					// Add transition.
					//
						TransitionNode * trans = new TransitionNode(parms->transition_duration);

						if (parms->overlay)
						{
							trans->remove_source = false;
						}

						t->motion_list.link(trans);
						t->transition_duration = parms->transition_duration;

						if (target->type == OBJECT)
						{
							if ((parms->flags & XLAT_BLEND_X) ||
								(parms->flags & XLAT_BLEND_Y) ||
								(parms->flags & XLAT_BLEND_Z))
							{
							// set up transition parameters. 

								Vector dx_dst = chan->get_translation(parms->transition_duration);
								float dm = dot_product(dx_dst, dx_dst);
								trans->special_blend = true;
								trans->pstart = Engine->get_position(target->object);
								trans->v0 = transition_source->get_translation(parms->transition_duration);

								trans->v0 *= trans->inv_duration;

								Vector v1 = dx_dst * trans->inv_duration;

								trans->dv = v1 - trans->v0;
							}
						}
					}

				// CHECK FOR VALID STATE.
					//t->motion_list.verify();
				}
			}
		}
	}

	return result;
}

bool ChannelControl::stop (CHANNEL_INSTANCE_INDEX idx)
{
	bool result = false;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
		{
			if (it->second.active)
			{
				it->second.object->suspend();

				result = true;
				it->second.active = false;
			}
		}
	}

	return result;
}

bool ChannelControl::get_start_parms (CHANNEL_INSTANCE_INDEX idx, Channel::StartParms* parms) const
{
	bool result = false;

	if ((INVALID_CHANNEL_INSTANCE_INDEX != idx) && parms)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
		{
			result = true;
			it->second.object->get_start_parms (parms);
		}
	}

	return result;
}

float ChannelControl::get_current_time (CHANNEL_INSTANCE_INDEX idx) const
{
	float result = -1.0;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			result = it->second.object->get_current_time ();
	}

	return result;
}

bool ChannelControl::set_current_time (CHANNEL_INSTANCE_INDEX idx, float time)
{
	bool result = false;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			result = it->second.object->set_current_time (time);
	}

	return result;
}

//

void ChannelControl::set_weight(CHANNEL_INSTANCE_INDEX idx, float weight)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
			it->second.object->set_weight(weight);
	}
}

//

float ChannelControl::get_weight(CHANNEL_INSTANCE_INDEX idx) const
{
	float result = 0;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			result = it->second.object->get_weight();
	}

	return result;
}

//

void ChannelControl::change_position(CHANNEL_INSTANCE_INDEX idx, const Vector & p)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
			it->second.object->change_position(p);
	}
}

//

void COMAPI ChannelControl::change_orientation(CHANNEL_INSTANCE_INDEX idx, const Matrix & R)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
			it->second.object->change_orientation(R);
	}
}

//

void EnumerateObjectsAndJoints(INSTANCE_INDEX root, INSTANCE_INDEX * objects, int & no, JOINT_INDEX * joints, int & nj)
{
	objects[no++] = root;

	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	while( (child = Engine->get_instance_child_next(root, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX) 
	{
		joints[nj++] = child;
		EnumerateObjectsAndJoints(child, objects, no, joints, nj);
	}
}

//

void COMAPI ChannelControl::update_instance_channels(INSTANCE_INDEX root, float dt)
{
	if (root != INVALID_INSTANCE_INDEX)
	{
		INSTANCE_INDEX objects[256];
		JOINT_INDEX joints[256];

		int num_objects = 0;
		int num_joints = 0;
		EnumerateObjectsAndJoints(root, objects, num_objects, joints, num_joints);

		Target target;
		if (num_objects)
		{
			target.type = OBJECT;
			for (int i = 0; i < num_objects; i++)
			{
				target.object = objects[i];
				S32 tidx = targets.search(&target);
				if (tidx != -1)
				{
					HashPoolTarget * t = targets.list + tidx;
					update_target(t, dt);
				}
			}
		}

		if (num_joints)
		{
			target.type = JOINT;
			for (int i = 0; i < num_joints; i++)
			{
				target.joint = joints[i];
				S32 tidx = targets.search(&target);
				if (tidx != -1)
				{
					HashPoolTarget * t = targets.list + tidx;
					update_target(t, dt);
				}
			}
		}

	// just search the whole goddamn list.
		HashPoolTarget * t = targets.list;
		for (int i = 0; i < targets.list_size; i++, t++)
		{
			if ((t->target.type == Channel::EVENT) && (t->target.object == root))
			{
			 	update_target(t, dt);
			}
		}
	}
}

//

bool COMAPI ChannelControl::channel_in_use(CHANNEL_INSTANCE_INDEX idx) const
{
	bool result = false;

	if (idx != INVALID_CHANNEL_INSTANCE_INDEX)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
		{
			Object * obj = it->second.object;
			const Target * target = &(it->second.object->get_target());

			S32 tidx = targets.search(target);
			if (tidx != -1)
			{
				HashPoolTarget * t = targets.list + tidx;

				MotionStackNode * node = t->motion_list.first();
				while (node)
				{
					if (node->type == MSN_CHANNEL)
					{
						ChannelNode * chan = (ChannelNode *) node;
						if (chan->object == it->second.object)
						{
							result = true;
							break;
						}
					}

					node = t->motion_list.next(node);
				}
			}
		}
	}

	return result;
}

//

Channel::Target COMAPI ChannelControl::get_channel_target(CHANNEL_INSTANCE_INDEX idx) const
{
	Channel::Target result;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			result = it->second.object->get_target();
	}

	return result;
}

BOOL32 COMAPI ChannelControl::get_channel_events(CHANNEL_INSTANCE_INDEX idx, EventIterator & events) const
{
	BOOL32 result = FALSE;

	if (idx != INVALID_CHANNEL_ARCHETYPE_INDEX)
	{
		ARCH_MAP::const_iterator it = archetypes.find (idx);
		
		if (it != archetypes.end ())
		{
			const Archetype& archetype = it->second;

			if (archetype.get_num_frames())
			{
				const void * frame_data = archetype.get_frame(0);
				const void * event_data = archetype.get_event_data();

				events.initialize(frame_data, archetype.get_num_frames(), event_data);
				result = TRUE;
			}
		}
	}
	return result;
}

//

void COMAPI ChannelControl::get_current_offset(float * abs, float * rel, CHANNEL_INSTANCE_INDEX idx) const
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::const_iterator it = instances.find (idx);

		if (it != instances.end ())
			it->second.object->get_current_offset(abs, rel);
	}
}

//

void COMAPI ChannelControl::adjust_start_position(CHANNEL_INSTANCE_INDEX idx, const Vector & dp)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end ())
			it->second.object->adjust_start_position(dp);
	}
}

//

BOOL32 COMAPI ChannelControl::get_channel_data_at_time(CHANNEL_INSTANCE_INDEX idx, float time, void * data)
{
	BOOL32 result = FALSE;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		INST_MAP::iterator it = instances.find (idx);

		if (it != instances.end () && it->second.object->get_data_at_time(time, data))
			result = TRUE;
	}

	return result;
}

//

inline PoolArchetype::PoolArchetype (void)
{
	archetype = NULL;
}

inline void PoolArchetype::initialize(const void *object)
{
	ref_cnt = 0;
	archetype = NULL;
}

inline void PoolArchetype::shutdown(void)
{
	delete archetype;
	archetype = NULL;
}

inline void PoolArchetype::display(void)
{
}

PoolChannelObj::PoolChannelObj (void)
{
	active = false;
	object = NULL;
}

inline void PoolChannelObj::initialize(const void *_object)
{
	active = false;
	object = NULL;
}

inline void PoolChannelObj::shutdown(void)
{
	active = false;

	delete object;
	object = NULL;
}

inline void PoolChannelObj::display(void)
{
}
