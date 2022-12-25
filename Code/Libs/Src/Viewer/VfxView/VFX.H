//****************************************************************************
//*                                                                          *
//*   VFX.H: DA COM wrapper for WINVFX library                               *
//*                                                                          *
//*   Source compatible with 32-bit 80386 C/C++                              *
//*                                                                          *
//*   V1.00 of 21-Jul-96: Initial version, derived from VFX.H 1.16           *
//*                                                                          *
//*   32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0       *
//*                                                                          *
//*   Project: 386FX Sound & Light(TM)                                       *
//*    Author: Ken Arnold, John Miles, John Lemberger                        *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*   Copyright (C) 1992-1996 Miles Design, Inc.                             *
//*                                                                          *
//*   Miles Design, Inc.                                                     *
//*   8301 Elander Drive                                                     *
//*   Austin, TX 78750                                                       *
//*                                                                          *
//*   70322.2457@compuserve.com                                              *
//*   (512) 345-2642 / FAX (512) 338-9630 / BBS (512) 454-9990               *
//*                                                                          *
//****************************************************************************

#ifndef VFX_VERSION

#define VFX_VERSION      "1.00"
#define VFX_VERSION_DATE "21-Jul-96"

#endif

#ifndef VFX_H
#define VFX_H

#ifdef WIN32
  #define IS_WIN32 1
#endif

#ifdef _WIN32
  #define IS_WIN32 1
#endif

//
// If compiling VFX library, use __declspec(dllexport) for both 
// declarations and definitions
//
// If compiling VFX application, use __declspec(dllimport) for
// declarations -- definitions don't matter
//

#ifdef IS_WIN32
#undef DXDEC
#undef DXDEF

  #ifdef BUILD_VFX
    #define DXDEC __declspec(dllexport)
    #define DXDEF __declspec(dllexport)
  #else
    #define DXDEC __declspec(dllimport)
  #endif

#else

  #define DXDEC
  #define DXDEF
  #define VFXEXPORT
  #define WINAPI

#endif

//
// General type definitions for portability
// 

#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

#ifndef FIX_TYPEDEFS

  typedef long F16;           // 16:16 fixed-point type [-32K,+32K]
  typedef long F30;           // 2:30 fixed-point type [-1.999,+1.999]

#endif

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

//
// Preference names and default values
//

#define N_VFX_PREFS                  0     // # of preference types

//
// Misc. definitions
//

#define SHAPE_FILE_VERSION '01.1' // 1.10 backwards for big-endian compare

#define GIF_SCRATCH_SIZE 20526L   // Temp memory req'd for GIF decompression

//
// VFX_map_polygon() flags
//

#define MP_XLAT      0x0001       // Use lookaside table (speed loss = ~9%)
#define MP_XP        0x0002       // Enable transparency (speed loss = ~6%)

//
// VFX_shape_transform() flags
//

#define ST_XLAT      0x0001       // Use shape_lookaside() table
#define ST_REUSE     0x0002       // Use buffer contents from prior call

//
// VFX_line_draw() modes
//  

#define LD_DRAW      0
#define LD_TRANSLATE 1
#define LD_EXECUTE   2

//
// VFX_pane_scroll() modes
//

#define PS_NOWRAP    0
#define PS_WRAP      1

#define NO_COLOR -1

//
// VFX_shape_visible_rectangle() mirror values
//

#define VR_NO_MIRROR 0
#define VR_X_MIRROR  1
#define VR_Y_MIRROR  2
#define VR_XY_MIRROR 3

//
// Transparent color keys for 8-bit and high-color modes
//

#define PAL_TRANSPARENT 255       // Default transparent color for many primitives
#define RGB_TRANSPARENT 0xfffe    // Reserved 16-bit RGB transparency key

//
// PANE_LIST.flags values
//

#define PL_FREE      0           // Free and available for assignment
#define PL_VALID     1           // Assigned; to be refreshed
#define PL_CONTAINED 2           // Contained within another pane; don't refresh

//
// Window flags
//

#define VWF_BUFF_OWNED 0x0001    // Set if VFX owns buffer memory, else clear
#define VWF_FRONT_LOCK 0x0002    // Window buffer = locked front surface
#define VWF_BACK_LOCK  0x0004    // Window buffer = locked back surface

//
// Mode/surface equates (compatible with SAL)
// 

#define SAL_FRONT_SURFACE 0
#define SAL_BACK_SURFACE  1

#define VFX_FULLSCREEN_MODE SAL_FULLSCREEN      // Set fullscreen DDraw mode
#define VFX_WINDOW_MODE     SAL_WINDOW          // Set DIB windowed mode
#define VFX_TRY_FULLSCREEN  SAL_TRY_FULLSCREEN  // Try fullscreen, fall back to DIB

