//$Header: /Libs/dev/Src/Anim/AnimComponent.cpp 29    3/21/00 4:29p Pbleisch $

/*
EMAURER Thread-safe creation of archetypes is achieved via the fact that there is no
database of archetypes which must be protected.
*/

#include <windows.h>
#pragma warning (disable : 4786)
#include "IAnim.h"
#include "Anim.h"
#include "TComponent.h"
#include "SysConsumerDesc.h"
#include "da_heap_utility.h"
#include "tempstr.h"
#include <map>
#include <set>

using namespace Animation;

struct InstAndArch
{
	Script inst;
	ScriptSet* archset;
};

typedef std::map<SCRIPT_INST, InstAndArch> SINST_MAP;

using namespace Channel;

typedef std::set<Object*> CINST_SET;

//

#include "mlist.h"
#include "mstack.h"

struct TargetAffectors
{
	MotionList motion_list;
	float transition_duration;
};

struct TargetLess
{
	int build_val (const Target& n0) const
	{
		int val0;

		switch (n0.type)
		{
			case OBJECT:
				val0 = OBJECT | (n0.object << 3);
			break;
			case JOINT:
				val0 = JOINT | (n0.joint << 3);
			break;
			case EVENT:
				val0 = (int)n0.event_handler;
			break;
			default:
				GENERAL_FATAL("Unknown Animation target type!\n");
		}

		return val0;
	}
		
	bool operator () (const Target& n0, const Target& n1) const
	{
		return build_val (n0) < build_val (n1);
	}
};

typedef std::map<Target, TargetAffectors, TargetLess> TARGET_MAP;

struct AnimComponent :  public IAnimation,
						public IAggregateComponent, 
						public IChannel,
						public IAnimation2,
						public IAnimation3
{
	DA_HEAP_DEFINE_NEW_OPERATOR(AnimComponent)

	int next_inst;
	SINST_MAP instances;

	BEGIN_DACOM_MAP_INBOUND(AnimComponent)
	DACOM_INTERFACE_ENTRY(IAnimation)
	DACOM_INTERFACE_ENTRY(IAnimation2)
	DACOM_INTERFACE_ENTRY(IAnimation3)
	DACOM_INTERFACE_ENTRY(IChannel)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAnimation,IAnimation)
	DACOM_INTERFACE_ENTRY2(IID_IAnimation2,IAnimation2)
	DACOM_INTERFACE_ENTRY2(IID_IAnimation3,IAnimation3)
	DACOM_INTERFACE_ENTRY2(IID_IChannel,IChannel)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	AnimComponent (void);
	~AnimComponent (void);

	GENRESULT init (SYSCONSUMERDESC* desc);

	GENRESULT COMAPI Initialize (void);

	// IAnimation
	SCRIPT_SET_ARCH COMAPI create_script_set_arch (IFileSystem* fs);
	void COMAPI release_script_set_arch (SCRIPT_SET_ARCH arch);

	float COMAPI get_duration (SCRIPT_SET_ARCH, const char* name) const;

	void COMAPI enumerate_scripts (SCRIPT_ENUM_CALLBACK cbfn, SCRIPT_SET_ARCH arch, void* misc) const;
	GENRESULT COMAPI enumerate_channels (CHANNEL_ENUM_CALLBACK, SCRIPT_SET_ARCH, const char* name, void* user) const;

	SCRIPT_INST COMAPI create_script_inst (SCRIPT_SET_ARCH arch,
													INSTANCE_INDEX root, 
													const char* name,
													Animation::IEventHandler * ev = NULL,
													void* user_data = NULL);

	void COMAPI release_script_inst (SCRIPT_INST inst);

	int COMAPI script_start (SCRIPT_INST inst, 
								unsigned int flags=FORWARD, 
								float time=CUR,
								float time_scale=1.0f,
								float transition_duration=0.0f,
								float weight=1.0f,
								float translation_scale=1.0f,
								const Transform * start_xform = NULL);

	void COMAPI script_stop (SCRIPT_INST inst);

	void COMAPI set_current_time(SCRIPT_INST, float);
	float COMAPI get_current_time (SCRIPT_INST) const;
	BOOL32 COMAPI script_is_done(SCRIPT_INST) const;

	void COMAPI change_script_position(SCRIPT_INST inst, const Vector & p);
	void COMAPI change_script_orientation(SCRIPT_INST inst, const Matrix & orient);

	BOOL32 COMAPI script_contains_channel(SCRIPT_INST script_inst, CHANNEL_INSTANCE_INDEX channel_inst) const;

	BOOL32 COMAPI get_root_data(SCRIPT_INST script_inst, Vector & absolute, Vector & offset) const;

	void COMAPI adjust_start_position(SCRIPT_INST script_inst, const Vector & dpos);

	float COMAPI get_duration(SCRIPT_INST inst) const;

	BOOL32 COMAPI get_instance_name(SCRIPT_INST inst, const char *& name) const;

	BOOL32 COMAPI get_script_start_height(SCRIPT_SET_ARCH arch, const char * name, float & start_height) const;
	BOOL32 COMAPI get_script_start_height(SCRIPT_INST inst, float & start_height) const;

	CHANNEL_INSTANCE_INDEX COMAPI find_channel(SCRIPT_INST inst, Animation::Target & target);

	void COMAPI update_instance (INSTANCE_INDEX root, SINGLE dt) ;
	void COMAPI update (SINGLE dt); 


	// IChannel
	CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (Animation::IVirtualChannel * ctrl, unsigned int data_type, const Animation::Target&);
	CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (IChannel2*, const Animation::Target&);
	GENRESULT COMAPI destroy_channel_instance (CHANNEL_INSTANCE_INDEX idx);
	GENRESULT COMAPI get_channel_archetype (CHANNEL_INSTANCE_INDEX idx, IChannel2*&);
	bool COMAPI start (CHANNEL_INSTANCE_INDEX idx, Animation::StartParms* parms);
	bool COMAPI stop (CHANNEL_INSTANCE_INDEX idx);
	float COMAPI get_current_time (CHANNEL_INSTANCE_INDEX idx) const;
	bool COMAPI set_current_time (CHANNEL_INSTANCE_INDEX idx, float time);
	void COMAPI set_weight(CHANNEL_INSTANCE_INDEX idx, float weight);
	float COMAPI get_weight(CHANNEL_INSTANCE_INDEX idx) const;
	void COMAPI change_position(CHANNEL_INSTANCE_INDEX idx, const Vector & p);
	void COMAPI change_orientation(CHANNEL_INSTANCE_INDEX idx, const Matrix & R);
	bool COMAPI channel_in_use(CHANNEL_INSTANCE_INDEX idx) const ;
	Animation::Target COMAPI get_channel_target(CHANNEL_INSTANCE_INDEX idx) const;
	void COMAPI adjust_start_position(CHANNEL_INSTANCE_INDEX idx, const Vector & dp) ;
	GENRESULT COMAPI is_forward (CHANNEL_INSTANCE_INDEX idx, bool&) const;

	// IAnimation2
	SCRIPT_SET_ARCH COMAPI create_empty_script_set_arch();
	GENRESULT COMAPI load_scripts (SCRIPT_SET_ARCH arch, IFileSystem *fs, const char *nameList[], int nameCount);
	GENRESULT COMAPI remove_scripts (SCRIPT_SET_ARCH arch, const char *nameList[], int nameCount);
	GENRESULT COMAPI enumerate_scripts_in_file (SCRIPT_ENUM_CALLBACK, IFileSystem* fs, void* misc=0) const;
	GENRESULT COMAPI verify_loaded (SCRIPT_SET_ARCH arch, const char *name);
	GENRESULT COMAPI script_count (SCRIPT_SET_ARCH arch, unsigned int &count);

	// IAnimation3
	GENRESULT COMAPI enumerate_channels2 (CHANNEL_INSTANCE_ENUM_CALLBACK cb, SCRIPT_INST inst, void* misc) const;

	protected:

