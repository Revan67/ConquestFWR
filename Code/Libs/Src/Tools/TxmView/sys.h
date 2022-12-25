//---------------------------------------------------------------------------
/*
	SYSTEM.H

	(Win32) Lancer (c) 1997 Digital Anvil

	02-05-97 created (pci)
	$Header: /Tools/TxmView/sys.h 2     7/14/99 12:46p Pisaac $
*/
//---------------------------------------------------------------------------

#ifndef SYS_H
#define SYS_H
//---------------------------------------------------------------------------

#include "window.h"

#include "LightMan.h"

extern struct ICOManager		*DACOM;
extern struct IRenderPipeline	*PIPE;
extern struct IEngine			*ENGINE;
extern struct IPhysics			*PHYSICS;
extern struct IModel			*MODEL;
extern struct IRenderer			*RENDER;
extern struct ITXMLib			*TXMLIB;
extern struct ILightManager		*LIGHT;
extern struct ICollision		*COLLISION;


//---------------------------------------------------------------------------
// System
//---------------------------------------------------------------------------

struct System
{
	HINSTANCE			hInstance;

	struct ISystemContainer	*SYS;

// Flags

	bool				fullscreen;

// Methods

	System (void)
	{
		hInstance = 0;

		SYS = 0;

		fullscreen = FALSE;
	}

	bool System::is_ready (void);
	// has Video mode been activated?

	void set_fullscreen (bool state);

	bool setup_display (void);

	bool startup (HINSTANCE hInstance);
	void shutdown (void);

	bool open_view (Window *win);
	void close_view (Window *win);

	bool update (int &result);
};

extern System TheSystem;

#define MSG(text) TheSystem.message(text)
#define DEBUG_MSG OutputDebugString

// Mouse Information
extern int LastMX;
extern int LastMY;
extern int LastBTN;

//---------------------------------------------------------------------------

#endif // SYS_H

