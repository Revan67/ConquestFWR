//--------------------------------------------------------------------------//
//                                                                          //
//                                Sound.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/EngComps/Sound/Sound.cpp 4     8/14/98 1:41a Mikes $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#include "engcomp.h"
#include "engine.h"
#include "model.h"
#include "tcomponent.h"
#include "SysConsumerDesc.h"
#include "Sound.h"
#include "audiomgr.h"
#include "stddat.h"
#include "vector.h"
#include "heapobj.h"
#include "filesys.h"
#include "ICamera.h"
#include "TSmartPointer.h"

// DEBUG
#include "display.h"
//

IAudioManager * audioManager;
//

struct DACOM_NO_VTABLE Sound : public IEngineComponent, public ISound
{
	BEGIN_DACOM_MAP_INBOUND(Sound)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY(ISound)
	DACOM_INTERFACE_ENTRY2(IID_ISound,ISound)
	END_DACOM_MAP()

//
// Implementation-specific stuff.
//
	class SArchetype
	{
	public:
		C8			filename[_MAX_PATH];
		SINGLE	volume;
		DWORD		loops;
		SINGLE	falloff;

		SArchetype() : volume(0.0f), loops(1), falloff(1000.0f)
		{
			filename[0] = '\0';
		}

		void set_filename(const C8* value)
		{
			strcpy(filename, value ? value : "");
			audioManager->precache(filename, SOUND_STEREO_ONLY);
		}

		void set_volume(SINGLE value)
		{
			volume = value;
		}

		void set_loops(DWORD value)
		{
			loops = value;
		}

		void set_falloff(SINGLE value)
		{
			falloff = value;
		}
	};

	class SInstance
	{
	public:
		INSTANCE_INDEX archIndex;
		INSTANCE_INDEX objectIndex;
		int				listIndex;
		SOUND_ID			soundId;
		SINGLE			volume;
		DWORD				loops;

		SInstance(INSTANCE_INDEX archIndexIn,
			       int				 listIndexIn) :
			archIndex(archIndexIn),
			objectIndex(INVALID_INSTANCE_INDEX),
			listIndex(listIndex),
			soundId(0),
			volume(0.0f),
			loops(1)
		{}
	};

	DynamicArray<TPointer <SArchetype> >			archetypes;
	mutable DynamicArray<TPointer <SInstance> >	instances;

	DynamicArray<INSTANCE_INDEX> instanceList;
	unsigned int					  instanceCount;

	IEngine*	engine;
	IModel* model;

	SINGLE masterVolume;

	Vector earPosition;

	Sound() :
		instances(64),
		archetypes(64),
		instanceList(64),
		instanceCount(0),
		engine(NULL)
	{
	}

	~Sound();

//
// IEngineComponent methods
//

	GENRESULT init(SYSCONSUMERDESC* info);

	virtual GENRESULT	COMAPI Initialize(void);
	virtual BOOL32		COMAPI create_archetype(ARCHETYPE_INDEX idx, IFileSystem* parent = NULL);
	virtual BOOL32		COMAPI set_archetype_properties (ARCHETYPE_INDEX idx, const PROPERTY *properties);
	virtual void		COMAPI destroy_archetype(ARCHETYPE_INDEX index);

	virtual BOOL32		COMAPI create_instance(INSTANCE_INDEX inst_idx, ARCHETYPE_INDEX arch_idx);
	virtual void		COMAPI destroy_instance(INSTANCE_INDEX index);

   virtual BOOL32		COMAPI create_instance(INSTANCE_INDEX  instance,
                                            const C8*			type_name);

	virtual void		COMAPI set_instance_property
                                   (INSTANCE_INDEX index,
								  	         const C8*		name,
                                    DACOM_VARIANT	value,
									bool recurse);

	virtual void COMAPI duplicate_archetype(ARCHETYPE_INDEX new_arch, ARCHETYPE_INDEX old_arch);

//
// This component ignores all accessory-related functions.
//

