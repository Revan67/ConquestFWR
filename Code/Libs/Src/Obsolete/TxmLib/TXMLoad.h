#ifndef __TXMLOAD_H
#define __TXMLOAD_H

#include "FileSys.h"
#include "fdump.h"

#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int rxrxrx = (x);	\
		ASSERT (rxrxrx);	\
	}
#else								
	 #define CHECK(x)	(void)(x);
#endif

bool Read4ByteVal (IFileSystem* fs, const char* name, long& out);

//---------------------------------------------------------------------------

typedef int OFFSET;
const OFFSET INVALID_OFFSET = -1;

//EMAURER after each call to AllocMem (), all ptrs obtained using MemPtr () are
//possibly invalid and should not be used.

unsigned char* MemPtr (OFFSET offset);

void HeapReset (void);

//EMAURER after each call to AllocMem (), all ptrs obtained using MemPtr () are
//possibly invalid and should not be used.

OFFSET AllocMem (int req);

//---------------------------------------------------------------------------

#pragma pack (push, 1)

struct RGB_888
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
};

#pragma pack (pop) 

#define RGB_565_R(rgb)	U8((rgb & 0xF800) >> 8)
#define RGB_565_G(rgb)	U8((rgb & 0x07E0) >> 3)
#define RGB_565_B(rgb)	U8((rgb & 0x001F) << 3)

//---------------------------------------------------------------------------

bool AlphaIsRange (const unsigned char* abits, int len);

//---------------------------------------------------------------------------

//EMAURER new format stuff

const unsigned int HINT_MAX_FIDELITY = 1;
const unsigned int HINT_NO_PALETTES = 2;
const unsigned int HINT_NO_MIPMAPS = 4;

//EMAURER return the number of mip maps loaded
unsigned int Load8BitSimple (IFileSystem* fs, 
								bool& has_alpha, 
								unsigned int x_size, 
								unsigned int y_size, 
								unsigned int hints = 0);

unsigned int Load16BitSimple (IFileSystem* fs, 
								bool& has_alpha, 
								unsigned int x_size, 
								unsigned int y_size,
								unsigned int hints = 0);

#endif
