// StateCache.h
//
// set()'s support force_to_hw flag so that cache may be ignored (and invalidated)
// and the value written directly to the hw.
//


#ifndef __StateCache_h__
#define __StateCache_h__

//

#define RP_DISABLE_CACHES 1
//

#include "da_d3dtypes.h"

//

#include "RenderDebugger.h"

//

#if !RP_DISABLE_CACHES 
#pragma message( "TODO: verify state blocks still work with caches enabled" )
#endif

//

struct CACHED_PIPELINE_STATE
{
	U32		value;
	bool	valid;

	//
	
	void invalidate( void )
	{
		valid = false;
	}

	//

	U32 get( void )
	{
		if( !valid ) {
			
			GENERAL_FATAL( _MS(( "CACHED_PIPELINE_STATE: get: called when state is not valid!\n" )) );

			value = 0;
		}

		return value;
	}

	//

	void set( U32 new_value, bool force_to_hw = false )
	{
		value = new_value;
		valid = true;
	}

	//

	bool is_different( U32 new_value )
	{
		return (!valid || (value != new_value));
	}

	//

	bool is_enabled( void ) 
	{
		ASSERT( valid );
		return !(value == 0);
	}

	//


};

//

struct CACHED_VIEWPORT
{
	D3DVIEWPORT9 value;
	bool		 dr_valid;
	bool		 vp_valid;

	//

	void invalidate( void )
	{
		dr_valid = false;
		vp_valid = false;
	}

	//

	void get( IDirect3DDevice9 *device, D3DVIEWPORT9 *out )
	{
		if( dr_valid && vp_valid ) {
			memcpy( out, &value, sizeof(value) );
		}
		else {

			HRESULT hr; 

			if( SUCCEEDED( hr = device->GetViewport( out ) ) ) {
				memcpy( &value, out, sizeof(value) );
				dr_valid = true;
				vp_valid = true;
			}
			else {
				GENERAL_TRACE_1( _MS(( "CACHED_VIEWPORT: get: GetViewport() failed: %s\n", rp_rd_ddmessage(hr) ))  );
			}
		}
	}

	//

	void get_viewport( IDirect3DDevice9 *device, U32 *out_x, U32 *out_y, U32 *out_w, U32 *out_h )
	{
		if( !vp_valid ) {

			HRESULT hr; 

			if( SUCCEEDED( hr = device->GetViewport( &value ) ) ) {
	
				dr_valid = true;
				vp_valid = true;

			}
			else {

				GENERAL_TRACE_1( _MS(( "CACHED_VIEWPORT: get: GetViewport() failed: %s\n", rp_rd_ddmessage(hr) ))  );

				memset( &value, 0, sizeof(value) );
			}
		}

		*out_x = value.X;
		*out_y = value.Y;
		*out_w = value.Width;
		*out_h = value.Height;
	}

	//

	void set_viewport( IDirect3DDevice9 *device, U32 x, U32 y, U32 w, U32 h, bool force_to_hw = false )
	{
		if( vp_valid && !force_to_hw && (x == value.X) && (y == value.Y) && (w == value.Width) && (h == value.Height) ) {
#if !RP_DISABLE_CACHES 
			return;
#endif
		}

		HRESULT hr; 

		
		/*
		w *= 800;
		x *= 800;
		w /= 1024;
		x /= 1024;

		h *= 600;
		y *= 600;
		h /= 768;
		y /= 768;
	*/
		//ASSERT(x + w <= 640);
		//ASSERT(y + h <= 480);
		if (y + h > 600)
		{
			int x = 42;
			x++;
		}

//		x = y = 0;
//		w = 640;
//		h = 480;

		value.X = x;
		value.Y = y;
		value.Width = w;
		value.Height = h;

		if( SUCCEEDED( hr = device->SetViewport( &value ) ) ) {
			rp_rd_viewport( &value );
			vp_valid = !force_to_hw;
		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_VIEWPORT: set_viewport: SetViewport() failed: %s\n", rp_rd_ddmessage(hr) ))  );
		}
	}

