// IVideoStreamControl.h
//
//
//


#ifndef IVideoStreamControl_H
#define IVideoStreamControl_H

//

#include "dacom.h"
#include "FileSys.h"

//

#define IVSC_F_AUTOSTART	(1<<0)		// start stream once opened
#define IVSC_F_STREAM		(1<<1)		// request stream be streamed from disk
#define IVSC_F_SILENT		(1<<2)		// don't add audio stream

//

struct IVSC_STREAMINFO
{
	const char *file_name;
	IFileSystem *file_system;
	
	U32  stream_width;
	U32  stream_height;
	
	float total_running_time;
};

//

enum IVSC_STREAMSTATE {
	
	IVSC_SS_UNINITIALIZED,
	IVSC_SS_LOADED,
	IVSC_SS_PLAYING,
	IVSC_SS_PAUSED,
	IVSC_SS_COMPLETED,
	IVSC_SS_ERROR
};

//

enum IVSC_EOSMODE {
	IVSC_EM_ONCE,
	IVSC_EM_LOOP
};


#define IID_IVideoStreamControl "IVideoStreamControl"

//

struct DACOM_NO_VTABLE IVideoStreamControl : public IDAComponent
{
	DEFMETHOD(load)( const char *filename, struct IFileSystem *parent, DWORD ivs_f_flags ) = 0;
	DEFMETHOD(unload)( void ) = 0;
	
	DEFMETHOD(play)( void ) = 0;
	DEFMETHOD(pause)( void ) = 0;
	DEFMETHOD(update)( void ) = 0;

	DEFMETHOD(get_info)( IVSC_STREAMINFO *stream_info ) = 0;
	DEFMETHOD(get_state)( IVSC_STREAMSTATE *out_status ) = 0;

	DEFMETHOD(set_stream_target_rect)( RECT target_rect ) = 0;
	DEFMETHOD(get_stream_target_rect)( RECT *out_target_rect ) = 0;

	DEFMETHOD(set_stream_eos_mode)( IVSC_EOSMODE mode ) = 0;
	DEFMETHOD(get_stream_eos_mode)( IVSC_EOSMODE *out_mode ) = 0;

	DEFMETHOD(set_stream_time)( float time ) = 0;
	DEFMETHOD(get_stream_time)( float *out_time ) = 0;

	DEFMETHOD(set_stream_frame_rate)( float fps ) = 0;
	DEFMETHOD(get_stream_frame_rate)( float *out_fps ) = 0;

	DEFMETHOD(set_stream_volume)( float volume ) = 0;
	DEFMETHOD(get_stream_volume)( float *out_volume ) = 0;
};


#endif // EOF

