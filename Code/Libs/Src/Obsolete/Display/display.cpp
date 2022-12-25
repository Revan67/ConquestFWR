//---------------------------------------------------------------------------
/*
	DISPLAY.CPP

	Copyright (C) 1997 Digital Anvil, Inc.

	Created: December 1997

	Author: Paul Isaac
*/
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define USE_HEAP 1

#include <stdio.h>	// sprintf

#define BUILD_DISPLAY 1
#include "display.h"

#include "draw.h"

#include "dacom.h"						// ICOManager
#include "TComponent.h"
#include "IProfileParser.h"
#include "IDumpText.h"

#if USE_HEAP
#include "HeapObj.h"
#endif

#define EMULATE_EXT 0					// provide emulation for extensions not supported by driver

// How many elements in a static array?
#define countof(list) (sizeof(list)/sizeof(list[0]))

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

C8 InterfaceName[] = "IDisplay";		// Interface name used for registration     

ICOManager *DACOM = 0;					// Handle to component manager
IDumpText * DUMP = 0;

void DebugPrint (char *fmt, ...);
//----------------------------------------------------------------------------
// STUB functions
//----------------------------------------------------------------------------

void StubFunction (void)
{
	DebugPrint("Display: Stub Function! (no context)\n");
}

void __stdcall v0 (void) { StubFunction(); }
void __stdcall v1 (int) { StubFunction(); }
void __stdcall v2 (int, int) { StubFunction(); }
void __stdcall v3 (int, int, int) { StubFunction(); }
void __stdcall v4 (int, int, int, int) { StubFunction(); }
void __stdcall v5 (int, int, int, int, int) { StubFunction(); }
void __stdcall v6 (int, int, int, int, int, int) { StubFunction(); }
void __stdcall v7 (int, int, int, int, int, int, int) { StubFunction(); }
void __stdcall v8 (int, int, int, int, int, int, int, int) { StubFunction(); }
void __stdcall v9 (int, int, int, int, int, int, int, int, int) { StubFunction(); }

void __stdcall v10 (double, double, double, double, double) { StubFunction(); }
void __stdcall v14 (double, double, double, double, double, double, double) { StubFunction(); }

void __stdcall v1d (double) { StubFunction(); }
void __stdcall v2d (double, double) { StubFunction(); }
void __stdcall v3d (double, double, double) { StubFunction(); }
void __stdcall v4d (double, double, double, double) { StubFunction(); }
void __stdcall v5d (double, double, double, double, double) { StubFunction(); }
void __stdcall v6d (double, double, double, double, double, double) { StubFunction(); }

int __stdcall i0 (void) { StubFunction(); return 0; }
int __stdcall i1 (int) { StubFunction(); return 0; }
int __stdcall i2 (int, int) { StubFunction(); return 0; }
int __stdcall i3 (int, int, int) { StubFunction(); return 0; }
int __stdcall i4 (int, int, int, int) { StubFunction(); return 0; }
int __stdcall i5 (int, int, int, int, int) { StubFunction(); return 0; }
int __stdcall i8 (int, int, int, int, int, int, int, int) { StubFunction(); return 0; }

//----------------------------------------------------------------------------
// INTERNAL POINTERS
//----------------------------------------------------------------------------

GLMETHOD(BOOL,_wglCopyContext) (HGLRC, HGLRC, UINT);
GLMETHOD(HGLRC,_wglCreateContext) (HDC);
GLMETHOD(HGLRC,_wglCreateLayerContext) (HDC, int);
GLMETHOD(BOOL,_wglDeleteContext) (HGLRC);
GLMETHOD(HGLRC,_wglGetCurrentContext) (VOID);
GLMETHOD(HDC,_wglGetCurrentDC) (VOID);
GLMETHOD(PROC,_wglGetProcAddress) (LPCSTR);
GLMETHOD(BOOL,_wglMakeCurrent) (HDC, HGLRC);
GLMETHOD(BOOL,_wglShareLists) (HGLRC, HGLRC);
GLMETHOD(BOOL,_wglUseFontBitmapsA) (HDC, DWORD, DWORD, DWORD);
GLMETHOD(BOOL,_wglUseFontBitmapsW) (HDC, DWORD, DWORD, DWORD);

GLMETHOD(int,_wglDescribeLayerPlane) (void);
GLMETHOD(int,_wglGetLayerPaletteEntries) (void);
GLMETHOD(int,_wglRealizeLayerPalette) (void);
GLMETHOD(int,_wglSetLayerPaletteEntries) (void);
GLMETHOD(int,_wglSwapLayerBuffers) (void);
GLMETHOD(int,_wglUseFontOutlinesA) (void);
GLMETHOD(int,_wglUseFontOutlinesW) (void);