/*
		typedef std::set<ScriptSet*> ARCH_DB;
		ARCH_DB archetypes;

		CriticalSection arch_db_lock;

		#define ENTER_CS() \ 
*/		

		void update_target(const Target& target, TargetAffectors& , float dt);
		void register_target_affector (Object* obj, float transition_duration, unsigned int flags);
		void unregister_target_affector (Object* obj);

		//EMAURER a database of all channel instances in the system. This is critical 
		//for updating technique
		CINST_SET cinstances;

		//EMAURER maps animation target to a list of channels that affect it.
		TARGET_MAP targets;

		//-------
	
		bool find_inst (SCRIPT_INST, Script*&);
		bool find_inst (SCRIPT_INST, Script const*&) const;
};

bool AnimComponent::find_inst (SCRIPT_INST inst, Script*& out)
{
	bool result = false;

	SINST_MAP::iterator it = instances.find (inst);

	if (it != instances.end ())
	{
		out = &((*it).second.inst);
		result = true;
	}

	return result;
}

bool AnimComponent::find_inst (SCRIPT_INST inst, Script const*& out) const
{
	bool result = false;

	SINST_MAP::const_iterator it = instances.find (inst);

	if (it != instances.end ())
	{
		out = &((*it).second.inst);
		result = true;
	}

	return result;
}

SCRIPT_SET_ARCH AnimComponent::create_script_set_arch (IFileSystem* fs)
{
	SCRIPT_SET_ARCH result = INVALID_SCRIPT_SET_ARCH;

	ScriptSet* ret = new ScriptSet;

	if (ret->load (fs) == GR_OK)
		result = reinterpret_cast<SCRIPT_SET_ARCH> (ret);
	else
		ret->Release ();

	return result;
}

SCRIPT_SET_ARCH AnimComponent::create_empty_script_set_arch ()
{
	ScriptSet* ret = new ScriptSet;
	SCRIPT_SET_ARCH result = reinterpret_cast<SCRIPT_SET_ARCH> (ret);
	return result;
}

