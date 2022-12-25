//****************************************************************************
//*                                                                          *
//*  GRAPHICS.H: DA COM real-time frame-oriented screen display manager      *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 9-Mar-97: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  The system component acts as a DA COM-compatible wrapper for the        *
//*  SAL (System Abstraction Layer) library and all system-specific events   *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef GRAPHICS_H
#define GRAPHICS_H

//
// VFX graphics primitive layer
//

#ifndef VFX_H
#include "vfx.h"
#endif

//
// Preferences and configuration equates
//

#define GR_FULLSCREEN     0                // Set fullscreen DDraw mode
#define GR_WINDOW         1                // Set DIB windowed mode
#define GR_TRY_FULLSCREEN 2                // Try fullscreen, fall back to DIB

#define GR_FRONT_SURFACE  0                // Lock front (visible) surface
#define GR_BACK_SURFACE   1                // Lock back (offscreen) surface

//
// Color indices for default system font
//

#define SYSFONT_BKGNDCOLOR 0               // System font background color
#define SYSFONT_TEXTCOLOR  2               // System font character color

//****************************************************************************
//*                                                                          *
//* GRAPHICS structures                                                      *
//*                                                                          *
//****************************************************************************

//
// Video mode information structure
//

struct VIDMODE_INFO
{
   S32 res_X;
   S32 res_Y;
   S32 bpp;
};

//****************************************************************************
//*                                                                          *
//* GRAPHICS class definition                                                *
//*                                                                          *
//****************************************************************************

#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

struct DACOM_NO_VTABLE IGraphics : public IDAComponent
{
   //
   // Set/get graphics display mode
   //

   virtual S32     COMAPI set_display_mode
                                          (S32   res_X, 
                                           S32   res_Y, 
                                           S32   bpp, 
                                           S32   mode, 
                                           S32   allow_mode_switch) = 0;

   virtual void    COMAPI get_display_mode
                                          (S32  *res_X       = NULL,
                                           S32  *res_Y       = NULL,
                                           S32  *pixel_pitch = NULL,
                                           S32  *bpp         = NULL,
                                           S32  *R_shift     = NULL,
                                           U32  *R_mask      = NULL,
                                           S32  *R_width     = NULL,
                                           S32  *G_shift     = NULL,
                                           U32  *G_mask      = NULL,
                                           S32  *G_width     = NULL,
                                           S32  *B_shift     = NULL,
                                           U32  *B_mask      = NULL,
                                           S32  *B_width     = NULL) = 0;

   //
   // Return a pointer to a list of available display modes (obtained
   // from DirectDraw)
   // 

   virtual VIDMODE_INFO * 
                   COMAPI get_display_mode_list 
                                          (S32  *cnt) = 0;

   //
   // Get pointer to VFX graphics primitive provider for currently-active
   // display mode
   //

   virtual IVFX *  COMAPI VFX             (void) = 0;

   //
   // Get pointers to fullscreen VFX_WINDOW and PANE structure, for 
   // convenience of applications and components that don't need to create
   // their own
   //
   // WARNING: Before writing to any pane which is associated with the
   // screen window, you MUST call IGraphics::lock_surface() first to
   // validate the window's buffer pointer.  The write operation should
   // be followed immediately with a call to IGraphics::unlock_surface()
   // to release the surface lock.
   //
   // VFX functions (pixel_write(), string_draw(), etc.) do NOT automatically
   // lock the surface -- this MUST be done by the application or control.
   //

   virtual VFX_WINDOW *
                   COMAPI screen_window   (void) = 0;

   virtual PANE *
                   COMAPI screen_pane     (void) = 0;

   //
   // Get pointer to default system font and its normal and inverse CLUTs
   // (later: make this language-independent!)
   //

   virtual VFX_FONT *
                   COMAPI default_font    (void) = 0;

   virtual void *
                   COMAPI default_font_CLUT
                                          (void) = 0;

   virtual void *
                   COMAPI default_font_inverse_CLUT
                                          (void) = 0;

   //
   // Return time taken by last frame-refresh period, in microseconds
   //

   virtual S32     COMAPI last_frame_time (void) = 0;

   //
   // Return frame # since graphics subsystem was initialized
   //

   virtual S32     COMAPI frame_count     (void) = 0;

   //
   // Lock surface, validating main VFX window buffer
   //
   // Multiple calls may be nested without penalty
   //

   virtual void    COMAPI lock_surface    (void) = 0;

   //
   // Release previously-acquired surface lock
   // 

   virtual void    COMAPI unlock_surface  (void) = 0;
};

#endif
