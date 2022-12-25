//--------------------------------------------------------------------------//
//                                                                          //
//                            LightMan.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/dev/Src/Shading/LightManager/LightManager.cpp 9     3/21/00 4:30p Pbleisch $
*/			    
//---------------------------------------------------------------------------

#pragma warning( disable: 4018 4100 4201 4244 4512 4530 4663 4688 4706 4710 4786 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dacom.h"
#include "tcomponent.h"
#include "tsmartpointer.h"
#include "fdump.h"
#include "inv_sqrt.h"
#include "stddat.h"
#include "3dmath.h"
#include "da_heap_utility.h"
#include "lightman.h"
#include "ILight.h"
#include "ICamera.h"

//
// Static variables
//
static	ISQRT	inv_sqrt;

//
// Compile switches
//
#define USE_FAST_INVSQRT 1
#define USE_TABLE_INVSQRT 0
#define USE_ASM_FAST_INVSQRT 1

//
// Inline functions
//

#if USE_ASM_FAST_INVSQRT
const float ONE_HALF		= 0.5f;
const float THREE_HALVES	= 1.5f;

__declspec (naked) float fsqrt_inv(float f)
{
	__asm // 18 cycles
	{
		fld		dword ptr [esp + 4]
		fmul		dword ptr [ONE_HALF]
		
		mov		eax, [esp + 4]
		mov		ecx, 0x5f3759df
		shr		eax, 1
		sub		ecx, eax
		mov		[esp + 4], ecx
		fmul		dword ptr [esp + 4]
		fld		dword ptr [esp + 4]
		fmul		dword ptr [esp + 4]
		fld		dword ptr [THREE_HALVES]
		fmul		dword ptr [esp + 4]
		fxch		st(2)
		fmulp		st(1), st
		fsubp		st(1), st

		ret
	}
}

#else

float fsqrt_inv(float f)
{
	long i;
	float x2, y;

	x2 = 0.5f*f;
	i = *(long *)&f;
	i = 0x5f3759df - (i>>1);
	y = *(float *)&i;

	// repeat this iteration for more accuracy
	y = 1.5f*y - (x2*y * y*y);

	return y;
}
#endif

inline float INV_SQRT (float val)
{
#if (USE_FAST_INVSQRT && USE_TABLE_INVSQRT)
#error "Pick either fast inv_sqrt or table inv_sqrt, not both!"
#endif

#if (!USE_FAST_INVSQRT && !USE_TABLE_INVSQRT)
	return 1.0 / sqrt(val);
#endif

#if USE_FAST_INVSQRT
	return fsqrt_inv(val);
#endif

#if USE_TABLE_INVSQRT
	return inv_sqrt.InvSqrt(val);
#endif
}

//
// Other stuff
//
const char		*CLSID_LightManager	="LightManager";

enum
{
	LIGHT_INFINITE	=1,
	LIGHT_SPOT		=2
};

struct	ManagedLight
{
	ILight		*instance;
	Vector		direction_in_world_space;
	Vector		Pos;
	U32			flags;

	// Cached values used inside the lighting inner loop. Saves a per-light, per-vertex transformation.
	Vector	    dir_in_object_space;
	Vector	    pos_in_object_space;

	//next cache line
	LightRGB	color;
	float		range;
	float		range_squared;
	float		one_over_range_squared;
	float		cutoff;					//degrees
	float		cos_umbra;				//cos(cutoff)
	float		cos_penumbra;
	float		one_over_delta_cos;

	ManagedLight( void );
	void update_state( void );
};

ManagedLight::ManagedLight(void)
{
	//TODO: init this thing
	range	=cutoff	=-1;	//invalid
	flags	=0;
}

