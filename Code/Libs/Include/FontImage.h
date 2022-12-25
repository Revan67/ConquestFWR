#ifndef FONTIMAGE_H
#define FONTIMAGE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               FontImage.h                                //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/Libs/Include/FontImage.h 5     4/28/00 11:57p Rmarr $
*/
//--------------------------------------------------------------------------//
 
#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

typedef unsigned short wchar_t;

//--------------------------------------------------------------------------//
//
// FONTFACTORYDESC members:
//		hWnd:   Handle to application main window
//		hFont:  Handle to windows font object. New Factory instance takes ownership
//			of the handle, and destroys the font object when its reference count reaches 0.

//--------------------------------------------------------------------------//
//

// NOTE: It is assumed here that the logical pixels per inch vertically for the display HDC as
// returned by GetDeviceCaps(GetDC(NULL), LOGPIXELSY) is 96.
const int FONT_LOGPIXELSY = 96;

struct FONTFACTORYDESC : DACOMDESC
{
	LOGFONT logFont;

	// This one simply clears out the descriptor. You have to completely fill it out.
	FONTFACTORYDESC () : DACOMDESC("IFontFactory")
	{
		memset(&logFont, 0, sizeof(logFont));
		size = sizeof(*this);
	}

	// This one just copies the given LOGFONT.
	FONTFACTORYDESC (LOGFONT _logFont) : DACOMDESC("IFontFactory")
	{
		logFont = _logFont;
		size = sizeof(*this);
	}

	// This one allows you to specify the point size and font name.
	// NOTE: If the given face it not available, you will get a font deemed by
	// Windows to be the closest match, which may be way off.
	FONTFACTORYDESC (const char *faceName, int pointSize) : DACOMDESC("IFontFactory")
	{
		memset(&logFont, 0, sizeof(logFont));
		size = sizeof(*this);
		// Fill out with default values
		logFont.lfHeight = -MulDiv(pointSize, FONT_LOGPIXELSY, 72);
		logFont.lfWidth = 0; // use aspect ratio of the font
		logFont.lfEscapement = 0;  // angle between base line and the x axis
		logFont.lfOrientation = 0; // under Win95, set to the same as lfEscapement
		logFont.lfWeight = FW_DONTCARE;
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
		logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		logFont.lfQuality = DEFAULT_QUALITY;
		logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		strncpy(logFont.lfFaceName, faceName, LF_FACESIZE-1);
		logFont.lfFaceName[LF_FACESIZE-1] = '\0';
		// NOTE: You can change any of these three after construction to TRUE to enable the
		// desired feature.
		logFont.lfItalic = FALSE;
		logFont.lfUnderline = FALSE;
		logFont.lfStrikeOut = FALSE;
	}
};
//--------------------------------------------------------------------------//
//
// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IFONTFACTORY_VERSION 1
#define IID_IFontFactory MAKE_IID("IFontFactory",1)

struct IFontFactory : IComponentFactory
{
	/*
		descriptor:  The address of a FONTIMAGEDESC structure. (See FONTIMAGEDESC, below.)
		instance:    Address of pointer to the newly created FontImage instance.
	*/
	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance) = 0;

	/*
		RETURNS: Height of the font, in pixels.
	*/
	virtual U32 __stdcall GetFontHeight (void) const = 0;

	/*
		RETURNS: Length of the font, in pixels. Assumes single line of text.
	*/
	virtual U32 __stdcall GetStringWidth (const wchar_t *string) = 0;

	/*
		RETURNS: Length of the font, in pixels. Assumes single line of text.
	*/
	virtual U32 __stdcall GetStringWidth (HINSTANCE hInstance, U32 dwID) = 0;

	/*
		RETURNS: Width of the character, in pixels.
	*/
	virtual int __stdcall GetCharWidth (wchar_t c) = 0;
};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
// FONTIMAGEDESC members:
//		dwFlags:
//			FDDFL_MULTILINE:	Text should be make to fit within bounding rect at *pBoundingRect.
//								"pBoundingRect" must be valid if this flag is specified. If this flag
//								is not set, text will be drawn on a single line.
//
//		pBoundingRect:			(Optional) Address of a bounding rectangle. If valid, text is
//								clipped to this area. "top" and "left" members of the rect should 
//								always be 0. "right" and "bottom" describe the width and height, respectively,
//								of the area.
//		hInstance:				Handle to the Module which holds the string resource, "dwResourceID".
//								If this member is non-zero, the "dwResource" is assumed to be valid. Otherwise,
//								"szString" is used.
//		szString:				Address of a null-terminated UNICODE string.
//		dwResourceID:			Identifier of a resource string to be used.

//--------------------------------------------------------------------------//
// values for "dwFlags" member of FONTIMAGEDESC
//--------------------------------------------------------------------------//
#define FDDFL_MULTILINE	0x00000001
#define FDDFL_CALCHEIGHT 0x00000002

//--------------------------------------------------------------------------//
//
struct FONTIMAGEDESC : DACOMDESC
{
	U32			dwFlags;
	LPRECT		pBoundingRect;
	HINSTANCE	hInstance;
	union
	{
		const wchar_t *szString;
		U32			dwResourceID;
	};
	
	FONTIMAGEDESC (void) : DACOMDESC("IFontImage")
	{
		memset(((C8*)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));
		size = sizeof(*this);
	}
};

//--------------------------------------------------------------------------//
//

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IFONTIMAGE_VERSION 1
#define IID_IFontImage MAKE_IID("IFontImage",1)

struct IFontImage : IDAComponent
{
	/*
		foreground: value to use for the "pen" color.
		background: value to use for pixels not drawn by the pen.

		NOTE: The default value for foreground is 0xFFFFFFFF(white opaque), background = 0(black transparent).
	*/
	virtual void __stdcall SetFontColor (U32 foregroundARGB, U32 backgroundARGB) = 0;
};


#endif