//--------------------------------------------------------------------------//
//                                                                          //
//                             ThornInterface.h								//
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/
#ifndef THORNINTERFACE_H
#define THORNINTERFACE_H

//
// This file describes the interfaces used by the playback engine to control scripted app objects and
//	the interface the app will use to communicate with the playback engine. The	app can implement one 
//	or more control interfaces for each entity and enable script control by	binding the interfaces with 
//	the playback engine for each entity. The app can disable script control for any entity by releasing
//	the entity.
//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "filesys.h"

#include <vector.h>
#include <xform.h>
#include <quat.h>

struct ThornEntity;
struct ThornEvent;
struct ThornIKEvent;
struct ThornMotionEvent;
struct ThornSoundEvent;
struct ScriptFileReader;
struct ScriptFileWriter;

typedef enum ThornEntityType	
{
	K_ENTITY_UNKNOWN = 0,
	K_ENTITY_COMPOUND,
	K_ENTITY_DEFORMABLE,
	K_ENTITY_CAMERA,
	K_ENTITY_MONITOR,
	K_ENTITY_LIGHT,
	K_ENTITY_SOUND,
	K_ENTITY_MARKER,
	K_ENTITY_HARDPOINT,
	K_ENTITY_SCENE,
	K_ENTITY_MOTION_PATH,
	K_ENTITY_DELETED,
	K_ENTITY_USER_ENTITY,
	K_LAST_ENTITY
};

const U32 K_FIRST_ENTITY = K_ENTITY_COMPOUND;
const U32 K_ENTITY_NUM_ENTITY_TYPES = K_LAST_ENTITY - K_FIRST_ENTITY;

//K_EVENT_START_PROPERTY_ANIM,			// linearly animate an entity's properties
typedef enum ThornEventType					// The order of these events (i.e. their value) determines the order in which they are updated
{											// Events which rely on other entities are at the end so that any events which might modify
	K_EVENT_UNDEFINED = 0,					// those entities will have already been updated
	K_EVENT_USER_EVENT,	
	K_EVENT_SET_MONITOR,					// sets the monitor to a camera
	K_EVENT_START_MOTION,					// start an animation
	K_EVENT_START_SOUND,					// start a sound
	K_EVENT_START_LIGHT_PROPERTY_ANIM,		// linearly animate an entity's light properties
	K_EVENT_START_CAMERA_PROPERTY_ANIM,		// linearly animate an entity's camera properties
	K_EVENT_START_SPATIAL_PROPERTY_ANIM,	// linearly animate an entity's spatial properties
	K_EVENT_START_IK,						// start ik on an entity
	K_EVENT_ATTACH_ENTITY,					// attach this entity to another entity to modify position, orientation and/or lookat
	K_EVENT_START_PATH_MOTION,				// start moving an entity along a path to modify position and/or orientation 
	K_LAST_EVENT
};
const U32 K_FIRST_EVENT = K_EVENT_UNDEFINED + 1;
const U32 K_EVENT_NUM_EVENT_TYPES = K_LAST_EVENT - K_FIRST_EVENT;

typedef enum ThornState
{
	K_THORN_INVALID = 0,					// instance created, but not initialized
	K_THORN_DELETED,						// script/entity has been deleted
	K_THORN_READY,							// script/entity ready
	K_THORN_STOPPED,						// script has been stopped
	K_THORN_COMPLETE,						// script has completed and is idle
	K_THORN_PAUSED,							// script has been paused and can be resumed by starting or restarted by stopping and then starting
	K_THORN_PLAYING,						// script is playing
	K_THORN_LOOPING							// script is playing and looping
};

// thorn flags constants
const U32	THORN_ENTITY_REFERENCE =		0x01;		// ENTITY FLAG: this entity should have been created by another script

			// sound entity and event flags
			// sound flags that can be set independently from the sound ENTITY FLAGs and interpreted however the app wants
			// a sound entity could be created as a 3d sound but give the app a hint (or an order) to play a specific instance
			// as a non 3D sound.
const U32	THORN_3D_SOUND =				0x02;		// ENTITY OR EVENT: this is a 3d sound
const U32	THORN_STREAM_SOUND =			0x04;		// ENTITY OR EVENT: this sound should be streamed

