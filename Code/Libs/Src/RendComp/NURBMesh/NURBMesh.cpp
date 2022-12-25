//****************************************************************************
//*                                                                          *
//*  NURBMESH.CPP: DA COM object-list renderer component for NURB patches    *
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
//#define INITGUID

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
#include "da_heap_utility.h"
#include "view2d.h"
#include "Tfuncs.h"

#include "NURBMesh.h"

#include "SysConsumerDesc.h"

#include "TSmartPointer.h"
#include "renderer.h"
#include "ICamera.h"
#include "FDump.h"

#pragma warning( push, 3 )

//

const char *CLSID_NURBMesh = "NURBMesh";

IEngine *	ENG = NULL;

//
NURBMESH::NURBMESH(void) : txm_lib (NULL), txm_lib_owned (false), PIPE(NULL),
							BATCH(NULL), LIGHTMAN(NULL), iigloo(NULL), evaluator_nr(NULL), evaluator_r(NULL)
{
	vertex_pool_len = 0;
	vertex_pool = NULL;
	light_pool = NULL;
	
	index_list_len = 0;
	index_list = NULL;	

	visible_vertex_cnt = 0;
	visible_vertex = NULL;
	light_points = NULL;
	tmp_lights = NULL;
	light_indices = NULL;

	visible_patch_cnt = 0;
	visible_patch = NULL;

	pixel_error = 1.0f;
}

//

NURBMESH::~NURBMESH()
{
//
// Free archetypes & instances.
//
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		destroy_instance( (*inst).first );
	}

	rarch_map::iterator abeg = render_archetypes.begin();
	rarch_map::iterator aend = render_archetypes.end();
	rarch_map::iterator arch;

	for( arch=abeg; arch!=aend; arch++ ) {
		destroy_archetype( (*arch).first );
	}

	ENG = NULL;

#ifdef _DEBUG
	pb.PrimitiveBuilder::~PrimitiveBuilder();
#endif
	
	DACOM_RELEASE( PIPE );
	DACOM_RELEASE( BATCH );
	DACOM_RELEASE( LIGHTMAN );

	if( txm_lib_owned ) {
		DACOM_RELEASE( txm_lib );
	}

	for(int s=0; s < NUM_ORDERS; s++)
	{
		for(int t=0; t < NUM_ORDERS; t++)
		{
			r_polynom[s][t]->Release();
			g_polynom[s][t]->Release();
			b_polynom[s][t]->Release();
		}
	}

	if(evaluator_r)
	{
		evaluator_r->Release();
	}

	if(evaluator_nr)
	{
		evaluator_nr->Release();
	}

	if(iigloo)
	{
		iigloo->Release();
	}
	
	::CoUninitialize();

	delete_pools();
	delete_lists();

	delete [] visible_vertex;
	delete [] light_points;
	delete [] tmp_lights;
	delete [] light_indices;

	delete [] visible_patch;
}

//

bool COMAPI NURBMESH::create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );


	rarch_map::iterator rarch;
	NURBInstance *instance;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( (instance = new NURBInstance()) == NULL ) {
		return false;
	}

	if( (instance->xnurb = (*rarch).second) == NULL ) {
		delete instance;
		return false;
	}

	instance->ieval_r = evaluator_r;
	instance->ieval_nr = evaluator_nr;
#if USE_CACHE
	instance->AssignCacheHandles();
#endif
	ENG->set_instance_bounding_sphere( inst_index, EN_DONT_RECURSE, instance->xnurb->radius, instance->xnurb->sphere_center );

	if( instances.insert( inst_index, instance ) == instances.end() ) {
		delete instance;
		return false;
	}

	return true;
}
                               
void COMAPI NURBMESH::destroy_instance (INSTANCE_INDEX index)
{
	ASSERT( index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;

	if( (inst = instances.find( index )) != instances.end() ) {
		inst->second->free();
		inst->second->initialize();
	}
}

bool NURBMESH::create_archetype (RENDER_ARCHETYPE rarch, IFileSystem* fs)
{
	ASSERT(fs);

	COMPTR<IFileSystem> nurbDir;
	DAFILEDESC fdesc = "NURB object";
	XNURB * xn = NULL;

	if (fs->CreateInstance(&fdesc, nurbDir) == GR_OK)
	{
		xn = new XNURB;

		if (!xn->read (nurbDir, txm_lib))
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

		if(!xn->InitBasis())
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

		if(!xn->InitPolys(poly_data_user, poly_data_linear))
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

		if(!xn->AssignBasis())
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

		if(!xn->AssignData())
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

#if CHORD_NORMALIZE
		GenerateUVs(xn);
#endif

		if(!xn->AssignUVData())
		{
			delete xn;
			xn = NULL;
			goto done_label;
		}

		GenerateNormals(xn);
	}

done_label:

	if( xn ) {
		render_archetypes.insert( rarch, xn );
		return true;
	}

	return false;
}

void NURBMESH::destroy_archetype (RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) != render_archetypes.end() ) {
		delete rarch->second;
		render_archetypes.erase( render_arch_index );
	}
}

//
vis_state NURBMESH::render_instance (INSTANCE_INDEX obj, RENDER_ARCHETYPE rarch,
									 struct ICamera *camera, float lod_fraction, U32 flags,
									 const Transform *tr)
{
	ASSERT( obj != INVALID_INSTANCE_INDEX );
	ASSERT( rarch != INVALID_RENDER_ARCHETYPE );
	ASSERT( camera != NULL );

	vis_state result = VS_UNKNOWN;

	result = render(camera, obj, flags, tr, lod_fraction);
	
	return result;
}

//

void NURBMESH::update (float /*dt*/)
{
}

