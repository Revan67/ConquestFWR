#ifndef APP_CONFIGURATION_H
#define APP_CONFIGURATION_H

#include "resource.h"

const char * const ini_sound_section = "Sound";
const char * const ini_playback_name = "Sound";
const char * const ini_capture_name = "Capture";
const U32 ds_app_min_bit_depth = 16;
const U32 ds_app_num_buffers = 3; // front, back, z - used to calc memory needed for available modes
const U32 ds_app_mem_multiplier = ds_app_num_buffers * ds_app_min_bit_depth / 8;

// resolutions allowed by this game
const BOOL ds_app_supported_resolutions[V_RES_LAST_RES] = {	false,	
															false,	//320x240
															true,	//512x384
															true,	//640x480
															true,	//800x600
															true,	//1024
															true,	//1152
															true,	//1280
															false	//1600
													};

// renderers that will be enumerated for this game
const U32 supported_renderers = RPUL_DIRECT3D | RPUL_RASTER; // | RPUL_OPENGL;

// game specific options
typedef enum {
	FIRST_CONFIGURABLE_OPTION	,
	OPTION_LOCKING	= FIRST_CONFIGURABLE_OPTION	,	
	OPTION_D3DTEXTURE			,	
	OPTION_HW_RENDERER			,
	OPTION_GDI					,
	OPTION_HW_CURSOR			,

	LAST_CONFIGURABLE_OPTION

} CONFIGURABLE_OPTIONS;

// [0] = id of control
const U32 CONTROL_ID = 0;
// [1] = current state of control
const U32 CURRENT_STATE = 1;
// [2] = default state of control
const U32 DEFAULT_STATE = 2;

U32 Configurable_Options[LAST_CONFIGURABLE_OPTION][3] = {
															{IDC_LOCKING,0,0},
															{IDC_D3DTEXTURE,0,0},
															{0,0,0},
															{IDC_GDI, 0,0},
															{IDC_HW_CURSOR, 0,1}
														};
#endif // APP_CONFIGURATION_H