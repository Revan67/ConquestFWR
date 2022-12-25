//
// TXTTexture.cpp - Text string based texture support
//

//
// Include files
//

#include "stdafx.h"
#include <fdump.h>
#include <tempstr.h>
#include <tsmartpointer.h>

#include "txttexture.h"

//
// External variables
//

extern IRenderPipeline *PIPE;

//
// Methods
//

bool TXTTexture::create (IFontFactory *font, HINSTANCE hInst, U32 wID)
{
	ASSERT (tx_handle == HTX_INVALID);
	ASSERT (hInst != NULL);

	// Create the font image.

	FONTIMAGEDESC desc;
	desc.hInstance = hInst;
	desc.dwResourceID = wID;

	COMPTR<IFontImage> stringImage;
	if (font->CreateInstance (&desc, stringImage) != GR_OK)
	{
		GENERAL_ERROR ("Failed to create stringImage");
		return false;
	}

	return create (stringImage);
}

bool TXTTexture::create (IFontFactory *font, const wchar_t *string)
{
	ASSERT (tx_handle == HTX_INVALID);
	ASSERT (font != NULL);
	ASSERT (string != NULL);

	// Create the font image.

	FONTIMAGEDESC desc;
	desc.szString = string;

	COMPTR<IFontImage> stringImage;
	if (font->CreateInstance (&desc, stringImage) != GR_OK)
	{
		GENERAL_ERROR ("Failed to create stringImage");
		return false;
	}

	return create (stringImage);
}

bool TXTTexture::create (IFontImage *stringImage)
{
	ASSERT (stringImage != NULL);

	// Query the IImageSource interface.
	COMPTR<IImageSource> stringSource;
	if (stringImage->QueryInterface (IID_IImageSource, stringSource) != GR_OK)
	{
		GENERAL_ERROR("Failed to query IImageSource");
		return false;
	}

	// Create a new texture.

	bool result = false;
	PixelFormat pf (16, 5, 5, 5, 1);
	U32 w, h;
	int mipLevels = 0;

	stringSource->GetDimensions (w, h);

	// Make sure the texture is a power of two in size.
	unsigned int txW, txH;

	txW = 256;
	txH = 256;
	ASSERT(w <= txW && h <= txH);

	int stride = txW * sizeof(short);

	// ACK! We need a local buffer into which we can place the pixels.
	// *** There has to be a better way!
	void *buffer = new short[txW * txH];
	memset (buffer, 0, txW*txH*sizeof(short));
	stringImage->SetFontColor (0x80000000, 0x7FFFFFFF);
	RECT r;
	r.top = r.left = 0;
	r.right = w;
	r.bottom = h;

	stringSource->GetImage (pf, buffer, stride, &r);

	ASSERT (PIPE != NULL);
	
	if (PIPE->create_texture (txW, txH, pf, mipLevels, tx_handle) != GR_OK)
	{
		GENERAL_ERROR (TEMPSTR("Failed to allocate %d by %d, %d bpp texture.", w, h, 16));
		goto done;
	}

	// Write the pixels into the new texture.
	if (PIPE->set_texture_level_data (tx_handle, mipLevels, pf, txW, txH, stride, pf, buffer, NULL, NULL) != GR_OK)
	{
		PIPE->destroy_texture (tx_handle);
		tx_handle = HTX_INVALID;
		goto done;
	}

	// We are done. Return true.
	result = true;

done:
	if (buffer != NULL)
	{
		delete buffer;
	}

	return result;
}

void TXTTexture::destroy ()
{
	if (tx_handle)
	{
		if (PIPE == NULL)
		{
			GENERAL_WARNING ("PIPE is invalid in TXTTexture::destroy().");
		}
		else
		{
			PIPE->destroy_texture (tx_handle);
		}
		tx_handle = HTX_INVALID;
	}
}