// GDI OVERLOADS

GLMETHOD(BOOL,_wglSwapBuffers) (HDC dc);
GLMETHOD(int,_wglChoosePixelFormat) (HDC dc, const PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(int,_wglDescribePixelFormat) (HDC dc, int iPixelFormat, unsigned int nBytes, PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(BOOL,_wglSetPixelFormat) (HDC dc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *pf);
GLMETHOD(int,_wglGetPixelFormat) (HDC dc);

// EXTENSIONS

// for convenience it is easier to get common extensions inside
// the App can still call wglGetProcAddress on its own...
// FUTURE: expose pointers and recompile libraries?

#if EMULATE_EXT
void __stdcall _glColorTableEXT (GLenum target, GLenum ifmt, GLsizei count, GLenum fmt, GLenum type, const GLvoid *data);
#endif

void (*SetFullscreen)(bool) = 0;
void (*FreeContextSurfaces)(void) = 0;
void (*AllocContextSurfaces)(void) = 0;

//----------------------------------------------------------------------------
// Function Pointer Initialization
//----------------------------------------------------------------------------

struct LINKER
{
	int support;

	void **ptr;
	const char *name;
	void *value;

	void reset (void)
	{
		*ptr = value;
	}
};

#define LINK2(p,func,value)			{ p,(void**)&func, #func, value }
#define LINK3(p,func,string,value)	{ p,(void**)&func, string, value }
#define LINKw(p,func,value)			{ p,(void**)&_##func, #func, value }

LINKER FuncList[] =
{
// MINIMAL SUPPORT

	LINKw(1,wglChoosePixelFormat,i2),
	LINKw(3,wglCopyContext,i3),
	LINKw(1,wglCreateContext,i1),
	LINKw(3,wglCreateLayerContext,i2),
	LINKw(1,wglDeleteContext,i1),
	LINKw(3,wglDescribeLayerPlane,i5),
	LINKw(1,wglDescribePixelFormat,i4),
	LINKw(3,wglGetCurrentContext,i0),
	LINKw(3,wglGetCurrentDC,i0),
	//		LINKw(3,wglGetDefaultProcAddress,),
	LINKw(3,wglGetLayerPaletteEntries,i5),
	LINKw(1,wglGetPixelFormat,i1),
	LINKw(1,wglGetProcAddress,i1),
	LINKw(1,wglMakeCurrent,i2),
	LINKw(3,wglRealizeLayerPalette,i3),
	LINKw(3,wglSetLayerPaletteEntries,i5),
	LINKw(1,wglSetPixelFormat,i3),
	LINKw(3,wglShareLists,i2),
	LINKw(1,wglSwapBuffers,v0),
	LINKw(3,wglSwapLayerBuffers,i2),
	LINKw(3,wglUseFontBitmapsA,i4),
	LINKw(3,wglUseFontBitmapsW,i4),
	LINKw(3,wglUseFontOutlinesA,i8),
	LINKw(3,wglUseFontOutlinesW,i8),

	LINK2(3,glAccum,v2),
	LINK2(2,glAlphaFunc,v2),
	LINK2(2,glAreTexturesResident,i3),
	LINK2(2,glArrayElement,v1),
	LINK2(1,glBegin,v1),
	LINK2(1,glBindTexture,v2),
	LINK2(2,glBitmap,v7),
	LINK2(2,glBlendFunc,v2),
	LINK2(1,glCallList,v1),
	LINK2(1,glCallLists,v3),
	LINK2(1,glClear,v1),
	LINK2(3,glClearAccum,v4),
	LINK2(1,glClearColor,v4),
	LINK2(2,glClearDepth,v2),
	LINK2(3,glClearIndex,v1),
	LINK2(3,glClearStencil,v1),
	LINK2(3,glClipPlane,v3),
	LINK2(3,glColor3b,v3),
	LINK2(3,glColor3bv,v1),
	LINK2(3,glColor3d,v6),
	LINK2(3,glColor3dv,v1),
	LINK2(1,glColor3f,v3),
	LINK2(1,glColor3fv,v1),
	LINK2(3,glColor3i,v3),
	LINK2(3,glColor3iv,v1),
	LINK2(3,glColor3s,v3),
	LINK2(3,glColor3sv,v1),
	LINK2(1,glColor3ub,v3),
	LINK2(1,glColor3ubv,v1),
	LINK2(3,glColor3ui,v3),
	LINK2(3,glColor3uiv,v1),
	LINK2(3,glColor3us,v3),
	LINK2(3,glColor3usv,v1),
	LINK2(3,glColor4b,v4),
	LINK2(3,glColor4bv,v1),
	LINK2(3,glColor4d,v8),
	LINK2(3,glColor4dv,v1),
	LINK2(1,glColor4f,v4),
	LINK2(1,glColor4fv,v1),
	LINK2(3,glColor4i,v4),
	LINK2(3,glColor4iv,v4),
	LINK2(3,glColor4s,v4),
	LINK2(3,glColor4sv,v4),
	LINK2(1,glColor4ub,v4),
	LINK2(1,glColor4ubv,v1),
	LINK2(3,glColor4ui,v4),
	LINK2(3,glColor4uiv,v4),
	LINK2(3,glColor4us,v4),
	LINK2(3,glColor4usv,v1),
	LINK2(3,glColorMask,v4),
	LINK2(2,glColorMaterial,v2),
	LINK2(2,glColorPointer,v4),
	LINK2(3,glCopyPixels,v5),
	LINK2(3,glCopyTexImage1D,v7),
	LINK2(3,glCopyTexImage2D,v8),
	LINK2(3,glCopyTexSubImage1D,v6),
	LINK2(3,glCopyTexSubImage2D,v8),
	LINK2(1,glCullFace,v1),
	LINK2(1,glDeleteLists,v2),
	LINK2(1,glDeleteTextures,i2),
	LINK2(2,glDepthFunc,v1),
	LINK2(2,glDepthMask,v1),
	LINK2(2,glDepthRange,v1),
	LINK2(1,glDisable,v1),
	LINK2(2,glDisableClientState,v1),
	LINK2(2,glDrawArrays,v3),
	LINK2(3,glDrawBuffer,v1),
	LINK2(2,glDrawElements,v4),
	LINK2(2,glDrawPixels,v5),
	LINK2(3,glEdgeFlag,v1),
	LINK2(3,glEdgeFlagPointer,v2),
	LINK2(3,glEdgeFlagv,v1),
	LINK2(1,glEnable,v1),
	LINK2(2,glEnableClientState,v1),
	LINK2(1,glEnd,v0),
	LINK2(1,glEndList,v0),
	LINK2(3,glEvalCoord1d,v2),
	LINK2(3,glEvalCoord1dv,v1),
	LINK2(3,glEvalCoord1f,v1),
	LINK2(3,glEvalCoord1fv,v1),
	LINK2(3,glEvalCoord2d,v4),
	LINK2(3,glEvalCoord2dv,v1),
	LINK2(3,glEvalCoord2f,v2),
	LINK2(3,glEvalCoord2fv,v1),
	LINK2(3,glEvalMesh1,v3),
	LINK2(3,glEvalMesh2,v5),
	LINK2(3,glEvalPoint1,v1),
	LINK2(3,glEvalPoint2,v2),
	LINK2(3,glFeedbackBuffer,v3),
	LINK2(1,glFinish,v0),
	LINK2(1,glFlush,v0),
	LINK2(3,glFogf,v2),
	LINK2(3,glFogfv,v2),
	LINK2(3,glFogi,v2),
	LINK2(3,glFogiv,v2),
	LINK2(1,glFrontFace,v1),
	LINK2(1,glFrustum,v6d),
	LINK2(1,glGenLists,i3),
	LINK2(1,glGenTextures,v2),
	LINK2(2,glGetBooleanv,v2),
	LINK2(3,glGetClipPlane,v2),
	LINK2(3,glGetDoublev,v2),
	LINK2(1,glGetError,i0),
	LINK2(2,glGetFloatv,v2),
	LINK2(2,glGetIntegerv,v2),
	LINK2(3,glGetLightfv,v3),
	LINK2(3,glGetLightiv,v3),
	LINK2(3,glGetMapdv,v3),
	LINK2(3,glGetMapfv,v3),
	LINK2(3,glGetMapiv,v3),
	LINK2(3,glGetMaterialfv,v3),
	LINK2(3,glGetMaterialiv,v3),
	LINK2(3,glGetPixelMapfv,v2),
	LINK2(3,glGetPixelMapuiv,v2),
	LINK2(3,glGetPixelMapusv,v2),
	LINK2(2,glGetPointerv,v2),
	LINK2(3,glGetPolygonStipple,v1),
	LINK2(1,glGetString,i1),
	LINK2(3,glGetTexEnvfv,v3),
	LINK2(3,glGetTexEnviv,v3),
	LINK2(3,glGetTexGendv,v3),
	LINK2(3,glGetTexGenfv,v3),
	LINK2(3,glGetTexGeniv,v3),
	LINK2(3,glGetTexImage,v5),
	LINK2(3,glGetTexLevelParameterfv,v4),
	LINK2(2,glGetTexLevelParameteriv,v4),
	LINK2(3,glGetTexParameterfv,v3),
	LINK2(2,glGetTexParameteriv,v3),
	LINK2(1,glHint,v2),
	LINK2(3,glIndexMask,v1),
	LINK2(3,glIndexPointer,v3),
	LINK2(3,glIndexd,v2),
	LINK2(3,glIndexdv,v1),
	LINK2(3,glIndexf,v1),
	LINK2(3,glIndexfv,v1),
	LINK2(2,glIndexi,v1),
	LINK2(3,glIndexiv,v1),
	LINK2(3,glIndexs,v1),
	LINK2(3,glIndexsv,v1),
	LINK2(3,glIndexub,v1),
	LINK2(3,glIndexubv,v1),
	LINK2(3,glInitNames,v0),
	LINK2(2,glInterleavedArrays,v3),
	LINK2(1,glIsEnabled,i1),
	LINK2(1,glIsList,i1),
	LINK2(1,glIsTexture,i1),
	LINK2(3,glLightModelf,v2),
	LINK2(2,glLightModelfv,v2),
	LINK2(2,glLightModeli,v2),
	LINK2(3,glLightModeliv,v2),
	LINK2(2,glLightf,v3),
	LINK2(2,glLightfv,v3),
	LINK2(3,glLighti,v3),
	LINK2(3,glLightiv,v3),
	LINK2(3,glLineStipple,v2),
	LINK2(3,glLineWidth,v1),
	LINK2(1,glListBase,v1),
	LINK2(1,glLoadIdentity,v0),
	LINK2(3,glLoadMatrixd,v1),
	LINK2(1,glLoadMatrixf,v1),
	LINK2(3,glLoadName,v1),
	LINK2(3,glLogicOp,v1),
	LINK2(3,glMap1d,v8),
	LINK2(3,glMap1f,v6),
	LINK2(3,glMap2d,v14),
	LINK2(3,glMap2f,v10),
	LINK2(3,glMapGrid1d,v5),
	LINK2(3,glMapGrid1f,v5),
	LINK2(3,glMapGrid2d,v10),
	LINK2(3,glMapGrid2f,v6),
	LINK2(2,glMaterialf,v3),
	LINK2(2,glMaterialfv,v3),
	LINK2(2,glMateriali,v3),
	LINK2(3,glMaterialiv,v3),
	LINK2(1,glMatrixMode,v1),
	LINK2(3,glMultMatrixd,v1),
	LINK2(1,glMultMatrixf,v1),
	LINK2(1,glNewList,v2),
	LINK2(3,glNormal3b,v3),
	LINK2(3,glNormal3bv,v1),
	LINK2(3,glNormal3d,v6),
	LINK2(3,glNormal3dv,v1),
	LINK2(1,glNormal3f,v3),
	LINK2(1,glNormal3fv,v1),
	LINK2(3,glNormal3i,v3),
	LINK2(3,glNormal3iv,v1),
	LINK2(3,glNormal3s,v3),
	LINK2(3,glNormal3sv,v1),
	LINK2(2,glNormalPointer,v4),
	LINK2(1,glOrtho,v6d),
	LINK2(3,glPassThrough,v1),
	LINK2(1,glPixelMapfv,v3),
	LINK2(3,glPixelMapuiv,v3),
	LINK2(3,glPixelMapusv,v3),
	LINK2(3,glPixelStoref,v2),
	LINK2(3,glPixelStorei,v2),
	LINK2(3,glPixelTransferf,v2),
	LINK2(3,glPixelTransferi,v2),
	LINK2(3,glPixelZoom,v2),
	LINK2(3,glPointSize,v1),
	LINK2(1,glPolygonMode,v2),
	LINK2(3,glPolygonOffset,v2),
	LINK2(3,glPolygonStipple,v1),
	LINK2(2,glPopAttrib,v0),
	LINK2(3,glPopClientAttrib,v0),
	LINK2(2,glPopMatrix,v0),
	LINK2(3,glPopName,v0),
	LINK2(2,glPrioritizeTextures,v3),
	LINK2(2,glPushAttrib,v1),
	LINK2(3,glPushClientAttrib,v1),
	LINK2(2,glPushMatrix,v1),
	LINK2(3,glPushName,v1),
	LINK2(3,glRasterPos2d,v4),
	LINK2(3,glRasterPos2dv,v1),
	LINK2(2,glRasterPos2f,v2),
	LINK2(2,glRasterPos2fv,v1),
	LINK2(3,glRasterPos2i,v2),
	LINK2(3,glRasterPos2iv,v1),
	LINK2(3,glRasterPos2s,v2),
	LINK2(3,glRasterPos2sv,v1),
	LINK2(3,glRasterPos3d,v3),
	LINK2(3,glRasterPos3dv,v1),
	LINK2(2,glRasterPos3f,v3),
	LINK2(2,glRasterPos3fv,v1),
	LINK2(3,glRasterPos3i,v3),
	LINK2(3,glRasterPos3iv,v1),
	LINK2(3,glRasterPos3s,v3),
	LINK2(3,glRasterPos3sv,v1),
	LINK2(3,glRasterPos4d,v8),
	LINK2(3,glRasterPos4dv,v1),
	LINK2(3,glRasterPos4f,v4),
	LINK2(3,glRasterPos4fv,v1),
	LINK2(3,glRasterPos4i,v4),
	LINK2(3,glRasterPos4iv,v1),
	LINK2(3,glRasterPos4s,v4),
	LINK2(3,glRasterPos4sv,v1),
	LINK2(3,glReadBuffer,v1),
	LINK2(3,glReadPixels,v7),
	LINK2(3,glRectd,v4d),
	LINK2(3,glRectdv,v2),
	LINK2(1,glRectf,v4),
	LINK2(3,glRectfv,v2),
	LINK2(3,glRecti,v4),
	LINK2(3,glRectiv,v2),
	LINK2(3,glRects,v4),
	LINK2(3,glRectsv,v2),
	LINK2(3,glRenderMode,i1),
	LINK2(3,glRotated,v4d),
	LINK2(1,glRotatef,v4),
	LINK2(3,glScaled,v3d),
	LINK2(1,glScalef,v3),
	LINK2(3,glScissor,v4),
	LINK2(3,glSelectBuffer,v2),
	LINK2(1,glShadeModel,v1),
	LINK2(3,glStencilFunc,v3),
	LINK2(3,glStencilMask,v1),
	LINK2(3,glStencilOp,v3),
	LINK2(3,glTexCoord1d,v1d),
	LINK2(3,glTexCoord1dv,v1),
	LINK2(3,glTexCoord1f,v1),
	LINK2(3,glTexCoord1fv,v1),
	LINK2(3,glTexCoord1i,v1),
	LINK2(3,glTexCoord1iv,v1),
	LINK2(3,glTexCoord1s,v1),
	LINK2(3,glTexCoord1sv,v1),
	LINK2(3,glTexCoord2d,v2d),
	LINK2(3,glTexCoord2dv,v1),
	LINK2(1,glTexCoord2f,v2),
	LINK2(1,glTexCoord2fv,v1),
	LINK2(3,glTexCoord2i,v2),
	LINK2(3,glTexCoord2iv,v1),
	LINK2(3,glTexCoord2s,v2),
	LINK2(3,glTexCoord2sv,v1),
	LINK2(3,glTexCoord3d,v3d),
	LINK2(3,glTexCoord3dv,v1),
	LINK2(3,glTexCoord3f,v3),
	LINK2(3,glTexCoord3fv,v1),
	LINK2(3,glTexCoord3i,v3),
	LINK2(3,glTexCoord3iv,v1),
	LINK2(3,glTexCoord3s,v3),
	LINK2(3,glTexCoord3sv,v1),
	LINK2(3,glTexCoord4d,v4d),
	LINK2(3,glTexCoord4dv,v1),
	LINK2(3,glTexCoord4f,v4),
	LINK2(3,glTexCoord4fv,v1),
	LINK2(3,glTexCoord4i,v4),
	LINK2(3,glTexCoord4iv,v1),
	LINK2(3,glTexCoord4s,v4),
	LINK2(3,glTexCoord4sv,v1),
	LINK2(2,glTexCoordPointer,v4),
	LINK2(2,glTexEnvf,v3),
	LINK2(2,glTexEnvfv,v3),
	LINK2(2,glTexEnvi,v3),
	LINK2(3,glTexEnviv,v3),
	LINK2(3,glTexGend,v4),
	LINK2(3,glTexGendv,v3),
	LINK2(3,glTexGenf,v3),
	LINK2(3,glTexGenfv,v3),
	LINK2(3,glTexGeni,v3),
	LINK2(3,glTexGeniv,v3),
	LINK2(3,glTexImage1D,v8),
	LINK2(1,glTexImage2D,v9),
	LINK2(3,glTexParameterf,v3),
	LINK2(3,glTexParameterfv,v3),
	LINK2(2,glTexParameteri,v3),
	LINK2(3,glTexParameteriv,v3),
	LINK2(3,glTexSubImage1D,v7),
	LINK2(3,glTexSubImage2D,v9),
	LINK2(3,glTranslated,v3d),
	LINK2(1,glTranslatef,v3),
	LINK2(3,glVertex2d,v2d),
	LINK2(3,glVertex2dv,v1),
	LINK2(2,glVertex2f,v2),
	LINK2(2,glVertex2fv,v1),
	LINK2(3,glVertex2i,v2),
	LINK2(3,glVertex2iv,v1),
	LINK2(3,glVertex2s,v2),
	LINK2(3,glVertex2sv,v1),
	LINK2(3,glVertex3d,v3d),
	LINK2(3,glVertex3dv,v1),
	LINK2(1,glVertex3f,v3),
	LINK2(2,glVertex3fv,v1),
	LINK2(3,glVertex3i,v3),
	LINK2(3,glVertex3iv,v1),
	LINK2(3,glVertex3s,v3),
	LINK2(3,glVertex3sv,v1),
	LINK2(3,glVertex4d,v4d),
	LINK2(3,glVertex4dv,v1),
	LINK2(2,glVertex4f,v4),
	LINK2(2,glVertex4fv,v1),
	LINK2(3,glVertex4i,v4),
	LINK2(3,glVertex4iv,v1),
	LINK2(3,glVertex4s,v4),
	LINK2(3,glVertex4sv,v1),
	LINK2(2,glVertexPointer,v4),
	LINK2(1,glViewport,v4),
};

LINKER ExtensionList[] =
{
#if EMULATE_EXT
	LINK2(9,glColorTableEXT,_glColorTableEXT),
#else
	LINK2(9,glColorTableEXT,0),
#endif

//	LINK2(2,gluOrtho2D,v4d),
//	LINK2(2,gluPerspective,v4d),
};

void ResetFunctions (void)
{
	int i;

	for (i=0; i<countof(FuncList); i++)
	{
		FuncList[i].reset();
	}

	for (i=0; i<countof(ExtensionList); i++)
	{
		ExtensionList[i].reset();
	}
}

//----------------------------------------------------------------------------
// DISPLAY
//----------------------------------------------------------------------------

struct DISPLAY : IDisplay, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(DISPLAY)
	DACOM_INTERFACE_ENTRY(IDisplay)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	END_DACOM_MAP()

	HINSTANCE library;

	DrawMgr DRAW;

	DISPLAY (void)
	{
		library = 0;
	}

	~DISPLAY (void)
	{
		DUMP = 0;
		if (library)
		{
			//glMakeCurrent(0,0);
			free_library();
		}

		DRAW.shutdown();
	}

// IDAComponent methods

	GENRESULT init (AGGDESC *desc)
	{
		if (!DRAW.startup())
			return GR_GENERIC;

		return GR_OK;
	}

// IAggregateComponent methods

	IDAComponent *getBase (void)
	{
		return (IDAComponent *)((IDisplay *)this);
	}

    virtual GENRESULT COMAPI Initialize (void)
	{
		IDAComponent *system = getBase();

		if (system->QueryInterface("IDumpText", (void **)&DUMP) == GR_OK)
			system->Release();		// release the extra reference

		return GR_OK;
	}

//----------------------------------------------------------------------------

// IDisplay methods

	VMETHOD(bool) set_display_mode (HDC hdc, int x, int y, int bpp)
	{
		HWND wnd = WindowFromDC(hdc);

DebugPrint("display: HWND = %X, DC = %X\n", wnd, hdc);
		if (FreeContextSurfaces)
		{
			FreeContextSurfaces();
		}

		bool result = DRAW.set_display_mode(wnd,x,y,bpp);

		if (SetFullscreen)
		{
			SetFullscreen(true);
		}
		if (AllocContextSurfaces)
		{
			AllocContextSurfaces();
		}

		return result;
	}

	VMETHOD(void) restore_display_mode (HDC hdc)
	{
		HWND wnd = WindowFromDC(hdc);

		if (FreeContextSurfaces)
		{
			FreeContextSurfaces();
		}
		DRAW.restore_display_mode(wnd);
		if (SetFullscreen)
		{
			SetFullscreen(false);
		}
		if (AllocContextSurfaces)
		{
			AllocContextSurfaces();
		}
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	VMETHOD(int) load_library (char *name, char *type)
	{
		int min = 0;

		if (library)
		{
			free_library();
		}

		library = ::LoadLibrary(name);
		if (library)
		{
			SetFullscreen = (void (__cdecl *)(bool)) ::GetProcAddress(library, "SetFullscreen");
			FreeContextSurfaces = (void (__cdecl *)(void)) ::GetProcAddress(library, "FreeContextSurfaces");
			AllocContextSurfaces = (void (__cdecl *)(void)) ::GetProcAddress(library, "AllocContextSurfaces");

			min = 255;
			int max = 0;

			for (int i=0; i<countof(FuncList); i++)
			{
				LINKER *p = FuncList+i;
				void *ptr = ::GetProcAddress(library,p->name);
				if (ptr)
				{
					if (p->support > max)
						max = p->support;

					*p->ptr = ptr;
				}
				else
				{
					if (p->support <= min)
						min = p->support-1;

					if (p->support == 1)
					{
						DebugPrint("Display: function '%s' not found\n",p->name);
					}
				}
			}

			if (max < min)
				min = max;

			if (min < 1) // require Minimal support?
			{
				free_library();
				min = 0;
			}
/*
			for (int x=0; x<countof(ExtensionList); x++)
			{
				LINKER *p = ExtensionList+x;
				void *ptr = wglGetProcAddress(p->name);
				*p->ptr = ptr;

				if (ptr)
				{
					DebugPrint("Display: found extension '%s'\n",p->name);
				}
			}
*/
		}

		return min;
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	VMETHOD(void) free_library (void)
	{
		if (library)
		{
			::FreeLibrary(library);
			library = 0;
		}

		ResetFunctions();
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	VMETHOD(void *) get_proc_address (char *func_name)
	{
		return 0;
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	VMETHOD(int) get_driver_string (int index, char *dst, int max)
	{
		IProfileParser *parser;

		int size = 0;

		if (DACOM->QueryInterface("IProfileParser", (void **)&parser) == GR_OK)
		{
			HANDLE hSection = parser->CreateSection("OpenGL_Drivers");
			if (hSection)
			{
				char buffer[256];
				int line=0;
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					if (index == line++)
					{
						size = strlen(buffer);
						if (size > max) size = max;
						strncpy(dst,buffer,size);
						break;
					}
				}
			}
			parser->Release();
		}
		dst[size] = 0;

		return size;
	}

	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

};

//----------------------------------------------------------------------------
// DLL Initialize
//----------------------------------------------------------------------------
#if USE_HEAP
void SetDllHeapMsg (HINSTANCE hInstance)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
	
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}
	
	SetDefaultHeapMsg(buffer+dwLen);
}

void main (void)
{
}
#endif

BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			#if USE_HEAP
			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);
			#endif
			CoInitialize(0);	// start up MSCOM system

			ResetFunctions();

			IComponentFactory *server;
			server = new DAComponentFactory2<DAComponentAggregate<DISPLAY>, AGGDESC> (InterfaceName);

			if (server != NULL)
			{
				DACOM = DACOM_Acquire();

				if (DACOM != NULL)
				{
					DACOM->RegisterComponent(server, InterfaceName, DACOM_LOW_PRIORITY);
				}
				server->Release();
			}
			break;
		}

		case DLL_PROCESS_DETACH:
			CoUninitialize();	// undo MSCOM init
			break;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// GDI override functions
