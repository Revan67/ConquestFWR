/*
	assume that the first channel that contains only vector data is the "root"
*/



#include <Windows.h>
#include "Anim.h"
#include "PersistAnim.h"
#include "FileSys.h"
#include "fdump.h"
#include "tempstr.h"
#include "channel.h"

IEngine* Engine = NULL;

using namespace Animation;

#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int result = (x);	\
		ASSERT (result);	\
	}
#else								
	 #define CHECK(x)	(void)(x);
#endif

unsigned int JointStemLen;
unsigned int ObjectStemLen;
unsigned int EventStemLen;

INSTANCE_INDEX	FindObj(INSTANCE_INDEX root, const char* child_name)
{
	const char const	*mname	=Engine->get_instance_part_name(root);
	INSTANCE_INDEX		result	=-1;
	INSTANCE_INDEX		child	=-1;

	if(mname && child_name)
	{
		if(!strcmp(mname, child_name))
		{
			result	=root;
			return	result;
		}
	}

	while((-1 == result) && ((child = Engine->get_instance_child_next (root, EN_DONT_RECURSE, child)) != -1))
	{
		result	=FindObj(child, child_name);
	}

	return	result;
}


GENRESULT LoadChannels (CHANNEL_ARCH_MAP& chnldb, IFileSystem* fs)
{
	ASSERT(fs);

	GENRESULT result = GR_OK;

	if (fs->SetCurrentDirectory ("Chnl"))
	{
		WIN32_FIND_DATA find_data;

		HANDLE srch = fs->FindFirstFile ("*", &find_data);

		if (INVALID_HANDLE_VALUE != srch)
		{
			do
			{
				if ((find_data.cFileName[0] != '.') && 
					(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					CHANNEL_ARCH_MAP::const_iterator it = chnldb.find (find_data.cFileName);

					if (it == chnldb.end ())
					{
						COMPTR<IFileSystem> cfs;
						DAFILEDESC cdesc (find_data.cFileName);
						cdesc.hFindFirst = srch;

						fs->CreateInstance (&cdesc, cfs);

						COMPTR<IChannel2> ptr;

						DACOMDESC desc (IID_IChannel2);

						GENRESULT ok = DACOM_Acquire ()->CreateInstance (&desc, ptr);
						
						if (ok == GR_OK)
						{
							if (GR_OK == (result = ptr->load (cfs)))
								chnldb.insert (CHANNEL_ARCH_MAP::value_type (find_data.cFileName, ptr));
						}
					}
				}
				
				if (!fs->FindNextFile (srch, &find_data))
				{
					ASSERT (ERROR_NO_MORE_FILES == fs->GetLastError ());
					break;
				}
			}
			while (true);

			CHECK (fs->FindClose (srch));
		}

		fs->SetCurrentDirectory ("..");
	}

	return result;
}

GENRESULT ScriptArchetype::create_instance (Script& s) const
{
	s.load(this);
	return GR_OK;
}

void ScriptArchetype::read_parent (IFileSystem* fs, ChannelMapping* map)
{
	ASSERT(fs);
	ASSERT(map);

	DAFILEDESC desc (PersistAnimParentFilename);
	desc.dwDesiredAccess = GENERIC_READ;
	desc.dwCreationDistribution = OPEN_EXISTING;

	//read in parent object name

	HANDLE handle = fs->OpenChild (&desc);
	ASSERT ((HANDLE)-1 != handle);

	DWORD fsize = fs->GetFileSize (handle, NULL);
	ASSERT (fsize < PersistAnimTARGET_NAME_MAX);

	DWORD read_size;

	char buf[PersistAnimTARGET_NAME_MAX];

	fs->ReadFile (handle, buf, fsize, &read_size);
	ASSERT (read_size == fsize);

	map->parent = buf;

	fs->CloseHandle (handle);
}

void ScriptArchetype::read_child (IFileSystem* fs, ChannelMapping* map) 
{
	ASSERT(fs);
	ASSERT(map);

	DAFILEDESC desc (PersistAnimChildFilename);
	desc.dwDesiredAccess = GENERIC_READ;
	desc.dwCreationDistribution = OPEN_EXISTING;

	HANDLE handle = fs->OpenChild (&desc);
	ASSERT ((HANDLE)-1 != handle);

	DWORD fsize = fs->GetFileSize (handle, NULL);
	ASSERT (fsize < PersistAnimTARGET_NAME_MAX);

	char buf[PersistAnimTARGET_NAME_MAX];

	DWORD read_size;

	fs->ReadFile (handle, buf, fsize, &read_size);
	ASSERT (read_size == fsize);

	map->child = buf;

	fs->CloseHandle (handle);
}

void ScriptArchetype::read_channel (IFileSystem* fs, ChannelMapping* map, const CHANNEL_ARCH_MAP& archetypes) 
{
	ASSERT(fs);
	ASSERT(map);

	DAFILEDESC desc (PersistAnimChannelFilename);

	HANDLE handle = fs->OpenChild (&desc);
	
	if (handle != INVALID_HANDLE_VALUE)
	{
		DWORD fsize = fs->GetFileSize (handle, NULL);
		ASSERT (fsize < PersistAnimCHANNEL_NAME_MAX);

		DWORD read_size;

		char channel_name[PersistAnimCHANNEL_NAME_MAX];

		fs->ReadFile (handle, channel_name, fsize, &read_size);
		ASSERT (read_size == fsize);

		fs->CloseHandle (handle);

		CHANNEL_ARCH_MAP::const_iterator it (archetypes.find (channel_name));

		ASSERT (it != archetypes.end ());
		map->archetype = it->second;

		ASSERT (map->archetype);
	}
	else if (fs->SetCurrentDirectory (PersistAnimChannelData))
	{
		DACOMDESC desc (IID_IChannel2);

		GENRESULT ok = DACOM_Acquire ()->CreateInstance (&desc, map->archetype);
		
		if (ok == GR_OK)
		{
			ok = map->archetype->load (fs);
			ASSERT ((ok == GR_OK) && "failed to load channel");
		}

		fs->SetCurrentDirectory ("..");
	}
	else
	{
		ASSERT (0 && "no channel found in channel map");
	}
}

void ScriptArchetype::init_new_format (IFileSystem* fs, const CHANNEL_ARCH_MAP& archetypes)
{
	ASSERT(fs);

	WIN32_FIND_DATA find_data;

	HANDLE srch = fs->FindFirstFile ("*", &find_data);

	if (INVALID_HANDLE_VALUE != srch)
	{
		do
		{
			if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (!strcmp(find_data.cFileName, PersistAnimRootHeightFilename))
				{
					COMPTR<IFileSystem> cfs;

					DAFILEDESC desc = find_data.cFileName;
					desc.hFindFirst = srch;

					GENRESULT h = fs->CreateInstance (&desc, cfs);

					if (h == GR_OK)
					{
						DWORD bytes_read;
						cfs->ReadFile(NULL, &start_height, sizeof(float), &bytes_read);
						ASSERT(bytes_read == sizeof(float));

						sh_valid = TRUE;
					}
				}
			}
			else if (find_data.cFileName[0] != '.')
			{
				if (!strncmp (find_data.cFileName, PersistAnimJointMapStem, JointStemLen))
				{
					COMPTR<IFileSystem> cfs;

					DAFILEDESC desc = find_data.cFileName;
					desc.hFindFirst = srch;

					fs->CreateInstance (&desc, cfs);

					ChannelMapping channelEntry;

					//read in parent object name

					read_parent (cfs, &channelEntry);

					//read in child object name

					read_child (cfs, &channelEntry);

					//read in channel name

					read_channel (cfs, &channelEntry, archetypes);

					channels.push_back (channelEntry);
				}
				else if (!strncmp (find_data.cFileName, PersistAnimObjectMapStem, ObjectStemLen))
				{
					ChannelMapping channelEntry;

					COMPTR<IFileSystem> cfs;

					DAFILEDESC desc = find_data.cFileName;
					desc.hFindFirst = srch;

					fs->CreateInstance (&desc, cfs);

					read_parent (cfs, &channelEntry);
					read_channel (cfs, &channelEntry, archetypes);

					channels.push_back (channelEntry);
				}
				else if (!strncmp (find_data.cFileName, PersistAnimEventMapStem, EventStemLen))
				{
					ChannelMapping channelEntry;

					COMPTR<IFileSystem> cfs;

					DAFILEDESC desc = find_data.cFileName;
					desc.hFindFirst = srch;

					fs->CreateInstance (&desc, cfs);

					read_channel (cfs, &channelEntry, archetypes);

					channels.push_back (channelEntry);
				}
			}
			
			if (!fs->FindNextFile (srch, &find_data))
			{
				int e = fs->GetLastError ();
				ASSERT (ERROR_NO_MORE_FILES == e);
				break;
			}
		}
		while (true);

		CHECK (fs->FindClose (srch));

		//get length (in seconds) of longest channel

		float duration = -1;

		for (CHANNEL_MAPPING_VECTOR::const_iterator cit = channels.begin ();
			cit != channels.end ();
			cit++)
		{
			SINGLE d;
			GENRESULT gr = (*cit).archetype->get_duration (d);
			ASSERT (gr == GR_OK);

			if (duration < d)
			{
				longest_channel = (*cit).archetype;
				duration = d;
			}
		}
	}
}