   virtual void COMAPI update         (SINGLE dt);
   virtual void COMAPI update_instance (INSTANCE_INDEX idx, SINGLE dt);
   virtual vis_state COMAPI render_instance(struct ICamera * camera, INSTANCE_INDEX instance, U32 flags, const Transform *tr);
   virtual vis_state COMAPI render_lod_instance(struct ICamera * camera, INSTANCE_INDEX instance, float lod_fraction, U32 flags, const Transform *tr);

//
// ISound methods.
//
	virtual void COMAPI set_ear_position(const Vector&);

	virtual SINGLE COMAPI get_master_volume() const;
	virtual void	COMAPI set_master_volume(SINGLE volume);

	virtual SINGLE COMAPI get_volume(INSTANCE_INDEX object) const;
	virtual void	COMAPI set_volume(INSTANCE_INDEX object, SINGLE volume);


	virtual void COMAPI play		(INSTANCE_INDEX object);
	virtual void COMAPI stop		(INSTANCE_INDEX object);
	virtual void COMAPI resume	(INSTANCE_INDEX object);

	static bool LoadFile (const char *fileName, void * buffer, U32 size, IFileSystem * parent);

	IDAComponent* get_base(void)
	{
		return (IEngineComponent*)this;
	}
};



HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

C8 *interface_name = "ISound";  // Interface name used for registration     

void SetDllHeapMsg (void)
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
	IComponentFactory *server;

	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			hInstance = hinstDLL;

			HEAP = HEAP_Acquire();
			SetDllHeapMsg();

			server = new DAComponentFactory2<DAComponentAggregate<Sound>, SYSCONSUMERDESC> (interface_name);

			if (server == NULL)
			{
				break;
			}

			DACOM = DACOM_Acquire();

		//
		// Register at object-renderer priority
		//
			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
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
// Sound class member functions
//

GENRESULT Sound::init(SYSCONSUMERDESC * info)
{
	return info->system->QueryInterface("IAudioManager", (void **)&audioManager);
}

Sound::~Sound()
{
	unsigned int i;

	for (i = 0; i < instances.num_entries(); i++)
	{
		destroy_instance(i);
	}

	for (i = 0; i < archetypes.num_entries(); i++)
	{
		destroy_archetype(i);
	}

	if (audioManager)
	{
		audioManager->Release();
		audioManager = 0;
	}
}

//

GENRESULT COMAPI Sound::Initialize(void)
{
	if (get_base()->QueryInterface("IEngine", (void **) &engine) == GR_OK)
	{
		get_base()->Release();
	}

	if (get_base()->QueryInterface("IModel", (void **) &model) == GR_OK)
	{
		get_base()->Release();
	}

	return GR_OK;
}

//

BOOL32 COMAPI Sound::create_archetype(ARCHETYPE_INDEX archIndex,
												  IFileSystem*		parent)
{
	if (archIndex == INVALID_ARCHETYPE_INDEX) return FALSE;

	if (archetypes[archIndex])
	{
		delete archetypes[archIndex];
		archetypes[archIndex] = NULL;
	}

	C8 parentName[_MAX_PATH];

	parent->GetFileName(parentName, sizeof parentName);

	C8 fileName[_MAX_PATH];
	memset(fileName, 0, sizeof fileName);

	if (LoadFile("FileName", fileName, sizeof(fileName), parent))
	{
		if (!archetypes[archIndex])
			archetypes[archIndex] = new SArchetype;

		C8 tempName[_MAX_PATH];
		strcpy(tempName, parentName);
		strcat(tempName, "\\");
		strcat(tempName, fileName);

		archetypes[archIndex]->set_filename(tempName);
	}

	C8 text[12];

	if (LoadFile("Loops", text, sizeof(text), parent))
	{
		if (!archetypes[archIndex])
			archetypes[archIndex] = new SArchetype;

		DWORD loops;

		loops = atoi(text);

		archetypes[archIndex]->set_loops(loops);
	}

	if (LoadFile("FallOff", text, sizeof(text), parent))
	{
		if (!archetypes[archIndex])
			archetypes[archIndex] = new SArchetype;

		SINGLE falloff;

		falloff = (SINGLE)atof(text);

		archetypes[archIndex]->set_falloff(falloff);
	}
	return (archetypes[archIndex] != NULL);
}