void NURBMESH::update_instance (INSTANCE_INDEX /*idx*/, float /*dt*/)
{
}

GENRESULT COMAPI NURBMESH::set_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result = GR_NOT_IMPLEMENTED;

	switch(name)
	{
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

GENRESULT COMAPI NURBMESH::get_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result = GR_NOT_IMPLEMENTED;

	switch(name)
	{
		case NURB_PIXEL_ERROR:
		{
			switch( value.variantType )
			{
				case DAVT_PSINGLE:
					*(SINGLE*)value = pixel_error;
					result = GR_OK;
				break;
				case DAVT_PDOUBLE:
					*(DOUBLE*)value = pixel_error;
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

bool NURBMESH::split_archetype (RENDER_ARCHETYPE,// idx, 
								const Vector&,// normal,
								float,// d, 
								RENDER_ARCHETYPE,// r0, 
								RENDER_ARCHETYPE,// r1,
								U32,//sa_flags,
								INSTANCE_INDEX)// i_idx)
{
#pragma message("TODO: implement split_archetype() " __FILE__  )
	GENERAL_TRACE_1("NURB split_archetype() not yet implemented!\n");
	return false;
}

bool COMAPI NURBMESH::get_archetype_statistics( RENDER_ARCHETYPE render_arch_index,
												float lod_fraction,
												enum StatType statistic,
												DACOM_VARIANT out_value )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction  < 1.1f );

	const XNURB * nurb;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;	
	}

	nurb = rarch->second;

	switch( statistic )
	{
		case ST_NUM_PRIMITIVES:
			if( ((unsigned long*)out_value) != NULL )
			{
				*((unsigned long*)out_value) = (unsigned long)nurb->patch_cnt;
				return true;
			}
			else
			{
				GENERAL_WARNING("NURBMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_PRIMITIVES\n");
			}
			break;
		case ST_NUM_FACES:
			if( ((unsigned long*)out_value) != NULL )
			{
				*((unsigned long*)out_value) = (unsigned long)nurb->last_face_cnt;
				return true;
			}
			else
			{
				GENERAL_WARNING("NURBMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_FACES\n");
			}
			break;
		case ST_NUM_VERTICES:
			if( ((unsigned long*)out_value) != NULL )
			{
				*((unsigned long*)out_value) = (unsigned long)nurb->last_vertex_cnt;
				return true;
			}
			else
			{
				GENERAL_WARNING("NURBMESH::get_archetype_statistics(): Incomatible DACOM_VARIANT for ST_NUM_VERTICES\n");
			}
			break;
	}

	return true;
}

bool COMAPI NURBMESH::get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index,
												 float lod_fraction,
												 SINGLE out_box[6] )
{
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( out_box );

	bool result = false;

	const XNURB * nurb;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;	
	}

	nurb = rarch->second;
	
	if( nurb )
	{
		nurb->get_bounding_box(out_box);
		result = true;
	}
	return result;
}

bool COMAPI NURBMESH::get_instance_bounding_box(
			INSTANCE_INDEX inst_index,
			RENDER_ARCHETYPE render_arch_index,
			float lod_fraction,
			SINGLE out_box[6])
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( out_box );

	bool result = false;

	if (render_arch_index != INVALID_RENDER_ARCHETYPE)
	{
		const XNURB * nurb;
		rarch_map::iterator rarch;
		
		if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
			return false;	
		}

		nurb = rarch->second;
		if (nurb)
		{			
			nurb->get_bounding_box(out_box);
			
			result = true;
		}
	}

	return result;
}

bool COMAPI NURBMESH::get_archetype_centroid( RENDER_ARCHETYPE render_arch_index,
											 float lod_fraction,
											 Vector& out_centroid )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	bool result = false;

	const XNURB * nurb;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;	
	}

	nurb = rarch->second;
	if( nurb )
	{
		out_centroid = nurb->centroid;

		result = true;
	}
	
	return result;
}

//

bool COMAPI NURBMESH::duplicate_archetype(RENDER_ARCHETYPE new_arch, RENDER_ARCHETYPE old_arch)
{
	ASSERT( new_arch != INVALID_RENDER_ARCHETYPE );
	ASSERT( old_arch != INVALID_RENDER_ARCHETYPE );

	XNURB * old_nurb ;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( old_arch )) == render_archetypes.end() ) {
		return false;	
	}

	old_nurb = rarch->second;

	if (old_nurb)
	{
		XNURB * new_nurb = new XNURB;

		new_nurb->copy_nurb(*old_nurb);
		
#if 1	// NOTE: InitBasis() could be skipped by copying them but then the archetype would
		// not be unique as far as IIGLOO is concerned (same goes for some of the below) 
		if(!new_nurb->InitBasis())
		{
			delete new_nurb;
			new_nurb = NULL;
			return false;
		}
#endif

		if(!new_nurb->InitPolys(poly_data_user, poly_data_linear))
		{
			delete new_nurb;
			new_nurb = NULL;
			return false;
		}

		if(!new_nurb->AssignBasis())
		{
			delete new_nurb;
			new_nurb = NULL;
			return false;
		}

		if(!new_nurb->AssignData())
		{
			delete new_nurb;
			new_nurb = NULL;
			return false;
		}

#if CHORD_NORMALIZE
		GenerateUVs(new_nurb);
#endif

		if(!new_nurb->AssignUVData())
		{
			delete new_nurb;
			new_nurb = NULL;
			return false;
		}

		GenerateNormals(new_nurb);

		render_archetypes.insert( new_arch, new_nurb );
		return true;
	}

	return false;
}

//

void NURBInstance::initialize (void)
{
	ieval_r = NULL;
	ieval_nr = NULL;
	xnurb = NULL;
	view_transform.set_identity();
	iicache_hangle_list = NULL;
}