//----------------------------------------------------------------------------

#define GDIMETHOD(type) __declspec(dllexport) type __stdcall

#pragma warning(disable:4273)		// allow override of GDI definitions

GDIMETHOD(BOOL) SwapBuffers (HDC dc)
{
	return _wglSwapBuffers(dc);
}

GDIMETHOD(int) ChoosePixelFormat (HDC dc, const PIXELFORMATDESCRIPTOR *pf)
{
	return _wglChoosePixelFormat(dc,pf);
}

GDIMETHOD(int) DescribePixelFormat (HDC dc, int iPixelFormat, unsigned int nBytes, PIXELFORMATDESCRIPTOR *pf)
{
	return _wglDescribePixelFormat(dc,iPixelFormat,nBytes,pf);
}

GDIMETHOD(BOOL) SetPixelFormat (HDC dc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *pf)
{
	return _wglSetPixelFormat(dc,iPixelFormat,pf);
}

GDIMETHOD(int) GetPixelFormat (HDC dc)
{
	return _wglGetPixelFormat(dc);
}

GDIMETHOD(BOOL) wglCopyContext (HGLRC c1, HGLRC c2, UINT l)				{ return _wglCopyContext(c1,c2,l); }
GDIMETHOD(HGLRC) wglCreateContext (HDC dc)								
{ 
	HGLRC result = _wglCreateContext(dc); 

	return result;
}

