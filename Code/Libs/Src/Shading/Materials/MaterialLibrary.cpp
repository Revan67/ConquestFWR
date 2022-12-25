// MaterialLibrary.cpp
//
//

#include "Materials.h"

//

#define CLSID_MaterialLibrary "MaterialLibrary"

const char *MaterialMapSection = "MaterialMap";
const char *MaterialMapSpecAll = "*";
const char *MaterialLibraryDirectory = "Material library";
const char *MaterialLibraryFileSpec  = "*.*";
const char *MaterialLibraryEntryTypeKey  = "Type";

//

struct stricmp_pred
{
	bool operator()( const std::string& lhs, const std::string& rhs ) const
	{
		return stricmp( lhs.c_str (), rhs.c_str () ) < 0;
	}
};
		
//


// --------------------------------------------------------------------------
// MaterialLibrary
//
// 
//

struct MaterialLibrary :	IMaterialLibrary,
							IAggregateComponent

{
	BEGIN_DACOM_MAP_INBOUND(MaterialLibrary)
	DACOM_INTERFACE_ENTRY(IMaterialLibrary)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IMaterialLibrary,IMaterialLibrary)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:		// public interface
    
	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT init( AGGDESC *desc );

	// IMaterialLibrary
	GENRESULT COMAPI load_library( IFileSystem *IFS ) ;
	GENRESULT COMAPI free_library( void ) ;
	GENRESULT COMAPI verify_library( U32 max_num_passes, float max_detail_level  ) ;
	GENRESULT COMAPI add_material_map( const char *material_class_spec, const char *clsid ) ;
	GENRESULT COMAPI find_material_map( const char *material_class_spec, U32 clsid_buffer_len, char *out_clsid ) ;
	GENRESULT COMAPI remove_material_map( const char *material_class_spec ) ;
	GENRESULT COMAPI create_material( const char *material_name, const char *material_class, IMaterial **out_material ) ;
	GENRESULT COMAPI add_material( const char *material_name, IMaterial *material ) ;
	GENRESULT COMAPI find_material( const char *material_name, IMaterial **out_material ) ;
	GENRESULT COMAPI remove_material( const char *material_name ) ;
	GENRESULT COMAPI get_material_count( U32 *out_material_count ) ;
	GENRESULT COMAPI get_material( U32 num_material, IMaterial **out_material ) ;
	
	//	
	MaterialLibrary(void);
	~MaterialLibrary(void);
	DA_HEAP_DEFINE_NEW_OPERATOR(MaterialLibrary);

protected:	// protected interface

	GENRESULT initialize_materials_maps( void );
	GENRESULT initialize( IDAComponent *system_container ) ;
	GENRESULT cleanup( void ) ;

	void lock (void);
	void unlock (void);

protected:	// protected data
	typedef std::pair<std::string,std::string>				material_map;
	typedef std::list<material_map>							material_map_list;
	typedef std::map<std::string,IMaterial*,stricmp_pred>	named_material_map;

	mutable CRITICAL_SECTION lock_obj;

	material_map_list	material_maps;
	named_material_map	named_materials;

	IDAComponent		*system_services;
};

// 

DECLARE_MATERIAL( MaterialLibrary, IS_AGGREGATE );

//

MaterialLibrary::MaterialLibrary( void )
{
	system_services = NULL;
	InitializeCriticalSection (&lock_obj);
}

//

MaterialLibrary::~MaterialLibrary( void )
{
	cleanup();
	DeleteCriticalSection (&lock_obj);
}

//

