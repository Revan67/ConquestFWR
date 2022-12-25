//--------------------------------------------------------------------------//
//                                                                          //
//                              Engine.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/dev/Src/Engine/engine.cpp 80    3/03/00 10:40a Emaurer $
*/			    
//---------------------------------------------------------------------------

#pragma warning( disable : 4786 )
#pragma warning( disable : 4530 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <functional>
#include <list>
#include <vector>
#include <map>

#include "dacom.h"                    // DA component manager
#include "TComponent.h"
#include "TSmartPointer.h"
#include "stddat.h"
#include "FDump.h"
#include "TempStr.h"
#include "da_heap_utility.h"
#include "3dmath.h"
#include "SysConsumerDesc.h"
#include "SearchPath.h"
#include "FileSys_Utility.h"
#include "IProfileParser.h"
#include "ICamera.h"
#include "engine.h"
#include "engcomp.h"
#include "engine2.h"

#include "Tfuncs.h"
#include "handlemap.h"

#include "EngineArchetype.h"
#include "EngineInstance.h"
#include "LocalEngineInstance.h"

//

const char *CLSID_Engine = "IEngine";	

const Vector    ZeroVector;
Matrix			IdentityMatrix;		// cannot be const because constructor does not set up Identity
const Transform IdentityTransform;

//

#define MAX_COMPONENT_NAME_LEN 48

//

#if !defined(FINAL_RELEASE) && defined(DEBUG_ARCHETYPE_REF_CNT)
#define DEBUG_REF_CNT_TRACE(x) GENERAL_TRACE_2(x)
#define DEBUG_REF_CNT_VERIFY_RELEASE(x) verify_release_archetype_is_safe( x )
#else
#define DEBUG_REF_CNT_TRACE(x)
#define DEBUG_REF_CNT_VERIFY_RELEASE(x) 
#endif

#if !defined(FINAL_RELEASE) && defined(DEBUG_ARCHETYPE_REF_CNT)
U32 start_ref_cnt = 0;
U32 end_ref_cnt = 0;
U32 before_load = 0;
U32 after_load = 0;
#define DEBUG_REF_CNT_GET(x) \
	{	\
		static_cast<IEngine*>(this)->AddRef();	\
		x = static_cast<IEngine*>(this)->Release();	\
	}
#define DEBUG_REF_CNT_TEST_EQUAL(x,y,msg) \
	{\
		static_cast<IEngine*>(this)->AddRef();	\
		y = static_cast<IEngine*>(this)->Release();	\
		if( (x) != (y) ) {\
			GENERAL_WARNING( msg );\
		}\
	}	
#else
#define DEBUG_REF_CNT_GET(x)		
#define DEBUG_REF_CNT_TEST_EQUAL(x,y,msg) 
#endif

//

#if !defined(FINAL_RELEASE) && defined(DEBUG_ARCHETYPE_LIFETIME)
#define DEBUG_LIFETIME_TRACE(x)	GENERAL_TRACE_1(x)
#else
#define DEBUG_LIFETIME_TRACE(x)
#endif

//

struct string_compare 
{
	bool operator()( const char * _X, const char * _Y) const
	{
		return (strcmp( _X, _Y ) < 0);
	}
};

//


// ..........................................................................
//
// LoadedComponent
//
// Templated interface pointer used in STL containers.
//
template< typename I >
struct LoadedComponent
{
	LoadedComponent( const char *_component_name = NULL, I *_component = NULL )
	{
		if( _component_name ) {
			strcpy( component_name, _component_name );
		}
		else {
			component_name[0] = 0;
		}

		component = _component;
	}
	
	//

	~LoadedComponent()
	{
		component_name[0] = 0;
		component = NULL;
	}

	char component_name[MAX_COMPONENT_NAME_LEN];
	I *component;
};

//

typedef LoadedComponent< IDAComponent >		LoadedDacomComponent;
typedef std::list< LoadedDacomComponent >	LoadedDacomComponentList;
typedef LoadedComponent< IEngineComponent >	LoadedEngineComponent;
typedef std::list< LoadedEngineComponent >	LoadedEngineComponentList;

//

typedef arch_handlemap< EngineArchetype* >	EngineArchetypeMap;
typedef inst_handlemap< EngineInstance* >	EngineInstanceMap;
typedef inst_handlemap< EngineInstance* >	EngineRootInstanceMap;

typedef handlemap<const char*,EngineInstance*,string_compare> PartNameToInstanceMap;


// ..........................................................................
// 
// ENGINE
//
// This is the Engine aggregate component in all of its glory.
//
//
struct DACOM_NO_VTABLE ENGINE : public IEngine, public IEngine2
{
public:
	// Define table of interfaces accessible through QueryInterface()
	//
	BEGIN_DACOM_MAP_INBOUND(ENGINE)
	DACOM_INTERFACE_ENTRY(IEngine)
	DACOM_INTERFACE_ENTRY2(IID_IEngine,IEngine)
	DACOM_INTERFACE_ENTRY(IEngine2)
	DACOM_INTERFACE_ENTRY2(IID_IEngine2,IEngine2)
	END_DACOM_MAP()

protected:	// Data

	COMPTR<IComponentFactory> fileFactory;			// used in create_file_system() call

	mutable EngineArchetypeMap		engine_archetypes;
	mutable EngineInstanceMap		engine_instances;
	mutable EngineRootInstanceMap	engine_instance_roots;

	bool have_loaded_components;
	LoadedDacomComponentList	loaded_components;	// contains "inner" interface pointers for all 
													// components aggregated into the engine
	LoadedEngineComponentList	engine_components;	// contains IEngineComponent interface pointers
													// for "true" engine components.

protected: // Interface
	void verify_release_archetype_is_safe( ARCHETYPE_INDEX arch_index );
	
	EngineArchetype *get_archetype( ARCHETYPE_INDEX arch_index ) const;
	ARCHETYPE_INDEX get_arch_index( const char *arch_name ) const;
	
	HRESULT create_compound_archetype_part( const char *part_dir, IFileSystem *root_filesys, HANDLE part_dir_srch_in_root_filesys, IFileSystem *parts_filesys, EngineArchetype::EngineArchetypePart *out_part );
	HRESULT create_compound_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *root_filesys, IFileSystem *parts_filesys );
	INSTANCE_INDEX	create_compound_instance( ARCHETYPE_INDEX arch_index, IEngineInstance * userInstance);

	EngineInstance *get_instance( INSTANCE_INDEX inst_index ) const;
	HRESULT destroy_instance_tree( EngineInstance *instance );
	HRESULT remove_child_from_parent( EngineInstance *parent, EngineInstance *child );
	HRESULT update_joints_above( EngineInstance *root_instance, EngineInstance *child_instance, EngineFlags en_f_flags );
	HRESULT update_joints_below( EngineInstance *root_instance );
	void update_instance( EngineInstance *instance, EngineFlags en_f_flags, SINGLE dt );