	//

	void get_depth_range( IDirect3DDevice9 *device, float *out_min, float *out_max )
	{
		if( !dr_valid ) {

			HRESULT hr; 

			if( SUCCEEDED( hr = device->GetViewport( &value ) ) ) {
	
				dr_valid = true;

			}
			else {

				GENERAL_TRACE_1( _MS(( "CACHED_VIEWPORT: get: GetViewport() failed: %s\n", rp_rd_ddmessage(hr) ))  );

				memset( &value, 0, sizeof(value) );
			}
		}

		*out_min = value.MinZ;
		*out_max = value.MaxZ;
	}

	//

	void set_depth_range( IDirect3DDevice9 *device, float min, float max, bool force_to_hw = false )
	{
		if( dr_valid && !force_to_hw && (min == value.MinZ) && (max == value.MaxZ) ) {
#if !RP_DISABLE_CACHES 
			return;
#endif
		}

		HRESULT hr; 

		value.MinZ = min;
		value.MaxZ = max;

		if( SUCCEEDED( hr = device->SetViewport( &value ) ) ) {
			
			rp_rd_viewport( &value );

			dr_valid = !force_to_hw;

		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_VIEWPORT: set_depth_range: SetViewport() failed: %s\n", rp_rd_ddmessage(hr) ))  );
		}
	}
};

//

struct CACHED_RENDERSTATE
{
	U32			value;					// render state value
	bool		valid;					// value is valid

	//

	void invalidate( void )
	{
		valid = false;
	}

	//

	U32 get( IDirect3DDevice9 *device, U32 state_idx )
	{
		if( !valid ) {

			HRESULT hr;

			if( SUCCEEDED( hr = device->GetRenderState( (D3DRENDERSTATETYPE)state_idx, &value ) ) ) {

				valid = true;
			
			}
			else {
				
				GENERAL_TRACE_1( _MS(( "CACHED_RENDERSTATE: get: GetRenderState( %d, out ) failed: %s\n", state_idx, rp_rd_ddmessage(hr) ))  );

				value = 0;
			}
		}

		return value;
	}

	//

	void set( IDirect3DDevice9 *device, U32 state_idx, U32 new_value, bool force_to_hw = false )
	{
		if( valid && !force_to_hw && (value == new_value) ) {
#if !RP_DISABLE_CACHES 
			return;
#endif
		}

		HRESULT hr; 

		if( SUCCEEDED( hr = device->SetRenderState( (D3DRENDERSTATETYPE)state_idx, new_value ) ) ) {

			rp_rd_render_state( (D3DRENDERSTATETYPE)state_idx, new_value );

			value = new_value;					

			valid = !force_to_hw;
		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_RENDERSTATE: set: SetRenderState( %d, %d ) failed: %s\n", state_idx, new_value, rp_rd_ddmessage(hr) ))  );
		}
	}

	//
};

//

struct CACHED_TEXTURESTATE
{
	U32			value;					// render state value
	bool		valid;					// value is valid

	//

	void invalidate( void )
	{
		valid = false;
	}

	//

	U32 get( IDirect3DDevice9 *device, U32 stage_idx, U32 state_idx )
	{
		if( !valid ) {

			HRESULT hr;

			if( SUCCEEDED( hr = device->GetTextureStageState( stage_idx, (D3DTEXTURESTAGESTATETYPE)state_idx, &value ) ) ) {

				valid = true;

			}
			else {
		
				GENERAL_TRACE_1( _MS(( "CACHED_TEXTURESTATE: get: GetTextureStageState( %d, %d, out ) failed: %s\n", stage_idx, state_idx, rp_rd_ddmessage(hr) ))  );

				value = 0;
			}
		}

		return value;
	}

	//

