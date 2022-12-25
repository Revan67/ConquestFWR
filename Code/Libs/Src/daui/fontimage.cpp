//
// FontImage.cpp - Standard font bitmap creation class
//

//
// Include files
//

#include <windows.h>
#include <fdump.h>
#include <tempstr.h>
#include <tcomponent.h>
#include <iimagesource.h>

#include "fontimage.h"

//
// Local routines
//

static const wchar_t * __cdecl _localLoadStringW (HINSTANCE hInstance, U32 dwID)
{
	HRSRC hRes;
	static wchar_t buffer[256];

	buffer[0] = 0;

	//
	// find address of string resource group (16 in a group)
	//
	if ((hRes = FindResource(hInstance, MAKEINTRESOURCE((dwID/16)+1), RT_STRING)) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hInstance, hRes)) != 0)
		{
			U16 *pData;

			if ((pData = (U16 *) LockResource(hGlobal)) != 0)
			{
				//
				// find the actual string within the group
				//
				int i = dwID % 16;
				U32 numChars;

				while (i-- > 0)
				{
					numChars = *pData++;
					pData += numChars;
				}
				
				numChars = *pData++;
				ASSERT(numChars < 128);

				memcpy(buffer, pData, numChars * sizeof(wchar_t));
				buffer[numChars] = 0;		// null terminate
			}
		}
	}

	return buffer;
}

static	wchar_t * __cdecl _localDupeStringW (HINSTANCE hInstance, U32 dwID)
{
	HRSRC hRes;
	static wchar_t *buffer	=NULL;

	//
	// find address of string resource group (16 in a group)
	//
	if ((hRes = FindResource(hInstance, MAKEINTRESOURCE((dwID/16)+1), RT_STRING)) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hInstance, hRes)) != 0)
		{
			U16 *pData;

			if ((pData = (U16 *) LockResource(hGlobal)) != 0)
			{
				//
				// find the actual string within the group
				//
				int i = dwID % 16;
				U32 numChars;

				while (i-- > 0)
				{
					numChars = *pData++;
					pData += numChars;
				}
				
				numChars = *pData++;
				ASSERT(numChars < 128);

				buffer	=new wchar_t[numChars + 1];

				memcpy(buffer, pData, numChars * sizeof(wchar_t));
				buffer[numChars] = 0;		// null terminate
			}
		}
	}

	return buffer;
}

//
// The standard font image component 
//

class Win32FontImage : public IFontImage, public IImageSource
{
protected:
	BEGIN_DACOM_MAP_INBOUND(Win32FontImage)
	DACOM_INTERFACE_ENTRY(IFontImage)
	DACOM_INTERFACE_ENTRY2(IID_IFontImage, IFontImage)
	DACOM_INTERFACE_ENTRY2(IID_IImageSource, IImageSource)
	END_DACOM_MAP()

	unsigned char *bits;   // the monochrome bits for the image bitmap.
	U32            w, h;   // the dimensions of the bitmap
	U32            fg, bg; // foreground and background colors.
	int            stride; // the "stride" of the bitmap, i.e. bytes to the next line.

public:
	Win32FontImage ()
	{
		bits = NULL;
		w = h = 0;
		fg = 0xFFFFFFFF;
		bg = 0;
	}

	virtual ~Win32FontImage ()
	{
		if (bits != NULL)
		{
			delete bits;
			bits = NULL;
		}
	}

	// Local initialization method
	void set_bitmap (HBITMAP hBitmap);

	// ====== IFontImage methods ======
	virtual void __stdcall SetFontColor (U32 foreground, U32 background);

	// ====== IImageSource methods ======
	DEFMETHOD(GetImage) (PixelFormat desiredFormat, void * buffer, int bufferStride, const RECT * rect = 0);
	DEFMETHOD(GetColorTable) (U32 * numColors, U32 * packedARGB);
	DEFMETHOD(GetDimensions) (U32 & width, U32 & height) const;
	DEFMETHOD(GetPreferredFormat) (PixelFormat *preferred);
};

