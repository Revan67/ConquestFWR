// VertexBufferManager.cpp
//
//
// TODO: add static vertex buffers
//

#pragma warning( disable: 4018 4100 4201 4512 4530 4663 4688 4710 4786 )

//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <list>
#include <map>

//

#include "DACOM.h"
#include "FDUMP.h"
#include "Tempstr.h"
#include "TSmartPointer.h"
#include "da_heap_utility.h"
#include "TComponent.h"
#include "IProfileParser_Utility.h"
#include "IVertexBufferManager.h"

//

#define CLSID_VertexBufferManager "VertexBufferManager"

//

void copy_vertex_buffer_desc( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *vb_desc );

//

// --------------------------------------------------------------------------
// VertexBufferManager
//
// 
//

struct VertexBufferManager :	IVertexBufferManager,
								IAggregateComponent

{
	BEGIN_DACOM_MAP_INBOUND(VertexBufferManager)
	DACOM_INTERFACE_ENTRY(IVertexBufferManager)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IVertexBufferManager,IVertexBufferManager)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:		// public interface
    
	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void);
	GENRESULT init( AGGDESC *desc );

	// IVertexBufferManager
	GENRESULT COMAPI initialize( const char *profile_name ) ;
	GENRESULT COMAPI cleanup( void ) ;
	GENRESULT COMAPI add_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags ) ;
	GENRESULT COMAPI remove_all_vertex_buffers( void ) ;
	GENRESULT COMAPI acquire_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags, U32 irp_lock_flags, U32 ivbm_avbf_flags, IRP_VERTEXBUFFERHANDLE *out_vbhandle, void **out_vbmemptr, U32 *out_vertex_format, U32 *out_num_verts ) ;
	GENRESULT COMAPI acquire_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags, U32 ivbm_avbf_flags, IRP_VERTEXBUFFERHANDLE *out_vbhandle, void **out_vbmemptr, U32 *out_vertex_format, U32 *out_num_verts ) ;
	GENRESULT COMAPI release_vertex_buffer( IRP_VERTEXBUFFERHANDLE vbhandle ) ;
	GENRESULT COMAPI copy_vertex_data( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_buffer ) ;

	//	
	VertexBufferManager(void);
	~VertexBufferManager(void);
	DA_HEAP_DEFINE_NEW_OPERATOR(VertexBufferManager);

protected:	// protected interface

protected:	// protected data

	//

	struct IVBM_VERTEXBUFFER
	{
		IRP_VERTEXBUFFERHANDLE handle;
		U32 vertex_format;
		U32 num_verts;
		U32 irp_vbf_flags;
	};

	//

	typedef std::map<IRP_VERTEXBUFFERHANDLE,IVBM_VERTEXBUFFER>	vertex_buffer_map;
	typedef std::list<IVBM_VERTEXBUFFER*>		vertex_buffer_list;

	//

	vertex_buffer_map	managed_vbs;
	vertex_buffer_list	available_vbs;

	//

	U32 vertex_count_granularity;
	U32 vertex_count_min;
	U32 vertex_count_max;

	U32 force_vbs_in_system_mem;

	//

	IDAComponent		*system_services;
	IRenderPipeline		*render_pipeline;
};

// 

//

VertexBufferManager::VertexBufferManager( void )
{
	system_services = NULL;
	render_pipeline = NULL;
}

//

VertexBufferManager::~VertexBufferManager( void )
{
	cleanup();
}

//

GENRESULT VertexBufferManager::init( AGGDESC *desc )
{ 
	// This is called during normal use.  We are a normal aggregate.
	// Specifically, this is a system aggregate.
	//
	system_services = desc->outer;

	return GR_OK;
}

//

GENRESULT COMAPI VertexBufferManager::Initialize(void)
{ 
	return GR_OK;
}

//

