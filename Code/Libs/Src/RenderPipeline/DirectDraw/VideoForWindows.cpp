// VideoForWindows.cpp
//
// VideoForWindows Utility functions
// 
//

#include <windows.h>
#include <vfw.h>	// Video For Windows API

//

#include "dacom.h"
#include "FDUMP.h"
#include "TSmartPointer.h"
#include "TComponent.h"
#include "IVideoStreamControl.h"

//

#include "VideoForWindows.h"
#include "DirectDraw.h"


//

struct RPDDVIDEOSTREAM_VFW : public IVideoStreamControl
{
	BEGIN_DACOM_MAP_INBOUND(RPDDVIDEOSTREAM_VFW)
	DACOM_INTERFACE_ENTRY(IVideoStreamControl)
	DACOM_INTERFACE_ENTRY2(IID_IVideoStreamControl,IVideoStreamControl)
	END_DACOM_MAP()


public: // Interface

	GENRESULT COMAPI load( const char *filename, struct IFileSystem *, DWORD ivs_f_flags )
	{
		HRESULT hr;
		BITMAPINFO *pbmi;
		RECT r;

		if( target_surface == NULL || filename == NULL ) {
			return GR_GENERIC;
		}

		unload();

		if( !avi_initialized ) {
			AVIFileInit();
			avi_initialized = true;
		}

		if( FAILED( hr = AVIStreamOpenFromFile( &avi_stream, filename, streamtypeVIDEO, 0, OF_READ, NULL ) ) ) {
            return GR_GENERIC;
        }

		// Load the video stream
		if( (avi_frame = AVIStreamGetFrameOpen( avi_stream, NULL )) == NULL ) {
			return GR_GENERIC;
		}

		// Get the stream info
		if( FAILED( hr = AVIStreamInfo( avi_stream, &avi_stream_info, sizeof(AVISTREAMINFO) ) ) ) {
			return GR_GENERIC;
		}

		// Get first frame
		if( FAILED( pbmi = (BITMAPINFO*)AVIStreamGetFrame( avi_frame, 0 ) ) ) {
			return GR_GENERIC;
		}

		if( avi_stream_info.dwScale != 0 ) {
			stream_fps = (double)avi_stream_info.dwRate / (double)avi_stream_info.dwScale;
		}
		else {
			stream_fps = avi_stream_info.dwRate ;
		}

		stream_running_time = avi_stream_info.dwLength / stream_fps ;
		stream_time = 0.0f;

		stream_width = pbmi->bmiHeader.biWidth;
		stream_height = abs( pbmi->bmiHeader.biHeight );
	
		switch( pbmi->bmiHeader.biBitCount ) {
		case 16:	stream_format.init( 16, 5,5,5,0 );	break;
		case 24:	stream_format.init( 24, 8,8,8,0 );	break;
		case 32:	stream_format.init( 32, 8,8,8,8 );	break;
		default:	stream_format.init( 16, 5,5,5,0 );	break;
		}

		stream_pitch = stream_width * ((stream_format.num_bits()+7) >> 3);

		SetRect( &r, 0, 0, stream_width, stream_height );
		if( FAILED( set_stream_target_rect( r ) ) ) {
			return GR_GENERIC;
		}

		if( !(ivs_f_flags & IVSC_F_SILENT) ) {
			stream_volume = 1.0f;
		}
		else {
			stream_volume = 0.0f;
		}

		stream_eos_mode = IVSC_EM_LOOP;
		stream_state = IVSC_SS_LOADED;

		if( ivs_f_flags & IVSC_F_AUTOSTART ) {
			play();
		}

		return GR_OK;
	}

	//


	GENRESULT COMAPI unload( void )
	{
		if( avi_stream ) {
			AVIStreamRelease( avi_stream );
			avi_stream = 0;
		}

		if( avi_initialized ) {
			AVIFileExit();
			avi_initialized = false;
		}
				
		return GR_OK;
	}

	//

