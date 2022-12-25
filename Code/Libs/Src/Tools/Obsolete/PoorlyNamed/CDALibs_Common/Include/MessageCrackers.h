// MessageCrackers.h
//


#ifndef MESSAGECRACKERS_H
#define MESSAGECRACKERS_H

#define STRICT
#include <windows.h>

// warning C4065: switch statement contains 'default' but no 'case' labels
#pragma warning( disable : 4065 )

// Message map stuff
//
// ON_MESSAGE( WM_MESSAGE, Function ) 
//	HRESULT Function( UINT message, WPARAM wParam, LPARAM lParam );
//
// ON_COMMAND( IDC_CONTROL, Function ) 
//	HRESULT Function( UINT wID, HWND hControl, UINT NotifyCode );
//

#define MSG_HANDLED		S_OK
#define MSG_NOT_HANDLED	E_FAIL

//
// Static WindowProc Message Maps
//
#define BEGIN_STATIC_WP_MAPS(Type) \
static LRESULT CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)  \
{ \
	U32 return_value = 0; \
	const char *TheType = #Type;\
	Type *a = (Type*)GetWindowLong( hWnd, GWL_USERDATA ); \
	if(a) \
		switch( message ) { 

#define END_STATIC_WP_MAPS \
		} \
	return DefWindowProc( hWnd,message,wParam,lParam );\
}



//
// Virtual WindowProc Message Maps
//
#define BEGIN_VIRTUAL_WP_MAPS(Type) \
virtual LRESULT CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)  \
{ \
	U32 return_value = 0; \
	const char *TheType = #Type;\
	Type *a = (Type*)this;\
	if(a) \
		switch( message ) { 

#define END_VIRTUAL_WP_MAPS \
		} \
	return E_FAIL;\
}



//
// General WindowProc Message Maps
//
#define BEGIN_WP_MAPS(Type) \
LRESULT CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)  \
{ \
	U32 return_value = 0; \
 	const char *TheType = #Type;\
	long lReturn = E_FAIL;\
	Type *a = (Type*)GetWindowLong( hWnd, GWL_USERDATA ); \
	DPF( "%s: HandleMessage: Enter\n", TheType );\
	if(a) \
		switch( message ) { 

#define END_WP_MAPS \
		} \
	return lReturn;\
}



//
// DialogProc Message Maps
//
#define BEGIN_STATIC_DP_MAPS(Type) \
static BOOL CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)  \
{ \
	U32 return_value = TRUE; \
	const char *TheType = #Type;\
	Type *a = (Type*)GetWindowLong( hWnd, DWL_USER ); \
	switch( message ) { \
		case WM_INITDIALOG:\
			SetWindowLong( hWnd, DWL_USER, lParam );\
			a = (Type*)lParam; \
			a->OnInitDialog( hWnd, message, wParam, lParam ); \
		break;

#define END_STATIC_DP_MAPS \
	}\
	return FALSE;\
}



// 
// Define Message Map
//

#define BEGIN_MESSAGE_MAP	// Begin Message Map

#define ON_MESSAGE(M,F)	case M:  \
			if( a->F( message, wParam, lParam ) == MSG_HANDLED ) {\
				return return_value; \
			}\
			break;

#define ON_MESSAGE_DEFAULT(F) default:  \
			if( a->F( message, wParam, lParam ) == MSG_HANDLED ) {\
				return return_value; \
			}\
			break;

#define END_MESSAGE_MAP			// End Message Map




//
// Define Command Map
//
#define BEGIN_COMMAND_MAP \
		case WM_COMMAND: \
			switch( LOWORD(wParam) ) {

#define ON_COMMAND(C,F)	case C:  \
				if( a->F( LOWORD(wParam), (HWND)lParam, HIWORD(wParam) ) == MSG_HANDLED ) {\
				return return_value; \
				}\
				break;


#define ON_COMMAND_DEFAULT(F)	default:  \
				if( a->F( LOWORD(wParam), (HWND)lParam, HIWORD(wParam) ) == MSG_HANDLED ) {\
				return return_value; \
				}\
				break;

#define END_COMMAND_MAP	\
			} \
			break;

//

#define DEF_ON_MESSAGE( Function ) \
	HRESULT Function( UINT message, WPARAM wParam, LPARAM lParam );

#define DEF_ON_COMMAND( Function ) \
	HRESULT Function( UINT wID, HWND hControl, UINT NotifyCode );


#endif


