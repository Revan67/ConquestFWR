#ifndef POLYBATCH_H
#define POLYBATCH_H

//

#include <stdlib.h>
#include "dacom.h"
#include "3dmath.h"
#include "display.h"

//
// IPolyBatch interface. Allows app to submit multiple GL blocks which are rendered
// together on end_batch() call as follows:
//
// 1. Blocks are segregated into opaque and translucent groups.
// 2. Opaque groups are sorted by texture, then rendered texture by texture in
//    order to a) minimize texture state switching, and b) optimize the number
//    of polys submitted per glBegin()/glEnd() pair.
// 3. Translucent polys are depth-sorted and rendered in back-to-front order with
//    the z-buffer set to read-only. THIS CAN RESULT IN A RIDICULOUS AMOUNT OF 
//	  TEXTURE STATE CHANGING. You can also render translucent polys without depth-
//    sorting.
//
struct IPolyBatch : public IDAComponent
{
	virtual void COMAPI begin_batch(void) = 0;

	virtual void COMAPI flush_opaque_polys(void) = 0;
	virtual void COMAPI flush_translucent_polys(BOOL32 depth_sort_translucent_polys = TRUE) = 0;

//
// End batch flushes both opaque & translucent polys. YOU MUST CALL end_batch()
// EVEN IF YOU'VE ALREADY MANUALLY flushed all the polys.
//
	virtual void COMAPI end_batch(BOOL32 depth_sort_translucent_polys = TRUE) = 0;

//
// Query whether a batch is currently in progress.
//
	virtual BOOL32 COMAPI in_batch(void) const = 0;

	virtual int COMAPI get_num_opaque_polys(void) const = 0;
	virtual int COMAPI get_num_translucent_polys(void) const = 0;

/*
	TERMINOLOGY: "Block" refers to a normal OpenGL Begin/End block. "Batch" refers
	to the IPolyBatch begin_batch/end_batch block, which may contain multiple GL blocks.

	Block submission functions are pretty much straight out of OpenGL, but 
	simplified in some cases. Usage:

	POLY->begin_batch();

		POLY->BindTexture(txm);
		POLY->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		POLY->Begin(GL_TRIANGLES);
			POLY->Color4ub(255, 255, 255, 128);
			POLY->TexCoord(0, 0);
			POLY->Vertex3f(x, y, z);
			etc. etc. etc.
		POLY->End();

	// More Begin/End blocks here...

	POLY->end_batch();


	NOTE WELL: If you call DrawElements(), the block is treated as indexed, 
	i.e. the vertices you've submitted are treated as vertex arrays. Unlike
	OpenGL, DrawElements() MUST be called between Begin()/End().

	If you call BindTexture(), texturing is enabled for the block, otherwise not.

	If you call BlendFunc(), blending is enabled for the block and it is treated
	as translucent (depth-sorted, etc.). Otherwise it's treated as opaque.

	GL_DEPTH_TEST is enabled for everything. You can change the DepthFunc to GL_ALWAYS
	if you need to disable depth testing for a given block. Or we can extend the
	interface to allow enable/disable depth testing per block.
*/

	virtual void COMAPI BindTexture(GLuint txm) = 0;
	virtual void COMAPI MultiBindTexture(GLuint txm) = 0;

	virtual void COMAPI BlendFunc(GLenum src, GLenum dst) = 0;
	virtual void COMAPI MultiBlendFunc(GLenum src, GLenum dst) = 0;

	virtual void COMAPI DepthFunc(GLenum func) = 0;

	virtual void COMAPI Begin(GLenum type) = 0;

		virtual void COMAPI Color3ub(GLubyte r, GLubyte g, GLubyte b) = 0;
		virtual void COMAPI Color3ubv(const GLfloat * vec) = 0;
		virtual void COMAPI Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) = 0;
		virtual void COMAPI Color4ubv(const GLfloat * vec) = 0;

		virtual void COMAPI TexCoord2f(float s, float t) = 0;
		virtual void COMAPI TexCoord2fv(const GLfloat * vec) = 0;

		virtual void COMAPI MultiTexCoord2f(float u, float v) = 0;
		virtual void COMAPI MultiTexCoord2fv(const GLfloat * vec) = 0;

		virtual void COMAPI Vertex3f(float x, float y, float z) = 0;
		virtual void COMAPI Vertex3fv(const GLfloat * vec) = 0;

		virtual void COMAPI DrawElements(int count, const GLuint * indices) = 0;

	virtual void COMAPI End(void) = 0;

//
// You can control the internal pool sizes for opaque & translucent vertices.
// You'll get assertions if you overrun the pools, at which point you can either
// increase the pool sizes or flush more often.
//
	virtual void			COMAPI set_opaque_pool_size(unsigned int size) = 0;
	virtual unsigned int	COMAPI get_opaque_pool_size(void) const = 0;

	virtual void			COMAPI set_translucent_pool_size(unsigned int size) = 0;
	virtual unsigned int	COMAPI get_translucent_pool_size(void) const = 0;
};

//

#endif