#define VFX_FRONT_SURFACE   SAL_FRONT_SURFACE   // VFX_lock_window_surface()
#define VFX_BACK_SURFACE    SAL_BACK_SURFACE


//
// VFX data structures
//

#pragma pack(1)                  // Do NOT allow compiler to reorder structs!

typedef struct
{
   S32 X_size;                   // Width of window for which stencil was made
   S32 Y_size;                   // Height of window

   S32 dir[1];                   // Stencil row-offset directory [height],
}                                // followed by stencil packet data...
VFX_STENCIL;

typedef struct _window
{
	void  *buffer;                // Pointer to window buffer

	S32    x_max;                 // Maximum X-coordinate in window [0,x_max)
	S32    y_max;                 // Maximum Y-coordinate in window [0,y_max)
                                 
   S32    pixel_pitch;           // # of bytes between adjacent pixels
   S32    bytes_per_pixel;       // # of bytes to write per pixel
                 
   //
   // RGB shift/mask values for mode active when window was created
   //

   S32    R_left;                // # of bits left to shift component
   S32    R_right;               // # of bits right to shift 8-bit component
   U32    R_mask;                // Component mask
   S32    R_width;               // # of bits in component

   S32    G_left;
   S32    G_right;
   U32    G_mask;
   S32    G_width;

   S32    B_left;
   S32    B_right;
   U32    B_mask;
   S32    B_width;

   S32    flags;
} 
VFX_WINDOW;

typedef struct _pane
{
	VFX_WINDOW *window;
	S32 x0;
	S32 y0;
	S32 x1;
	S32 y1;
} 
PANE;

typedef struct _pane_list
{
   PANE  *array;
   U32   *flags;
   S32    list_size;
}
PANE_LIST;

#ifndef VFX_RGB_DEFINED
#define VFX_RGB_DEFINED

typedef struct    // Binary-compatible with SAL_RGB
{
   U8 r;
   U8 g;
   U8 b;
}
VFX_RGB;

#endif

typedef struct
{
   U8      color;
   VFX_RGB rgb;
}
VFX_CRGB;

typedef struct
{
   S32 x;
   S32 y;
}
VFX_POINT;

typedef struct
{
   S32 version;
   S32 char_count;
   S32 char_height;
   S32 font_background;

   //
   // Font data follows header
   //
}
VFX_FONT;

typedef struct
{
   U32 version;
   U32 shape_count;

   //
   // rest of table...
   //
}
VFX_SHAPETABLE;

typedef struct        // Vertex structure used by VFX3D polygon primitives
{
   S32 x;             // Screen X
   S32 y;             // Screen Y

   F16 color;         // Color/addition value used by some primitives

   F16 u;             // Texture source X
   F16 v;             // Texture source Y
   F30 w;             // Homogeneous perspective divisor (unused by VFX3D)
}
SCRNVERTEX;               

typedef struct tagVFX_RECT
{
   S32 x0;
   S32 y0;
   S32 x1;
   S32 y1;
}
VFX_RECT;

#pragma pack()       // Restore default structure-packing convention

#define INT_TO_F16(x)       (((long)(int)(x)) << 16)
#define DOUBLE_TO_F16(x)    ((long) ((x) * 65536.0 + 0.5))
#define F16_TO_DOUBLE(x)    (((double)(x)) / 65536.0)
#define F16_TO_SINGLE(x)    (((float)(x)) / 65536.0)
#define F16_TO_INT(x)       ((int) ((x)<0 ? -(-(x) >> 16) : (x) >> 16))
#define ROUND_F16_TO_INT(x) ((int) ((x)<0 ? -((32768-(x)) >> 16) : ((x)+32768) >> 16))

#define F16_TO_F30(x)       ((x) << 14)
#define F30_TO_F16(x)       ((x) >> 14)
#define F30_TO_DOUBLE(x)    (((double)x) / 1073741824.0)
#define F30_TO_SINGLE(x)    (((float)x) / 1073741824.0)
#define DOUBLE_TO_F30(x)    ((long) (x * 1073741824.0 + 0.5))
#define SINGLE_TO_F30(x)    ((long) (x * 1073741824.0 + 0.5))

#define PIXELS_IN_PANE(pane)    (((pane).x1-(pane).x0+1)*((pane).y1-(pane).y0+1))
#define PIXELS_IN_PANEP(pane)   (((pane)->x1-(pane)->x0+1)*((pane)->y1-(pane)0>y0+1))

