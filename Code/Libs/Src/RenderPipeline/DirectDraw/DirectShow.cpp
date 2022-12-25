// DirectShow.cpp
//
// DirectShow Utility functions
// 
//

#include <windows.h>
#include <ddraw.h>
#include <mmstream.h>   // Multimedia stream interfaces
#include <amstream.h>   // DirectShow multimedia stream interfaces
#include <ddstream.h>   // DirectDraw multimedia stream interfaces
#include <control.h>

//

#include "dacom.h"
#include "fdump.h"
#include "TSmartPointer.h"
#include "TComponent.h"
#include "IVideoStreamControl.h"
#include "TempStr.h"

//

#include "DirectShow.h"
#include "DirectDraw.h"

//

inline STREAM_TIME convert_seconds_to_streamtime( float seconds )
{
	return ( ((double)seconds) * 10000000.00 );
}

inline float convert_streamtime_to_seconds( STREAM_TIME stream_time )
{
	return ((float)( ((double)stream_time) / 10000000.00 ));
}


//

struct RPDDVIDEOSTREAM_DSHOW : public IVideoStreamControl
{
	BEGIN_DACOM_MAP_INBOUND(RPDDVIDEOSTREAM_DSHOW)
	DACOM_INTERFACE_ENTRY(IVideoStreamControl)
	DACOM_INTERFACE_ENTRY2(IID_IVideoStreamControl,IVideoStreamControl)
	END_DACOM_MAP()


public: // Interface

	DEFMETHOD(load)( const char *filename, struct IFileSystem *parent, DWORD ivs_f_flags )
	{
		WCHAR wFile[MAX_PATH];

		if( _IAMMultiMediaStream == NULL || target_surface == NULL ) {
			return GR_GENERIC;
		}

		unload();

		MultiByteToWideChar( CP_ACP, 0, filename, -1, wFile, sizeof(wFile)/sizeof(wFile[0]) );

		HRESULT	ret;

		ret	=_IAMMultiMediaStream->OpenFile( wFile, 0 );

		if( FAILED( ret ) ) {
			GENERAL_WARNING( TEMPSTR( "_IAMMultiMediaStream->OpenFile(%s,0) returned %x", filename, ret ) ) ;
		}

		if( !(ivs_f_flags & IVSC_F_SILENT) ) {
			COMPTR<IGraphBuilder> _IGraphBuilder;
			if( FAILED( _IAMMultiMediaStream->GetFilterGraph( _IGraphBuilder ) ) ) {
				return GR_GENERIC;
			}

			if( FAILED( _IGraphBuilder->QueryInterface( IID_IBasicAudio, (void**) &_IBasicAudio ) ) ) {
				return GR_GENERIC;
			}
		}

		STREAM_TIME dur;

		if( SUCCEEDED( _IAMMultiMediaStream->GetDuration( &dur ) ) ) {
			stream_running_time = convert_streamtime_to_seconds( dur );
		}
		else {
			stream_running_time = 0.0f;
		}

		stream_fps = 0.0f;

		HRESULT hr;
		COMPTR<IMediaStream> _IMediaStream;
		if( SUCCEEDED( hr = _IAMMultiMediaStream->GetMediaStream( MSPID_PrimaryVideo, _IMediaStream ) ) ) {
			
			COMPTR<IDirectDrawMediaStream> _IDirectDrawMediaStream;
			if( SUCCEEDED( hr = _IMediaStream->QueryInterface( IID_IDirectDrawMediaStream, (void **)&_IDirectDrawMediaStream ) ) ) {
			
				STREAM_TIME dur;
				if( SUCCEEDED( _IDirectDrawMediaStream->GetTimePerFrame( &dur ) ) ) {
					stream_fps = convert_streamtime_to_seconds( dur );
					if( stream_fps ) {
						stream_fps = 1.0f / stream_fps;
					}
				}

				DDSURFACEDESC ddsd;
				ddsd.dwSize = sizeof(ddsd);
				if( SUCCEEDED( hr = _IDirectDrawMediaStream->GetFormat( &ddsd, NULL, NULL, NULL ) ) ) {
					stream_width = ddsd.dwWidth;
					stream_height = ddsd.dwHeight;
					format.init( ddsd.ddpfPixelFormat );
				}
			}
		}

		RECT r;
		SetRect( &r, 0, 0, stream_width, stream_height );
		if( FAILED( set_stream_target_rect( r ) ) ) {
			return GR_GENERIC;
		}

		stream_eos_mode = IVSC_EM_LOOP;
		
		state = IVSC_SS_LOADED;

		if( ivs_f_flags & IVSC_F_AUTOSTART ) {
			play();
		}

		return GR_OK;
	}

