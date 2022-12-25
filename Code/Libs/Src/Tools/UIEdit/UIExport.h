#ifndef UIEXPORT_H
#define UIEXPORT_H
//
// UIExport.h - Header for the interfaces used by the DA User Interface Editor
//

//
// Design Notes:
//      The output of this editor is a collection of screens, which are themselves collections of references
// to named rectangles in a collection of art files.  This tool allows these to be placed interactively, and
// allows a game team to plug in their own exporter for these data files.
//      This file describes the interface used by exporters to gain access to the edited data. It is intended for
// use by the exporters only.  The editor itself uses the actual implementation to get both read and write
// access to the data.
//      An exporter need only include this file, then implement the single function UIExport().  The export
// function is responsible for all user interface elements, such as a file selection dialog.
//

//
// Include files
//

//
// Simple type definitions
//

typedef unsigned long UIHandle;  // handle to the various UI entities.

//
// Classes and structures
//

struct UIRect
{
	long x, y;  // upper left hand corner coordinates
	long w, h;  // width and height of the rectangle
};

//
// Interfaces
//

// These interfaces are for the various objects in the user interface
// file. They are interfaces so that their implementation may be completely
// transparent to the exporter.

// WARNING: This is dependant on the compiler's implementation of classes and virtual
// method tables.

class ArtFileInterface
{
public:
	virtual const char *GetName() = 0;
	virtual long GetW() = 0;
	virtual long GetH() = 0;
};

class ArtRectInterface
{
public:
	virtual UIHandle GetArtFile() = 0;
	virtual const UIRect *GetRect() = 0;
	virtual const char *GetName() = 0;
};

class ControlInterface
{
public:
	virtual UIHandle GetArtRect() = 0;
	virtual long GetX() = 0;
	virtual long GetY() = 0;
	virtual const char *GetName() = 0;
};

class ScreenInterface
{
public:
	virtual long GetControlCount () = 0;
	virtual const ControlInterface *GetControl (long which) = 0;
	virtual UIHandle GetBgArt () = 0;
	virtual const char *GetName () = 0;
};

class UIDataInterface
{
public:
	virtual const ScreenInterface *GetScreen (UIHandle which) = 0;
	virtual const ArtFileInterface *GetArtFile (UIHandle which) = 0;
	virtual const ArtRectInterface *GetArtRect (UIHandle which) = 0;
};

//
// Function prototypes
//

// This function must be implemented by an export DLL.
// It must have the given prototype and by called "UIExport".
typedef void UIExportFunc (UIDataInterface *data);
typedef UIExportFunc *UIExportFuncPtr;

//
// Constants
//

const char UIExportFuncName[] = "UIExport";
const UIHandle UIHANDLE_INVALID = 0L;

#endif
