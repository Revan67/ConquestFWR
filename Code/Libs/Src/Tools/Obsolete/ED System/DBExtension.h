// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DBExtension_h
#define DBExtension_h
// --------------------------------------------------------------------------
#include "Typedefs.h"
#include "StringType.h"
#include "StringList.h"
// --------------------------------------------------------------------------
namespace DBE
{
// --------------------------------------------------------------------------
const U32 kStringLength = 200;
// --------------------------------------------------------------------------
typedef C8	CharString[kStringLength];	// Null terminated
typedef U32 EntityType;
typedef U32	StringCount;
typedef U32 StringIndex;
// --------------------------------------------------------------------------
const EntityType	kCompound = 0;
const EntityType	kDeformable = 1;
const EntityType	kAudio = 2;
const EntityType	kEvent = 3;
const EntityType	kUnknown = 4;

const StringCount	kCompoundStringsPerEntity = 1;
const StringCount	kDeformableStringsPerPart = 4;
const StringCount	kAudioStringsPerEntity = 4;

const StringIndex	kCompoundFilename = 0;

const StringIndex	kDeformableEntityName = 0;
const StringIndex	kDeformableMeshFilename = 1;
const StringIndex	kDeformableSkeletonPath = 2;
const StringIndex	kDeformableAnimationFilename = 3;

const StringIndex	kAudioFilename    = 0;
const StringIndex	kAudioAttenuation = 1;
const StringIndex	kAudioMinDistance = 2;
const StringIndex	kAudioMaxDistance = 3;
// --------------------------------------------------------------------------
// DB startup. Returns true on success, or false on failure. Invoke this first.
bool			startup					(const ROS::ROSString& db_extension_filename, const ROS::ROSString& db_data_path);
// DB shutdown. Invoke this last.									
void			shutdown				();
// Returns the same value as that returned by the startup function.
bool			has_started_up			();
// Returns the data path with which the startup() function was invoked
ROS::ROSString get_data_path			();

/* Category access functions */										
// Returns the number of categories of entities in the DB			
U32				get_category_count		();
// Gets the string identifier of the category in the DB
ROS::ROSString	get_category_name		(U32 index);
// Returns the type of the entity corresonding to the specified index
EntityType		get_category_type		(U32 index);

/* Entity access functions */										
// Returns the number of entities in the DB.
U32				get_entity_count		();
/* Entity property access functions */
// Gets the name of the entity corresonding to the specified index
ROS::ROSString	get_entity_name			(U32 index);
// Returns the category index of the entity corresonding to the specified index
U32				get_entity_category		(U32 index);
// Returns the number of strings associated with the entity corresonding to the specified index
U32				get_entity_string_count	(U32 index);
// Gets the strings associated with the entity corresonding to the specified index. The number of strings will be the same as
// that returned by get_entity_string_count() for the same entity index.
void			get_entity_strings		(U32 index, ROS::StringList& string_list);

// Sets the description strings to those provided by the extension for the specified symbolic name
void			get_entity_strings		(const ROS::ROSString& entity_name, ROS::ROSString& entity_category, ROS::StringList& string_list);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif