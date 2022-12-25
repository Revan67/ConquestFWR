//****************************************************************************
//*                                                                          *
//*  BEZIERMESH.CPP: DA COM object-list renderer component for NURB patches    *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 22-Dec-98: Initial                                             *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <windows.h>

#include <stdio.h>
#include <limits.h>

#include "dacom.h"
#include "engine.h"
#include "3dmath.h"
#include "filesys.h"
#include "typedefs.h"
#include "stddat.h"
#include "tcomponent.h"
#include "heapobj.h"
#include "view2d.h"

#include "BezierMesh.h"
#include "SysConsumerDesc.h"

#include "TSmartPointer.h"
#include "renderer.h"
#include "ICamera.h"
#include "FDump.h"

#pragma warning( push, 3 )

//

const char *CLSID_BezierMesh = "BezierMesh";

IEngine *	ENG = NULL;

//

BEZIERMESH::BEZIERMESH(void) : render_archetypes (128), txm_lib (NULL), txm_lib_owned (false), PIPE(NULL),
							BATCH(NULL), LIGHTMAN(NULL)
{
	vertex_pool = NULL;
	normal_pool = NULL;
	normal_index_pool = NULL;
	normal_pool_cnt = NULL;
	light_pool = NULL;
	scratch_xyz = NULL;
	scratch_uv = NULL;
	scratch_xyz_rp_idx = NULL;
	scratch_xyz_uv_idx = NULL;
	vertex_pool_len = 0;
	vertex_pool_index = 0;
	scratch_xyz_index = 0;
	scratch_uv_index = 0;
	
	index_list = NULL;
	index_list_len = 0;
	index_list_index = 0;

	patch_list = NULL;
	patch_normals = NULL;
	patch_D = NULL;
	patch_corners = NULL;
	patch_depth = NULL;
	patch_alt_tri_style = NULL;
	split_direction = NULL;
	patch_list_len = 0;
	patch_list_index = 0;

	sub_div_cnt = MAX_SUB_DIV_CNT;
	pixel_error = 2.0f;
}

//

BEZIERMESH::~BEZIERMESH()
{
//
// Free archetypes & instances.
//
	unsigned int i;

	for (i = 0; i < instances.num_entries(); i++)
	{
		destroy_instance(i);
	}

	for (i = 0; i < render_archetypes.num_entries(); i++)
	{
		destroy_archetype(i);
	}

	ENG = NULL;

	pb.PrimitiveBuilder::~PrimitiveBuilder();
	
	if( PIPE ) 
	{
		PIPE->Release();
		PIPE = NULL;
	}

	if( BATCH ) 
	{
		BATCH->Release();
		BATCH = NULL;
	}

	if (txm_lib && txm_lib_owned)
	{
		txm_lib->Release ();
	}

	delete_pools();
	delete_lists();
	delete_edge_lists();
	delete_patch_lists();
}

//

bool COMAPI BEZIERMESH::create_instance (INSTANCE_INDEX  where,
                                         RENDER_ARCHETYPE index)
{
	bool result = false;

	if (INVALID_RENDER_ARCHETYPE != index)
	{
		instances[where].patch = render_archetypes[index];

		if(instances[where].patch)
		{
			ENG->set_instance_bounding_sphere(where, EN_DONT_RECURSE,
				instances[where].patch->radius, instances[where].patch->sphere_center);
		}

		result = true;
	}

	return result;
}
                               
void COMAPI BEZIERMESH::destroy_instance (INSTANCE_INDEX index)
{
	if (index != INVALID_INSTANCE_INDEX)
	{
		instances[index].free();
		instances[index].initialize();
	}
}

bool BEZIERMESH::create_archetype (RENDER_ARCHETYPE rarch, IFileSystem* fs)
{
	COMPTR<IFileSystem> nurbDir;
	DAFILEDESC fdesc = "Bezier Patch object";

	if (fs->CreateInstance(&fdesc, nurbDir) == GR_OK)
	{
		BezierMesh * patch = new BezierMesh;

		if (!patch->read (nurbDir, txm_lib))
		{
			delete patch;
			patch = NULL;
			goto done_label;
		}

done_label:
		render_archetypes[rarch] = patch;
	}


	return (NULL != (BezierMesh*)(render_archetypes[rarch]));
}

void BEZIERMESH::destroy_archetype (RENDER_ARCHETYPE rarch)
{
	if (INVALID_RENDER_ARCHETYPE != rarch && render_archetypes[rarch])
	{
		delete render_archetypes[rarch];
		render_archetypes[rarch] = NULL;
	}
}

//

vis_state BEZIERMESH::render_instance (INSTANCE_INDEX obj, RENDER_ARCHETYPE rarch, struct ICamera * camera,
									   float lod_fraction, U32 flags, const Transform *tr)
{
	ASSERT( obj != INVALID_INSTANCE_INDEX);
	ASSERT( rarch != INVALID_RENDER_ARCHETYPE);
	ASSERT( camera );

	vis_state result = VS_UNKNOWN;

	result = render(camera, obj, flags, tr, lod_fraction);
	
	return result;
}

//

void BEZIERMESH::update (float /*dt*/)
{
}

void BEZIERMESH::update_instance (INSTANCE_INDEX /*idx*/, float /*dt*/)
{
}

GENRESULT COMAPI BEZIERMESH::set_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result;

	switch(name)
	{
		case BEZIER_SUBDIV_CNT:
		{
			sub_div_cnt = Tmin<U32>(Tmax<U32>(0, value.longVal), MAX_SUB_DIV_CNT);
			result = GR_OK;
		}
		break;

		case NURB_PIXEL_ERROR:
		{
			switch( value.variantType )
			{
			case DAVT_SINGLE:
				pixel_error = (SINGLE)value;
				result = GR_OK;
			break;
			case DAVT_DOUBLE:
				pixel_error = (DOUBLE)value;
				result = GR_OK;
			break;
			}
		}
		break;
		default:
			result = GR_GENERIC;
	}

	return result;
}

GENRESULT COMAPI BEZIERMESH::get_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result = GR_GENERIC;

	return result;
}

//

bool BEZIERMESH::split_archetype (RENDER_ARCHETYPE,// idx, 
								const Vector&,// normal,
								float,// d, 
								RENDER_ARCHETYPE,// r0, 
								RENDER_ARCHETYPE,// r1,
								U32,// sa_flags,
								INSTANCE_INDEX)// i_idx)
{
#pragma message("TODO: implement split_archetype() " __FILE__  )
	GENERAL_TRACE_1("Bezier split_archetype() not yet implemented!\n");
	return false;
}

//

bool COMAPI BEZIERMESH::duplicate_archetype(RENDER_ARCHETYPE new_arch, RENDER_ARCHETYPE old_arch)
{
	BezierMesh * old_patch = render_archetypes[old_arch];

	if (old_patch)
	{
		BezierMesh * new_patch = new BezierMesh;

		new_patch->copy_bezier(*old_patch);

		render_archetypes[new_arch] = new_patch;

		return true;
	}
	else
	{
		return false;
	}
}

//

/*
bool BEZIERMESH::expand_bounding_box (RENDER_ARCHETYPE arch, float* box)
{
	bool result = false;

	if (arch != INVALID_RENDER_ARCHETYPE && render_archetypes[arch] != NULL)
		result = render_archetypes[arch]->expand_bounding_box (box);
	
	return result;
}
*/

bool COMAPI BEZIERMESH::get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] )
{
#pragma message("TODO: implement get_archetype_bounding_box() " __FILE__  )
	GENERAL_TRACE_1("Bezier get_archetype_bounding_box() not yet implemented!\n");
	return false;
}

//

bool COMAPI BEZIERMESH::get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] )
{
#pragma message("TODO: implement get_instance_bounding_box() " __FILE__  )
	GENERAL_TRACE_1("Bezier get_instance_bounding_box() not yet implemented!\n");
	return false;
}

//

bool COMAPI BEZIERMESH::get_archetype_centroid (RENDER_ARCHETYPE arch, float lod_fraction, Vector & centroid)
{
	ASSERT( arch != INVALID_RENDER_ARCHETYPE );

	bool result = false;

	BezierMesh *bmesh = render_archetypes[arch];

	if( bmesh )
	{
		centroid = bmesh->centroid;
		result = true;
	}
	
	return result;
}

//

bool COMAPI BEZIERMESH::get_archetype_statistics( RENDER_ARCHETYPE arch, float lod_fraction, enum StatType statistic,
												   DACOM_VARIANT out_value)
{
	ASSERT( arch != INVALID_RENDER_ARCHETYPE );

	BezierMesh * bm = render_archetypes[arch];

	if ( bm )
	{
		switch (statistic)
		{
			case ST_NUM_PRIMITIVES:
				if( ((unsigned long*)out_value) != NULL )
				{
					*((unsigned long*)out_value) = bm->patch_cnt;
				}
				else
				{
					GENERAL_WARNING("BEZIERMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_PRIMITIVES\n");
				}
				break;
			case ST_NUM_FACES:
				if( ((unsigned long*)out_value) != NULL )
				{
					*((unsigned long*)out_value) = bm->last_face_cnt;
				}
				else
				{
					GENERAL_WARNING("BEZIERMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_PRIMITIVES\n");
				}
				break;
			case ST_NUM_VERTICES:
				if( ((unsigned long*)out_value) != NULL )
				{
					*((unsigned long*)out_value) = bm->last_vertex_cnt;
				}
				else
				{
					GENERAL_WARNING("BEZIERMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_PRIMITIVES\n");
				}
				break;
		}

		return true;
	}
	else
	{
		return false;
	}
}

//

bool COMAPI BEZIERMESH::query_instance_interface( INSTANCE_INDEX, RENDER_ARCHETYPE, const char *, IDAComponent ** ) 
{
	return false;
}

//

bool COMAPI BEZIERMESH::query_archetype_interface( RENDER_ARCHETYPE, const char *, IDAComponent ** ) 
{
	return false;
}

//

void BezierInstance::initialize (void)
{
	patch = NULL;
	view_transform.set_identity();
}

BezierInstance::BezierInstance(void)
{
	initialize ();
}

void BezierInstance::free (void)
{
	patch = NULL; // don't free! (owned by archetype)
}

BezierInstance::~BezierInstance(void)
{
	free ();
}

// BEZIERMESH specific code
GENRESULT BEZIERMESH::init (RendCompDesc* info)
{
	ASSERT(info);

	if( info->system_services == NULL || NULL == info->engine_services ) {
		return GR_INVALID_PARMS;		// must have a system container
	}

	if( FAILED( info->system_services->QueryInterface( "IRenderPrimitive", (void**) &BATCH ) ) ) {
		GENERAL_ERROR( "BEZIERMESH: Failed to get IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	
	if( FAILED( info->system_services->QueryInterface( "IRenderPipeline", (void**) &PIPE ) ) ) {	
		GENERAL_ERROR( "BEZIERMESH: Failed to get IRenderPipeline" );
		return GR_GENERIC;
	}

//
// Get engine component resources
// 
	if( FAILED( info->engine_services->QueryInterface( IID_IEngine, (void **) &ENG ) ) ) {
		GENERAL_ERROR( "BEZIERMESH: Unable to get IEngine" );
		return GR_GENERIC;
	}
	ENG->Release();

	if( FAILED( info->system_services->QueryInterface ( IID_ITXMLib, (void**)&txm_lib ) ) ) {
		GENERAL_ERROR( "BEZIERMESH: Failed to get ITXMLib" );
		return GR_GENERIC;
	}
	txm_lib->Release();
	txm_lib_owned = false;

	if( FAILED( info->system_services->QueryInterface( IID_ILightManager, (void **) &LIGHTMAN ) ) ) {
		GENERAL_ERROR( "BEZIERMESH: Failed to get ILightManager" );
		return GR_GENERIC;
	}
	LIGHTMAN->Release();

	if(BATCH)
	{
		pb.SetIRenderPrimitive(BATCH);
	}
	else
	{
		pb.SetPipeline(PIPE);
	}

	return GR_OK;
}


//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************

void SetDllHeapMsg (HINSTANCE hInstance)
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


BOOL COMAPI DllMain(HINSTANCE hinstDLL,
                    DWORD     fdwReason,
                    LPVOID    /*lpvReserved*/)
{
   IComponentFactory *server;

   switch (fdwReason)
      {
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:

         HEAP = HEAP_Acquire();
         SetDllHeapMsg(hinstDLL);

         server = new DAComponentFactory<DAComponent<BEZIERMESH>, RendCompDesc> (CLSID_BezierMesh);

         if (server == NULL)
            {
            break;
            }

         ICOManager* DACOM = DACOM_Acquire();

         //
         // Register at environment-renderer priority
         //

         if (DACOM != NULL)
            {
            DACOM->RegisterComponent(server, 
                                     CLSID_BezierMesh, 
                                     DACOM_NORMAL_PRIORITY);
            }

         server->Release();
         break;
		}

   return TRUE;
}

#ifdef _DEBUG
#include <float.h>
void BEZIERMESH::TrapFpu(bool on)
{
	_clear87();

	if(on)
	{
		// prepare to trap floating point exceptions
		unsigned int control_word = _controlfp (0, 0);

		//control_word &= ~(EM_INVALID | EM_UNDERFLOW | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
		control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
		_control87 (control_word, MCW_EM);
	}
	else
	{
		_control87(_CW_DEFAULT, 0xfffff);
	}
}
#else
void TrapFpu(bool){}
#endif

#pragma warning( pop )