public:	// Interface
	
	// Local publics
	//
	ENGINE( void );
	~ENGINE( void );
	GENRESULT init( DACOMDESC *info );

	DA_HEAP_DEFINE_NEW_OPERATOR(ENGINE)

	
	// IEngine methods
	GENRESULT COMAPI load_engine_components( struct IDAComponent *system ) ;
	int	COMAPI enumerate_engine_components( EngineComponentCallback callback ) ;
	void COMAPI update( SINGLE dt ) ;
	void COMAPI set_search_path( const char *path ) ;
	U32 COMAPI get_search_path( char *out_path, U32 max_path_len ) const ;
	void COMAPI set_search_path2( struct IComponentFactory * fileFactory ) ;
	void COMAPI get_search_path2( struct IComponentFactory **fileFactory ) const ;
	GENRESULT COMAPI create_file_system( const char *filename, struct IFileSystem **out_filesystem, IComponentFactory *filesystem_factory  ) ;

	ARCHETYPE_INDEX	COMAPI allocate_archetype( const C8 *arch_name );
	ARCHETYPE_INDEX	COMAPI create_archetype( const C8 *arch_name, struct IFileSystem *filesys );
	ARCHETYPE_INDEX	COMAPI duplicate_archetype( ARCHETYPE_INDEX arch_index, const C8 *new_arch_name );
	void COMAPI hold_archetype( ARCHETYPE_INDEX arch_index );
	void COMAPI release_archetype( ARCHETYPE_INDEX arch_index );
	const C8* COMAPI get_archetype_name( ARCHETYPE_INDEX arch_index ) const ;
	ARCHETYPE_INDEX	COMAPI get_archetype_by_name( const char *arch_name );
	GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	GENRESULT COMAPI enumerate_archetype_joints( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback callback, void *user_data ) const ;
	GENRESULT COMAPI enumerate_archetype_parts( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback callback, void *user_data ) const ;
	BOOL32 COMAPI is_archetype_compound( ARCHETYPE_INDEX arch_index ) const ;
	
	INSTANCE_INDEX COMAPI create_instance( const C8 *arch_name, struct IFileSystem *filesys, IEngineInstance *UserInstance ) ;
	INSTANCE_INDEX COMAPI create_instance2( ARCHETYPE_INDEX arch_index, IEngineInstance *UserInstance );
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index );
	ARCHETYPE_INDEX	COMAPI get_instance_archetype( INSTANCE_INDEX inst_index );
	const C8* COMAPI get_instance_part_name( INSTANCE_INDEX inst_index ) const ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, SINGLE dt );
	vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, EngineFlags en_f_flags, float lod_fraction, U32 rf_flags, const Transform *modifier_transform );
	void COMAPI set_instance_handler( INSTANCE_INDEX inst_index, IEngineInstance *newUserInstance );
	void COMAPI get_instance_handler( INSTANCE_INDEX inst_index, IEngineInstance **out_UserInstance );
	GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
	void COMAPI set_user_data( INSTANCE_INDEX inst_index, S32 data );
	S32 COMAPI get_user_data( INSTANCE_INDEX inst_index ) const;
	void COMAPI set_instance_bounding_sphere( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, const float radius, const Vector &center );
	void COMAPI get_instance_bounding_sphere( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, float *radius, Vector *center ) const;
	void COMAPI set_position( INSTANCE_INDEX inst_index, const Vector &position );
	const Vector & COMAPI get_position( INSTANCE_INDEX inst_index ) const;
	void COMAPI set_orientation( INSTANCE_INDEX inst_index, const Matrix &orientation );
	const Matrix & COMAPI get_orientation( INSTANCE_INDEX inst_index ) const;
	void COMAPI set_transform( INSTANCE_INDEX inst_index, const Transform &transform );
	const Transform & COMAPI get_transform( INSTANCE_INDEX inst_index ) const;
	void COMAPI set_velocity( INSTANCE_INDEX inst_index, const Vector &vel );
	const Vector & COMAPI get_velocity( INSTANCE_INDEX inst_index ) const;
	void COMAPI set_angular_velocity( INSTANCE_INDEX inst_index, const Vector &ang );
	const Vector & COMAPI get_angular_velocity( INSTANCE_INDEX inst_index ) const;
	
	BOOL32 COMAPI create_joint( INSTANCE_INDEX parent_inst_index, INSTANCE_INDEX child_inst_index, const JointInfo *joint_info ) ;
	BOOL32 COMAPI destroy_joint( INSTANCE_INDEX parent_inst_index, INSTANCE_INDEX child_inst_index ) ;
	const JointInfo *COMAPI get_joint_info( INSTANCE_INDEX child_inst_index ) const ;
	BOOL32 COMAPI set_joint_state( INSTANCE_INDEX child_inst_index, IE_JOINTSTATETYPE state_type, const float *state_vector ) const ;
	BOOL32 COMAPI get_joint_state( INSTANCE_INDEX child_inst_index, IE_JOINTSTATETYPE state_type, float *out_state_vector ) const ;
	INSTANCE_INDEX COMAPI get_root_instance_next( INSTANCE_INDEX prev_root_inst_index = INVALID_INSTANCE_INDEX ) const ;
	INSTANCE_INDEX COMAPI get_instance_parent( INSTANCE_INDEX child_inst_index ) const ;
	INSTANCE_INDEX COMAPI get_instance_child_next( INSTANCE_INDEX parent_inst_index, EngineFlags en_f_flags, INSTANCE_INDEX prev_child_inst_index = INVALID_INSTANCE_INDEX ) const ;
	INSTANCE_INDEX COMAPI get_instance_root( INSTANCE_INDEX inst_index ) const ;

public:

	// IEngine2 functions
	GENRESULT COMAPI enumerate_archetype_joints2( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback2 callback, void *user_data ) const;
	const C8* COMAPI get_archetype_part_name2( ARCHETYPE_INDEX arch_index ) const;
};

//

ENGINE::ENGINE( void )
{
	loaded_components.clear();
	engine_components.clear();
	have_loaded_components = false;

	IdentityMatrix.set_identity();
}

//

ENGINE::~ENGINE (void)
{

	fileFactory = NULL;

	EngineInstanceMap::iterator inst;
	for( inst = engine_instances.begin(); inst != engine_instances.end(); inst++ ) {
		GENERAL_TRACE_1( TEMPSTR( "Engine: dtor: Instance %d using archetype %d dangling.\n", (*inst).first, (*inst).second->archetype->arch_index ) );
#ifdef _DEBUG
		EngineInstance *d_inst = static_cast<EngineInstance*>(inst->second);
#endif
	}

	EngineArchetypeMap::iterator arch;
	for( arch = engine_archetypes.begin(); arch != engine_archetypes.end(); arch++ ) {
		GENERAL_TRACE_1( TEMPSTR( "Engine: dtor: Archetype %d has %d dangling references.\n", (*arch).first, (*arch).second->ref_cnt ) );
#ifdef _DEBUG
		EngineArchetype *d_arch = static_cast<EngineArchetype*>(arch->second);
#endif
	}

	engine_instance_roots.clear();

	// unload engine components

	LoadedDacomComponentList::iterator lb = loaded_components.begin();
	LoadedDacomComponentList::iterator le = loaded_components.end();
	LoadedDacomComponentList::iterator lc;

	IDAComponent *dac;

	for( lc = lb; lc != le ; ) {
		dac = (*lc).component;
		dac->Release();
		lc = loaded_components.erase( lc );
	}

	engine_components.clear();
	have_loaded_components = false;
}

//

GENRESULT ENGINE::init( DACOMDESC *info )
{
	return GR_OK;
}

//

EngineArchetype *ENGINE::get_archetype( ARCHETYPE_INDEX arch_index ) const
{
	if( arch_index == INVALID_ARCHETYPE_INDEX ) {
		return NULL;
	}

	EngineArchetypeMap::const_iterator arch;

	if( (arch = engine_archetypes.find( arch_index )) == engine_archetypes.end() ) {
		return NULL;
	}

	return const_cast<EngineArchetype*>( (arch->second) );
}

//

ARCHETYPE_INDEX ENGINE::get_arch_index( const char *arch_name ) const
{
	if( arch_name == NULL ) {
		return INVALID_ARCHETYPE_INDEX ;
	}

#pragma message( "TODO: fix O(n) get_arch_index " )

	EngineArchetypeMap::const_iterator ea;
	EngineArchetypeMap::const_iterator beg = engine_archetypes.begin();
	EngineArchetypeMap::const_iterator end = engine_archetypes.end();

	for( ea = beg; ea != end; ea++ ) {
		if( strcmp( (*ea).second->arch_name, arch_name ) == 0 ) {
			return (*ea).first;
		}
	}

	return INVALID_ARCHETYPE_INDEX;
}

//

EngineInstance *ENGINE::get_instance( INSTANCE_INDEX inst_index ) const
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return NULL;
	}

	EngineInstanceMap::const_iterator inst;

	if( (inst = engine_instances.find( inst_index )) == engine_instances.end() ) {
		return NULL;
	}

	return const_cast<EngineInstance*>(inst->second);
}

//

GENRESULT ENGINE::load_engine_components( struct IDAComponent *system )
{
	if( have_loaded_components ) {
		return GR_OK;
	}

	if( system == NULL ) {
		GENERAL_NOTICE( "Engine: load_engine_components: system parameter is NULL, continuing with complete faith in the client...\n" );
	}

	DEBUG_REF_CNT_GET( before_load );

	ICOManager *dacom = DACOM_Acquire ();
	COMPTR<IProfileParser> parser;
	HANDLE hSection;
	char buffer[256];
	int line=0;

	if( FAILED( dacom->QueryInterface( IID_IProfileParser, (void**) &parser ) ) ) {
		GENERAL_ERROR( "Engine: load_engine_components: unable to acquire IID_IProfileParser\n" );
		return GR_GENERIC;
	}

	if( (hSection = parser->CreateSection( "Engine" )) == 0 ) {
		have_loaded_components = true;
		return GR_OK;
	}


	while( parser->ReadProfileLine( hSection, line++, buffer, sizeof(buffer) ) != 0 ) {

		char* ptr = NULL;
		char* ptr2 = NULL;
		bool eq_seen = false;
					
		char* tok = strtok( buffer, " \t" );

		while( tok != NULL ) {
			if( *tok != ';' ) {
				if( !ptr ) {
					ptr = tok;
				}
				else if( eq_seen ) {
					ptr2 = tok;
				}
				else if( *tok == '=' ) {
					eq_seen = true;
				}
			}
			else {
				break;
			}
			
			tok = strtok( NULL, " \t" );
		}
					
		if( ptr == NULL ) {
			continue;
		}
		
		IDAComponent *inner = NULL;		//stash
		IDAComponent *discard = NULL;
		SYSCONSUMERDESC info;
		const char *response = "";

		info.system = system;
		info.interface_name = ptr;
		info.outer = (IEngine*) this;
		info.inner = &inner;
		info.description = ptr2;
		
		if( (ptr = const_cast<char *>(strchr( info.interface_name, ' ' ))) != 0 ) {
			*ptr = 0;
		}

		if( (ptr = const_cast<char *>(strchr( info.interface_name, '\t' ))) != 0 ) {
			*ptr = 0;
		}

		DEBUG_REF_CNT_GET( start_ref_cnt );

		if( SUCCEEDED( dacom->CreateInstance( &info, (void **)&discard ) ) ) {
			ASSERT( inner );
			response = "[OK]";
			loaded_components.push_back( LoadedDacomComponent( info.interface_name, inner ) );
		}
		else {
			response = "[FAILED]";
		}

		DEBUG_REF_CNT_TEST_EQUAL( start_ref_cnt, end_ref_cnt, 
								  TEMPSTR( "Creation of '%s' caused possible Engine ref count leak.", info.interface_name ) );

		GENERAL_NOTICE( TEMPSTR( "ENGINE::load_engine_components: aggregation of '%s' [%s] returned %s\n", 
					   info.interface_name, 
					   info.description ? info.description : "",
					   response ) );

	} // end while( ReadLine() )
			
	IAggregateComponent *agg = NULL;
	IEngineComponent *engcomp = NULL;
	LoadedDacomComponentList::iterator nec, nbg, nen;

	// Initialize the aggregated components
	//
	nec = nbg = loaded_components.begin();
	nen = loaded_components.end();

	while( nec != nen ) { 

		if( agg ) {
			agg->Release();
			agg = NULL;
		}

		if( SUCCEEDED( (*nec).component->QueryInterface( IID_IAggregateComponent, (void**) &agg ) ) ) {
		
			DEBUG_REF_CNT_GET( start_ref_cnt );

			if( FAILED( agg->Initialize() ) ) {

				GENERAL_WARNING( TEMPSTR( "ENGINE::load_engine_components: initialization of '%s' failed, removing from aggregate\n", (*nec).component_name ) ) ;

				DEBUG_REF_CNT_TEST_EQUAL( start_ref_cnt, end_ref_cnt, 
										  TEMPSTR( "Failed initialization of '%s' caused possible Engine ref count leak.", (*nec).component_name ) );

				loaded_components.erase( nec );

				// start the traversal over
				nec = loaded_components.begin();
				nen = loaded_components.end();
			}
			else {
				DEBUG_REF_CNT_TEST_EQUAL( start_ref_cnt, end_ref_cnt, 
										  TEMPSTR( "Successful initialization of '%s' caused possible Engine ref count leak.", (*nec).component_name ) );
				nec++;
			}

		}
	}

	if( agg ) {
		agg->Release();
		agg = NULL;
	}



	// Build engine components list from aggregated components list
	//
	nec = nbg = loaded_components.begin();
	nen = loaded_components.end();

	while( nec != nen ) { 
		
		DEBUG_REF_CNT_GET( start_ref_cnt );

		if( SUCCEEDED( (*nec).component->QueryInterface( IID_IEngineComponent, (void**)&engcomp ) ) ) {
			engcomp->Release();
			engine_components.push_back( LoadedEngineComponent( (*nec).component_name, engcomp ) );

			DEBUG_REF_CNT_TEST_EQUAL( start_ref_cnt, end_ref_cnt, 
									  TEMPSTR( "Successful query for IEngineComponent on '%s' caused possible Engine ref count leak.", (*nec).component_name ) );

		}
		else {
			DEBUG_REF_CNT_TEST_EQUAL( start_ref_cnt, end_ref_cnt, 
									  TEMPSTR( "Failed query for IEngineComponent on '%s' caused possible Engine ref count leak.", (*nec).component_name ) );
		}

		nec++;
	}

	DEBUG_REF_CNT_TEST_EQUAL( before_load, after_load, "Loading engine components caused possible ref count leak." );
	
	have_loaded_components = true;

	return GR_OK;
}

