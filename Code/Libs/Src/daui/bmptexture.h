#ifndef BMPTEXTURE_H
#define BMPTEXTURE_H
//
// BMPTexture.h - A simple class for managing textures loaded from .BMP files
//

//
// Include files
//

#include <rendpipeline.h>

//
// Class and structure definitions
//

class BMPTexture
{
protected:
	U32 tx_handle;

public:
	BMPTexture () { tx_handle = HTX_INVALID; }
	~BMPTexture () { destroy (); }

	bool create (const char *filename);
	void destroy ();

	U32 get_handle () { return tx_handle; }
};

#endif