GDIMETHOD(HGLRC) wglCreateLayerContext (HDC dc, int l)					{ return _wglCreateLayerContext(dc,l); }
GDIMETHOD(BOOL) wglDeleteContext (HGLRC rc)								{ return _wglDeleteContext(rc); }
GDIMETHOD(HGLRC) wglGetCurrentContext (VOID)							{ return _wglGetCurrentContext(); }
GDIMETHOD(HDC) wglGetCurrentDC (VOID)									{ return _wglGetCurrentDC(); }
GDIMETHOD(PROC) wglGetProcAddress (LPCSTR n)							{ return _wglGetProcAddress(n); }
GDIMETHOD(BOOL) wglMakeCurrent (HDC dc, HGLRC rc)						
{ 
	BOOL result = _wglMakeCurrent(dc,rc); 
	for (int x=0; x<countof(ExtensionList); x++)
	{
		LINKER *p = ExtensionList+x;
		void *ptr = wglGetProcAddress(p->name);
		*p->ptr = ptr;

		if (ptr)
		{
			DebugPrint("Display: found extension '%s'\n",p->name);
		}
	}
	return result;
}
GDIMETHOD(BOOL) wglShareLists (HGLRC c1, HGLRC c2)						{ return _wglShareLists(c1,c2); }
GDIMETHOD(BOOL) wglUseFontBitmapsA (HDC dc, DWORD f, DWORD c, DWORD l)	{ return _wglUseFontBitmapsA(dc,f,c,l); }
GDIMETHOD(BOOL) wglUseFontBitmapsW (HDC dc, DWORD f, DWORD c, DWORD l)	{ return _wglUseFontBitmapsW(dc,f,c,l); }