GENRESULT AnimComponent::load_scripts (SCRIPT_SET_ARCH arch, IFileSystem* fs, const char *nameList[], int nameCount)
{
	GENRESULT result = GR_GENERIC;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet * set = reinterpret_cast<ScriptSet *> (arch);
		result = set->load_subset (fs, nameList, nameCount);
	}

	return result;
}

GENRESULT AnimComponent::remove_scripts (SCRIPT_SET_ARCH arch, const char *nameList[], int nameCount)
{
	GENRESULT result = GR_GENERIC;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet * set = reinterpret_cast<ScriptSet *> (arch);
		result = set->unload_subset (nameList, nameCount);
	}

	return result;
}

GENRESULT AnimComponent::script_count (SCRIPT_SET_ARCH arch, unsigned int &count)
{
	GENRESULT result = GR_GENERIC;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet * set = reinterpret_cast<ScriptSet *> (arch);
		count = set->scripts.size();
		result = GR_OK;
	}

	return result;
}

void AnimComponent::release_script_set_arch (SCRIPT_SET_ARCH arch)
{
	if (arch != INVALID_SCRIPT_SET_ARCH && arch != 0)
		reinterpret_cast<ScriptSet*> (arch)->Release ();
}

float AnimComponent::get_duration (SCRIPT_SET_ARCH arch, const char* name) const
{
	float result = -1.0;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet const* aptr = reinterpret_cast<ScriptSet const*> (arch);

		SCRIPT_ARCH_MAP::const_iterator ait = aptr->scripts.find (name);

		if (ait != aptr->scripts.end ())
			result = (*ait).second.get_duration ();
	}

	return result;
}

void AnimComponent::set_current_time(SCRIPT_INST inst, float time)
{
	Script* iptr;

	if (find_inst (inst, iptr))
		iptr->set_current_time(time);
}

float AnimComponent::get_current_time (SCRIPT_INST inst) const
{
	float result = -1;

	Script const* iptr;

	if (find_inst (inst, iptr))
		result = iptr->get_current_time ();

	return result;
}

BOOL32 AnimComponent::script_is_done(SCRIPT_INST inst) const
{
	BOOL32 result = 0;

	Script const* iptr;

	if (find_inst (inst, iptr))
		result = iptr->all_channels_finished(this);

	return result;
}

SCRIPT_INST AnimComponent::create_script_inst (SCRIPT_SET_ARCH arch,
												INSTANCE_INDEX root, 
												const char* name,
												Animation::IEventHandler * cb,
												void* user_data)
{
	SCRIPT_INST i = INVALID_SCRIPT_INST;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet* aptr = reinterpret_cast<ScriptSet*> (arch);

		SCRIPT_ARCH_MAP::const_iterator ait = aptr->scripts.find (name);

		if (ait != aptr->scripts.end ())
		{
			InstAndArch s;
			s.archset = aptr;

			if (GR_OK == (*ait).second.create_instance (s.inst))
			{
				s.inst.connect(root, cb, user_data);

				for (CHANNEL_VECTOR::const_iterator chnlit = s.inst.channels.begin ();
					chnlit != s.inst.channels.end ();
					chnlit++)
				{
					cinstances.insert (*chnlit);
				}
				
				aptr->AddRef ();

				i = next_inst++;
				instances.insert (SINST_MAP::value_type (i, s));
			}
		}
	}

	return i;
}

void AnimComponent::release_script_inst (SCRIPT_INST inst)
{
	SINST_MAP::iterator it = instances.find (inst);

	if (it != instances.end ())
	{
		ScriptSet* aval = (*it).second.archset;

		for (CHANNEL_VECTOR::const_iterator chnlit = (*it).second.inst.channels.begin ();
			chnlit != (*it).second.inst.channels.end ();
			chnlit++)
		{
			unregister_target_affector (*chnlit);
			cinstances.erase (*chnlit);
		}

		(*it).second.inst.free ();
		instances.erase (it);
		
		aval->Release ();
	}
}

int AnimComponent::script_start (SCRIPT_INST inst, unsigned int flags, float time, float scale, float transition_duration, float weight, float translation_scale, const Transform * start_xform)
{
	int result = -1;

	Script* iptr;

	if (find_inst (inst, iptr))
	{
		result = iptr->start (flags, time, scale, transition_duration, weight, translation_scale, start_xform);

		if (result != -1)
		{
			StartParms parms;

			for (CHANNEL_VECTOR::iterator it = iptr->channels.begin ();
				it != iptr->channels.end ();
				it++)
			{
				register_target_affector (*it, transition_duration, flags);
			}
		}
	}
		
	return result;
}

void AnimComponent::script_stop (SCRIPT_INST inst)
{
	Script* iptr;

	if (find_inst (inst, iptr))
	{
		iptr->stop ();

		for (CHANNEL_VECTOR::iterator it = iptr->channels.begin ();
			it != iptr->channels.end ();
			it++)
			unregister_target_affector (*it);
	}
}