void ManagedLight::update_state(void)
{
	ASSERT(instance);

	SINGLE	value;
	Vector	direction;


	instance->GetColor(color);
	
	if((value = instance->GetRange()) != range)
	{
		range			=value;
		range_squared	=range * range;
		
		if(range_squared)
		{
			one_over_range_squared	=1.0f / range_squared;
		}
		else
		{
			// *** TODO: Make this a better arbitrarily large value.
			// *** Is there a better way to do this?
			one_over_range_squared	=1000.0f;	//arbitrary large number
		}
	}

	instance->GetDirection(direction);

	value	=(direction.x * direction.x) + (direction.y * direction.y) + (direction.z * direction.z);
	if(value)
	{
		value		= 1.0f / sqrt(value);	//normalize direction vector
		direction	*=value;
	}

	if (instance->IsInfinite())
	{
		flags |= LIGHT_INFINITE;
	}
	else
	{
		flags &= (~LIGHT_INFINITE);
	}

	if((value = instance->GetCutoff()) != cutoff)
	{
		cutoff	=value;
		
		if(value <= 0 || value >= 180.0F)
		{
			flags	&=~LIGHT_SPOT;
		}
		else
		{
			float	cutoff_radians		=value * MUL_DEG_TO_RAD;
			float	penumbra_radians	=1.3f * cutoff_radians;

			penumbra_radians	=__min(penumbra_radians, 180.0F * MUL_DEG_TO_RAD);
			cos_umbra			=cos(cutoff_radians);
			cos_penumbra		=cos(penumbra_radians);

			float	shadow_diff	=cos_umbra - cos_penumbra;

			if(shadow_diff)
			{
				one_over_delta_cos	=1.0f / (shadow_diff);
			}
			else
			{
				one_over_delta_cos	=1000.0f;	//arbitrary large number, it doesn't really matter
			}
			
			flags	|=LIGHT_SPOT;
		}
	}

	Transform	tempTrans;

	instance->GetTransform(tempTrans);

	instance->GetPosition(Pos);

	// jasony (direction is always relative to orientation now)
	if(flags)
	{
		direction_in_world_space	=tempTrans.rotate(direction);
	}
	else
	{
		direction_in_world_space.zero();
	}
}


#define LM_FLIP_NORMAL		(0x80000000 )
#define LM_IS_FLIP( idx )	((idx) & LM_FLIP_NORMAL)
#define LM_IDX( idx )		((idx) & (~(LM_FLIP_NORMAL)))
#define LM_ADD_STRIDE(ptr, val_bytes)	( ((U8*)ptr) + val_bytes) 


struct LightStruct
{
	ManagedLight *light;
	float	score;
};


struct LightManager : public ILightManager,
					  public IAggregateComponent
{

public:
	BEGIN_DACOM_MAP_INBOUND(LightManager)
	DACOM_INTERFACE_ENTRY(ILightManager)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_ILightManager,ILightManager)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	MetaList <ManagedLight>	lights;
	int max_lights;

	ManagedLight **			active_light_map;

	unsigned int		light_buffer_len;
	struct LightStruct * light_buffer;

	LightRGB			ambient;

	Vector				cam_position;	// updated by update_lighting()

public: // Interface

	LightManager( void );
	~LightManager( void );
	DA_HEAP_DEFINE_NEW_OPERATOR(LightManager);

	GENRESULT init( AGGDESC *desc );

	/* IAggregateComponent methods */

	GENRESULT COMAPI Initialize( void );

	/* ILightManager methods */
	
	void COMAPI set_ambient_light( int r, int g, int b ) ;
	void COMAPI get_ambient_light( int & r, int & g, int & b ) ;
	int	COMAPI get_max_active_lights( void ) const ;
	void COMAPI activate_lights( ILight * const * lights, int num_lights ) ;
	void COMAPI deactivate_lights( ILight * const * lights, int num_lights ) ;
	void COMAPI deactivate_all_lights( void ) ;
	int COMAPI get_active_lights( ILight ** lights ) ;
	void COMAPI register_light( ILight * light ) ;
	void COMAPI unregister_light( ILight * light ) ;
	int COMAPI get_registered_lights( ILight ** lights ) ;
	void COMAPI query_lights( bool * status, ILight * const * lights, int num_lights ) ;	
	int	COMAPI get_best_lights( ILight ** lights, int num_lights, const Vector & spot, float radius ) ;
	void COMAPI update_lighting( struct ICamera *camera ) ;
	void COMAPI light_vertices( LightRGB * rgb, const Vector * vertices, const Vector * normals, int n, const Transform * world_to_object, ClampFlags clamp ) ;
	void COMAPI light_vertices_strided( LightRGB * rgb, const U32 lstride, const Vector * vertices, const U32 vstride, const Vector * normals, const U32 nstride, const U32 *nindices, const U32 nistride, int n, const Transform *world_to_object, ClampFlags clamp, LightRGB *back_rgb ) ;

	void COMAPI light_vertices_U8( LightRGB_U8 * rgb, const Vector * vertices, const Vector * normals,int n, const Transform * world_to_object, ClampFlags clamp);

protected: // Interface
};

//

LightManager::LightManager( void )
{
	max_lights = 8;
	active_light_map = new ManagedLight *[max_lights];

	memset(active_light_map, 0, sizeof(ManagedLight *) * max_lights);

	light_buffer = 0;
	light_buffer = NULL;

	ambient.r = 
	ambient.g = 
	ambient.b = 0;
}

//

LightManager::~LightManager (void)
{
	MetaNode<ManagedLight> * light_node = NULL;
	while (lights.traverse(light_node))
	{
		delete light_node->object;
	}
	lights.free();

	if (active_light_map)
	{
		delete [] active_light_map;
		active_light_map = NULL;
	}

	if (light_buffer)
	{
		delete [] light_buffer;
		light_buffer = NULL;
		light_buffer_len = 0;
	}
}

