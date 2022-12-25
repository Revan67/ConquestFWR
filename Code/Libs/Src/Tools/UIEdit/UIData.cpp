//
// UIData.cpp - The implementation of the database and the exporter interfaces
//

//
// Design Notes:
//

//
// Include files
//

#include "stdafx.h"
#include "uidata.h"

//
// Methods
//

// ArtFile methods

bool ArtFile::load ()
{
	// NOTE: Attempts to load as a bitmap the file whose name is stored in name[].
	
	// Free any old bitmap first.
	free ();

	// Attempt to load the image.
	if (name[0] != '\0')
	{
		bitmap = (HBITMAP) LoadImage ( NULL, name, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
		if (bitmap)
		{
			// Create a DC compatible with the current display environment.
			memDc = CreateCompatibleDC (NULL);
			if (memDc)
			{
				// Get the width and height of the bitmap
				BITMAP bm;
				if (GetObject (bitmap, sizeof(BITMAP), &bm))
				{
					w = bm.bmWidth;
					h = bm.bmHeight;
					return true;
				}

				// Failure above, so clean up
				DeleteDC (memDc);
				memDc = NULL;
			}

			// Failure above, so clean up
			DeleteObject (bitmap);
			bitmap = NULL;
		}
	}

	// We failed.
	return false;
}

bool ArtFile::free ()
{
	if (memDc)
	{
		DeleteDC(memDc);
		memDc = NULL;
	}

	if (bitmap)
	{
		DeleteObject (bitmap);
		bitmap = NULL;
	}

	return true;
}

// ArtRect methods

void ArtRect::drawAt (HDC hdc, class UIData *data, long x, long y)
{
	ArtFile *af = data->getArtFile(hArtFile);
	if (af != NULL)
	{
		af->drawFrom (hdc, x, y, rect);
	}
}

// Control methods

void Control::draw (HDC hdc, class UIData *data)
{
	ArtRect *ar = data->getArtRect(hArtRect);
	if (ar != NULL)
	{
		ar->drawAt (hdc, data, x, y);
	}
}

// Screen methods

void Screen::draw (HDC hdc, class UIData *data)
{
	// Draw the background.
	ArtFile *bga = data->getArtFile (hBgArt);
	if (bga != NULL)
	{
		UIRect r;
		r.x = r.y = 0;
		r.w = bga->w;
		r.h = bga->h;
		bga->drawFrom(hdc, 0, 0, r);
	}

	// Draw all of the controls.
	POSITION here = controls.GetHeadPosition();

	while(here != NULL)
	{
		controls.GetAt(here)->draw(hdc, data);
		controls.GetNext(here);
	}
}

Control *Screen::addControl()
{
	// Allocate a new control, add it to the list, and return its pointer

	Control *c = new Control;
	c->hArtRect = UIHANDLE_INVALID;
	c->x = c->y = 0;
	c->name[0] = '\0';

	controls.AddTail (c);
	return c;
}

bool Screen::removeControl (Control *which)
{
	// Find the control in the list, returning false if it is not
	// in the list.

	POSITION found = controls.Find(which);
	if (!found)
	{
		return false;
	}

	// Remove the item from the list, then delete the control
	controls.RemoveAt(found);
	delete which;
	return true;
}

Control *Screen::findControl (char *name)
{
	// Search through the list for a control with the given name.
	// If none is found return NULL, otherwise return the control pointer

	int count = controls.GetCount();
	int i;
	for (i = 0; i < count; ++i)
	{
		Control *c = controls.GetAt(controls.FindIndex(i));
		if (!c)
		{
			// Something bad happened. Exit.
			break;
		}

		if (!stricmp(name, c->name))
		{
			return c;
		}
	}

	return NULL;
}

Control *Screen::findControl (long x, long y, UIData *data)
{
	// Find the control which contains the given coordinates, starting the
	// search from the end of the list, since the controls are drawn in order from
	// the start of the list.

	int i = controls.GetCount();
	while (i--)
	{
		Control *c = controls.GetAt(controls.FindIndex(i));
		if (!c)
		{
			// Something bad happened. Exit.
			break;
		}

		ArtRect *arp = data->getArtRect(c->hArtRect);
		if (arp)
		{
			CPoint p(x,y);
			CRect r(c->x, c->y, c->x + arp->rect.w, c->y + arp->rect.h);
			if (r.PtInRect(p))
			{
				return c;
			}
		}
	}

	return NULL;
}

// UIData methods

UIData::UIData()
{
	nextHandle = UIHANDLE_INVALID + 1;
}

UIData::~UIData()
{
	// What do I do here? Perhaps delete all of the objects stored in its containers. Yes, I will do that.
	clear ();
}

void UIData::clear ()
{
	// *** TODO: Delete all contained objects here.
}

// Load/save API
const char HeaderMagic[] = "UI Layout File\x1A";
const unsigned long FILE_VERSION = 1;

// NEVER EVER CHANGE THE SIZE OR LAYOUT OF THIS STRUCTURE!
struct UIDataFileHeader
{
	unsigned char magic[sizeof(HeaderMagic)];
	unsigned long version;
};

struct UIDataHeader
{
	// Information about the data
	unsigned long artCount;
	unsigned long rectCount;
	unsigned long screenCount;

	// Information about the editing environment
	unsigned long currentArt;
	unsigned long editMode;
	unsigned long nextHandle;
};

struct UIArtFileData
{
	unsigned long handle;
	unsigned long nameLength;
	// char name[nameLength];
};

struct UIArtRectData
{
	unsigned long handle;
	unsigned long artHandle;
	UIRect        rect;
	unsigned long nameLength;
	// char name[nameLength];
};

struct UIControlData
{
	unsigned long rectHandle;
	int           x, y;
	unsigned short nameLength;
	// char name[nameLength];
};

struct UIScreenData
{
	unsigned long controlCount;
	unsigned long bgArtHandle;
	unsigned long nameLength;
	// char name[nameLength]
	// UIControlData controls[controlCount]
};

bool UIData::load (CArchive& ar)
{
	// Clear out all of the data before loading
	clear ();

	// This is the format of the data file:
	// Header structure: defines the version number and the count of art files, rectangles, and screens
	// List of art files
	// List of rectangles
	// List of screens.
	// NOTE: Since a screen has a variable number of buttons in it, care must be taken when reading in the
	// screen data.

	try 
	{
		// Read in the file header and validate the file
		UIDataFileHeader head;
		ar.Read(&head, sizeof(head));

		if (strncmp(HeaderMagic, (char *) &head.magic, sizeof(HeaderMagic)))
		{
			// Not a user interface layout file.
			return false;
		}

		// Check the version.
		// For now, versions must match absolutely.
		if (FILE_VERSION != head.version)
		{
			return false;
		}

		// Read in the data header
		UIDataHeader fData;
		ar.Read(&fData, sizeof(fData));

		// Read in the art files.
		unsigned int i;
		for (i = 0; i < fData.artCount; ++i)
		{
			UIArtFileData afd;
			char artName[MAX_FILENAME_SIZE];

			ar.Read(&afd, sizeof(afd));
			ar.Read(artName, afd.nameLength);

			// *** What do we do about paths?

			ArtFile *afp = new ArtFile;
			afp->load (artName);
			addArtFile (afp, afd.handle);
		}

		// Read in the rect data
		for (i = 0; i < fData.rectCount; ++i)
		{
			UIArtRectData ard;
			char rectName[MAX_NAME_SIZE];

			ar.Read(&ard, sizeof(ard));
			ar.Read(rectName, ard.nameLength);

			ArtRect *arp = new ArtRect;
			arp->hArtFile = ard.artHandle;
			arp->rect = ard.rect;
			strncpy (arp->name, rectName, ard.nameLength);
			addArtRect (arp, ard.handle);
		}

		// Read in the screens

		for (i = 0; i < fData.screenCount; ++i)
		{
			UIScreenData sd;
			char screenName[MAX_NAME_SIZE];

			ar.Read (&sd, sizeof(sd));
			ar.Read (screenName, sd.nameLength);

			Screen *s = new Screen;
			s->hBgArt = sd.bgArtHandle;
			strncpy(s->name, screenName, sd.nameLength);

			unsigned int j;
			for (j = 0; j < sd.controlCount; ++j)
			{
				UIControlData cd;
				char controlName[MAX_NAME_SIZE];

				ar.Read (&cd, sizeof(cd));
				ar.Read (controlName, cd.nameLength);

				Control *c = s->addControl();
				c->hArtRect = cd.rectHandle;
				c->x = cd.x;
				c->y = cd.y;
				strncpy (c->name, controlName, cd.nameLength);
			}
		}

		// Set the editing environment data, including the next handle
		// *** TODO: Read this data from the document loading routine instead of here.
		nextHandle = fData.nextHandle;
	}
	catch (CFileException ex)
	{
		ex.ReportError();
		return false;
	}
	return true;
}

bool UIData::save (CArchive& ar)
{
	// See the load() method for a description of the file format.
	// *** TODO: Write this code.
	return false;
}