void AnimComponent::enumerate_scripts (SCRIPT_ENUM_CALLBACK cbfn, SCRIPT_SET_ARCH arch, void* misc) const
{
	if (cbfn && (arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet const* aptr = reinterpret_cast<ScriptSet const*> (arch);

		for (SCRIPT_ARCH_MAP::const_iterator sarch_it = aptr->scripts.begin ();
			sarch_it != aptr->scripts.end ();
			sarch_it++)
		{
			cbfn ((*sarch_it).first.c_str (), misc);
		}
	}
}

GENRESULT AnimComponent::enumerate_channels (CHANNEL_ENUM_CALLBACK cb, SCRIPT_SET_ARCH arch, const char* name, void* user) const
{
	GENRESULT result = GR_INVALID_PARMS;

	if (cb && (arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet const* aptr = reinterpret_cast<ScriptSet const*> (arch);

		SCRIPT_ARCH_MAP::const_iterator sarch_it = aptr->scripts.find (name);

		if (sarch_it != aptr->scripts.end ())
		{
			result = GR_OK;

			for (CHANNEL_MAPPING_VECTOR::const_iterator chit = (*sarch_it).second.channels.begin ();
				chit != (*sarch_it).second.channels.end ();
				chit++)
			{
				if ((*chit).archetype)
				{
					(*chit).archetype->AddRef ();
					cb ((*chit).archetype, user);
				}
			}
		}
	}
	return result;
}

//

void AnimComponent::change_script_position(SCRIPT_INST inst, const Vector & p)
{
	Script* iptr;

	if (find_inst (inst, iptr))
		iptr->change_position (p);
}

//

void AnimComponent::change_script_orientation(SCRIPT_INST inst, const Matrix & orient)
{
	Script* iptr;

	if (find_inst (inst, iptr))
		iptr->change_orientation (orient);
}

//

BOOL32 AnimComponent::get_script_start_height(SCRIPT_SET_ARCH arch, const char * name, float & start_height) const
{
	BOOL32 result = FALSE;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0))
	{
		ScriptSet const* aptr = reinterpret_cast<ScriptSet const*> (arch);

		SCRIPT_ARCH_MAP::const_iterator ait = aptr->scripts.find (name);

		if (result = (ait != aptr->scripts.end ()) && (*ait).second.start_height_valid ())
			start_height = (*ait).second.get_start_height ();
	}

	return result;
}

//

BOOL32 AnimComponent::script_contains_channel(SCRIPT_INST script_inst, CHANNEL_INSTANCE_INDEX channel_inst) const
{
	BOOL32 result = 0;

	Script const* iptr;

	if (find_inst (script_inst, iptr))
		result = iptr->contains_channel(channel_inst);

	return result;
}

//

BOOL32 AnimComponent::get_root_data(SCRIPT_INST script_inst, Vector & absolute, Vector & offset) const
{
	BOOL32 result = FALSE;

	Script const* iptr;

	if (find_inst (script_inst, iptr))
		result = iptr->get_root_data(absolute, offset);

	return result;
}

//

void AnimComponent::adjust_start_position(SCRIPT_INST script_inst, const Vector & dpos)
{
	Script* iptr;

	if (find_inst (script_inst, iptr))
		iptr->adjust_start_position(dpos);
}

float AnimComponent::get_duration(SCRIPT_INST inst) const
{
	float result = 0;

	Script const* iptr;

	if (find_inst (inst, iptr))
		result = iptr->archetype->get_duration();

	return result;
}

//

BOOL32 AnimComponent::get_instance_name(SCRIPT_INST inst, const char *& name) const
{
	BOOL32 result = FALSE;

	SINST_MAP::const_iterator cit = instances.find (inst);

	if (cit != instances.end ())
	{
		ScriptSet const* aptr = reinterpret_cast<ScriptSet const*> ((*cit).second.archset);

		for (SCRIPT_ARCH_MAP::const_iterator sarchit = aptr->scripts.begin ();
			sarchit != aptr->scripts.end ();
			sarchit++)
		{
			if (&((*sarchit).second) == (*cit).second.inst.archetype)
			{
				name = (*sarchit).first.c_str ();
				result = TRUE;
				break;
			}
		}
	}

	return result;
}

//

BOOL32 AnimComponent::get_script_start_height(SCRIPT_INST inst, float & start_height) const
{
	BOOL32 result = FALSE;

	Script const* iptr;

	if (find_inst (inst, iptr) && iptr->archetype && iptr->archetype->start_height_valid ())
	{
		start_height = iptr->archetype->get_start_height();
		result = TRUE;
	}

	return result;
}

//

CHANNEL_INSTANCE_INDEX AnimComponent::find_channel(SCRIPT_INST inst, Animation::Target & target)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	Script* iptr;

	if (find_inst (inst, iptr))
		result = iptr->find_channel(target);

	return result;
}

//
//-----------------------------------------------------------------------------------------------

