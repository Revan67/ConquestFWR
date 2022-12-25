//--------------------------------------------------------------------------//
//                                                                          //
//                                TGAREAD.CPP                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/Tgaread.cpp 5     3/18/00 12:31a Jasony $
*/
//------------------------------- #INCLUDES --------------------------------//

#include "pch.h"
#include <globals.h>
#include "IImageReader.h"
#include "CQTrace.h"

#include <TComponent.h>
#include <HeapObj.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#pragma pack( push, 1)
struct TGAHEADER
{
    U8 idlength;          /* length of image ID string */
    U8 colormaptype;      /* 1 if a color-map is included */
	U8 imagetype;         /* Image type: 0 => no image
								1 => color-mapped
								2 => true color
								3 => monochrome  */

	U16 colormapstart;    /* index to start of map */
	U16 colormaplength;   /* number of map entries */
	U8 colormapbits;      /* number of bits per entry */
	U16 xstart;			  /* top left corner of image */
	U16 ystart;
	U16 width;            /* size of image in pixels */
	U16 depth;
	U8 bits;              /* number of bits per pixel */
	U8 descriptor;        /* 0x20 means start at top left
							 0x21 means top right
							 0x00 means bottom left
							 0x01 means bottom right
							 (generally, always use 0x20) */
};
#pragma pack( pop )

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE TGAREADER : IImageReader
{
	BEGIN_DACOM_MAP_INBOUND(TGAREADER)
  	DACOM_INTERFACE_ENTRY(IImageReader)
  	END_DACOM_MAP()
		
  	//------------------------------
  	// data members
  	//------------------------------
  
	TGAHEADER tgaheader;
	COLORREF  palette[256];
	U32 bytesPerPixel;
	
	union 
	{
		U8 *		indexMap;
		COLORREF *	colorMap;
	};
	
	
	TGAREADER (void) { };
	
	~TGAREADER (void) { free(); }
	
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
	
	/* TGAREADER methods */
	
	void decompressTarga (const U8 * srcData, U8 * dstData);
	
	U8 * readHeader (void * fileImage);
	
	void readPixels (void * pixelImage);

	void getRGBFormat (void *buffer) const;

	void getRGBAFormat (void *buffer) const;

	void getIndexFormat (U8 *buffer, const RECT * rect) const;

	void getRGBFormat (void *buffer, const RECT * rect) const;

	void getRGBAFormat (void *buffer, const RECT * rect) const;
	
	U8 readByte (U8 * & fileImage)
	{
		return *fileImage++;
	}

	U16 readShort (U8 * & fileImage)
	{
		U16 result;

		result = *((U16 *)fileImage);
		fileImage += sizeof(U16);

		return result;
	}
	
	U32 readLong (U8 * & fileImage)
	{
		U32 result;
		
		result = *((U32 *)fileImage);
		fileImage += sizeof(U32);

		return result;
	}

	BOOL32 verifyPoint (S32 x, S32 y) const
	{
		if ((U32)y >= tgaheader.depth || (U32)x >= tgaheader.width)
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
//-----------------------------TGAREADER METHODS ----------------------------
//---------------------------------------------------------------------------

void TGAREADER::free (void)
{
    ::free(indexMap);
	indexMap = 0;
}
//---------------------------------------------------------------------------
//
GENRESULT TGAREADER::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber)
{
	GENRESULT result = GR_OK;
	U8 * pixelImage;

	free();

	pixelImage = readHeader(fileImage);

	if (bytesPerPixel==0)
		result = GR_GENERIC;
	else
		readPixels(pixelImage);

	return result;
}
//---------------------------------------------------------------------------
//
GENRESULT TGAREADER::GetImage (U32 desiredFormat, void * buffer, const RECT * rect) const
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
	    if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
		{
			if (rect)
				getIndexFormat((U8 *)buffer, rect);
			else
				memcpy(buffer, indexMap, tgaheader.width * tgaheader.depth);
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
GENRESULT TGAREADER::GetColorTable (U32 desiredFormat, void * buffer) const
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
	    if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
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
	    if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
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
U32 TGAREADER::GetWidth (void) const
{
	return tgaheader.width;
}
//---------------------------------------------------------------------------
//
U32 TGAREADER::GetHeight (void) const
{
	return tgaheader.depth;
}
//---------------------------------------------------------------------------
// buffer contains compressed data
// uncompress into another buffer, already allocated
//
void TGAREADER::decompressTarga (const U8 * srcData, U8 * dstData)
{
    const U8 * src = srcData;
    U8 * dst = dstData;
    U8 count;
    int pixels_left = tgaheader.width * tgaheader.depth;  // number of pixels
    int size;
    int i;
    
	//    if ((tmp.alloc(pixels_left*4)) == 0)
	//      	::Fatal("No memory for Targa decompression.\n");
    
    while (pixels_left > 0) 
    {
        count = *src++;
        size = (count & 0x7f)+1;
        pixels_left -= size;    
		
        switch(count&0x80)
        {
		case 0x80:      /* repeating colors */
			switch(tgaheader.bits)
			{
			case 1:
				//("Unsupported TARGA format.\n");
				break;
			case 8:     /* compressed 8bit palette driven */
				for (i=0; i < size; i++)
					*dst++ = *src;  
				src++;
				break;
			case 16:    /* compressed 16bit RGB pixels */
				for (i=0; i < size; i++)
				{
					(*((unsigned short *) dst)) = *((unsigned short *) src);
					dst += sizeof(short);
				}
				src += sizeof(short);
				break;
			case 24:    /* compressed 24bit RGB pixels */
				for (i=0; i < size; i++)
				{
					(*((VFX_RGB *) dst)) = *((VFX_RGB *) src);
					dst += sizeof(VFX_RGB);
				}
				src += sizeof(VFX_RGB);
				break;
			case 32:    /* compressed 32bit RGB pixels */
				for (i=0; i < size; i++)
				{
					(*((COLORREF *) dst)) = *((COLORREF *) src);
					dst += sizeof(COLORREF);
				}
				src += sizeof(COLORREF);
				break;
			}
			break;
            default:            /* literal colors */
                switch(tgaheader.bits)
                {
				case 1:
					CQBOMB0("Unsupported TARGA format.");
					break;
				case 8:     /* compressed 8bit palette driven */
					for (i=0; i < size; i++)
						*dst++ = *src++;    
					break;
				case 16:    /* compressed 16bit RGB pixels */
					for (i=0; i < size; i++)
					{
						*((unsigned short *) dst) = *((unsigned short *) src);
						dst += sizeof(short);
						src += sizeof(short);
					}
					break;
				case 24:    /* compressed 24bit RGB pixels */
					for (i=0; i < size; i++)
					{
						*((VFX_RGB *) dst) = *((VFX_RGB *) src);
						dst += sizeof(VFX_RGB);
						src += sizeof(VFX_RGB);
					}
					break;
				case 32:    /* compressed 32bit RGB pixels */
					for (i=0; i < size; i++)
					{
						(*((COLORREF *) dst)) = *(((COLORREF *) src));
						dst += sizeof(COLORREF);
						src += sizeof(COLORREF);
					}
					break;
                }
                break;
        }
    } 
}
//---------------------------------------------------------------------------
// read the header 
//
U8 * TGAREADER::readHeader (void * fileImage)
{
	U8 * imagePtr = (U8 *) fileImage;
    
    /* read the file header */

    tgaheader.idlength = readByte(imagePtr);
    tgaheader.colormaptype = readByte(imagePtr);
    tgaheader.imagetype = readByte(imagePtr);
    tgaheader.colormapstart = readShort(imagePtr);
    tgaheader.colormaplength = readShort(imagePtr);
    tgaheader.colormapbits = readByte(imagePtr);
    tgaheader.xstart = readShort(imagePtr);
    tgaheader.ystart = readShort(imagePtr);
    tgaheader.width = readShort(imagePtr);
    tgaheader.depth = readShort(imagePtr);
    tgaheader.bits = readByte(imagePtr);
    tgaheader.descriptor = readByte(imagePtr);

	imagePtr += tgaheader.idlength;  // skip optional bogus info
	
    /* read the color table if there is one */
    
    if ((tgaheader.colormaptype) && (tgaheader.bits == 8))
    {
		//        if ((palette = (RGB *) calloc( tgaheader.colormaplength, sizeof(RGB))) == NULL)
		//            ::Fatal("Out of memory.\n");
		
        for(U16 i = 0; i < tgaheader.colormaplength; i++)
        {
		    U8 r, g, b, a;

			r = g = b = 0;
			a = 0xFF;

            switch(tgaheader.colormapbits)
            {
			case 8:
				{
					U8 tmp = readByte(imagePtr);
					r = tmp & 0xE0;
					g = (tmp << 3) & 0xE0;
					b = tmp << 6;
				}
				break;

			case 15:             /* five bits per color */
				{
					U16 tmp = readShort(imagePtr);
					r = (tmp & 0x7C00) >> (10-3);
					g = (tmp & 0x03E0) >> (5-3);
					b = tmp << 3;
				}
				break;
			case 16:             /* 5-6-5 bits per color */
				{
					U16 tmp = readShort(imagePtr);
					r = (tmp & 0xF800) >> (11-3);
					g = (tmp & 0x07E0) >> (5-2);
					b = tmp << 3;
				}
				break;
			case 32:          /* 32 bits per color */
				a = readByte(imagePtr);
				// fall through intentional
					
			case 24:   /* eight bits per color -- easy! */
				b = readByte(imagePtr);
				g = readByte(imagePtr);
				r = readByte(imagePtr);
				break;
            }
			
			palette[i] = RGB(r,g,b) | (((DWORD)a)<<24);
        }
    }
	
    bytesPerPixel = (tgaheader.bits + 7) / 8;
	return imagePtr;
}
//---------------------------------------------------------------------------
// always make pixels into COLORREF values (4 bytes per pixels) or index values
// assumes file pointer is positioned after the header
//
void TGAREADER::readPixels (void * pixelImage)
{
    int x, y, c;
    int xinc, yinc, xstart, ystart, xend, yend;
	U8 * tmpIndexMap, * fptr;
	const int allocSize = tgaheader.width * tgaheader.depth * ((tgaheader.colormaptype)?1:sizeof(COLORREF));
	bool allocatedTmp=false;

	::free(indexMap);
    indexMap = (U8 *) calloc (allocSize, 1);

	//
	// if compressed image, uncompress it
	//

    if (tgaheader.imagetype & 8) 
	{
	    tmpIndexMap = (U8 *) calloc (allocSize, 1);
		allocatedTmp = true;

        decompressTarga((U8 *)pixelImage, tmpIndexMap);
        tgaheader.imagetype &= ~8;
	}
	else	// not compressed
	{
		tmpIndexMap = (U8 *) pixelImage;
	}
  
	fptr = tmpIndexMap;


    /* which corner do we start in? */
	
    if (tgaheader.descriptor & 0x20)       /* from the top down */
        yinc = 1,
        ystart = tgaheader.ystart,
        yend = tgaheader.ystart + tgaheader.depth;
    else                             /* from the bottom up */
        yinc = -1,
        ystart = tgaheader.ystart + tgaheader.depth - 1,
        yend = tgaheader.ystart - 1;
    if (tgaheader.descriptor & 0x10)       /* right to left */
        xinc = -1,
        xstart = tgaheader.xstart + tgaheader.width - 1,
        xend = tgaheader.xstart - 1;
    else                             /* left to right */
        xinc = 1,
        xstart = tgaheader.xstart,
        xend = tgaheader.xstart + tgaheader.width;
	
    for (y = ystart; y != yend; y += yinc)
    {   
        for (x = xstart; x !=xend; x += xinc)
        {
            if (tgaheader.colormaptype)   /* color map -- easy! */
            {   
                c = *fptr++;
                indexMap[x + (y*tgaheader.width)] = c;
            }
            else
            {
				U8 r, g, b, a;

				a = 0xFF;
				r = g = b = 0;

				switch(tgaheader.bits)
				{
				case 8:
					{
						U8 tmp = *fptr++;
						r = tmp & 0xE0;
						g = (tmp << 3) & 0xE0;
						b = tmp << 6;
					}
					break;

				case 15:             /* five bits per color */
					{
						U16 tmp = *((U16 *)fptr);
						fptr += sizeof(U16);
						r = (tmp & 0x7C00) >> (10-3);
						g = (tmp & 0x03E0) >> (5-3);
						b = tmp << 3;
					}
					break;
				case 16:             /* 5-6-5 bits per color */
					{
						U16 tmp = *((U16 *)fptr);
						fptr += sizeof(U16);
						r = (tmp & 0xF800) >> (11-3);
						g = (tmp & 0x07E0) >> (5-2);
						b = tmp << 3;
					}
					break;
				case 32:          /* 32 bits per color */
					b = *fptr++;
					g = *fptr++;
					r = *fptr++;
					a = *fptr++;
					break;
						
				case 24:   /* eight bits per color -- easy! */
					b = *fptr++;
					g = *fptr++;
					r = *fptr++;
					break;
				}

                colorMap[x + (y*tgaheader.width)] = RGB(r,g,b) | (((DWORD)a)<<24);
            }
        }
    }
	
	if (allocatedTmp)
		::free(tmpIndexMap);
}
//-------------------------------------------------------------------------//
//
void TGAREADER::getRGBFormat (void *buffer) const
{
    const int pixels_left = tgaheader.width * tgaheader.depth;  // number of pixels
	int i;
	VFX_RGB *rgb = (VFX_RGB *) buffer;

	if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
	{
		U8 * indexes = indexMap;

		for (i = 0; i < pixels_left; i++, rgb++, indexes++)
			rgb[0] = *((VFX_RGB *)(&palette[indexes[0]]));
	}
	else
	{
		COLORREF * colors = colorMap;

		for (i = 0; i < pixels_left; i++, rgb++, colors++)
			rgb[0] = *((VFX_RGB *)(&colors[0]));
	}
}
//-------------------------------------------------------------------------//
//
void TGAREADER::getRGBFormat (void *buffer, const RECT * rect) const
{
	VFX_RGB *rgb = (VFX_RGB *) buffer;

	if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
	{
		S32 rectWidth = rect->right - rect->left;
		S32 rectHeight = rect->bottom - rect->top;
		S32 i_inc = (rectWidth>0) ? 1:-1;
		rectWidth += i_inc;
		S32 y_inc = (rectHeight>0)? (tgaheader.width-rectWidth):(-rectWidth-tgaheader.width);
		S32 j_inc = (rectHeight>0)? 1:-1;
		rectHeight += j_inc;
		S32 i,j;
		S32 x, y;

		const U8 * src = indexMap + rect->left + (rect->top * tgaheader.width);

		for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
		{
			for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgb++, x+=i_inc)
				if (verifyPoint(x,y))
					rgb[0] = *((VFX_RGB *)(&palette[src[0]]));
		}
	}
	else
	{
		S32 rectWidth = rect->right - rect->left;
		S32 rectHeight = rect->bottom - rect->top;
		S32 i_inc = (rectWidth>0) ? 1:-1;
		rectWidth += i_inc;
		S32 y_inc = (rectHeight>0)? (tgaheader.width-rectWidth):(-rectWidth-tgaheader.width);
		S32 j_inc = (rectHeight>0)? 1:-1;
		rectHeight += j_inc;
		S32 i,j;
		S32 x, y;

		const COLORREF * src = colorMap + rect->left + (rect->top * tgaheader.width);

		for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
		{
			for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgb++, x+=i_inc)
				if (verifyPoint(x,y))
					rgb[0] = *((VFX_RGB *)(&src[0]));
		}
	}
}
//-------------------------------------------------------------------------//
//
void TGAREADER::getRGBAFormat (void *buffer) const
{
    const int pixels_left = tgaheader.width * tgaheader.depth;  // number of pixels

	if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
	{
		COLORREF *rgba = (COLORREF *) buffer;
		U8 * indexes = indexMap;
		int i;

		for (i = 0; i < pixels_left; i++, rgba++, indexes++)
			rgba[0] = *((COLORREF *)(&palette[indexes[0]]));
	}
	else
	{
		memcpy(buffer, colorMap, pixels_left * sizeof(COLORREF));
	}
}
//-------------------------------------------------------------------------//
//
void TGAREADER::getRGBAFormat (void *buffer, const RECT * rect) const
{
	if ((tgaheader.colormaptype) && (tgaheader.bits == 8))	// if image really is 8 bit indexed
	{
		COLORREF *rgba = (COLORREF *) buffer;
		S32 rectWidth = rect->right - rect->left;
		S32 rectHeight = rect->bottom - rect->top;
		S32 i_inc = (rectWidth>0) ? 1:-1;
		rectWidth += i_inc;
		S32 y_inc = (rectHeight>0)? (tgaheader.width-rectWidth):(-rectWidth-tgaheader.width);
		S32 j_inc = (rectHeight>0)? 1:-1;
		rectHeight += j_inc;
		S32 i,j;
		S32 x,y;

		const U8 * src = indexMap + rect->left + (rect->top * tgaheader.width);

		for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
		{
			for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, rgba++, x+=i_inc)
				if (verifyPoint(x,y))
					rgba[0] = *((COLORREF *)(&palette[src[0]]));
		}
	}
	else  // source is RGBA
	{
		S32 rectWidth = rect->right - rect->left;
		S32 rectHeight = rect->bottom - rect->top;
		S32 i_inc = (rectWidth>0) ? 1:-1;
		rectWidth += i_inc;
		S32 y_inc = (rectHeight>0)? (tgaheader.width-rectWidth):(-rectWidth-tgaheader.width);
		S32 j_inc = (rectHeight>0)? 1:-1;
		rectHeight += j_inc;
		S32 i,j;
		S32 x,y;

		const COLORREF * src = colorMap + rect->left + (rect->top * tgaheader.width);
		COLORREF *rgba = (COLORREF *) buffer;

		for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
		{
			for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, x+=i_inc, rgba++)
				if (verifyPoint(x,y))
					*rgba = *src;
		}
	}
}
//-------------------------------------------------------------------------//
//
void TGAREADER::getIndexFormat (U8 *buffer, const RECT * rect) const
{
	S32 rectWidth = rect->right - rect->left;
	S32 rectHeight = rect->bottom - rect->top;
	S32 i_inc = (rectWidth>0) ? 1:-1;
	rectWidth += i_inc;
	S32 y_inc = (rectHeight>0)? (tgaheader.width-rectWidth):(-rectWidth-tgaheader.width);
	S32 j_inc = (rectHeight>0)? 1:-1;
	rectHeight += j_inc;
	S32 i,j;
	S32 x,y;

	const U8 * src = indexMap + rect->left + (rect->top * tgaheader.width);

	for (y=rect->top, j = 0; j != rectHeight; j += j_inc, src += y_inc, y+=j_inc)
	{
		for (x=rect->left, i = 0; i != rectWidth; i += i_inc, src += i_inc, x+=i_inc, buffer++)
			if (verifyPoint(x,y))
				*buffer = *src;
	}
}
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//
void __stdcall CreateTGAReader (struct IImageReader ** reader)
{
	*reader = new DAComponent<TGAREADER>;
}
//-------------------------------------------------------------------------//
//----------------------------END TGAREAD.CPP------------------------------//
//-------------------------------------------------------------------------//
