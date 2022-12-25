//--------------------------------------------------------------------------//
//                                                                          //
//                            DrawAgent16.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DrawAgent16.cpp 39    10/06/00 7:00p Rmarr $

	
   Implementation of FontDrawAgent that can draw system fonts
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "DrawAgent.h"
#include "IImageReader.h"
#include "CQTrace.h"
#include "Resource.h"
#include "Startup.h"
#include "GenData.h"
#include <DFonts.h>

#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <FileSys.h>

#include <malloc.h>
#include <stdlib.h>

#define BITMAP_HEIGHT 64
#define BITMAP_WIDTH  1024
#define NUM_FONT_COLORS 2

// search.asm -- find first non-matching byte
void * __fastcall unmemchr (const void * ptr, int c, int size);
void __stdcall CreateDrawAgentForFonts (struct IImageReader * reader, struct IDrawAgent ** _drawAgent, RECT * pRect=0);
void __stdcall CreateMultilineFontDrawAgent (PGENTYPE pArchetype, HFONT hFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent);
//--------------------------------------------------------------------------//
//--------------------------FontDrawAgent class-----------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE FontDrawAgent16 : IFontDrawAgent, IImageReader
{
	BEGIN_DACOM_MAP_INBOUND(FontDrawAgent16)
  	DACOM_INTERFACE_ENTRY(IFontDrawAgent)
  	END_DACOM_MAP()

	//-------------------------------
	// static data
	//-------------------------------

	static HDC hdc;
	static HBITMAP hBitmap;
	static U8 *bitmapBits;
	static S32 users;

	//-------------------------------
	// instance data
	//-------------------------------

	PGENTYPE pArchetype;
	HFONT hFont;

	COMPTR<IDrawAgent> agent;

	COLORREF palette[NUM_FONT_COLORS];
	wchar_t *lastString;
	U32 lastID;
	U16 stringWidth;
	U16 fontHeight;
	U16 leadingWidth;		// blank space at left
	bool bOwnFont:1;
 

	//-------------------------------
	//-------------------------------

	FontDrawAgent16 (void);

	virtual ~FontDrawAgent16 (void);

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
		lastID = 0;
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
		return false;
	}

	DEFMETHOD_(int,GetNumRows) (const wchar_t * string, PANE * pane)
	{
		return 1;
	}

	/* FontDrawAgent methods  */

	GENRESULT getImageRGBA (COLORREF *buffer, const RECT * rect) const;
 
	U32 getStringWidth (const wchar_t * string) const;

	void stringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string);

	void drawToBitmap (const wchar_t * string) const;

	void init (HFONT hFont, BOOL32 bOwnFont, COLORREF pen,COLORREF background);//const COLORREF palette[2]);

	void updateStringData (const wchar_t * string, BOOL32 bCreateAgent);
	
	static void createGDIObjects (void);

	static void destroyGDIObjects (void);

