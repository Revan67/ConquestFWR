//---------------------------------------------------------------------------
/*
	DISPLAY.H

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: December 1997

	Author: Paul Isaac & Bill Baldwin

   $Header: /Libs/Include/display.h 43    5/28/98 2:55p Jasony $
*/
//---------------------------------------------------------------------------

#ifndef _DISPLAY_H
#define _DISPLAY_H

//#include <gl\gl.h>	// standard GL header for reference

#ifndef DACOM_H
#include "DACOM.h"		// IDAComponent
#endif

// WINDOWS TYPES

#ifdef STRICT

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

#else
typedef void *	HANDLE;

typedef HANDLE	HGLRC;
typedef HANDLE	HDC;

typedef int		BOOL;
#endif

//---------------------------------------------------------------------------
// IDisplay class definition
//---------------------------------------------------------------------------

#define VMETHOD(type) virtual type COMAPI

#define DISPLAY_GL		"GL"
#define DISPLAY_GLU		"GLU"
#define DISPLAY_GLUT	"GLUT"

struct IDisplay : public IDAComponent
{
	VMETHOD(bool) set_display_mode (HDC hdc, int x, int y, int bpp) = 0;
	VMETHOD(void) COMAPI restore_display_mode (HDC hdc) = 0;

	VMETHOD(int) load_library (char *name, char *type=DISPLAY_GL) = 0;
	VMETHOD(void) free_library (void) = 0;

	VMETHOD(void *) get_proc_address (char *func_name) = 0;

	VMETHOD(int) get_driver_string (int driver_index, char *dst, int max) = 0;
};

//---------------------------------------------------------------------------
// OpenGL emulation
//---------------------------------------------------------------------------

typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef signed char		GLbyte;
typedef short			GLshort;
typedef int				GLint;
typedef int				GLsizei;
typedef unsigned char	GLubyte;
typedef unsigned short	GLushort;
typedef unsigned int	GLuint;
typedef float			GLfloat;
typedef float			GLclampf;
typedef double			GLdouble;
typedef double			GLclampd;
typedef void			GLvoid;

//---------------------------------------------------------------------------

enum GLenum
{
	GL_NONE					=0,

// Begin(modes)

	GL_POINTS				=0,
	GL_LINES,
	GL_LINE_LOOP,
	GL_LINE_STRIP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_QUADS,
	GL_QUAD_STRIP,
	GL_POLYGON,

// AlphaFunction
	GL_NEVER				=0x0200,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS,

// BlendingFactorDest
	GL_ZERO					=0,
	GL_ONE					=1,
	GL_SRC_COLOR			=0x0300,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
// BlendingFactorSrc
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA_SATURATE,

// DrawBufferMode

//	GL_NONE
	GL_FRONT_LEFT                       =0x0400,
	GL_FRONT_RIGHT,
	GL_BACK_LEFT,
	GL_BACK_RIGHT,
	GL_FRONT,
	GL_BACK,
	GL_LEFT,
	GL_RIGHT,
	GL_FRONT_AND_BACK,
	GL_AUX0,
	GL_AUX1,
	GL_AUX2,
	GL_AUX3,

// ErrorCode

	GL_NO_ERROR                         =0,
	GL_INVALID_ENUM                     =0x0500,
	GL_INVALID_VALUE,
	GL_INVALID_OPERATION,
	GL_STACK_OVERFLOW,
	GL_STACK_UNDERFLOW,
	GL_OUT_OF_MEMORY,

// Fog mode

	GL_EXP								=0x0800,
	GL_EXP2								=0x0801,

// FrontFaceDirection

	GL_CW								=0x0900,
	GL_CCW								=0x0901,

// GetTarget