void Win32FontImage::set_bitmap (HBITMAP hBitmap)
{
	ASSERT(hBitmap != NULL);

#pragma warning(disable: 4127)	//conditional constant
	ASSERT(bits == NULL && "You may only set_bitmap() once on Win32FontImage");
#pragma warning(default: 4127)

	// Get the information about the bitmap.
	BITMAP bm;
	if (GetObject (hBitmap, sizeof(bm), &bm) == 0)
	{
		GENERAL_ERROR("Failed to get font image bitmap info.");
		return;
	}

#pragma warning(disable: 4127)	//conditional constant
	ASSERT(bm.bmBitsPixel == 1 && "Only monochrome bitmaps are supported");
#pragma warning(default: 4127)

	w = bm.bmWidth;
	h = bm.bmHeight;
	stride = bm.bmWidthBytes;

	// Make a copy of the bitmap bits, because we are not taking ownership of the bitmap,
	// and therefore it may go away.

	int size = h * stride;
	bits = new unsigned char[size]; // we will accept the exception here if allocation fails.
	GetBitmapBits (hBitmap, size, bits);
	//memcpy (bits, bm.bmBits, size);
}

// ====== IFontImage methods ======
void __stdcall Win32FontImage::SetFontColor (U32 foreground, U32 background)
{
	fg = foreground;
	bg = background;
}

GENRESULT COMAPI Win32FontImage::GetPreferredFormat (PixelFormat *preferred)
{
	ASSERT(preferred);
	// WARNING: It is assumed that the caller and this module agree on what the PixelFormat structure
	// is.
	// The preferred format for this is 8 bit palettized.
	preferred->init (8, 0, 0, 0, 0);
	return GR_OK;
}

GENRESULT COMAPI Win32FontImage::GetColorTable (U32 *numColors, U32 *packedARGB)
{
	// Since there are only two colors in the entire image, we will return only two colors.
	// This works equally well for 1 bit or 8 bit palttized images.
	ASSERT(numColors);
	ASSERT(packedARGB);

	*numColors = 2;
	packedARGB[0] = bg;
	packedARGB[1] = fg;
	return GR_OK;
}