//

GENRESULT LightManager::init( AGGDESC *desc )
{		
	return GR_OK;
}

//

GENRESULT LightManager::Initialize( void )
{
	return GR_OK;
}



//--------------------------------------------------------------------------//
//
int	LightManager::get_max_active_lights(void) const
{
	return max_lights;
}

//--------------------------------------------------------------------------//
//
int LightManager::get_registered_lights( ILight ** list )
{
	int result = lights.count();

	if (list)
	{
		ILight ** dst = list;

		MetaNode<ManagedLight> * node = NULL;
		while (lights.traverse(node))
		{
			*(dst++) = node->object->instance;
		}
	}

	return result;
}

//--------------------------------------------------------------------------//
//
int LightManager::get_active_lights (ILight ** list)
{
	int result = 0;

	ManagedLight ** map = active_light_map;
	for (int i = 0; i < max_lights; i++, map++)
	{
		ManagedLight * light = *map;

		if (light)
		{
			if (list)
			{
				list[result] = light->instance;
			}
			result++;
		}
	}

	return result;
}

//--------------------------------------------------------------------------//
//
static int __cdecl LSCompare(const void *elem1, const void *elem2 )
{
	const LightStruct * l1 = (const LightStruct *) elem1;
	const LightStruct * l2 = (const LightStruct *) elem2;

	int result;
	if (l1->score < l2->score)
	{
		result = 1;
	}
	else if (l1->score > l2->score)
	{
		result = -1;
	}
	else
	{
		result = 0;
	}

	return result;
}

//--------------------------------------------------------------------------//
//
int	LightManager::get_best_lights (ILight ** list, int num_lights, const Vector & spot, float radius)
{
	int result = 0;

	// Allocate/reallocate a holding buffer for the list of lights affecting the spot.
	if (!light_buffer)
	{
		light_buffer_len = lights.count();
		if (light_buffer_len)
			light_buffer = new LightStruct[light_buffer_len];
	}
	else if (light_buffer_len < lights.count())
	{
		light_buffer_len = lights.count();
		delete [] light_buffer;
		light_buffer = new LightStruct[light_buffer_len];
	}

	float r2 = radius * radius;

	int list_len = 0;
	LightStruct * buff = light_buffer;

	// Traverse the list of managed lights, skipping lights that cannot possibly affect the given spot, and
	// scoring those that do by their contribution.
	// *** This should probably take intensity into account.

	MetaNode<ManagedLight> * node = NULL;
	while (lights.traverse(node))
	{
		ManagedLight * l = node->object;

		if (l->flags & LIGHT_INFINITE)
		{
			// Since we are checking a point and don't have a normal, all infinite lights are automatically
			// given the highest score.
			// *** This is where the intensity would be taken into account.
			buff->light = l;
			buff->score = 1.0f;

			buff++;
			list_len++;
		}
		else
		{
			// Check for spot in light range and within check range.
			Vector	L = spot - l->Pos;
			float dist2 = L.magnitude_squared();
			if (dist2 <= r2 && dist2 <= l->range_squared)
			{
				// Light in both ranges, so compute score.
				float baseScore = 1.0f;

				// Calc score as contribution of light.
				buff->light = l;
				buff->score = baseScore;

				if (l->flags & LIGHT_SPOT)
				{
					// If the spot is within the hemisphere of the spotlight, we include it.
					// Otherwise, we skip the light altogther.
					SINGLE cos_angle = dot_product (L, l->direction_in_world_space);
//					if (cos_angle > 0.0f)
					{
						if (dist2 < 1e-5)
						{
							// Spot is very close to the light, so the normalization in the code below will
							// probably have numeric error. Just leave the score as-is
						}
						else
						{
							// Adjust the score according to where the spot is relative to the light.
							// Spot lights adjust their contribution based on the size of the penumbra
							float invSqrtDist = INV_SQRT(dist2);
							cos_angle *= invSqrtDist;
							if (cos_angle <= l->cos_penumbra)
							{
								// Spot is outside of the penumbera. We still add the light to the list, but its
								// score is set low. This is because the test spot is probably the center of the object,
								// and large objects could still be lit by this light even if the object center is not.
								buff->score *= cos_angle;
							}
							else if (cos_angle < l->cos_umbra)
							{
								register SINGLE cone_atten = (cos_angle - l->cos_penumbra) * l->one_over_delta_cos;
								buff->score *= cone_atten;
							}
							else
							{
								// Leave the score as-is. The spot is within the bright part of the spotlight.
							}
						}
					}
#if 0
					else
					{
						// Skip this light.
						continue;
					}
#endif
				}

				// Adjust the score for distance from the light
				buff->score *= 1.0f - (dist2 * l->one_over_range_squared);

				// Increment for the next light
				buff++;
				list_len++;
			}
		}
	}

	if (list_len <= num_lights)
	{
		// Fewer lights than requested meet distance criterion. Use all of them.
		result = list_len;
	}
	else
	{
		// Sort list, use num_list best entries.
		qsort(light_buffer, list_len, sizeof(LightStruct), LSCompare);
		result = num_lights;
	}

	// Copy the light information over to the output buffer and return the count.
	LightStruct * src = light_buffer;
	ILight ** dst = list;
	for (int i = 0; i < result; i++, src++, dst++)
	{
		*dst = src->light->instance;
	}

	return result;
}

