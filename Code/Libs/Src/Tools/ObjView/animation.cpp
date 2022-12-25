#ifdef ANIMATION

#include "StdDAT.h"
#include "TSmartPointer.h"
#include "IAnim.h"
#include "Engine.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "FileSys.h"

//

IAnimation* Anim = NULL;
SCRIPT_SET_ARCH Scripts = INVALID_SCRIPT_SET_ARCH;

void UpdateAnimation (float dt)
{
	if (Anim)
		Anim->update (dt);
}

struct ScriptInstance
{
	SCRIPT_INST script_index;
	unsigned int flags;
	INSTANCE_INDEX target_root;

	ScriptInstance (void) : flags (0), 
							script_index (INVALID_SCRIPT_INST), 
							target_root (INVALID_INSTANCE_INDEX)
	{
	}
};

ScriptInstance* ScriptInstances = NULL;
int ScriptInstanceCnt = 0;

void __cdecl AnimEnumCB (const char* name, void* misc)
{
	MetaList<char>* list = (MetaList<char>*)misc;

	char* dup = new char[strlen (name) + 1];
	strcpy (dup, name);

	list->append (dup);
}

void CleanupAnimation (void)
{
	if (Anim)
	{
		for (ScriptInstance* i = ScriptInstances; i < ScriptInstances + ScriptInstanceCnt; i++)
			Anim->release_script_inst (i->script_index);

		delete [] ScriptInstances;
		ScriptInstances = NULL;

		ScriptInstanceCnt = 0;

		Anim->release_script_set_arch (Scripts);
		Scripts = INVALID_SCRIPT_SET_ARCH;
	}
}

struct MyIEventHandler : public Animation::IEventHandler
{

	void COMAPI on_event(unsigned int channel_id, void* user_supplied, const EventIterator& event_iter)
	{
		for (unsigned int i = 0; i < event_iter.get_event_count (); i++)
		{
			char buf[64];
			OutputDebugString ("EventHanlder called. channel id: ");
			OutputDebugString (itoa (channel_id, buf, 10));
			OutputDebugString (" first 'int': ");
			OutputDebugString (itoa (*(int*)event_iter.get_event_data(i), buf, 10));
			OutputDebugString ("\n");
		}
	}

	void COMAPI on_finished(unsigned int channel_id, void * user_supplied) {}
	void COMAPI on_loop(unsigned int channel_id, Transform & xform, void * user_supplied) {}
};

MyIEventHandler CBTest;

void LoadAnimation (const char* filename, INSTANCE_INDEX obj)
{
	if (Anim)
	{
		COMPTR<IFileSystem> fs;

		DAFILEDESC desc (filename);

		if (DACOM_Acquire ()->CreateInstance (&desc, fs) == GR_OK)
		{
			Scripts = Anim->create_script_set_arch (fs);

			if (Scripts != INVALID_SCRIPT_SET_ARCH)
			{
				MetaList<char> names;
				Anim->enumerate_scripts (AnimEnumCB, Scripts, &names);

				if (ScriptInstanceCnt = names.count ())
				{
					ScriptInstances = new ScriptInstance[ScriptInstanceCnt];

					MetaNode<char>* node = NULL;

					ScriptInstance* cur = ScriptInstances;

					while (names.traverse (node))
					{
						cur->script_index = Anim->create_script_inst (Scripts, obj, node->object, &CBTest);
						cur->target_root = obj;

						cur++;
						delete [] node->object;
						node->object = NULL;
					}
				}
			}
		}
	}
}

extern bool ExerciseJoints;

void StartAnimation (int i, bool loop)
{
	if (Anim && i < ScriptInstanceCnt)
	{
		ExerciseJoints = false;

		unsigned int flags = Animation::FORWARD;

		float time = Animation::BEGIN;

		if (loop)
		{
			flags |= Animation::LOOP;
			time = Animation::CUR;
		}

		Anim->script_start (ScriptInstances[i].script_index, flags, time);
	}
}

void StopAnimation (int i)
{
	if (Anim && i < ScriptInstanceCnt)
	{
		ExerciseJoints = false;
		Anim->script_stop (ScriptInstances[i].script_index);
	}
}

void StartupAnimation (IEngine* engine)
{
	if (GR_OK != engine->QueryInterface (IID_IAnimation, (void**)&Anim))
		GENERAL_WARNING( "Failed to create IAnimation provider\n" );
}

void ReleaseAnimationComponent (void)
{
	DACOM_RELEASE( Anim );
}

#endif