	GL_CURRENT_COLOR                  =0x0B00,
	GL_CURRENT_INDEX                  =0x0B01,
	GL_CURRENT_NORMAL                 =0x0B02,
	GL_CURRENT_TEXTURE_COORDS         =0x0B03,
	GL_CURRENT_RASTER_COLOR           =0x0B04,
	GL_CURRENT_RASTER_INDEX           =0x0B05,
	GL_CURRENT_RASTER_TEXTURE_COORDS  =0x0B06,
	GL_CURRENT_RASTER_POSITION        =0x0B07,
	GL_CURRENT_RASTER_POSITION_VALID  =0x0B08,
	GL_CURRENT_RASTER_DISTANCE        =0x0B09,
	GL_POINT_SMOOTH                   =0x0B10,
	GL_POINT_SIZE                     =0x0B11,
	GL_POINT_SIZE_RANGE               =0x0B12,
	GL_POINT_SIZE_GRANULARITY         =0x0B13,
	GL_LINE_SMOOTH                    =0x0B20,
	GL_LINE_WIDTH                     =0x0B21,
	GL_LINE_WIDTH_RANGE               =0x0B22,
	GL_LINE_WIDTH_GRANULARITY         =0x0B23,
	GL_LINE_STIPPLE                   =0x0B24,
	GL_LINE_STIPPLE_PATTERN           =0x0B25,
	GL_LINE_STIPPLE_REPEAT            =0x0B26,
	GL_LIST_MODE                      =0x0B30,
	GL_MAX_LIST_NESTING               =0x0B31,
	GL_LIST_BASE                      =0x0B32,
	GL_LIST_INDEX                     =0x0B33,
	GL_POLYGON_MODE                   =0x0B40,
	GL_POLYGON_SMOOTH                 =0x0B41,
	GL_POLYGON_STIPPLE                =0x0B42,
	GL_EDGE_FLAG                      =0x0B43,
	GL_CULL_FACE                      =0x0B44,
	GL_CULL_FACE_MODE                 =0x0B45,
	GL_FRONT_FACE                     =0x0B46,
	GL_LIGHTING                       =0x0B50,
	GL_LIGHT_MODEL_LOCAL_VIEWER       =0x0B51,
	GL_LIGHT_MODEL_TWO_SIDE           =0x0B52,
	GL_LIGHT_MODEL_AMBIENT            =0x0B53,
	GL_SHADE_MODEL                    =0x0B54,
	GL_COLOR_MATERIAL_FACE            =0x0B55,
	GL_COLOR_MATERIAL_PARAMETER       =0x0B56,
	GL_COLOR_MATERIAL                 =0x0B57,
	GL_FOG                            =0x0B60,
	GL_FOG_INDEX                      =0x0B61,
	GL_FOG_DENSITY                    =0x0B62,
	GL_FOG_START                      =0x0B63,
	GL_FOG_END                        =0x0B64,
	GL_FOG_MODE                       =0x0B65,
	GL_FOG_COLOR                      =0x0B66,
	GL_DEPTH_RANGE                    =0x0B70,
	GL_DEPTH_TEST                     =0x0B71,
	GL_DEPTH_WRITEMASK                =0x0B72,
	GL_DEPTH_CLEAR_VALUE              =0x0B73,
	GL_DEPTH_FUNC                     =0x0B74,
	GL_ACCUM_CLEAR_VALUE              =0x0B80,
	GL_STENCIL_TEST                   =0x0B90,
	GL_STENCIL_CLEAR_VALUE            =0x0B91,
	GL_STENCIL_FUNC                   =0x0B92,
	GL_STENCIL_VALUE_MASK             =0x0B93,
	GL_STENCIL_FAIL                   =0x0B94,
	GL_STENCIL_PASS_DEPTH_FAIL        =0x0B95,
	GL_STENCIL_PASS_DEPTH_PASS        =0x0B96,
	GL_STENCIL_REF                    =0x0B97,
	GL_STENCIL_WRITEMASK              =0x0B98,
	GL_MATRIX_MODE                    =0x0BA0,
	GL_NORMALIZE                      =0x0BA1,
	GL_VIEWPORT                       =0x0BA2,
	GL_MODELVIEW_STACK_DEPTH          =0x0BA3,
	GL_PROJECTION_STACK_DEPTH         =0x0BA4,
	GL_TEXTURE_STACK_DEPTH            =0x0BA5,
	GL_MODELVIEW_MATRIX               =0x0BA6,
	GL_PROJECTION_MATRIX              =0x0BA7,
	GL_TEXTURE_MATRIX                 =0x0BA8,
	GL_ATTRIB_STACK_DEPTH             =0x0BB0,
	GL_CLIENT_ATTRIB_STACK_DEPTH      =0x0BB1,
	GL_ALPHA_TEST                     =0x0BC0,
	GL_ALPHA_TEST_FUNC                =0x0BC1,
	GL_ALPHA_TEST_REF                 =0x0BC2,
	GL_DITHER                         =0x0BD0,
	GL_BLEND_DST                      =0x0BE0,
	GL_BLEND_SRC                      =0x0BE1,
	GL_BLEND                          =0x0BE2,
	GL_LOGIC_OP_MODE                  =0x0BF0,
	GL_INDEX_LOGIC_OP                 =0x0BF1,
	GL_COLOR_LOGIC_OP                 =0x0BF2,
	GL_AUX_BUFFERS                    =0x0C00,
	GL_DRAW_BUFFER                    =0x0C01,
	GL_READ_BUFFER                    =0x0C02,
	GL_SCISSOR_BOX                    =0x0C10,
	GL_SCISSOR_TEST                   =0x0C11,
	GL_INDEX_CLEAR_VALUE              =0x0C20,
	GL_INDEX_WRITEMASK                =0x0C21,
	GL_COLOR_CLEAR_VALUE              =0x0C22,
	GL_COLOR_WRITEMASK                =0x0C23,
	GL_INDEX_MODE                     =0x0C30,
	GL_RGBA_MODE                      =0x0C31,
	GL_DOUBLEBUFFER                   =0x0C32,
	GL_STEREO                         =0x0C33,
	GL_RENDER_MODE                    =0x0C40,
	GL_PERSPECTIVE_CORRECTION_HINT    =0x0C50,
	GL_POINT_SMOOTH_HINT              =0x0C51,
	GL_LINE_SMOOTH_HINT               =0x0C52,
	GL_POLYGON_SMOOTH_HINT            =0x0C53,
	GL_FOG_HINT                       =0x0C54,
	GL_TEXTURE_GEN_S                  =0x0C60,
	GL_TEXTURE_GEN_T                  =0x0C61,
	GL_TEXTURE_GEN_R                  =0x0C62,
	GL_TEXTURE_GEN_Q                  =0x0C63,
	GL_PIXEL_MAP_I_TO_I               =0x0C70,
	GL_PIXEL_MAP_S_TO_S               =0x0C71,
	GL_PIXEL_MAP_I_TO_R               =0x0C72,
	GL_PIXEL_MAP_I_TO_G               =0x0C73,
	GL_PIXEL_MAP_I_TO_B               =0x0C74,
	GL_PIXEL_MAP_I_TO_A               =0x0C75,
	GL_PIXEL_MAP_R_TO_R               =0x0C76,
	GL_PIXEL_MAP_G_TO_G               =0x0C77,
	GL_PIXEL_MAP_B_TO_B               =0x0C78,
	GL_PIXEL_MAP_A_TO_A               =0x0C79,
	GL_PIXEL_MAP_I_TO_I_SIZE          =0x0CB0,
	GL_PIXEL_MAP_S_TO_S_SIZE          =0x0CB1,
	GL_PIXEL_MAP_I_TO_R_SIZE          =0x0CB2,
	GL_PIXEL_MAP_I_TO_G_SIZE          =0x0CB3,
	GL_PIXEL_MAP_I_TO_B_SIZE          =0x0CB4,
	GL_PIXEL_MAP_I_TO_A_SIZE          =0x0CB5,
	GL_PIXEL_MAP_R_TO_R_SIZE          =0x0CB6,
	GL_PIXEL_MAP_G_TO_G_SIZE          =0x0CB7,
	GL_PIXEL_MAP_B_TO_B_SIZE          =0x0CB8,
	GL_PIXEL_MAP_A_TO_A_SIZE          =0x0CB9,
	GL_UNPACK_SWAP_BYTES              =0x0CF0,
	GL_UNPACK_LSB_FIRST               =0x0CF1,
	GL_UNPACK_ROW_LENGTH              =0x0CF2,
	GL_UNPACK_SKIP_ROWS               =0x0CF3,
	GL_UNPACK_SKIP_PIXELS             =0x0CF4,
	GL_UNPACK_ALIGNMENT               =0x0CF5,
	GL_PACK_SWAP_BYTES                =0x0D00,
	GL_PACK_LSB_FIRST                 =0x0D01,
	GL_PACK_ROW_LENGTH                =0x0D02,
	GL_PACK_SKIP_ROWS                 =0x0D03,
	GL_PACK_SKIP_PIXELS               =0x0D04,
	GL_PACK_ALIGNMENT                 =0x0D05,
	GL_MAP_COLOR                      =0x0D10,
	GL_MAP_STENCIL                    =0x0D11,
	GL_INDEX_SHIFT                    =0x0D12,
	GL_INDEX_OFFSET                   =0x0D13,
	GL_RED_SCALE                      =0x0D14,
	GL_RED_BIAS                       =0x0D15,
	GL_ZOOM_X                         =0x0D16,
	GL_ZOOM_Y                         =0x0D17,
	GL_GREEN_SCALE                    =0x0D18,
	GL_GREEN_BIAS                     =0x0D19,
	GL_BLUE_SCALE                     =0x0D1A,
	GL_BLUE_BIAS                      =0x0D1B,
	GL_ALPHA_SCALE                    =0x0D1C,
	GL_ALPHA_BIAS                     =0x0D1D,
	GL_DEPTH_SCALE                    =0x0D1E,
	GL_DEPTH_BIAS                     =0x0D1F,
	GL_MAX_EVAL_ORDER                 =0x0D30,
	GL_MAX_LIGHTS                     =0x0D31,
	GL_MAX_CLIP_PLANES                =0x0D32,
	GL_MAX_TEXTURE_SIZE               =0x0D33,
	GL_MAX_PIXEL_MAP_TABLE            =0x0D34,
	GL_MAX_ATTRIB_STACK_DEPTH         =0x0D35,
	GL_MAX_MODELVIEW_STACK_DEPTH      =0x0D36,
	GL_MAX_NAME_STACK_DEPTH           =0x0D37,
	GL_MAX_PROJECTION_STACK_DEPTH     =0x0D38,
	GL_MAX_TEXTURE_STACK_DEPTH        =0x0D39,
	GL_MAX_VIEWPORT_DIMS              =0x0D3A,
	GL_MAX_CLIENT_ATTRIB_STACK_DEPTH  =0x0D3B,
	GL_SUBPIXEL_BITS                  =0x0D50,
	GL_INDEX_BITS                     =0x0D51,
	GL_RED_BITS                       =0x0D52,
	GL_GREEN_BITS                     =0x0D53,
	GL_BLUE_BITS                      =0x0D54,
	GL_ALPHA_BITS                     =0x0D55,
	GL_DEPTH_BITS                     =0x0D56,
	GL_STENCIL_BITS                   =0x0D57,
	GL_ACCUM_RED_BITS                 =0x0D58,
	GL_ACCUM_GREEN_BITS               =0x0D59,
	GL_ACCUM_BLUE_BITS                =0x0D5A,
	GL_ACCUM_ALPHA_BITS               =0x0D5B,
	GL_NAME_STACK_DEPTH               =0x0D70,
	GL_AUTO_NORMAL                    =0x0D80,
	GL_MAP1_COLOR_4                   =0x0D90,
	GL_MAP1_INDEX                     =0x0D91,
	GL_MAP1_NORMAL                    =0x0D92,
	GL_MAP1_TEXTURE_COORD_1           =0x0D93,
	GL_MAP1_TEXTURE_COORD_2           =0x0D94,
	GL_MAP1_TEXTURE_COORD_3           =0x0D95,
	GL_MAP1_TEXTURE_COORD_4           =0x0D96,
	GL_MAP1_VERTEX_3                  =0x0D97,
	GL_MAP1_VERTEX_4                  =0x0D98,
	GL_MAP2_COLOR_4                   =0x0DB0,
	GL_MAP2_INDEX                     =0x0DB1,
	GL_MAP2_NORMAL                    =0x0DB2,
	GL_MAP2_TEXTURE_COORD_1           =0x0DB3,
	GL_MAP2_TEXTURE_COORD_2           =0x0DB4,
	GL_MAP2_TEXTURE_COORD_3           =0x0DB5,
	GL_MAP2_TEXTURE_COORD_4           =0x0DB6,
	GL_MAP2_VERTEX_3                  =0x0DB7,
	GL_MAP2_VERTEX_4                  =0x0DB8,
	GL_MAP1_GRID_DOMAIN               =0x0DD0,
	GL_MAP1_GRID_SEGMENTS             =0x0DD1,
	GL_MAP2_GRID_DOMAIN               =0x0DD2,
	GL_MAP2_GRID_SEGMENTS             =0x0DD3,
	GL_TEXTURE_1D                     =0x0DE0,
	GL_TEXTURE_2D                     =0x0DE1,
	GL_FEEDBACK_BUFFER_POINTER        =0x0DF0,
	GL_FEEDBACK_BUFFER_SIZE           =0x0DF1,
	GL_FEEDBACK_BUFFER_TYPE           =0x0DF2,
	GL_SELECTION_BUFFER_POINTER       =0x0DF3,
	GL_SELECTION_BUFFER_SIZE          =0x0DF4,
//	GL_TEXTURE_BINDING_1D
//	GL_TEXTURE_BINDING_2D
//	GL_VERTEX_ARRAY
//	GL_NORMAL_ARRAY
//	GL_COLOR_ARRAY
//	GL_INDEX_ARRAY
//	GL_TEXTURE_COORD_ARRAY
//	GL_EDGE_FLAG_ARRAY
//	GL_VERTEX_ARRAY_SIZE
//	GL_VERTEX_ARRAY_TYPE
//	GL_VERTEX_ARRAY_STRIDE
//	GL_NORMAL_ARRAY_TYPE
//	GL_NORMAL_ARRAY_STRIDE
//	GL_COLOR_ARRAY_SIZE
//	GL_COLOR_ARRAY_TYPE
//	GL_COLOR_ARRAY_STRIDE
//	GL_INDEX_ARRAY_TYPE
//	GL_INDEX_ARRAY_STRIDE
//	GL_TEXTURE_COORD_ARRAY_SIZE
//	GL_TEXTURE_COORD_ARRAY_TYPE
//	GL_TEXTURE_COORD_ARRAY_STRIDE
//	GL_EDGE_FLAG_ARRAY_STRIDE
//	GL_POLYGON_OFFSET_FACTOR
//	GL_POLYGON_OFFSET_UNITS

// Texture parameters.
	GL_TEXTURE_WIDTH                    =0x1000,
	GL_TEXTURE_HEIGHT                   =0x1001,
	GL_TEXTURE_INTERNAL_FORMAT          =0x1003,
	GL_TEXTURE_COMPONENTS               =GL_TEXTURE_INTERNAL_FORMAT,
	GL_TEXTURE_BORDER_COLOR             =0x1004,
	GL_TEXTURE_BORDER                   =0x1005,


// HintMode

