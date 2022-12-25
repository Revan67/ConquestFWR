// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DBExtension.h"
#include "EntityDB.h"
#include "StringList.h"
#include "CodeMsg.h"
#include "tempstr.h"
#include "fdump.h"
// --------------------------------------------------------------------------
namespace DBE
{
// --------------------------------------------------------------------------
#if 1
// Temporary solution to make audio file paths relative
static ROS::ROSString							g_db_data_path;
#endif
	
static ROS::ROSString							g_extension_filename;

static HINSTANCE								g_DLL_H							= NULL;

static edb_startup_func_type					g_edb_startup					= NULL;
static edb_shutdown_func_type					g_edb_shutdown					= NULL;
static edb_get_category_count_func_type			g_edb_get_category_count		= NULL;
static edb_get_category_name_func_type			g_edb_get_category_name			= NULL;
static edb_get_category_type_func_type			g_edb_get_category_type			= NULL;
static edb_get_entity_count_func_type			g_edb_get_entity_count			= NULL;
static edb_get_entity_name_func_type			g_edb_get_entity_name			= NULL;
static edb_get_entity_category_func_type		g_edb_get_entity_category		= NULL;
static edb_get_entity_string_count_func_type	g_edb_get_entity_string_count	= NULL;
static edb_get_entity_strings_func_type			g_edb_get_entity_strings		= NULL;
// --------------------------------------------------------------------------
void ErrorMessageBox(const ROS::ROSString& message)
{
	const ROS::ROSString	messageText = ROS::ROSString("Extension: ") + g_extension_filename + ROS::ROSString("\nError: ") + message;

	MessageBox(NULL, messageText.c_str(), "Error in extension", MB_OK);
}
// --------------------------------------------------------------------------
#define LOAD_FUNCTION(function_name)													\
g_##function_name = (function_name##_func_type)GetProcAddress(g_DLL_H, #function_name);	\
if(!g_##function_name)																	\
{																						\
	ErrorMessageBox("Function " #function_name "() was not found.");					\
	FreeLibrary();																		\
	return false;																		\
}
// --------------------------------------------------------------------------
static void FreeLibrary()
{
	ASSERT(g_DLL_H);

	::FreeLibrary(g_DLL_H);
	g_DLL_H							= NULL;
									
	g_edb_startup					= NULL;
	g_edb_shutdown					= NULL;
	g_edb_get_category_count		= NULL;
	g_edb_get_category_name			= NULL;
	g_edb_get_category_type			= NULL;
	g_edb_get_entity_count			= NULL;
	g_edb_get_entity_name			= NULL;
	g_edb_get_entity_category		= NULL;
	g_edb_get_entity_string_count	= NULL;
	g_edb_get_entity_strings		= NULL;
}
// --------------------------------------------------------------------------
// DB startup. Returns true on success, or false on failure. Invoke this first.
bool startup(const ROS::ROSString& db_extension_filename, const ROS::ROSString& db_data_path)
{
	ASSERT(g_DLL_H == NULL);	// startup should be called only once!
	ASSERT(kEDBCompound == kCompound && kEDBDeformable == kDeformable &&	// The constants should match
			kEDBAudio == kAudio && kEDBEvent == kEvent);
	ASSERT(kEDBCompoundStringsPerEntity == kCompoundStringsPerEntity &&		// The constants should match
			kEDBDeformableStringsPerPart == kDeformableStringsPerPart &&
			kEDBAudioStringsPerEntity == kAudioStringsPerEntity);
	
	g_extension_filename = db_extension_filename;
#if 1
// Temporary solution to make audio file paths relative
	g_db_data_path = db_data_path;
#endif
	// Load the library
	g_DLL_H = LoadLibrary(g_extension_filename.c_str());
 
	if(!g_DLL_H)
	{
		DWORD	error = GetLastError();
		
		ErrorMessageBox("Failed to load extension. Check if it exists.");
		return false;
	}

	// Acquire pointers to all the functions
	LOAD_FUNCTION(edb_startup);
	LOAD_FUNCTION(edb_shutdown);
	LOAD_FUNCTION(edb_get_category_count);
	LOAD_FUNCTION(edb_get_category_name);
	LOAD_FUNCTION(edb_get_category_type);	
	LOAD_FUNCTION(edb_get_entity_count);
	LOAD_FUNCTION(edb_get_entity_name);
	LOAD_FUNCTION(edb_get_entity_category);
	LOAD_FUNCTION(edb_get_entity_string_count);
	LOAD_FUNCTION(edb_get_entity_strings);

	// startup the extension
	EDBCharString	edbDataPath;
	strcpy(edbDataPath, db_data_path.c_str());

	const bool	startedup = g_edb_startup(edbDataPath);

	if(!startedup)
	{
		FreeLibrary();

		ErrorMessageBox("Failed to initialize extension. Function edb_startup() failed.");
		return false;
	}
	
	return 0;
}
// --------------------------------------------------------------------------
// DB shutdown. Invoke this last.									
void shutdown()
{
	ASSERT(g_edb_shutdown);

	g_edb_shutdown();

	FreeLibrary();
}
// --------------------------------------------------------------------------
// Returns the same value as that returned by the startup function.
bool has_started_up()
{
	return g_DLL_H != NULL;
}
// --------------------------------------------------------------------------
// Returns the data path with which the startup() function was invoked
ROS::ROSString get_data_path()
{
	return g_db_data_path;
}
// --------------------------------------------------------------------------
/* Category access functions */										
// Returns the number of categories of entities in the DB			
U32 get_category_count()
{
	ASSERT(g_edb_get_category_count);

	return g_edb_get_category_count();
}
// --------------------------------------------------------------------------
// Gets the string identifier of the category in the DB
ROS::ROSString get_category_name(U32 index)
{
	ASSERT(g_edb_get_category_name);

	CharString	name;

	g_edb_get_category_name(index, name);

	return ROS::ROSString(name);
}
// --------------------------------------------------------------------------
/* Entity access functions */										
// Returns the number of entities in the DB.
U32 get_entity_count()
{
	ASSERT(g_edb_get_entity_count);

	return g_edb_get_entity_count();
}
// --------------------------------------------------------------------------
/* Entity property access functions */
// Gets the name of the entity corresonding to the specified index
ROS::ROSString get_entity_name(U32 index)
{
	ASSERT(g_edb_get_entity_name);

	CharString name;

	g_edb_get_entity_name(index, name);

	return ROS::ROSString(name);
}
// --------------------------------------------------------------------------
// Returns the category index of the entity corresonding to the specified index
U32 get_entity_category(U32 index)
{
	ASSERT(g_edb_get_entity_category);

	return g_edb_get_entity_category(index);
}
// --------------------------------------------------------------------------
// Returns the type of the entity corresonding to the specified index
EntityType get_category_type(U32 index)
{
	ASSERT(g_edb_get_category_type);

	const EDBEntityType	entityType = g_edb_get_category_type(index);

	if(!(entityType == kEDBCompound || entityType == kEDBDeformable || entityType == kEDBAudio || entityType == kEDBEvent))
	{
		ErrorMessageBox("The extension returned an unrecognized entity type for the selected entity. Error in function edb_get_category_type().");	// Unknown case
	}

	return entityType;
}
// --------------------------------------------------------------------------
// Returns the number of strings associated with the entity corresonding to the specified index
U32 get_entity_string_count(U32 index)
{
	ASSERT(g_edb_get_entity_string_count);

	const U32			category = get_entity_category(index);
	const EntityType	entityType = get_category_type(category);
	const U32			stringCount = g_edb_get_entity_string_count(index);

	if(	(entityType == kCompound && stringCount == kCompoundStringsPerEntity) || 
		(entityType == kDeformable && (stringCount % kDeformableStringsPerPart) == 0) ||
		(entityType == kAudio && stringCount == kAudioStringsPerEntity) ||
		(entityType == kEvent && stringCount == 0))
	{
		return stringCount;
	}
	else
	{
		ErrorMessageBox("Function edb_get_entity_string_count() returned a count that is incorrect for the selected entity's type.");
		
		return 0;
	}
}
// --------------------------------------------------------------------------
// Gets the strings associated with the entity corresonding to the specified index. The number of strings will be the same as
// that returned by get_entity_string_count() for the same entity index.
void get_entity_strings(U32 index, ROS::StringList& stringList)
{	
	ASSERT(g_edb_get_entity_strings);

	const unsigned int	stringCount = stringList.GetStringCount();

	ASSERT(stringCount == get_entity_string_count(index));

	EDBCharString*	descriptionStrings = new EDBCharString[stringCount];

	g_edb_get_entity_strings(index, descriptionStrings, stringCount);

	try
	{
		for(unsigned int stringIdx = 0; stringIdx < stringCount; ++stringIdx)
		{
			stringList.Replace(stringIdx, descriptionStrings[stringIdx]);
		}
	}
	catch(...)
	{
		delete[] descriptionStrings;

		MessageBox(NULL, "Failed to copy entity's description strings.", "Error", MB_OK);

		return;
	}

	delete[] descriptionStrings;
	descriptionStrings = NULL;
}
// --------------------------------------------------------------------------
// Sets the description strings to those provided by the extension for the specified symbolic name
void get_entity_strings(const ROS::ROSString& entity_name, ROS::ROSString& entity_category, ROS::StringList& string_list)
{
	const U32	string_count = string_list.GetStringCount();
	const U32	entity_count = get_entity_count();
	
	for(U32 entity_idx = 0; entity_idx < entity_count; ++entity_idx)
	{
		if(entity_name == get_entity_name(entity_idx))
		{
			// Found a match!
			get_entity_strings(entity_idx, string_list);
			entity_category = get_category_name(get_entity_category(entity_idx));
			
			return;
		}
	}

	GENERAL_ERROR (TEMPSTR("Entity not found. Name=\"%s\" Catagory=\"%s\"\r\n", entity_name.c_str(), entity_category.c_str()));
	ASSERT(0);	// Entity not found
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