	//


	DEFMETHOD(unload)( void )
	{
		_IDirectDrawStreamSample = NULL;
		
		return GR_OK;
	}

	//

	DEFMETHOD(update)( void )
	{
		if( _IDirectDrawStreamSample != NULL ) {
			HRESULT hr;
			if( SUCCEEDED( hr = _IDirectDrawStreamSample->Update( 0, NULL, NULL, 0 ) ) ) {
				if( (hr != S_OK) && (stream_eos_mode == IVSC_EM_LOOP) ) {
					hr = _IAMMultiMediaStream->SetState( STREAMSTATE_STOP );
					hr = _IAMMultiMediaStream->Seek( convert_seconds_to_streamtime( stream_time = 0.0f ) );
					hr = _IAMMultiMediaStream->SetState( STREAMSTATE_RUN );
				}

				if( intermediate_surface != NULL ) {
					target_surface->Blt( &stream_target_rect, intermediate_surface, NULL, 0, NULL );
				}

				return GR_OK;
			}
		}
		
		return GR_GENERIC;
	}

	//

	DEFMETHOD(play)( void )
	{
		if( state == IVSC_SS_PLAYING ) {
			return GR_OK;
		}

		if( state < IVSC_SS_LOADED ) {
			return GR_GENERIC;	// IVS_E_NOT_LOADED;
		}

		HRESULT hr;
		if( SUCCEEDED( hr = _IAMMultiMediaStream->SetState( STREAMSTATE_RUN ) ) ) {
			state = IVSC_SS_PLAYING;
			return GR_OK;
		}

		state = IVSC_SS_ERROR;

		return GR_GENERIC;
	}

	//

	DEFMETHOD(pause)( void )
	{
		if( state == IVSC_SS_PAUSED ) {
			return GR_OK;
		}

		if( state < IVSC_SS_LOADED ) {
			return GR_GENERIC;	// IVS_E_NOT_LOADED;
		}

		HRESULT hr;
		if( SUCCEEDED( hr = _IAMMultiMediaStream->SetState( STREAMSTATE_STOP ) ) ) {
			state = IVSC_SS_PAUSED;
			return GR_OK;
		}

		state = IVSC_SS_ERROR;

		return GR_GENERIC;
	}

	//

	DEFMETHOD(get_info)( IVSC_STREAMINFO *stream_info )
	{
		stream_info->file_name = NULL;
		stream_info->file_system = NULL;
		
		stream_info->stream_width = stream_width;
		stream_info->stream_height = stream_height;
		
		stream_info->total_running_time = stream_running_time;
		
		return GR_OK;
	}

	//

	DEFMETHOD(get_state)( IVSC_STREAMSTATE *out_status )
	{
		*out_status = state;
		return GR_OK;
	}

	//