ScriptArchetype::ScriptArchetype (const char* _name, 
									IFileSystem* fs, 
									bool is_dir,
									const CHANNEL_ARCH_MAP& archetypes) : 
										start_height(0.0f)

{
	ASSERT (_name);
	ASSERT(fs);

	sh_valid = FALSE;

//EMAURER  if '_name' is a file, data is "old" format.  if '_name' is
//a directory, data is "new" format.

	if (is_dir)
		init_new_format (fs, archetypes);
	else
	{
	//"old" format
		DWORD fsize = fs->GetFileSize (NULL, NULL);
		unsigned int num_channels = fsize / sizeof (PersistAnimChannelMapping);

		ASSERT (fsize % sizeof (PersistAnimChannelMapping) == 0);

		PersistAnimChannelMapping* mappings = new PersistAnimChannelMapping[num_channels];

		DWORD read_size;

		fs->ReadFile (NULL, mappings, sizeof (PersistAnimChannelMapping) * num_channels, &read_size);
		ASSERT (read_size == sizeof (PersistAnimChannelMapping) * num_channels);

		channels.resize(num_channels);

		for (unsigned int i = 0; i < num_channels; i++)
		{
			channels[i].parent = mappings[i].parent;
			channels[i].child =  mappings[i].child;

			CHANNEL_ARCH_MAP::const_iterator it = archetypes.find (mappings[i].channel);

			ASSERT (it != archetypes.end ());
			channels[i].archetype = (*it).second;

			//get length (in seconds) of longest channel

			float duration = -1.0f;

			SINGLE d;
			GENRESULT gr = channels[i].archetype->get_duration (d);
			ASSERT (gr == GR_OK);

			if (duration < d)
			{
				longest_channel = channels[i].archetype;
				duration = d;
			}
		}

		delete [] mappings;
	}
}