//

BOOL32 COMAPI Sound::set_archetype_properties (ARCHETYPE_INDEX archIndex,
															  const PROPERTY*	properties)
{
	if (archIndex == INVALID_ARCHETYPE_INDEX)
		return FALSE;

	BOOL32 result = FALSE;

	SArchetype* archetype = archetypes[archIndex];

	const PROPERTY *p = properties;

	while (p->name != NULL)
	{
		if (!strcmp(p->name, "Filename"))
		{
			if (!archetype) archetype = new SArchetype;
			archetype->set_filename((const char*)p->value);
			result = TRUE;
		}
		else if (!strcmp(p->name, "Volume"))
		{
			if (!archetype) archetype = new SArchetype;
			archetype->set_volume(p->value2);
			result = TRUE;
		}
		else if (!strcmp(p->name, "Loops"))
		{
			if (!archetype) archetype = new SArchetype;
			archetype->set_loops(p->value);
			result = TRUE;
		}
		else if (!strcmp(p->name, "Falloff"))
		{
			if (!archetype) archetype = new SArchetype;
			archetype->set_falloff(p->value2);
			result = TRUE;
		}
		++p;
	}

	archetypes[archIndex] = archetype;

	return result;
}

//

void COMAPI Sound::duplicate_archetype(ARCHETYPE_INDEX new_arch, ARCHETYPE_INDEX old_arch)
{
	SArchetype * arch = archetypes[old_arch];
	if (arch)
	{
		*archetypes[new_arch] = *arch;
	}
}

//

void COMAPI Sound::destroy_archetype(ARCHETYPE_INDEX archIndex)
{
	if (archIndex == INVALID_ARCHETYPE_INDEX) return;

	delete archetypes[archIndex];
	archetypes[archIndex] = NULL;
}

//

BOOL32 COMAPI Sound::create_instance(INSTANCE_INDEX  instIndex,
												 ARCHETYPE_INDEX archIndex)
{
	if ((instIndex == INVALID_INSTANCE_INDEX)  ||
		 (archIndex == INVALID_ARCHETYPE_INDEX) ||
		 (!archetypes[archIndex]))
	{
		return FALSE;
	}

	if (instances[instIndex]) delete instances[instIndex];

	instances[instIndex] = new SInstance(archIndex,
												    instanceCount);

	instances[instIndex]->volume = archetypes[archIndex]->volume;
	instances[instIndex]->loops = archetypes[archIndex]->loops;

	instanceList[instanceCount++] = instIndex;

	return TRUE;
}

//

BOOL32 COMAPI Sound::create_instance(INSTANCE_INDEX instIndex, const C8 *typeName)
{
	return FALSE;
}

//

void COMAPI Sound::destroy_instance(INSTANCE_INDEX instIndex)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) || (!instances[instIndex])) return;

	SInstance* instance = instances[instIndex];

	stop(instIndex);

	for (unsigned int i = 0; i < instanceCount; i++)
	{
		if (instanceList[i] == instIndex)
		{
			instanceCount--;

			for (; i < instanceCount; i++)
			{
				instanceList[i] = instanceList[i+1];
			}
			break;
		}
	}
	delete instances[instIndex];
	instances[instIndex] = NULL;
}

//

void COMAPI Sound::set_instance_property(INSTANCE_INDEX	instIndex,
													  const C8*			name,
													  DACOM_VARIANT	value, bool recurse)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) || (!instances[instIndex])) return;

	SInstance* instance = instances[instIndex];

	if (strcmp (name, "Volume") == 0)
	{
		instance->volume = value;
	}
	else if (strcmp (name, "Loops") == 0)
	{
		instance->loops = value;
	}
	else if (strcmp (name, "Object") == 0)
	{
		instance->objectIndex = value;

		Joint joint;
		joint.type = JT_FIXED;
		joint.parent = instance->objectIndex;
		joint.child = instIndex;

		model->connect(&joint);
	}
}

//

