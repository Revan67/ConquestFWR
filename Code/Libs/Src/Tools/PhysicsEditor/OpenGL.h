//---------------------------------------------------------------------------
/*
	OpenGL.H

	(c) 1998 Digital Anvil

	08-04-98 created (pci)

	$Header: /Tools/dev/Physics Editor/OpenGL.h 3     12/16/99 6:23p Kbaird $
*/
//---------------------------------------------------------------------------

#ifndef OPENGL_H
#define OPENGL_H

#include "main.h"

#define __gl_h_
#define __GL_H__

//---------------------------------------------------------------------------

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

// fuck it
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA_EXT                       0x80E1

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
// App-Side EMULATION Layer
//---------------------------------------------------------------------------

typedef unsigned char uchar;

namespace OpenGL
{
// STATES

	void glDisable (GLenum f);
	void glEnable (GLenum f);

	void glDepthFunc (GLenum func);
	void glDepthMask (GLboolean b);

// TEXUTRE

// VERTICES

	void glBegin (GLenum m);
	void glEnd (void);

	void glColor3ub (uchar r, uchar g, uchar b);
	void glColor3ubv (const uchar *rgb);
	void glColor3f (float r, float g, float b);
	void glColor3fv (const float *rgb);
	void glColor4ub (uchar r, uchar g, uchar b, uchar a);
	void glColor4f (float r, float g, float b, float a);

	void glTexCoord2f (float u, float v);
	void glTexCoord2fv (const float *uv);

	void glVertex2f (float x, float y);
	void glVertex3f (float x, float y, float z);
	void glVertex3fv (const float *xyz);

// EXTENSIONS

	void glCube(Vector center, SINGLE xd, SINGLE yd, SINGLE zd);

/*
// UNUSED  --- not bother to reimplement properly under dx7+ (tlp)
//				USE RENDERPIPELINE INTERFACE
	
	void glClearColor (float r, float g, float b, float a);
	void glClearDepth (float depth);	// 0..1 (far)
	void glClear (int bits);

	void glBindTexture (GLenum target, GLuint texture);
	void glBlendFunc (GLenum sfactor, GLenum dfactor);

	void glTexEnvi (GLenum t, GLenum pname, GLenum value);
	void glTexParameteri (GLenum target, GLenum pname, GLint param); 
	void glTexImage2D (GLenum target, GLint level, GLint components, GLsizei width, GLsizei height,
						GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void glColorTableEXT (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data); 
	void glGenTextures (int count, GLuint *txms); 
	void glDeleteTextures (int count, GLuint *txms);
	bool glIsTexture (GLuint txm);

	void glGetIntegerv (GLenum param, void *ptr);

	void glVertexArray (Vector *list, int count);
	void glArrayElement (int v);

	void glScissor (int x, int y, int w, int h);

	void glRotatef (float angle, float x, float y, float z);
	void glScalef (float x, float y, float z);

	void glFlush (void);

	void glFogi (GLenum pname, int param);
	void glFogf (GLenum pname, float param);
	void glFogfv (GLenum pname, const float *params);

	void glCullFace(GLenum mode);

	void glHint (GLenum target, GLenum mode);
*/
}

//---------------------------------------------------------------------------

using namespace OpenGL;

#endif // OPENGL_H

