//
// BMPTexture.cpp - BMP based texture support
//

//
// Include files
//

#include "stdafx.h"
#include <fdump.h>
#include <tempstr.h>

#include "bmptexture.h"

//
// External variables
//

extern IRenderPipeline *PIPE;

//
// Methods
//

bool BMPTexture::create (const char *filename)
{
	ASSERT (tx_handle == HTX_INVALID);

	// Load the bitmap.
	HANDLE hBmp;
	bool result = false;

	hBmp = 
		LoadImage 
		(
			NULL,
			filename,
			IMAGE_BITMAP,
			0, 0,  
			LR_CREATEDIBSECTION | LR_LOADFROMFILE
		);

	ASSERT (hBmp != NULL);

	// Get its information and confirm that it is a format we want.
	// Only 16 bit, uncompressed formats are supported, for now.

	DIBSECTION dib;
	
	int gotit = GetObject ((HGDIOBJ) hBmp, sizeof(dib), &dib);
	ASSERT (gotit != 0);

	int w, h, bpp;
	int rbits, gbits, bbits;
	int stride;

	w = dib.dsBm.bmWidth;
	h = dib.dsBm.bmHeight;
	bpp = dib.dsBm.bmBitsPixel;
	stride = dib.dsBm.bmWidthBytes;
	rbits = dib.dsBitfields[0];
	gbits = dib.dsBitfields[1];
	bbits = dib.dsBitfields[2];
	void *bits = dib.dsBm.bmBits;
	int mipLevels = 0;

	PixelFormat pf (bpp, rbits, gbits, bbits, 0);

	if (dib.dsBmih.biCompression != BI_RGB /*|| bpp != 16*/)
	{
		goto done;
	}

	// Create a new texture.

	ASSERT (PIPE != NULL);
	
	if (PIPE->create_texture (w, h, pf, mipLevels, tx_handle) != GR_OK)
	{
		GENERAL_ERROR (TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, bpp));
		goto done;
	}

	// Write the pixels into the new texture.
	if (PIPE->set_texture_level_data (tx_handle, mipLevels, pf, w, h, stride, pf, bits, NULL, NULL) != GR_OK)
	{
		PIPE->destroy_texture (tx_handle);
		tx_handle = HTX_INVALID;
		goto done;
	}

	// We are done. Return true.
	result = true;

done:
	if (hBmp != NULL)
	{
		DeleteObject ((HGDIOBJ) hBmp);
		hBmp = NULL;
	}
	return result;
}

void BMPTexture::destroy ()
{
	if (tx_handle)
	{
		if (PIPE == NULL)
		{
			GENERAL_WARNING ("PIPE is invalid in BMPTexture::destroy().");
		}
		else
		{
			PIPE->destroy_texture (tx_handle);
		}
		tx_handle = HTX_INVALID;
	}
}