#pragma warning(default:4273)

//---------------------------------------------------------------------------
// EXTENSIONS
//---------------------------------------------------------------------------

#pragma warning(disable:4244)	// conversion double to float
#pragma warning(disable:4305)	// truncation double to float

#include <math.h>
#define PI			3.141592654
#define DEG2RAD		(PI/180.0)


// this is NOT exactly GL compatible
// because the fovy we use here is really only 1/2 of the actual fovy
void __stdcall gluPerspective (GLdouble fovy, GLdouble aspect, GLdouble z0, GLdouble z1)
{
	float fy = z0 * tan(fovy * PI/180);
	float fx = fy * aspect;

	glFrustum(-fx,fx,-fy,fy, z0,z1);
}

//---------------------------------------------------------------------------

#if EMULATE_EXT
	struct RGB
	{
		GLubyte r,g,b;
	};

void __stdcall _glColorTableEXT (GLenum target, GLenum ifmt, GLsizei count, GLenum fmt, GLenum type, const GLvoid *data)
{
// DEFAULT HANDLER = convert to standard commands

	if (target != GL_TEXTURE_2D ||
		ifmt != GL_RGB8 ||
		count > 256 ||
		fmt != GL_RGB ||
		type != GL_UNSIGNED_BYTE)
	{
		DebugPrint("Display: glColorTableEXT\n   parameters not supported?\n");
		return;
	}

	const RGB *palette = (const RGB *)data;

	float table[256];
	double i2f = 1.0/255;
	int i;
	for (i=0; i<256; i++) { table[i] = palette[i].r * i2f; }
	glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,table);
	for (i=0; i<256; i++) { table[i] = palette[i].g * i2f; }
	glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,table);
	for (i=0; i<256; i++) { table[i] = palette[i].b * i2f; }
	glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,table);
}
#endif

struct tagRGB
{
	GLubyte r,g,b;
};
struct tagRGBA 
{
	GLubyte r,g,b,a;
};
//---------------------------------------------------------------------------
//
void __stdcall dgluSetColorTables (GLsizei count, GLenum fmt, const GLvoid *data)
{
	if (count > 256)
		count = 256;

	if (fmt == GL_RGB)
	{
		const tagRGB *palette = (const tagRGB *)data;

		float table[256];
		double i2f = 1.0/255;
		int i;
		for (i=0; i<count; i++) { table[i] = palette[i].r * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].g * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].b * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B,count,table);
	}
	else
	if (fmt == GL_RGBA)
	{
		const tagRGBA *palette = (const tagRGBA *)data;

		float table[256];
		double i2f = 1.0/255;
		int i;
		for (i=0; i<count; i++) { table[i] = palette[i].r * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].g * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].b * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B,count,table);
		for (i=0; i<count; i++) { table[i] = palette[i].a * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_A,count,table);
	}
	else
	{
		DebugPrint("Display: dgluSetColorTables\n   parameters not supported?\n");
	}
}
//---------------------------------------------------------------------------
//------------------------------End Display.cpp------------------------------
//---------------------------------------------------------------------------