//
// Macro to allow direct specification of 8-8-8 RGB triplet instead of
// global palette index
//
// The following functions accept either a global palette index (0-255)
// or RGB_NATIVE / RGB_TRIPLET() value:
//
//   line_draw()
//   ellipse_draw()
//   ellipse_fill()
//   pane_wipe()
//   pane_scroll()
//   pixel_write()
//   rectangle_hash()
//

#define RGB_NATIVE(r,g,b) triplet_value(r,g,b) | 0x80000000

#define RGB_TRIPLET(r,g,b) ((((r) >> 3) << 10) | \
                            (((g) >> 3) << 5)  | \
                            (((b) >> 3) << 0)  | \
                            0x40000000)

//****************************************************************************
//*                                                                          *
//* VFX wrapper class definition                                             *
//*                                                                          *
//****************************************************************************

#ifdef __cplusplus   // Exclude when running H2INC 

#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

//
// Use VFXDESC on call to CreateInstance
//

struct VFXDESC : public AGGDESC
{
   S32 bits_per_pixel;

   VFXDESC (S32 _bpp = 16, const C8 *_interface_name = "IVFX") : AGGDESC(_interface_name)
      {
      bits_per_pixel = _bpp;
	  size = sizeof(*this);
      }
};

struct IVFX : public IDAComponent
{
   //
   // Device-independent VFX API functions (WINVFXxx.CPP)
   //
   
   virtual S32  COMAPI set_display_mode      (S32             display_size_X,
                                              S32             display_size_Y,
                                              S32             display_bpp,
                                              S32             initial_window_mode,
                                              S32             allow_mode_switch) = 0;
   
   virtual void COMAPI lock_window_surface   (VFX_WINDOW     *window, 
                                              S32             surface = VFX_BACK_SURFACE) = 0;
   
   virtual void COMAPI unlock_window_surface (VFX_WINDOW     *window, 
                                              S32             perform_flip = TRUE) = 0;
   
   virtual void COMAPI set_palette_entry     (S32             index,
                                              VFX_RGB        *entry,
                                              S32             wait_flag) = 0;
                                                
   virtual void COMAPI get_palette_entry     (S32             index,
                                              VFX_RGB        *entry) = 0;
   
   virtual void COMAPI set_palette_range     (S32             index,
                                              S32             num_entries,
                                              VFX_RGB        *entry_list,
                                              S32             wait_flag) = 0;
                                                
   virtual void COMAPI get_palette_range     (S32             index,
                                              S32             num_entries,
                                              VFX_RGB        *entry_list) = 0;
   
   virtual U32  COMAPI pixel_value           (VFX_RGB        *VFX_RGB) = 0;
   
   virtual U32  COMAPI triplet_value         (U32 r, 
                                              U32 g, 
                                              U32 b) = 0;
   
   virtual VFX_RGB * 
                COMAPI RGB_value             (U32             native_pixel) = 0;

   virtual VFX_RGB *
                COMAPI color_to_RGB          (U32             color) = 0;

   virtual U32  COMAPI stencil_size          (VFX_WINDOW     *source, 
                                              U32             transparent_color) = 0;
   
   virtual VFX_STENCIL * 
                COMAPI stencil_construct     (VFX_WINDOW     *source, 
                                              VFX_STENCIL    *dest, 
                                              U32             transparent_color) = 0;
   
   virtual void COMAPI stencil_destroy       (VFX_STENCIL    *stencil) = 0;
   
   virtual VFX_WINDOW * 
                COMAPI window_construct      (S32             width, 
                                              S32             height) = 0;
   
   virtual void * 
                COMAPI assign_window_buffer  (VFX_WINDOW     *window,
                                              void           *buffer = 0,
                                              S32             pitch  = -1) = 0;
   
   virtual void COMAPI window_destroy        (VFX_WINDOW     *window) = 0;
   
   virtual PANE * 
                COMAPI pane_construct        (VFX_WINDOW     *window, 
                                              S32             x0, 
                                              S32             y0, 
                                              S32             x1, 
                                              S32             y1) = 0;
   
   virtual void COMAPI pane_destroy          (PANE           *pane) = 0;
   
   virtual PANE_LIST * 
                COMAPI pane_list_construct   (S32             n_entries) = 0;
   
   virtual void COMAPI pane_list_destroy     (PANE_LIST      *list) = 0;
   
   virtual void COMAPI pane_list_clear       (PANE_LIST      *list) = 0;
   
   virtual S32  COMAPI pane_list_add         (PANE_LIST      *list, 
                                              PANE           *target) = 0;
   
   virtual S32  COMAPI pane_list_add_area    (PANE_LIST      *list, 
                                              VFX_WINDOW     *window, 
                                              S32             x0, 
                                              S32             y0,
                                              S32             x1, 
                                              S32             y1) = 0;
   
