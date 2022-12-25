#ifndef IIMAGESOURCE_H
#define IIMAGESOURCE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IImageSource.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/iimagesource.h 5     4/28/00 11:57p Rmarr $
*/

//
// Design Notes:
//     The interface also as a general interface for things that can expose bitmap
// data. The user interface libraries use this interface when they need access to bitmap
// data.
//     It is planned that a library system for loading bitmap files will be put into place,
// where the return value is an object that implements IImageSource.
//
/*
	//----------------------------------
	//
	GENRESULT IImageSource::GetImage (PixelFormat desiredFormat, void * buffer, int bufferStride, const RECT * rect = 0) const;
		INPUT:
			desiredFormat: description of the pixels pointed to by "buffer".
			    See Pixel.h for the description of PixelFormat.
			buffer:	User defined memory area where pixel data will be stored.
			bufferStride: bytes in each line of the destination buffer
			rect: (Optional) Address of RECT struture describing a subimage.
				The rect coordinates are inclusive, so a rectangle with a 'left' = 0, and
				a 'right' = 639 would have a width of 640.
				If 'top' is higher than 'bottom', the returned subimage is inverted along the y axis.
				If 'left' is higher than 'right', the returned subimage is inverted along the x axis.
		RETURNS:
			GR_OK if pixel data was written to the user's buffer.
			GR_GENERIC if no image has been loaded, or GL_COLOR_INDEX was specified in
				'desiredFormat' but the source is not palettized.
			GR_INVALID_PARMS if 'buffer' is NULL or 'desiredFormat' is not valid.
		OUTPUT:
			Writes pixel information to a user supplied buffer. If the user requests
			GL_RGBA format and the source image does not contain an alpha channel, the outputed
			alpha component defaults to 0xFF. If the user requests GL_COLOR_INDEX format, but
			the source image is not palettized, the method will fail.

	//----------------------------------
	//
	GENRESULT IImageSource::GetColorTable (U32 *numColors, U32 *packedARGB);
		INPUT:
			numColors: pointer to storage for the color count
			packedARGB:	storage for the color entries, in ARGB format.
		RETURNS:
			GR_OK if palette data was written to the user's buffer.
			GR_GENERIC if no image has been loaded, or the source is not palettized.
			GR_INVALID_PARMS if 'buffer' or numColors is NULL.
		OUTPUT:
			Writes the color palette from the image into the packedARGB buffer, then returns
		the number of colors written to numColors.
	//----------------------------------
	//
	GENRESULT IImageSource::GetDimensions (U32 &width, U32 &height) const;
		INPUT:
			width, height: references to storage for the width and height values
		RETURNS:
			GR_OK if all is well.
		OUTPUT:
			Width and height (in pixels) of the image.

	//----------------------------------
	//
	GENRESULT IImageSource::GetPreferredFormat (PixelFormat *preferred);
		INPUT:
			preferred: pointer to storage for the preferred pixel format.
		RETURNS:
			GR_OK if the preferred format is returned
			GR_GENERIC if 'preferred' is NULL
		OUTPUT:
			The pixel format that this image can return most efficiently.

	//--------------------------------------------------------------------------------------
*/
//------------------------------- #INCLUDES --------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef PIXEL_H
#include <Pixel.h>
#endif

typedef struct tagRECT RECT;

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IIMAGESOURCE_VERSION 2
#define IID_IImageSource MAKE_IID("IImageSource",2)

struct DACOM_NO_VTABLE IImageSource : IDAComponent
{
	// Gets the image bits in the desired format, if possible. Palettized images will be converted to
	// non-palettized images by this method, but non-palettized images will NOT be converted to
	// palettized.
	// Buffer stride is the number of bytes to go down exactly one line in the destination buffer.
	// If rect is NULL, the entire image is returned, otherwise the give rect is returned.
	DEFMETHOD(GetImage) (PixelFormat desiredFormat, void * buffer, int bufferStride, const RECT * rect = 0) = 0;

	// If the image is palettized, this will fill the given array with the palette and set numColors to
	// the count of colors in the palette. 2, 16, and 256 colors are supported.
	DEFMETHOD(GetColorTable) (U32 * numColors, U32 * packedARGB) = 0;

	// Returns the dimensions of the image
	DEFMETHOD(GetDimensions) (U32 & width, U32 & height) const = 0;

	// Returns the "preferred" pixel format for the image. This is defined as the format that 
	// GetImage() can return most efficiently.
	DEFMETHOD(GetPreferredFormat) (PixelFormat *preferred) = 0;
};    

#endif
