//$Header: /Conquest/Libs/Include/IAnim.h 5     4/28/00 11:57p Rmarr $

#ifndef __IANIM_H
#define __IANIM_H

typedef int SCRIPT_SET_ARCH;
typedef int SCRIPT_INST;

const SCRIPT_SET_ARCH INVALID_SCRIPT_SET_ARCH = -1;
const SCRIPT_INST INVALID_SCRIPT_INST = -1;

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef __ANIMTYPES_H
#include "AnimTypes.h"
#endif

#include "ichannel.h"

typedef S32 INSTANCE_INDEX;		// also defined in Engine.h
struct IFileSystem;

#include "3DMath.h"

//

typedef void (__cdecl * SCRIPT_ENUM_CALLBACK)(const char* script_name, void* misc);
typedef void (__cdecl * CHANNEL_ENUM_CALLBACK) (IChannel2* obj, void* user);
typedef bool (__cdecl * CHANNEL_INSTANCE_ENUM_CALLBACK) (CHANNEL_INSTANCE_INDEX obj, void* user);

#define IID_IAnimation MAKE_IID("IAnimation", 1)

struct DACOM_NO_VTABLE IAnimation : public IDAComponent
{
	virtual SCRIPT_SET_ARCH COMAPI create_script_set_arch (IFileSystem* fs) = 0;
	virtual void COMAPI release_script_set_arch (SCRIPT_SET_ARCH arch) = 0;

	virtual float COMAPI get_duration (SCRIPT_SET_ARCH, const char* arch_name) const = 0;

	virtual void COMAPI enumerate_scripts (SCRIPT_ENUM_CALLBACK, SCRIPT_SET_ARCH, void* misc=0) const = 0;

	//you must call Release () on all IChannel2 ptrs you receive.
	virtual GENRESULT COMAPI enumerate_channels (CHANNEL_ENUM_CALLBACK, SCRIPT_SET_ARCH, const char* name, void* user) const = 0;

	virtual SCRIPT_INST COMAPI create_script_inst (SCRIPT_SET_ARCH arch,
													INSTANCE_INDEX root, 
													const char* name,
													Animation::IEventHandler * call_back = 0,
													void* user_data = 0) = 0;

	virtual void COMAPI release_script_inst (SCRIPT_INST inst) = 0;

	//EMAURER returns 0 on success, 1 if data incorrect for time submitted (such
	//as playing a script backwards that contains channels of variable length)
	//ANIMATION STILL STARTS, -1 on some other failure. 

	virtual int COMAPI script_start (SCRIPT_INST inst, 
										unsigned int flags=Animation::FORWARD, 
										float time=Animation::CUR,
										float time_scale=1.0f,
										float transition_duration=0.0f,
										float weight=1.0f,
										float translation_scale=1.0f,
										const Transform * start_xform = 0) = 0;

	virtual void COMAPI script_stop (SCRIPT_INST inst) = 0;

	virtual void COMAPI set_current_time(SCRIPT_INST, float time) = 0;
	virtual float COMAPI get_current_time (SCRIPT_INST) const = 0;

// WARNING: script_is_done() is not necessarily the best way to check for the end of
// simple scripts. It takes into account transitions and other complicated situations in
// which a script's duration may have been exceeded, but the channels are still in use
// for some reason. 
// For simpler cases, particularly those where you're not destroying the SCRIPT_INST yet,
// it's much cheaper to just check the start time vs. the script's duration.
	virtual BOOL32 COMAPI script_is_done(SCRIPT_INST) const = 0;

	virtual void COMAPI change_script_position(SCRIPT_INST inst, const Vector & pos) = 0;
	virtual void COMAPI change_script_orientation(SCRIPT_INST inst, const Matrix & orient) = 0;

	virtual BOOL32 COMAPI script_contains_channel(SCRIPT_INST script_inst, CHANNEL_INSTANCE_INDEX channel_inst) const = 0;

	virtual BOOL32 COMAPI get_root_data(SCRIPT_INST script_inst, Vector & absolute, Vector & offset) const = 0;
	virtual void COMAPI adjust_start_position(SCRIPT_INST script_inst, const Vector & dpos) = 0;

	virtual float COMAPI get_duration(SCRIPT_INST inst) const = 0;

	virtual BOOL32 COMAPI get_instance_name(SCRIPT_INST inst, const char *& name) const = 0;

	virtual BOOL32 COMAPI get_script_start_height(SCRIPT_SET_ARCH arch, const char * name, float & start_height) const = 0;
	virtual BOOL32 COMAPI get_script_start_height(SCRIPT_INST inst, float & start_height) const = 0;

// get the channel index controlling a given joint.
	virtual CHANNEL_INSTANCE_INDEX COMAPI find_channel(SCRIPT_INST inst, Animation::Target & target) = 0;

	virtual void COMAPI update_instance (INSTANCE_INDEX root, SINGLE dt) = 0;
	virtual void COMAPI update (SINGLE dt) = 0; 
};


// This interface allows the loading of a subset of animations from an animation file, only if the file is in the
// "new" format.
// This interface is queried off of an object implementing IAnimation, so it shares the same archetype idenfication
// namespace. To use it, query it off of your IAnimation interface. If the query succeeds, you can use it, otherwise
// you cannot.

#define IID_IAnimation2 MAKE_IID("IAnimation2", 1)

struct DACOM_NO_VTABLE IAnimation2 : public IDAComponent
{
	// Creates a script set archive without any animations. Returns INVALID_SCRIPT_SET_ARCH on failure.
	virtual SCRIPT_SET_ARCH COMAPI create_empty_script_set_arch () = 0;

	// Load and remove scripts from a script set archetype. Loading will only work on "new" style animation files,
	// where the channel information is stored under the directory for each animation and not in a seperate
	// channels directory. Both return GR_OK on success.
	// NOTE: If there are name collisions between new animations and existing animations, the old animation will
	// not be replaced, a notice will be issued, and the function will return GR_OK. Requests to remove animations
	// that don't exist will also generate a notice but return GR_OK.
	virtual GENRESULT COMAPI load_scripts (SCRIPT_SET_ARCH arch, IFileSystem *fs, const char *nameList[], int nameCount) = 0;
	virtual GENRESULT COMAPI remove_scripts (SCRIPT_SET_ARCH arch, const char *nameList[], int nameCount) = 0;
	
	// This method will make the callback for each animation in the given filesystem, as long as the file system is
	// of the new format. Returns GR_OK on success.
	virtual GENRESULT COMAPI enumerate_scripts_in_file (SCRIPT_ENUM_CALLBACK, IFileSystem* fs, void* misc=0) const = 0;
	
	// This method returns GR_OK if the given script set archetype contains an animation whose name matches the given
	// name.
	virtual GENRESULT COMAPI verify_loaded (SCRIPT_SET_ARCH arch, const char *name) = 0;
	
	// This method returns the count of scripts in the given script set archetype. Returns GR_OK if the archetype
	// is valid, GR_GENERIC otherwise.
	virtual GENRESULT COMAPI script_count (SCRIPT_SET_ARCH arch, unsigned int &count) = 0;
};

// IAnimation3

#define IID_IAnimation3 MAKE_IID("IAnimation3", 1)

struct DACOM_NO_VTABLE IAnimation3 : public IDAComponent
{
	//EMAURER as long as the callback returns true channels are enumerated.
	virtual GENRESULT COMAPI enumerate_channels2 (CHANNEL_INSTANCE_ENUM_CALLBACK, SCRIPT_INST, void* misc=0) const = 0;
};

#endif