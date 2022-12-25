// StateInfo.h
//
//
//

#ifndef STATEINFO_H
#define STATEINFO_H

//

#pragma warning( disable : 4245 )	// conversion from int to long
#pragma warning( disable : 4530 )   // exceptions disabled
#pragma warning( disable : 4702 )   // unreachable code
#pragma warning( disable : 4710 )   // function 'foo' not inlined
#pragma warning( disable : 4786 )   // identifier truncated

#include <map>
#include <vector>

//

#define RPSI_F_VALID			(1<<0)
#define RPSI_F_IGNORE_DEFAULT	(1<<1)
#define RPSI_F_USE_COMPILE_DEFAULT	(1<<2)

//

struct RPSTATEINFO
{
public:	// Interface

	RPSTATEINFO( U32 _e=0, const char *_k=NULL, U32 _ct=0, U32 _flags = 0, U32 _rt = 0 )
	{
		set( _e, _k, _ct, _flags, _rt );
	}
	
	//

	RPSTATEINFO( const RPSTATEINFO &si )
	{
		operator=(si);
	}

	//

	RPSTATEINFO &operator=( const RPSTATEINFO &si )
	{
		enum_value = si.enum_value;
		key_name = si.key_name;
		ct_default_value = si.ct_default_value;
		rt_default_value = si.rt_default_value;
		rpsi_f_flags = si.rpsi_f_flags;
		return *this;
	}

	//

	bool is_valid( void )
	{
		return (rpsi_f_flags & RPSI_F_VALID);
	}

	//

	void set( U32 _e, const char *_k, U32 _ct, U32 _flags = RPSI_F_VALID, U32 _rt = 0 )
	{
		enum_value = _e;
		key_name = _k;
		ct_default_value = _ct;
		rt_default_value = _rt;
		rpsi_f_flags = _flags;
	}

	//

	void set_runtime_default( U32 _rt )
	{
		rt_default_value = _rt;
	}

	//

	bool get_enum_and_default( U32 *out_enum, U32 *out_default )
	{
		if( (rpsi_f_flags & RPSI_F_VALID) && !(rpsi_f_flags & RPSI_F_IGNORE_DEFAULT) ) {
			*out_enum = enum_value;
			if( rpsi_f_flags & RPSI_F_USE_COMPILE_DEFAULT ) {
				*out_default = ct_default_value;
			}
			else {
				*out_default = rt_default_value;
			}
			return true;
		}

		return false;
	}

public: // Data
	const char *key_name;
	U32 enum_value;
	U32	ct_default_value;	// compile time default
	U32 rt_default_value;	// runtime default
	U32	rpsi_f_flags;


};

//

typedef std::map<U32,RPSTATEINFO> RenderStateArray;
typedef std::map<U32,RPSTATEINFO> PipelineStateArray;
typedef std::map<U32,RPSTATEINFO> TextureStageStateArray;
typedef std::map<U32,RPSTATEINFO> AbilitiesArray;

//

