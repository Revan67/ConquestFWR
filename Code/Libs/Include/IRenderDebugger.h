// IRenderDebugger.h
//
//

#ifndef IRenderDebugger_H
#define IRenderDebugger_H

//

#define IID_IRenderDebugger MAKE_IID( "IRenderDebugger", 1 )

//

typedef U32 IRD_STATETYPE;

const IRD_STATETYPE IRD_ST_TEXTURE			= (1<<0);	// textures
const IRD_STATETYPE IRD_ST_TEXTURE_STATE	= (1<<1);	// texture state
const IRD_STATETYPE IRD_ST_TEXTURE_TRANSFORM= (1<<2);	// texture transforms
const IRD_STATETYPE IRD_ST_TRANSFORM		= (1<<3);	// world, view, projection, and viewport transforms
const IRD_STATETYPE IRD_ST_PRIMITIVE		= (1<<4);	// dp, dip, dpvb, dipvb
const IRD_STATETYPE IRD_ST_LIGHT			= (1<<5);	// light and light enable values 
const IRD_STATETYPE IRD_ST_RENDER_STATE		= (1<<6);	// render state
const IRD_STATETYPE IRD_ST_TEXTURE_DATA		= (1<<7);	// save texture data to .dds files

const IRD_STATETYPE IRD_ST_OFF = 0;						// no state
const IRD_STATETYPE IRD_ST_ALL = 0xFFFFFFFF;			// all state
const IRD_STATETYPE IRD_ST_MOST= ~(IRD_ST_TEXTURE_DATA);// most useful stuff

//


//
// IRenderDebugger
//
//
struct IRenderDebugger : public IDAComponent
{
	// dump all of the selected state
	//
	virtual void COMAPI dump_current_state( const IRD_STATETYPE state_traces, const char *directory ) = 0; 

	// trace state changes
	//
	virtual void COMAPI set_trace_enable( const IRD_STATETYPE state_traces ) = 0; 
	virtual void COMAPI clear_trace_capture( void ) = 0;
	virtual void COMAPI set_trace_output_dir( const char *directory ) = 0;
	virtual void COMAPI add_trace_message( const char *fmt, ... ) = 0;
	virtual void COMAPI begin_trace_section( const char *fmt, ... ) = 0;
	virtual void COMAPI end_trace_section( void ) = 0;
	virtual void COMAPI save_trace_capture( const char *filename ) = 0;	

	// use the wacky extended frustum viewing
	//
	virtual void COMAPI set_frustum_view_enable( bool onoff ) = 0;
	virtual void COMAPI set_frustum_view_options( void ) = 0;

	// set delays for debugging, 0.0f will be no delay (the default)
	//
	virtual void COMAPI set_sb_delay( float milliseconds ) = 0;	// swap_buffers(), 
	virtual void COMAPI set_dp_delay( float milliseconds ) = 0;	// draw_*_primitive_*(), 

	// save a texture to the .DDS format
	//
	virtual void COMAPI save_texture( const char *filename, U32 irp_texture_handle ) = 0;
	
	// save the render target buffer to the .DDS format
	// 
	// if delay_to_swap_buffers is true, this schedules the save to occur just before
	// the next actual swap_buffers()
	//
	virtual void COMAPI save_screen_capture( const char *filename, bool delay_to_swap_buffers ) = 0;

	// (quickly) enable the reference rasterizer
	//
	virtual void COMAPI set_ref_enable( bool onoff ) = 0;
};


#endif // EOF