GENRESULT COMAPI VertexBufferManager::initialize( const char *profile_name ) 
{
	ASSERT( system_services );

	ICOManager *DACOM;
	char vbsection[MAX_PATH], vbdesc[MAX_PATH];
	char *p;
	COMPTR<IProfileParser> parser;
	U32 line;
	HANDLE vb;

	cleanup();

	DACOM = DACOM_Acquire();

	if( FAILED( system_services->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		return GR_GENERIC;
	}
	render_pipeline->Release();		// artificial reference

	// Read profile
	//
	if( FAILED( DACOM->QueryInterface( IID_IProfileParser, parser ) ) ) {
		return GR_GENERIC;
	}

	if( profile_name == NULL ) {
		profile_name = CLSID_VertexBufferManager;
	}

	U32 tnl = 0;

	if( FAILED( render_pipeline->query_device_ability( RP_A_DEVICE_GEOMETRY, &tnl ) ) ) {
		tnl = 0;
	}
	
	if( tnl ) {
		opt_get_u32( DACOM, parser, profile_name, "ForceSystemMemory", 0, &force_vbs_in_system_mem );
	}
	else {
		force_vbs_in_system_mem = 1;
	}

	opt_get_u32( DACOM, parser, profile_name, "VertexCountGranularity", 128, &vertex_count_granularity );
	opt_get_u32( DACOM, parser, profile_name, "VertexCountMin", 64, &vertex_count_min );
	opt_get_u32( DACOM, parser, profile_name, "VertexCountMax", 128, &vertex_count_max );

	opt_get_string( DACOM, parser, profile_name, "PreAllocate", "VertexBuffers", vbsection, MAX_PATH );
	
	for( p=vbsection; *p && *p != ' ' && *p != '\t' && *p != ';'; p++ );
	*p = 0;

	if( (vb = parser->CreateSection( vbsection )) == 0 ) {
		return GR_GENERIC;
	}

	line = 0;
	
	while( parser->ReadProfileLine( vb, line++, vbdesc, MAX_PATH ) ) {
		
		char *tok;
		U32 count, num_verts, vertex_format, flags;
		char vertex_format_string[MAX_PATH], flags_string[MAX_PATH];

		// TODO: check for comments

		// count, num_verts, vertex_format, flags
		// 1,		1024,		0x0322,		read|system			

		// count
		//
		if( (tok = strtok( vbdesc, " \t," )) == NULL ) {
			continue;
		}
		
		count = atoi( tok );

		// num_verts
		//
		if( (tok = strtok( NULL, " \t," )) == NULL ) {
			continue;
		}
		
		num_verts = atoi( tok );

		// vertex format
		//
		if( (tok = strtok( NULL, " \t," )) == NULL ) {
			continue;
		}

		strcpy( vertex_format_string, tok );

		// flags
		//
		if( (tok = strtok( NULL, " \t," )) == NULL ) {
			continue;
		}
			
		strcpy( flags_string, tok );

		// parse vertex format
		//
		vertex_format = 0;

		if( (tok = strtok( vertex_format_string, " \t|" )) != NULL ) {
			
			// P|RHW|N|CO|C1|UV0|UV1
			do {
				if( stricmp( tok, "P" ) == 0 ) {
					vertex_format |= D3DFVF_XYZ ;
				}
				else if( stricmp( tok, "RHW" ) == 0 ) {
					vertex_format &= ~(D3DFVF_XYZ);
					vertex_format |= D3DFVF_XYZRHW;		
				}
				else if( stricmp( tok, "N" ) == 0 ) {
					vertex_format |= D3DFVF_NORMAL;
				}
				else if( stricmp( tok, "C0" ) == 0 ) {
					vertex_format |= D3DFVF_DIFFUSE;
				}
				else if( stricmp( tok, "C1" ) == 0 ) {
					vertex_format |= D3DFVF_DIFFUSE;
					vertex_format |= D3DFVF_SPECULAR;	// specular implies diffuse
				}
				else if( stricmp( tok, "UV0" ) == 0 ) {
					vertex_format |= D3DFVF_TEX1;
				}
				else if( stricmp( tok, "UV1" ) == 0 ) {
					vertex_format &= ~(D3DFVF_TEX1);
					vertex_format |= D3DFVF_TEX2;		// tex2 implies tex1
				}
			}
			while( (tok = strtok( NULL, " \t|" )) != NULL ) ;
		}

		ASSERT( vertex_format != 0 );

		// parse flags
		//
		flags = 0;

		if( (tok = strtok( flags_string, " \t|" )) != NULL ) {
			
			// read | system | noclip 
			do {
				if( stricmp( tok, "READ" ) == 0 ) {
					flags |= IRP_VBF_READ;
				}
				else if( stricmp( tok, "SYSTEM" ) == 0 ) {
					flags |= IRP_VBF_SYSTEM;
				}
				else if( stricmp( tok, "NOCLIP" ) == 0 ) {
					flags |= IRP_VBF_NO_CLIP;
				}
			}
			while( (tok = strtok( NULL, " \t|" )) != NULL ) ;
		}

		for( U32 c=0; c<count; c++ ) {
			// actually create the buffer
			//
			if( FAILED( add_vertex_buffer( vertex_format, num_verts, flags ) ) ) {
				GENERAL_WARNING( TEMPSTR( "Unable to add vertex buffer in line %d of profile [%s]", line, vbsection ) );
			}
		}
	}

	parser->CloseSection( vb );

	return GR_OK; 
}

//

GENRESULT COMAPI VertexBufferManager::cleanup( void ) 
{
	remove_all_vertex_buffers();

	render_pipeline = NULL;
	
	// NOTE: do not invalidate system_services as it is used 
	// NOTE: outside of initialize/cleanup pairs

	return GR_OK;
}

//

GENRESULT VertexBufferManager::add_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags ) 
{
	IVBM_VERTEXBUFFER vb;

	if( force_vbs_in_system_mem ) {
		irp_vbf_flags |= IRP_VBF_SYSTEM ;
	}

	if( FAILED( render_pipeline->create_vertex_buffer( vertex_format, num_verts, irp_vbf_flags, &vb.handle ) ) ) {
		return GR_GENERIC;
	}

	vb.vertex_format = vertex_format;
	vb.irp_vbf_flags = irp_vbf_flags;
	vb.num_verts = num_verts;

	managed_vbs[vb.handle] = vb ;
	available_vbs.push_back( &(managed_vbs[vb.handle]) );

	return GR_OK;
}