//--------------------------------------------------------------------------//
//
void LightManager::activate_lights (ILight * const * list, int num_lights)
{
	num_lights = __min(__max(0, num_lights), max_lights);

	ILight * const * src = list;
	for (int i = 0; i < num_lights; i++, src++)
	{
		bool skip = false;
		ManagedLight ** map = active_light_map;
		for (int j = 0; j < max_lights; j++, map++)
		{
			ManagedLight * light = *map;
			if (light && (light->instance == *src))
			{
			// ManagedLight is already active.
				skip = true;
				break;
			}
		}

		if (skip)
		{
			PERFORMANCE_TRACE_5("ILightManager: Attempt to activate already-active light.\n");
			continue;
		}
				
		bool found_slot = false;

		map = active_light_map;
		for (int j = 0; j < max_lights; j++, map++)
		{
			if (!(*map))
			{
			// Found free slot.
				found_slot = true;

				MetaNode<ManagedLight> * node = NULL;
				while (lights.traverse(node))
				{
					if (node->object->instance == *src)
					{
						*map = node->object;
						break;
					}
				}

				break;
			}
		}
		 
		if (!found_slot)
		{
			PERFORMANCE_TRACE_3("ILightManager: Not enough light slots available to activate light.\n");
		}
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::deactivate_lights (ILight * const * lights, int num_lights)
{
	ILight * const * src = lights;

	for (int i = 0; i < num_lights; i++, src++)
	{
		ManagedLight ** map = active_light_map;
		for (int j = 0; j < max_lights; j++, map++)
		{
			ManagedLight * light = *map;
			if (light && (light->instance == *src))
			{
				*map = NULL;
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::deactivate_all_lights (void)
{
	ManagedLight ** map = active_light_map;
	for (int i = 0; i < max_lights; i++, map++)
	{
		ManagedLight * light = *map;
		if (light)
		{
			*map = NULL;
		}
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::query_lights (bool * status, ILight * const * lights, int num_lights)
{
	ILight * const * l = lights;
	bool * s = status;
	for (int i = 0; i < num_lights; i++, l++, s++)
	{
		*s = false;
		ManagedLight ** map = active_light_map;
		for (int j = 0; j < max_lights; j++, map++)
		{
			ManagedLight * light = *map;
			if (light && (light->instance == *l))
			{
				*s = true;
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------//
//
inline int GetClampColorU8(const int & c)
{
	if(c >= 255)
	{
		return 255;
	}
	else
	if(c <= 0)
	{
		return 0;
	}
	else
	{
		return c;
	}
}

inline void ClampColorU8(int & c)
{
	if(c > 255)
	{
		c = 255;
	}
}


//--------------------------------------------------------------------------//
//
#define LM_ADD_STRIDE(ptr, val_bytes)	( ((U8*)ptr) + val_bytes) 

void LightManager::light_vertices_strided (LightRGB * rgb, const U32 lstride, const Vector * vertices,
										   const U32 vstride, const Vector * normals, const U32 nstride,
										   const U32 *nindices, const U32 nistride, int n,
										   const Transform * world_to_object, ClampFlags clamp,
										   LightRGB * back_rgb)
{
	LightRGB *dst = rgb;
	const Vector * p = vertices;
	const U32 *Ni = nindices;
	const Vector * N = &normals[*Ni];
	int		max_c;

	dst = rgb;
	for (int l = 0; l < n; l++) 
	{
		*dst = ambient;
		dst = (LightRGB*)LM_ADD_STRIDE( dst, lstride );
	}

	if(back_rgb)
	{
		LightRGB *back_dst = back_rgb;
		for (int l = 0; l < n; l++) 
		{
			*back_dst = ambient;
			back_dst = (LightRGB*)LM_ADD_STRIDE( back_dst, lstride );
		}

	}

	dst = rgb;

	// Store off the position and direction of each light in object space before entering the inner lighting loop
	{
		register ManagedLight ** light_ptr = active_light_map;
		for (int l = 0; l < max_lights; l++, light_ptr++)
		{
			register ManagedLight * light = *light_ptr;
			if (light)
			{
				if(light->flags & LIGHT_INFINITE)
				{
					light->pos_in_object_space = light->Pos;

					if (world_to_object)
					{
						light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
					}
				}
				else // cone or point-source light
				{
					if(world_to_object)
					{
						light->pos_in_object_space = world_to_object->rotate_translate(light->Pos);
						if(light->flags & LIGHT_SPOT)
						{
							light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
						}
						else
						{
							light->dir_in_object_space = light->direction_in_world_space;
						}
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
						light->pos_in_object_space = light->Pos;
					}
				}
			}
		}
	}
	
	// Light each vertex.
	for (int i = 0; i < n;
		i++,
		p=(Vector *)LM_ADD_STRIDE(p,vstride),
		dst=(LightRGB*)LM_ADD_STRIDE(dst,lstride),
		Ni=(U32*)LM_ADD_STRIDE(Ni,nistride),
		N=(Vector*)LM_ADD_STRIDE(normals,nstride*(LM_IDX(*Ni))) ) 
	{
		BOOL32	ValidLight;

		ManagedLight ** light_ptr = active_light_map;
		for (int l = ValidLight = 0; l < max_lights; l++, light_ptr++)
		{
			ManagedLight * light = *light_ptr;
			if (light)
			{
				ValidLight	=TRUE;
				if(light->flags & LIGHT_INFINITE)
				{
					const SINGLE dot = LM_IS_FLIP(*Ni) ? -dot_product(light->dir_in_object_space, *N) : dot_product(light->dir_in_object_space, *N);
					if (dot < 0.0f)
					{
						S32 atten2 = S32(-dot*1024); //rmarr (avoiding ftol)
						dst->r += (light->color.r * atten2)>>10;
						dst->g += (light->color.g * atten2)>>10;
						dst->b += (light->color.b * atten2)>>10;
					}
					else if(back_rgb)
					{
						LightRGB * dst_back = back_rgb + (dst - rgb);
						S32 atten2 = S32(dot*1024);
						dst_back->r += (light->color.r * atten2)>>10;
						dst_back->g += (light->color.g * atten2)>>10;
						dst_back->b += (light->color.b * atten2)>>10;
					}
				}
				else // cone or point-source light
				{
					const Vector L ( light->pos_in_object_space - *p );

					const SINGLE dist_squared = L.magnitude_squared();
					if (light->range_squared > dist_squared)
					{
					// Compute attenuation based on angle of incident light.
						
						const SINGLE dot = LM_IS_FLIP(*Ni) ? -dot_product(L, *N) : dot_product(L, *N);

						if (dot > 0.0f)
						{
							// ACCURATE magnitude.
							// *** Removed to get correct lighting. -TNB
//							const SINGLE one_over_L = inv_sqrt.InvSqrt(dist_squared);
							const SINGLE one_over_L = INV_SQRT (dist_squared);

							const SINGLE angle_atten = dot * one_over_L;
							SINGLE net_attenuation = angle_atten;

						// Compute cone attenuation for spotlights.
							if (light->flags & LIGHT_SPOT)
							{
							// Vary intensity according to angle from light direction.
								const SINGLE cos_angle = -dot_product(L, light->dir_in_object_space) * one_over_L;
								if (cos_angle <= light->cos_penumbra)
								{
									//cone_atten = 0;
									continue;
								}
								else if (cos_angle < light->cos_umbra)
								{
									const SINGLE cone_atten = (cos_angle - light->cos_penumbra) * light->one_over_delta_cos;
									net_attenuation *= cone_atten;
								}
								else
								{
									// cone_atten = 1.0;
								}
							}
							
						// Compute distance attenuation.
							const SINGLE dist_atten = 1.0f - (dist_squared * light->one_over_range_squared);

							// DEBUG - Remove for testing.
							net_attenuation *= dist_atten;

						//SINGLE net_attenuation = dist_atten * angle_atten * cone_atten;

							#define MIN_ATTENUATION (1.0f/255.0f)

							if (net_attenuation >= MIN_ATTENUATION)
							{
								const S32 atten2 = S32(net_attenuation*1024); //rmarr (avoiding ftol)
								dst->r += (light->color.r * atten2)>>10;
								dst->g += (light->color.g * atten2)>>10;
								dst->b += (light->color.b * atten2)>>10;
							}
						}
						else if(back_rgb)
						{
							// ACCURATE magnitude.
							// *** Removed to get correct lighting. -TNB
//							const SINGLE one_over_L = inv_sqrt.InvSqrt(dist_squared);
							const SINGLE one_over_L = INV_SQRT (dist_squared);

							const SINGLE angle_atten = -dot * one_over_L;
							SINGLE net_attenuation = angle_atten;

						// Compute cone attenuation for spotlights.
							if (light->flags & LIGHT_SPOT)
							{
							// Vary intensity according to angle from light direction.
								const SINGLE cos_angle = -dot_product(L, light->dir_in_object_space) * one_over_L;
								if (cos_angle <= light->cos_penumbra)
								{
									//cone_atten = 0;
									continue;
								}
								else if (cos_angle < light->cos_umbra)
								{
									const SINGLE cone_atten = (cos_angle - light->cos_penumbra) * light->one_over_delta_cos;
									net_attenuation *= cone_atten;
								}
								else
								{
									// cone_atten = 1.0;
								}
							}
							
						// Compute distance attenuation.
							const SINGLE dist_atten = (light->range_squared - dist_squared) * light->one_over_range_squared;

							net_attenuation *= dist_atten;

						//SINGLE net_attenuation = dist_atten * angle_atten * cone_atten;

							#define MIN_ATTENUATION (1.0f/255.0f)

							if (net_attenuation >= MIN_ATTENUATION)
							{
								LightRGB * dst_back = back_rgb + (dst - rgb);
								const S32 atten2 = S32(net_attenuation*1024); //rmarr (avoiding ftol)
								dst_back->r += (light->color.r * atten2)>>10;
								dst_back->g += (light->color.g * atten2)>>10;
								dst_back->b += (light->color.b * atten2)>>10;
							}
						}
					}							   
				}
			}
		}

		// Only clamp if there was a valid light.
		if(ValidLight)
		{
			if(clamp == CF_INTENSITY)
			{
				// assume all colors >= 0
				if(255 < (max_c = __max(__max(dst->r, dst->g), dst->b)))
				{
					dst->r = (dst->r * 255) / max_c;
					dst->g = (dst->g * 255) / max_c;
					dst->b = (dst->b * 255) / max_c;
				}

				if(back_rgb)
				{
					LightRGB *back_dst = back_rgb + (dst - rgb);
					// assume all colors >= 0
					if(255 < (max_c = __max(__max(back_dst->r, back_dst->g), back_dst->b)))
					{
						back_dst->r = (back_dst->r * 255) / max_c;
						back_dst->g = (back_dst->g * 255) / max_c;
						back_dst->b = (back_dst->b * 255) / max_c;
					}
				}
			}
			else if(clamp == CF_COLOR)
			{
				ClampColorU8(dst->r);
				ClampColorU8(dst->g);
				ClampColorU8(dst->b);

				if(back_rgb)
				{
					LightRGB *back_dst = back_rgb + (dst - rgb);
					ClampColorU8(back_dst->r);
					ClampColorU8(back_dst->g);
					ClampColorU8(back_dst->b);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::light_vertices (LightRGB * rgb, const Vector * vertices, const Vector * normals,
								   int n, const Transform * world_to_object, ClampFlags clamp)
{
	LightRGB	*color	=rgb;
	int			i, l, max_c;

	for(i=0;i < n;i++, color++)
	{
		*color	=ambient;
	}

	// Store off the position and direction of each light in object space before entering the inner lighting loop
	{
		register ManagedLight ** light_ptr = active_light_map;
		for (l = 0; l < max_lights; l++, light_ptr++)
		{
			register ManagedLight * light = *light_ptr;
			if (light)
			{
				if(light->flags & LIGHT_INFINITE)
				{
					light->pos_in_object_space = light->Pos;

					if (world_to_object)
					{
						light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
					}
				}
				else // cone or point-source light
				{
					if(world_to_object)
					{
						light->pos_in_object_space = world_to_object->rotate_translate(light->Pos);
						if(light->flags & LIGHT_SPOT)
						{
							light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
						}
						else
						{
							light->dir_in_object_space = light->direction_in_world_space;
						}
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
						light->pos_in_object_space = light->Pos;
					}
				}
			}
		}
	}

	LightRGB * dst = rgb;
	const Vector * p = vertices;
	const Vector * N = normals;

	for(i=0;i < n;i++, dst++, N++, p++)
	{
		BOOL32	ValidLight;
		ManagedLight ** light_ptr = active_light_map;
		for (l = ValidLight = 0; l < max_lights; l++, light_ptr++)
		{
			ManagedLight * light = *light_ptr;
			if (light)
			{
				ValidLight	=TRUE;
				if(light->flags & LIGHT_INFINITE)
				{
					SINGLE dot = dot_product(light->dir_in_object_space, *N);
					if(dot < 0.0f)
					{
						SINGLE	atten	=-dot;
						S32		atten2	=S32(atten*1024); //rmarr (avoiding ftol)
						dst->r	+=(light->color.r * atten2)>>10;
						dst->g	+=(light->color.g * atten2)>>10;
						dst->b	+=(light->color.b * atten2)>>10;
					}
				}
				else
				{
					// cone or point-source light
					Vector L ( light->pos_in_object_space - *p );

					SINGLE dist_squared = L.magnitude_squared();
					if (light->range_squared >= dist_squared) {

						// Compute attenuation based on angle of incident light.
						SINGLE dot = dot_product(L, *N);

						if (dot > 0.0f) {
							// ACCURATE magnitude.
							// *** Removed to get correct lighting. -TNB
//							SINGLE one_over_L = inv_sqrt.InvSqrt(dist_squared);
							SINGLE one_over_L = INV_SQRT(dist_squared);

							SINGLE angle_atten = dot * one_over_L;
							SINGLE net_attenuation = angle_atten;

						// Compute cone attenuation for spotlights.
							if (light->flags & LIGHT_SPOT)
							{
							// Vary intensity according to angle from light direction.
								SINGLE cos_angle = -dot_product(L, light->dir_in_object_space) * one_over_L;
								if (cos_angle <= light->cos_penumbra)
								{
									//cone_atten = 0;
									continue;
								}
								else if (cos_angle < light->cos_umbra)
								{
									SINGLE cone_atten = (cos_angle - light->cos_penumbra) * light->one_over_delta_cos;
									net_attenuation *= cone_atten;
								}
								else
								{
									// cone_atten = 1.0;
								}
							}
							
						// Compute distance attenuation.
							SINGLE dist_atten = 1.0 - (dist_squared * light->one_over_range_squared);

							net_attenuation *= dist_atten;

						//SINGLE net_attenuation = dist_atten * angle_atten * cone_atten;

							#define MIN_ATTENUATION (1.0f/255.0f)

							if (net_attenuation >= MIN_ATTENUATION)
							{
								S32 atten2 = S32(net_attenuation*1024); //rmarr (avoiding ftol)
								dst->r += (light->color.r * atten2)>>10;
								dst->g += (light->color.g * atten2)>>10;
								dst->b += (light->color.b * atten2)>>10;
							}
						}
					}
				}
			}
		}
		// Only clamp if there was a valid light.
		if(ValidLight)
		{
			if(clamp == CF_INTENSITY)
			{
				// assume all colors >= 0
				if(255 < (max_c = __max(__max(dst->r, dst->g), dst->b)))
				{
					dst->r = (dst->r * 255) / max_c;
					dst->g = (dst->g * 255) / max_c;
					dst->b = (dst->b * 255) / max_c;
				}
			}
			else if(clamp == CF_COLOR)
			{
				ClampColorU8(dst->r);
				ClampColorU8(dst->g);
				ClampColorU8(dst->b);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void LightManager::light_vertices_U8( LightRGB_U8 * rgb, const Vector * vertices, const Vector * normals,int n, const Transform * world_to_object, ClampFlags clamp)
{
	LightRGB_U8	*color	=rgb;
	int			i, l;

	for(i=0;i < n;i++, color++)
	{
		color->r	=ambient.r;
		color->g	=ambient.g;
		color->b	=ambient.b;
	}

	// Store off the position and direction of each light in object space before entering the inner lighting loop
	{
		register ManagedLight ** light_ptr = active_light_map;
		for (l = 0; l < max_lights; l++, light_ptr++)
		{
			register ManagedLight * light = *light_ptr;
			if (light)
			{
				if(light->flags & LIGHT_INFINITE)
				{
					light->pos_in_object_space = light->Pos;

					if (world_to_object)
					{
						light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
					}
				}
				else // cone or point-source light
				{
					if(world_to_object)
					{
						light->pos_in_object_space = world_to_object->rotate_translate(light->Pos);
						if(light->flags & LIGHT_SPOT)
						{
							light->dir_in_object_space = world_to_object->rotate(light->direction_in_world_space);
						}
						else
						{
							light->dir_in_object_space = light->direction_in_world_space;
						}
					}
					else
					{
						light->dir_in_object_space = light->direction_in_world_space;
						light->pos_in_object_space = light->Pos;
					}
				}
			}
		}
	}

	LightRGB_U8 * dst = rgb;
	const Vector * p = vertices;
	const Vector * N = normals;

	for(i=0;i < n;i++, dst++, N++, p++)
	{
		BOOL32	ValidLight;
		ManagedLight ** light_ptr = active_light_map;
		for (l = ValidLight = 0; l < max_lights; l++, light_ptr++)
		{
			ManagedLight * light = *light_ptr;
			if (light)
			{
				ValidLight	=TRUE;
				if(light->flags & LIGHT_INFINITE)
				{
					SINGLE dot = dot_product(light->dir_in_object_space, *N);
					if(dot < 0.0f)
					{
						SINGLE	atten	=-dot;
						S32		atten2	=S32(atten*1024); //rmarr (avoiding ftol)
						S32 lightColor = (light->color.r * atten2)>>10;
						dst->r	= __min(dst->r+lightColor,255);
						lightColor = (light->color.g * atten2)>>10;
						dst->g	=__min(dst->g+lightColor,255);
						lightColor = (light->color.b * atten2)>>10;
						dst->b	=__min(dst->b+lightColor,255);
					}
				}
				else
				{
					// cone or point-source light
					Vector L ( light->pos_in_object_space - *p );

					SINGLE dist_squared = L.magnitude_squared();
					if (light->range_squared >= dist_squared) {

						// Compute attenuation based on angle of incident light.
						SINGLE dot = dot_product(L, *N);

						if (dot > 0.0f) {
							// ACCURATE magnitude.
							// *** Removed to get correct lighting. -TNB
//							SINGLE one_over_L = inv_sqrt.InvSqrt(dist_squared);
							SINGLE one_over_L = INV_SQRT(dist_squared);

							SINGLE angle_atten = dot * one_over_L;
							SINGLE net_attenuation = angle_atten;

						// Compute cone attenuation for spotlights.
							if (light->flags & LIGHT_SPOT)
							{
							// Vary intensity according to angle from light direction.
								SINGLE cos_angle = -dot_product(L, light->dir_in_object_space) * one_over_L;
								if (cos_angle <= light->cos_penumbra)
								{
									//cone_atten = 0;
									continue;
								}
								else if (cos_angle < light->cos_umbra)
								{
									SINGLE cone_atten = (cos_angle - light->cos_penumbra) * light->one_over_delta_cos;
									net_attenuation *= cone_atten;
								}
								else
								{
									// cone_atten = 1.0;
								}
							}
							
						// Compute distance attenuation.
							SINGLE dist_atten = 1.0 - (dist_squared * light->one_over_range_squared);

							net_attenuation *= dist_atten;

						//SINGLE net_attenuation = dist_atten * angle_atten * cone_atten;

							#define MIN_ATTENUATION (1.0f/255.0f)

							if (net_attenuation >= MIN_ATTENUATION)
							{
								S32 atten2 = S32(net_attenuation*1024); //rmarr (avoiding ftol)
								S32 lightColor = (light->color.r * atten2)>>10;
								dst->r = __min(dst->r+lightColor,255);
								lightColor = (light->color.g * atten2)>>10;
								dst->g = __min(dst->g+lightColor,255);;
								lightColor = (light->color.b * atten2)>>10;
								dst->b = __min(dst->b+lightColor,255);;
							}
						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void LightManager::set_ambient_light (int r, int g, int b)
{
	ambient.r = GetClampColorU8(r);
	ambient.g = GetClampColorU8(g);
	ambient.b = GetClampColorU8(b);
}

//--------------------------------------------------------------------------//
//
void LightManager::register_light (ILight * _light)
{
	MetaNode<ManagedLight>* light_node = NULL;
	while (lights.traverse(light_node))
	{
		if (light_node->object->instance == _light)
		{
			return;		// already in the list
		}
	}

	ManagedLight * light = new ManagedLight;
	if (light)
	{
		light->instance = _light;
		lights.append(light);
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::unregister_light (ILight * _light)
{
	MetaNode<ManagedLight>* light_node = NULL;
	while (lights.traverse(light_node))
	{
		if (light_node->object->instance == _light)
		{
			deactivate_lights(&_light, 1);
			delete light_node->object;
			lights.remove(light_node);
			break;
		}
	}
}

//--------------------------------------------------------------------------//
//
void LightManager::update_lighting( ICamera * camera )
{
	if (camera)
		cam_position = camera->get_position();

	MetaNode<ManagedLight>* light_node = NULL;
	while (lights.traverse(light_node))
		light_node->object->update_state();
}

//--------------------------------------------------------------------------//
//
void LightManager::get_ambient_light (int & r, int & g, int & b)
{
	r = ambient.r;
	g = ambient.g;
	b = ambient.b;
}


//

void main( void ) {}	// linker bug

//

BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager *DACOM = DACOM_Acquire();
			IComponentFactory *server1;

			// Register System aggragate factory
			if( (server1 = new DAComponentFactory2<DAComponentAggregate<LightManager>, AGGDESC>(CLSID_LightManager)) != NULL ) {
				DACOM->RegisterComponent( server1, CLSID_LightManager, DACOM_NORMAL_PRIORITY );
				server1->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}



// EOF