void AnimComponent::update_target(const Target& target, TargetAffectors& affectors, float dt)
{
	MotionStackNode * list[64];
	int list_cnt = 0;

// DO update loop, THEN stack loop. Combining them is too confusing.
	ASSERT(affectors.motion_list.count() < 64);
	MotionStackNode * node = affectors.motion_list.first();
	while (node)
	{
		list[list_cnt++] = node;
		node = affectors.motion_list.next(node);
	}

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
					chan->object->change_position(Engine->get_position(target.object));
				}
	// ALWAYS REMOVE SOURCE?
				if (trans->remove_source)
				{
				// Recursively remove transition's source node...

					affectors.motion_list.remove_transition(node, false);
				}
				else
				{
				// Remove the transition, leave everything else.
					affectors.motion_list.free(node);
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
						affectors.motion_list.free(node);
					}
				}
				else
				{
				// Nothing comes after. See if there are previous active motions:
					MotionStackNode * prev = node->prev;
					if (prev)
					{
					// Transition back into previous motion.
						TransitionNode * trans = new TransitionNode(affectors.transition_duration, true);

						//PoseNode * pose = new PoseNode(chan);
						//t->motion_list.free(node);
						//t->motion_list.link(pose, next);
						chan->object->suspend();

					// Swap source/dest motions.
						affectors.motion_list.unlink(node);
						affectors.motion_list.link(node, prev);

						affectors.motion_list.link(trans);
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
			affectors.motion_list.verify();
		}
	}

	MotionStack stack;
	node = affectors.motion_list.first();
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
		node = affectors.motion_list.next(node);
	}

	if (!stack.is_empty())
	{
		int data_size;
		switch (target.type)
		{
			case JOINT:
			{
				const JointInfo *ji;
				if( (ji = Engine->get_joint_info( target.joint )) != NULL ) {
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
		float du[7];
		stack.evaluate(u, data_size);

	// Notify target.
		switch (target.type)
		{
			case JOINT:
			{
				// this will be done more correctly by the motion stack but IChannel2 has to change
				if( dt > 0.0f ) // compute rate of change
				{
					const float inv_dt = 1.0f / dt;
					Engine->get_joint_state( target.joint, IE_JST_BASIC, du );
					for(int i = 0; i < data_size; i++)
					{
						du[i] = (u[i] - du[i]) * inv_dt;
					}
					Engine->set_joint_state( target.joint, IE_JST_FIRST_DERIVATIVE, du );
				}
				Engine->set_joint_state( target.joint, IE_JST_BASIC, u );
				break;
			}
			case OBJECT:
			{
				Engine->set_position(target.object, *((Vector *) u));
				Engine->set_orientation(target.object, *((Quaternion *) (u+3)));
				break;
			}
		}
	}
	else // stop joint velocity
	{
		if( target.type == JOINT )
		{
			float du[7];
			memset(du, 0, 7 * sizeof(float));
			Engine->set_joint_state( target.joint, IE_JST_FIRST_DERIVATIVE, du );
		}
	}

	node = affectors.motion_list.first();
	while (node)
	{
		MotionStackNode * next = affectors.motion_list.next(node);
		if (node->delete_me)
		{
			affectors.motion_list.free(node);
		}
		node = next;
	}
}

//

void AnimComponent::update (SINGLE dt)
{
/*
	int tsize = targets.size ();
	int isize = cinstances.size ();
	int sisize = instances.size ();

	char buf[64];
	sprintf (buf, "targets %d, channels %d, scripts %d\n", tsize, isize, sisize);
	OutputDebugString (buf);
*/

// NEED TO UPDATE ANY CHANNELS THAT AREN'T IN A TARGET'S MOTION LIST
	CINST_SET::iterator it;
	for (it = cinstances.begin ();
		it != cinstances.end ();
		it++)
	{
		(*it)->clear_update_flag ();
	}

	TARGET_MAP::iterator tit = targets.begin ();

	while (tit != targets.end ())
	{
		update_target((*tit).first, (*tit).second, dt);

		if (!(*tit).second.motion_list.count ())
			tit = targets.erase (tit);
		else
			tit++;
	}

	for (it = cinstances.begin ();
		it != cinstances.end ();
		it++)
	{
		if ((*it)->active && !(*it)->was_updated())
		{
			if ((*it)->update(dt) == Channel::ANIMATION_COMPLETE)
			{
				(*it)->active = false;
				unregister_target_affector (*it);
			}
		}
	}
}

//

CHANNEL_INSTANCE_INDEX AnimComponent::create_channel_instance (Animation::IVirtualChannel * ctrl, unsigned int data_type, const Animation::Target& tgt)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	if (ctrl)
	{
		Object * obj;
		switch (data_type)
		{
			case DT_FLOAT:
				obj = new FloatStream(ctrl);
				break;
			case DT_VECTOR:
				obj = new VectorStream(ctrl);
				break;
			case DT_QUATERNION:
				obj = new QuatStream(ctrl);
				break;
			case DT_VECTOR | DT_QUATERNION:
				obj = new FullStream(ctrl);
				break;
			case DT_EVENT:
				obj = new Object(ctrl);
				break;
			default:
				GENERAL_FATAL("Unknown data type!\n");
		}

		if (obj->set_target (tgt))
		{
			cinstances.insert (obj);
			result = (CHANNEL_INSTANCE_INDEX)obj;
		}
		else
			delete obj;
	}

	return result;
}

CHANNEL_INSTANCE_INDEX AnimComponent::create_channel_instance (IChannel2* in, const Animation::Target& tgt)
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	if (in)
	{
		U32 dtype;
		GENRESULT gr = in->get_data_type (dtype);
		ASSERT (gr == GR_OK);

		Object * obj;
		switch (dtype)
		{
			case DT_FLOAT:
				obj = new FloatStream(in);
				break;
			case DT_VECTOR:
				obj = new VectorStream(in);
				break;
			case DT_QUATERNION:
				obj = new QuatStream(in);
				break;
			case DT_VECTOR | DT_QUATERNION:
				obj = new FullStream(in);
				break;
			case DT_EVENT:
				obj = new Object(in);
				break;
			default:
				GENERAL_FATAL("Unknown data type!\n");
		}

		if (obj->set_target (tgt))
		{
			cinstances.insert (obj);
			result = (CHANNEL_INSTANCE_INDEX)obj;
		}
	}

	return result;
}


