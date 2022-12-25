#ifndef __ANIM_H
#define __ANIM_H

#include "Engine.h"
#include "AnimTypes.h"
#include "channel.h"
#include "TComponent.h"
#include "IAnim.h"

#pragma warning (disable : 4786)

#include <map>
#include <vector>
#include <string>

using namespace Animation;

	class Script;

	extern IEngine* Engine;

	//EMAURER the a database of channel archetypes used by a script or set of scripts.
	typedef std::map<std::string, COMPTR<IChannel2> > CHANNEL_ARCH_MAP;
	GENRESULT LoadChannels (CHANNEL_ARCH_MAP&, IFileSystem*);
	GENRESULT EnumScripts (SCRIPT_ENUM_CALLBACK, IFileSystem* fs, void* misc=0);

	//EMAURER describes a mapping from a joint to the channel archetype that is to drive it.
	struct ChannelMapping
	{
		//EMAURER likely that only one of these will be used. To save space, do not make 'em fixed-length.
		std::string parent;
		std::string child;

		COMPTR<IChannel2> archetype;
	};

	typedef std::vector<ChannelMapping>	CHANNEL_MAPPING_VECTOR;

	//EMAURER a collection of channel mappings designed to affect a compound object.
	class ScriptArchetype
	{
		//EMAURER don't let anyone with a pointer to this, delete it.
		void operator delete (void*);

		friend Script;

			COMPTR<IChannel2> longest_channel;

			BOOL32 sh_valid;
			float start_height;

			void read_parent (IFileSystem* fs, ChannelMapping* map);
			void read_child (IFileSystem* fs, ChannelMapping* map);
			void read_channel (IFileSystem* fs, 
								ChannelMapping* map, 
								const CHANNEL_ARCH_MAP& archetypes);

			void init_new_format (IFileSystem* fs, const CHANNEL_ARCH_MAP& archetypes);

		public:

			GENRESULT create_instance (Script&) const;

			float get_duration (void) const;

			BOOL32 start_height_valid(void) const
			{
				return sh_valid;
			}

			float get_start_height(void) const
			{
				return start_height;
			}

			ScriptArchetype (const char* _name, IFileSystem* fs, bool is_dir, const CHANNEL_ARCH_MAP& archetypes);

			CHANNEL_MAPPING_VECTOR channels;
	};

	inline float ScriptArchetype::get_duration (void) const
	{
		SINGLE dur;

		GENRESULT gr = longest_channel->get_duration (dur);

		if (gr == GR_OK)
			return dur;
		else
			return -1;
	}

	// A database of named script archetypes.
	typedef std::map<std::string, ScriptArchetype> SCRIPT_ARCH_MAP;

	GENRESULT LoadScripts (SCRIPT_ARCH_MAP&, const CHANNEL_ARCH_MAP&, IFileSystem*);

	//Contains all archetype information, channels and scripts, 
	//loaded from a compound file.
	struct ScriptSet : public IDAComponent
	{
		SCRIPT_ARCH_MAP scripts;

		GENRESULT load (IFileSystem* parent);
		GENRESULT load_subset (IFileSystem* parent, const char *nameList[], int nameCount);
		GENRESULT unload_subset (const char *nameList[], int nameCount);

		DEFMETHOD_ (U32, AddRef) (void);
		DEFMETHOD_ (U32, Release) (void);

		ScriptSet (void)
		{
			ref_count = 1;
		}

		private:

			DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);

			LONG ref_count;

			~ScriptSet (void)
			{
			}
	};

//-------------------------------------------------------------------------------
//Instance releated objects.  
		
	typedef std::vector<Channel::Object*> CHANNEL_VECTOR;
	
	//A 'Script' is a collection of channels.
	class Script
	{
			Channel::Object* longest;

		public:
		
			const ScriptArchetype* archetype;

			Script (void);
			void load (const ScriptArchetype* _archetype);

			void connect (INSTANCE_INDEX root, IEventHandler *, void* user_data);

			int start (unsigned int flags = FORWARD, float time=CUR, float scale=1.0f, float transition_duration=0.0f, float weight=1.0f, float translation_scale=1.0, const Transform * start_xform = NULL);
			void stop (void);

			void set_current_time(float t);
			float get_current_time (void) const;

			void change_position(const Vector & p);
			void change_orientation(const Matrix & orient);

			void adjust_start_position(const Vector & dp);

			void free (void);

			CHANNEL_INSTANCE_INDEX find_channel(Target & target) const;

			BOOL32 all_channels_finished(const IChannel*) const;

			BOOL32 contains_channel(CHANNEL_INSTANCE_INDEX chan) const;
			BOOL32 get_root_data(Vector & absolute, Vector & offset) const;

			CHANNEL_VECTOR channels;
	};

#endif