const U32	THORN_USE_SCRIPT_DURATION =		0x01;		// EVENT FLAG: this event's duration is always = the duration of the script

			// motion path event/attach event flags
const U32	THORN_UPDATE_POSITION =			0x02;		// EVENT FLAG: use the path/entity to determine position
const U32	THORN_UPDATE_ORIENTATION =		0x04;		// EVENT FLAG: use the path/entity to determine orientation
const U32	THORN_LOOK_AT_ENTITY =			0x08;		// EVENT FLAG: use the path/entity to determine orientation by looking at entity
	
const U32	THORN_QUATERNION_ORIENTATION =	0x10;		// EVENT FLAG: indicates quaternion orientation was read and should be written

// Interface used by THORN to control a bound entity.
#define IENTITYCONTROL_VERSION 1
#define IID_ICompoundControl MAKE_IID("ICompoundControl", 1)
struct ICompoundControl: IDAComponent
{
	virtual GENRESULT COMAPI start_motion(const ThornMotionEvent &event) = 0;	// start a compound motion
	virtual GENRESULT COMAPI stop_motion(const ThornMotionEvent &event) = 0;	// stop a compound motion
	virtual GENRESULT COMAPI update (U32 deltaTime) = 0;						// update a compound - only called once for every delta time step (which may be more that once per update())
};

#define IENTITYCONTROL_VERSION 1
#define IID_ISoundControl MAKE_IID("ISoundControl", 1)
struct ISoundControl: IDAComponent
{
	virtual GENRESULT COMAPI start_sound(const ThornSoundEvent &event) = 0;			// start a sound
	virtual GENRESULT COMAPI stop_sound(const ThornSoundEvent &event) = 0;			// stop a sound
};

// Interface used by THORN to control IK on a bound entity.
// the ik_pos and ik_orient is calculated by thorn and passed to the app every update_ik()
#define IIKCONTROL_VERSION 1
#define IID_IIKControl MAKE_IID("IIKControl", 1)
struct IIKControl: IDAComponent
{
	virtual GENRESULT COMAPI start_ik(const ThornIKEvent &event, const Vector & ik_pos, const Matrix & ik_orient) =0;
	virtual GENRESULT COMAPI stop_ik(const ThornIKEvent &event) = 0;
	virtual GENRESULT COMAPI update_ik(const ThornIKEvent &event, const Vector & ik_pos, const Matrix & ik_orient) = 0;
	// position of the end effector is returned in pos
	virtual GENRESULT COMAPI get_end_effector_position(const ThornIKEvent &event, const char * end_effector, Vector &pos) = 0;
};

// Interface used by THORN to control the position and orientation of objects directly.
#define IGEOTRANSFORM_VERSION 1
#define IID_IGeoTransformControl MAKE_IID("IGeoTransformControl", 1)
struct IGeoTransformControl: IDAComponent
{
	virtual GENRESULT COMAPI set_position(const Vector &position) = 0;
	virtual GENRESULT COMAPI set_orientation(const Matrix &orientation) = 0;
	virtual GENRESULT COMAPI get_position(Vector &position) = 0;
	virtual GENRESULT COMAPI get_orientation(Matrix &orientation) = 0;
};

// Interface used by THORN to control the properties of cameras directly. 
#define ICAMERACONTROL_VERSION 1
#define IID_ICameraControl MAKE_IID("ICameraControl", 1)
struct ICameraControl: IDAComponent
{
	virtual GENRESULT COMAPI set_h_fov(SINGLE fov) = 0;
	virtual GENRESULT COMAPI set_v_fov(SINGLE fov) = 0;
	virtual GENRESULT COMAPI set_v_to_h_aspect_ratio(SINGLE aspectRatio) = 0;
	virtual GENRESULT COMAPI set_near_plane_distance(SINGLE nearDist) = 0;
	virtual GENRESULT COMAPI set_far_plane_distance(SINGLE farDist) = 0;
	virtual GENRESULT COMAPI get_h_fov(SINGLE &fov) = 0;
	virtual GENRESULT COMAPI get_v_fov(SINGLE &fov) = 0;
	virtual GENRESULT COMAPI get_v_to_h_aspect_ratio(SINGLE &aspectRatio) = 0;
	virtual GENRESULT COMAPI get_near_plane_distance(SINGLE &nearDist) = 0;
	virtual GENRESULT COMAPI get_far_plane_distance(SINGLE &farDist) = 0;
};