//

GENRESULT AnimComponent::get_channel_archetype (CHANNEL_INSTANCE_INDEX idx, IChannel2*& out)
{
	GENRESULT result = GR_GENERIC;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
		{
			result = GR_OK;

			if (out = (*it)->archetype)
				out->AddRef ();
		}
	}

	return result;
}

GENRESULT AnimComponent::destroy_channel_instance (CHANNEL_INSTANCE_INDEX idx)
{
	GENRESULT result = GR_GENERIC;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Channel::Object*)idx);

		if (it != cinstances.end ())
		{
			unregister_target_affector (*it);
			result = GR_OK;

			delete (*it);
			cinstances.erase (it);
		}
	}

	return result;
}

void AnimComponent::unregister_target_affector (Object* obj)
{
// TO DO: If channel has never been started, target is invalid, so targets.search()
// will return undefined results.
// Skip this step if channel has no valid target.

//
// Remove from targets list.
//
	TARGET_MAP::iterator tit = targets.find (obj->get_target ());

	if (tit != targets.end ())
	{
		MotionList& mlist = (*tit).second.motion_list;

	// IF GIVEN CHANNEL IS SRC OF A TRANSITION, REPLACE IT WITH POSE NODE.
	// If destination of a transition, remove entire transition.

		MotionStackNode * node = mlist.first();
		while (node)
		{
			if (node->type == MSN_CHANNEL)
			{
				ChannelNode * chan = (ChannelNode *) node;
				if (chan->object == obj)
				{
					break;
				}

			}
			node = mlist.next(node);
		}

		if (node)
		{
			if (node->is_transition_source())
			{
			// replace with pose node.
			// also force the given transition node to destroy its source, regardless of its
			// current flag settings, since otherwise the pose node will linger.
				PoseNode * pose = new PoseNode((*tit).first);
				TransitionNode * trans = (TransitionNode *) node->next->next;
				trans->remove_source = true;
				mlist.link(pose, node->next);
			}
			else if (node->is_transition_destination())
			{
			// remove transition.
				TransitionNode * trans = (TransitionNode *) node->next;
				if (trans->is_transition_source())
				{
					PoseNode * pose = new PoseNode((*tit).first);
					TransitionNode * poseTrans = (TransitionNode *) trans->next->next;
					poseTrans->remove_source = true;
					mlist.link(pose, trans->next);
				}

				mlist.remove_transition(trans, false);
			}

			mlist.free(node);
		}

		mlist.verify();

		if (mlist.count() == 0)
		{
		// No more motions affect this target, so get rid of pool node.
			targets.erase (tit);
		}
	}
}