//

int COMAPI ENGINE::enumerate_engine_components( EngineComponentCallback callback )
{
	if( callback != NULL ) {
		LoadedEngineComponentList::iterator ec;
		LoadedEngineComponentList::iterator bg = engine_components.begin();
		LoadedEngineComponentList::iterator en = engine_components.end();
		for( ec=bg; ec != en; ec++ ) {
			if( false == callback( (*ec).component_name, (*ec).component ) )
			{
				break;
			}
		}
	}
	return engine_components.size();
}

//	

void ENGINE::set_search_path( const char *_path )
{
	SEARCHPATHDESC sdesc;
	COMPTR<ISearchPath> searchPath;

	if( _path == NULL ) {
		GENERAL_WARNING( "Engine: set_search_path: path is NULL" );
		return;
	}

	if( FAILED( DACOM_Acquire()->CreateInstance( &sdesc, searchPath ) ) ) {
		GENERAL_WARNING( "Engine: set_search_path: unable to create search path" );
		return;
	}

	searchPath->SetPath( _path );

	fileFactory = searchPath;
}

//

void ENGINE::set_search_path2( struct IComponentFactory *_fileFactory )
{
	if( _fileFactory == NULL ) {
		GENERAL_TRACE_1( "Engine: set_search_path2: setting search path to NULL\n" );
	}

	fileFactory = _fileFactory;
}

//

U32 ENGINE::get_search_path( char *out_path, U32 path_max_len ) const
{
	COMPTR<ISearchPath> searchPath;

	if( fileFactory == 0 || FAILED( fileFactory->QueryInterface( IID_ISearchPath, searchPath ) ) ) {
		*out_path = 0;
		return 0;
	}
	
	return searchPath->GetPath( out_path, path_max_len );
}

//

void ENGINE::get_search_path2( struct IComponentFactory **_fileFactory ) const
{
	if( (*_fileFactory = fileFactory) == NULL ) {  
		*_fileFactory = DACOM_Acquire();
	}

	(*_fileFactory)->AddRef();
}

//

GENRESULT ENGINE::create_file_system( const char *filename, IFileSystem **out_filesys, IComponentFactory *filesys_factory )
{
	if( filename == NULL || out_filesys == NULL ) {
		GENERAL_TRACE_1( "Engine: create_file_system: filename or out_filesys is NULL\n" );
		return GR_INVALID_PARMS;
	}

	DAFILEDESC desc( filename );

	if( filesys_factory == NULL ) {
		// check to see if user has given us an explicit path
		//
		if( filename[1] == ':' || (filename[0] == '\\' && filename[1] == '\\') ) {
			filesys_factory = DACOM_Acquire();
		}
		else if( (filesys_factory = fileFactory) == NULL ) {
			filesys_factory = fileFactory = DACOM_Acquire();
		}
	}

	return filesys_factory->CreateInstance( &desc, (void**)out_filesys );
}

//

ARCHETYPE_INDEX ENGINE::allocate_archetype( const C8 *arch_name )
{
	char buf[32];
	ARCHETYPE_INDEX new_arch_index;

	if( arch_name == NULL ) {
		// Allocate an anonymous archetype name.
		// 
		// Start with a counter tacked on the end of a string.
		// Then search and make sure no one is using that archetype name already
		//
		static int anonymous_name_counter = 0;
		
		do {
			wsprintf( buf, "Anonymous-%d", anonymous_name_counter++ );		// get a unique name
		} while( get_arch_index( buf ) != INVALID_ARCHETYPE_INDEX );

		arch_name = buf;
	}

	if( (new_arch_index = engine_archetypes.allocate_handle()) == INVALID_ARCHETYPE_INDEX ) {
		GENERAL_TRACE_1( "Engine: allocate_archetype: unable to allocate archetype\n" );
		return INVALID_ARCHETYPE_INDEX;
	}
	
	engine_archetypes.insert( new_arch_index, new EngineArchetype( arch_name, new_arch_index ) );

	hold_archetype( new_arch_index );
	
	return new_arch_index;
}

//

HRESULT ENGINE::create_compound_archetype_part( const char *part_dir, IFileSystem *root_filesys, HANDLE part_dir_srch_in_root_filesys, IFileSystem *parts_filesys, EngineArchetype::EngineArchetypePart *out_part )
{
	char file_name[PARTFILENAME_MAX];
	COMPTR<IFileSystem> part_file;
	DAFILEDESC desc;
	int index;
	ARCHETYPE_INDEX new_arch_index;
	HRESULT hr0 = -1;
	HRESULT hr1 = -1;
	HRESULT hr2 = -1;

	{
		COMPTR<IFileSystem> part_names_fs;
		DAFILEDESC part_names_desc (part_dir);
		part_names_desc.hFindFirst = part_dir_srch_in_root_filesys;
		
		if( GR_OK == root_filesys->CreateInstance ( &part_names_desc, part_names_fs) ) {

			hr0 = read_string( part_names_fs, OBJECT_NAME, PARTNAME_MAX, out_part->part_name );
			hr1 = read_string( part_names_fs, FILE_NAME, PARTFILENAME_MAX, file_name );
			hr2 = read_type( part_names_fs, "Index", &index );
		}
	}

	if( FAILED(hr0) || FAILED(hr1) || FAILED(hr2) ) {
		return E_FAIL;
	}

	if( create_file_system( file_name, part_file, parts_filesys ) != GR_OK ) {
		if( create_file_system( file_name, part_file, NULL ) != GR_OK ) {
			return E_FAIL;
		}
	}

	if( (new_arch_index = create_archetype( file_name, part_file )) == INVALID_ARCHETYPE_INDEX ) {
		return E_FAIL;
	}

	out_part->part_archetype = get_archetype( new_arch_index );

	return S_OK;
}

//

