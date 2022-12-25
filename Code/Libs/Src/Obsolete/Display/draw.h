#include <ddraw.h>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

struct DrawMgr
{
	bool DDraw_active;
	HINSTANCE DDraw_lib_handle;

//	IDirectDraw2 *lpDD;
	IDirectDraw *lpDD;

	bool fullscreen;

	HWND hWnd;

	DrawMgr (void)
	{
		DDraw_active = false;
		DDraw_lib_handle = 0;

		lpDD = 0;

		hWnd = 0;

		fullscreen = false;
	}

	~DrawMgr (void)
	{
		shutdown();
	}

	bool startup (void);
	void shutdown (void);

	bool set_display_mode (HWND hWnd, int w, int h, int bpp);
	bool restore_display_mode (HWND hWnd);
};

//---------------------------------------------------------------------------