	GENRESULT COMAPI update( void )
	{
		ASSERT( avi_frame != NULL );
		ASSERT( target_surface != NULL );
		DDSURFACEDESC2 ddsd;
		HRESULT hr;
		BITMAPINFO *pbmi;
		DWORD current_frame;

		ddsd.dwSize = sizeof(DDSURFACEDESC2);

		ASSERT( target_surface != NULL );

		if( FAILED( hr = target_surface->Lock( NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL ) ) ) {
			return GR_GENERIC;
		}

		current_frame = (DWORD)( stream_time * stream_fps );

		// handle end of stream mode
		//
		if( current_frame >= avi_stream_info.dwLength ) {
			switch( stream_eos_mode ) {
			
			case IVSC_EM_ONCE:
				current_frame = avi_stream_info.dwLength - 1;
				break;

			case IVSC_EM_LOOP:
				stream_time  -= stream_running_time;
				current_frame = (DWORD)( stream_time * stream_fps );
				break;
			}
		}

		// get current frame of the video
		//
		if( FAILED( pbmi = (BITMAPINFO*)AVIStreamGetFrame( avi_frame, current_frame ) ) ) {
			return GR_GENERIC;
		}

		// Copy the current frame image to the surface. We recieve the video data
		// as a void pointer to a packed DIB. We have to skip past the header that
		// preceeds the raw video data.
		//
		char *pSrc  = (char*)( sizeof(BITMAPINFOHEADER) + (BYTE*)pbmi );
		char *pDest = (char*)( ((char*)ddsd.lpSurface) 
							   + target_rect.top * ddsd.lPitch 
							   + target_rect.left * ((target_format.num_bits()+7)>>3) );


#if 0
		// Copy the bits, pixel by pixel. Note that we need to handle the 565 and
		// 555 formats diffrently.
		//
		bool is_565 = (ddsd.ddpfPixelFormat.dwGBitMask==0x7E0)?TRUE:FALSE;

		if( is_565 ) {
			for( DWORD i=0; i < 128*128; i++ ) {
				WORD color = *pSrc++;
				*pDest++ = ((color & 0x1F)|((color & 0xFFE0)<<1));
			}
		}
		else {
			memcpy( pDest, pSrc, sizeof(WORD) * 128 * 128 );
		}
#else


		if(pbmi->bmiHeader.biHeight < 0)
		{
			mem_bitblt( pDest, target_rect.right-target_rect.left, target_rect.bottom-target_rect.top, ddsd.lPitch, target_format,
					pSrc, stream_width, stream_height, stream_pitch, stream_format,
					NULL, NULL );
		}
		else
		{
			mem_bitblt_invert( pDest, target_rect.right-target_rect.left, target_rect.bottom-target_rect.top, ddsd.lPitch, target_format,
					pSrc, stream_width, stream_height, stream_pitch, stream_format,
					NULL, NULL );
		}

#endif

		target_surface->Unlock( NULL );
		
		return GR_OK;
	}

	//

	GENRESULT COMAPI play( void )
	{
		if( stream_state < IVSC_SS_LOADED ) {
			return GR_GENERIC;	
		}

		stream_state = IVSC_SS_PLAYING;

		return GR_OK;
	}

	//

	GENRESULT COMAPI pause( void )
	{
		if( stream_state < IVSC_SS_LOADED ) {
			return GR_GENERIC;
		}

		stream_state = IVSC_SS_PAUSED;

		return GR_OK;
	}

	//

	GENRESULT COMAPI get_info( IVSC_STREAMINFO *stream_info )
	{
		stream_info->file_name = NULL;
		stream_info->file_system = NULL;
		
		stream_info->stream_width = stream_width;
		stream_info->stream_height = stream_height;
		
		stream_info->total_running_time = stream_running_time;
		
		return GR_OK;
	}

	//

	GENRESULT COMAPI get_state( IVSC_STREAMSTATE *out_status )
	{
		*out_status = stream_state;
		return GR_OK;
	}

	//

	GENRESULT COMAPI set_stream_target_rect( RECT _target_rect )
	{
		ASSERT( target_surface != NULL );

		target_rect = _target_rect;

		return GR_OK;
	}

	//

	GENRESULT COMAPI get_stream_target_rect( RECT *out_target_rect )
	{
		out_target_rect->left = target_rect.left;
		out_target_rect->top = target_rect.top;
		out_target_rect->right = target_rect.right;
		out_target_rect->bottom = target_rect.bottom;
		return GR_GENERIC;
	}
	
	//

	GENRESULT COMAPI set_stream_eos_mode( IVSC_EOSMODE mode )
	{
		stream_eos_mode = mode;
		return GR_OK;
	}

	//