HRESULT ENGINE::create_compound_archetype( ARCHETYPE_INDEX arch_index, IFileSystem *root_filesys, IFileSystem *parts_filesys )
{
	WIN32_FIND_DATA find_data;
	HANDLE srch;
	char part_filter[32];
	EngineArchetype *compound_root_archetype = NULL;
	EngineArchetype::EngineArchetypePart new_part;

	GENERAL_TRACE_5( TEMPSTR( "Engine: create_compound_archetype_parts: creating compound rooted at %d\n", arch_index ) );

	if( (compound_root_archetype = get_archetype( arch_index )) == NULL ) {
		return E_FAIL;
	}

	// step 1: create root archetype and other sub parts
	//
	if( SUCCEEDED( create_compound_archetype_part( ROOT_OBJ_NAME, root_filesys, INVALID_HANDLE_VALUE, parts_filesys, &new_part ) ) ) {
		compound_root_archetype->parts.push_back( new_part );
	}

	strcpy( part_filter, PART_STEM );
	strcat( part_filter, "*" );

	if( (srch = root_filesys->FindFirstFile( part_filter, &find_data )) != INVALID_HANDLE_VALUE ) {
		
		do {
			if( FAILED( create_compound_archetype_part( find_data.cFileName, root_filesys, srch, parts_filesys, &new_part ) ) ) {
				GENERAL_WARNING( "Engine: create_compound_archetype: unable to create part archetype, ignoring..." );
				continue;
			}

			ASSERT( new_part.part_archetype != NULL ) ;
			compound_root_archetype->parts.push_back( new_part );
		}
		while( root_filesys->FindNextFile( srch, &find_data ) ) ;

		root_filesys->FindClose( srch );
	}

	// step 3: build connections
	//
	if( !root_filesys->SetCurrentDirectory( CONNECTION_DIR_NAME ) ) {
		GENERAL_WARNING( "Engine: create_compound_archetype_parts: unable to find connections." );
		return E_FAIL;
	}

	if( (srch = root_filesys->FindFirstFile( "*", &find_data )) != INVALID_HANDLE_VALUE ) {

		do {
			if( find_data.cFileName[0] == '.') {
				continue;
			}

			ASSERT( !(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) );

			DAFILEDESC desc( find_data.cFileName );
			desc.hFindFirst = srch;
			HANDLE hFile;
			U32 sizeof_file, joint_count, joint, br, sizeof_type;
			Cyl in;	
			JointType type;

			if( (hFile = root_filesys->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {

				sizeof_file = root_filesys->GetFileSize( hFile, NULL );
				type = JointInfo::get_type_from_name( find_data.cFileName );

				switch( type ) {
				case JT_FIXED:			sizeof_type = sizeof(Fix);		break;
				case JT_PRISMATIC:		sizeof_type = sizeof(Pris);		break;
				case JT_REVOLUTE:		sizeof_type = sizeof(Rev);		break;
				case JT_DAMPED_SPRING:	sizeof_type = sizeof(Spring);	break;
				case JT_CYLINDRICAL:	sizeof_type = sizeof(Cyl);		break;
				case JT_SPHERICAL:		sizeof_type = sizeof(PersistSphere);	break;
				case JT_TRANSLATIONAL:	sizeof_type = sizeof(Trans);	break;
				case JT_LOOSE:			sizeof_type = sizeof(Loose);	break;
				default:				
					sizeof_type = 0;
					GENERAL_WARNING( "Engine: unknown joint type, probably random garbage" );
				}

				ASSERT( sizeof_type <= sizeof(Cyl) );

				joint_count = sizeof_file / sizeof_type;

				for( joint = 0; joint < joint_count; joint++ ) {
						
					if( (root_filesys->ReadFile( hFile, &in, sizeof_type, &br )) && br==sizeof_type ) {
							
						EngineArchetype::EngineArchetypeJoint eaj;

						strcpy( eaj.parent_part.part_name, in.parent );
						eaj.parent_part.part_archetype = compound_root_archetype->get_part_archetype( in.parent );
						
						strcpy( eaj.child_part.part_name, in.child );
						eaj.child_part.part_archetype = compound_root_archetype->get_part_archetype( in.child );
						
						if( eaj.parent_part.part_archetype == NULL || eaj.child_part.part_archetype == NULL ) {
							GENERAL_WARNING( TEMPSTR( "Joint %s<->%s not found!", in.parent, in.child ) );
							continue;
						}

						switch( eaj.joint_info.type = type ) {
						
						case JT_LOOSE:			
						case JT_TRANSLATIONAL:	
						case JT_FIXED:			
							eaj.joint_info.rel_position = ((Fix*)&in)->pos;
							eaj.joint_info.rel_orientation = ((Fix*)&in)->orient;
							break;

						case JT_PRISMATIC:		
						case JT_REVOLUTE:		
							eaj.joint_info.parent_point = ((Rev*)&in)->parent_point;
							eaj.joint_info.child_point = ((Rev*)&in)->child_point;
							eaj.joint_info.rel_orientation = ((Rev*)&in)->rel_orientation;
							eaj.joint_info.axis = ((Rev*)&in)->axis;
							eaj.joint_info.min0 = ((Rev*)&in)->min;
							eaj.joint_info.max0 = ((Rev*)&in)->max;
							break;

						case JT_DAMPED_SPRING:	
							eaj.joint_info.parent_point = ((Spring*)&in)->parent_point;
							eaj.joint_info.child_point = ((Spring*)&in)->child_point;
							eaj.joint_info.spring_constant = ((Spring*)&in)->spring_constant;
							eaj.joint_info.damping_constant = ((Spring*)&in)->damping_constant;
							eaj.joint_info.rest_length = ((Spring*)&in)->rest_length;
							break;

						case JT_CYLINDRICAL:	
							eaj.joint_info.parent_point = ((Cyl*)&in)->parent_point;
							eaj.joint_info.child_point = ((Cyl*)&in)->child_point;
							eaj.joint_info.rel_orientation = ((Cyl*)&in)->rel_orientation;
							eaj.joint_info.axis = ((Cyl*)&in)->axis;
							eaj.joint_info.min0 = ((Cyl*)&in)->min_trans;
							eaj.joint_info.max0 = ((Cyl*)&in)->max_trans;
							eaj.joint_info.min1 = ((Cyl*)&in)->min_rot;
							eaj.joint_info.max1 = ((Cyl*)&in)->max_rot;
							break;

						case JT_SPHERICAL:		
							eaj.joint_info.parent_point = ((PersistSphere*)&in)->parent_point;
							eaj.joint_info.child_point = ((PersistSphere*)&in)->child_point;
							eaj.joint_info.rel_orientation = ((PersistSphere*)&in)->rel_orientation;
							eaj.joint_info.min0 = ((PersistSphere*)&in)->min_about_i;
							eaj.joint_info.max0 = ((PersistSphere*)&in)->max_about_i;
							eaj.joint_info.min1 = ((PersistSphere*)&in)->min_about_j;
							eaj.joint_info.max1 = ((PersistSphere*)&in)->max_about_j;
							eaj.joint_info.min2 = ((PersistSphere*)&in)->min_about_k;
							eaj.joint_info.max2 = ((PersistSphere*)&in)->max_about_k;
							break;

						}

						compound_root_archetype->joints.push_back( eaj );
					}
				}

				root_filesys->CloseHandle( hFile );
			}

		}
		while( root_filesys->FindNextFile( srch, &find_data ) );

		root_filesys->FindClose( srch );
	}

	root_filesys->SetCurrentDirectory( ".." );

	return S_OK;
}

//

ARCHETYPE_INDEX COMAPI ENGINE::create_archetype( const C8 *arch_name, IFileSystem *filesys )
{
	ARCHETYPE_INDEX arch_index = INVALID_ARCHETYPE_INDEX;
	int num_created = 0;
	COMPTR<IFileSystem> cmpnd_dir;
	DAFILEDESC desc( IFS_COMPOUND_DIRECTORY );

	if( arch_name == NULL ) {
		GENERAL_TRACE_1( "Engine: create_archetype: arch_name is NULL\n" );
		return INVALID_ARCHETYPE_INDEX;
	}
	
	// See if archetype has already been loaded
	// If so, return its index
	//
	if( (arch_index = get_arch_index( arch_name )) != INVALID_ARCHETYPE_INDEX ) {
		hold_archetype( arch_index );	// add reference to archetype
		return arch_index;
	}

	// Everything below here requires a filesystem
	//
	if( filesys == NULL ) {
		GENERAL_TRACE_1( "Engine: create_archetype: filesys is NULL\n" );
		return INVALID_ARCHETYPE_INDEX;
	}
	
	// Allocate new archetype entry in engine list and all components' lists
	// Note that allocate_archetype() increments the reference count (to 1)
	//
	if( (arch_index = allocate_archetype( arch_name )) == INVALID_ARCHETYPE_INDEX ) {
		GENERAL_TRACE_1( "Engine: create_archetype: unable to allocate_archetype\n" );
		return INVALID_ARCHETYPE_INDEX;
	}

	if( FAILED( filesys->CreateInstance( &desc, cmpnd_dir ) ) ||
		FAILED( create_compound_archetype( arch_index, cmpnd_dir, filesys ) ) ) {

		// Otherwise, try to load it as a simple engine archetype
		//

		num_created = 0;

		LoadedEngineComponentList::iterator ec;
		LoadedEngineComponentList::iterator bg = engine_components.begin();
		LoadedEngineComponentList::iterator en = engine_components.end();

		for( ec=bg; ec != en; ec++ ) {
			if( (*ec).component->create_archetype( arch_index, filesys ) ) {
				DEBUG_LIFETIME_TRACE( TEMPSTR( "Engine: create_archetype: '%s' created archetype %d (%s)\n", (*ec).component_name, arch_index, arch_name ) );
				num_created++;
			}
		}

		if( num_created == 0 ) {
			// remove the archetype from the system
			//
			EngineArchetypeMap::iterator arch;
			if( (arch = engine_archetypes.find( arch_index )) != engine_archetypes.end() ) {
				engine_archetypes.erase( arch_index );
				delete arch->second;
			}
			return INVALID_ARCHETYPE_INDEX;
		}

	}

	return arch_index;
}

//

ARCHETYPE_INDEX	COMAPI ENGINE::duplicate_archetype( ARCHETYPE_INDEX arch_index, const C8 *new_arch_name )
{
	ARCHETYPE_INDEX new_arch_index;

	if( arch_index == INVALID_ARCHETYPE_INDEX ) {
		GENERAL_TRACE_1( "Engine: duplicate_archetype: called w/ an invalid archetype index.\n" );
		return INVALID_ARCHETYPE_INDEX;
	}

#pragma message( "TODO: make duplicate_archetype work with compounds" )
	if( get_archetype( arch_index ) && get_archetype( arch_index )->parts.size() > 0 ) {
		GENERAL_WARNING("Engine: duplicate_archetype: arch_index is a compound archetype (not valid).");
		return INVALID_ARCHETYPE_INDEX;
	}

	if( get_arch_index( new_arch_name ) != INVALID_ARCHETYPE_INDEX ) {
		GENERAL_WARNING("Engine: duplicate_archetype: archetype name already exists in duplicate_archetype().");
		return INVALID_ARCHETYPE_INDEX;
	}

	// Allocate new archetype entry in engine list and all components' lists
	// Note that allocate_archetype() increments the reference count (to 1)
	//
	if( (new_arch_index  = allocate_archetype( new_arch_name )) == INVALID_ARCHETYPE_INDEX ) {
		GENERAL_TRACE_1( "Engine: duplicate_archetype: unable to allocate_archetype\n" );
		return INVALID_ARCHETYPE_INDEX;
	}
	
	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();
	for( ec=bg; ec != en; ec++ ) {
		(*ec).component->duplicate_archetype( new_arch_index, arch_index );
	}

	return new_arch_index;
}

//

void ENGINE::hold_archetype( ARCHETYPE_INDEX arch_index )
{
	EngineArchetype *ea;
	if( (ea = get_archetype( arch_index )) != NULL ) {
		ea->add_ref( this );
		DEBUG_REF_CNT_TRACE( TEMPSTR( "Engine: hold_archetype: arch_index: %d (%s) ref_count: %d\n", arch_index, ea->arch_name, ea->ref_cnt ) );
	}
}

//

void ENGINE::verify_release_archetype_is_safe( ARCHETYPE_INDEX arch_index )
{
	EngineInstanceMap::iterator ei;
	EngineInstanceMap::iterator beg = engine_instances.begin();
	EngineInstanceMap::iterator end = engine_instances.end();

	for( ei = beg; ei != end; ei++ ) {
		if( (*ei).second->archetype->arch_index == arch_index ) {
			GENERAL_WARNING( TEMPSTR("Engine: release_archetype: %d releasing archetype with active instances", arch_index ) );
			break;
		}
	}
}

//

void ENGINE::release_archetype( ARCHETYPE_INDEX arch_index )
{
	EngineArchetype *ea;
	if( (ea = get_archetype( arch_index )) != NULL ) {

		int ref_cnt = ea->release_ref( this );

		if( ref_cnt == 0 ) {

			DEBUG_REF_CNT_TRACE( TEMPSTR( "Engine: release_archetype: arch_index: %d (%s) ref_count: %d (releasing)\n", arch_index, ea->arch_name, ea->ref_cnt ) );
			DEBUG_REF_CNT_VERIFY_RELEASE( arch_index );

			LoadedEngineComponentList::iterator ec;
			LoadedEngineComponentList::iterator bg = engine_components.begin();
			LoadedEngineComponentList::iterator en = engine_components.end();
			
			ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

			for( ec=bg; ec != en; ec++ ) {
				(*ec).component->destroy_archetype( arch_index );
			}
			
			engine_archetypes.erase( ea->arch_index );
			delete ea;

			return;
		}

		DEBUG_REF_CNT_TRACE( TEMPSTR( "Engine: release_archetype: arch_index: %d (%s) ref_count: %d (not releasing)\n", arch_index, ea->arch_name, ea->ref_cnt ) );
	}
}

//

ARCHETYPE_INDEX COMAPI ENGINE::get_archetype_by_name( const char *arch_name )
{ 
	if( arch_name == NULL ) {
		return INVALID_ARCHETYPE_INDEX;
	}

	ARCHETYPE_INDEX arch_index;
	
	if( (arch_index = get_arch_index( arch_name )) != INVALID_ARCHETYPE_INDEX ) {
		hold_archetype( arch_index );
	}

	return arch_index;
}

//

GENRESULT COMAPI ENGINE::query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) 
{
	*out_iif = NULL;

	if( arch_index == INVALID_ARCHETYPE_INDEX || iid == NULL || out_iif == NULL ) {
		return GR_INVALID_PARMS;
	}
	
	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();
	for( ec=bg; ec != en; ec++ ) {
		if( SUCCEEDED( (*ec).component->query_archetype_interface( arch_index, iid, out_iif ) ) ) {
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

INSTANCE_INDEX COMAPI ENGINE::create_instance( const C8 *arch_name, IFileSystem *filesys, IEngineInstance *UserInstance  )
{
	ASSERT( arch_name );

	ARCHETYPE_INDEX arch_index;
	INSTANCE_INDEX inst_index;
	
	// Create archetype for named object, or simply retrieve its index if it
	// has already been created
	//
	if( (arch_index = create_archetype( arch_name, filesys )) == INVALID_ARCHETYPE_INDEX ) {
		//emaurer:  if the intention was to create an archetype 
		//and it didn't happen, return failure.
		GENERAL_TRACE_1( "Engine: create_instance: unable to acquire archetype\n" );
		return INVALID_INSTANCE_INDEX;
	}

	// Create instance based on specified archetype
	//
	if( (inst_index = create_instance2( arch_index, UserInstance )) == INVALID_INSTANCE_INDEX ) {
		GENERAL_TRACE_1( "Engine: create_instance: unable to create instance\n" );
		release_archetype( arch_index );	// release our local archetype reference
	
		return INVALID_INSTANCE_INDEX;
	}
	
	release_archetype( arch_index );	// release our local archetype reference

	return inst_index;
}

//

INSTANCE_INDEX COMAPI ENGINE::create_instance2( ARCHETYPE_INDEX arch_index, IEngineInstance *userInstance )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	INSTANCE_INDEX inst_index = INVALID_INSTANCE_INDEX;
	EngineArchetype *archetype;

	if( (archetype = get_archetype( arch_index )) == NULL ) {
		return INVALID_INSTANCE_INDEX;
	}

	if( archetype->parts.size() > 0 ) {
		// Create compound instance by creating an instance tree based
		// on the joint template stored in the archetype.
		//

		if( (inst_index = create_compound_instance( arch_index, userInstance ) ) == INVALID_INSTANCE_INDEX ) {
			return INVALID_INSTANCE_INDEX;
		}

		get_instance( inst_index )->instance_handler->create_instance( inst_index );

		engine_instance_roots.insert( inst_index,  get_instance( inst_index ) );
	}
	else {
		// Create simple instance based on specified archetype in lists maintained by 
		// engine and all of its components
		//

		hold_archetype( arch_index );

		inst_index = engine_instances.allocate_handle();
		engine_instances.insert( inst_index, new EngineInstance( inst_index, NULL, archetype, NULL ) );

		set_instance_handler( inst_index, userInstance );
		
		LoadedEngineComponentList::iterator ec;
		LoadedEngineComponentList::iterator bg = engine_components.begin();
		LoadedEngineComponentList::iterator en = engine_components.end();
		for( ec=bg; ec != en; ec++ ) {
			(*ec).component->create_instance( inst_index, arch_index );
		}

		get_instance( inst_index )->instance_handler->create_instance( inst_index );
	}

	return inst_index;
}

//

HRESULT ENGINE::destroy_instance_tree( EngineInstance *instance )
{
	ASSERT( instance != NULL );
	ASSERT( instance->instance_handler != NULL );

	DEBUG_REF_CNT_TRACE( TEMPSTR( "Engine: destroy_instance: inst_index: %d destroying...\n", instance->inst_index ) );

	// destroy our children first
	//
	EngineInstanceJointList::iterator c_kid ;
	while( (c_kid = instance->children.begin()) != instance->children.end() ) {
		// note that this call will remove the child from our children
		//
		destroy_instance_tree( (*c_kid).child_instance );
	}

	// destroy this instance
	//
	instance->instance_handler->destroy_instance( instance->inst_index );

	if( instance->parent ) {
		instance->parent->remove_child( instance );		// remove us from our parent
	}

	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();

	for( ec=bg; ec != en; ec++ ) {
		(*ec).component->destroy_instance( instance->inst_index );
	}
	
	if( instance->ei_f_flags & EI_F_DELETE_INSTANCE_HANDLER ) {
		delete instance->instance_handler;
	}
	instance->instance_handler = NULL;

	engine_instance_roots.erase( instance->inst_index );
	engine_instances.erase( instance->inst_index );

	release_archetype( instance->archetype->arch_index );
	
	if( instance->root_archetype ) {
		release_archetype( instance->root_archetype->arch_index );
	}

	DEBUG_REF_CNT_TRACE( TEMPSTR( "Engine: destroy_instance: inst_index: %d dead\n", instance->inst_index ) );
	
	delete instance;

	return S_OK;
}

//

void COMAPI ENGINE::destroy_instance( INSTANCE_INDEX inst_index )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) != NULL ) {
		
		destroy_instance_tree( instance );
	}
}