	GL_DONT_CARE						=0x1100,
	GL_FASTEST							=0x1101,
	GL_NICEST							=0x1102,

// LightParameter

	GL_AMBIENT                          =0x1200,
	GL_DIFFUSE                          =0x1201,
	GL_SPECULAR                         =0x1202,
	GL_POSITION                         =0x1203,
	GL_SPOT_DIRECTION                   =0x1204,
	GL_SPOT_EXPONENT                    =0x1205,
	GL_SPOT_CUTOFF                      =0x1206,
	GL_CONSTANT_ATTENUATION             =0x1207,
	GL_LINEAR_ATTENUATION               =0x1208,
	GL_QUADRATIC_ATTENUATION            =0x1209,

// ListMode

	GL_COMPILE							=0x1300,
	GL_COMPILE_AND_EXECUTE				=0x1301,

// DataType

	GL_BYTE								=0x1400,
	GL_UNSIGNED_BYTE,
	GL_SHORT,
	GL_UNSIGNED_SHORT,
	GL_INT,
	GL_UNSIGNED_INT,
	GL_FLOAT,
	GL_2_BYTES,
	GL_3_BYTES,
	GL_4_BYTES,
	GL_DOUBLE,

// MaterialParameter
	GL_EMISSION                         =0x1600,
	GL_SHININESS                        =0x1601,
	GL_AMBIENT_AND_DIFFUSE              =0x1602,
	GL_COLOR_INDEXES                    =0x1603,
//	GL_AMBIENT
//	GL_DIFFUSE
//	GL_SPECULAR

// MatrixMode

	GL_MODELVIEW						=0x1700,
	GL_PROJECTION						=0x1701,
	GL_TEXTURE							=0x1702,

// PixelFormat

	GL_COLOR_INDEX						=0x1900,
	GL_STENCIL_INDEX                  	=0x1901,
	GL_DEPTH_COMPONENT                	=0x1902,
	GL_RED                            	=0x1903,
	GL_GREEN                          	=0x1904,
	GL_BLUE                           	=0x1905,
	GL_ALPHA                          	=0x1906,
	GL_RGB                            	=0x1907,
	GL_RGBA                           	=0x1908,
	GL_LUMINANCE                      	=0x1909,
	GL_LUMINANCE_ALPHA                	=0x190A,

// PolygonMode

	GL_POINT							=0x1B00,
	GL_LINE,
	GL_FILL,

// RenderingMode

	GL_RENDER							=0x1C00,
	GL_FEEDBACK							=0x1C01,
	GL_SELECT							=0x1C02,

// ShadingModel

	GL_FLAT                             =0x1D00,
	GL_SMOOTH                           =0x1D01,

// StringName
	GL_VENDOR							=0x1F00,
	GL_RENDERER							=0x1F01,
	GL_VERSION							=0x1F02,
	GL_EXTENSIONS						=0x1F03,

// TextureCoordName
	GL_S								=0x2000,
	GL_T								=0x2001,
	GL_R								=0x2002,
	GL_Q								=0x2003,

// TexCoordPointerType
//	GL_SHORT
//	GL_INT
//	GL_FLOAT
//	GL_DOUBLE

// TextureEnvMode
	GL_MODULATE							=0x2100,
	GL_DECAL							=0x2101,
//	GL_BLEND
	GL_REPLACE							=0x2103,

// TextureEnvParameter
	GL_TEXTURE_ENV_MODE					=0x2200,
	GL_TEXTURE_ENV_COLOR				=0x2201,

// TextureEnvTarget
	GL_TEXTURE_ENV						=0x2300,

// TextureGenMode
	GL_EYE_LINEAR						=0x2400,
	GL_OBJECT_LINEAR					=0x2401,
	GL_SPHERE_MAP						=0x2402,

// TextureGenParameter
	GL_TEXTURE_GEN_MODE					=0x2500,
	GL_OBJECT_PLANE						=0x2501,
	GL_EYE_PLANE						=0x2502,

// TextureMagFilter
	GL_NEAREST                          =0x2600,
	GL_LINEAR                           =0x2601,
// TextureMinFilter
	GL_NEAREST_MIPMAP_NEAREST           =0x2700,
	GL_LINEAR_MIPMAP_NEAREST            =0x2701,
	GL_NEAREST_MIPMAP_LINEAR            =0x2702,
	GL_LINEAR_MIPMAP_LINEAR             =0x2703,

// TextureParameterName
	GL_TEXTURE_MAG_FILTER               =0x2800,
	GL_TEXTURE_MIN_FILTER               =0x2801,
	GL_TEXTURE_WRAP_S                   =0x2802,
	GL_TEXTURE_WRAP_T                   =0x2803,
//	GL_TEXTURE_BORDER_COLOR
//	GL_TEXTURE_PRIORITY

// TextureWrapMode
	GL_CLAMP                            =0x2900,
	GL_REPEAT                           =0x2901,

// TextureTarget
//	GL_TEXTURE_1D
//	GL_TEXTURE_2D
//	GL_PROXY_TEXTURE_1D
//	GL_PROXY_TEXTURE_2D

// VertexPointerType
//	GL_SHORT
//	GL_INT
//	GL_FLOAT
//	GL_DOUBLE

// ClipPlaneName
	GL_CLIP_PLANE0						=0x3000,
	GL_CLIP_PLANE1						=0x3001,
	GL_CLIP_PLANE2						=0x3002,
	GL_CLIP_PLANE3						=0x3003,
	GL_CLIP_PLANE4						=0x3004,
	GL_CLIP_PLANE5						=0x3005,

// ClientAttribMask
	GL_CLIENT_PIXEL_STORE_BIT			=0x00000001,
	GL_CLIENT_VERTEX_ARRAY_BIT			=0x00000002,
	GL_CLIENT_ALL_ATTRIB_BITS			=0xffffffff,

// polygon_offset
/*
	GL_POLYGON_OFFSET_FACTOR          0x8038
	GL_POLYGON_OFFSET_UNITS           0x2A00
	GL_POLYGON_OFFSET_POINT           0x2A01
	GL_POLYGON_OFFSET_LINE            0x2A02
	GL_POLYGON_OFFSET_FILL            0x8037
*/

// texture
	GL_ALPHA4                         =0x803B,
	GL_ALPHA8                         =0x803C,
	GL_ALPHA12                        =0x803D,
	GL_ALPHA16                        =0x803E,
	GL_LUMINANCE4                     =0x803F,
	GL_LUMINANCE8                     =0x8040,
	GL_LUMINANCE12                    =0x8041,
	GL_LUMINANCE16                    =0x8042,
	GL_LUMINANCE4_ALPHA4              =0x8043,
	GL_LUMINANCE6_ALPHA2              =0x8044,
	GL_LUMINANCE8_ALPHA8              =0x8045,
	GL_LUMINANCE12_ALPHA4             =0x8046,
	GL_LUMINANCE12_ALPHA12            =0x8047,
	GL_LUMINANCE16_ALPHA16            =0x8048,
	GL_INTENSITY                      =0x8049,
	GL_INTENSITY4                     =0x804A,
	GL_INTENSITY8                     =0x804B,
	GL_INTENSITY12                    =0x804C,
	GL_INTENSITY16                    =0x804D,
	GL_R3_G3_B2                       =0x2A10,
	GL_RGB4                           =0x804F,
	GL_RGB5                           =0x8050,
	GL_RGB8                           =0x8051,
	GL_RGB10                          =0x8052,
	GL_RGB12                          =0x8053,
	GL_RGB16                          =0x8054,
	GL_RGBA2                          =0x8055,
	GL_RGBA4                          =0x8056,
	GL_RGB5_A1                        =0x8057,
	GL_RGBA8                          =0x8058,
	GL_RGB10_A2                       =0x8059,
	GL_RGBA12                         =0x805A,
	GL_RGBA16                         =0x805B,
	GL_TEXTURE_RED_SIZE               =0x805C,
	GL_TEXTURE_GREEN_SIZE             =0x805D,
	GL_TEXTURE_BLUE_SIZE              =0x805E,
	GL_TEXTURE_ALPHA_SIZE             =0x805F,
	GL_TEXTURE_LUMINANCE_SIZE         =0x8060,
	GL_TEXTURE_INTENSITY_SIZE         =0x8061,
	GL_PROXY_TEXTURE_1D               =0x8063,
	GL_PROXY_TEXTURE_2D               =0x8064,

// texture_object