GENRESULT COMAPI Win32FontImage::GetImage (PixelFormat pf, void * buffer, int bufferStride, const RECT * subImage)
{
	U32 bpp = pf.num_bits();
	ASSERT(bpp == 1 || bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32);

	// Translate pixels from our monochrome bitmap into the given buffer, using the
	// current foreground and background colors, taking the destination bit depth, stride, and sub image rectangle
	// into account. No stretching occurs, and the sub-image rectangle must be entirely inside the dimensions
	// of our bitmap.

	int destW = w;
	int destH = h;
	int startX = 0;
	int startY = 0;
	if (subImage)
	{
		destW = subImage->right - subImage->left;
		destH = subImage->bottom - subImage->top;
		startX = subImage->left;
		startY = subImage->top;
	}

	// Default buffer stride number of BYTES from to go down one line.
	// It is at least bpp * width, but may be longer due to padding.
	if (!bufferStride)
	{
		bufferStride = destW;
	}

	// Copy the bits, line by line.
	unsigned char *srcLine = bits;
	unsigned char *destLine = (unsigned char *) buffer;

	for (unsigned int i = 0; i < h; ++i)
	{
		// Translate the pixels from this line to the buffer.
		// NOTE: Each line always starts aligned in the destination buffer, but is offset in the
		// source buffer by the startX value.
		unsigned char *src = srcLine + (startX/8);
		unsigned char srcMask =(U8)(0x80 >> (startX % 8));
		switch (bpp)
		{
		case 1:
			{
				// This is a copy operation that is complicated by the fact that
				// the source and destination bitmaps may not have the same byte alignement.
				// Therefore, we do it the hard way.
				// NOTE: These are really indices into the palette.
				unsigned char *dest = destLine;
				unsigned char destMask = 0x80;

				//initialized but never used
//				unsigned char fColor =(fg&1) ? (U8)0xFF : (U8)0;
//				unsigned char bColor =(bg&1) ? (U8)0xFF : (U8)0;
				*dest = 0;
				for (unsigned int j = 0; j < w; ++j)
				{
					*dest |= (*src & srcMask) ? destMask : 0;
					destMask >>= 1;
					if (!destMask)
					{
						++dest;
						*dest = 0;
						destMask = 1;
					}
					srcMask >>= 1;
					if (!srcMask)
					{
						++src;
						srcMask = 0x80;
					}
				}
			}
			break;

		case 8:
			{
				// Palettized, so just stored 0 or 1 indices into the palette.
				unsigned char *dest = destLine;
				for (unsigned int j = 0; j < w; ++j)
				{
					*dest = (*src & srcMask) ? (U8)1: (U8)0;
					++dest;
					srcMask >>= 1;
					if (!srcMask)
					{
						++src;
						srcMask = 0x80;
					}
				}
			}
			break;

		case 16:
			{
				// NOTE: Use the pixel format to convert foreground and background colors from
				// RGBA to the correct 16 bit format.
				dword fg16 = pf.compute
					(
						(unsigned char) (fg & 0xff),
						(unsigned char) ((fg>>8) & 0xff),
						(unsigned char) ((fg>>16) & 0xff),
						(unsigned char) ((fg>>24) & 0xff)
					);
				dword bg16 = pf.compute
					(
						(unsigned char) (bg & 0xff),
						(unsigned char) ((bg>>8) & 0xff),
						(unsigned char) ((bg>>16) & 0xff),
						(unsigned char) ((bg>>24) & 0xff)
					);
				unsigned short *dest = (unsigned short *) destLine;
				for (unsigned int j = 0; j < w; ++j)
				{
					*dest = (*src & srcMask) ? (unsigned short) fg16 : (unsigned short) bg16;
					++dest;
					srcMask >>= 1;
					if (!srcMask)
					{
						++src;
						srcMask = 0x80;
					}
				}
			}
			break;

		case 24:
			{
				// NOTE: Assume that 24 bit color is 8880 RGBA.
				unsigned char *dest = destLine;
				for (unsigned int j = 0; j < w; ++j)
				{
					if (*src & srcMask)
					{
						*(unsigned short *)dest = (unsigned short) fg;
						dest[2] = ((unsigned char *)&fg)[2];
					}
					else
					{
						*(unsigned short *)dest = (unsigned short) bg;
						dest[2] = ((unsigned char *)&bg)[2];
					}
					dest += 3;
					srcMask >>= 1;
					if (!srcMask)
					{
						++src;
						srcMask = 0x80;
					}
				}
			}
			break;

		case 32:
			{
				// NOTE: Assume that 32 bit color is 8888 RGBA
				unsigned long *dest = (unsigned long *) destLine;
				for (unsigned int j = 0; j < w; ++j)
				{
					*dest = (*src & srcMask) ? fg : bg;
					++dest;
					srcMask >>= 1;
					if (!srcMask)
					{
						++src;
						srcMask = 0x80;
					}
				}
			}
			break;
		}

		srcLine += stride;
		destLine += bufferStride;
	}

	return GR_OK;
}

GENRESULT COMAPI Win32FontImage::GetDimensions (U32 & width, U32 & height) const
{
	width = w;
	height = h;
	return GR_OK;
}

//
// The standard font image factory component
//

class Win32FontFactory : public IFontFactory
{
protected:
	BEGIN_DACOM_MAP_INBOUND(Win32FontFactory)
	DACOM_INTERFACE_ENTRY2(IID_IFontFactory,IFontFactory)
	DACOM_INTERFACE_ENTRY2(IID_IComponentFactory,IComponentFactory)
	DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	LOGFONT logFont;  // the logical font description used to create hFont.
	HFONT hFont;      // the font, preallocated from Windows.
	HFONT hOldFont;   // the old font for the device context
	U32   fontHeight; // the max height for the font
	HDC   hDC;        // the device context used to create images and retreive metrics


public:
	Win32FontFactory ()
	{
		memset(&logFont, 0, sizeof(logFont));
		hDC = NULL;
		hOldFont = NULL;
		fontHeight = 0;
	}

	~Win32FontFactory ()
	{
		// Release any allocated resources
		if (hOldFont)
		{
			SelectObject (hDC, hOldFont);
		}
		DeleteDC(hDC);
		hDC = NULL;
	}