//

const C8 * COMAPI ENGINE::get_archetype_name( ARCHETYPE_INDEX arch_index ) const
{
	if( INVALID_ARCHETYPE_INDEX == arch_index ) {
		return NULL;
	}

	EngineArchetype *ea;
	if( (ea = get_archetype( arch_index )) != NULL ) {
		return ea->arch_name;
	}
	
	return NULL;
}

//

ARCHETYPE_INDEX ENGINE::get_instance_archetype( INSTANCE_INDEX inst_index )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return INVALID_INSTANCE_INDEX;
	}

	ASSERT( instance->archetype->arch_index != INVALID_ARCHETYPE_INDEX );

	hold_archetype( instance->archetype->arch_index );

	return instance->archetype->arch_index;
}

//

void ENGINE::set_instance_handler( INSTANCE_INDEX inst_index, IEngineInstance *newUserInstance )
{
	IEngineInstance *new_instance_handler;
	IEngineInstance *old_instance_handler;
	EngineInstance *instance;
	U32 old_ei_f_flags;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return ;
	}

	old_instance_handler = instance->instance_handler;
	old_ei_f_flags = instance->ei_f_flags;

	if( newUserInstance == NULL ) {
		instance->ei_f_flags |= EI_F_DELETE_INSTANCE_HANDLER;
		new_instance_handler = new LocalEngineInstance;
	}
	else {
		instance->ei_f_flags &= ~(EI_F_DELETE_INSTANCE_HANDLER);
		new_instance_handler = newUserInstance;
	}

	ASSERT( new_instance_handler );

	if( new_instance_handler == old_instance_handler ) {
		return;
	}

	new_instance_handler->initialize_instance( inst_index );

	if( old_instance_handler ) {
		Vector radius_center;
		float radius;

		// set the state in the new handler based on the old handler
		new_instance_handler->set_transform( inst_index, old_instance_handler->get_transform( inst_index ) );
		new_instance_handler->set_velocity( inst_index, old_instance_handler->get_velocity( inst_index ) );
		new_instance_handler->set_angular_velocity( inst_index, old_instance_handler->get_angular_velocity( inst_index ) );
		old_instance_handler->get_centered_radius( inst_index, &radius, &radius_center );
		new_instance_handler->set_centered_radius( inst_index, radius, radius_center );

#pragma message( "TODO: spec. instance_handler->destroy_instance" )

		old_instance_handler->destroy_instance( inst_index );
	}

	if( old_ei_f_flags & EI_F_DELETE_INSTANCE_HANDLER ) {
		delete old_instance_handler;
		old_instance_handler = NULL;
	}

	instance->instance_handler = new_instance_handler;
}