	GENRESULT COMAPI get_stream_eos_mode( IVSC_EOSMODE *out_mode )
	{
		*out_mode = stream_eos_mode;
		return GR_OK;
	}

	//

	GENRESULT COMAPI set_stream_time( float time )
	{
		if( stream_state < IVSC_SS_LOADED ) {
			stream_time = 0.0;
			return GR_GENERIC;	// IVS_E_NOT_LOADED;
		}


		switch( stream_eos_mode ) {
		
		case IVSC_EM_ONCE:
			if( time > stream_running_time ) {
				time = stream_running_time;
			}
			break;

		case IVSC_EM_LOOP:
			while( time > stream_running_time ) {
				time -= stream_running_time;
			}
			break;
		}

		stream_time = time;

		return GR_OK;
	}

	//

	GENRESULT COMAPI get_stream_time( float *out_time )
	{
		*out_time = 0.0f;

		if( stream_state < IVSC_SS_LOADED ) {
			return GR_OK;
		}
		
		*out_time = stream_time;

		return GR_GENERIC;
	}

	//

	GENRESULT COMAPI set_stream_frame_rate( float fps )
	{
		stream_fps = fps;
		return GR_GENERIC;
	}

	//

	GENRESULT COMAPI get_stream_frame_rate( float *out_fps )
	{
		*out_fps = stream_fps;
		return GR_OK;
	}

	//

	GENRESULT COMAPI set_stream_volume( float volume )
	{
		return GR_GENERIC;
	}
	
	//
	
	GENRESULT COMAPI get_stream_volume( float *out_volume )
	{
		*out_volume = stream_volume ;
		return GR_OK;
	}

	//

	RPDDVIDEOSTREAM_VFW()
	{
		stream_height = 0;
		stream_width = 0;
		stream_volume = 1.0f;
		stream_time = 0.0f;
		stream_running_time = 0.0;
		stream_state = IVSC_SS_UNINITIALIZED;
	}

	//

	~RPDDVIDEOSTREAM_VFW()
	{
		stream_height = 0;
		stream_width = 0;
	}

	//

	HRESULT internal_initialize( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 _target_surface, U32 flags )
	{
		if( _target_surface == NULL ) {
			return E_FAIL;
		}
				
		DDSURFACEDESC2 ddsd;
		memset( &ddsd, 0, sizeof(ddsd) );
		ddsd.dwSize = sizeof(ddsd);

		SetRect( &target_rect, 0,0,0,0 );
		
		target_surface = _target_surface;
		
		if( SUCCEEDED( _target_surface->Lock( NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL ) ) ) {
			target_format.init( ddsd.ddpfPixelFormat );
			_target_surface->Unlock( NULL );
		}
		else {
			target_format.init( 16, 5,5,5,0 );
		}


		stream_width = 0;
		stream_height = 0;
		stream_format.init( 0,0,0,0,0 );
		stream_pitch = 0;
		
		avi_stream = 0;	
		avi_frame = 0;	

		return S_OK;
	}


public: // Data
	
	U32			stream_width;
	U32			stream_height;
	U32			stream_pitch;
	PixelFormat stream_format;

	RECT						target_rect;
	COMPTR<IDirectDrawSurface7>	target_surface;
	PixelFormat					target_format;

	float stream_running_time;
	float stream_fps;
	float stream_time;
	float stream_volume;

	IVSC_EOSMODE		stream_eos_mode;
	IVSC_STREAMSTATE	stream_state;
	PAVISTREAM    avi_stream;		// The AVI stream
	PGETFRAME     avi_frame;		// Where in the stream to get the next frame
	AVISTREAMINFO avi_stream_info;	// Info about the AVI stream
	bool		  avi_initialized;
};

//




//

HRESULT rp_vfw_init( LPGUID lpGuid, LPDIRECTDRAW7 *lpDD )
{
	return S_OK;
}

//

HRESULT rp_vfw_vstream_control_create( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 target_surface, U32 flags, IVideoStreamControl **out_ivsc )
{
	RPDDVIDEOSTREAM_VFW *vs;

	*out_ivsc = NULL;

	if( (vs = new DAComponent<RPDDVIDEOSTREAM_VFW>()) == NULL ) {
		return E_FAIL;
	}

	if( FAILED( vs->internal_initialize( lpDD, target_surface, flags ) ) ) {
		delete vs;
		return E_FAIL;
	}

	*out_ivsc = vs;

	return S_OK;
}