	DEFMETHOD(set_stream_target_rect)( RECT target_rect )
	{
		HRESULT hr;
		COMPTR<IMediaStream> _IMediaStream;
		COMPTR<IDirectDrawMediaStream> _IDirectDrawMediaStream;
		COMPTR<IDirectDrawSurface> lpDDS;

		ASSERT( target_surface != NULL );

		if( FAILED( hr = _IAMMultiMediaStream->GetMediaStream( MSPID_PrimaryVideo, _IMediaStream ) ) ) {
			return GR_GENERIC;
		}

		if( FAILED( hr = _IMediaStream->QueryInterface( IID_IDirectDrawMediaStream, (void **)&_IDirectDrawMediaStream ) ) ) {
			return GR_GENERIC;
		}


		if( FAILED( hr = target_surface->QueryInterface( IID_IDirectDrawSurface, (void **) &lpDDS ) ) ) {
			return GR_GENERIC;	
		}

		DDSURFACEDESC ddsd;
		ddsd.dwSize = sizeof (ddsd);
		lpDDS->GetSurfaceDesc( &ddsd );
		
		unsigned int target_width  = target_rect.right - target_rect.left;
		unsigned int target_height = target_rect.bottom - target_rect.top;

		if( (ddsd.dwWidth != stream_width || ddsd.dwHeight != stream_height) ||
			(target_width < stream_width || target_height < stream_height) 
			) {

			PixelFormat iformat( ddsd.ddpfPixelFormat );
			if( SUCCEEDED( rp_dd_create_surface( _IDirectDraw7, RPDD_CSF_DYNAMIC, DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY, stream_width, stream_height, 0, &iformat, intermediate_surface ) ) ) {

				if( FAILED( hr = intermediate_surface->QueryInterface( IID_IDirectDrawSurface, (void **) &lpDDS ) ) ) {
					return GR_GENERIC;	
				}

				lpDDS->GetSurfaceDesc( &ddsd );

				if( SUCCEEDED( hr = _IDirectDrawMediaStream->SetFormat( &ddsd, NULL ) ) ) {
					
					RECT sample_rect;
					SetRect( &sample_rect, 0, 0, stream_width, stream_height );

					memcpy( &stream_target_rect, &target_rect, sizeof(stream_target_rect) );

					if( SUCCEEDED( hr = _IDirectDrawMediaStream->CreateSample( lpDDS, &sample_rect, DDSFF_PROGRESSIVERENDER, _IDirectDrawStreamSample ) ) ) {
						return GR_OK;
					} 
				}

			}
		}
		else {

			if( SUCCEEDED( hr = _IDirectDrawMediaStream->SetFormat( &ddsd, NULL ) ) ) {

				memcpy( &stream_target_rect, &target_rect, sizeof(stream_target_rect) );
//				SetRect( &stream_target_rect, 0, 0, stream_width, stream_height ); 
				if( SUCCEEDED( hr = _IDirectDrawMediaStream->CreateSample( lpDDS, &stream_target_rect, DDSFF_PROGRESSIVERENDER, _IDirectDrawStreamSample ) ) ) {
					return GR_OK;
				} 
			}
		}
		
		return GR_GENERIC;
	}

	//

	DEFMETHOD(get_stream_target_rect)( RECT *out_target_rect )
	{
		out_target_rect->left = stream_target_rect.left;
		out_target_rect->top = stream_target_rect.top;
		out_target_rect->right = stream_target_rect.right;
		out_target_rect->bottom = stream_target_rect.bottom;
		return GR_GENERIC;
	}
	
	//

	DEFMETHOD(set_stream_eos_mode)( IVSC_EOSMODE mode )
	{
		stream_eos_mode = mode;
		return GR_OK;
	}

	//

	DEFMETHOD(get_stream_eos_mode)( IVSC_EOSMODE *out_mode )
	{
		*out_mode = stream_eos_mode;
		return GR_OK;
	}

	//

	DEFMETHOD(set_stream_time)( float time )
	{
		if( state < IVSC_SS_LOADED ) {
			stream_time = 0.0;
			return GR_GENERIC;	// IVS_E_NOT_LOADED;
		}

		if( time > stream_running_time ) {
			time -= stream_running_time;
		}

		STREAM_TIME seek_time = convert_seconds_to_streamtime( time );
		if( SUCCEEDED( _IAMMultiMediaStream->Seek( seek_time ) ) ) {
			stream_time = time;
			return GR_OK;
		}

		stream_time = 0.0;
		state = IVSC_SS_ERROR;

		return GR_GENERIC;
	}

	//

	DEFMETHOD(get_stream_time)( float *out_time )
	{
		*out_time = 0.0f;

		if( state < IVSC_SS_LOADED ) {
			return GR_OK;
		}
		

		STREAM_TIME seek_time;
		if( SUCCEEDED( _IAMMultiMediaStream->GetTime( &seek_time ) ) ) {
			*out_time = convert_streamtime_to_seconds( seek_time );
			return GR_OK;
		}

		return GR_GENERIC;
	}

	//