//

void ENGINE::get_instance_handler( INSTANCE_INDEX inst_index, IEngineInstance **out_UserInstance )
{
	EngineInstanceMap::iterator ei;

	*out_UserInstance = NULL;

	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return ;
	}

	*out_UserInstance = get_instance( inst_index )->instance_handler;
}

//

GENRESULT ENGINE::query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) 
{
	*out_iif = NULL;

	if( inst_index == INVALID_INSTANCE_INDEX || iid == NULL || out_iif == NULL ) {
		return GR_INVALID_PARMS;
	}
	
	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();
	for( ec=bg; ec != en; ec++ ) {
		if( SUCCEEDED( (*ec).component->query_instance_interface( inst_index, iid, out_iif ) ) ) {
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

vis_state ENGINE::render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, EngineFlags en_f_flags, float lod_fraction, U32 rf_flags, const Transform *tr )
{
	if( inst_index == INVALID_INSTANCE_INDEX || camera == NULL ) {
		GENERAL_TRACE_1( "Engine: render_instance: inst_index or camera is invalid\n" );
		return VS_UNKNOWN;
	}

	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();

	vis_state most_visible_return = VS_UNKNOWN;

	for( ec=bg; ec != en; ec++ ) {
		most_visible_return = Tmax<vis_state>( most_visible_return, 
											   (*ec).component->render_instance( camera, inst_index, lod_fraction, rf_flags, tr ) );
	}

	return most_visible_return;
}

//

void COMAPI ENGINE::set_user_data( INSTANCE_INDEX inst_index, S32 data )
{
	EngineInstance *instance;
	if( (instance = get_instance( inst_index )) != NULL ) {
		instance->user_data = data;
	}
}

//

S32 COMAPI ENGINE::get_user_data( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;
	if( (instance = get_instance( inst_index )) != NULL ) {
		return instance->user_data ;
	}

	return 0;
}

//

void COMAPI ENGINE::set_instance_bounding_sphere( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, const float radius, const Vector &center )
{
	EngineInstance *instance;
	
	if( (instance = get_instance( inst_index )) == NULL ) {
		GENERAL_TRACE_1( "Engine: set_instance_bounding_sphere: inst_index is invalid, not setting.\n" );
		return;
	}

	if( en_f_flags & EN_DONT_RECURSE ) {
		instance->instance_handler->set_centered_radius( inst_index, radius, center );

		instance->set_dirty_bounding_sphere( true );
	}
	else {
		instance->bounding_sphere_radius = radius;
		instance->bounding_sphere_center = center;

		instance->set_dirty_bounding_sphere( false );
	}
}

//

void COMAPI ENGINE::get_instance_bounding_sphere( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, float *out_radius, Vector *out_center ) const
{
	ASSERT( out_radius );
	ASSERT( out_center );

	EngineInstance *instance;
	
	if( (instance = get_instance( inst_index )) == NULL ) {
		GENERAL_TRACE_1( "Engine: get_instance_bounding_sphere: inst_index is invalid, returning goofy radius\n" );
		*out_radius = -1.0f;
		out_center->zero();
		return;
	}

	if( en_f_flags & EN_DONT_RECURSE ) {
		instance->get_bounding_sphere( false, out_radius, out_center );
	}
	else
	{
		instance->get_bounding_sphere( true, out_radius, out_center );
	}	
}

//

void COMAPI ENGINE::set_position( INSTANCE_INDEX inst_index, const Vector &position )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index ))== NULL ) {
		return;
	}
	
	ASSERT( instance->instance_handler != NULL );
	instance->instance_handler->set_position( inst_index, position );
}

//

const Vector & COMAPI ENGINE::get_position( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return ZeroVector;
	}

	ASSERT( instance->instance_handler != NULL );

	return instance->instance_handler->get_position( inst_index );
}

//

void COMAPI ENGINE::set_orientation( INSTANCE_INDEX inst_index, const Matrix &orientation )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index ))== NULL ) {
		return;
	}
	
	ASSERT( instance->instance_handler != NULL );
	instance->instance_handler->set_orientation( inst_index, orientation );
}

//

const Matrix & COMAPI ENGINE::get_orientation( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return IdentityMatrix;
	}


	ASSERT( instance->instance_handler != NULL );

	return instance->instance_handler->get_orientation( inst_index );
}

//

void COMAPI ENGINE::set_transform( INSTANCE_INDEX inst_index, const Transform &transform )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index ))== NULL ) {
		return;
	}
	
	ASSERT( instance->instance_handler != NULL );
	instance->instance_handler->set_transform( inst_index, transform );
}

//

const Transform & COMAPI ENGINE::get_transform( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return IdentityTransform;
	}

	ASSERT( instance->instance_handler != NULL );

	return instance->instance_handler->get_transform( inst_index );
}

//

void COMAPI ENGINE::set_velocity (INSTANCE_INDEX inst_index, const Vector & vel)
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return ;
	}
	
	get_instance( inst_index )->instance_handler->set_velocity( inst_index, vel );
}

//

const Vector & COMAPI ENGINE::get_velocity (INSTANCE_INDEX inst_index) const
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return ZeroVector;
	}
	
	return get_instance( inst_index )->instance_handler->get_velocity( inst_index );
}

//

void COMAPI ENGINE::set_angular_velocity( INSTANCE_INDEX inst_index, const Vector & ang )
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return ;
	}

	get_instance( inst_index )->instance_handler->set_angular_velocity( inst_index, ang );
}

//

const Vector & COMAPI ENGINE::get_angular_velocity( INSTANCE_INDEX inst_index ) const
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return ZeroVector;
	}
	
	return get_instance( inst_index )->instance_handler->get_angular_velocity( inst_index );
}

//

// ..........................................................................
//
// Engine_Impl
//
//

struct Engine_Impl : public DAComponent<ENGINE>
{
	DEFMETHOD(QueryInterface)( const C8 *interface_name, void **instance );
};

//

GENRESULT Engine_Impl::QueryInterface( const C8 *interface_name, void **instance )
{
	if( DAComponent<ENGINE>::QueryInterface(interface_name, instance) == GR_OK ) {
		return GR_OK;
	}

	// search components first (things like the deformable component)
	// 
	LoadedDacomComponentList::iterator nec;
	LoadedDacomComponentList::iterator nbg = loaded_components.begin();
	LoadedDacomComponentList::iterator nen = loaded_components.end();
	for( nec=nbg; nec != nen; nec++ ) {
		if( SUCCEEDED( (*nec).component->QueryInterface( interface_name, instance ) ) ) {
			return GR_OK;
		}
	}

	*instance = 0;
	return GR_INTERFACE_UNSUPPORTED;
}

// ..........................................................................
// 
// DllMain()
// 
// 

BOOL COMAPI DllMain( HINSTANCE hinstDLL, S32 fdwReason, LPVOID lpvReserved )
{
	IComponentFactory *server;
	
	switch( fdwReason ) {

	case DLL_PROCESS_ATTACH:

		DA_HEAP_ACQUIRE_HEAP(HEAP);
		DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);
		
		if( (server = new DAComponentFactory< Engine_Impl, DACOMDESC >( CLSID_Engine )) != NULL ) {
			DACOM_Acquire()->RegisterComponent( server, CLSID_Engine, DACOM_LOW_PRIORITY );
			server->Release();
		}
		
		break;
	}
	
	return TRUE;
}

//
// Convert all relative coordinates to absolute Cartesian coordinates.
//
void COMAPI ENGINE::update( SINGLE dt )
{
	LoadedEngineComponentList::iterator ec;
	LoadedEngineComponentList::iterator bg = engine_components.begin();
	LoadedEngineComponentList::iterator en = engine_components.end();
	for( ec=bg; ec != en; ec++ ) {
		(*ec).component->update( dt ) ;
	}

	// Update trees based on new state
	//
	EngineRootInstanceMap::iterator r_beg = engine_instance_roots.begin();
	EngineRootInstanceMap::iterator r_end = engine_instance_roots.end();
	EngineRootInstanceMap::iterator root;

	for( root = r_beg; root != r_end; root++ ) {

		update_joints_below( root->second );
	}
}