#include "ILight.h"

// Interface used by THORN to control the properties of lights directly. 
#define ILIGHTCONTROL_VERSION 1
#define IID_ILightControl MAKE_IID("ILightControl", 1)
struct ILightControl: IDAComponent
{
	virtual GENRESULT COMAPI set_color (const LightRGB & color) = 0;
	virtual GENRESULT COMAPI set_direction (const Vector & direction) = 0;
	virtual GENRESULT COMAPI set_range (SINGLE distance) = 0;
	virtual GENRESULT COMAPI set_cutoff (SINGLE angle) = 0;
	virtual GENRESULT COMAPI set_on (SINGLE on) = 0;
	virtual GENRESULT COMAPI get_color (LightRGB & color) = 0;
	virtual GENRESULT COMAPI get_direction (Vector & direction) = 0;
	virtual GENRESULT COMAPI get_range (SINGLE &distance) = 0;
	virtual GENRESULT COMAPI get_cutoff (SINGLE &angle) = 0;
	virtual GENRESULT COMAPI get_on (SINGLE &on) = 0;
};

#define ISCRIPT_VERSION 1
#define IID_IScriptEngine MAKE_IID("IScriptEngine", 1)
struct IScriptEngine : public IComponentFactory
{
	// load/save the file creating a filesys from a file name
	virtual GENRESULT COMAPI load_script(const char *string) = 0;
	virtual GENRESULT COMAPI save_script(const char *string) = 0;

	// read/write from/to a passed in IFileSys
	virtual GENRESULT COMAPI read_script_file(IFileSystem *file) = 0;
	virtual GENRESULT COMAPI write_script_file(IFileSystem *file) = 0;

	// entity releated methods
	virtual U32 COMAPI get_entity_count() = 0;												// how many entities thorn read from the script
	// get_entity_data... methods return a pointer which may be cast to the appropriate entity type if the the entity_type is correct
	virtual const ThornEntity * COMAPI get_entity_data_by_index(U32 index) = 0;				// get entity data by INDEX
	virtual const ThornEntity * COMAPI get_entity_data(S32 entity_id) = 0;					// get entity data by ENTITY_ID
	// bind/release entity interfaces
	virtual GENRESULT COMAPI bind_entity(U32 index, IDAComponent * entityInterface) = 0;	// bind the interfaces that the app wants thorn to control
	virtual GENRESULT COMAPI release_entity(U32 index) = 0;									// release all bound interfaces (thorn will no longer control the entity)
	
	// these methods set/get user data that can be read from the script and/or added at runtime by the app
	// floats properties
	virtual U32 COMAPI get_entity_user_value_count(U32 entityIndex) = 0;											// get the number of user values
	virtual const char * COMAPI get_entity_user_value_name(U32 entityIndex, U32 propertyIndex) = 0;							// get the name of the Nth user value
	virtual SINGLE COMAPI get_entity_user_value(U32 entityIndex, const char * propertyName) = 0;					// get a named float value for this entity
	virtual GENRESULT COMAPI set_entity_user_value(U32 entityIndex, const char * propertyName, SINGLE value) = 0;	// set a named float value for this entity
	// string properties
	virtual U32 COMAPI get_entity_user_string_count(U32 entityIndex) = 0;											// get the number of user strings
	virtual const char * COMAPI get_entity_user_string_name(U32 entityIndex, U32 propertyIndex) = 0;					// get the name of the Nth user string
	virtual const char * COMAPI get_entity_user_string(U32 index, const char * propertyName) = 0;					// get a named string for this entity
	virtual GENRESULT COMAPI set_entity_user_string(U32 index, const char * propertyName, const char * string) = 0;	//set a named string for this entity

	// entity editing methods
	//	methods to manipulate the script entities
	virtual GENRESULT COMAPI set_entity_data(U32 entity_id, ThornEntity &event) = 0;			// set entity specific data
	virtual GENRESULT COMAPI get_entity_initial_data(S32 entity_id, ThornEntity &event) = 0;	// set the entity's current data
	virtual GENRESULT COMAPI set_entity_initial_data(S32 entity_id, ThornEntity &event) = 0;	// set the entity's initial data
	//	add/remove thorn entities
	//	existing events cannot reference added events, new events can reference all existing (loaded + added) entities
	virtual GENRESULT COMAPI add_entity(ThornEntity &entity, S32 &entity_id) = 0;				// add an entity to thorn
	//	removing an entity will remove all existing events that reference the removed entity
	virtual GENRESULT COMAPI remove_entity(S32 entity_id) = 0;									// remove an entity