GENRESULT MaterialLibrary::init( AGGDESC *desc )
{ 
	// This is called during normal use.  We are a normal aggregate.
	// Specifically, this is a system aggregate.
	//
	system_services = desc->outer;

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::Initialize(void)
{ 
	return initialize( system_services );
}

//

inline void MaterialLibrary::lock (void)
{
	EnterCriticalSection (&lock_obj);
}

//

inline void MaterialLibrary::unlock (void)
{
	LeaveCriticalSection (&lock_obj);
}

//

GENRESULT MaterialLibrary::initialize_materials_maps( void )
{
	// Read optional overrides from the ini file
	//

	ICOManager *DACOM = DACOM_Acquire();
	COMPTR<IProfileParser> IPP;
	
	char *p, szBuffer[1024+1], *spec, *clsid;
	int line = 0;
	HANDLE hTFC;

	if( SUCCEEDED( DACOM->QueryInterface( IID_IProfileParser, (void**) &IPP ) ) ) {
		
		if( (hTFC = IPP->CreateSection( MaterialMapSection )) != 0 ) {
			
			while( IPP->ReadProfileLine( hTFC, line, szBuffer, 1024 ) ) {
				
				if( (clsid = strchr( szBuffer, '=' )) != NULL ) {
					
					// eat leading/trailing whitespace of spec
					for( p=szBuffer; *p && (*p!=';') && ((*p==' ') || (*p=='\t')); p++ );
					spec = p;
					for( p=clsid; *p && ((*p=='=') || (*p==';') || (*p==' ') || (*p=='\t')); p-- );
					p++;
					*p = 0;

					// eat leading/trailing whitespace of clsid
					clsid++;		// move to beginning of clsid
					for( p=clsid; *p && (*p!=';') && ((*p==' ') || (*p=='\t')); p++ );
					clsid = p;
					for( p=&clsid[strlen(clsid)-1]; *p && ((*p==';') || (*p==' ') || (*p=='\t')); p-- );
					p++;
					*p = 0;

					add_material_map( spec, clsid );
				}
				
				line++;
			}

			IPP->CloseSection( hTFC );
		}
	}

	return GR_OK;
}

//

GENRESULT MaterialLibrary::initialize( IDAComponent *system_container ) 
{
	ASSERT( system_container );

	ICOManager *DACOM;
	COMPTR<IProfileParser> parser;

	cleanup();

	system_services = system_container;  // artificial reference, don't addref
	DACOM = DACOM_Acquire();

	if( FAILED( DACOM->QueryInterface( IID_IProfileParser, parser ) ) ) {
		return GR_GENERIC;
	}

	initialize_materials_maps();

	return GR_OK; 
}

//

GENRESULT MaterialLibrary::cleanup( void ) 
{
	material_maps.clear();

	lock ();

	named_material_map::iterator beg = named_materials.begin();
	named_material_map::iterator end = named_materials.end();
	named_material_map::iterator nm;

	if( beg != end ) {
		U32 ref_cnt;
		char name[255], type[255];

		for( nm=beg; nm!=end; nm++ ) {
			nm->second->AddRef();
			ref_cnt = nm->second->Release();
			nm->second->get_name( name, 255 );
			nm->second->get_type( type, 255 );
			GENERAL_TRACE_1( TEMPSTR( "MaterialLibrary: cleanup: material '%s' [%s] has %d dangling refs\n", name, type, ref_cnt ) );
		}

		GENERAL_TRACE_1( "MaterialLibrary: cleanup: did you forget to call free_library()\n" );
	}

	named_materials.clear();

	unlock ();

	system_services = NULL;

	return GR_GENERIC;
}

//

GENRESULT COMAPI MaterialLibrary::load_library( IFileSystem *IFS ) 
{
	WIN32_FIND_DATA SearchData;
	HANDLE hSearch;
	bool got_any;
	IMaterial *material;
	char type[MAX_PATH];

	if( IFS == NULL ) {
		GENERAL_WARNING( "IFileSystem argument is NULL in MaterialLibrary::load_library" );
		return GR_GENERIC;
	}
	
	got_any = false;

	if( IFS->SetCurrentDirectory( MaterialLibraryDirectory ) ) {

		if( (hSearch = IFS->FindFirstFile( MaterialLibraryFileSpec, &SearchData )) != INVALID_HANDLE_VALUE ) {
			
			do {
				if( SearchData.cFileName[0] == '.' || !(SearchData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
					continue;
				}

				material = NULL;

				// Attempt to find this material first.
				//
				if( FAILED( find_material( SearchData.cFileName, &material ) ) ) {

					// Otherwise create the material
					//
					if( IFS->SetCurrentDirectory( SearchData.cFileName ) ) {

						if( SUCCEEDED( read_string( IFS, MaterialLibraryEntryTypeKey, MAX_PATH, type ) ) ) {
							
							if( FAILED( create_material( SearchData.cFileName, type, &material ) ) ) {
								GENERAL_WARNING( TEMPSTR( "Unable to create material '%s' of type '%s'", SearchData.cFileName, type ) );
							}
							else if( FAILED( material->load_from_filesystem( IFS ) ) ) {
								GENERAL_WARNING( TEMPSTR( "Unable to load material '%s' as type '%s'", SearchData.cFileName, type ) );
								remove_material( SearchData.cFileName );	// Force material library to release their reference
								DACOM_RELEASE( material );					// Release our reference
							}
							else {
								material->set_name( SearchData.cFileName );
								DACOM_RELEASE( material );					// Release our reference
							}
						}
						
						IFS->SetCurrentDirectory( ".." );
					}
				}
			
			} while( IFS->FindNextFile( hSearch, &SearchData ) );

			IFS->FindClose (hSearch);
		}

		IFS->SetCurrentDirectory( ".." );
		
		got_any = true;
	}

	return got_any? GR_OK : GR_GENERIC;
}

//

GENRESULT COMAPI MaterialLibrary::free_library( void ) 
{
	lock ();

	named_material_map::iterator end = named_materials.end();
	named_material_map::iterator nm  = named_materials.begin();

	U32 ref_cnt;
	char name[255], type[255];

	while( nm!=end ) {

		nm->second->AddRef();
		ref_cnt = nm->second->Release();
		if( ref_cnt > 1 ) {
			nm->second->get_name( name, 255 );
			nm->second->get_type( type, 255 );
			GENERAL_TRACE_1( TEMPSTR( "MaterialLibrary: free_library: material '%s' [%s] has %d outstanding refs\n", name, type, ref_cnt ) );
		}
		
		DACOM_RELEASE( nm->second )
		named_materials.erase( nm );
		nm = named_materials.begin();
	}

	unlock ();

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::verify_library( U32 max_num_passes, float max_detail_level ) 
{
	lock ();

	named_material_map::iterator beg = named_materials.begin();
	named_material_map::iterator end = named_materials.end();
	named_material_map::iterator nm;

	for( nm=beg; nm!=end; nm++ ) {
		nm->second->verify( max_num_passes, max_detail_level );
	}

	unlock ();

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::add_material_map( const char *material_class_spec, const char *clsid ) 
{
	material_maps.push_front( material_map( material_class_spec, clsid ) );

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::find_material_map( const char *material_class_spec, U32 clsid_buffer_len, char *out_clsid ) 
{
	const char *spec;
	material_map_list::iterator beg = material_maps.begin();
	material_map_list::iterator end = material_maps.end();
	material_map_list::iterator mm;

	for( mm=beg; mm!=end; mm++ ) {

		spec = mm->first.c_str();
		
		if( (strcmp( spec, MaterialMapSpecAll ) == 0) || (stricmp( spec, material_class_spec ) == 0) ) {

			strcpy( out_clsid, mm->second.c_str() );
			
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

GENRESULT COMAPI MaterialLibrary::remove_material_map( const char *material_class_spec ) 
{
	material_map_list::iterator beg = material_maps.begin();
	material_map_list::iterator end = material_maps.end();
	material_map_list::iterator mm;

	for( mm=beg; mm!=end; mm++ ) {
		
		if( mm->first == material_class_spec ) {
		
			material_maps.erase( mm );
			
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

GENRESULT COMAPI MaterialLibrary::create_material( const char *material_name, const char *material_class, IMaterial **out_material ) 
{
	char clsid[MAX_PATH];
	DACOMDESC desc;

	if( SUCCEEDED( find_material( material_name, out_material ) ) ) {
		return GR_OK;
	}

	*out_material = NULL;

	if( FAILED( find_material_map( material_class, MAX_PATH, clsid ) ) ) {
		strcpy( clsid, material_class );
	}

	desc.interface_name = clsid;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)out_material ) ) ) {
		return GR_GENERIC;
	}

	if( FAILED( (*out_material)->initialize( system_services ) ) ) {
		GENERAL_WARNING( TEMPSTR( "Unable to initialize material '%s' as type '%s'", material_name, material_class ) );
		DACOM_RELEASE( (*out_material) );
		return GR_GENERIC;
	}

	add_material( material_name, *out_material );

	// NOTE: the refcount of the material is now 2 (CreateInstance + add_material)
	// NOTE: this is *correct* as the material library maintains a reference and
	// NOTE: we are supposed to increment the refcount when returning the interface
	// NOTE: via an out pointer.

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::add_material( const char *material_name, IMaterial *material )
{
	ASSERT( material_name );
	ASSERT( material );

	if( SUCCEEDED( find_material( material_name, NULL ) ) ) {
		return GR_GENERIC;
	}

	lock ();

	named_materials.insert( named_material_map::value_type( material_name, material ) );

	unlock ();
	
	material->AddRef();

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::find_material( const char *material_name, IMaterial **out_material )
{
	ASSERT( material_name );

	lock ();

	named_material_map::iterator nm ( named_materials.find( material_name ) );
	bool ck = nm == named_materials.end();

	unlock ();

	if( ck ) {
		return GR_GENERIC;
	}

	if( out_material != NULL ) {
		*out_material = nm->second;
		(*out_material)->AddRef();
	}

	return GR_OK;
}

//

GENRESULT COMAPI MaterialLibrary::remove_material( const char *material_name )
{
	ASSERT( material_name );

	GENRESULT ret;

	named_material_map::iterator nm;

	lock ();

	if( (nm = named_materials.find( material_name )) == named_materials.end() ) {
		ret = GR_GENERIC;
	}
	else
	{
		DACOM_RELEASE( nm->second );

		named_materials.erase( nm );

		ret = GR_OK;
	}

	unlock ();

	return ret;
}

//

GENRESULT MaterialLibrary::get_material_count( U32 *out_material_count ) 
{
	lock ();
	
	*out_material_count = named_materials.size();

	unlock ();

	return GR_OK;
}

//

GENRESULT MaterialLibrary::get_material( U32 num_material, IMaterial **out_material ) 
{
	GENRESULT ret = GR_GENERIC;

	lock ();

	named_material_map::iterator beg = named_materials.begin();
	named_material_map::iterator end = named_materials.end();
	named_material_map::iterator nm;	
	U32 cnt = 0;

	if( num_material >= named_materials.size() ) {
		ret = GR_GENERIC;
	}
	else
	{
		for( nm = beg; nm != end; nm++ ) {
			if( cnt == num_material ) {
				*out_material = nm->second;
				nm->second->AddRef();
				ret = GR_OK;
				break;
			}
			cnt++;
		}
	}

	unlock ();

	return ret;
}

// EOF