	void set( IDirect3DDevice9 *device, U32 stage_idx, U32 state_idx, U32 new_value, bool force_to_hw = false )
	{
		if( valid && !force_to_hw && (value != new_value) ) {
#if !RP_DISABLE_CACHES 
			return;
#endif
		}

		HRESULT hr; 

		if( SUCCEEDED( hr = device->SetTextureStageState( stage_idx, (D3DTEXTURESTAGESTATETYPE)state_idx, new_value ) ) ) {

			rp_rd_texture_state( stage_idx, (D3DTEXTURESTAGESTATETYPE)state_idx, new_value );

			value = new_value;					
			
			valid = !force_to_hw;

		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_TEXTURESTATE: set: SetTextureStageState( %d, %d ) failed: %s\n", stage_idx, state_idx, new_value, rp_rd_ddmessage(hr) ))  );
		}
	}

	//
};

//

struct CACHED_TEXTURE
{
	U32					value;		// irp texture handle
	bool				valid;		// texture is valid

	//

	void invalidate( void )
	{
		valid = false;
	}

	//

	U32 get( IDirect3DDevice9 *device, U32 stage_idx )
	{
		if( !valid ) {

			HRESULT hr; 

			if( SUCCEEDED( hr = device->GetTexture( stage_idx, (IDirect3DBaseTexture9**)&value ) ) ) {

				valid = true;

			}
			else {

				GENERAL_TRACE_1( _MS(( "CACHED_TEXTURE: get: GetTexture( %d, out ) failed: %s\n", stage_idx, rp_rd_ddmessage(hr) ))  );

				value = 0;
			}
		}

		return value;
	}

	//

	void set( IDirect3DDevice9 *device, U32 stage_idx, U32 new_value, bool force_to_hw = false )
	{
		if( valid && !force_to_hw && (value == new_value) ) {
#if !RP_DISABLE_CACHES 
			return;
#endif
		}

		HRESULT hr; 

		if( SUCCEEDED( hr = device->SetTexture( stage_idx, (IDirect3DBaseTexture9*)new_value ) ) ) {

			rp_rd_texture( stage_idx, (IDirectDrawSurface7*)new_value );

			value = new_value;					

			valid = !force_to_hw;
		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_TEXTURE: set: SetTexture( %d, %d ) failed: %s\n", stage_idx, new_value, rp_rd_ddmessage(hr) ))  );
		}
	}

	//
};

//

struct CACHED_MATRIX
{
	D3DMATRIX	value;
	bool		valid;

	void invalidate( void )
	{
		valid = false;
	}

	//

	void get( IDirect3DDevice9 *device, U32 which_matrix, D3DMATRIX *out )
	{
		if( valid ) {
			memcpy( out, &value, sizeof(value) );
		}
		else {

			HRESULT hr; 

			if( SUCCEEDED( hr = device->GetTransform( (D3DTRANSFORMSTATETYPE)which_matrix, out ) ) ) {
				memcpy( &value, out, sizeof(value) );
				valid = true;
			}
			else {
				GENERAL_TRACE_1( _MS(( "CACHED_MATRIX: get: GetTransform( %d, out ) failed: %s\n", which_matrix, rp_rd_ddmessage(hr) ))  );
			}
		}
	}

	//

	void set( IDirect3DDevice9 *device, U32 which_matrix, D3DMATRIX *new_value, bool force_to_hw = false )
	{
		// Do not check the cache when setting the matrix (too many compares)
		//
		HRESULT hr; 

		if( SUCCEEDED( hr = device->SetTransform( (D3DTRANSFORMSTATETYPE)which_matrix, new_value ) ) ) {

			rp_rd_transform( (D3DTRANSFORMSTATETYPE)which_matrix, new_value );

			memcpy( &value, new_value, sizeof(value) );

			valid = !force_to_hw;

		}
		else {
			GENERAL_TRACE_1( _MS(( "CACHED_MATRIX: set: SetTransform( %d, new ) failed: %s\n", which_matrix, new_value, rp_rd_ddmessage(hr) ))  );
		}
	}

};


//

#endif // EOF