void AnimComponent::register_target_affector (Object* obj, float transition_duration, unsigned int flags)
{
//
// Add target to target hashpool if not already present, add object to target's list.
//
	TARGET_MAP::iterator tit = targets.find (obj->get_target ());

	if (tit == targets.end ())
		tit = targets.insert (TARGET_MAP::value_type (obj->get_target (), TargetAffectors ())).first;

	{
		MotionList& mlist = (*tit).second.motion_list;

	// See if it already exists...
		MotionStackNode * node = mlist.first();
		while (node)
		{
			if (node->type == MSN_CHANNEL)
			{
				ChannelNode * chan = (ChannelNode *) node;
				if (chan->object == obj)
					return;
			}
			node = mlist.next(node);
		}

		MotionStackNode * transition_source = NULL;
	
		bool transFromPose = false;		
		bool trans = (transition_duration > 0);
		if (trans)
		{
			if (mlist.count() == 0)
			{
			//
			// MOTION LIST IS EMPTY. Need to save current pose as 
			// source channel for transition.
			//
				PoseNode * pose = new PoseNode((*tit).first);
				ASSERT(pose);
				mlist.link(pose);

				transition_source = pose;
				transFromPose = true;
			}
			else
			{
				MotionStackNode * last = mlist.last();
				transition_source = last;
			}
		}
		else
		{
		// No transitions, no point in keeping around previous cinstances.
		// They'll just gum up the works.
			if (mlist.count() != 0)
			{
				MotionStackNode * n = mlist.first();
				while (n)
				{
					if (n->type == MSN_CHANNEL)
					{
						ChannelNode * c = (ChannelNode *) n;
						c->object->finish();
					}

					n = mlist.next(n);
				}

				mlist.free();
			}
		}

	//
	// Add new motion.
	//
		ChannelNode * chan = new ChannelNode(obj);
		ASSERT(chan);
		mlist.link(chan);

		if (trans)
		{
		//
		// Add transition.
		//
			TransitionNode * trans = new TransitionNode(transition_duration);

			if (flags & OVERLAY)
			{
				// If we are overlaying, set this flag to false to prevent the source node
				// from being removed when the transition completes.
				// However, if we are transitioning from a pose node (i.e. there were no motions from
				// which to transition), kill the pose node after the transition, to prevent it from
				// laying around after the transition is completed.
				if (!transFromPose)
				{
					trans->remove_source = false;
				}
			}

			mlist.link(trans);
			(*tit).second.transition_duration = transition_duration;

			if (obj->get_target ().type == OBJECT)
			{
				if ((flags & XLAT_BLEND_X) ||
					(flags & XLAT_BLEND_Y) ||
					(flags & XLAT_BLEND_Z))
				{
				// set up transition parameters. 

					Vector dx_dst = chan->get_translation(transition_duration);
					float dm = dot_product(dx_dst, dx_dst);
					trans->special_blend = true;
					trans->pstart = Engine->get_position(obj->get_target ().object);
					trans->v0 = transition_source->get_translation(transition_duration);

					trans->v0 *= trans->inv_duration;

					Vector v1 = dx_dst * trans->inv_duration;

					trans->dv = v1 - trans->v0;
				}
			}
		}

	// CHECK FOR VALID STATE.
		mlist.verify();
	}
}

bool AnimComponent::start (CHANNEL_INSTANCE_INDEX idx, Animation::StartParms* parms)
{
	bool result = false;

	if ((INVALID_CHANNEL_INSTANCE_INDEX != idx) && parms)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
		{
			result = (*it)->active = (*it)->start(*parms);

			if (result)
				register_target_affector (*it, parms->transition_duration, parms->flags);
		}
	}

	return result;
}

bool AnimComponent::stop (CHANNEL_INSTANCE_INDEX idx)
{
	bool result = false;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
		{
			if ((*it)->active)
			{
				(*it)->suspend();

				result = true;
				(*it)->active = false;
			}
		}
	}

	return result;
}

float AnimComponent::get_current_time (CHANNEL_INSTANCE_INDEX idx) const
{
	float result = -1.0;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			result = (*it)->get_current_time ();
	}

	return result;
}

bool AnimComponent::set_current_time (CHANNEL_INSTANCE_INDEX idx, float time)
{
	bool result = false;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			result = (*it)->set_current_time (time);
	}

	return result;
}

//

void AnimComponent::set_weight(CHANNEL_INSTANCE_INDEX idx, float weight)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			(*it)->set_weight(weight);
	}
}

//

float AnimComponent::get_weight(CHANNEL_INSTANCE_INDEX idx) const
{
	float result = 0;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			result = (*it)->get_weight();
	}

	return result;
}

//

void AnimComponent::change_position(CHANNEL_INSTANCE_INDEX idx, const Vector & p)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			(*it)->change_position(p);
	}
}

//

void AnimComponent::change_orientation(CHANNEL_INSTANCE_INDEX idx, const Matrix & R)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			(*it)->change_orientation(R);
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

void COMAPI AnimComponent::update_instance (INSTANCE_INDEX root, SINGLE dt)
{
	/*
	int tsize = targets.size ();
	int isize = cinstances.size ();
	int sisize = instances.size ();

	char buf[64];
	sprintf (buf, "targets %d, channels %d, scripts %d\n", tsize, isize, sisize);
	OutputDebugString (buf);
	*/

	if (root != INVALID_INSTANCE_INDEX)
	{
		INSTANCE_INDEX objects[512];
		JOINT_INDEX joints[512];

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

				TARGET_MAP::iterator tit = targets.find (target);

				if (tit != targets.end ())
				{
					update_target((*tit).first, (*tit).second, dt);

					if (!(*tit).second.motion_list.count ())
						targets.erase (tit);
				}
			}
		}

		if (num_joints)
		{
			target.type = JOINT;
			for (int i = 0; i < num_joints; i++)
			{
				target.joint = joints[i];
				TARGET_MAP::iterator tit = targets.find (target);

				if (tit != targets.end ())
				{
					update_target((*tit).first, (*tit).second, dt);

					if (!(*tit).second.motion_list.count ())
						targets.erase (tit);
				}
			}
		}

	// just search the whole goddamn list.
		
		TARGET_MAP::iterator shit = targets.begin ();

		while (shit != targets.end ())
		{
			if (((*shit).first.type == EVENT) && ((*shit).first.object == root))
			{
			 	update_target((*shit).first, (*shit).second, dt);

				if (!(*shit).second.motion_list.count ())
					shit = targets.erase (shit);
				else
					shit++;
			}
			else
				shit++;
		}
	}
}