	DEFMETHOD(set_stream_frame_rate)( float fps )
	{
		return GR_GENERIC;
	}

	//

	DEFMETHOD(get_stream_frame_rate)( float *out_fps )
	{
		*out_fps = stream_fps;
		return GR_OK;
	}

	//

	DEFMETHOD(set_stream_volume)( float volume )
	{
		// map [0.0, 1.0] ->  [-10000, 0] 
		volume = __max( 0.0f, __min( volume, 1.0f ) );
		float db = (volume - 1.0f) * 10000.00f;

		if( SUCCEEDED( _IBasicAudio->put_Volume( db ) ) ) {
			stream_volume = volume;
			return GR_OK;
		}

		return GR_GENERIC;
	}
	
	//
	
	DEFMETHOD(get_stream_volume)( float *out_volume )
	{
		*out_volume = stream_volume ;
		return GR_OK;
	}

	//

	RPDDVIDEOSTREAM_DSHOW()
	{
		stream_height = 0;
		stream_width = 0;
		stream_volume = 1.0f;
		stream_time = 0.0f;
		stream_running_time = 0.0;
		state = IVSC_SS_UNINITIALIZED;
	}

	//

	~RPDDVIDEOSTREAM_DSHOW()
	{
		stream_height = 0;
		stream_width = 0;
	}

	//

	HRESULT internal_initialize( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 _target_surface, U32 flags )
	{
		HRESULT hr;

		if( !_target_surface ) {
			return E_FAIL;
		}
				
		target_surface = _target_surface;
		
		_IDirectDraw7 = lpDD;

		//Create the AMMultiMediaStream object
		if( FAILED( hr = CoCreateInstance( CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, (void **)&_IAMMultiMediaStream ) ) ) {
			return E_FAIL;
		}

			//Initialize stream
		if( FAILED( hr = _IAMMultiMediaStream->Initialize( STREAMTYPE_READ, 0, NULL ) ) ) {
			return E_FAIL;
		}

		if( FAILED( hr = _IAMMultiMediaStream->AddMediaStream( NULL, &MSPID_PrimaryVideo, 0, NULL ) ) ) {
			return E_FAIL;
		}

		stream_volume = 0.0f;
		if( !(flags & RPVS_F_SILENT) ) {
			if( FAILED( hr = _IAMMultiMediaStream->AddMediaStream( NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL ) ) ) {
				GENERAL_TRACE_1( "RPDDVIDEOSTREAM_DSHOW: internal_initialize: no sound stream will be used\n" );
			}
			stream_volume = 1.0f;
		}

		stream_width = 0;
		stream_height = 0;

		return S_OK;
	}


public: // Data
	
	PixelFormat format;

	U32 stream_width;
	U32 stream_height;
	float stream_running_time;
	float stream_fps;
	float stream_time;
	float stream_volume;
	IVSC_EOSMODE stream_eos_mode;
	RECT stream_target_rect;

	IVSC_STREAMSTATE state;

	COMPTR<IAMMultiMediaStream>		_IAMMultiMediaStream;
	COMPTR<IBasicAudio>				_IBasicAudio;
	COMPTR<IDirectDrawStreamSample> _IDirectDrawStreamSample;
	COMPTR<IMediaSeeking>			_IMediaSeeking;
	COMPTR<IDirectDraw7>			_IDirectDraw7;

	COMPTR<IDirectDrawSurface7>		target_surface;
	COMPTR<IDirectDrawSurface7>		intermediate_surface;
};

//




//

HRESULT rp_ds_init( LPGUID lpGuid, LPDIRECTDRAW7 *lpDD )
{
	return S_OK;
}

//

HRESULT rp_ds_vstream_control_create( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 target_surface, U32 flags, IVideoStreamControl **out_ivsc )
{
	RPDDVIDEOSTREAM_DSHOW *vs;

	*out_ivsc = NULL;

	if( (vs = new DAComponent<RPDDVIDEOSTREAM_DSHOW>()) == NULL ) {
		return E_FAIL;
	}

	if( FAILED( vs->internal_initialize( lpDD, target_surface, flags ) ) ) {
		delete vs;
		return E_FAIL;
	}

	*out_ivsc = vs;

	return S_OK;
}