	// Methods required for DAAggregateComponent and DAComponent templates
	GENRESULT init (FONTFACTORYDESC * desc);

	// ====== IComponentFactory methods ======
	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance);

	// ====== IFontFactory methods ======
	virtual U32 __stdcall GetFontHeight (void) const;
	virtual U32 __stdcall GetStringWidth (const wchar_t *string);
	virtual U32 __stdcall GetStringWidth (HINSTANCE hInstance, U32 dwID);
	virtual int __stdcall GetCharWidth (wchar_t c);
};

GENRESULT Win32FontFactory::init (FONTFACTORYDESC * desc)
{
	// Create a memory dc for use with this font.
	hDC = CreateCompatibleDC (NULL);
	if (!hDC)
	{
		GENERAL_ERROR ("Failed to create DC for font factory.");
		return GR_GENERIC;
	}

	// Create a font handle from this description.
	logFont = desc->logFont;
	hFont = CreateFontIndirect (&logFont);
	if (hFont == NULL)
	{
		GENERAL_ERROR ("Failed to create font from LOGFONT.");
		return GR_GENERIC;
	}

	// Select the font into the device context and get some information from it.

	hOldFont = (HFONT) SelectObject (hDC, hFont);
	if (hOldFont == NULL)
	{
		GENERAL_ERROR ("Failed to select font into DC.");
		return GR_GENERIC;
	}

	TEXTMETRIC tm;
	if (!GetTextMetrics (hDC, &tm))
	{
		GENERAL_ERROR ("Failed to get font metrics.");
		return GR_GENERIC;
	}

	fontHeight = tm.tmHeight;

	return GR_OK;
}