//

bool COMAPI AnimComponent::channel_in_use(CHANNEL_INSTANCE_INDEX idx) const
{
	bool result = false;

	if (idx != INVALID_CHANNEL_INSTANCE_INDEX)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
		{
			Object * obj = (*it);

			TARGET_MAP::const_iterator tit = targets.find (obj->get_target ());

			if (tit != targets.end ())
			{
				MotionStackNode * node = (*tit).second.motion_list.first();
				while (node)
				{
					if (node->type == MSN_CHANNEL)
					{
						ChannelNode * chan = (ChannelNode *) node;
						if (chan->object == obj)
						{
							result = true;
							break;
						}
					}

					node = (*tit).second.motion_list.next(node);
				}
			}
		}
	}

	return result;
}

//

Animation::Target AnimComponent::get_channel_target(CHANNEL_INSTANCE_INDEX idx) const
{
	Animation::Target result;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			result = (*it)->get_target();
	}

	return result;
}

//

void AnimComponent::adjust_start_position(CHANNEL_INSTANCE_INDEX idx, const Vector & dp)
{
	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
			(*it)->adjust_start_position(dp);
	}
}

GENRESULT AnimComponent::is_forward (CHANNEL_INSTANCE_INDEX idx, bool& out) const
{
	GENRESULT result = GR_INVALID_PARMS;

	if (INVALID_CHANNEL_INSTANCE_INDEX != idx)
	{
		CINST_SET::const_iterator it = cinstances.find ((Object*)idx);

		if (it != cinstances.end ())
		{
			result = GR_OK;
			out = !(*it)->is_backwards ();
		}
	}

	return result;
}

GENRESULT AnimComponent::enumerate_scripts_in_file (SCRIPT_ENUM_CALLBACK callback, IFileSystem* fs, void* misc) const
{
	return EnumScripts (callback, fs, misc);
}

GENRESULT AnimComponent::verify_loaded (SCRIPT_SET_ARCH arch, const char *name)
{
	GENRESULT result = GR_GENERIC;

	if ((arch != INVALID_SCRIPT_SET_ARCH) && (arch != 0) && name != NULL)
	{
		ScriptSet * set = reinterpret_cast<ScriptSet *> (arch);
		if (set->scripts.find (name) != set->scripts.end())
		{
			result = GR_OK;
		}
	}

	return result;
}

//


AnimComponent::AnimComponent (void)
{
	next_inst = 0xED;
}

GENRESULT AnimComponent::init (SYSCONSUMERDESC* desc)
{
	return GR_OK;
}

GENRESULT AnimComponent::Initialize (void)
{
	GENRESULT r = GR_GENERIC;

	if ((r = ((IAnimation*)this)->QueryInterface (IID_IEngine, (void**)&Engine)) != GR_OK)
		return r;
	((IAnimation*)this)->Release();		// release the extra reference

	r = GR_OK;

	return r;
}

AnimComponent::~AnimComponent (void)
{
//	targets.free();

#ifndef FINAL_RELEASE
	SINST_MAP::const_iterator rit = instances.begin ();

	for (; rit != instances.end (); rit++)
	{
		const char* name = NULL;
		get_instance_name ((*rit).first, name);
		GENERAL_TRACE_1 (TEMPSTR("Anim script %d (%s) [refers to arch %d] is still active\n", (*rit).first, name, (*rit).second.archset));
	}

/*
	SARCH_SET::const_iterator it = arch_sets.begin ();

	for (; it != arch_sets.end (); it++)
	{
		GENERAL_TRACE_1 (TEMPSTR("Anim script set archetype %d has %d dangling references\n", (*it).first, (*it).second.ref_count));
	}
*/
#endif
}

// IAnimation3

GENRESULT AnimComponent::enumerate_channels2 (CHANNEL_INSTANCE_ENUM_CALLBACK cb, SCRIPT_INST inst, void* misc) const 
{
	GENRESULT result = GR_GENERIC;

	const Script* iptr;

	if (find_inst (inst, iptr))
	{
		result = GR_OK;

		for (CHANNEL_VECTOR::const_iterator it = iptr->channels.begin ();
			it != iptr->channels.end ();
			it++)
		{
			if (!cb (reinterpret_cast<CHANNEL_INSTANCE_INDEX> (*it), misc))
				break;
		}
	}
		
	return result;
}

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL,
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			
			DA_HEAP_ACQUIRE_HEAP( HEAP );
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			IComponentFactory* server = new DAComponentFactory2<DAComponentAggregate<AnimComponent>, SYSCONSUMERDESC> ("IAnimation");

			if (server)
			{
				DACOM_Acquire ()->RegisterComponent(server, "IAnimation", DACOM_LOW_PRIORITY);
				server->Release();
			}
		}
		break;
	}
	return TRUE;
}