	GL_TEXTURE_PRIORITY					=0x8066,
	GL_TEXTURE_RESIDENT					=0x8067,
	GL_TEXTURE_BINDING_1D				=0x8068,
	GL_TEXTURE_BINDING_2D				=0x8069,

// vertex_array

	GL_VERTEX_ARRAY                   =0x8074,
	GL_NORMAL_ARRAY                   =0x8075,
	GL_COLOR_ARRAY                    =0x8076,
	GL_INDEX_ARRAY                    =0x8077,
	GL_TEXTURE_COORD_ARRAY            =0x8078,
	GL_EDGE_FLAG_ARRAY                =0x8079,
	GL_VERTEX_ARRAY_SIZE              =0x807A,
	GL_VERTEX_ARRAY_TYPE              =0x807B,
	GL_VERTEX_ARRAY_STRIDE            =0x807C,
	GL_NORMAL_ARRAY_TYPE              =0x807E,
	GL_NORMAL_ARRAY_STRIDE            =0x807F,
	GL_COLOR_ARRAY_SIZE               =0x8081,
	GL_COLOR_ARRAY_TYPE               =0x8082,
	GL_COLOR_ARRAY_STRIDE             =0x8083,
	GL_INDEX_ARRAY_TYPE               =0x8085,
	GL_INDEX_ARRAY_STRIDE             =0x8086,
	GL_TEXTURE_COORD_ARRAY_SIZE       =0x8088,
	GL_TEXTURE_COORD_ARRAY_TYPE       =0x8089,
	GL_TEXTURE_COORD_ARRAY_STRIDE     =0x808A,
	GL_EDGE_FLAG_ARRAY_STRIDE         =0x808C,
	GL_VERTEX_ARRAY_POINTER           =0x808E,
	GL_NORMAL_ARRAY_POINTER           =0x808F,
	GL_COLOR_ARRAY_POINTER            =0x8090,
	GL_INDEX_ARRAY_POINTER            =0x8091,
	GL_TEXTURE_COORD_ARRAY_POINTER		=0x8092,
	GL_EDGE_FLAG_ARRAY_POINTER			=0x8093,

// Volume clipping hint extension.
	GL_VOLUME_CLIPPING_HINT				=0x80F0,

// Cull vertex.
	GL_CULL_VERTEX		                =0x81AA,
	GL_CULL_VERTEX_EYE_POSITION			=0x81AB,
	GL_CULL_VERTEX_OBJECT_POSITION		=0x81AC,


	GL_V2F                            =0x2A20,
	GL_V3F                            =0x2A21,
	GL_C4UB_V2F                       =0x2A22,
	GL_C4UB_V3F                       =0x2A23,
	GL_C3F_V3F                        =0x2A24,
	GL_N3F_V3F                        =0x2A25,
	GL_C4F_N3F_V3F                    =0x2A26,
	GL_T2F_V3F                        =0x2A27,
	GL_T4F_V4F                        =0x2A28,
	GL_T2F_C4UB_V3F                   =0x2A29,
	GL_T2F_C3F_V3F                    =0x2A2A,
	GL_T2F_N3F_V3F                    =0x2A2B,
	GL_T2F_C4F_N3F_V3F                =0x2A2C,
	GL_T4F_C4F_N3F_V4F                =0x2A2D,

// Light name.

	GL_LIGHT0		= 0x4000,
	GL_LIGHT1		= 0x4001,
	GL_LIGHT2		= 0x4002,
	GL_LIGHT3		= 0x4003,
	GL_LIGHT4		= 0x4004,
	GL_LIGHT5		= 0x4005,
	GL_LIGHT6		= 0x4006,
	GL_LIGHT7		= 0x4007,

// DA Extensions

	GL_FEATURES=0xFF00,

	GL_FEATURE_0, // Ex. GL->Disable(GL_FEATURE_0); // NO TRANSFORM, CLIP, PROJECT, SCALE
	GL_FEATURE_1,
	GL_FEATURE_2,
	GL_FEATURE_3,
	GL_FEATURE_4,
	GL_FEATURE_5,
	GL_FEATURE_6,
	GL_FEATURE_7,

	GL_COLOR_INDEX1_EXT		=0x80E2,
	GL_COLOR_INDEX2_EXT		=0x80E3,
	GL_COLOR_INDEX4_EXT		=0x80E4,
	GL_COLOR_INDEX8_EXT		=0x80E5,
	GL_COLOR_INDEX12_EXT	=0x80E6,
	GL_COLOR_INDEX16_EXT	=0x80E7
};

// Boolean

#define GL_TRUE							1
#define GL_FALSE						0

// AttribMask

#define GL_CURRENT_BIT                    0x00000001
#define GL_POINT_BIT                      0x00000002
#define GL_LINE_BIT                       0x00000004
#define GL_POLYGON_BIT                    0x00000008
#define GL_POLYGON_STIPPLE_BIT            0x00000010
#define GL_PIXEL_MODE_BIT                 0x00000020
#define GL_LIGHTING_BIT                   0x00000040
#define GL_FOG_BIT                        0x00000080
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_ACCUM_BUFFER_BIT               0x00000200
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_VIEWPORT_BIT                   0x00000800
#define GL_TRANSFORM_BIT                  0x00001000
#define GL_ENABLE_BIT                     0x00002000
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_HINT_BIT                       0x00008000
#define GL_EVAL_BIT                       0x00010000
#define GL_LIST_BIT                       0x00020000
#define GL_TEXTURE_BIT                    0x00040000
#define GL_SCISSOR_BIT                    0x00080000
#define GL_ALL_ATTRIB_BITS                0x000fffff

