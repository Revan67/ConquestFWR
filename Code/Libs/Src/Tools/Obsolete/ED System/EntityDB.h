// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef EntityDB_h
#define EntityDB_h
// --------------------------------------------------------------------------
#include"Typedefs.h"
// --------------------------------------------------------------------------
#ifdef BUILD_ENTITY_DB_DLL
	#define EDB_DLL_DEC __declspec(dllexport)
	#define EDB_DLL_DEF __declspec(dllexport)
#else
	#define EDB_DLL_DEC __declspec(dllimport)
#endif
// --------------------------------------------------------------------------
#ifdef __cplusplus 
extern "C" { 
#endif
// --------------------------------------------------------------------------
const U32 kEDBStringLength = 200;
// --------------------------------------------------------------------------
typedef C8	EDBCharString[kEDBStringLength];	// Null terminated
typedef U32 EDBEntityIndex;
typedef U32 EDBCategoryIndex;
typedef U32 EDBEntityType;
typedef U32 EDBStringCount;
typedef U32 EDBStringIndex;
// --------------------------------------------------------------------------
const EDBEntityType	kEDBCompound   = 0;
const EDBEntityType	kEDBDeformable = 1;
const EDBEntityType kEDBAudio      = 2;
const EDBEntityType kEDBEvent      = 3;

const EDBStringCount	kEDBCompoundStringsPerEntity = 1;
const EDBStringCount	kEDBDeformableStringsPerPart = 4;
const EDBStringCount	kEDBAudioStringsPerEntity    = 4;

const EDBStringIndex	kEDBCompoundFilename = 0;

const EDBStringIndex	kEDBDeformableEntityName        = 0;
const EDBStringIndex	kEDBDeformableMeshFilename      = 1;
const EDBStringIndex	kEDBDeformableSkeletonPath      = 2;
const EDBStringIndex	kEDBDeformableAnimationFilename = 3;

const EDBStringIndex	kEDBAudioFilename    = 0;
const EDBStringIndex	kEDBAudioAttenuation = 1;
const EDBStringIndex	kEDBAudioMinDistance = 2;
const EDBStringIndex	kEDBAudioMaxDistance = 3;

// --------------------------------------------------------------------------
// Note: function pointer typedefs are provided for use when loading the
// dll at runtime.
// --------------------------------------------------------------------------
/* DB startup and shutdown */
// DB startup. Returns true on success, or false on failure. Invoke this first.
EDB_DLL_DEC	bool				__cdecl edb_startup									(const EDBCharString data_path);
typedef		bool				(__cdecl *edb_startup_func_type)					(const EDBCharString data_path);
// DB shutdown. Invoke this last.									
EDB_DLL_DEC	void				__cdecl edb_shutdown								();
typedef		void				(__cdecl *edb_shutdown_func_type)					();
																	
/* Category access functions */										
// Returns the number of categories of entities in the DB			
EDB_DLL_DEC	U32					__cdecl edb_get_category_count						();
typedef		U32					(__cdecl *edb_get_category_count_func_type)			();
// Gets the string identifier of the category in the DB
EDB_DLL_DEC	void				__cdecl edb_get_category_name						(EDBCategoryIndex index, EDBCharString name);
typedef		void				(__cdecl *edb_get_category_name_func_type)			(EDBCategoryIndex index, EDBCharString name);

// Convert a category to a type
EDB_DLL_DEC	EDBEntityType	__cdecl edb_get_category_type					(EDBCategoryIndex category);
typedef		EDBEntityType	(__cdecl *edb_get_category_type_func_type)		(EDBCategoryIndex category);

EDB_DLL_DEC EDBCategoryIndex __cdecl edb_get_entity_category_from_name (EDBCharString name);
// Get the category given the nickname, used only for fixing up old scripts

/* Entity access functions */										
// Returns the number of entities in the DB.
EDB_DLL_DEC	U32					__cdecl edb_get_entity_count						();
typedef		U32					(__cdecl *edb_get_entity_count_func_type)			();
/* Entity property access functions */
// Gets the name of the entity corresonding to the specified index
EDB_DLL_DEC	void				__cdecl edb_get_entity_name							(EDBEntityIndex index, EDBCharString name);
typedef		void				(__cdecl *edb_get_entity_name_func_type)			(EDBEntityIndex index, EDBCharString name);

// Returns the category index of the entity corresonding to the specified index
EDB_DLL_DEC	EDBCategoryIndex	__cdecl edb_get_entity_category						(EDBEntityIndex index);
typedef		EDBCategoryIndex	(__cdecl *edb_get_entity_category_func_type)		(EDBEntityIndex index);

#if 0
// OBSOLETE
// Returns the type of the entity corresonding to the specified index
EDB_DLL_DEC	EDBEntityType		__cdecl edb_get_entity_type							(EDBEntityIndex index);
typedef		EDBEntityType		(__cdecl *edb_get_entity_type_func_type)			(EDBEntityIndex index);
#endif

// Returns the number of strings associated with the entity corresonding to the specified index
EDB_DLL_DEC	EDBStringCount		__cdecl	edb_get_entity_string_count					(EDBEntityIndex index);
typedef		EDBStringCount		(__cdecl *edb_get_entity_string_count_func_type)	(EDBEntityIndex index);
// Gets the strings associated with the entity corresonding to the specified index. The number of strings will be the same as
// that returned by edb_get_entity_string_count() for the same entity index.
EDB_DLL_DEC	void				__cdecl edb_get_entity_strings						(EDBEntityIndex index, EDBCharString* strings, EDBStringCount string_count);
typedef		void				(__cdecl *edb_get_entity_strings_func_type)			(EDBEntityIndex index, EDBCharString* strings, EDBStringCount string_count);
// --------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
// --------------------------------------------------------------------------
#endif
