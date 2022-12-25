//--------------------------------------------------------------------------//
//                                                                          //
//                            MultiLineFont.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/MultiLineFont.cpp 11    10/08/01 11:28a Tmauer $

   Implementation of FontDrawAgent that can draw system fonts, using multiple rows of text.
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "DrawAgent.h"
#include "IImageReader.h"
#include "CQTrace.h"
#include "Resource.h"
#include "GenData.h"

#include <TComponent.h>
#include <HeapObj.h>
#include <TSmartPointer.h>

#include <malloc.h>
#include <stdlib.h>

#define NUM_FONT_COLORS 2
#define MAX_MULTI_CHARS 1024

// search.asm -- find first non-matching byte
void * __fastcall unmemchr (const void * ptr, int c, int size);
void __stdcall CreateDrawAgentForFonts (struct IImageReader * reader, struct IDrawAgent ** _drawAgent, RECT * pRect=0);
//--------------------------------------------------------------------------//
//--------------------------------MLFont class------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE MLFont : IFontDrawAgent, IImageReader
{
	BEGIN_DACOM_MAP_INBOUND(MLFont)
  	DACOM_INTERFACE_ENTRY(IFontDrawAgent)
  	END_DACOM_MAP()

	//-------------------------------
	// data set by createGDIObjects()
	//-------------------------------
	HDC hdc;
	HBITMAP hBitmap;
	U8 *bitmapBits;
	int width, height; 

	//-------------------------------
	// instance data
	//-------------------------------

	HFONT hFont;
	PGENTYPE pArchetype;
	COLORREF palette[NUM_FONT_COLORS];
	U32 fontHeight;
	wchar_t lastString[MAX_MULTI_CHARS];
	U8 lastCharWidths[MAX_MULTI_CHARS];
	PANE lastPane;
 
	COMPTR<IDrawAgent> agent;
	//-------------------------------
	//-------------------------------

	MLFont (void);

	virtual ~MLFont (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	/* IFontDrawAgent methods  */
	
	DEFMETHOD_(U32,GetFontHeight) (void) const;

	DEFMETHOD_(U32,GetStringWidth) (const wchar_t *string);

	DEFMETHOD_(U32,GetStringWidth) (U32 dwID);

	DEFMETHOD_(void,StringDraw) (PANE *pane, S32 x, S32 y, const wchar_t *string)
	{
		stringDraw(pane, x, y, string);
	}
	
	DEFMETHOD_(void,StringDraw) (PANE *pane, S32 x, S32 y, U32 dwID);

	DEFMETHOD(CreateDuplicate) (IFontDrawAgent ** agent);

	DEFMETHOD_(int,GetCharWidth) (wchar_t c);

	/* IImageReader methods */

	DEFMETHOD(LoadImage) (void *fileImage, U32 fileSize, U32 imageNumber=0);

	DEFMETHOD(GetImage) (U32 desiredFormat, void * buffer, const RECT * rect = 0) const;

	DEFMETHOD(GetColorTable) (U32 desiredFormat, void * buffer) const;

	DEFMETHOD_(U32,GetWidth) (void) const;

	DEFMETHOD_(U32,GetHeight) (void) const;

	DEFMETHOD_(void,SetFontColor) (COLORREF fg,COLORREF bg);

	DEFMETHOD_(bool,IsMultiLine) (void)
	{
		return true;
	}

	DEFMETHOD_(int,GetNumRows) (const wchar_t * _string, PANE * pane)
	{
		U32 boundingWidth = pane->x1 - pane->x0;

		if (GetStringWidth(_string) <= boundingWidth)
		{
			return 1;
		}
		
		updateStringData(pane, _string);

		const wchar_t * string = lastString;
		const U8 * widths = lastCharWidths;
		int length;
		int nRows = 1;

		length = getNextLineLength(string, widths, boundingWidth);
	
		string += length;
		widths += length;

		while (string[0] == 13 || string[0] == 10)
		{
			string++;
			widths++;
		}

		while ((length = getNextLineLength(string, widths, boundingWidth)) != 0)
		{
			string += length;
			widths += length;

			while (string[0] == 13 || string[0] == 10)
			{
				string++;
				widths++;
			}
			nRows++;
		}

		return nRows;
	}

	/* FontDrawAgent methods  */

	GENRESULT getImageRGBA (COLORREF *buffer, const RECT * rect) const;
 
	void stringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string);

	void drawToBitmap (void);

	void init (PGENTYPE pArchetype, HFONT hFont, COLORREF pen,COLORREF background);

	void updateStringData (PANE * pane, const wchar_t * string);
	
	void createGDIObjects (void);

	void destroyGDIObjects (void);

	U8 getBitmapValue (int x, int y) const;

	void getCharWidths (void);

	static int getNextLineLength (const wchar_t * string, const U8 * widths, const int maxWidth);

	U32 getStringWidth (const wchar_t * string) const;

	IDAComponent * getBase (void)
	{
		return static_cast<IFontDrawAgent *> (this);
	}
};
//--------------------------------------------------------------------------//
// lastPane must contain valid data
//
void MLFont::createGDIObjects (void)
{
	if (hdc)
		destroyGDIObjects();
	hdc = CreateCompatibleDC(hMainDC);
	
	width  = (lastPane.x1 - lastPane.x0 + 1 + 31) & ~31;		// align on dword boundary
	height = lastPane.y1 - lastPane.y0 + 1;

	CQASSERT(width>0 && height>0);

	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	bitmapBits = (U8 *) calloc(width/8, height);
}
//--------------------------------------------------------------------------//
//
void MLFont::destroyGDIObjects (void)
{
	free(bitmapBits);
	bitmapBits = 0;
	if (hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = 0;
	}
	if (hdc)
	{
		DeleteDC(hdc);
		hdc = 0;
	}
}
//--------------------------------------------------------------------------//
//
inline U8 MLFont::getBitmapValue (int x, int y) const
{
	U8 result;

	if ((U32)x < (U32)width && (U32)y < (U32)height)
	{
		result = bitmapBits[((y*width)+x) / 8];
		result >>= 7 - (x & 7);
		result &= 1;
	}
	else
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
MLFont::MLFont (void)
{
}
//--------------------------------------------------------------------------//
//
MLFont::~MLFont (void)
{
	destroyGDIObjects();
//	DeleteObject(hFont);		// don't own the font
	if (pArchetype)
		GENDATA->Release(pArchetype);		// decrement the usage count
}
//--------------------------------------------------------------------------//
//
U32 MLFont::GetFontHeight (void) const
{
	return fontHeight;
}
//--------------------------------------------------------------------------//
//
U32 MLFont::GetStringWidth (const wchar_t *string)
{
	return getStringWidth(string);
}
//--------------------------------------------------------------------------//
//
U32 MLFont::GetStringWidth (U32 dwID)
{
	return getStringWidth(_localLoadStringW(dwID));
}
//--------------------------------------------------------------------------//
//
void MLFont::StringDraw (PANE *pane, S32 x, S32 y, U32 dwID)
{
	stringDraw(pane, x, y, _localLoadStringW(dwID));
}
//--------------------------------------------------------------------------//
//
GENRESULT MLFont::CreateDuplicate (IFontDrawAgent ** _agent)
{
	GENRESULT result = GR_OK;
	
	CreateMultilineFontDrawAgent(pArchetype,hFont, palette[0], palette[1], _agent);

	if (*_agent != 0)
	{
		if (pArchetype != 0)
			GENDATA->AddRef(pArchetype);
	}
	else
		result = GR_GENERIC;
	return result;
}
//--------------------------------------------------------------------------//
//
int MLFont::GetCharWidth (wchar_t c)
{
	HGDIOBJ hOldFont = SelectObject(hMainDC, hFont);

	int charWidth;
	::GetCharWidthW(hMainDC,c,c,&charWidth);

	SelectObject(hMainDC, hOldFont);

	return charWidth;
}
//--------------------------------------------------------------------------//
//
GENRESULT MLFont::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber)
{
	CQBOMB0("Not supported.");
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT MLFont::GetImage (U32 desiredFormat, void * _buffer, const RECT * rect) const
{
	U8 * buffer = (U8 *) _buffer;

	if (desiredFormat == PF_RGBA)
		return getImageRGBA((COLORREF *)_buffer, rect);

	CQASSERT(desiredFormat == PF_COLOR_INDEX);
		
	if (rect == 0)
	{
		int i, j;

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j+=8)
			{
				U8 field = bitmapBits[(i*width+j)/8];
				int count=8;

				if (j + count > width)
					count = width - j;
				while (count-- > 0)
				{
					*buffer++ = field >> 7;
					field <<= 1;
				}
			}
	}
	else	// subimage
	{
		CQASSERT(rect->top <= rect->bottom);
		CQASSERT(rect->left <= rect->right);
		CQASSERT(rect->top >= 0);
		CQASSERT(rect->left >= 0);

		S32 i, j;

		for (i = rect->top; i <= rect->bottom; i++)
			for (j = rect->left; j <= rect->right; j++)
			{
				*buffer++ = getBitmapValue(j, i);
			}
	}
	
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT MLFont::getImageRGBA (COLORREF *buffer, const RECT * rect) const
{
	if (rect == 0)
	{
		int i, j;

		for (i = 0; i < height; i++)
			for (j = 0; j < width; j+=8)
			{
				U8 field = bitmapBits[(i*width+j)/8];
				int count=8;

				if (j + count > width)
					count = width - j;
				while (count-- > 0)
				{
					*buffer++ = palette[field >> 7];
					field <<= 1;
				}
			}
	}
	else	// subimage
	{
		CQASSERT(rect->top <= rect->bottom);
		CQASSERT(rect->left <= rect->right);
		CQASSERT(rect->top >= 0);
		CQASSERT(rect->left >= 0);

		S32 i, j;

		for (i = rect->top; i <= rect->bottom; i++)
			for (j = rect->left; j <= rect->right; j++)
			{
				*buffer++ = palette[getBitmapValue(j, i)];
			}
	}
	
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT MLFont::GetColorTable (U32 desiredFormat, void * buffer) const
{
	GENRESULT result = GR_GENERIC;
	COLORREF * _palette = (COLORREF *) buffer;

	CQASSERT(desiredFormat == PF_RGBA || desiredFormat == PF_RGB);

	if (desiredFormat == PF_RGB)
	{
		const COLORREF * colors = palette;
		VFX_RGB * rgb = (VFX_RGB *) buffer;
		int i;

		memset(_palette, 0, sizeof(VFX_RGB) * 256);
		for (i = 0; i < NUM_FONT_COLORS; i++, rgb++, colors++)
			rgb[0] = *((VFX_RGB *)(&colors[0]));
	}
	else
	{
		memset(_palette, 0, sizeof(COLORREF)*256);
		memcpy(_palette, palette, sizeof(palette));
	}
	result = GR_OK;
	return result;
}
//--------------------------------------------------------------------------//
//
U32 MLFont::GetWidth (void) const
{
	return width;
}
//--------------------------------------------------------------------------//
//
U32 MLFont::GetHeight (void) const
{
	return height;
}
//--------------------------------------------------------------------------//
//
void MLFont::SetFontColor (COLORREF fg,COLORREF bg)
{
	if (palette[0] != fg || palette[1] != bg)
	{
		palette[0] = fg;
		palette[1] = bg;
		agent = 0;
	}
}
//--------------------------------------------------------------------------//
//
U32 MLFont::getStringWidth (const wchar_t * string) const
{
	const wchar_t * ptr = string;
	HGDIOBJ hOldFont = SelectObject(hMainDC, hFont);
	int charWidth;
	U32 result = 0;

	while (*ptr)
	{
		::GetCharWidthW(hMainDC,*ptr,*ptr,&charWidth);
		result += charWidth;
		ptr++;
	}

	SelectObject(hMainDC, hOldFont);

	return result;
}
//--------------------------------------------------------------------------//
//
void MLFont::stringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string)
{
	if (pane==0)
	{
		lastPane.x0 = x;
		lastPane.x1 = x + getStringWidth(string) - 1;
		lastPane.y0 = y;
		lastPane.y1 = y + fontHeight - 1;
	}

	updateStringData(pane, string);
	agent->Draw(pane, x, y);
}
//--------------------------------------------------------------------------//
//
void MLFont::getCharWidths (void)
{
	const wchar_t * ptr = lastString;
	int charWidth;
	U8 *widths = &lastCharWidths[0];

	while (*ptr)
	{
		::GetCharWidthW(hdc,*ptr,*ptr,&charWidth);
		*widths++ = charWidth;
		ptr++;
	}
}
//--------------------------------------------------------------------------//
// return number of characters to print in the next line.
// look for newlines, spaces as clues to where to break
//
int MLFont::getNextLineLength (const wchar_t * string, const U8 * widths, const int maxWidth)
{
	int width = 0;
	int lastSpace = -1;		// index of last space character found
	int index = 0;
	wchar_t letter = string[index];

	while (letter && width <= maxWidth)
	{
		if (letter == 13 || letter==10)		// newline character?
		{
			break;
		}
		else
		if (letter == ' ')
			lastSpace = index;

		width += widths[index];
		letter = string[++index];
	}

	// back up until
	if (width > maxWidth)
	{
		if (lastSpace>=0 && index - lastSpace < 20)		// did we find a break in last 15 characters?
			index = lastSpace + 1;
		else
		if (index>1)
			index--;
	}
	
	return index;
}
//--------------------------------------------------------------------------//
//
struct MYBITMAPINFO : BITMAPINFO
{
	RGBQUAD		extra[1];
};

void MLFont::drawToBitmap (void)
{
	RECT rect;
	MYBITMAPINFO bitmapInfo;
	HGDIOBJ hOldBitmap;
	HGDIOBJ hOldFont = SelectObject(hdc, hFont);
	const wchar_t * string = lastString;
	const U8 * widths = lastCharWidths;
	int length, Y, trueWidth;

	getCharWidths();

	hOldBitmap = SelectObject(hdc, hBitmap);
	rect.top = rect.left = 0;
	rect.right = width;
	rect.bottom = height;

	trueWidth = lastPane.x1 - lastPane.x0 + 1;
	length = getNextLineLength(string, widths, trueWidth);
	ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rect, string, length, 0);
	string += length;
	widths += length;
	Y = fontHeight;
	while (string[0] == 13 || string[0] == 10)
	{
		string++;
		widths++;
	}

	while ((length = getNextLineLength(string, widths, trueWidth)) != 0)
	{
		ExtTextOutW(hdc, 0, Y, 0, NULL, string, length, 0);

		Y += fontHeight;
		string += length;
		widths += length;

		while (string[0] == 13 || string[0] == 10)
		{
			string++;
			widths++;
		}
	}

	memset(&bitmapInfo, 0, sizeof(bitmapInfo));
	memset(bitmapInfo.extra, 0xCF, sizeof(bitmapInfo.extra));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	//bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 1;
	bitmapInfo.bmiHeader.biSizeImage = (width/8)*height;
	SelectObject(hdc, hOldBitmap);
	GetDIBits(hdc, hBitmap, 0, height, bitmapBits, &bitmapInfo, DIB_RGB_COLORS);
	SelectObject(hdc, hOldFont);
}
//--------------------------------------------------------------------------//
//
void MLFont::updateStringData (PANE * pane, const wchar_t * string)
{
	if (wcscmp(string, lastString) != 0 || agent==0 || (pane && memcmp(pane, &lastPane, sizeof(lastPane)) != 0))
	{
		CQASSERT(wcslen(string) < MAX_MULTI_CHARS);
		wcscpy(lastString, string);
		if (pane)
			lastPane = *pane;
		createGDIObjects();
		drawToBitmap();

		CreateDrawAgentForFonts(this, agent);
	}
}
//--------------------------------------------------------------------------//
//
void MLFont::init (PGENTYPE _pArchetype, HFONT _hFont, COLORREF pen, COLORREF background)
{
	hFont = _hFont;
	pArchetype = _pArchetype;

	palette[0] = pen;
	palette[1] = background;

	{
		HGDIOBJ hOldFont = SelectObject(hMainDC, hFont);
		TEXTMETRIC tm;

		if (::GetTextMetrics(hMainDC, &tm))
			fontHeight = tm.tmHeight;
		else
		{
			CQTRACE10("GetTextMetrics() failed!");
			fontHeight = 5;
		}

		SelectObject(hMainDC, hOldFont);
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void __stdcall CreateMultilineFontDrawAgent (PGENTYPE pArchetype, HFONT hFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent)
{
	MLFont * fontDrawAgent = new DAComponent<MLFont>;

	CQASSERT(hFont);

	fontDrawAgent->init(pArchetype, hFont, pen, background);

	*_fontDrawAgent	= fontDrawAgent;
}
//-----------------------------------------------------------------------------------------//
//---------------------------------End MultiLineFont.cpp-----------------------------------//
//-----------------------------------------------------------------------------------------//