// ====== IComponentFactory methods ======
GENRESULT COMAPI Win32FontFactory::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	HBITMAP	hBitmap, hOldBitmap;

	// Create a font image component based on our font.

	// Verify that we have a proper descriptor.
	if (descriptor->size != sizeof(FONTIMAGEDESC) || strcmp(descriptor->interface_name, "IFontImage"))
	{
		return GR_INTERFACE_UNSUPPORTED;
	}

	FONTIMAGEDESC *desc = (FONTIMAGEDESC *) descriptor;

	// If multiline, fit the text into the bounding rectangle, otherwise treat it as
	// a single line.
	if(desc->dwFlags & FDDFL_MULTILINE)
	{
		int		i, j, startpos, width, rectwidth, rectheight, szlen, curwidth;
		wchar_t *szString, csave;
		RECT	brect;

		//Multi-line
		//get the full width of the string

		rectwidth	=desc->pBoundingRect->right - desc->pBoundingRect->left;
		rectheight	=desc->pBoundingRect->bottom - desc->pBoundingRect->top;

		if(desc->hInstance)
		{
			width		=GetStringWidth(desc->hInstance, desc->dwResourceID);
			szString	=_localDupeStringW(desc->hInstance, desc->dwResourceID);
		}
		else
		{
			width		=GetStringWidth (desc->szString);
			szString	=wcsdup(desc->szString);
		}

		szlen	=wcslen(szString);

		if(width <= 0 || szlen <= 0)
		{
			return	GR_GENERIC;
		}

		//first pass to calculate height if needed
		//walk forward untill a newline or past rect border
		if(desc->dwFlags & FDDFL_CALCHEIGHT)
		{
			for(j = startpos = 0;;j++)
			{
				for(i = startpos;i < szlen;i++)
				{
					//look for newline
					if(szString[i] == '\n')
					{
						i++;
						break;
					}

					//terminate at counter
					csave		=szString[i];
					szString[i]	=0;
					curwidth	=GetStringWidth(szString + startpos);
					szString[i]	=csave;

					if(curwidth > rectwidth)
					{
						i--;	//back up one
						break;
					}
				}
				if(i >= szlen)
				{
					break;	//finished
				}
				else
				{
					startpos	=i;
				}
			}
			brect.top		=0;
			brect.left		=0;
			brect.bottom	=desc->pBoundingRect->top + (j + 1) * fontHeight;
			brect.right		=desc->pBoundingRect->left + rectwidth;
		}
		else
		{
			brect.top		=0;
			brect.left		=0;
			brect.bottom	=desc->pBoundingRect->top + rectheight;
			brect.right		=desc->pBoundingRect->left + rectwidth;
		}

		hBitmap	=CreateCompatibleBitmap(hDC, brect.right, brect.bottom);

		if(!hBitmap)
		{
			GENERAL_ERROR ("Failed to create bitmap.");
			return GR_GENERIC;
		}

		hOldBitmap = (HBITMAP) SelectObject (hDC, hBitmap);
		if (!hOldBitmap)
		{
			GENERAL_ERROR ("Failed to select new bitmap.");
			return GR_GENERIC;
		}

		// Set the pen color to white (1) and the background color to black (0).
		SetTextColor (hDC, 0x00FFFFFF);
		SetBkColor (hDC, 0x00000000);

		//fill with the background color to cover up text gaps
		j	=FillRect(hDC, &brect, (HBRUSH)GetStockObject(BLACK_BRUSH));

		//walk forward untill a newline or past rect border
		for(j = startpos = 0;;j++)
		{
			if(!(desc->dwFlags & FDDFL_CALCHEIGHT))
			{
				//ensure nothing is written past the end
				if((j + 1) * (int)fontHeight > rectheight)
				{
					break;
				}
			}
			for(i = startpos;i < szlen;i++)
			{
				//terminate at counter
				csave		=szString[i];
				szString[i]	=0;				
				curwidth	=GetStringWidth(szString + startpos);
				szString[i]	=csave;

				if(szString[i] == '\n')
				{
					i++;	//skip past the newline on next line

					if(!ExtTextOutW(hDC, desc->pBoundingRect->left, desc->pBoundingRect->top + j * fontHeight, ETO_OPAQUE, NULL, szString + startpos, i - 1 - startpos, 0))
					{
						GENERAL_ERROR ("ExtTextOut() failed\n");
						SelectObject (hDC, hOldBitmap);
						DeleteObject (hBitmap);
						return GR_GENERIC;
					}
					else
					{
						break;
					}
				}

				if(curwidth > rectwidth)
				{
					//make sure a single character can fit
					if(!i)
					{
						break;
					}

					//back up one for the next line
					i--;

					if(!ExtTextOutW(hDC, desc->pBoundingRect->left, desc->pBoundingRect->top + j * fontHeight, ETO_OPAQUE, NULL, szString + startpos, i  - startpos, 0))
					{
						GENERAL_ERROR ("ExtTextOut() failed\n");
						SelectObject (hDC, hOldBitmap);
						DeleteObject (hBitmap);
						return GR_GENERIC;
					}
					else
					{
						break;
					}
				}
			}
			if(i >= szlen)
			{
				if(!ExtTextOutW(hDC, desc->pBoundingRect->left, desc->pBoundingRect->top + j * fontHeight, ETO_OPAQUE, NULL, szString + startpos, szlen - startpos, 0))
				{
					GENERAL_ERROR ("ExtTextOut() failed\n");
					SelectObject (hDC, hOldBitmap);
					DeleteObject (hBitmap);
					return GR_GENERIC;
				}
				else
				{
					break;
				}
			}
			else
			{
				startpos	=i;
			}
		}
		free(szString);
	}
	else
	{
		// Single line

		// If hInstance is valid, load the string from a resource, otherwise just use the
		// string pointer.

		int width;
		const wchar_t *szString;
		if (desc->hInstance)
		{
			width = GetStringWidth (desc->hInstance, desc->dwResourceID);
			szString = _localLoadStringW (desc->hInstance, desc->dwResourceID);
		}
		else
		{
			width = GetStringWidth (desc->szString);
			szString = desc->szString;
		}

		if(width <= 0)
		{
			return	GR_GENERIC;
		}
		// Create a bitmap of the proper size, then select it into our HDC.
		// NOTE: Although the DC was created using CreateCompatibleDC, its bitmap is
		// a 1x1 pixel monochrome bitmap. For this reason, the CreateCompatibleBitmap()
		// call below will create a monochrome bitmap.

		hBitmap = CreateCompatibleBitmap(hDC, width, fontHeight);
		if (hBitmap == NULL)
		{
			GENERAL_ERROR ("Failed to create bitmap.");
			return GR_GENERIC;
		}

		hOldBitmap = (HBITMAP) SelectObject (hDC, hBitmap);
		if (!hOldBitmap)
		{
			GENERAL_ERROR ("Failed to select new bitmap.");
			return GR_GENERIC;
		}

		// Set the pen color to white (1) and the background color to black (0).
		SetTextColor (hDC, 0x00FFFFFF);
		SetBkColor (hDC, 0x00000000);

		// Draw the text into the new bitmap

		RECT rect;

		rect.top = rect.left = 0;
		rect.right = width;
		rect.bottom = fontHeight;

		if (!ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &rect, szString, wcslen(szString), 0))
		{
			GENERAL_ERROR ("ExtTextOut() failed\n");
			SelectObject (hDC, hOldBitmap);
			DeleteObject (hBitmap);
			return GR_GENERIC;
		}
	}

	// Select the old bitmap back into the DC, since we cannot get the bits for a
	// selected bitmap.

	if (!SelectObject (hDC, hOldBitmap))
	{
		GENERAL_ERROR ("Failed to select old bitmap\n");
		return GR_GENERIC;
	}

	// Now that the bitmap has been drawn, create a new font image DA component and 
	// initialize it with the bitmap.
	// NOTE: The initialization interface is private to this implementation and is
	// not exposed outside of this module.

	Win32FontImage *fontImage = new DAComponent<Win32FontImage>;
	ASSERT(fontImage);
	fontImage->set_bitmap (hBitmap);
	*instance = fontImage;

	// Clean up and return

	if (!DeleteObject (hBitmap))
	{
		GENERAL_TRACE_1 ("Failed to delete bitmap.\n");
		// NOTE: The instance is already created by this time, so go ahead and return
		// GR_OK.
	}
	return GR_OK;
}