//	static void getBitmapExtent (U32 & width, U32 & height, U32 fontHeight);

	static U8 getBitmapValue (int x, int y);

	IDAComponent * getBase (void)
	{
		return static_cast<IFontDrawAgent *> (this);
	}
};
//--------------------------------------------------------------------------//
//
HDC FontDrawAgent16::hdc;
HBITMAP FontDrawAgent16::hBitmap;
U8 *FontDrawAgent16::bitmapBits;
S32 FontDrawAgent16::users;
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::createGDIObjects (void)
{
	hdc = CreateCompatibleDC(hMainDC);
	hBitmap = CreateCompatibleBitmap(hdc, BITMAP_WIDTH, BITMAP_HEIGHT);
	bitmapBits = (U8 *) calloc(((BITMAP_WIDTH+7)/8)*BITMAP_HEIGHT, 1);
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::destroyGDIObjects (void)
{
	free(bitmapBits);
	bitmapBits = 0;
	DeleteObject(hBitmap);
	hBitmap = 0;
	DeleteDC(hdc);
	hdc = 0;
}
//--------------------------------------------------------------------------//
//
inline U8 FontDrawAgent16::getBitmapValue (int x, int y)
{
	typedef U8 _bits_type[BITMAP_HEIGHT][((BITMAP_WIDTH+7)/8)];
	_bits_type & bits = *((_bits_type *) bitmapBits);
	U8 result;

	if ((U32)x < BITMAP_WIDTH && (U32)y < BITMAP_HEIGHT)
	{
		result = bits[y][x/8];
		result >>= 7 - (x & 7);
		result &= 1;
	}
	else
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
FontDrawAgent16::FontDrawAgent16 (void)
{
	if (users++ == 0)
		createGDIObjects();

}
//--------------------------------------------------------------------------//
//
FontDrawAgent16::~FontDrawAgent16 (void)
{
	if (--users == 0)
		destroyGDIObjects();
	CQASSERT(users >= 0);

	if (bOwnFont)
		DeleteObject(hFont);

	if (pArchetype)
		GENDATA->Release(pArchetype);		// decrement the usage count

	if (lastString)
		::free(lastString);
}
//--------------------------------------------------------------------------//
//
U32 FontDrawAgent16::GetFontHeight (void) const
{
	return fontHeight;
}
//--------------------------------------------------------------------------//
//
U32 FontDrawAgent16::GetStringWidth (const wchar_t *string)
{
	return getStringWidth(string);
}
//--------------------------------------------------------------------------//
//
U32 FontDrawAgent16::GetStringWidth (U32 dwID)
{
	if (lastID == dwID)
		return stringWidth;

	return getStringWidth(_localLoadStringW(dwID));
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::StringDraw (PANE *pane, S32 x, S32 y, U32 dwID)
{
	const wchar_t * tmp;
	
	tmp = (lastID != dwID) ? _localLoadStringW(dwID) : lastString;
	lastID = dwID;

	stringDraw(pane, x, y, tmp);
}
//--------------------------------------------------------------------------//
//
GENRESULT FontDrawAgent16::CreateDuplicate (IFontDrawAgent ** _agent)
{
	GENRESULT result = GR_OK;
	COMPTR<DAComponent<FontDrawAgent16> >agent;
	CreateFontDrawAgent(hFont, 0, palette[0], palette[1], _agent);
	
	if ((agent = (DAComponent<FontDrawAgent16> *) (*_agent)) != 0)
	{
		if ((agent->pArchetype = pArchetype) != 0)
			GENDATA->AddRef(pArchetype);
	}
	else
		result = GR_GENERIC;
	return result;
}
//--------------------------------------------------------------------------//
//
int FontDrawAgent16::GetCharWidth (wchar_t c)
{
	HGDIOBJ hOldFont = SelectObject(hMainDC, hFont);

	int charWidth;
	::GetCharWidthW(hMainDC,c,c,&charWidth);

	SelectObject(hMainDC, hOldFont);

	return charWidth;
}
//--------------------------------------------------------------------------//
//
GENRESULT FontDrawAgent16::LoadImage (void *fileImage, U32 fileSize, U32 imageNumber)
{
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT FontDrawAgent16::GetImage (U32 desiredFormat, void * _buffer, const RECT * rect) const
{
	typedef U8 _bits_type[BITMAP_HEIGHT][((BITMAP_WIDTH+7)/8)];
	_bits_type & bits = *((_bits_type *) bitmapBits);
	U8 * buffer = (U8 *) _buffer;

	if (desiredFormat == PF_RGBA)
		return getImageRGBA((COLORREF *)_buffer, rect);

	CQASSERT(desiredFormat == PF_COLOR_INDEX);
		
	if (rect == 0)
	{
		U32 i, j;

		for (i = 0; i < fontHeight; i++)
			for (j = 0; j < stringWidth; j+=8)
			{
				U8 field = bits[i][j/8];
				int count=8;

				if (j + count > stringWidth)
					count = stringWidth - j;
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
GENRESULT FontDrawAgent16::getImageRGBA (COLORREF *buffer, const RECT * rect) const
{
	typedef U8 _bits_type[BITMAP_HEIGHT][((BITMAP_WIDTH+7)/8)];
	_bits_type & bits = *((_bits_type *) bitmapBits);

	if (rect == 0)
	{
		U32 i, j;

		for (i = 0; i < fontHeight; i++)
			for (j = 0; j < stringWidth; j+=8)
			{
				U8 field = bits[i][j/8];
				int count=8;

				if (j + count > stringWidth)
					count = stringWidth - j;
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
GENRESULT FontDrawAgent16::GetColorTable (U32 desiredFormat, void * buffer) const
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
U32 FontDrawAgent16::GetWidth (void) const
{
	return stringWidth;
}
//--------------------------------------------------------------------------//
//
U32 FontDrawAgent16::GetHeight (void) const
{
	return fontHeight;
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::SetFontColor (COLORREF fg,COLORREF bg)
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
U32 FontDrawAgent16::getStringWidth (const wchar_t * string) const
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

	// add one for leading, 2 for following
	if (palette[1] & 0x80000000)	// check the alpha level of background
		result += 3;				// if solid background

	return result;
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::stringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string)
{
	updateStringData(string, 1);
	
	BATCH->set_state(RPR_BATCH,FALSE);
	agent->Draw(pane, x, y);
}
//--------------------------------------------------------------------------//
//
struct MYBITMAPINFO : BITMAPINFO
{
	RGBQUAD		extra[1];
};

void FontDrawAgent16::drawToBitmap (const wchar_t * string) const
{
	RECT rect;
	MYBITMAPINFO bitmapInfo;
	HGDIOBJ hOldBitmap;
	HGDIOBJ hOldFont = SelectObject(hdc, hFont);
	int height = __min(fontHeight, BITMAP_HEIGHT);

	hOldBitmap = SelectObject(hdc, hBitmap);
	rect.top = rect.left = 0;
	rect.right = __min(stringWidth, BITMAP_WIDTH);
	rect.bottom = height;

	ExtTextOutW(hdc, leadingWidth, 0, ETO_OPAQUE, &rect, string, wcslen(string), 0);

	memset(&bitmapInfo, 0, sizeof(bitmapInfo));
	memset(bitmapInfo.extra, 0xCF, sizeof(bitmapInfo.extra));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	//bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo);
	bitmapInfo.bmiHeader.biWidth = BITMAP_WIDTH;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 1;
	bitmapInfo.bmiHeader.biSizeImage = ((BITMAP_WIDTH+7)/8)*BITMAP_HEIGHT;
	SelectObject(hdc, hOldBitmap);
	GetDIBits(hdc, hBitmap, 0, height, bitmapBits, &bitmapInfo, DIB_RGB_COLORS);
	SelectObject(hdc, hOldFont);
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::updateStringData (const wchar_t * string, BOOL32 bCreateAgent)
{
	if (lastString==0 || wcscmp(string, lastString) != 0 || (bCreateAgent && agent==0))
	{
		//wcsncpy(lastString, string, sizeof(lastString)/sizeof(wchar_t) - 1);
		if (lastString)
			::free(lastString);
		lastString = _wcsdup(string);
		if ((stringWidth = getStringWidth(string)) == 0)
			stringWidth++;
		drawToBitmap(string);

		agent.free();
		if (bCreateAgent)
			CreateDrawAgentForFonts((FontDrawAgent16 *)this, agent);
	}
}
//--------------------------------------------------------------------------//
//
void FontDrawAgent16::init (HFONT _hFont, BOOL32 _bOwnFont, COLORREF pen, COLORREF background)//const COLORREF _palette[2])
{
	hFont = _hFont;
	bOwnFont = (_bOwnFont==TRUE);

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
		CQASSERT(fontHeight < BITMAP_HEIGHT);
	}

	if (palette[1] & 0x80000000)	// check the alpha level of background
	{
		// if background is solid, not transparent
		fontHeight += 1;	// leave one blank row at bottom
		leadingWidth = 1;
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void __stdcall CreateFontDrawAgent (HFONT hFont, BOOL32 bOwnFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent)
{
	FontDrawAgent16 * fontDrawAgent = new DAComponent<FontDrawAgent16>;

	CQASSERT(hFont);

	fontDrawAgent->init(hFont, bOwnFont, pen,background);

	*_fontDrawAgent	= fontDrawAgent;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// need multi-res code in here before we can begin
struct NumberFont : DAComponent<FontDrawAgent16>
{
	//-------------------------------
	// instance data
	//-------------------------------

	COMPTR<IDrawAgent> prefix;
	COMPTR<IDrawAgent> digit[10];
	COMPTR<IDrawAgent> comma;

	U16 prefixWidth;
	U16 digitWidth[10];
	U16 commaWidth;
	U16 spaceWidth;
	//-------------------------------
	//-------------------------------

	NumberFont (void) { }

	virtual ~NumberFont (void) { }

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IFontDrawAgent methods */

//	DEFMETHOD_(U32,GetFontHeight) (void) const;

	DEFMETHOD_(U32,GetStringWidth) (const wchar_t *string);

	DEFMETHOD_(U32,GetStringWidth) (U32 dwID);

	DEFMETHOD_(void,StringDraw) (PANE *pane, S32 x, S32 y, const wchar_t *string);
	
	DEFMETHOD_(void,StringDraw) (PANE *pane, S32 x, S32 y, U32 dwID);

	DEFMETHOD(CreateDuplicate) (IFontDrawAgent ** agent);

//	DEFMETHOD_(int,GetCharWidth) (wchar_t c);

	// NumberFont methods

	void init (HFONT _hFont, BOOL32 bOwnFont, COLORREF pen, COLORREF background);
};
//--------------------------------------------------------------------------//
//
void NumberFont::init (HFONT _hFont, BOOL32 _bOwnFont, COLORREF pen, COLORREF background)
{
	int i;
	wchar_t num[2];

	FontDrawAgent16::init(_hFont, _bOwnFont, pen, background);

	updateStringData(_localLoadStringW(IDS_DOLLAR_SIGN), 1);
	prefix = agent;
	prefixWidth = stringWidth;
	updateStringData(_localLoadStringW(IDS_MONEY_SEPARATOR), 1);
	comma = agent;
	commaWidth = stringWidth;

	spaceWidth = GetCharWidth(' ');

	num[1] = 0;

	for (i = 9; i >= 0; i--)
	{
		num[0] = '0'+ i;
		updateStringData(num, 1);
		digit[i] = agent;
		digitWidth[i] = stringWidth;
	}
}
//----------------------------------------------------------------------------------------//
//
U32 NumberFont::GetStringWidth (const wchar_t *string)
{
	CQBOMB0("GetStringWidth() not supported.");
	return 0;
}
//----------------------------------------------------------------------------------------//
//
U32 NumberFont::GetStringWidth (U32 dwID)
{
	CQBOMB0("GetStringWidth() not supported.");
	return 0;
}
//----------------------------------------------------------------------------------------//
//
void NumberFont::StringDraw (PANE *pane, S32 x, S32 y, U32 dwID)
{
	CQBOMB0("NumberFont::StringDraw(dwID) not supported.");
}
//----------------------------------------------------------------------------------------//
//
void NumberFont::StringDraw (PANE *pane, S32 x, S32 y, const wchar_t *string)
{
	if (iswdigit(string[0]) == 0)
		FontDrawAgent16::StringDraw(pane, x, y, L"Error!");
	else
	{
		prefix->Draw(pane, x, y);
		x += prefixWidth+spaceWidth;

		while (*string)
		{
			if (*string != 0x20)
			{
				if (*string == ',')
				{
					comma->Draw(pane, x, y);
					x += commaWidth;		// inc by width of ','
				}
				else
				{
					int i;

					i = *string - '0';
					CQASSERT(i>=0 && i < 10 && "Illegal use of money font");		// must be a digit character
					digit[i]->Draw(pane, x, y);
					x += digitWidth[i];		// inc by width of digit
				}
			}
			string++;
		}
	}
}
//------------------------------------------------------------------------------------------//
//
GENRESULT NumberFont::CreateDuplicate (IFontDrawAgent ** _agent)
{
	*_agent = this;
	AddRef();
	return GR_OK;
}
//------------------------------------------------------------------------------------------//
//
void __stdcall CreateNumericFontDrawAgent (HFONT hFont, BOOL32 bOwnFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent)
{
	NumberFont * font = new NumberFont;

	CQASSERT(hFont);

	font->init(hFont, bOwnFont, pen,background);

	*_fontDrawAgent	= font;
}
//------------------------------------------------------------------------------------------
//---------------------------FontFactory class----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FontFactory : public ICQFactory
{
	struct OBJTYPE
	{
		HFONT hFont;
		PGENTYPE pArchetype;
		bool bMultiline:1;
		bool bToolbarMoney:1;

		InterfaceRes resolutionMode;//mode the this font was made for

		OBJTYPE (void)
		{
			hFont = 0;
		}

		~OBJTYPE (void)
		{
			if (hFont)
				DeleteObject(hFont);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(FontFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	FontFactory (void) { }

	~FontFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ICQFactory methods */

	virtual HANDLE CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *data);

	virtual BOOL32 DestroyArchetype (HANDLE hArchetype);

	virtual GENRESULT CreateInstance (HANDLE hArchetype, IDAComponent **pInstance);

	/* FontFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
FontFactory::~FontFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void FontFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE FontFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_FONT)
	{
		GT_FONT * data = (GT_FONT *) _data;
		OBJTYPE * result = new OBJTYPE;

		result->pArchetype = pArchetype;
		result->hFont = CQCreateFont(data->font, data->bNotScaling);
		result->bMultiline = data->bMultiline;
		result->bToolbarMoney = data->bToolbarMoney;
		result->resolutionMode = GetCurrentInterfaceRes();
		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 FontFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * type = (OBJTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT FontFactory::CreateInstance (HANDLE hArchetype, IDAComponent **pInstance)
{
	OBJTYPE * type = (OBJTYPE *) hArchetype;
	CQASSERT(type->hFont);

//	if (type->resolutionMode != GetCurrentInterfaceRes())
//		CQBOMB1("Attempt to share font '%s' between Resolution Modes.", GENDATA->GetArchName(type->pArchetype));

	if (type->bMultiline)
	{
		IFontDrawAgent *result;
		CreateMultilineFontDrawAgent(type->pArchetype, type->hFont, RGB(255,255,255)|0xFF000000, 0, &result);
	
		*pInstance = result;
	}
	else
	if (type->bToolbarMoney)
	{
		NumberFont * font = new NumberFont;

		font->init(type->hFont, 0, RGB(0,255,255)|0xFF000000, 0);
		font->pArchetype = type->pArchetype;
		*pInstance	= font->getBase();
	}
	else
	{
		FontDrawAgent16 * result = new DAComponent<FontDrawAgent16>;

		result->init(type->hFont, 0, RGB(255,255,255)|0xFF000000, 0);
		result->pArchetype = type->pArchetype;

		*pInstance = result->getBase();
	}

	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _fontfactory : GlobalComponent
{
	FontFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<FontFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _fontfactory startup;
//-----------------------------------------------------------------------------------------//
//-----------------------------------End DrawAgent16.cpp-----------------------------------//
//-----------------------------------------------------------------------------------------//