NURBInstance::NURBInstance(void)
{
	initialize ();
}

void NURBInstance::free (void)
{
#if USE_CACHE
	if(iicache_hangle_list)
	{
		for(int i = 0; i < xnurb->patch_cnt; i++)
		{
			const NURBPatch & patch = xnurb->patch_list[i];
			for(int s = 0; s < patch.s_basis_cnt; s++)
			{
				for(int t = 0; t < patch.t_basis_cnt; t++)
				{
					if(patch.weight_list)
					{
						ii_result( 
							ieval_r->Release_Evaluation_Handle(
							iicache_hangle_list[i][s * patch.t_basis_cnt + t] ) );
					}
					else
					{
						ii_result( 
							ieval_nr->Release_Evaluation_Handle(
							iicache_hangle_list[i][s * patch.t_basis_cnt + t] ) );
					}
				}
			}
			delete [] iicache_hangle_list[i];
			iicache_hangle_list[i] = NULL;
		}

		delete [] iicache_hangle_list;
	}
#endif

	xnurb = NULL; // don't free! (owned by archetype)
	ieval_r = NULL;
	ieval_nr = NULL;
}

NURBInstance::~NURBInstance(void)
{
	free ();
}

#if USE_CACHE
void NURBInstance::AssignCacheHandles(void)
{
	ASSERT(xnurb);
	ASSERT(iicache_hangle_list == NULL);

	iicache_hangle_list = new IIEvaluationHandle* [xnurb->patch_cnt];

	for(int i = 0; i < xnurb->patch_cnt; i++)
	{
		NURBPatch & patch = xnurb->patch_list[i];

		iicache_hangle_list[i] = new IIEvaluationHandle[patch.s_basis_cnt * patch.t_basis_cnt];

		//SetPatchEvaluatorState(patch);

		IIUInt		row = 0;
		IIUInt		point_step;
		for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
		{
			patch.s_basis_list[s_basis]->Get_Control_Point_Step( &point_step );
			row += point_step;

			IIUInt column = 0;

			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				patch.t_basis_list[t_basis]->Get_Control_Point_Step( &point_step );
				column += point_step;

				/*
				// assign basis to polynomials
				AssignBasisToPolynomials(patch, s_basis, t_basis );

				AssignDataToPolynomials(patch, row, column, s_basis, t_basis, UV_ON | LIGHT_ON);

				// assign polynomials to evaluator
				AssignPolynomialsToEvaluator(patch, UV_ON | LIGHT_ON);
				*/
	
				if(patch.weight_list)
				{
					ii_result( ieval_r->Get_New_Evaluation_Handle( 
						&(iicache_hangle_list[i][s_basis * patch.t_basis_cnt + t_basis]) ) );
				}
				else
				{
					ii_result( ieval_nr->Get_New_Evaluation_Handle( 
						&(iicache_hangle_list[i][s_basis * patch.t_basis_cnt + t_basis]) ) );
				}
			}
		}
	}
}
#endif

//

NURB * NURBMESH::get_archetype_NURB(RENDER_ARCHETYPE rarch_index)
{
	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( rarch_index )) == render_archetypes.end() ) {
		return NULL;
	}	

	return rarch->second;
}

//

NURB * NURBMESH::get_instance_NURB(INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index)
{
	inst_map::iterator inst;

	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return NULL;
	}	

	return inst->second->xnurb;
}