//

GENRESULT LoadScripts (SCRIPT_ARCH_MAP& map, const CHANNEL_ARCH_MAP& channels, IFileSystem* fs)
{
	ASSERT(fs);

	if (fs->SetCurrentDirectory ("Script"))
	{
		JointStemLen = strlen (PersistAnimJointMapStem);
		ObjectStemLen = strlen (PersistAnimObjectMapStem);
		EventStemLen = strlen (PersistAnimEventMapStem);
		
		WIN32_FIND_DATA find_data;

		HANDLE srch = fs->FindFirstFile ("*", &find_data);

		if (INVALID_HANDLE_VALUE != srch)
		{
			do
			{
				if (find_data.cFileName[0] != '.')
				{
					//EMAURER  removed assert below.  "new" data format permits
					//this "file" to be a directory.

					//ASSERT (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

					DAFILEDESC dsc (find_data.cFileName);
					dsc.hFindFirst = srch;

					COMPTR<IFileSystem> cfs;
					fs->CreateInstance (&dsc, cfs);

					map.insert (SCRIPT_ARCH_MAP::value_type (find_data.cFileName, 
																ScriptArchetype (find_data.cFileName, 
																				cfs, 
																				(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0, 
																				channels)));
				}
				
				if (!fs->FindNextFile (srch, &find_data))
				{
					int e = fs->GetLastError ();
					ASSERT (ERROR_NO_MORE_FILES == e);
					break;
				}
			}
			while (true);

			CHECK (fs->FindClose (srch));
		}

		fs->SetCurrentDirectory ("..");
	}

	return GR_OK;
}

GENRESULT EnumScripts (SCRIPT_ENUM_CALLBACK callback, IFileSystem* fs, void* misc)
{
	ASSERT(fs);

	GENRESULT gr = GR_GENERIC;

	if (fs->SetCurrentDirectory ("Animation"))
	{
		// Determine if this is old or new format. It is old format if it has a "Chnl" directory
		if (fs->SetCurrentDirectory ("Chnl"))
		{
			// Old format.
			fs->SetCurrentDirectory ("..");
		}
		else
		{
			// New format.
			if (fs->SetCurrentDirectory ("Script"))
			{
				gr = GR_OK;
				WIN32_FIND_DATA find_data;

				HANDLE srch = fs->FindFirstFile ("*", &find_data);

				if (INVALID_HANDLE_VALUE != srch)
				{
					do
					{
						if (find_data.cFileName[0] != '.')
						{
							//EMAURER  removed assert below.  "new" data format permits
							//this "file" to be a directory.

							//ASSERT (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
							if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							{
								callback (find_data.cFileName, misc);
							}
						}
						
						if (!fs->FindNextFile (srch, &find_data))
						{
							int e = fs->GetLastError ();
							ASSERT (ERROR_NO_MORE_FILES == e);
							break;
						}
					}
					while (true);

					CHECK (fs->FindClose (srch));
				}

				fs->SetCurrentDirectory ("..");
			}
		}

		fs->SetCurrentDirectory ("..");
	}

	return gr;
}

GENRESULT LoadSubScripts (SCRIPT_ARCH_MAP& map, IFileSystem* fs, const char *nameList[], int nameCount)
{
	ASSERT(fs);

	if (fs->SetCurrentDirectory ("Script"))
	{
		JointStemLen = strlen (PersistAnimJointMapStem);
		ObjectStemLen = strlen (PersistAnimObjectMapStem);
		EventStemLen = strlen (PersistAnimEventMapStem);
		
		// Walk the list of names, attempting to load each one as a new style animation.
		for (int i = 0; i < nameCount; ++i)
		{
			const char *filename = nameList[i];
			if (filename == NULL)
			{
				GENERAL_WARNING ("Null name in animation sub set. Aborting subset load.\n");
				break;
			}

			// New style animations are stored in directories only.
			if (fs->GetFileAttributes (filename) & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Ensure that the filename is not already in the map. If it is, skip loading here, because it
				// is already loaded.

				if (map.find (filename) == map.end())
				{
					DAFILEDESC dsc (filename);
					COMPTR<IFileSystem> cfs;
					if (fs->CreateInstance (&dsc, cfs) == GR_OK)
					{
						CHANNEL_ARCH_MAP channels; // dummy value. Not used by the ScriptArchetype constructor for new style
						map.insert (SCRIPT_ARCH_MAP::value_type (filename, 
																 ScriptArchetype (filename, 
																				  cfs, 
																				  true, 
																				  channels)));
					}
					else
					{
						GENERAL_WARNING (TEMPSTR("Failed to open animation \"%s\". Skipping.\n", filename));
					}
				}
				else
				{
					GENERAL_TRACE_1 (TEMPSTR("Skipping load of animation \"%s\". Already loaded.\n", filename));
				}
			}
			else
			{
				GENERAL_WARNING (TEMPSTR("Unknown or old style animation \"%s\". Skipping.\n", filename));
			}
		}

		fs->SetCurrentDirectory ("..");
	}

	return GR_OK;
}

//------------------

GENRESULT ScriptSet::load (IFileSystem* parent)
{
	ASSERT(parent);

	GENRESULT gr = GR_GENERIC;

	if (parent->SetCurrentDirectory ("Animation"))
	{
		CHANNEL_ARCH_MAP channels;
		LoadChannels (channels, parent);
		LoadScripts (scripts, channels, parent);

		parent->SetCurrentDirectory ("..");

		gr = GR_OK;
	}

	return gr;
}

//------------------

GENRESULT ScriptSet::load_subset (IFileSystem* parent, const char *nameList[], int nameCount)
{
	ASSERT(parent);

	GENRESULT gr = GR_GENERIC;

	if (parent->SetCurrentDirectory ("Animation"))
	{
		// Determine if this is old or new format. If it has a channels directory, it is old format and
		// this won't work.
		if (parent->SetCurrentDirectory ("Chnl"))
		{
			parent->SetCurrentDirectory ("..");
		}
		else
		{
			gr = LoadSubScripts (scripts, parent, nameList, nameCount);
		}

		parent->SetCurrentDirectory ("..");
	}

	return gr;
}

//------------------

GENRESULT ScriptSet::unload_subset (const char *nameList[], int nameCount)
{
	// Unload all of the animations whose names match those in the list.
	for (int i = 0; i < nameCount; ++i)
	{
		const char *name = nameList[i];
		if (name)
		{
			if (scripts.erase (name) == 0)
			{
				GENERAL_WARNING (TEMPSTR("Failed to remove animation \"%s\".\n", name));
			}
		}
	}

	return GR_OK;
}

//------------------

Script::Script (void)
{
	longest = NULL;
	archetype = NULL;
}

void Script::load (const ScriptArchetype* _archetype)
{
	archetype = _archetype;

	ASSERT (archetype);
	ASSERT (archetype->channels.size ());

	//create channel instances based on the channel archetypes 
	//in the ScriptArchetype.

	channels.resize (archetype->channels.size());

	CHANNEL_VECTOR::iterator inst_it = channels.begin ();

	for (CHANNEL_MAPPING_VECTOR::const_iterator i = archetype->channels.begin (); 
		i < archetype->channels.end (); 
		i++, inst_it++)
	{
		IChannel2* channel_arch = (*i).archetype;

		Channel::Object* object = NULL;

		U32 dtype;
		channel_arch->get_data_type (dtype);

		switch (dtype)
		{
			case DT_FLOAT:
				object = new Channel::FloatStream (channel_arch);
				break;
			case DT_VECTOR:
				object = new Channel::VectorStream (channel_arch);
				break;
			case DT_QUATERNION:
				object = new Channel::QuatStream (channel_arch);
				break;
			case DT_QUATERNION | DT_VECTOR:
				object = new Channel::FullStream (channel_arch);
				break;
			case DT_EVENT:
				object = new Channel::Object (channel_arch);
				break;
		}

		ASSERT (object);

		object->active = false;
		(*inst_it) = object;
	}
}

BOOL32 Script::all_channels_finished (const IChannel* ICHANNEL) const
{
	BOOL32 result = TRUE;

	CHANNEL_VECTOR::const_iterator chan = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	while (chan < end)
	{
	// Don't consider event channels when deciding whether channels are finished or not.
		U32 dtype = (*chan)->get_data_type ();

		if ((dtype != DT_EVENT) && ICHANNEL->channel_in_use(CHANNEL_INSTANCE_INDEX(*chan)))
		{
			result = FALSE;
			break;
		}
		chan++;
	}


	return result;
}

BOOL32 Script::contains_channel(CHANNEL_INSTANCE_INDEX channel_instance) const
{
	BOOL32 result = FALSE;

	CHANNEL_VECTOR::const_iterator chan = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	while (chan < end)
	{
		CHANNEL_INSTANCE_INDEX idx = (CHANNEL_INSTANCE_INDEX)(*chan);
		if (idx == channel_instance)
		{
			result = TRUE;
			break;
		}
		chan++;
	}

	return result;
}

//

BOOL32 Script::get_root_data(Vector & absolute, Vector & offset) const
{
	CHANNEL_VECTOR::const_iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	BOOL32 result = FALSE;

	for ( ; begin < end; begin++)
	{
		if ((*begin)->get_data_type() & DT_VECTOR)
		{
			(*begin)->get_current_offset(&absolute.x, &offset.x);
			result = TRUE;
			break;
		}
	}

	return result;
}

//

bool BuildTarget (Target& target, INSTANCE_INDEX root, const ChannelMapping& mapping, IEventHandler * cb, void* user_data)
{
	bool result = true;

	if (root == INVALID_INSTANCE_INDEX && !cb)
	{
		memset (&target, 0, sizeof (target));
		result = false;
	}
	else
	{
		target.user_data = user_data;

		//map to either a joint or an object

		result = false;

		if (!mapping.child.empty ())
		{
			INSTANCE_INDEX c = FindObj (root, mapping.child.c_str ());

			if (c != INVALID_INSTANCE_INDEX)
			{
				INSTANCE_INDEX p = Engine->get_instance_parent(c);
			/*
				INSTANCE_INDEX p = FindObj (root, parent);
			
				if (p == INVALID_INSTANCE_INDEX && Engine->get_parent (c) == root)
					p = root;
			*/
				if (INVALID_INSTANCE_INDEX != p && mapping.parent == Engine->get_instance_part_name (p))
				{
					if( Engine->get_instance_parent(c) == p ) {
						target.type = JOINT;
						target.joint = c;
						target.event_handler = cb;

						result = true;
					}
					else if( Engine->get_instance_parent(p) == c ) {
						target.type = JOINT;
						target.joint = p;
						target.event_handler = cb;

						result = true;
					}
				}
			}
		}
		else if (!mapping.parent.empty ())
		{
			INSTANCE_INDEX t = FindObj (root, mapping.parent.c_str ());

			if (INVALID_INSTANCE_INDEX == t)
				t = root;
			
			target.type = OBJECT;
			target.object = t;
			target.event_handler = cb;

			result = true;
		}
		else
		{
			target.object = root;
			target.event_handler = cb;

		// Event handler function should be optional. Let things
		// continue even if there's no handler.

			target.type = EVENT;
			result = target.event_handler != NULL;
		}
	}

	return result;
}

void Script::connect (INSTANCE_INDEX root, IEventHandler * cb, void* user_data)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();

	unsigned int i = 0;

	longest = NULL;
	float ldur = -1.0f;

	while (begin < channels.end ())
	{
		Target target;

		if (!BuildTarget (target, root, archetype->channels[i], cb, user_data) ||
			!(*begin)->set_target (target))
		{
			//EMAURER only build database of the channels that worked correctly.

			const ChannelMapping& map = archetype->channels[i];

			if (!map.child.empty ())
			{
				GENERAL_WARNING (TEMPSTR("failed to map JOINT (parent: %s, child: %s) channel.\n",
				map.parent.c_str (), map.child.c_str ()) );
			}
			else if (!map.parent.empty ())
			{
				GENERAL_WARNING (TEMPSTR("failed to map OBJECT (%s) channel.\n",	map.parent.c_str ()));
			}
			else
			{
				//EMAURER TNB told me this should be TRACE_1 and that the others should be WARNINGs
				GENERAL_TRACE_1 (TEMPSTR("failed to map EVENT channel (no event_handler supplied?).\n"));
			}

			delete *begin;
			begin = channels.erase (begin);			
		}
		else
		{
			float d;
			GENRESULT gr = archetype->channels[i].archetype->get_duration (d);
			ASSERT (gr == GR_OK);

			if (ldur < d)
			{
				longest = *begin;
				ldur = d;
			}

			begin++;
		}

		i++;
	}
}

