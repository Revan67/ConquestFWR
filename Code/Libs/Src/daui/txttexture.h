#ifndef TXTTEXTURE_H
#define TXTTEXTURE_H
//
// TXTTexture.h - A simple class for managing textures created from text strings
//

//
// Include files
//

#include <rendpipeline.h>

#include <iimagesource.h>
#include "fontimage.h"

//
// Class and structure definitions
//

class TXTTexture
{
protected:
	U32 tx_handle;

protected:
	bool create (IFontImage *stringImage);

public:
	TXTTexture () { tx_handle = HTX_INVALID; }
	~TXTTexture () { destroy (); }

	bool create (IFontFactory *font, const wchar_t *string);
	bool create (IFontFactory *font, HINSTANCE hInst, U32 wID);
	void destroy ();

	U32 get_handle () { return tx_handle; }
};

#endif