//

void COMAPI ENGINE::update_instance( INSTANCE_INDEX inst_index, EngineFlags en_f_flags, SINGLE dt )
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return ;
	}

	update_joints_above( instance->parent, instance, en_f_flags );
	
	update_instance( instance, en_f_flags, dt );
}

//

void ENGINE::update_instance( EngineInstance *instance, EngineFlags en_f_flags, SINGLE dt )
{
	if( EN_UPDATE_WO_COMPONENTS != (EN_UPDATE_WO_COMPONENTS & en_f_flags) )
	{
		LoadedEngineComponentList::iterator ec;
		LoadedEngineComponentList::iterator bg = engine_components.begin();
		LoadedEngineComponentList::iterator en = engine_components.end();
		for( ec=bg; ec != en; ec++ ) {
			(*ec).component->update_instance( instance->inst_index, dt );
		}
	}

	ASSERT( instance->instance_handler );

	if( EN_DONT_RECURSE != (EN_DONT_RECURSE & en_f_flags) )
	{
		EngineInstanceJointList::iterator c_beg = instance->children.begin();
		EngineInstanceJointList::iterator c_end = instance->children.end();
		EngineInstanceJointList::iterator c_kid;

		for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
			instance->update_joint( &(*c_kid) );
			if( EN_UPDATE_WO_DERIVATIVES != (EN_UPDATE_WO_DERIVATIVES & en_f_flags) ) {
				instance->update_joint_derivatives( &(*c_kid) );
			}
			update_instance( (*c_kid).child_instance, en_f_flags, dt );
		}
	}
}

//
// this will update child_instance and all it's parents
HRESULT ENGINE::update_joints_above( EngineInstance *root_instance, EngineInstance *child_instance, EngineFlags en_f_flags )
{
	if( root_instance )
	{
		update_joints_above( root_instance->parent, root_instance, en_f_flags );

		if( child_instance )
		{
			EngineInstanceJointList::iterator c_kid = root_instance->children.begin();
			EngineInstanceJointList::iterator c_end = root_instance->children.end();

			while( c_kid != c_end )
			{
				if( c_kid->child_instance == child_instance )
				{
					root_instance->update_joint( &(*c_kid) );
					if( EN_UPDATE_WO_DERIVATIVES != (EN_UPDATE_WO_DERIVATIVES & en_f_flags) ) {
						root_instance->update_joint_derivatives( &(*c_kid) );
					}
					break;
				}

				c_kid++;
			}
		}
	}

	return S_OK;
}

//
// this will NOT update root_instance (only it's children)
HRESULT ENGINE::update_joints_below( EngineInstance *root_instance )
{
	EngineInstanceJointList::iterator c_beg = root_instance->children.begin();
	EngineInstanceJointList::iterator c_end = root_instance->children.end();
	EngineInstanceJointList::iterator c_kid;

	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		root_instance->update_joint( &(*c_kid) );
		root_instance->update_joint_derivatives( &(*c_kid) );
		update_joints_below( (*c_kid).child_instance );
	}

	return S_OK;
}

//

BOOL32 COMAPI ENGINE::create_joint( INSTANCE_INDEX parent_inst_index, INSTANCE_INDEX child_inst_index, const JointInfo *joint_info )
{
	ASSERT( joint_info );

	EngineInstance *parent_instance;
	EngineInstance *child_instance;

	if( (parent_instance = get_instance( parent_inst_index )) == NULL ) {
		return FALSE;
	}

	if( (child_instance = get_instance( child_inst_index )) == NULL ) {
		GENERAL_WARNING( TEMPSTR( "Engine: create_joint: Failing to connect %d <- %d, invalid instances.", parent_inst_index, child_inst_index ) );
		return FALSE;
	}

	if( child_instance->parent != NULL ) {
		GENERAL_WARNING( TEMPSTR( "Engine: create_joint: Failing to connect %d <- %d, child already has a parent.\n", parent_inst_index, child_inst_index ) );
		return FALSE;
	}

	if( child_instance->parent == NULL && child_instance->children.size() > 0) {
		engine_instance_roots.erase( child_instance->inst_index );
	}

	parent_instance->add_child( child_instance, joint_info );
	
	if( parent_instance->children.size() == 1 && parent_instance->parent == NULL ) {
		engine_instance_roots.insert( parent_instance->inst_index, parent_instance );
	}

	update_instance(child_inst_index, EN_UPDATE_WO_COMPONENTS, 0.0f);

	return TRUE;
}

//

BOOL32 COMAPI ENGINE::destroy_joint( INSTANCE_INDEX parent_inst_index, INSTANCE_INDEX child_inst_index )
{
	EngineInstance *child_instance;
	EngineInstance *parent_instance;

	if( (child_instance = get_instance( child_inst_index )) == NULL ) {
		GENERAL_WARNING( TEMPSTR( "Engine: destroy_joint: Failing to disconnect %d <- %d, child not found.", parent_inst_index, child_inst_index ) );
		return FALSE;
	}

	parent_instance = child_instance->parent;
	if( parent_instance == NULL ) {
		GENERAL_WARNING( TEMPSTR( "Engine: destroy_joint: Failing to disconnect %d <- %d, not connected.", parent_inst_index, child_inst_index ) );
		return FALSE;
	}

	parent_instance->remove_child( child_instance ); // sets child_instance->parent to NULL

	if( parent_instance->children.size() == 0 && parent_instance->parent == NULL ) {
		engine_instance_roots.erase( parent_instance->inst_index );
	}

	if( child_instance->children.size() > 0 ) {
		engine_instance_roots.insert( child_instance->inst_index, child_instance );
	}

	return TRUE;
}

//

INSTANCE_INDEX COMAPI ENGINE::get_root_instance_next( INSTANCE_INDEX prev_root_inst_index ) const
{
	EngineRootInstanceMap::iterator inst;

	if( prev_root_inst_index == INVALID_INSTANCE_INDEX ) {
		if( (inst = engine_instance_roots.begin()) != engine_instance_roots.end() ) {
			return inst->first;
		}
	}
	else {
		if( (inst = engine_instance_roots.find( prev_root_inst_index )) != engine_instance_roots.end() ) {
			inst++;
			if( inst != engine_instance_roots.end() ) {
				return inst->first;
			}
		}
	}

	return INVALID_INSTANCE_INDEX;
}

//

GENRESULT COMAPI ENGINE::enumerate_archetype_joints( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback callback, void *user_data ) const
{
	ASSERT( callback );

	EngineArchetypeMap::iterator ea;

	if( arch_index == INVALID_ARCHETYPE_INDEX ) {
		return GR_GENERIC;
	}	

	if( (ea = engine_archetypes.find( arch_index )) == engine_archetypes.end() ) {
		return GR_GENERIC;
	}

	EngineArchetype::EngineArchetypeJointList::iterator cx_beg = ea->second->joints.begin();
	EngineArchetype::EngineArchetypeJointList::iterator cx_end = ea->second->joints.end();
	EngineArchetype::EngineArchetypeJointList::iterator cx;

	for( cx = cx_beg; cx != cx_end; cx++ ) {
		if( false == callback( cx->parent_part.part_archetype->arch_index, cx->child_part.part_archetype->arch_index, user_data ) ) {
			break;
		}
	}

	return GR_OK;
}

//

GENRESULT COMAPI ENGINE::enumerate_archetype_parts( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback callback, void *user_data ) const
{
	ASSERT( callback );

	EngineArchetypeMap::iterator ea;

	if( arch_index == INVALID_ARCHETYPE_INDEX ) {
		return GR_GENERIC;
	}	

	if( (ea = engine_archetypes.find( arch_index )) == engine_archetypes.end() ) {
		return GR_GENERIC;
	}

	EngineArchetype::EngineArchetypePartList::iterator al_beg = ea->second->parts.begin();
	EngineArchetype::EngineArchetypePartList::iterator al_end = ea->second->parts.end();
	EngineArchetype::EngineArchetypePartList::iterator al;

	for( al = al_beg; al != al_end; al++ ) {
		if( false == callback( arch_index, al->part_archetype->arch_index, user_data ) ) {
			break;
		}
	}

	return GR_OK;
}

//

BOOL32 COMAPI ENGINE::is_archetype_compound( ARCHETYPE_INDEX arch_index ) const
{
	EngineArchetype *archetype;

	if( (archetype = get_archetype( arch_index )) == NULL ) {
		return FALSE;
	}

	return !archetype->parts.empty();
}

//

INSTANCE_INDEX COMAPI ENGINE::get_instance_root( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return INVALID_INSTANCE_INDEX;
	}
	
	while( instance->parent ) {
		instance = instance->parent;
	}

	return instance->inst_index;
}

//

INSTANCE_INDEX COMAPI ENGINE::get_instance_parent( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return INVALID_INSTANCE_INDEX;
	}
	
	return instance->parent ? instance->parent->inst_index : INVALID_INSTANCE_INDEX ;
}

//

