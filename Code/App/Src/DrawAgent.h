#ifndef DRAWAGENT_H
#define DRAWAGENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DrawAgent.h                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/App/Src/DrawAgent.h 8     9/14/00 11:37a Rmarr $

   A DrawAgent can draw a 2D image to the screen using the "best" implementation,
   given the circumstances. 
		1) Lock the frame buffer, and use VFX_shape_draw(),
		2) Load image as a texture, draw a quad to screen using GL
		3) Draw pixels using drawPixels().
*/
//--------------------------------------------------------------------------//
 
#ifndef DACOM_H
#include <DACOM.h>
#endif

typedef struct _pane PANE;


struct IDrawAgent : IDAComponent
{
	virtual void __stdcall Draw (PANE *pane, S32 x, S32 y) = 0;

	virtual void __stdcall Draw (PANE *src, PANE *dst) = 0;
	
	virtual void __stdcall GetDimensions (U16 & width, U16 & height) const = 0;
};


struct IFontDrawAgent : IDAComponent
{
	virtual U32 __stdcall GetFontHeight (void) const = 0;

	virtual U32 __stdcall GetStringWidth (const wchar_t *string) = 0;

	virtual U32 __stdcall GetStringWidth (U32 dwID) = 0;

	virtual void __stdcall StringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string) = 0;
	
	virtual void __stdcall StringDraw (PANE *pane, S32 x, S32 y, U32 dwID) = 0;

	virtual GENRESULT __stdcall CreateDuplicate (IFontDrawAgent ** agent) = 0;

	virtual int __stdcall GetCharWidth (wchar_t c) = 0;

	virtual void __stdcall SetFontColor (COLORREF fg,COLORREF bg) = 0;

	virtual bool __stdcall IsMultiLine (void) = 0;

	virtual int  __stdcall GetNumRows (const wchar_t * string, PANE * pane) = 0;
};

struct IDebugFontDrawAgent : IDAComponent
{
	virtual U32 __stdcall GetFontHeight (void) const = 0;

	virtual U32 __stdcall GetStringWidth (const char *string) const = 0;

	virtual void __stdcall StringDraw (PANE *pane, S32 x, S32 y, const char *string, COLORREF color=-1) = 0;
};

void EnableFastDrawAgent (bool bEnable);

#endif