// NURBMESH specific code
GENRESULT NURBMESH::init (RendCompDesc* info)
{
	ASSERT(info);

	if( info->system_services == NULL || NULL == info->engine_services ) {
		return GR_INVALID_PARMS;		// must have a system container
	}

	if( FAILED( info->system_services->QueryInterface( IID_IRenderPrimitive, (void**) &BATCH ) ) ) {
		GENERAL_ERROR( "NURBMESH: Failed to get IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	
	if( FAILED( info->system_services->QueryInterface( IID_IRenderPipeline, (void**) &PIPE ) ) ) {	
		GENERAL_ERROR( "NURBMESH: Failed to get IRenderPipeline" );
		return GR_GENERIC;
	}

//
// Get engine component resources
// 
	if( FAILED( info->engine_services->QueryInterface(IID_IEngine, (void **) &ENG ) ) ) {
		GENERAL_ERROR( "NURBMESH: Unable to get IEngine" );
		return GR_GENERIC;
	}
	ENG->Release();

	if( FAILED( info->system_services->QueryInterface ( IID_ITextureLibrary, (void**)&txm_lib ) ) ) {
		GENERAL_ERROR( "NURBMESH: Failed to get ITextureLibrary" );
		return GR_GENERIC;
	}
	
	txm_lib_owned = true;

	if( FAILED( info->system_services->QueryInterface( IID_ILightManager, (void **) &LIGHTMAN ) ) ) {
		GENERAL_ERROR( "NURBMESH: Failed to get ILightManager" );	
	}
	

#ifdef _DEBUG
	if(BATCH)
	{
		pb.SetIRenderPrimitive(BATCH);
	}
	else
	{
		pb.SetPipeline(PIPE);
	}
#endif

//
// IIGLOO stuff
//

	HRESULT hr = CoInitialize(NULL);
	if(hr != S_OK && hr != S_FALSE)
	{
		GENERAL_FATAL("Could NOT initialize MS COM!\n");
		return GR_GENERIC;
	}
	else
	if(hr == S_OK)
	{
		GENERAL_WARNING("The app should call CoInitialize() before using nurbmesh.dll!");
	}

	hr = ::CoCreateInstance( CLSID_IIGLOO,
							 NULL,
							 CLSCTX_INPROC_SERVER,
							 IID_IIGLOO,
							 (void**)&iigloo);
	if( FAILED(hr) )
	{
		GENERAL_FATAL("Could NOT find IIGLOO!\n");
		return GR_GENERIC;
	}

	evaluator_r = GetEvaluator(iigloo);
	if( evaluator_r == NULL)
	{
		return GR_GENERIC;
	}

	evaluator_nr = GetEvaluator(iigloo);
	if( evaluator_nr == NULL)
	{
		return GR_GENERIC;
	}

	// create dest tessellation buffers
	IIUInt		min_vertex_count, min_index_count;
	evaluator_r->Get_Suggested_Tessellation_Output_Array_Size(&min_vertex_count, &min_index_count);
	//min_vertex_count = _MIN<IIUInt>(1536, min_vertex_count);
	
	verify_pools(min_vertex_count);
	verify_lists(min_index_count);

	ActivateEvaluator(NON_RATIONAL);
	SetEvaluatorDefaults();

	ActivateEvaluator(RATIONAL);
	SetEvaluatorDefaults();


	if( PolyEnumData() == false )
	{
		return GR_GENERIC;
	}

	if( InitPolynomials() == false )
	{
		return GR_GENERIC;
	}

	return GR_OK;
}

bool NURBMESH::PolyEnumData(void)
{
	for(int i = 0; i < NUM_ORDERS; i++)
	{
		for(int j = 0; j < NUM_ORDERS; j++)
		{
			// setup the surface polynomials for geometry
			poly_data_user[j * NUM_ORDERS + i].poly_found = false;
			poly_data_user[j * NUM_ORDERS + i].s_type = IIGLOO_USER_CURVE_TYPE;
			poly_data_user[j * NUM_ORDERS + i].t_type = IIGLOO_USER_CURVE_TYPE;
			poly_data_user[j * NUM_ORDERS + i].s_order = Order_From_Count(LOWEST_ORDER + j);
			poly_data_user[j * NUM_ORDERS + i].t_order = Order_From_Count(LOWEST_ORDER + i);

			iigloo->Enumerate_Polynomials( PolynomialEnumerator, &poly_data_user[j * NUM_ORDERS + i] );

			if ( !poly_data_user[j * NUM_ORDERS + i].poly_found )
			{
				return false;
			}
		}
	}
		

	// setup the surface polynomials for UV
	poly_data_linear.poly_found = false;
	poly_data_linear.s_type = IIGLOO_LINEAR;
	poly_data_linear.t_type = IIGLOO_LINEAR;
	poly_data_linear.s_order = IIGLOO_POLYNOMIAL_DEGREE_1;
	poly_data_linear.t_order = IIGLOO_POLYNOMIAL_DEGREE_1;

	iigloo->Enumerate_Polynomials( PolynomialEnumerator, &poly_data_linear );

	if ( !poly_data_linear.poly_found )
	{
		return false;
	}

	return true;
}

void NURBMESH::ActivateEvaluator(const ev_type type)
{
	if(type == RATIONAL)
	{
		evaluator = evaluator_r;
	}
	else
	{
		evaluator = evaluator_nr;
	}

	active_type = type;
}

// used no matter what kind of patch we tessellate
void NURBMESH::SetEvaluatorDefaults(void)
{
	// set evaluator state
	ii_result( evaluator->Unlock_State() );

		// needed for lock to succeed (why???)
		if(active_type == RATIONAL)
			evaluator->Set_Type_State( IIGLOO_RATIONAL_GEOMETRY_DATA, IIGLOO_POLYNOMIAL_ORDER_4, IIGLOO_POLYNOMIAL_ORDER_4 );
		else
			evaluator->Set_Type_State( IIGLOO_GEOMETRY_DATA, IIGLOO_POLYNOMIAL_ORDER_4, IIGLOO_POLYNOMIAL_ORDER_4 );

		evaluator->Enable( IIGLOO_INDEXED_TESSELLATION_OUTPUT );
		evaluator->Disable( IIGLOO_NORMAL_GENERATION );
		evaluator->Disable( IIGLOO_NORMAL_NORMALIZATION );
#if USE_CACHE
		ii_result( evaluator->Enable( IIGLOO_EVALUATION_CACHING ) );
#endif
	ii_result( evaluator->Lock_State() );


	// XYZ
	evaluator->Set_Tessellation_Output_Array_Size( vertex_pool_len );

	if(active_type == RATIONAL)
	{
#if !DIVIDE_BY_W
		evaluator->Set_Tessellation_Output_Array( IIGLOO_RATIONAL_GEOMETRY_DATA,
			&(vertex_pool[0].pos.x), sizeof(RPVertex) );
#else
		evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, 
			&(vertex_pool[0].pos.x), sizeof(RPVertex) );
#endif
	}
	else
	{
		evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, 
			&(vertex_pool[0].pos.x), sizeof(RPVertex) );
	}

	evaluator->Set_Tessellation_Output_Array( IIGLOO_VERTEX_INDEX_DATA, index_list, 0 );

	// UV
	evaluator->Set_Tessellation_Output_Array( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
										&(vertex_pool[0].u), sizeof(RPVertex) );

	// LIGHT
	evaluator->Set_Tessellation_Output_Array( IIGLOO_COLOR_DATA, 
			&(light_pool[0]), sizeof(Vector));


	ii_result( evaluator->Unlock_State() );
		ii_result( evaluator->Set_Value( IIGLOO_MAXIMUM_TESSELLATION, 32 ) ); // default is 64
	ii_result( evaluator->Lock_State() );

}