	// general script control methods
	virtual U32 COMAPI get_duration () = 0;														// get the script duration
	// all updating will stop when the duration is reached, no pending events will happen
	virtual GENRESULT COMAPI set_duration (U32 newDuration) = 0;								// set the duration
	virtual U32 COMAPI get_current_time () = 0;													// get the script's current time
	virtual GENRESULT set_looping(bool state) = 0;												// loop the entire script
	virtual ThornState COMAPI get_current_state() = 0;											// get the state of the script
	virtual GENRESULT COMAPI start(U32 startTime = 0) = 0;										// start playing the script with offset startTime
	virtual GENRESULT COMAPI stop() = 0;														// stop the script (doesn't reset the script)
	virtual GENRESULT COMAPI pause() = 0;														// pause the script (resume will continue at the point pause was called)
	virtual GENRESULT COMAPI resume(U32 resumeTime) = 0;										// resume playback
	virtual GENRESULT COMAPI update(S32 deltaTime) = 0;											// update the script to time (get_current_time() + deltaTime)

	// methods to control (extend) reading/writing of thorn files
	virtual GENRESULT COMAPI set_script_reader(ScriptFileReader * parser) = 0;					// provide a new object thorn will use to read a script
	virtual GENRESULT COMAPI set_script_writer(ScriptFileWriter * writer) = 0;					// provide a new object thorn will use to write a script
		
	// methods to manipulate the script events
	virtual U32 COMAPI get_event_count() = 0;													// get the event cound
	virtual const ThornEvent * COMAPI get_event_data_by_index(U32 index) = 0;					// get event data by INDEX
	virtual const ThornEvent * COMAPI get_event_data(S32 event_id) = 0;							// get event data by EVENT_ID
	virtual GENRESULT COMAPI add_event(ThornEvent &event, S32 &event_id) = 0;					// add a new event
	virtual GENRESULT COMAPI remove_event(S32 event_id) = 0;									// remove an event
};

//
// structure definitions
//

// structure passed to app from thorn containing entity data
// structure passed to thorn from app when creating a new entity

const S32 K_ENTITY_INVALID_INDEX = -1;
struct ThornEntity
{
	ThornEntityType		m_entityType;		// type
	U32					m_thornFlags;		// thorn specific flags, ENTITY FLAG:s are part of the event data
	Vector				m_position;			// current position
	Matrix				m_orientation;		// current orientation
	const char *		m_label;			// name
	const char *		m_templateName;		// name of the database template used for this entity
	U32					m_templateID;		// database template ID
};


// used to create an entity and for initial properties
struct ThornCameraProperties
{
	float h_fov;							// horizontal FOV
	float near_plane;						// near plane distance.
	float far_plane;						// far plane distance.
	float hvaspect;							// near_plane_w / near_plane_h
};

struct ThornCameraEntity : public ThornEntity
{
	ThornCameraProperties props;
};

struct MotionPathProps
{
	U32 num_points;
	Vector * point_list;
};

struct ThornMotionPathEntity : public ThornEntity
{
	MotionPathProps props;
};

struct ThornPropertyAnimEntity : public ThornEntity
{
	MotionPathProps props;
};

// used to create an entity and for initial properties
struct ThornLightProperties
{
	LightRGB color;
	Vector direction;
	SINGLE range;
	SINGLE cutoff;
	BOOL32 on;
};

struct ThornLightEntity : public ThornEntity
{
	ThornLightProperties props;
};

struct SceneEntityProperties
{
	LightRGB ambient;
	U32 duration;
	U32 start_time;
};

struct ThornSceneEntity : public ThornEntity
{
	SceneEntityProperties props;
};

