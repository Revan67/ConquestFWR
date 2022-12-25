//--------------------------------------------------------------------------//
//                                                                          //
//                                VfxRead.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/VfxRead.cpp 5     3/18/00 12:31a Jasony $
*/
//------------------------------- #INCLUDES --------------------------------//

#include "pch.h"
#include <globals.h>
#include "IImageReader.h"

#include <TComponent.h>
#include <HeapObj.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE VFXREADER : IImageReader
{
	BEGIN_DACOM_MAP_INBOUND(VFXREADER)
  	DACOM_INTERFACE_ENTRY(IImageReader)
  	END_DACOM_MAP()
		
  	//------------------------------
  	// data members
  	//------------------------------
  
	COLORREF  palette[256];
	U8 * indexMap;
	U32 width, height;
	
	VFXREADER (void) { };
	
	~VFXREADER (void) { free(); }
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	void free (void);

	/* IImageReader methods */
	
	DEFMETHOD(LoadImage) (void *fileImage, U32 fileSize, U32 imageNumber);
	
	DEFMETHOD(GetImage) (U32 desiredFormat, void * buffer, const RECT * rect) const;
	
	DEFMETHOD(GetColorTable) (U32 desiredFormat, void * buffer) const;
	
	DEFMETHOD_(U32,GetWidth) (void) const;
	
	DEFMETHOD_(U32,GetHeight) (void) const;
	
	/* VFXREADER methods */
	
	void getRGBFormat (void *buffer) const;

	void getRGBAFormat (void *buffer) const;

	void getIndexFormat (U8 *buffer, const RECT * rect) const;

	void getRGBFormat (void *buffer, const RECT * rect) const;

	void getRGBAFormat (void *buffer, const RECT * rect) const;

	BOOL32 verifyPoint (S32 x, S32 y) const
	{
		if ((U32)y >= height ||	(U32)x >= width)
		{
			return 0;
		}
		return 1;
	}

	IDAComponent * GetBase (void)
	{
		return (IImageReader *) this;
	}
};    


//---------------------------------------------------------------------------
//-----------------------------VFXREADER METHODS ----------------------------
//---------------------------------------------------------------------------