void NURBMESH::SetPatchEvaluatorState(const NURBPatch & patch)
{
	ASSERT( (active_type == RATIONAL && patch.weight_list) ||
			(active_type == NON_RATIONAL && !patch.weight_list) );


	ii_result( evaluator->Unlock_State() );

		// XYZ
		if(active_type == RATIONAL)
		{
#if DIVIDE_BY_W
			ii_result( evaluator->Enable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
#endif
			evaluator->Set_Type_State( IIGLOO_RATIONAL_GEOMETRY_DATA,
				Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) );
		}
		else
		{
#if DIVIDE_BY_W
			//ii_result( evaluator->Disable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
#endif
			evaluator->Set_Type_State( IIGLOO_GEOMETRY_DATA,
				Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) );
			//evaluator->Set_Name_State( IIGLOO_W, IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );	
		}

		// UV
		if(patch.uv_list)
		{
			evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
										IIGLOO_POLYNOMIAL_ORDER_2, IIGLOO_POLYNOMIAL_ORDER_2);
		}
		else
		{
			evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
										IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE);	
		}					  

		// LIGHT
		evaluator->Set_Type_State( IIGLOO_COLOR_DATA,
			Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) );

	ii_result( evaluator->Lock_State() );
}

// add a pre evaluator and split evaluator into w/ & w/o w ??
void NURBMESH::GenerateNormals(XNURB *xnurb)
{
	ASSERT(xnurb);
	//TrapFpu(true);

	float vert[4];
	float norm[4];
	Vector & vertex = *(Vector*)vert;
	Vector & normal = *(Vector*)norm;

	ActivateEvaluator(RATIONAL);
	evaluator->Unlock_State();
		evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Set_Type_State( IIGLOO_COLOR_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Disable( IIGLOO_INDEXED_TESSELLATION_OUTPUT );
		evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );

		evaluator->Enable( IIGLOO_NORMAL_GENERATION );
		//evaluator->Enable( IIGLOO_NORMAL_NORMALIZATION );
		evaluator->Disable( IIGLOO_NORMAL_NORMALIZATION );
		evaluator->Set_Tessellation_Output_Array( IIGLOO_NORMAL_DATA, &(normal.x), 0 );
	ii_result( evaluator->Lock_State() );

	ActivateEvaluator(NON_RATIONAL);
	evaluator->Unlock_State();
		evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Set_Type_State( IIGLOO_COLOR_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Disable( IIGLOO_INDEXED_TESSELLATION_OUTPUT );
		evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );

		evaluator->Enable( IIGLOO_NORMAL_GENERATION );
		//evaluator->Enable( IIGLOO_NORMAL_NORMALIZATION );
		evaluator->Disable( IIGLOO_NORMAL_NORMALIZATION );
		evaluator->Set_Tessellation_Output_Array( IIGLOO_NORMAL_DATA, &(normal.x), 0 );
	ii_result( evaluator->Lock_State() );
	
	for(int i = 0; i < xnurb->patch_cnt; i++)
	{
		NURBPatch & patch = xnurb->patch_list[i];

		if(patch.weight_list)
			ActivateEvaluator(RATIONAL);
		else
			ActivateEvaluator(NON_RATIONAL);

		patch.s_vertex_cnt = patch.s_basis_cnt * (patch.s_order - 1) + 1;
		patch.t_vertex_cnt = patch.t_basis_cnt * (patch.t_order - 1) + 1;
		patch.normals = new Vector[patch.s_vertex_cnt * patch.t_vertex_cnt];
		patch.vertices = new Vector[patch.s_vertex_cnt * patch.t_vertex_cnt];
		patch.D_coefficient = new float[patch.s_vertex_cnt * patch.t_vertex_cnt];

		ii_result( evaluator->Unlock_State() );
			evaluator->Set_Tessellation_Output_Array_Size( 1 );
			if(active_type == RATIONAL)
			{
#if DIVIDE_BY_W
				ii_result( evaluator->Enable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
				evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, &(vertex.x), 0 );
#else
				evaluator->Set_Tessellation_Output_Array( IIGLOO_RATIONAL_GEOMETRY_DATA, &(vertex.x), 0 );
#endif
				ii_result( evaluator->Set_Type_State( IIGLOO_RATIONAL_GEOMETRY_DATA,
					Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) ) );
			}
			else
			{
#if DIVIDE_BY_W
				//ii_result( evaluator->Disable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
#endif
				evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, &(vertex.x), 0 );
				ii_result( evaluator->Set_Type_State( IIGLOO_GEOMETRY_DATA,
					Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) ) );
				//ii_result( evaluator->Set_Name_State( IIGLOO_W, IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE ) );
			}
		ii_result( evaluator->Lock_State() );

		// mark all normals so we can test which normals were already generated
		for(int tmpi=0; tmpi < patch.s_vertex_cnt * patch.t_vertex_cnt; tmpi++)
		{
			patch.normals[tmpi] = Vector(10, 10, 10);
		}

		IIUInt		row = 0;
		IIUInt		point_step;
		for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
		{
			patch.s_basis_list[s_basis]->Get_Control_Point_Step( &point_step );
			row += point_step;

			IIUInt column = 0;

			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				patch.t_basis_list[t_basis]->Get_Control_Point_Step( &point_step );
				column += point_step;

				// assign basis to polynomials
				AssignDataToPolynomials(patch, row, column, s_basis, t_basis, 0);

				// assign polynomials to evaluator
				AssignPolynomialsToEvaluator(patch, s_basis, t_basis, 0);

				float t = 0.0f, s= 0.0f;
				float s_step = 1.0 / (patch.s_order - 1);
				float t_step = 1.0 / (patch.t_order - 1);
				for(int s_pos = 0; s_pos < patch.s_order; s_pos++)
				{
					int cv_s =  ((patch.s_order - 1) * s_basis) + s_pos;
					for(int t_pos = 0; t_pos < patch.t_order; t_pos++)
					{
						int cv_t = ((patch.t_order - 1) * t_basis) + t_pos;

						HRESULT err = evaluator->Evaluate_Point(
							Tmin(Tmax(.001f, s), .999f),
							Tmin(Tmax(.001f, t), .999f));

						if ( SUCCEEDED(err) )
						{
							ASSERT( !_isnan(vertex.x) && !_isnan(vertex.y) && !_isnan(vertex.z));
							ASSERT( !_isnan(normal.x) && !_isnan(normal.y) && !_isnan(normal.z));

							const float mag = normal.magnitude();
							if( mag > 0.0f)
							{
								normal /= mag;
							}
							else
							{
								GENERAL_WARNING("Zero length NURB normal!");
							}
							
							//TrapFpu(true);
							// average values at seams
							if( patch.normals[cv_s * patch.t_vertex_cnt + cv_t].x != 10.0f ||
								patch.normals[cv_s * patch.t_vertex_cnt + cv_t].y != 10.0f ||
								patch.normals[cv_s * patch.t_vertex_cnt + cv_t].z != 10.0f)
							{
#ifdef _DEBUG
								float dot = patch.normals[cv_s * patch.t_vertex_cnt + cv_t].x * normal.x +
											patch.normals[cv_s * patch.t_vertex_cnt + cv_t].y * normal.y +
											patch.normals[cv_s * patch.t_vertex_cnt + cv_t].z * normal.z;

								if( dot < .995f ) // ~5deg
								{
									int iii=0;
								}

								Vector tmpv( patch.vertices[cv_s * patch.t_vertex_cnt + cv_t] -
											 vertex);

								float mag = tmpv.magnitude();
								if(mag > .0001)
								{
									int iii=0;
								}
#endif
								patch.normals[cv_s * patch.t_vertex_cnt + cv_t] += normal;
								patch.normals[cv_s * patch.t_vertex_cnt + cv_t].normalize();

								patch.vertices[cv_s * patch.t_vertex_cnt + cv_t] += vertex;
								patch.vertices[cv_s * patch.t_vertex_cnt + cv_t] *= .5f;

								patch.D_coefficient[cv_s * patch.t_vertex_cnt + cv_t] +=
									-dot_product(normal, vertex);
								patch.D_coefficient[cv_s * patch.t_vertex_cnt + cv_t] *= .5f;
							}
							else
							{
								patch.normals[cv_s * patch.t_vertex_cnt + cv_t] = normal;
								patch.vertices[cv_s * patch.t_vertex_cnt + cv_t] = vertex;
								patch.D_coefficient[cv_s * patch.t_vertex_cnt + cv_t] =
									-dot_product(normal, vertex);
							}
							//TrapFpu(false);

							t += t_step;
						}
						else
						{
							long tmp_err = (err & 0xffff) - 0x8000;
							// evaluation failed
							GENERAL_FATAL("IIGLOO evaluation failed!\n"); // IIGLOOERROR_IO_DATA_UNAVAILABLE
						}
					}
					s += s_step;
					t = 0.0f;
				}
			}
		}

		// make sure the visibility array is big enough for this patch
		if(visible_vertex_cnt < patch.s_vertex_cnt * patch.t_vertex_cnt)
		{
			visible_vertex_cnt = patch.s_vertex_cnt * patch.t_vertex_cnt;
			delete [] visible_vertex;
			visible_vertex = new bool[visible_vertex_cnt];
			delete [] light_points;
			light_points = new f_LightRGB[visible_vertex_cnt];
			delete [] tmp_lights;
			tmp_lights = new LightRGB[visible_vertex_cnt];
			delete [] light_indices;
			light_indices = new U32[visible_vertex_cnt];
			// can go away if light_vertices supports non indexed input
			for(int id = 0; id < visible_vertex_cnt; id++)
			{
				light_indices[id] = id;
			}
		}

		if(visible_patch_cnt < patch.s_basis_cnt * patch.t_basis_cnt)
		{
			visible_patch_cnt = patch.s_basis_cnt * patch.t_basis_cnt;
			delete [] visible_patch;
			visible_patch = new vis_state[visible_patch_cnt];
		}
	}

	TrapFpu(false);

	ActivateEvaluator(RATIONAL);
	SetEvaluatorDefaults();

	ActivateEvaluator(NON_RATIONAL);
	SetEvaluatorDefaults();
}

