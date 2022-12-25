/* $Id: glu.h,v 1.9 1998/01/16 02:29:26 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: /Tools/ROS System/glu.h $
 * 
 * 1     5/01/98 11:36a Svarma
 * Revision 1.9  1998/01/16 02:29:26  brianp
 * minor changes for Windows compilation (Theodore Jump)
 *
 * Revision 1.8  1997/10/29 02:03:20  brianp
 * added WINGDIAPI, APIENTRY stuff (David Bucciarelli, v20 3dfx driver)
 *
 * Revision 1.7  1997/08/19 02:35:07  brianp
 * added some Macintosh-only pragmas (Miklos Fazekas)
 *
 * Revision 1.6  1997/07/13 22:59:34  brianp
 * added const to viewport parameter of gluPickMatrix()
 *
 * Revision 1.5  1997/05/28 02:31:01  brianp
 * added a comment about typedefs
 *
 * Revision 1.4  1997/02/19 10:13:54  brianp
 * now test for __QUICKDRAW__ like for __BEOS__ (Randy Frank)
 *
 * Revision 1.3  1997/02/03 20:05:33  brianp
 * patches for BeOS
 *
 * Revision 1.2  1997/02/03 19:15:15  brianp
 * conditionally include glu_mangle.h
 *
 * Revision 1.1  1996/09/13 01:26:41  brianp
 * Initial revision
 *
 */


#ifndef GLU_H
#define GLU_H


#if defined(USE_MGL_NAMESPACE)
#include "glu_mangle.h"
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


//#include "Display.h"
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

// Boolean

#define GL_TRUE							1
#define GL_FALSE						0

#ifdef macintosh
	#pragma enumsalwaysint on
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import on
	#endif
#endif


#define GLU_VERSION_1_1		1


#define GLU_TRUE   GL_TRUE
#define GLU_FALSE  GL_FALSE



/*
 * These are the GLU 1.1 typedefs.  GLU 1.2 has different ones!
 */
typedef struct GLUquadricObj GLUquadricObj;

typedef struct GLUtriangulatorObj GLUtriangulatorObj;

typedef struct GLUnurbsObj GLUnurbsObj;



#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export on
#endif


/*
 *
 * Miscellaneous functions
 *
 */


extern void APIENTRY gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
                                GLdouble centerx, GLdouble centery,
                                GLdouble centerz,
                                GLdouble upx, GLdouble upy, GLdouble upz );


extern void APIENTRY gluOrtho2D( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top );


extern void APIENTRY gluPerspective( GLdouble fovy, GLdouble aspect,
                                     GLdouble zNear, GLdouble zFar );


extern void APIENTRY gluPickMatrix( GLdouble x, GLdouble y,
                                    GLdouble width, GLdouble height,
                                    const GLint viewport[4] );

extern GLint APIENTRY gluProject( GLdouble objx, GLdouble objy, GLdouble objz,
                                  const GLdouble modelMatrix[16],
                                  const GLdouble projMatrix[16],
                                  const GLint viewport[4],
                                  GLdouble *winx, GLdouble *winy,
                                  GLdouble *winz );

extern GLint APIENTRY gluUnProject( GLdouble winx, GLdouble winy,
                                    GLdouble winz,
                                    const GLdouble modelMatrix[16],
                                    const GLdouble projMatrix[16],
                                    const GLint viewport[4],
                                    GLdouble *objx, GLdouble *objy,
                                    GLdouble *objz );

#if 0
extern const GLubyte* APIENTRY gluErrorString( GLenum errorCode );



/*
 *
 * Mipmapping and image scaling
 *
 */

extern GLint APIENTRY gluScaleImage( GLenum format,
                                     GLint widthin, GLint heightin,
                                     GLenum typein, const void *datain,
                                     GLint widthout, GLint heightout,
                                     GLenum typeout, void *dataout );

extern GLint APIENTRY gluBuild1DMipmaps( GLenum target, GLint components,
                                         GLint width, GLenum format,
                                         GLenum type, const void *data );

extern GLint APIENTRY gluBuild2DMipmaps( GLenum target, GLint components,
                                         GLint width, GLint height,
                                         GLenum format,
                                         GLenum type, const void *data );



/*
 *
 * Quadrics
 *
 */

extern GLUquadricObj* APIENTRY gluNewQuadric( void );

extern void APIENTRY gluDeleteQuadric( GLUquadricObj *state );

extern void APIENTRY gluQuadricDrawStyle( GLUquadricObj *quadObject,
                                          GLenum drawStyle );