inline void rpsi_build_render_state_info( RenderStateArray &rsa )
{
	rsa.clear();

#define RSA_SET( enum_suffix, default_value ) \
	rsa[D3DRS_ ## enum_suffix] = RPSTATEINFO( D3DRS_ ## enum_suffix, # enum_suffix, default_value, RPSI_F_VALID )

//	RSA_SET( ANTIALIAS, FALSE );
	RSA_SET( ZENABLE, TRUE );
	RSA_SET( FILLMODE, D3DFILL_SOLID );
	RSA_SET( SHADEMODE, D3DSHADE_GOURAUD );
	RSA_SET( ZWRITEENABLE, TRUE );
	RSA_SET( ALPHATESTENABLE, FALSE );
	RSA_SET( SRCBLEND, D3DBLEND_ONE );
	RSA_SET( DESTBLEND, D3DBLEND_ZERO );
	RSA_SET( CULLMODE, D3DCULL_NONE );
	RSA_SET( ZFUNC, D3DCMP_LESS );
	RSA_SET( ALPHAREF, 0 );
	RSA_SET( ALPHAFUNC, D3DCMP_ALWAYS );
	RSA_SET( DITHERENABLE, FALSE );
	RSA_SET( ALPHABLENDENABLE, FALSE );
	RSA_SET( FOGENABLE, FALSE );
	RSA_SET( SPECULARENABLE, FALSE );
//	RSA_SET( STIPPLEDALPHA, FALSE );
	RSA_SET( FOGCOLOR, 0x00000000 );
	RSA_SET( FOGTABLEMODE, D3DFOG_EXP );
	RSA_SET( FOGSTART, 0 );
	RSA_SET( FOGEND, 0 );
	RSA_SET( FOGDENSITY, 0 );
//	RSA_SET( EDGEANTIALIAS, FALSE );
//	RSA_SET( ZBIAS, 0 );
	RSA_SET( RANGEFOGENABLE, FALSE );
	RSA_SET( STENCILENABLE, FALSE );
	RSA_SET( STENCILFAIL, D3DSTENCILOP_KEEP );
	RSA_SET( STENCILZFAIL, D3DSTENCILOP_KEEP );
	RSA_SET( STENCILPASS, D3DSTENCILOP_KEEP );
	RSA_SET( STENCILFUNC, D3DCMP_ALWAYS );
	RSA_SET( STENCILREF, 0xFFFFFFFF );
	RSA_SET( STENCILMASK, 0xFFFFFFFF );
	RSA_SET( STENCILWRITEMASK, 0xFFFFFFFF );
	RSA_SET( TEXTUREFACTOR, 0x00000000 );
	RSA_SET( WRAP0, 0 );
	RSA_SET( WRAP1, 0 );
	RSA_SET( WRAP2, 0 );
	RSA_SET( WRAP3, 0 );
	RSA_SET( WRAP4, 0 );
	RSA_SET( WRAP5, 0 );
	RSA_SET( WRAP6, 0 );
	RSA_SET( WRAP7, 0 );
	RSA_SET( CLIPPING, TRUE );
	RSA_SET( LIGHTING, FALSE );
//	RSA_SET( EXTENTS, FALSE );
	RSA_SET( AMBIENT, 0x00000000 );
	RSA_SET( FOGVERTEXMODE, D3DFOG_NONE );
	RSA_SET( COLORVERTEX, FALSE );
	RSA_SET( LOCALVIEWER, TRUE );
	RSA_SET( NORMALIZENORMALS, FALSE );
//	RSA_SET( COLORKEYBLENDENABLE, FALSE );
	RSA_SET( DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );
	RSA_SET( SPECULARMATERIALSOURCE, D3DMCS_COLOR2 );
	RSA_SET( AMBIENTMATERIALSOURCE, D3DMCS_COLOR2 );
	RSA_SET( EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
//	RSA_SET( VERTEXBLEND, D3DVBLEND_DISABLE );
	RSA_SET( CLIPPLANEENABLE, 0 );
//	RSA_SET( TEXTUREPERSPECTIVE, TRUE );
}

//

inline void rpsi_build_pipeline_state_info( PipelineStateArray &psa )
{
	psa.clear();

#define PSA_SET( enum_suffix, default_value ) \
	psa[ RP_ ## enum_suffix ] = RPSTATEINFO( RP_ ## enum_suffix, # enum_suffix, default_value, RPSI_F_VALID )

	PSA_SET( BUFFERS_COLOR_BPP, 16 );
	PSA_SET( BUFFERS_DEPTH_BPP, 16 );
	PSA_SET( BUFFERS_STENCIL_BPP, 0 );
	PSA_SET( BUFFERS_COUNT, 2 );
	PSA_SET( BUFFERS_HWFLIP, TRUE );
	PSA_SET( BUFFERS_FULLSCREEN, FALSE );
	PSA_SET( BUFFERS_VSYNC, TRUE );
	PSA_SET( BUFFERS_OFFSCREEN, FALSE );
	PSA_SET( BUFFERS_HWMEM, TRUE );
	PSA_SET( BUFFERS_WIDTH, 0 );
	PSA_SET( BUFFERS_HEIGHT, 0 );
	PSA_SET( BUFFERS_DEPTH_AUTOW, FALSE );
	PSA_SET( BUFFERS_SWAP_STALL, FALSE );
	PSA_SET( BUFFERS_ANTIALIAS, FALSE );
	PSA_SET( TEXTURE, TRUE );
	PSA_SET( TEXTURE_LOD, TRUE );
	PSA_SET( TEXTURE_ALLOW_8BIT, TRUE );
	PSA_SET( TEXTURE_ALLOW_DXT, FALSE );
	PSA_SET( CLEAR_COLOR, 0x00000000 );
	PSA_SET( CLEAR_DEPTH, 0xFFFFFFFF );
	PSA_SET( CLEAR_STENCIL, 0xFFFFFFFF );
	PSA_SET( DEVICE_FPU_SETUP, 0 );
	PSA_SET( DEVICE_MULTITHREADED, FALSE );
	PSA_SET( DEBUG_PROFILE_LOG, FALSE );
	PSA_SET( DEBUG_FILL_COLOR, 0x00000000 );
	PSA_SET( DEVICE_VIDEO_STREAM_MODE, 0x00000001 );
}

//

inline void rpsi_build_abilities_info( AbilitiesArray &aa )
{
	aa.clear();

#define AA_SET( enum_suffix ) \
	aa[ RP_ ## enum_suffix ] = RPSTATEINFO( RP_ ## enum_suffix, # enum_suffix, 0, RPSI_F_VALID )

	AA_SET( A_DEVICE_2D_ONLY );
	AA_SET( A_DEVICE_3D_ONLY );
	AA_SET( A_DEVICE_GAMMA );
	AA_SET( A_DEVICE_WINDOWED );
	AA_SET( A_DEVICE_MEMORY );
	AA_SET( A_DEVICE_SOFTWARE );
	AA_SET( A_TEXTURE_NONLOCAL );
	AA_SET( A_TEXTURE_SQUARE_ONLY );
	AA_SET( A_TEXTURE_STAGES );
	AA_SET( A_TEXTURE_MAX_WIDTH );
	AA_SET( A_TEXTURE_MAX_HEIGHT );
	AA_SET( A_TEXTURE_NUMA );
	AA_SET( A_TEXTURE_BILINEAR );
	AA_SET( A_TEXTURE_TRILINEAR );
	AA_SET( A_TEXTURE_LOD );
	AA_SET( A_TEXTURE_SIMULTANEOUS );
	AA_SET( A_TEXTURE_COORDINATES );
	AA_SET( A_BUFFERS_DEPTH_LINEAR );
	AA_SET( A_ALPHA_ITERATED );
	AA_SET( A_ALPHA_TEST );
	AA_SET( A_ALPHA_TEST_ALL );
	AA_SET( A_BLEND_MUL_SRC );
	AA_SET( A_BLEND_MUL_DST );
	AA_SET( A_BLEND_ADD_SRC );
	AA_SET( A_BLEND_ADD_DST );
	AA_SET( A_BLEND_TRANSPARENCY_SRC );
	AA_SET( A_BLEND_TRANSPARENCY_DST );
	AA_SET( A_BLEND_MATRIX );
	AA_SET( A_DEPTH_BIAS );
	AA_SET( A_TEXTURE_OPS );
	AA_SET( A_TEXTURE_ARG_FLAGS );
	AA_SET( A_DEVICE_FULLSCENE_ANTIALIAS );
}

//

inline void rpsi_build_texture_stage_state_info( TextureStageStateArray &tssa, bool enabled )
{
	tssa.clear();

#define TSSA_SET( enum_suffix, default_value ) \
	tssa[ D3DTSS_ ## enum_suffix ] = RPSTATEINFO( D3DTSS_ ## enum_suffix, # enum_suffix, default_value, RPSI_F_VALID|RPSI_F_USE_COMPILE_DEFAULT )

	if( enabled ) {
		TSSA_SET( COLOROP, D3DTOP_MODULATE );
		TSSA_SET( ALPHAOP, D3DTOP_SELECTARG1 );
	}
	else {
		TSSA_SET( COLOROP, D3DTOP_DISABLE );
		TSSA_SET( ALPHAOP, D3DTOP_DISABLE );
	}

	TSSA_SET( COLORARG1, D3DTA_TEXTURE );
	TSSA_SET( COLORARG2, D3DTA_CURRENT );
	TSSA_SET( ALPHAARG1, D3DTA_DIFFUSE );
	TSSA_SET( ALPHAARG2, D3DTA_CURRENT );
	TSSA_SET( BUMPENVMAT00, 0 );
	TSSA_SET( BUMPENVMAT01, 0 );
	TSSA_SET( BUMPENVMAT10, 0 );
	TSSA_SET( BUMPENVMAT11, 0 );
	TSSA_SET( TEXCOORDINDEX, 0 );
//	TSSA_SET( ADDRESS, D3DTADDRESS_WRAP );
//	TSSA_SET( ADDRESSU, D3DTADDRESS_WRAP );
//	TSSA_SET( ADDRESSV, D3DTADDRESS_WRAP );
//	TSSA_SET( BORDERCOLOR, 0 );
//	TSSA_SET( MAGFILTER, D3DTFG_LINEAR );
//	TSSA_SET( MINFILTER, D3DTFN_LINEAR );
//	TSSA_SET( MIPFILTER, D3DTFP_POINT );
//	TSSA_SET( MIPMAPLODBIAS, 0 );
//	TSSA_SET( MAXMIPLEVEL, 0 );
//	TSSA_SET( MAXANISOTROPY, 1 );
	TSSA_SET( BUMPENVLSCALE, 0 );
	TSSA_SET( BUMPENVLOFFSET, 0 );
	TSSA_SET( TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
}

//

#endif // EOF