// ====== IFontFactory methods ======
U32 __stdcall Win32FontFactory::GetFontHeight (void) const
{
	return fontHeight;
}

U32 __stdcall Win32FontFactory::GetStringWidth (const wchar_t *string)
{
	// Use the Win32 functions to find the width of the given string, as one line.
	const wchar_t * ptr = string;
	U32 result = 0;

	while (*ptr)
	{
		result += GetCharWidth (*ptr);
		ptr++;
	}
	return result;
}

U32 __stdcall Win32FontFactory::GetStringWidth (HINSTANCE hInstance, U32 dwID)
{
	// Load the string resource with the given ID from the given instance, then find its
	// width using the other version of GetStringWidth.

	return GetStringWidth(_localLoadStringW(hInstance, dwID));
}

int __stdcall Win32FontFactory::GetCharWidth (wchar_t c)
{
	// Find the width of just the specified character.
	int charWidth = 0;
	if (!::GetCharWidthW(hDC,c,c,&charWidth))
	{
		GENERAL_ERROR (TEMPSTR("GetCharWidth() failed for character 0x%x. Skipping", c));
	}
	return charWidth;
}


//===================================
// DLLMain entry point.
//===================================
#include <HeapObj.h>

void font_startup()
{
	ICOManager *DACOM = DACOM_Acquire();

	IComponentFactory *server;
	server = new DAComponentFactory<DAComponent<Win32FontFactory>,FONTFACTORYDESC>("IFontFactory");
	DACOM->RegisterComponent(server, "IFontFactory", DACOM_LOW_PRIORITY);
	server->Release();
}

void SetDllHeapMsg (HINSTANCE hInstance)
{
   DWORD dwLen;
   char buffer[260];
   
   dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
   while (dwLen > 0)
   {
      if (buffer[dwLen] == '\\')
      {
         dwLen++;
         break;
      }
      dwLen--;
   }

   SetDefaultHeapMsg(buffer+dwLen);
}

extern	void	bigimage_startup(void);

//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL	COMAPI	DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	lpvReserved;

	switch(fdwReason)
	{		
		case DLL_PROCESS_ATTACH:	//Create object server component and register
			HEAP	=HEAP_Acquire();//it with DACOM manager
			SetDllHeapMsg(hinstDLL);
			font_startup();
			bigimage_startup();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return	TRUE;
}