//

GENRESULT VertexBufferManager::remove_all_vertex_buffers( void ) 
{
	vertex_buffer_map::iterator vb ;

	while( (vb = managed_vbs.begin()) != managed_vbs.end() ) {
		
		render_pipeline->destroy_vertex_buffer( vb->second.handle );
		
		managed_vbs.erase( vb );
	}

	available_vbs.clear();

	return GR_OK;
}

//

GENRESULT VertexBufferManager::acquire_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags, U32 irp_lock_flags, U32 ivbm_avbf_flags, IRP_VERTEXBUFFERHANDLE *out_vbhandle, void **out_vbmemptr, U32 *out_vertex_format, U32 *out_num_verts ) 
{
	vertex_buffer_list::iterator beg = available_vbs.begin();
	vertex_buffer_list::iterator end = available_vbs.end();
	vertex_buffer_list::iterator avb;

	IVBM_VERTEXBUFFER *vb;
	void *vbmemptr;
	U32 vf_items, vf_items_match, vf_uv, vf_uv_match, vbnum_verts;

	vf_items = vertex_format & ~(D3DFVF_TEXCOUNT_MASK);
	vf_uv = vertex_format & D3DFVF_TEXCOUNT_MASK;

	for( avb=beg; avb!=end; avb++ ) {

		vb = *avb;

		// TODO: support _EXACT
		
		vf_items_match = ((vb->vertex_format & ~(D3DFVF_TEXCOUNT_MASK)) & vf_items);
		vf_uv_match = (vb->vertex_format & D3DFVF_TEXCOUNT_MASK) == vf_uv;		
	
		if(    ((vb->irp_vbf_flags & irp_vbf_flags) == irp_vbf_flags) 
			&& (vf_items_match == vf_items) 
			&& vf_uv_match 
			&& (vb->num_verts > num_verts) ) {
			
			if( out_vbmemptr ) {

				if( FAILED( render_pipeline->lock_vertex_buffer( vb->handle, irp_lock_flags, &vbmemptr, &vbnum_verts ) ) ) {
					continue;
				}

				*out_vbmemptr = vbmemptr;
			}
			
			*out_vbhandle = vb->handle;
			*out_vertex_format = vb->vertex_format; 
			*out_num_verts = vb->num_verts;	

			available_vbs.erase( avb );
				
			return GR_OK;
		}	
	}

	if( ivbm_avbf_flags & IVBM_AVBF_NO_CREATE ) {
		return GR_GENERIC;
	}

	// could not find a vertex buffer to use 
	// -- or -- 
	// none of the available buffers were lockable.
	//
	// so we allocate a new buffer and add it to the managed list.

	IVBM_VERTEXBUFFER new_vb;

	vbnum_verts  = (num_verts / vertex_count_granularity) * vertex_count_granularity;
	vbnum_verts += (num_verts % vertex_count_granularity)? vertex_count_granularity : 0;
	
	if( vbnum_verts < vertex_count_min ) {
		vbnum_verts = vertex_count_min;
	}

	if( force_vbs_in_system_mem ) {
		irp_vbf_flags |= IRP_VBF_SYSTEM ;
	}

	if( FAILED( render_pipeline->create_vertex_buffer( vertex_format, vbnum_verts, irp_vbf_flags, &new_vb.handle ) ) ) {
		return GR_GENERIC;
	}

	if( out_vbmemptr ) {

		if( FAILED( render_pipeline->lock_vertex_buffer( new_vb.handle, irp_lock_flags, &vbmemptr, &vbnum_verts ) ) ) {
			return GR_GENERIC;
		}
		*out_vbmemptr = vbmemptr;
	}

	*out_vbhandle = new_vb.handle;
	*out_vertex_format = new_vb.vertex_format = vertex_format;
	*out_num_verts = new_vb.num_verts = vbnum_verts;
	new_vb.irp_vbf_flags = irp_vbf_flags;

	managed_vbs[new_vb.handle] = new_vb;
	
	return GR_OK;
}

//

GENRESULT VertexBufferManager::release_vertex_buffer( IRP_VERTEXBUFFERHANDLE vbhandle ) 
{
	available_vbs.push_back( &managed_vbs[vbhandle] );
	return GR_OK;
}

//

GENRESULT VertexBufferManager::copy_vertex_data( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_buffer ) 
{
	copy_vertex_buffer_desc( dst_buffer, dst_vertex_format, src_buffer );
	return GR_OK;
}

//

// linker bug
void main (void)
{
}

//

BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch( fdwReason ) {
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;

			// Register System aggragate factory
			if( DACOM && (server1 = new DAComponentFactory2<DAComponentAggregate<VertexBufferManager>, AGGDESC>(CLSID_VertexBufferManager)) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_VertexBufferManager, DACOM_NORMAL_PRIORITY );
				server1->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

// EO