#if CHORD_NORMALIZE

struct Length
{
	const int length_cnt;		// uv_cnt / number of paths
	const int sub_length_cnt;	// basis_cnt / number of chunks each path is split into

	float *list;

	inline Length(const int in_length_cnt, const int in_sub_length_cnt) :
			length_cnt(in_length_cnt), sub_length_cnt(in_sub_length_cnt)
	{
		list = new float[length_cnt * sub_length_cnt];
		memset(list, 0, length_cnt * sub_length_cnt * sizeof(float));
	}

	//Length(void) : length_cnt(0), sub_length_cnt(0) {}

	~Length()
	{
		delete [] list;
	}

	inline float* operator [] (const int i)
	{
		return list + i * sub_length_cnt;
	}
};

void NURBMESH::GenerateUVs(XNURB *xnurb)
{
	//TrapFpu(true);

	float vert[4];
	Vector & vertex = *(Vector*)vert;

	ActivateEvaluator(RATIONAL);
	evaluator->Unlock_State();
		evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Set_Type_State( IIGLOO_COLOR_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Disable( IIGLOO_INDEXED_TESSELLATION_OUTPUT );
		evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
	ii_result( evaluator->Lock_State() );

	ActivateEvaluator(NON_RATIONAL);
	evaluator->Unlock_State();
		evaluator->Set_Type_State( IIGLOO_TEXTURE_2D_COORDINATE1_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Set_Type_State( IIGLOO_COLOR_DATA,
								   IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE );
		evaluator->Disable( IIGLOO_INDEXED_TESSELLATION_OUTPUT );
		evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
	ii_result( evaluator->Lock_State() );
	
	for(int i = 0; i < xnurb->patch_cnt; i++)
	{
		NURBPatch & patch = xnurb->patch_list[i];

		if(patch.weight_list)
			ActivateEvaluator(RATIONAL);
		else
			ActivateEvaluator(NON_RATIONAL);

		// used for UV chord length normalization
		ASSERT_FATAL (patch.u_cnt == patch.s_basis_cnt + 1);
		ASSERT_FATAL (patch.v_cnt == patch.t_basis_cnt + 1);
		Length vs_lengths(patch.v_cnt, patch.s_basis_cnt + 1);
		Length ut_lengths(patch.u_cnt, patch.t_basis_cnt + 1);

		ii_result( evaluator->Unlock_State() );
			evaluator->Set_Tessellation_Output_Array_Size( 1 );
			if(active_type == RATIONAL)
			{
#if DIVIDE_BY_W
				ii_result( evaluator->Enable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
				evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, &(vertex.x), 0 );
#else
				evaluator->Set_Tessellation_Output_Array( IIGLOO_RATIONAL_GEOMETRY_DATA, &(vertex.x), 0 );
#endif
				ii_result( evaluator->Set_Type_State( IIGLOO_RATIONAL_GEOMETRY_DATA,
					Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) ) );
			}
			else
			{
#if DIVIDE_BY_W
				//ii_result( evaluator->Disable( IIGLOO_GEOMETRY_HOMOGENEOUS_DIVISION ) );
#endif
				evaluator->Set_Tessellation_Output_Array( IIGLOO_GEOMETRY_DATA, &(vertex.x), 0 );
				ii_result( evaluator->Set_Type_State( IIGLOO_GEOMETRY_DATA,
					Order_From_Count(patch.s_order), Order_From_Count(patch.t_order) ) );
				//ii_result( evaluator->Set_Name_State( IIGLOO_W, IIGLOO_POLYNOMIAL_ORDER_NONE, IIGLOO_POLYNOMIAL_ORDER_NONE ) );
			}
		ii_result( evaluator->Lock_State() );


		IIUInt		row = 0;
		IIUInt		point_step;
		for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
		{
			patch.s_basis_list[s_basis]->Get_Control_Point_Step( &point_step );
			row += point_step;

			IIUInt column = 0;

			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				patch.t_basis_list[t_basis]->Get_Control_Point_Step( &point_step );
				column += point_step;

				// assign basis to polynomials
				AssignDataToPolynomials(patch, row, column, s_basis, t_basis, 0);

				// assign polynomials to evaluator
				AssignPolynomialsToEvaluator(patch, s_basis, t_basis, 0);

				float t = 0.0f, s= 0.0f;
				float s_step = 1.0 / (patch.s_order - 1);
				float t_step = 1.0 / (patch.t_order - 1);
			
				// generate a fine tesselation so that UV's can be chord lenght normalized
				const int N = 64;
				s_step = 1.0 / (N - 1);
				t_step = 1.0 / (N - 1);
				Vector last_v;

				s = 0.0f;
				t = 0.001f;
				evaluator->Evaluate_Point(s, t);
				last_v = vertex;
				s = s_step;
				for(int sp = 1; sp < N; sp++)
				{
					evaluator->Evaluate_Point(s, t);
					vs_lengths[t_basis][s_basis] += (vertex - last_v).magnitude(); 
			
					last_v = vertex;
					s += s_step;
				}

				s = 0.0f;
				t = 0.999f;
				evaluator->Evaluate_Point(s, t);
				last_v = vertex;
				s = s_step;
				for(sp = 1; sp < N; sp++)
				{
					evaluator->Evaluate_Point(s, t);
					vs_lengths[t_basis+1][s_basis] += (vertex - last_v).magnitude(); 

					last_v = vertex;
					s += s_step;
				}

				s = 0.001f;
				t = 0.0f;
				evaluator->Evaluate_Point(s, t);
				last_v = vertex;
				t = t_step;
				for(int st = 1; st < N; st++)
				{
					evaluator->Evaluate_Point(s, t);
					ut_lengths[s_basis][t_basis] += (vertex - last_v).magnitude(); 

					last_v = vertex;
					t += t_step;
				}

				s = 0.999f;
				t = 0.0f;
				evaluator->Evaluate_Point(s, t);
				last_v = vertex;
				t = t_step;
				for(st = 1; st < N; st++)
				{
					evaluator->Evaluate_Point(s, t);
					ut_lengths[s_basis+1][t_basis] += (vertex - last_v).magnitude(); 

					last_v = vertex;
					t += t_step;
				}
			}
		}

		// average out interior boundaries since they were computed twice
		for ( int vv = 1; vv < patch.v_cnt - 1; vv++ )
		{
			for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
			{
				vs_lengths[vv][s_basis] *= .5f;
			}
		}

		for ( int uu = 1; uu < patch.u_cnt - 1; uu++ )
		{
			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				ut_lengths[uu][t_basis] *= .5f;
			}
		}

		// normalize
		for ( vv = 0; vv < patch.v_cnt; vv++ )
		{
			float sum = 0.0f;
			for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
			{
				sum += vs_lengths[vv][s_basis];
			}

			if(sum > 0.0f)
			{
				sum = 1.0f / sum;
				for ( s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
				{
					vs_lengths[vv][s_basis] *= sum;
				}
			}
			//else // should generate uniform spacing ?
		}

		for ( uu = 0; uu < patch.u_cnt; uu++ )
		{
			float sum = 0.0f;
			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				sum += ut_lengths[uu][t_basis];
			}

			if(sum > 0.0f)
			{
				sum = 1.0f / sum;
				for ( t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
				{
					ut_lengths[uu][t_basis] *= sum;
				}
			}
			//else // generate uniform spacing ?
		}

		// generate fractions
		for ( vv = 0; vv < patch.v_cnt; vv++ )
		{
			for ( int uu = 1; uu < patch.s_basis_cnt; uu++ )
			{
				vs_lengths[vv][uu] += vs_lengths[vv][uu-1];
			}

			// shift
			for ( uu = patch.u_cnt - 1; uu > 0; uu-- )
			{
				vs_lengths[vv][uu] = vs_lengths[vv][uu-1];
			}
			vs_lengths[vv][0] = 0.0f;
		}

		for ( uu = 0; uu < patch.u_cnt; uu++ )
		{
			for ( int vv = 1; vv < patch.t_basis_cnt; vv++ )
			{
				ut_lengths[uu][vv] += ut_lengths[uu][vv-1];
			}

			// shift
			for ( vv = patch.v_cnt - 1; vv > 0; vv-- )
			{
				ut_lengths[uu][vv] = ut_lengths[uu][vv-1];
			}

			ut_lengths[uu][0] = 0.0f;
		}

		const float u00 = patch.uv_list[0].u;
		const float u01 = patch.uv_list[patch.v_cnt - 1].u;
		const float u10 = patch.uv_list[(patch.u_cnt - 1) * patch.v_cnt].u;
		const float u11 = patch.uv_list[patch.u_cnt * patch.v_cnt - 1].u;

		const float v00 = patch.uv_list[0].v;
		const float v01 = patch.uv_list[patch.v_cnt - 1].v;
		const float v10 = patch.uv_list[(patch.u_cnt - 1) * patch.v_cnt].v;
		const float v11 = patch.uv_list[patch.u_cnt * patch.v_cnt - 1].v;

		for(uu = 0; uu < patch.u_cnt; uu++)
		{
			for(int vv = 0; vv < patch.v_cnt; vv++)
			{
				const float s_fraction = vs_lengths[vv][uu];
				const float om_s_fraction = 1.0f - s_fraction;
				const float t_fraction = ut_lengths[uu][vv];
				const float om_t_fraction = 1.0f - t_fraction;

				float tmp_u1 = s_fraction * u10 + om_s_fraction * u00;
				float tmp_u2 = s_fraction * u11 + om_s_fraction * u01;
				patch.uv_list[uu * patch.v_cnt + vv].u =
					t_fraction * tmp_u2 + om_t_fraction * tmp_u1;

				float tmp_v1 = s_fraction * v10 + om_s_fraction * v00;
				float tmp_v2 = s_fraction * v11 + om_s_fraction * v01;
				patch.uv_list[uu * patch.v_cnt + vv].v =
					t_fraction * tmp_v2 + om_t_fraction * tmp_v1;
			}
		}
	}

	TrapFpu(false);

	ActivateEvaluator(RATIONAL);
	SetEvaluatorDefaults();

	ActivateEvaluator(NON_RATIONAL);
	SetEvaluatorDefaults();
}
#endif

