#ifndef ENG_H
#define ENG_H

#include "fdump.h"
#include "DACom.h"
#include "Engine.h"
#include "IAnim.h"
#include "LightMan.h"
#include "ITXMLib.h"
#include "Model.h"
#include "IHardpoint.h"
#include "stddat.h"
#include "Physics.h"
#include "Collision.h"
#include "IDumpText.h"
#include "ICamera.h"
#include "rendpipeline.h"
#include "RPUL/MTPrimitiveBuilder.h"

#define RWM_DONT_TEXTURE	(1<<0)

namespace Deform
{
	extern ICOManager *			DACOM;
	extern IEngine *			ENG;
	extern IModel *				MODEL;
	extern ILightManager *		LIGHT;
	extern ITXMLib *			TXMLIB;
	extern IAnimation *			ANIM;
	extern IChannel *			CHANNEL;
	extern IHardpoint *			HARDPOINT;
	extern IPhysics *			PHYSICS;
	extern ICollision *			COLLIDE;
	extern IDumpText *			DUMP;
	extern IRenderPipeline *	PIPE;
	extern IRenderPrimitive *	BATCH;

	extern MTPrimitiveBuilder	pb;

	
	extern int						vertex_pool_len;
	extern int						vertex_pool_index;
	extern MTVERTEX *				vertex_pool;
	extern Vector *					normal_pool;
	extern U32 *					normal_index_pool;
	extern LightRGB *				light_pool;
	
	extern int						index_list_len;
	extern int						index_list_index;
	extern U16 *					index_list;
	extern U16 *					vertex_slot;

	// Patch stuff
	extern int						sub_div_cnt;
	extern float *					div_weights;
	extern float *					div_n_weights;
	extern Vector					patch_aux[9];

	extern bool						active;

	extern U32						default_material_flags;
	extern U32						device_supports_uvchannel1;
	extern U32						device_num_tss;

	extern U32						specular_mode;		// 0, 1, or 2
	extern char						specular_texture_name[64];

	extern U32						diffuse2_fallback_blend[2];	// src,dst framebuffer blend modes for Diffuse1*Diffuse2
	extern U32						emissive_fallback_blend[2];	// src,dst framebuffer blend modes for + Emissive
	extern U32						specular_fallback_blend[2];	// src,dst framebuffer blend modes for + Specular

	extern float					min_poly_size;

	extern bool						got_ini_info;

	void DebugPrint (char *fmt, ...);
	void TrapFpu(bool on);
	void get_ini_info( void );

	void delete_pools( void );
	void verify_pools(const int size);
	void delete_lists( void );
	void verify_lists(const int size);
	void SetDivWeights(int div_cnt);
};

template<class _Ty> inline const _Ty& _MAX(const _Ty& _X, const _Ty& _Y)
{ return (_X < _Y ? _Y : _X); }

template<class _Ty> inline const _Ty& _MIN(const _Ty& _X, const _Ty& _Y)
{ return (_Y < _X ? _Y : _X); }

//

#endif