//---------------------------------------------------------------------------

/*
#include <wingdi.h>

typedef struct tagPIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR;
*/

//---------------------------------------------------------------------------
// IGL - graphics layers interface
//---------------------------------------------------------------------------

// #undef BUILD_DISPLAY		= application
// #define BUILD_DISPLAY 1	= compile DISPLAY.dll
// #define BUILD_DISPLAY -1	= compile DISPLAY.dll

#ifndef BUILD_DISPLAY
	#define GLMETHOD(type,name) extern __declspec(dllimport) type (__stdcall *name)
	#define GLUMETHOD(type,name) extern __declspec(dllimport) type (__stdcall name)
#else
	#if (BUILD_DISPLAY == -1)
		#define GLMETHOD(type,name) type __stdcall name
		#define GLUMETHOD(type,name) type __stdcall name
	#else
		#define GLMETHOD(type,name) __declspec(dllexport) type (__stdcall *name)
		#define GLUMETHOD(type,name) __declspec(dllexport) type (__stdcall name)
	#endif
#endif

#ifdef __cplusplus 
extern "C" { 
#endif

GLMETHOD(void,glAccum) (GLenum op, GLfloat value);
GLMETHOD(void,glAlphaFunc) (GLenum func, GLclampf ref);
GLMETHOD(GLboolean,glAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences);
GLMETHOD(void,glArrayElement) (GLint i);
GLMETHOD(void,glBegin) (GLenum mode);
GLMETHOD(void,glBindTexture) (GLenum target, GLuint texture);
GLMETHOD(void,glBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
GLMETHOD(void,glBlendFunc) (GLenum sfactor, GLenum dfactor);
GLMETHOD(void,glCallList) (GLuint list);
GLMETHOD(void,glCallLists) (GLsizei n, GLenum type, const GLvoid *lists);
GLMETHOD(void,glClear) (GLbitfield mask);
GLMETHOD(void,glClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLMETHOD(void,glClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GLMETHOD(void,glClearDepth) (GLclampd depth);
GLMETHOD(void,glClearIndex) (GLfloat c);
GLMETHOD(void,glClearStencil) (GLint s);
GLMETHOD(void,glClipPlane) (GLenum plane, const GLdouble *equation);
GLMETHOD(void,glColor3b) (GLbyte red, GLbyte green, GLbyte blue);
GLMETHOD(void,glColor3bv) (const GLbyte *v);
GLMETHOD(void,glColor3d) (GLdouble red, GLdouble green, GLdouble blue);
GLMETHOD(void,glColor3dv) (const GLdouble *v);
GLMETHOD(void,glColor3f) (GLfloat red, GLfloat green, GLfloat blue);
GLMETHOD(void,glColor3fv) (const GLfloat *v);
GLMETHOD(void,glColor3i) (GLint red, GLint green, GLint blue);
GLMETHOD(void,glColor3iv) (const GLint *v);
GLMETHOD(void,glColor3s) (GLshort red, GLshort green, GLshort blue);
GLMETHOD(void,glColor3sv) (const GLshort *v);
GLMETHOD(void,glColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
GLMETHOD(void,glColor3ubv) (const GLubyte *v);
GLMETHOD(void,glColor3ui) (GLuint red, GLuint green, GLuint blue);
GLMETHOD(void,glColor3uiv) (const GLuint *v);
GLMETHOD(void,glColor3us) (GLushort red, GLushort green, GLushort blue);
GLMETHOD(void,glColor3usv) (const GLushort *v);
GLMETHOD(void,glColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
GLMETHOD(void,glColor4bv) (const GLbyte *v);
GLMETHOD(void,glColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GLMETHOD(void,glColor4dv) (const GLdouble *v);
GLMETHOD(void,glColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLMETHOD(void,glColor4fv) (const GLfloat *v);
GLMETHOD(void,glColor4i) (GLint red, GLint green, GLint blue, GLint alpha);
GLMETHOD(void,glColor4iv) (const GLint *v);
GLMETHOD(void,glColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
GLMETHOD(void,glColor4sv) (const GLshort *v);
GLMETHOD(void,glColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GLMETHOD(void,glColor4ubv) (const GLubyte *v);
GLMETHOD(void,glColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
GLMETHOD(void,glColor4uiv) (const GLuint *v);
GLMETHOD(void,glColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
GLMETHOD(void,glColor4usv) (const GLushort *v);
GLMETHOD(void,glColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLMETHOD(void,glColorMaterial) (GLenum face, GLenum mode);
GLMETHOD(void,glColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
GLMETHOD(void,glCopyTexImage1D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
GLMETHOD(void,glCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLMETHOD(void,glCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GLMETHOD(void,glCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLMETHOD(void,glCullFace) (GLenum mode);
GLMETHOD(void,glDeleteLists) (GLuint list, GLsizei range);
GLMETHOD(void,glDeleteTextures) (GLsizei n, const GLuint *textures);
GLMETHOD(void,glDepthFunc) (GLenum func);
GLMETHOD(void,glDepthMask) (GLboolean flag);
GLMETHOD(void,glDepthRange) (GLclampd zNear, GLclampd zFar);
GLMETHOD(void,glDisable) (GLenum cap);
GLMETHOD(void,glDisableClientState) (GLenum array);
GLMETHOD(void,glDrawArrays) (GLenum mode, GLint first, GLsizei count);
GLMETHOD(void,glDrawBuffer) (GLenum mode);
GLMETHOD(void,glDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GLMETHOD(void,glDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLMETHOD(void,glEdgeFlag) (GLboolean flag);
GLMETHOD(void,glEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glEdgeFlagv) (const GLboolean *flag);
GLMETHOD(void,glEnable) (GLenum cap);
GLMETHOD(void,glEnableClientState) (GLenum array);
GLMETHOD(void,glEnd) (void);
GLMETHOD(void,glEndList) (void);
GLMETHOD(void,glEvalCoord1d) (GLdouble u);
GLMETHOD(void,glEvalCoord1dv) (const GLdouble *u);
GLMETHOD(void,glEvalCoord1f) (GLfloat u);
GLMETHOD(void,glEvalCoord1fv) (const GLfloat *u);
GLMETHOD(void,glEvalCoord2d) (GLdouble u, GLdouble v);
GLMETHOD(void,glEvalCoord2dv) (const GLdouble *u);
GLMETHOD(void,glEvalCoord2f) (GLfloat u, GLfloat v);
GLMETHOD(void,glEvalCoord2fv) (const GLfloat *u);
GLMETHOD(void,glEvalMesh1) (GLenum mode, GLint i1, GLint i2);
GLMETHOD(void,glEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
GLMETHOD(void,glEvalPoint1) (GLint i);
GLMETHOD(void,glEvalPoint2) (GLint i, GLint j);
GLMETHOD(void,glFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer);
GLMETHOD(void,glFinish) (void);
GLMETHOD(void,glFlush) (void);
GLMETHOD(void,glFogf) (GLenum pname, GLfloat param);
GLMETHOD(void,glFogfv) (GLenum pname, const GLfloat *params);
GLMETHOD(void,glFogi) (GLenum pname, GLint param);
GLMETHOD(void,glFogiv) (GLenum pname, const GLint *params);
GLMETHOD(void,glFrontFace) (GLenum mode);
GLMETHOD(void,glFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLMETHOD(GLuint,glGenLists) (GLsizei range);
GLMETHOD(void,glGenTextures) (GLsizei n, GLuint *textures);
GLMETHOD(void,glGetBooleanv) (GLenum pname, GLboolean *params);
GLMETHOD(void,glGetClipPlane) (GLenum plane, GLdouble *equation);
GLMETHOD(void,glGetDoublev) (GLenum pname, GLdouble *params);
GLMETHOD(GLenum,glGetError) (void);
GLMETHOD(void,glGetFloatv) (GLenum pname, GLfloat *params);
GLMETHOD(void,glGetIntegerv) (GLenum pname, GLint *params);
GLMETHOD(void,glGetLightfv) (GLenum light, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetLightiv) (GLenum light, GLenum pname, GLint *params);
GLMETHOD(void,glGetMapdv) (GLenum target, GLenum query, GLdouble *v);
GLMETHOD(void,glGetMapfv) (GLenum target, GLenum query, GLfloat *v);
GLMETHOD(void,glGetMapiv) (GLenum target, GLenum query, GLint *v);
GLMETHOD(void,glGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetMaterialiv) (GLenum face, GLenum pname, GLint *params);
GLMETHOD(void,glGetPixelMapfv) (GLenum map, GLfloat *values);
GLMETHOD(void,glGetPixelMapuiv) (GLenum map, GLuint *values);
GLMETHOD(void,glGetPixelMapusv) (GLenum map, GLushort *values);
GLMETHOD(void,glGetPointerv) (GLenum pname, GLvoid* *params);
GLMETHOD(void,glGetPolygonStipple) (GLubyte *mask);
GLMETHOD(const GLubyte *,glGetString) (GLenum name);
GLMETHOD(void,glGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
GLMETHOD(void,glGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params);
GLMETHOD(void,glGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetTexGeniv) (GLenum coord, GLenum pname, GLint *params);
GLMETHOD(void,glGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
GLMETHOD(void,glGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
GLMETHOD(void,glGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);
GLMETHOD(void,glGetTexParameteriv) (GLenum target, GLenum pname, GLint *params);
GLMETHOD(void,glHint) (GLenum target, GLenum mode);
GLMETHOD(void,glIndexMask) (GLuint mask);
GLMETHOD(void,glIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glIndexd) (GLdouble c);
GLMETHOD(void,glIndexdv) (const GLdouble *c);
GLMETHOD(void,glIndexf) (GLfloat c);
GLMETHOD(void,glIndexfv) (const GLfloat *c);
GLMETHOD(void,glIndexi) (GLint c);
GLMETHOD(void,glIndexiv) (const GLint *c);
GLMETHOD(void,glIndexs) (GLshort c);
GLMETHOD(void,glIndexsv) (const GLshort *c);
GLMETHOD(void,glIndexub) (GLubyte c);
GLMETHOD(void,glIndexubv) (const GLubyte *c);
GLMETHOD(void,glInitNames) (void);
GLMETHOD(void,glInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer);
GLMETHOD(GLboolean,glIsEnabled) (GLenum cap);
GLMETHOD(GLboolean,glIsList) (GLuint list);
GLMETHOD(GLboolean,glIsTexture) (GLuint texture);
GLMETHOD(void,glLightModelf) (GLenum pname, GLfloat param);
GLMETHOD(void,glLightModelfv) (GLenum pname, const GLfloat *params);
GLMETHOD(void,glLightModeli) (GLenum pname, GLint param);
GLMETHOD(void,glLightModeliv) (GLenum pname, const GLint *params);
GLMETHOD(void,glLightf) (GLenum light, GLenum pname, GLfloat param);
GLMETHOD(void,glLightfv) (GLenum light, GLenum pname, const GLfloat *params);
GLMETHOD(void,glLighti) (GLenum light, GLenum pname, GLint param);
GLMETHOD(void,glLightiv) (GLenum light, GLenum pname, const GLint *params);
GLMETHOD(void,glLineStipple) (GLint factor, GLushort pattern);
GLMETHOD(void,glLineWidth) (GLfloat width);
GLMETHOD(void,glListBase) (GLuint base);
GLMETHOD(void,glLoadIdentity) (void);
GLMETHOD(void,glLoadMatrixd) (const GLdouble *m);
GLMETHOD(void,glLoadMatrixf) (const GLfloat *m);
GLMETHOD(void,glLoadName) (GLuint name);
GLMETHOD(void,glLogicOp) (GLenum opcode);
GLMETHOD(void,glMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GLMETHOD(void,glMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GLMETHOD(void,glMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GLMETHOD(void,glMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
GLMETHOD(void,glMapGrid1d) (GLint un, GLdouble u1, GLdouble u2);
GLMETHOD(void,glMapGrid1f) (GLint un, GLfloat u1, GLfloat u2);
GLMETHOD(void,glMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
GLMETHOD(void,glMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
GLMETHOD(void,glMaterialf) (GLenum face, GLenum pname, GLfloat param);
GLMETHOD(void,glMaterialfv) (GLenum face, GLenum pname, const GLfloat *params);
GLMETHOD(void,glMateriali) (GLenum face, GLenum pname, GLint param);
GLMETHOD(void,glMaterialiv) (GLenum face, GLenum pname, const GLint *params);
GLMETHOD(void,glMatrixMode) (GLenum mode);
GLMETHOD(void,glMultMatrixd) (const GLdouble *m);
GLMETHOD(void,glMultMatrixf) (const GLfloat *m);
GLMETHOD(void,glNewList) (GLuint list, GLenum mode);
GLMETHOD(void,glNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
GLMETHOD(void,glNormal3bv) (const GLbyte *v);
GLMETHOD(void,glNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
GLMETHOD(void,glNormal3dv) (const GLdouble *v);
GLMETHOD(void,glNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
GLMETHOD(void,glNormal3fv) (const GLfloat *v);
GLMETHOD(void,glNormal3i) (GLint nx, GLint ny, GLint nz);
GLMETHOD(void,glNormal3iv) (const GLint *v);
GLMETHOD(void,glNormal3s) (GLshort nx, GLshort ny, GLshort nz);
GLMETHOD(void,glNormal3sv) (const GLshort *v);
GLMETHOD(void,glNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLMETHOD(void,glPassThrough) (GLfloat token);
GLMETHOD(void,glPixelMapfv) (GLenum map, GLsizei mapsize, const GLfloat *values);
GLMETHOD(void,glPixelMapuiv) (GLenum map, GLsizei mapsize, const GLuint *values);
GLMETHOD(void,glPixelMapusv) (GLenum map, GLsizei mapsize, const GLushort *values);
GLMETHOD(void,glPixelStoref) (GLenum pname, GLfloat param);
GLMETHOD(void,glPixelStorei) (GLenum pname, GLint param);
GLMETHOD(void,glPixelTransferf) (GLenum pname, GLfloat param);
GLMETHOD(void,glPixelTransferi) (GLenum pname, GLint param);
GLMETHOD(void,glPixelZoom) (GLfloat xfactor, GLfloat yfactor);
GLMETHOD(void,glPointSize) (GLfloat size);
GLMETHOD(void,glPolygonMode) (GLenum face, GLenum mode);
GLMETHOD(void,glPolygonOffset) (GLfloat factor, GLfloat units);
GLMETHOD(void,glPolygonStipple) (const GLubyte *mask);
GLMETHOD(void,glPopAttrib) (void);
GLMETHOD(void,glPopClientAttrib) (void);
GLMETHOD(void,glPopMatrix) (void);
GLMETHOD(void,glPopName) (void);
GLMETHOD(void,glPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
GLMETHOD(void,glPushAttrib) (GLbitfield mask);
GLMETHOD(void,glPushClientAttrib) (GLbitfield mask);
GLMETHOD(void,glPushMatrix) (void);
GLMETHOD(void,glPushName) (GLuint name);
GLMETHOD(void,glRasterPos2d) (GLdouble x, GLdouble y);
GLMETHOD(void,glRasterPos2dv) (const GLdouble *v);
GLMETHOD(void,glRasterPos2f) (GLfloat x, GLfloat y);
GLMETHOD(void,glRasterPos2fv) (const GLfloat *v);
GLMETHOD(void,glRasterPos2i) (GLint x, GLint y);
GLMETHOD(void,glRasterPos2iv) (const GLint *v);
GLMETHOD(void,glRasterPos2s) (GLshort x, GLshort y);
GLMETHOD(void,glRasterPos2sv) (const GLshort *v);
GLMETHOD(void,glRasterPos3d) (GLdouble x, GLdouble y, GLdouble z);
GLMETHOD(void,glRasterPos3dv) (const GLdouble *v);
GLMETHOD(void,glRasterPos3f) (GLfloat x, GLfloat y, GLfloat z);
GLMETHOD(void,glRasterPos3fv) (const GLfloat *v);
GLMETHOD(void,glRasterPos3i) (GLint x, GLint y, GLint z);
GLMETHOD(void,glRasterPos3iv) (const GLint *v);
GLMETHOD(void,glRasterPos3s) (GLshort x, GLshort y, GLshort z);
GLMETHOD(void,glRasterPos3sv) (const GLshort *v);
GLMETHOD(void,glRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLMETHOD(void,glRasterPos4dv) (const GLdouble *v);
GLMETHOD(void,glRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLMETHOD(void,glRasterPos4fv) (const GLfloat *v);
GLMETHOD(void,glRasterPos4i) (GLint x, GLint y, GLint z, GLint w);
GLMETHOD(void,glRasterPos4iv) (const GLint *v);
GLMETHOD(void,glRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w);
GLMETHOD(void,glRasterPos4sv) (const GLshort *v);
GLMETHOD(void,glReadBuffer) (GLenum mode);
GLMETHOD(void,glReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GLMETHOD(void,glRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
GLMETHOD(void,glRectdv) (const GLdouble *v1, const GLdouble *v2);
GLMETHOD(void,glRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
GLMETHOD(void,glRectfv) (const GLfloat *v1, const GLfloat *v2);
GLMETHOD(void,glRecti) (GLint x1, GLint y1, GLint x2, GLint y2);
GLMETHOD(void,glRectiv) (const GLint *v1, const GLint *v2);
GLMETHOD(void,glRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
GLMETHOD(void,glRectsv) (const GLshort *v1, const GLshort *v2);
GLMETHOD(GLint,glRenderMode) (GLenum mode);
GLMETHOD(void,glRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
GLMETHOD(void,glRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GLMETHOD(void,glScaled) (GLdouble x, GLdouble y, GLdouble z);
GLMETHOD(void,glScalef) (GLfloat x, GLfloat y, GLfloat z);
GLMETHOD(void,glScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
GLMETHOD(void,glSelectBuffer) (GLsizei size, GLuint *buffer);
GLMETHOD(void,glShadeModel) (GLenum mode);
GLMETHOD(void,glStencilFunc) (GLenum func, GLint ref, GLuint mask);
GLMETHOD(void,glStencilMask) (GLuint mask);
GLMETHOD(void,glStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
GLMETHOD(void,glTexCoord1d) (GLdouble s);
GLMETHOD(void,glTexCoord1dv) (const GLdouble *v);
GLMETHOD(void,glTexCoord1f) (GLfloat s);
GLMETHOD(void,glTexCoord1fv) (const GLfloat *v);
GLMETHOD(void,glTexCoord1i) (GLint s);
GLMETHOD(void,glTexCoord1iv) (const GLint *v);
GLMETHOD(void,glTexCoord1s) (GLshort s);
GLMETHOD(void,glTexCoord1sv) (const GLshort *v);
GLMETHOD(void,glTexCoord2d) (GLdouble s, GLdouble t);
GLMETHOD(void,glTexCoord2dv) (const GLdouble *v);
GLMETHOD(void,glTexCoord2f) (GLfloat s, GLfloat t);
GLMETHOD(void,glTexCoord2fv) (const GLfloat *v);
GLMETHOD(void,glTexCoord2i) (GLint s, GLint t);
GLMETHOD(void,glTexCoord2iv) (const GLint *v);
GLMETHOD(void,glTexCoord2s) (GLshort s, GLshort t);
GLMETHOD(void,glTexCoord2sv) (const GLshort *v);
GLMETHOD(void,glTexCoord3d) (GLdouble s, GLdouble t, GLdouble r);
GLMETHOD(void,glTexCoord3dv) (const GLdouble *v);
GLMETHOD(void,glTexCoord3f) (GLfloat s, GLfloat t, GLfloat r);
GLMETHOD(void,glTexCoord3fv) (const GLfloat *v);
GLMETHOD(void,glTexCoord3i) (GLint s, GLint t, GLint r);
GLMETHOD(void,glTexCoord3iv) (const GLint *v);
GLMETHOD(void,glTexCoord3s) (GLshort s, GLshort t, GLshort r);
GLMETHOD(void,glTexCoord3sv) (const GLshort *v);
GLMETHOD(void,glTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLMETHOD(void,glTexCoord4dv) (const GLdouble *v);
GLMETHOD(void,glTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLMETHOD(void,glTexCoord4fv) (const GLfloat *v);
GLMETHOD(void,glTexCoord4i) (GLint s, GLint t, GLint r, GLint q);
GLMETHOD(void,glTexCoord4iv) (const GLint *v);
GLMETHOD(void,glTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q);
GLMETHOD(void,glTexCoord4sv) (const GLshort *v);
GLMETHOD(void,glTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glTexEnvf) (GLenum target, GLenum pname, GLfloat param);
GLMETHOD(void,glTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
GLMETHOD(void,glTexEnvi) (GLenum target, GLenum pname, GLint param);
GLMETHOD(void,glTexEnviv) (GLenum target, GLenum pname, const GLint *params);
GLMETHOD(void,glTexGend) (GLenum coord, GLenum pname, GLdouble param);
GLMETHOD(void,glTexGendv) (GLenum coord, GLenum pname, const GLdouble *params);
GLMETHOD(void,glTexGenf) (GLenum coord, GLenum pname, GLfloat param);
GLMETHOD(void,glTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params);
GLMETHOD(void,glTexGeni) (GLenum coord, GLenum pname, GLint param);
GLMETHOD(void,glTexGeniv) (GLenum coord, GLenum pname, const GLint *params);
GLMETHOD(void,glTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLMETHOD(void,glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLMETHOD(void,glTexParameterf) (GLenum target, GLenum pname, GLfloat param);
GLMETHOD(void,glTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
GLMETHOD(void,glTexParameteri) (GLenum target, GLenum pname, GLint param);
GLMETHOD(void,glTexParameteriv) (GLenum target, GLenum pname, const GLint *params);
GLMETHOD(void,glTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GLMETHOD(void,glTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLMETHOD(void,glTranslated) (GLdouble x, GLdouble y, GLdouble z);
GLMETHOD(void,glTranslatef) (GLfloat x, GLfloat y, GLfloat z);
GLMETHOD(void,glVertex2d) (GLdouble x, GLdouble y);
GLMETHOD(void,glVertex2dv) (const GLdouble *v);
GLMETHOD(void,glVertex2f) (GLfloat x, GLfloat y);
GLMETHOD(void,glVertex2fv) (const GLfloat *v);
GLMETHOD(void,glVertex2i) (GLint x, GLint y);
GLMETHOD(void,glVertex2iv) (const GLint *v);
GLMETHOD(void,glVertex2s) (GLshort x, GLshort y);
GLMETHOD(void,glVertex2sv) (const GLshort *v);
GLMETHOD(void,glVertex3d) (GLdouble x, GLdouble y, GLdouble z);
GLMETHOD(void,glVertex3dv) (const GLdouble *v);
GLMETHOD(void,glVertex3f) (GLfloat x, GLfloat y, GLfloat z);
GLMETHOD(void,glVertex3fv) (const GLfloat *v);
GLMETHOD(void,glVertex3i) (GLint x, GLint y, GLint z);
GLMETHOD(void,glVertex3iv) (const GLint *v);
GLMETHOD(void,glVertex3s) (GLshort x, GLshort y, GLshort z);
GLMETHOD(void,glVertex3sv) (const GLshort *v);
GLMETHOD(void,glVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLMETHOD(void,glVertex4dv) (const GLdouble *v);
GLMETHOD(void,glVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLMETHOD(void,glVertex4fv) (const GLfloat *v);
GLMETHOD(void,glVertex4i) (GLint x, GLint y, GLint z, GLint w);
GLMETHOD(void,glVertex4iv) (const GLint *v);
GLMETHOD(void,glVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w);
GLMETHOD(void,glVertex4sv) (const GLshort *v);
GLMETHOD(void,glVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLMETHOD(void,glViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

// EXTENSIONS

GLUMETHOD(void,gluPerspective) (GLdouble fovy, GLdouble aspect, GLdouble z0, GLdouble z1);

// dgluSetColorTables()  Sets global index mapping for R,G,B, and A (optional)
//
// count = # of palette entries
// fmt = GL_RGB or GL_RGBA
// data = address of palette data (unsigned char is assumed data type)
GLUMETHOD(void,dgluSetColorTables) (GLsizei count, GLenum fmt, const GLvoid *data);

// WARNING: This function can be NULL if extension is not supported!!
//
GLMETHOD(void,glColorTableEXT) (GLenum target, GLenum ifmt, GLsizei count, GLenum fmt, GLenum type, const GLvoid *data);


// wgl EXTENSIONS

#if 0
GLMETHOD(BOOL,wglCopyContext) (HGLRC, HGLRC, UINT);
GLMETHOD(HGLRC,wglCreateContext) (HDC);
GLMETHOD(HGLRC,wglCreateLayerContext) (HDC, int);
GLMETHOD(BOOL,wglDeleteContext) (HGLRC);
GLMETHOD(HGLRC,wglGetCurrentContext) (VOID);
GLMETHOD(HDC,wglGetCurrentDC) (VOID);
GLMETHOD(PROC,wglGetProcAddress) (LPCSTR);
GLMETHOD(BOOL,wglMakeCurrent) (HDC, HGLRC);
GLMETHOD(BOOL,wglShareLists) (HGLRC, HGLRC);
GLMETHOD(BOOL,wglUseFontBitmapsA) (HDC, DWORD, DWORD, DWORD);
GLMETHOD(BOOL,wglUseFontBitmapsW) (HDC, DWORD, DWORD, DWORD);
#endif

// related GDI Functions

/*
GLMETHOD(BOOL,SwapBuffers) (HDC dc);
GLMETHOD(int,ChoosePixelFormat) (HDC dc, const PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(int,DescribePixelFormat) (HDC dc, int iPixelFormat, unsigned int nBytes, PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(BOOL,SetPixelFormat) (HDC dc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(int,GetPixelFormat) (HDC dc);
*/

#ifdef __cplusplus 
} 
#endif

//---------------------------------------------------------------------------
// Custom Extensions
//---------------------------------------------------------------------------

typedef void (__stdcall *LOCKBUFFEREXT) (void **, int *);
typedef void (__stdcall *UNLOCKBUFFEREXT) (void);
typedef void (__stdcall *CLEARCOUNTEXT) (void);
typedef GLint (__stdcall *GETCOUNTEXT) (void);
typedef void (__stdcall *SETDUMPTEXTEXT) (struct IDumpText * DUMP);

/*
LOCK BUFFER USAGE: 

	// GLOBAL
	LOCKBUFFEREXT glLockBufferEXT = 0;

	// after LoadLibrary()
	glLockBufferEXT = (LOCKBUFFEREXT)wglGetProcAddress("glLockBufferEXT");

	// normal function call
	void *buffer;
	int stride;
	glLockBufferEXT(&pixels,&stride);

POLY COUNT USAGE:

	// GLOBAL
	CLEARCOUNTEXT glClearCountEXT = 0;
	GETCOUNTEXT glGetCountEXT = 0;

	// after LoadLibrary()
	glClearCountEXT = (CLEARCOUNTEXT)wglGetProcAddress("glClearCountEXT");
	glGetCountEXT = (GETCOUNTEXT)wglGetProcAddress("glGetCountEXT");

	// normal function call
	glClearCountEXT();

  // draw some stuff.
	int num_primitives_drawn = glGetCountEXT();

DEBUG DUMP TEXT USAGE:
	glSetDumpTextEXT (struct IDumpText * DUMP);
	// set the DUMP pointer to override debug printing destination

*/

//
// TEXTURE MEMORY statistics available through the following extensions:
//
// GLuint glGetTotalTextureSizeEXT(void);
//		RETURNS: Total memory used by all textures registered with GL.
//
// GLuint glGetResidentTextureSizeEXT(void);
//		RETURNS: Memory used by textures currently resident in texture memory.
//
// GLuint glGetTextureSizeEXT(GLuint n, const GLuint * textures);
//		RETURNS: Memory used by specified list of textures. Parameters identical
//				 to glDeleteTextures(), etc.
//
// GLuint glGetTotalTextureCountEXT(void);
//		RETURNS: Number of textures registered with GL.
//
// GLuint glGetResidentTextureCountEXT(void);
//		RETURNS: Number of textures currently resident in texture memory.
//
// void	glGetTotalTexturesEXT(GLuint * textures);
//		OUTPUT: Fills in "textures" array with indices of all textures. Be
//				sure your array is large enough by calling 
//				glGetTotalTextureCountEXT() first.
//
// void glGetResidentTexturesEXT(GLuint * textures);
//		OUTPUT: Fills in "textures" array with indices of currently resident
//				textures. Be sure your array is large enough by calling
//				glGetResidentTextureCountEXT() first.
//
// SEE ALSO: glIsTexture(), glAreTexturesResident().
//
typedef GLuint	(__stdcall * GETTOTALTEXTURESIZEEXT)(void);
typedef GLuint	(__stdcall * GETRESIDENTTEXTURESIZEEXT)(void);
typedef GLuint	(__stdcall * GETTEXTURESIZEEXT)(GLuint n, const GLuint * textures);
typedef GLuint	(__stdcall * GETTOTALTEXTURECOUNTEXT)(void);
typedef GLuint	(__stdcall * GETRESIDENTTEXTURECOUNTEXT)(void);
typedef void	(__stdcall * GETTOTALTEXTURESEXT)(GLuint * textures);
typedef void	(__stdcall * GETRESIDENTTEXTURESEXT)(GLuint * textures);

//---------------------------------------------------------------------------
// OUTLINE OF OPENGL COMMANDS
//---------------------------------------------------------------------------

/*

// BUFFERS

	glClear

	glClearColor

	glClearIndex

	glClearDepth

	glFlush
	glFinish

	glScissor

	glRenderMode

	glSelectBuffer

// PERSPECTIVE

	glViewport

	glFrustum

	glOrtho

//	gluOrtho2D
//	gluPerspective

// MATH

	glMatrixMode

	glLoadIdentity
	glLoadMatrix*
	glMultMatrix*

	glPushMatrix
	glPopMatrix

	glRotate*
	glScale*
	glTranslate*

// STATE

	glEnable
	glDisable
	glIsEnabled

	glGetBooleanv
	glGetIntegerv
	glGetFloatv

	glGetString

	glGetError

	glPushAttrib
	glPopAttrib

// LIST

	glGenLists

	glIsList

	glNewList
	glEndList

	glDeleteLists

	glCallList

	glListBase
	glCallLists		// glCallLists(24, GL_UNSIGNED_BYTE, "Hello OpenGL World"); 

	Example:

		int list = glGenLists(1);
		glNewList(list);
			glColor3ub(255,0,0);
			glBegin(...);
				glVertex(x,y,z);
				...
			glEnd();
		glEndList();

		glCallList(list);

		glDeleteLists(list,1);

// FEATURES

	glFrontFace
	glCullFace

	glPolygonMode

	glShadeModel
	glAlphaFunc
	glBlendFunc

	glDepthFunc
	glDepthMask
	glDepthRange

	glFog*

	glHint

	glClipPlane
	glGetClipPlane

	glLineWidth

// PRIMITIVES

	glBegin
	glEnd

	glColor3*
	glColor4*
	glIndex*

	glTexCoord*

	glNormal3*

	glVertex2*
	glVertex3*
	glVertex4*

	glRect*

// ARRAYS

	glEnableClientState
	glDisableClientState

	glArrayElement
	glDrawArrays
	glDrawElements

	glGetPointer*

	glColorPointer
	glNormalPointer
	glTexCoordPointer
	glVertexPointer
	glInterleavedArrays

// LIGHT

	glLight*

	glLightModel*

	glGetLight*

// MATERIALS

	glColorMaterial

	glMaterial*

	glGetMaterial*

// TEXTURE

	glGenTextures

	glIsTexture

	glAreTexturesResident

	glPrioritizeTextures

	glBindTexture

	glDeleteTextures

	glTexParameter*

	glGetTexParameter*

	glGetTexLevelParameter*

	glPixelMap*

	glTexImage1D
	glTexImage2D

	glTexEnv*

	glCopyTexImage2D
	glCopyTexSubImage2D

	glTexSubImage1D
	glTexSubImage2D

// BITMAPS & PIXELS

	glDrawBuffer
	glReadBuffer

	glRasterPos*

	glBitmap

	glCopyPixels

	glReadPixels

	glDrawPixels

// wgl EXTENSIONS

	wglMakeCurrent

	wglSwapBuffers

	wglChoosePixelFormat

	wglDescribePixelFormat

	wglSetPixelFormat

	wglGetPixelFormat
*/

//---------------------------------------------------------------------------

#endif // _DISPLAY_H

