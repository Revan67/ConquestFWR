#ifndef UIDATA_H
#define UIDATA_H
//
// UIData.h - Header for the classes which define the DA User Interface data structure.
//

//
// Design Notes:
//      The output of this editor is a collection of screens, which are themselves collections of references
// to named rectangles in a collection of art files.  This tool allows these to be placed interactively, and
// allows a game team to plug in their own exporter for these data files.
//      This file describes the interface used by exporters to gain access to the edited data. It is intended for
// use by the exporters only.  The editor itself uses the actual implementation to get both read and write
// access to the data.
//

//
// Include files
//

#include <windows.h>
#include "uiexport.h"
#include <afxtempl.h>

//
// Constants
//

const int MAX_FILENAME_SIZE = 1024;  // max size for all filenames
const int MAX_NAME_SIZE     = 128;   // max size for all symbolic names

//
// Classes and structures
//

class ArtFile : public ArtFileInterface
{
public:
	char name[MAX_FILENAME_SIZE];
	int w, h;
	HBITMAP bitmap;
	HDC     memDc;

public:
	// Constructors/Destructors
	ArtFile ()
	{
		name[0] = '\0';
		w = h = 0;
		bitmap = NULL;
		memDc = NULL;
	}

	ArtFile (char *filename)
	{
		strncpy (name, filename, MAX_FILENAME_SIZE);
		name[MAX_FILENAME_SIZE-1] = '\0';
		load ();
	}

	~ArtFile ()
	{
		free ();
	}

	// Visual API
	void drawFrom (HDC hdc, long destX, long destY, UIRect srcRect)
	{
		HGDIOBJ oldBitmap = SelectObject (memDc, bitmap);
		BitBlt (hdc, destX, destY, srcRect.w, srcRect.h, memDc, srcRect.x, srcRect.y, SRCCOPY);
		SelectObject (memDc, oldBitmap);
	}

	// Loading API
	bool load ();
	bool load (char *filename)
	{
		if (name[0] != '\0')
		{
			return false;
		}

		strncpy (name, filename, MAX_FILENAME_SIZE);
		name[MAX_FILENAME_SIZE-1] = '\0';
		return load ();
	}
	bool free ();

	// ArtFileInterface 
	virtual const char *GetName() { return name; }
	virtual long GetW() { return w; }
	virtual long GetH() { return h; }
};

class ArtRect : public ArtRectInterface
{
public:
	UIHandle  hArtFile; // handle to the ArtFile instance 
	UIRect    rect;
	char      name[MAX_NAME_SIZE];

public:
	// Visual API
	void drawAt (HDC hdc, class UIData *data, long x, long y);

	// ArtRectInterface 
	virtual UIHandle GetArtFile()   { return hArtFile; }
	virtual const UIRect *GetRect() { return &rect; }
	virtual const char *GetName() { return name; }
};

class Control : public ControlInterface
{
public:
	UIHandle  hArtRect; // handle to the ArtRect for this control
	int       x, y;     // the position of the upper left corner of the control in the screen
	char      name[MAX_NAME_SIZE];

public:
	// Visual API
	void draw (HDC hdc, class UIData *data);

	// ControlInterface 
	virtual UIHandle GetArtRect() { return hArtRect; }
	virtual long GetX() { return x; }
	virtual long GetY() { return y; }
	virtual const char *GetName() { return name; }
};

class Screen : public ScreenInterface 
{
public:
	CList<Control *, Control *> controls;            // the list of controls in this screen
	UIHandle                    hBgArt;              // the background art for this screen.
	char                        name[MAX_NAME_SIZE]; // the name of this screen.

public:
	// Visual API
	void draw (HDC hdc, class UIData *data);

	// Control Manipulation API
	Control *addControl();
	bool removeControl (Control *which);
	Control *findControl (char *name);
	Control *findControl (long x, long y, UIData *data);

	// ScreenInterface
	virtual long GetControlCount () { return controls.GetCount(); }
	virtual const ControlInterface *GetControl (long which)
	{
		if (which >= 0 && which < controls.GetCount())
		{
			return controls.GetAt(controls.FindIndex(which));
		}
		else
		{
			return NULL;
		}
	}
	virtual UIHandle GetBgArt () { return hBgArt; }
	virtual const char *GetName () { return name; }
};

//
// This class is the container of the entire database. It can individually access ArtFiles, ArtRects,
// and Screens.
//

class UIData : public UIDataInterface
{
public:
	CMap<UIHandle, UIHandle, ArtFile *, ArtFile *> arts;
	CMap<UIHandle, UIHandle, ArtRect *, ArtRect *> rects;
	CMap<UIHandle, UIHandle, Screen *,  Screen *>  screens;

	UIHandle nextHandle;

protected:
	UIHandle incHandle()
	{
		UIHandle result = nextHandle++;
		if (nextHandle == UIHANDLE_INVALID) ++nextHandle;
		return result;
	}

public:
	// Constructors/destructors
	UIData();
	~UIData();
	void clear ();

	// Load/save API
	bool load (CArchive& ar);
	bool save (CArchive& ar);

	// Addition and Retrieval API
	UIHandle addScreen (Screen *ptr, UIHandle handle = UIHANDLE_INVALID)
	{
		// By default, use the next available handle
		if (handle == UIHANDLE_INVALID)
		{
			handle = incHandle();
		}

		// NOTE: This is not an array access!
		screens[handle] = ptr;
		return handle;
	}

	Screen *getScreen (UIHandle which)
	{
		Screen *ptr;
		if (screens.Lookup (which, ptr))
		{
			return ptr;
		}
		else
		{
			return (Screen *) NULL;
		}
	}

	UIHandle addArtFile (ArtFile *ptr, UIHandle handle = UIHANDLE_INVALID)
	{
		// By default, use the next available handle
		if (handle == UIHANDLE_INVALID)
		{
			handle = incHandle();
		}

		// NOTE: This is not an array access!
		arts[handle] = ptr;
		return handle;
	}

	ArtFile *getArtFile (UIHandle which)
	{
		ArtFile *ptr;
		if (arts.Lookup (which, ptr))
		{
			return ptr;
		}
		else
		{
			return (ArtFile *) NULL;
		}
	}

	ArtFile *getArtByIndex (int index, UIHandle &handle)
	{
		POSITION here = arts.GetStartPosition();
		if (here)
		{
			UIHandle h;
			ArtFile *afp;
			arts.GetNextAssoc (here, h, afp);

			while (here && index)
			{
				arts.GetNextAssoc (here, h, afp);
				--index;
			}

			if (index == 0)
			{
				handle = h;
				return afp;
			}
		}
		return (ArtFile *) NULL;
	}

	UIHandle addArtRect (ArtRect *ptr, UIHandle handle = UIHANDLE_INVALID)
	{
		// By default, use the next available handle
		if (handle == UIHANDLE_INVALID)
		{
			handle = incHandle();
		}

		// NOTE: This is not an array access!
		rects[handle] = ptr;
		return handle;
	}

	ArtRect *getArtRect (UIHandle which)
	{
		ArtRect *ptr;
		if (rects.Lookup (which, ptr))
		{
			return ptr;
		}
		else
		{
			return (ArtRect *) NULL;
		}
	}

	// UIDataInterface
	virtual const ScreenInterface *GetScreen (UIHandle which)   { return getScreen(which); }
	virtual const ArtFileInterface *GetArtFile (UIHandle which) { return getArtFile(which); }
	virtual const ArtRectInterface *GetArtRect (UIHandle which) { return getArtRect(which); }
};

#endif