void COMAPI Sound::update(SINGLE time_step)
{
	if (!engine) return;

	SInstance* instance;
	SArchetype* archetype;

	Vector objPosition;
	Vector relativePosition;

	INSTANCE_INDEX index;

	for (unsigned int i = 0; i < instanceCount; i++)
	{
		index = instanceList[i];

		instance = instances[index];

		archetype = archetypes[instance->archIndex];

		objPosition = engine->get_position(index);

		relativePosition = objPosition - earPosition;

		audioManager->set_sound_bearing(instance->soundId,
												  relativePosition,
												  archetype->falloff);	
	}
}

void COMAPI Sound::update_instance (INSTANCE_INDEX index, SINGLE time_step)
{
	if (!engine) return;
	if ((index == INVALID_INSTANCE_INDEX) || (!instances[index])) return;

	SInstance* instance;
	SArchetype* archetype;

	Vector objPosition;
	Vector relativePosition;

	instance = instances[index];

	archetype = archetypes[instance->archIndex];

	objPosition = engine->get_position(index);

	relativePosition = objPosition - earPosition;

	audioManager->set_sound_bearing(instance->soundId,
											  relativePosition,
											  archetype->falloff);	
}

vis_state COMAPI Sound::render_instance(struct ICamera * camera,
											  INSTANCE_INDEX	instIndex,
											  U32					flags,
											  const Transform *tr)
{
	if (!engine) return (vis_state)0;

	set_ear_position(camera->get_position());

	return (vis_state)0;
}

vis_state COMAPI Sound::render_lod_instance(struct ICamera * camera,
											  INSTANCE_INDEX	instIndex,
											  float lod_fraction,
											  U32					flags,
											  const Transform * tr)
{
	return render_instance(camera, instIndex, flags, tr);
}

//

void COMAPI Sound::set_master_volume(SINGLE volume)
{
	masterVolume = volume;
	update(0.0f);
}

//

void COMAPI Sound::set_volume(INSTANCE_INDEX instIndex, SINGLE volume)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) ||
		 (!instances[instIndex]) ||
		 (!audioManager))
		return;

	instances[instIndex]->volume = volume;

	audioManager->set_volume(instances[instIndex]->soundId, volume);

}

SINGLE COMAPI Sound::get_master_volume() const 
{
	return masterVolume;
}

//

SINGLE COMAPI Sound::get_volume(INSTANCE_INDEX instIndex) const
{
	if ((instIndex == INVALID_INSTANCE_INDEX) ||
		 (!instances[instIndex]))
		return 0.0f;

	return instances[instIndex]->volume;
}

//

void COMAPI Sound::play(INSTANCE_INDEX instIndex)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) ||
		 (!instances[instIndex]) ||
		 (!audioManager))
		return;

	SInstance* instance = instances[instIndex];

	const C8* filename = archetypes[instance->archIndex]->filename;

	audioManager->precache(filename, SOUND_STEREO_ONLY);

	instance->soundId = audioManager->get_ID(filename);

	//
	// TBD: The sound bearing needs to be set here, but
	// first we need a way to determine where the ear is
	//
	audioManager->set_loop_count(instance->soundId, instance->loops);
	audioManager->play(instance->soundId);
}

//

void COMAPI Sound::stop(INSTANCE_INDEX instIndex)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) ||
		 (!instances[instIndex]) ||
		 (!audioManager))
		return;

	audioManager->stop(instances[instIndex]->soundId);
}

//

void COMAPI Sound::resume(INSTANCE_INDEX instIndex)
{
	if ((instIndex == INVALID_INSTANCE_INDEX) ||
		 (!instances[instIndex]) ||
		 (!audioManager))
		return;

	audioManager->resume(instances[instIndex]->soundId);
}

//

void COMAPI Sound::set_ear_position(const Vector& pos)
{
	earPosition = pos;
}

bool Sound::LoadFile (const char *fileName, void * buffer, U32 size, IFileSystem * parent)
{
	DAFILEDESC fdesc = fileName;
	HANDLE hFile = parent->OpenChild(&fdesc);
	bool result = false;

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwRead;

		result = (parent->ReadFile(hFile, buffer, size, &dwRead, 0) != 0);
		parent->CloseHandle(hFile);
	}

	return result;
}