int Script::start (unsigned int flags, float time, float scale, float transition_duration, float weight, float translation_scale, const Transform * start_xform)
{
	int result = 0;

	StartParms parms;

	parms.flags = flags & (LOOP|OSCILLATE|XLAT_BLEND_X|XLAT_BLEND_Y|XLAT_BLEND_Z|NO_XLAT_OFFSET|OVERLAY);

	parms.time_scale = (flags & BACKWARDS) ? -1 : 1;
	parms.time_scale *= fabs (scale);

	parms.transition_duration = transition_duration;
	parms.weight = weight;
	parms.translation_scale = translation_scale;
	parms.xform = start_xform;
	parms.start_time = time;

	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::iterator end = channels.end();

	if (flags & BACKWARDS)
	{
		float dur = -1;

		for (; begin < end; ++begin)
		{
			if (dur < 0.0f)
				dur = (*begin)->get_duration ();
			else if (dur != (*begin)->get_duration ())
				result = 1;
		}
	}

	//EMAURER if a script is started at a time that is past the end of
	//a channel that is part of the script, set that channel to it's end.

	for (begin = channels.begin (); begin < end; ++begin)
	{
		if (!(*begin)->start (parms))
			result = -1;
	}

	return result;
}

void Script::stop (void)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	for (; begin < end; ++begin)
	{
		(*begin)->active = false;
		(*begin)->suspend ();
	}
}