bool NURBMESH::InitPolynomials(void)
{
	bool result = false;

	if(iigloo)
	{
		for(int s = 0; s < NUM_ORDERS; s++)
		{
			for(int t = 0; t < NUM_ORDERS; t++)
			{
				PolynomialEnumData	poly_data;

				// lighting
				poly_data.poly_found = false;
				poly_data.s_type = IIGLOO_BEZIER;
				poly_data.t_type = IIGLOO_BEZIER;
				poly_data.s_order = Order_From_Count(LOWEST_ORDER + s);
				poly_data.t_order = Order_From_Count(LOWEST_ORDER + t);

				iigloo->Enumerate_Polynomials( PolynomialEnumerator, &poly_data );
				if ( poly_data.poly_found )
				{
					HRESULT hr = ::CoCreateInstance( poly_data.poly_id,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IIPolynomial,
										(void**)&(r_polynom[s][t]) );

					if ( FAILED(hr) )
					{
						return ( false );
					}

					hr = ::CoCreateInstance( poly_data.poly_id,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IIPolynomial,
										(void**)&(g_polynom[s][t]) );

					if ( FAILED(hr) )
					{
						return ( false );
					}

					hr = ::CoCreateInstance( poly_data.poly_id,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IIPolynomial,
										(void**)&(b_polynom[s][t]) );

					if ( FAILED(hr) )
					{
						return ( false );
					}
				}
				else
				{
					return false;
				}
			}
		}
	
		result = true;
	}

	return result;
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

		  DA_HEAP_ACQUIRE_HEAP(HEAP);
		  DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

         server = new DAComponentFactory<DAComponent<NURBMESH>, RendCompDesc> (CLSID_NURBMesh);

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
                                     CLSID_NURBMesh, 
                                     DACOM_NORMAL_PRIORITY);
            }

         server->Release();
         break;
		}

   return TRUE;
}

#ifdef _DEBUG
#include <float.h>
void TrapFpu(bool on)
{
	if(on)
	{
		_clear87();

		// prepare to trap floating point exceptions
		unsigned int control_word = _controlfp (0, 0);

		//control_word &= ~(EM_INVALID | EM_UNDERFLOW | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
		control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
		_control87 (control_word, MCW_EM);
	}
	else
	{
		_clear87();
		_control87(_CW_DEFAULT, 0xfffff);
	}
}
#else
void TrapFpu(bool){}
#endif

#pragma warning( pop )