void VFXREADER::free (void)
{
    ::free(indexMap);
	indexMap = 0;
}
//---------------------------------------------------------------------------
//
GENRESULT VFXREADER::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber)
{
	GENRESULT result = GR_OK;
	VFX_CRGB colors[256];
	S32 numColors, i;
	U32 _widthHeight;
	VFX_WINDOW window;
	PANE pane;
	U8 transparent=0xFF;

	free();

	if (imageNumber >= ((VFX_SHAPETABLE *)fileImage)->shape_count)
		return GR_GENERIC;

	numColors = VFX_shape_colors((VFX_SHAPETABLE *)fileImage, imageNumber, colors);
	_widthHeight = VFX_shape_bounds((VFX_SHAPETABLE *)fileImage, imageNumber);

	width = (_widthHeight >> 16)+1;
	height = (_widthHeight & 0xFFFF)+1;

	memset(palette, 0, sizeof(palette));
	for (i = 0; i < numColors; i++)
		palette[colors[i].color] = RGB(colors[i].rgb.r<<2, colors[i].rgb.g<<2, colors[i].rgb.b<<2) | 0xFF000000;

	for (i = 255; i >= 0; i--)
		if ((palette[i] & 0xFF000000) == 0)		// if index is not used
		{
			transparent = (U8) i;
			break;
		}

	::free(indexMap);
	indexMap = (U8 *) malloc(width * height);
	memset(indexMap, transparent, width * height);

	//
	// draw the shape into the buffer
	//
	window.buffer = indexMap;
	window.x_max = width - 1;
	window.y_max = height - 1;
	window.pixel_pitch = 1;
	window.bytes_per_pixel = 1;

	pane.window = &window;
	pane.x0 = pane.y0 = 0;
	pane.x1 = width - 1;
	pane.y1 = height - 1;

	VFX_shape_draw8(&pane, (VFX_SHAPETABLE *)fileImage, imageNumber, 0, 0);
	
	return result;
}
//---------------------------------------------------------------------------
//
GENRESULT VFXREADER::GetImage (U32 desiredFormat, void * buffer, const RECT * rect) const
{
	GENRESULT result = GR_OK;

	if (buffer==0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (indexMap == 0)
	{
		result = GR_GENERIC;
		goto Done;
	}

	switch (desiredFormat)
	{
	case PF_RGB:
		if (rect)
			getRGBFormat(buffer, rect);
		else
			getRGBFormat(buffer);
		break;
	
	case PF_RGBA:
		if (rect)
			getRGBAFormat(buffer, rect);
		else
			getRGBAFormat(buffer);
		break;

	case PF_COLOR_INDEX:
		if (rect)
			getIndexFormat((U8 *)buffer, rect);
		else
			memcpy(buffer, indexMap, width * height);
		break;

	default:
		result = GR_INVALID_PARMS;
		goto Done;
	}

Done:
	return result;
}
//---------------------------------------------------------------------------
//
GENRESULT VFXREADER::GetColorTable (U32 desiredFormat, void * buffer) const
{
	GENRESULT result = GR_OK;

	if (buffer==0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (indexMap == 0)
	{
		result = GR_GENERIC;
		goto Done;
	}

	switch (desiredFormat)
	{
	case PF_RGB:
		{
			const COLORREF * colors = palette;
			VFX_RGB * rgb = (VFX_RGB *) buffer;
			int i;

			for (i = 0; i < 256; i++, rgb++, colors++)
				rgb[0] = *((VFX_RGB *)(&colors[0]));
		}
		break;
	
	case PF_RGBA:
		memcpy(buffer, palette, sizeof(palette));
		break;

	default:
		result = GR_INVALID_PARMS;
		goto Done;
	}

Done:
	return result;
}
//---------------------------------------------------------------------------
//
U32 VFXREADER::GetWidth (void) const
{
	return width;
}
//---------------------------------------------------------------------------
//
U32 VFXREADER::GetHeight (void) const
{
	return height;
}
//-------------------------------------------------------------------------//
//
void VFXREADER::getRGBFormat (void *buffer) const
{
    const int pixels_left = width * height;  // number of pixels
	int i;
	VFX_RGB *rgb = (VFX_RGB *) buffer;
	U8 * indexes = indexMap;

	for (i = 0; i < pixels_left; i++, rgb++, indexes++)
		rgb[0] = *((VFX_RGB *)(&palette[indexes[0]]));
}
//-------------------------------------------------------------------------//
//
void VFXREADER::getRGBFormat (void *buffer, const RECT * rect) const
{
	VFX_RGB *rgb = (VFX_RGB *) buffer;
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (width-rectWidth):(-rectWidth-width);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const U8 * src = indexMap + rect->left + (rect->top * width);

	for (y = rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x = rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgb++, x+=i_inc)
			if (verifyPoint(x, y))
				rgb[0] = *((VFX_RGB *)(&palette[src[0]]));
	}
}
//-------------------------------------------------------------------------//
//
void VFXREADER::getRGBAFormat (void *buffer) const
{
    const int pixels_left = width * height;  // number of pixels
	COLORREF *rgba = (COLORREF *) buffer;
	U8 * indexes = indexMap;
	int i;

	for (i = 0; i < pixels_left; i++, rgba++, indexes++)
		rgba[0] = *((COLORREF *)(&palette[indexes[0]]));
}
//-------------------------------------------------------------------------//
//
void VFXREADER::getRGBAFormat (void *buffer, const RECT * rect) const
{
	COLORREF *rgba = (COLORREF *) buffer;
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (width-rectWidth):(-rectWidth-width);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const U8 * src = indexMap + rect->left + (rect->top * width);

	for (y = rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x = rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgba++, x+=i_inc)
			if (verifyPoint(x, y))
				rgba[0] = *((COLORREF *)(&palette[src[0]]));
	}
}
//-------------------------------------------------------------------------//
//
void VFXREADER::getIndexFormat (U8 *buffer, const RECT * rect) const
{
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (width-rectWidth):(-rectWidth-width);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const U8 * src = indexMap + rect->left + (rect->top * width);

	for (y = rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x = rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, x += i_inc, buffer++)
			if (verifyPoint(x, y))
				*buffer = *src;
	}
}
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//
void __stdcall CreateVFXReader (struct IImageReader ** reader)
{
	*reader = new DAComponent<VFXREADER>;
}
//-------------------------------------------------------------------------//
//----------------------------END VFXREAD.CPP------------------------------//
//-------------------------------------------------------------------------//