   virtual void COMAPI pane_list_delete_entry(PANE_LIST      *list, 
                                              S32             entry_num) = 0;

   virtual S32  COMAPI pane_list_identify_point
                                             (PANE_LIST      *list,
                                              S32             x,
                                              S32             y) = 0;
   
   virtual PANE *
                COMAPI pane_list_get_entry   (PANE_LIST      *list,
                                              S32             entry_num) = 0;

   virtual void COMAPI pane_list_refresh     (PANE_LIST      *list) = 0;
   
   //
   // Device-independent VFX API functions (WINVFXxx.ASM)
   //
   
   virtual S32  COMAPI line_draw             (PANE           *pane, 
                                              S32             x0, 
                                              S32             y0, 
                                              S32             x1, 
                                              S32             y1, 
                                              S32             mode, 
                                              U32             parm) = 0;
   
   virtual void COMAPI shape_draw            (PANE           *pane, 
                                              VFX_SHAPETABLE *shape_table,
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY) = 0;
   
   virtual void COMAPI shape_lookaside       (U8             *table) = 0;
   
   virtual void COMAPI shape_translate_draw  (PANE           *pane, 
                                              VFX_SHAPETABLE *shape_table,
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY) = 0;
   
   virtual void COMAPI shape_transform       (PANE           *pane,
                                              VFX_SHAPETABLE *shape_table, 
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY,
                                              void           *buffer, 
                                              S32             rot, 
                                              S32             x_scale, 
                                              S32             y_scale, 
                                              U32             flags) = 0;
   
   virtual void COMAPI shape_area_translate  (PANE           *pane,
                                              VFX_SHAPETABLE *shape_table, 
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY,
                                              void           *buffer, 
                                              S32             rot, 
                                              S32             x_scale, 
                                              S32             y_scale, 
                                              U32             flags,
                                              void           *lookaside) = 0;

   virtual void COMAPI shape_remap_colors    (VFX_SHAPETABLE *shape_table,
                                              U32             shape_number) = 0;
   
   virtual void COMAPI shape_visible_rectangle
                                             (VFX_SHAPETABLE *shape_table,
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY,
                                              S32             mirror, 
                                              VFX_RECT       *rectangle) = 0;
   
   virtual S32  COMAPI shape_scan            (PANE           *pane, 
                                              U32             transparent_color,
                                              S32             hotX, 
                                              S32             hotY, 
                                              VFX_SHAPETABLE *shape_table) = 0;
   
   virtual S32  COMAPI shape_bounds          (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num) = 0;
   
   virtual S32  COMAPI shape_origin          (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num) = 0;
   
   virtual S32  COMAPI shape_resolution      (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num) = 0;
   
   virtual S32  COMAPI shape_minxy           (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num) = 0;
   
   virtual void COMAPI shape_palette         (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num,
                                              VFX_RGB        *palette) = 0;
   
   virtual S32  COMAPI shape_colors          (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_num,
                                              VFX_CRGB       *colors) = 0;
   
   virtual S32  COMAPI shape_set_colors      (VFX_SHAPETABLE *shape_table, 
                                              S32             shape_number,
                                              VFX_CRGB       *colors) = 0;
   
   virtual S32  COMAPI shape_count           (VFX_SHAPETABLE *shape_table) = 0;
   
   virtual S32  COMAPI shape_list            (VFX_SHAPETABLE *shape_table, 
                                              U32            *index_list) = 0;
   
   virtual S32  COMAPI shape_palette_list    (VFX_SHAPETABLE *shape_table, 
                                              U32            *index_list) = 0;
   
   virtual U32  COMAPI pixel_write           (PANE           *pane, 
                                              S32             x, 
                                              S32             y, 
                                              U32             color) = 0;
   
   virtual U32  COMAPI pixel_read            (PANE           *pane, 
                                              S32             x, 
                                              S32             y) = 0;
   
   virtual S32  COMAPI rectangle_hash        (PANE           *pane, 
                                              S32             x0, 
                                              S32             y0,
                                              S32             x1, 
                                              S32             y1, 
                                              U32             color) = 0;
                                                              
   virtual S32  COMAPI pane_wipe             (PANE           *pane, 
                                              U32             color) = 0;
   
   virtual S32  COMAPI pane_copy             (PANE           *source, 
                                              S32             sx, 
                                              S32             sy,
                                              PANE           *target, 
                                              S32             tx, 
                                              S32             ty, 
                                              S32             fill) = 0;
   
