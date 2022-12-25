#ifndef BIGIMAGE_H
#define BIGIMAGE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               BigImage.h                                //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/Libs/Include/bigimage.h 5     4/28/00 11:57p Rmarr $
*/
//--------------------------------------------------------------------------//

//BigImage will take images of any size and render to the screen using user specified
//rects and alpha.  It will automagically break images into textures and piece them
//together in the right spots on the screen.  The image can be rendered to a 2d rect
//on the screen or to a plane in camera space.  The destination rect for 2d rendering
//can be reversed or flipped to orient the image as desired.
//
//To use BigImage, construct a BIGIMAGEDESC using either an IImageSource, or a raw
//pointer to image bits.  Pass in either with the pixel formats, pipe, and size info
//to createinstance.
//
//Display for the 3d case requires the modelview to be set up by the app before calling
//RenderSquare().  Note the bizzare winding order on the planar verts... Not sure why
//I did it like that.
//
//Please email me with questions or to note anything missing here.  If something is
//left unexplained, I'd like to get it in here.
//
//kbaird@digitalanvil.com
 
#ifndef DACOM_H
#include <DACOM.h>
#endif

#include <iimagesource.h>
#include <rendpipeline.h>

// Describes the image to the IBigImage factory. It can be created from either an IImageSource object,
// or from a raw memory buffer.
struct BIGIMAGEDESC : DACOMDESC
{
	IRenderPipeline	*PIPE;		//pipe to use
	IImageSource	*SrcImage;	//iimagesource to chop up. Must be NULL if using a memory buffer.
	PixelFormat		*DstFmt;	//destination pixel format desired for the textures
	PixelFormat		*SrcFmt;	//source pixel format for the input bits.
	U8				*pBits;     //pointer to memory buffer. Must be NULL if using an IImageSource.
	int				Width, Height, Stride;

	//this constructor utilizes an IImageSource to get the job done
	BIGIMAGEDESC(IRenderPipeline *dPipe, IImageSource *src, PixelFormat *pf) : DACOMDESC("IBigImage")
	{
		PIPE		=dPipe;
		SrcImage	=src;
		DstFmt		=pf;
		size		=sizeof(*this);
	}

	//this constructor passes raw bits around
	BIGIMAGEDESC(IRenderPipeline *dPipe, PixelFormat *dpf, U8 *pb, PixelFormat *spf, int width, int height, int stride) : DACOMDESC("IBigImage")
	{
		PIPE		=dPipe;
		SrcImage	=NULL;
		DstFmt		=dpf;
		pBits		=pb;
		SrcFmt		=spf;
		Width		=width;
		Height		=height;
		Stride		=stride;
		size		=sizeof(*this);
	}
};

//--------------------------------------------------------------------------//
//

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IBIGIMAGE_VERSION 1
#define IID_IBigImage MAKE_IID("IBigImage",1)

struct IBigImage : IDAComponent
{
	// ====== IBigImage methods ======
	//if the source rect is null, the entire bigimage is utilized
	//pnts must have 4 valid planar vectors
	//NOTE: The vertex order is: topleft, topright, bottomleft, bottomright
	virtual void __stdcall RenderSquare(RECT *src, Vector *pnts) = 0;

	//if the source rect is null, the entire bigimage is utilized
	//source rect always behaves normalized
	//destination rect can be arbitrary for reversing or flipping
	virtual void __stdcall RenderRects(RECT *src, RECT *dst) = 0;

	//returns the dimensions of the bigimage
	virtual void __stdcall GetDimensions(U32 & width, U32 & height) const = 0;
	// Sets the alpha value and color used for each vertex when rendering.
	virtual void __stdcall SetAlpha(float a) = 0;
	virtual void __stdcall SetColor(Vector *color) = 0;
};

#endif
