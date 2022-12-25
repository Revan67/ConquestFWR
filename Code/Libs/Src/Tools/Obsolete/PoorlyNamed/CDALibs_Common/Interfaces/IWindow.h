// IWindow
//
//
//

#ifndef IWindow_H
#define IWindow_H

#include "DACOM.h"
#include "DACOM_Utility.h"


#define IWINDOW_MANAGED		(1<<0)
#define IWINDOW_EMBEDDED	(1<<1)
#define IWINDOW_MODAL		(1<<2)

struct IWINDOWCREATEDATA
{
	HINSTANCE					hModuleInstance;
	HINSTANCE					hResourceInstance;
	HWND						hParent;
	RECT						rChildRect;
	U32							uWindowStyle;
	
	IWINDOWCREATEDATA()
	{
		hModuleInstance = GetModuleHandle(NULL);
		hResourceInstance = GetModuleHandle(NULL);
		hParent = NULL;
		SetRect( &rChildRect, 0, 0, 0, 0 );
		uWindowStyle = 0;
	}
};

//

// defined message classes
//
#define IWINDOW_MC_WIN32	1
#define IWINDOW_MC_USER		0x8000

struct WINDOWSMESSAGEDATA
{
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;

	WINDOWSMESSAGEDATA( UINT m, WPARAM w=0, LPARAM l=0 )
	{
		uMsg = m;
		wParam = w;
		lParam = l;
	}
};

// HandleMessage should return one of these
//
#define IWINDOW_MESSAGE_HANDLED		S_OK
#define IWINDOW_MESSAGE_NOT_HANDLED E_FAIL

//

static const char *IID_IWindow = "IWindow";
dacom_interface( IWindow )
{
	DACOM_INTERFACE_METHOD( Create,			( IWINDOWCREATEDATA *create_data, HWND *out_hChild ));
	DACOM_INTERFACE_METHOD( Destroy,		( void ));
//	DACOM_INTERFACE_METHOD( HandleMessage,	( U32 message, void *message_data ));
	DACOM_INTERFACE_METHOD(	GetHandle,		( HWND *out_hChild ));
//	DACOM_INTERFACE_METHOD(	, ());
};



static const char *IID_IModalWindow = "IModalWindow";
dacom_interface( IModalWindow )
{
	DACOM_INTERFACE_METHOD( DoModal,	( HWND hParent, U32 *out_return ));
};


#endif