extern void APIENTRY gluQuadricOrientation( GLUquadricObj *quadObject,
                                            GLenum orientation );

extern void APIENTRY gluQuadricNormals( GLUquadricObj *quadObject,
                                        GLenum normals );

extern void APIENTRY gluQuadricTexture( GLUquadricObj *quadObject,
                                        GLboolean textureCoords );

extern void APIENTRY gluQuadricCallback( GLUquadricObj *qobj,
                                         GLenum which, void (CALLBACK *fn)() );

extern void APIENTRY gluCylinder( GLUquadricObj *qobj,
                                  GLdouble baseRadius,
                                  GLdouble topRadius,
                                  GLdouble height,
                                  GLint slices, GLint stacks );

extern void APIENTRY gluSphere( GLUquadricObj *qobj,
                                GLdouble radius, GLint slices, GLint stacks );

extern void APIENTRY gluDisk( GLUquadricObj *qobj,
                              GLdouble innerRadius, GLdouble outerRadius,
                              GLint slices, GLint loops );

extern void APIENTRY gluPartialDisk( GLUquadricObj *qobj, GLdouble innerRadius,
                                     GLdouble outerRadius, GLint slices,
                                     GLint loops, GLdouble startAngle,
                                     GLdouble sweepAngle );



/*
 *
 * Nurbs
 *
 */

extern GLUnurbsObj* APIENTRY gluNewNurbsRenderer( void );

extern void APIENTRY gluDeleteNurbsRenderer( GLUnurbsObj *nobj );

extern void APIENTRY gluLoadSamplingMatrices( GLUnurbsObj *nobj,
                                              const GLfloat modelMatrix[16],
                                              const GLfloat projMatrix[16],
                                              const GLint viewport[4] );

extern void APIENTRY gluNurbsProperty( GLUnurbsObj *nobj, GLenum property,
                                       GLfloat value );

extern void APIENTRY gluGetNurbsProperty( GLUnurbsObj *nobj, GLenum property,
                                          GLfloat *value );

extern void APIENTRY gluBeginCurve( GLUnurbsObj *nobj );

extern void APIENTRY gluEndCurve( GLUnurbsObj * nobj );

extern void APIENTRY gluNurbsCurve( GLUnurbsObj *nobj, GLint nknots,
                                    GLfloat *knot, GLint stride,
                                    GLfloat *ctlarray, GLint order,
                                    GLenum type );

extern void APIENTRY gluBeginSurface( GLUnurbsObj *nobj );

extern void APIENTRY gluEndSurface( GLUnurbsObj * nobj );

extern void APIENTRY gluNurbsSurface( GLUnurbsObj *nobj,
                                      GLint sknot_count, GLfloat *sknot,
                                      GLint tknot_count, GLfloat *tknot,
                                      GLint s_stride, GLint t_stride,
                                      GLfloat *ctlarray,
                                      GLint sorder, GLint torder,
                                      GLenum type );

extern void APIENTRY gluBeginTrim( GLUnurbsObj *nobj );

extern void APIENTRY gluEndTrim( GLUnurbsObj *nobj );

extern void APIENTRY gluPwlCurve( GLUnurbsObj *nobj, GLint count,
                                  GLfloat *array, GLint stride, GLenum type );

extern void APIENTRY gluNurbsCallback( GLUnurbsObj *nobj, GLenum which,
                                       void (CALLBACK *fn)() );



/*
 *
 * Polygon tesselation
 *
 */

extern GLUtriangulatorObj* APIENTRY gluNewTess( void );

extern void APIENTRY gluTessCallback( GLUtriangulatorObj *tobj, GLenum which,
                                      void (CALLBACK *fn)() );

extern void APIENTRY gluDeleteTess( GLUtriangulatorObj *tobj );

extern void APIENTRY gluBeginPolygon( GLUtriangulatorObj *tobj );

extern void APIENTRY gluEndPolygon( GLUtriangulatorObj *tobj );

extern void APIENTRY gluNextContour( GLUtriangulatorObj *tobj, GLenum type );

extern void APIENTRY gluTessVertex( GLUtriangulatorObj *tobj, GLdouble v[3],
                                    void *data );



/*
 *
 * New functions in GLU 1.1
 *
 */

extern const GLubyte* APIENTRY gluGetString( GLenum name );
#endif

#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export off
#endif


#ifdef macintosh
	#pragma enumsalwaysint reset
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import off
	#endif
#endif


#ifdef __cplusplus
}
#endif


#endif
