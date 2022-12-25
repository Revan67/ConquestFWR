//--------------------------------------------------------------------------//
//                                                                          //
//                                BmpRead.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/BmpRead.cpp 6     3/18/00 12:29a Jasony $
*/
//------------------------------- #INCLUDES --------------------------------//

#include "pch.h"
#include <globals.h>

#include "IImageReader.h"

#include <TComponent.h>

#include <malloc.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE BMP_READER : IImageReader
{
	BEGIN_DACOM_MAP_INBOUND(BMP_READER)
  	DACOM_INTERFACE_ENTRY(IImageReader)
  	END_DACOM_MAP()
		
  	//------------------------------
  	// data members
  	//------------------------------
  
	BITMAPINFOHEADER bmpheader;
	LONG trueHeight;
	COLORREF  palette[256];
	
	U8 *		indexMap;
	COLORREF *	colorMap;
	
	
	BMP_READER (void) { };
	
	~BMP_READER (void) { free(); }
	
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
	
	/* BMP_READER methods */

	void loadIndexBits (BITMAPINFOHEADER * header);

	void depalettize (BITMAPINFOHEADER * header);
	
	void getRGBFormat (void *buffer) const;

	void getRGBAFormat (void *buffer) const;

	void getIndexFormat (U8 *buffer, const RECT * rect) const;

	void getRGBFormat (void *buffer, const RECT * rect) const;

	void getRGBAFormat (void *buffer, const RECT * rect) const;

	BOOL32 verifyPoint (S32 x, S32 y) const
	{
		if ((U32)y >= (U32)trueHeight || (U32)x >= (U32)bmpheader.biWidth)
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
//-----------------------------BMP_READER METHODS ---------------------------
//---------------------------------------------------------------------------

void BMP_READER::free (void)
{
    ::free(indexMap);
    ::free(colorMap);
	indexMap = 0;
	colorMap = 0;
}
//------------------------------------------------------------------------
//
void BMP_READER::loadIndexBits (BITMAPINFOHEADER * header)
{
	RGBQUAD *_palette;
	int i, paletteSize;
	U8 * indexTable;

	_palette = (RGBQUAD *) (header+1);

	if ((paletteSize = header->biClrUsed) == 0)
		paletteSize = (1 << header->biBitCount);
	indexTable = (U8 *) (_palette+paletteSize);

	for (i = 0; i < paletteSize; i++)
		palette[i] = RGB(_palette[i].rgbRed, _palette[i].rgbGreen, _palette[i].rgbBlue) | 0xFF000000;

    ::free(indexMap);
	indexMap = (U8 *) malloc(header->biWidth * trueHeight * sizeof(U8));

	// write it upside down for proper GL behavior
	if (header->biBitCount == 8)
	{
		U8 * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = indexMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize; j++)
				buffer[j] = indexTable[j];
			buffer -= rowSize;
			indexTable += rowSize;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 4)
	{
		U8 * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = indexMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize/2; j++)
			{
				buffer[j*2]     = indexTable[j] >> 4;
				buffer[(j*2)+1] = indexTable[j] & 0xF;
			}
			buffer -= rowSize;
			indexTable += (rowSize+1)/2;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 1)
	{
		U8 * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = indexMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize/8; j++)
			{
				buffer[j*8]     = (indexTable[j] >> 7) & 1;
				buffer[(j*8)+1] = (indexTable[j] >> 6) & 1;
				buffer[(j*8)+2] = (indexTable[j] >> 5) & 1;
				buffer[(j*8)+3] = (indexTable[j] >> 4) & 1;
				buffer[(j*8)+4] = (indexTable[j] >> 3) & 1;
				buffer[(j*8)+5] = (indexTable[j] >> 2) & 1;
				buffer[(j*8)+6] = (indexTable[j] >> 1) & 1;
				buffer[(j*8)+7] = (indexTable[j] >> 0) & 1;
			}
			buffer -= rowSize;
			indexTable += (rowSize+1)/8;
			numRows--;
		}
	}
}
//------------------------------------------------------------------------
//
void BMP_READER::depalettize (BITMAPINFOHEADER * header)
{
	COLORREF rgb[256];
	U8 * indexTable, * maskTable=0;

	if (header->biBitCount <= 8)	// indexed color?
	{
		RGBQUAD *_palette;
		int i, paletteSize;

		_palette = (RGBQUAD *) (header+1);

		if ((paletteSize = header->biClrUsed) == 0)
			paletteSize = (1 << header->biBitCount);

		indexTable = (U8 *) (_palette+paletteSize);
		if (header->biHeight == trueHeight * 2)
			maskTable = indexTable + (((header->biWidth*trueHeight)*header->biBitCount) / 8);

		for (i = 0; i < paletteSize; i++)
			rgb[i] = RGB(_palette[i].rgbRed, _palette[i].rgbGreen, _palette[i].rgbBlue);
	}
	else
	{
		indexTable = (U8 *) (header + 1);
	}

    ::free(colorMap);
	colorMap = (COLORREF *) malloc(header->biWidth * trueHeight * sizeof(COLORREF));

	// write it upside down for proper GL behavior
	if (header->biBitCount == 8)
	{
		COLORREF * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = colorMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize; j++)
			{
				buffer[j] = rgb[indexTable[j]];
				if (maskTable==0 || (maskTable[j/8] & (0x80 >> (j & 7))) == 0)
					buffer[j] |= 0xFF000000;	// set the alpha bits
			}
			buffer -= rowSize;
			indexTable += rowSize;
			if (maskTable)
				maskTable += (rowSize + 7) / 8;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 4)
	{
		COLORREF * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = colorMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize/2; j++)
			{
				buffer[j*2]     = rgb[indexTable[j] >> 4];
				buffer[(j*2)+1] = rgb[indexTable[j] & 0xF];
				if (maskTable)
				{
					if ((maskTable[(j*2)/8] & (0x80 >> ((j*2) & 7))) == 0)
						buffer[j*2] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[((j*2)+1)/8] & (0x80 >> (((j*2)+1) & 7))) == 0)
						buffer[(j*2)+1] |= 0xFF000000;	// set the alpha bits
				}
				else
				{
					buffer[j*2] |= 0xFF000000;	// set the alpha bits
					buffer[(j*2)+1] |= 0xFF000000;	// set the alpha bits
				}
			}
			buffer -= rowSize;
			indexTable += (rowSize+1)/2;
			if (maskTable)
				maskTable += (rowSize + 7) / 8;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 1)
	{
		COLORREF * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;

		buffer = colorMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize/8; j++)
			{
				buffer[j*8]     = rgb[(indexTable[j] >> 7) & 1];
				buffer[(j*8)+1] = rgb[(indexTable[j] >> 6) & 1];
				buffer[(j*8)+2] = rgb[(indexTable[j] >> 5) & 1];
				buffer[(j*8)+3] = rgb[(indexTable[j] >> 4) & 1];
				buffer[(j*8)+4] = rgb[(indexTable[j] >> 3) & 1];
				buffer[(j*8)+5] = rgb[(indexTable[j] >> 2) & 1];
				buffer[(j*8)+6] = rgb[(indexTable[j] >> 1) & 1];
				buffer[(j*8)+7] = rgb[(indexTable[j] >> 0) & 1];
				
				if (maskTable)
				{
					if ((maskTable[j] & (0x80 >> 0)) == 0)
						buffer[j*8] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 1)) == 0)
						buffer[(j*8)+1] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 2)) == 0)
						buffer[(j*8)+2] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 3)) == 0)
						buffer[(j*8)+3] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 4)) == 0)
						buffer[(j*8)+4] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 5)) == 0)
						buffer[(j*8)+5] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 6)) == 0)
						buffer[(j*8)+6] |= 0xFF000000;	// set the alpha bits
					if ((maskTable[j] & (0x80 >> 7)) == 0)
						buffer[(j*8)+7] |= 0xFF000000;	// set the alpha bits
				}
				else
				{
					buffer[j*8] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+1] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+2] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+3] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+4] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+5] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+6] |= 0xFF000000;	// set the alpha bits
					buffer[(j*8)+7] |= 0xFF000000;	// set the alpha bits
				}
			}
			buffer -= rowSize;
			indexTable += (rowSize+7)/8;
			if (maskTable)
				maskTable += (rowSize + 7) / 8;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 32)
	{
		COLORREF * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;
		VFX_CRGB * cIndexTable = (VFX_CRGB *) indexTable;

		buffer = colorMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize; j++)
				buffer[j] = RGB(cIndexTable[j].rgb.b, cIndexTable[j].rgb.g, cIndexTable[j].rgb.r) | (((U32)cIndexTable[j].color) << 24);
			buffer -= rowSize;
			cIndexTable += rowSize;
			numRows--;
		}
	}
	else
	if (header->biBitCount == 24)
	{
		COLORREF * buffer;
		U32 rowSize = header->biWidth;
		U32 numRows = trueHeight;
		U32 j;
		VFX_RGB * cIndexTable = (VFX_RGB *) indexTable;

		buffer = colorMap + (rowSize * (numRows-1));

		while (numRows)
		{
			for (j = 0; j < rowSize; j++)
				buffer[j] = RGB(cIndexTable[j].b, cIndexTable[j].g, cIndexTable[j].r) | 0xFF000000;
			buffer -= rowSize;
			cIndexTable += rowSize;
			numRows--;
		}
	}
}
//---------------------------------------------------------------------------
//
GENRESULT BMP_READER::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber)
{
	GENRESULT result = GR_OK;
	BITMAPINFOHEADER *header = (BITMAPINFOHEADER *) fileImage;

	free();

	if (header->biSize!=sizeof(bmpheader))
		result = GR_GENERIC;
	else
	{
		bmpheader = *header;
		trueHeight = header->biHeight;
		if (header->biBitCount <= 8)	// indexed color?
		{
			if (header->biSizeImage)
			{
				trueHeight = (header->biSizeImage * 8) / (header->biWidth * header->biBitCount);
				if (trueHeight != header->biHeight)
					trueHeight = header->biHeight / 2;
			}
			loadIndexBits(header);
		}

		depalettize(header);
	}

	return result;
}
//---------------------------------------------------------------------------
//
GENRESULT BMP_READER::GetImage (U32 desiredFormat, void * buffer, const RECT * rect) const
{
	GENRESULT result = GR_OK;

	if (buffer==0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (indexMap == 0 && colorMap==0)
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
	    if (bmpheader.biBitCount <= 8)	// if image really is 8 bit indexed
		{
			if (rect)
				getIndexFormat((U8 *)buffer, rect);
			else
				memcpy(buffer, indexMap, bmpheader.biWidth * trueHeight);
		}
		else	// not indexed, can't convert it
			result = GR_GENERIC;
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
GENRESULT BMP_READER::GetColorTable (U32 desiredFormat, void * buffer) const
{
	GENRESULT result = GR_OK;

	if (buffer==0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (indexMap == 0 && colorMap==0)
	{
		result = GR_GENERIC;
		goto Done;
	}

	switch (desiredFormat)
	{
	case PF_RGB:
	    if (bmpheader.biBitCount <= 8)	// if image really is 8 bit indexed
		{
			const COLORREF * colors = palette;
			VFX_RGB * rgb = (VFX_RGB *) buffer;
			int i;

			for (i = 0; i < 256; i++, rgb++, colors++)
				rgb[0] = *((VFX_RGB *)(&colors[0]));
		}
		else
			result = GR_GENERIC;
		break;
	
	case PF_RGBA:
	    if (bmpheader.biBitCount <= 8)	// if image really is 8 bit indexed
			memcpy(buffer, palette, sizeof(palette));
		else
			result = GR_GENERIC;
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
U32 BMP_READER::GetWidth (void) const
{
	return bmpheader.biWidth;
}
//---------------------------------------------------------------------------
//
U32 BMP_READER::GetHeight (void) const
{
	return trueHeight;
}
//-------------------------------------------------------------------------//
//
void BMP_READER::getRGBFormat (void *buffer) const
{
    const int pixels_left = bmpheader.biWidth * trueHeight;  // number of pixels
	int i;
	VFX_RGB *rgb = (VFX_RGB *) buffer;
	COLORREF * colors = colorMap;

	for (i = 0; i < pixels_left; i++, rgb++, colors++)
		rgb[0] = *((VFX_RGB *)(&colors[0]));
}
//-------------------------------------------------------------------------//
//
void BMP_READER::getRGBFormat (void *buffer, const RECT * rect) const
{
	VFX_RGB *rgb = (VFX_RGB *) buffer;
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (bmpheader.biWidth-rectWidth):(-rectWidth-bmpheader.biWidth);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const COLORREF * src = colorMap + rect->left + (rect->top * bmpheader.biWidth);

	for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgb++, x+=i_inc)
			if (verifyPoint(x, y))
				rgb[0] = *((VFX_RGB *)(&src[0]));
	}
}
//-------------------------------------------------------------------------//
//
void BMP_READER::getRGBAFormat (void *buffer) const
{
    const int pixels_left = bmpheader.biWidth * trueHeight;  // number of pixels

	memcpy(buffer, colorMap, pixels_left * sizeof(COLORREF));
}
//-------------------------------------------------------------------------//
//
void BMP_READER::getRGBAFormat (void *buffer, const RECT * rect) const
{
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (bmpheader.biWidth-rectWidth):(-rectWidth-bmpheader.biWidth);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const COLORREF * src = colorMap + rect->left + (rect->top * bmpheader.biWidth);
	COLORREF *rgba = (COLORREF *) buffer;

	for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc,y+=j_inc)
	{
		for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, x+=i_inc, rgba++)
			if (verifyPoint(x, y))
				*rgba = *src;
	}
}
//-------------------------------------------------------------------------//
//
void BMP_READER::getIndexFormat (U8 *buffer, const RECT * rect) const
{
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (bmpheader.biWidth-rectWidth):(-rectWidth-bmpheader.biWidth);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x, y;

	const U8 * src = indexMap + rect->left + (rect->top * bmpheader.biWidth);

	for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, buffer++, x+=i_inc)
			if (verifyPoint(x, y))
				*buffer = *src;
	}
}
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//
void __stdcall CreateBMPReader (struct IImageReader ** reader)
{
	*reader = new DAComponent<BMP_READER>;
}
//-------------------------------------------------------------------------//
//----------------------------END BmpRead.CPP------------------------------//
//-------------------------------------------------------------------------//
