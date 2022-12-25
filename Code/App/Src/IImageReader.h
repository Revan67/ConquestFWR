#ifndef IIMAGEREADER_H
#define IIMAGEREADER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IImageReader.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IImageReader.h 2     3/15/99 1:33p Rmarr $
*/

/*
	//----------------------------------
	//
	GENRESULT IImageReader::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber);
		INPUT:
			fileImage: Points to a memory mapping image of the file.
			fileSize: Size (in bytes) of the memory mapped image.
			imageNumber: Specifies which image within the file to load.
		RETURNS:
			GR_OK if the image loaded correctly.
			GR_GENERIC if the format of the image was incorrect or unrecognized.
		OUTPUT:
			Loads the header and pixel information from the file. This method makes
			a local copy of the data.

	//----------------------------------
	//
	GENRESULT IImageReader::GetImage (U32 desiredFormat, void * buffer, const RECT * rect = 0) const;
		INPUT:
			desiredFormat: Can be one of the following (see Display.h / GL.h)
				PF_RGB:			Output 24 bit color pixel info.
				PF_RGBA:		Output 32 bit color pixel info. 
				PF_COLOR_INDEX:	Output 8 bit indexed color info.
			buffer:	User defined memory area where pixel data will be stored.
			rect: (Optional) Address of RECT struture describing a subimage.
				The rect coordinates are inclusive, so a rectangle with a 'left' = 0, and
				a 'right' = 639 would have a width of 640.
				If 'top' is higher than 'bottom', the returned subimage is inverted along the y axis.
				If 'left' is higher than 'right', the returned subimage is inverted along the x axis.
		RETURNS:
			GR_OK if pixel data was written to the user's buffer.
			GR_GENERIC if no image has been loaded, or PF_COLOR_INDEX was specified in
				'desiredFormat' but the source is not palettized.
			GR_INVALID_PARMS if 'buffer' is NULL or 'desiredFormat' is not valid.
		OUTPUT:
			Writes pixel information to a user supplied buffer. If the user requests
			PF_RGBA format and the source image does not contain an alpha channel, the outputed
			alpha component defaults to 0xFF. If the user requests PF_COLOR_INDEX format, but
			the source image is not palettized, the method will fail.

	//----------------------------------
	//
	GENRESULT IImageReader::GetColorTable (U32 desiredFormat, void * buffer) const;
		INPUT:
			desiredFormat: Can be one of the following (see Display.h / GL.h)
				PF_RGB:			Output 24 bit color pixel info.
				PF_RGBA:		Output 32 bit color pixel info. 
			buffer:	User defined memory area where palette data will be stored.
		RETURNS:
			GR_OK if palette data was written to the user's buffer.
			GR_GENERIC if no image has been loaded, or the source is not palettized.
			GR_INVALID_PARMS if 'buffer' is NULL or 'desiredFormat' is not valid.
		OUTPUT:
			Writes 256 palette entries to a user supplied buffer. If the user requests
			PF_RGBA format and the source palette does not contain an alpha channel, the 
			alpha component defaults to 0xFF in the outputed palette.
	//----------------------------------
	//
	U32 IImageReader::GetWidth (void) const;
		RETURNS:
			Width (in pixels) of the image.

	//----------------------------------
	//
	U32 IImageReader::GetHeight (void) const;
		RETURNS:
			Height (in pixels) of the image.


	//--------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------
*/
//------------------------------- #INCLUDES --------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

typedef struct tagRECT RECT;

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

struct DACOM_NO_VTABLE IImageReader : IDAComponent
{
	DEFMETHOD(LoadImage) (void *fileImage, U32 fileSize, U32 imageNumber=0) = 0;

	DEFMETHOD(GetImage) (U32 desiredFormat, void * buffer, const RECT * rect = 0) const = 0;

	DEFMETHOD(GetColorTable) (U32 desiredFormat, void * buffer) const = 0;

	DEFMETHOD_(U32,GetWidth) (void) const = 0;

	DEFMETHOD_(U32,GetHeight) (void) const = 0;
};    



#endif