INSTANCE_INDEX COMAPI ENGINE::get_instance_child_next( INSTANCE_INDEX parent_inst_index, EngineFlags en_f_flags, INSTANCE_INDEX prev_child_inst_index ) const
{
	EngineInstance *parent_instance = NULL;
	EngineInstance *child_instance = NULL;
	EngineInstanceJoint *joint;

	if( parent_inst_index == INVALID_INSTANCE_INDEX )
	{
		GENERAL_TRACE_1( TEMPSTR( "ENGINE::get_instance_child_next() INVALID_INSTANCE_INDEX passed in as parent_inst_index.\n" ) );
		return INVALID_INSTANCE_INDEX;
	}

	if( prev_child_inst_index == INVALID_INSTANCE_INDEX ) {
		
		// First child
		//
		
		if( (parent_instance = get_instance( parent_inst_index )) == NULL ) {
			return INVALID_INSTANCE_INDEX;
		}

		if( (joint = parent_instance->get_child_joint_after( NULL )) != NULL ) {
			return joint->child_instance->inst_index;
		}

	}
	else if( (child_instance = get_instance( prev_child_inst_index )) != NULL ) {

		ASSERT( child_instance->parent );

		if( en_f_flags & EN_DONT_RECURSE ) {
			// Don't recurse.  Return the next child after us in our parent's list.
			//
			if( (joint = child_instance->parent->get_child_joint_after( child_instance )) != NULL ) {
				return joint->child_instance->inst_index;
			}
		}
		else {
			// Recurse. This is done as a depth-first traversal of our tree.
			//
			if( !child_instance->children.empty() ) {
				if( (joint = child_instance->get_child_joint_after( NULL )) != NULL ) {
					// Return our first child
					//
					return joint->child_instance->inst_index;
				}
			}
			else {

				EngineInstance *child = child_instance;
				do
				{
					if( (joint = child->parent->get_child_joint_after( child )) != NULL ) {
						// Return the next sibling after us in our parent's list
						//
						return joint->child_instance->inst_index;
					}

					child = child->parent;

				// child chould never be NULL if prev_child_inst_index is really below parent_inst_index
				}while( child && child->inst_index != parent_inst_index );

			}
		}
	}

	return INVALID_INSTANCE_INDEX;
}

//

const C8* COMAPI ENGINE::get_instance_part_name( INSTANCE_INDEX inst_index ) const
{
	EngineInstance *instance;

	if( (instance = get_instance( inst_index )) == NULL ) {
		return NULL;
	}

	if( instance->root_archetype == NULL ) {
		return NULL;
	}

	return instance->part_name;
}

//

const JointInfo *COMAPI ENGINE::get_joint_info( INSTANCE_INDEX child_inst_index ) const 
{
	EngineInstance *instance;
	EngineInstanceJoint *joint;

	if( (instance = get_instance( child_inst_index )) == NULL ) {
		return NULL;
	}

	if( instance->parent == NULL ) {
		return NULL;	 // not connected
	}

	if( (joint = instance->parent->get_child_joint( instance )) == NULL ) {
		return NULL;
	}

	return &joint->info;
}

//

BOOL32 COMAPI ENGINE::get_joint_state( INSTANCE_INDEX child_inst_index, IE_JOINTSTATETYPE state_type, float *out_state_vector ) const 
{
	ASSERT( out_state_vector );

	EngineInstance *instance;
	EngineInstanceJoint *joint;

	if( (instance = get_instance( child_inst_index )) == NULL ) {
		return FALSE;
	}

	if( instance->parent == NULL ) {
		return FALSE;	 // not connected
	}

	if( (joint = instance->parent->get_child_joint( instance )) == NULL ) {
		return FALSE;
	}

	switch( state_type ) {
	
	case IE_JST_BASIC:
		joint->get_state_vector( out_state_vector );
		break;

	case IE_JST_FIRST_DERIVATIVE:
		joint->get_state_vector_derivatives( out_state_vector );
		break;
	
	}

	return TRUE;
}

//

BOOL32 COMAPI ENGINE::set_joint_state( INSTANCE_INDEX child_inst_index, IE_JOINTSTATETYPE state_type, const float *state_vector ) const 
{
	ASSERT( state_vector );

	EngineInstance *instance;
	EngineInstanceJoint *joint;

	if( (instance = get_instance( child_inst_index )) == NULL ) {
		return FALSE;
	}

	if( instance->parent == NULL ) {
		return FALSE;	 // not connected
	}

	if( (joint = instance->parent->get_child_joint( instance )) == NULL ) {
		return FALSE;
	}

	switch( state_type ) {
	
	case IE_JST_BASIC:
		joint->set_state_vector( state_vector );
		break;

	case IE_JST_FIRST_DERIVATIVE:
		joint->set_state_vector_derivatives( state_vector );
		break;
	
	}

	return TRUE;
}

//

INSTANCE_INDEX ENGINE::create_compound_instance( ARCHETYPE_INDEX root_arch_index, IEngineInstance *instance_handler )
{
	ASSERT( root_arch_index != INVALID_ARCHETYPE_INDEX );

	EngineArchetype *root_archetype;

	if( (root_archetype = get_archetype( root_arch_index )) == NULL ) {
		return INVALID_INSTANCE_INDEX;
	}

	// first, create all of the necessary parts 
	//
	INSTANCE_INDEX part_inst_index;
	ARCHETYPE_INDEX part_arch_index;
	EngineInstance *part_instance;
	PartNameToInstanceMap part_inst_map;

	EngineArchetype::EngineArchetypePartList::iterator c_beg = root_archetype->parts.begin();
	EngineArchetype::EngineArchetypePartList::iterator c_end = root_archetype->parts.end();
	EngineArchetype::EngineArchetypePartList::iterator c_cur;

	IEngineInstance *i_handler = instance_handler;
	for( c_cur = c_beg; c_cur != c_end; c_cur++ ) {

		part_arch_index = get_arch_index( (*c_cur).part_archetype->arch_name );

		if( (part_inst_index = create_instance2( part_arch_index, i_handler )) == INVALID_INSTANCE_INDEX ) {
			continue;
		}

		part_instance = engine_instances.find( part_inst_index )->second;
		
		ASSERT( part_instance );

		part_instance->root_archetype = root_archetype;
		
		part_inst_map.insert( (*c_cur).part_name, part_instance );

		part_instance->part_name = (*c_cur).part_name;

		hold_archetype( root_arch_index );

		i_handler = NULL;
	}

	// next, connect the instances according to the archetype joint template
	//
	EngineArchetype::EngineArchetypeJointList::iterator j_beg = root_archetype->joints.begin();
	EngineArchetype::EngineArchetypeJointList::iterator j_end = root_archetype->joints.end();
	EngineArchetype::EngineArchetypeJointList::iterator j_cur ;

	PartNameToInstanceMap::iterator child_inst;
	PartNameToInstanceMap::iterator parent_inst;
	EngineInstance *child_instance = NULL;
	EngineInstance *parent_instance = NULL;

	for( j_cur = j_beg; j_cur != j_end; j_cur++ ) {

		// find parts

		if( (child_inst = part_inst_map.find( j_cur->child_part.part_name )) == part_inst_map.end() ) {
			GENERAL_WARNING( TEMPSTR( "Unable to find child part '%s' in archetype '%s'", j_cur->child_part.part_name, root_archetype->arch_name ) );
			continue;
		}
		child_instance = child_inst->second;

		if( (parent_inst = part_inst_map.find( j_cur->parent_part.part_name )) == part_inst_map.end() ) {
			GENERAL_WARNING( TEMPSTR( "Unable to find parent part '%s' in archetype '%s'", j_cur->parent_part.part_name, root_archetype->arch_name ) );
			continue;
		}
		parent_instance = parent_inst->second;

		ASSERT( child_instance != NULL );
		ASSERT( parent_instance != NULL );
		ASSERT( parent_instance != child_instance );

		if( !parent_instance->add_child( child_instance, &j_cur->joint_info ) ) {
			GENERAL_WARNING( "Unable to connect child to parent" );
		}
	}

	// next, update tree from the root to intialize joints
	//
	while( parent_instance && parent_instance->parent ) // find root
	{
		parent_instance = parent_instance->parent;
	}
	
	engine_instance_roots.insert( parent_instance->inst_index, parent_instance );

	return parent_instance->inst_index;
}

//--------------------------------------------------------------------------//
// Engine2 functions
//--------------------------------------------------------------------------//

GENRESULT COMAPI ENGINE::enumerate_archetype_joints2( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback2 callback, void *user_data ) const
{
	ASSERT( callback );

	EngineArchetypeMap::iterator ea;

	if( arch_index == INVALID_ARCHETYPE_INDEX ) {
		return GR_GENERIC;
	}	

	if( (ea = engine_archetypes.find( arch_index )) == engine_archetypes.end() ) {
		return GR_GENERIC;
	}

	EngineArchetype::EngineArchetypeJointList::iterator cx_beg = ea->second->joints.begin();
	EngineArchetype::EngineArchetypeJointList::iterator cx_end = ea->second->joints.end();
	EngineArchetype::EngineArchetypeJointList::iterator cx;

	for( cx = cx_beg; cx != cx_end; cx++ ) {
		if( false == callback( cx->parent_part.part_archetype->arch_index, cx->child_part.part_archetype->arch_index, &(cx->joint_info), user_data ) ) {
			break;
		}
	}

	return GR_OK;
}

const C8* COMAPI ENGINE::get_archetype_part_name2( ARCHETYPE_INDEX arch_index ) const
{
	if (arch_index == INVALID_ARCHETYPE_INDEX)
	{
		return NULL;
	}

	EngineArchetypeMap::iterator ea;

	if( (ea = engine_archetypes.find( arch_index )) == engine_archetypes.end() ) {
		return NULL;
	}

	EngineArchetype::EngineArchetypePartList::iterator cx_beg = ea->second->parts.begin();
	EngineArchetype::EngineArchetypePartList::iterator cx_end = ea->second->parts.end();
	EngineArchetype::EngineArchetypePartList::iterator cx;

	for( cx = cx_beg; cx != cx_end; cx++ )
	{
		if (cx->part_archetype == ea->second)
		{
			return cx->part_name;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------//
//--------------------------END engine.cpp----------------------------------//
//--------------------------------------------------------------------------//