   virtual S32  COMAPI pane_scroll           (PANE           *pane, 
                                              S32             dx, 
                                              S32             dy,
                                              S32             mode, 
                                              S32             parm) = 0;
   
   virtual void COMAPI ellipse_draw          (PANE           *pane, 
                                              S32             xc, 
                                              S32             yc, 
                                              S32             width, 
                                              S32             height, 
                                              U32             color) = 0;
   
   virtual void COMAPI ellipse_fill          (PANE           *pane, 
                                              S32             xc, 
                                              S32             yc, 
                                              S32             width, 
                                              S32             height, 
                                              U32             color) = 0;
   
   virtual void COMAPI point_transform       (VFX_POINT      *in, 
                                              VFX_POINT      *out, 
                                              VFX_POINT      *origin,
                                              S32             rot, 
                                              S32             x_scale, 
                                              S32             y_scale) = 0;
   
   virtual void COMAPI Cos_Sin               (S32             angle, 
                                              F16            *cos, 
                                              F16            *sin) = 0;
   
   virtual void COMAPI fixed_mul             (F16             M1, 
                                              F16             M2,
                                              F16            *result) = 0;
   
   virtual S32  COMAPI font_height           (VFX_FONT       *font) = 0;
   
   virtual S32  COMAPI character_width       (VFX_FONT       *font, 
                                              S32             character) = 0;
   
   virtual S32  COMAPI character_draw        (PANE           *pane, 
                                              S32             x, 
                                              S32             y, 
                                              VFX_FONT       *font,
                                              S32             character, 
                                              void           *color_translate) = 0;
   
   virtual void COMAPI string_draw           (PANE           *pane, 
                                              S32             x, 
                                              S32             y, 
                                              VFX_FONT       *font,
                                              const char     *string, 
                                              void           *color_translate) = 0;
   
   virtual S32  COMAPI ILBM_draw             (PANE           *pane, 
                                              void           *ILBM_buffer) = 0;
   
   virtual void COMAPI ILBM_palette          (void           *ILBM_buffer, 
                                              VFX_RGB        *palette) = 0;
   
   virtual S32  COMAPI ILBM_resolution       (void           *ILBM_buffer) = 0;
   
   virtual void COMAPI PCX_draw              (PANE           *pane, 
                                              S32             PCX_file_size, 
                                              void           *PCX_buffer) = 0;
   
   virtual void COMAPI PCX_palette           (void           *PCX_buffer, 
                                              S32             PCX_file_size,
                                              VFX_RGB        *palette) = 0;
   
   virtual S32  COMAPI PCX_resolution        (void           *PCX_buffer) = 0;
   
   virtual S32  COMAPI GIF_draw              (PANE           *pane, 
                                              void           *GIF_buffer) = 0;
   
   virtual void COMAPI GIF_palette           (void           *GIF_buffer, 
                                              VFX_RGB        *palette) = 0;
   
   virtual S32  COMAPI GIF_resolution        (void           *GIF_buffer) = 0;
   
   virtual S32  COMAPI color_scan            (PANE           *pane, 
                                              U32            *colors) = 0;
   
   //
   // VFX 2D polygon functions (originally from VFX3D.ASM)
   //
   // These legacy functions all use the global palette in high-color mode, 
   // except flat_polygon(), which can (optionally) use the VFX_RGB_TRIPLET() 
   // macro to work with explicit VFX_RGB values, and translate_polygon(), 
   // which uses a native-pixel-format lookup table exclusively.
   //
   
   virtual void COMAPI flat_polygon          (PANE           *pane, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist) = 0;
   
   virtual void COMAPI Gouraud_polygon       (PANE           *pane, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist) = 0;
   
   virtual void COMAPI dithered_Gouraud_polygon
                                             (PANE           *pane, 
                                              F16             dither_amount, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist) = 0;
   
   virtual void COMAPI map_lookaside         (U8             *table) = 0;
   
   virtual void COMAPI map_polygon           (PANE           *pane, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist,
                                              VFX_WINDOW     *texture, 
                                              U32             flags) = 0;
   
   virtual void COMAPI translate_polygon     (PANE           *pane, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist,
                                              void           *lookaside) = 0;
   
   virtual void COMAPI illuminate_polygon    (PANE           *pane, 
                                              F16             dither_amount, 
                                              S32             vcnt, 
                                              SCRNVERTEX     *vlist) = 0;

   virtual S32  COMAPI shape_point_opacity   (VFX_SHAPETABLE *shape_table,
                                              S32             shape_number, 
                                              S32             hotX, 
                                              S32             hotY,
                                              S32             testX, 
                                              S32             testY) = 0;
};

#endif
#endif