CHANNEL_INSTANCE_INDEX Script::find_channel(Target & target) const
{
	CHANNEL_INSTANCE_INDEX result = INVALID_CHANNEL_INSTANCE_INDEX;

	CHANNEL_VECTOR::const_iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	while (begin < end)
	{
		Target ct = const_cast<Channel::Object*>(*begin)->get_target();

		if (ct == target)
		{
			result = (CHANNEL_INSTANCE_INDEX)(*begin);
			break;
		}

		begin++;
	}

	return result;
}

void Script::set_current_time(float t)
{
	CHANNEL_VECTOR::iterator chan = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	while (chan < end)
	{
		(*chan)->set_current_time(t);
		chan++;
	}
}

float Script::get_current_time (void) const
{
	ASSERT (longest);
	return longest->get_current_time ();
}

void Script::free (void)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	for (; begin < end; ++begin)
		delete (*begin);

	channels.clear ();

	longest = NULL;
}

void Script::change_position(const Vector & p)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	for ( ; begin < end; begin++)
	{
		(*begin)->change_position(p);
	}
}


void Script::change_orientation(const Matrix & orient)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	for ( ; begin < end; begin++)
	{
		(*begin)->change_orientation(orient);
	}
}

void Script::adjust_start_position(const Vector & dp)
{
	CHANNEL_VECTOR::iterator begin = channels.begin();
	const CHANNEL_VECTOR::const_iterator end = channels.end();

	for ( ; begin < end; begin++)
	{
		(*begin)->adjust_start_position(dp);
	}
}


U32 ScriptSet::AddRef (void)
{
	return ++ref_count;
}

U32 ScriptSet::Release (void)
{
	U32 ret = --ref_count;

	if (!ret)
		delete this;

	return ret;
}

GENRESULT ScriptSet::QueryInterface (const char*, void**)
{
	return GR_GENERIC;
}