const S32 K_EVENT_INVALID_INDEX = -1;
struct ThornEvent
	{
	ThornEventType		event_type;			// type of event
	U32					event_time;			// starting time
	U32					thorn_flags;		// thorn specific flags
	U32					event_flags;		// event_type specific flags
	U32					user_info;			// this is persisted over the life of the event and can be set and retrieved by the user
	S32					event_id;			// unique id of an event instance
	S32					duration;			// duration
	S32					entity_index1;		// entity index (unique id) that the app can use to refernce the entity
	S32					entity_index2;		// entity index (unique id) that the app can use to refernce the entity
};

// for ik and lookat
enum Axis
{
	kXAxis = 0,
	kYAxis,
	kZAxis,
	kNXAxis,
	kNYAxis,
	kNZAxis
};

struct ThornIKEvent : public ThornEvent
{
	U32					ik_id;				// id used to identify a specific ik event (so that start_ik, update_ik, and stop_ik can determine which ik is being referenced)
	U32					count_to_root;		// how far up the chain to modify w/ the ik_event
	Axis				up;					// which Axis is up
	Axis				front;				// which Axis is front
	SINGLE				damping_factor;		// damping factor
	bool				point_at;			// should this ik motion affect the end_effector by looking at the target OR
	bool				move_to;			// by moving to the target
	SINGLE				transition_duration;// transition duration
	const char *		end_effector;		// end effector of this ik chain
};

struct ThornMotionEvent : public ThornEvent
{
	float				start_time;			// ms starting offset into this motion
	float				transition_duration;// transition duration
	float				time_scale;			// time scale 
	float				weight;				// 
	const char *		script_name;		// name of motion to start
	// Deformable Specific data
	float				heading;			//
	// Compound Specific data	
	bool				overlay;			//
	float				translation_scale;	//
	const				Transform * start_xform;
};

struct ThornSoundEvent : public ThornEvent
{
	float start_offset_ms;				// ms offset to start sample with
};

struct ThornAttachEntityEvent : public ThornEvent
{
	Vector offset;							// position offset from the target for the attached entity
};

struct ThornSetMonitorEvent : public ThornEvent
{
};

struct ThornPropertyAnimEvent : public ThornEvent
{
	U32 property_mask;			// masks indicates the properties that will be animated
};

// property animation event affecting light properties
// light property_mask bit positions correspond to these indices
enum LightPropertyIndex
{
	K_LIGHT_COLOR = 0,
	K_LIGHT_DIRECTION,
	K_LIGHT_RANGE,
	K_LIGHT_CUTOFF,
	K_LIGHT_ON,
	K_LIGHT_NUM_PROPERTIES
};
struct ThornLightPropertyAnimEvent : public ThornPropertyAnimEvent
{
	ThornLightProperties target_props;
};

// camera property_mask bit positions correspond to these indices
enum CameraPropertyIndex
{
	K_CAMERA_HFOV = 0,
	K_CAMERA_NEAR,
	K_CAMERA_FAR,
	K_CAMERA_ASPECT,
	K_CAMERA_NUM_PROPERTIES
};
struct ThornCameraPropertyAnimEvent : public ThornPropertyAnimEvent
{
	ThornCameraProperties target_props;
};

// spatial property_mask bit positions correspond to these indices
enum SpatialPropertyIndex
{
	K_SPATIAL_POSITION = 0,
	K_SPATIAL_ORIENTATION,
	K_SPATIAL_NUM_PROPERTIES
};

struct ThornSpatialProperties
{
	Transform xform;			// position and orientation in std form
	Quaternion orientation;		// orientation in quaternions 
};

// Spatial property animation event
// SET this events thorn_flags to include THORN_QUATERNION_ORIENTATION if you want ThornSpatialProperties.orientation value to be used
struct ThornSpatialPropertyAnimEvent : public ThornPropertyAnimEvent
{
	ThornSpatialProperties target_props;
};

// used to create a thorn event
// none of these structs are passed to the app during normal playback
struct AccelerationInfo
{
	U32 num_points;
	SINGLE * time;
	SINGLE * acceleration;
};

struct ThornPathAnimEvent : public ThornEvent
{
	SINGLE start_percent;					// starting path percent
	SINGLE stop_percent;					// stop path percent
	SINGLE length;							// percent length of animation (stop_percent - start_percent)
	SINGLE velocity;						// initial velocity (and constant velocity if no acceleration data is provided)
	Vector offset;							// position offset from the path for the connected entity
	AccelerationInfo acceleration_info;		// acceleration description
};

#endif //THORNINTERFACE_H
