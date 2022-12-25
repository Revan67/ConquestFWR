//---------------------------------------------------------------------------
// CLIENT
//---------------------------------------------------------------------------
#include "..\common.h"

HINSTANCE hinst=NULL;

modeless_box * clientdialog=NULL;	// dialog handler
struct connect_data * connectdata;			// data client sends to server on connect
NetLink link;						// communication channel

//---------------------------------------------------------------------------
// add a message to the chat list part of the dialog
inline void add_message( const char * string )
{
	if (clientdialog)
		ListBox_InsertString( GetDlgItem( clientdialog->hwnd, IDC_CL_CHATLIST ), -1, string );
}

//---------------------------------------------------------------------------
// find server callback 
// desc can be null b/c query provides a timed heartbeat
// doesnt currently have a context function though it should
GameDesc found_desc;

BOOL CALLBACK ConnectingDialogProc( HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		return true;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
	}
	
	return false;
}


bool QueryCallback( const GameDesc * desc, void * context )
{
	static modeless_box * connecting= NULL;

	if (!connecting)
		connecting = new modeless_box( hinst, IDD_CONNECTING, ConnectingDialogProc );

	if (!connecting->handle_msgs())
	{
		delete connecting;
		connecting= NULL;
		return false;
	}

	if (desc)
	{
		found_desc= *desc;

		if (connecting)
		{
			delete connecting;
			connecting= NULL;
		}

		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
// data that the client should send to the server
// including a way to get that dialog (via a modal IDD_CONNECT dialog)
struct connect_data
{
	connect_data( void ) { *name=0; }
	char name[NAMELEN];

	BOOL32 get_connect_data( HINSTANCE hins, HWND hwnd )
	{
		return DialogBoxParam(hins, MAKEINTRESOURCE( IDD_CONNECT ), hwnd, ConnectDialogProc, (long) this);
	}

	static BOOL CALLBACK ConnectDialogProc(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam )
	{
		switch (uMsg)
		{
			case WM_INITDIALOG:
			{
				if (!lParam) DestroyWindow(hwnd);
				SetWindowLong(hwnd, GWL_USERDATA, lParam);
				return TRUE;
			}

			case WM_COMMAND:
			{
				connect_data * d= (connect_data*) GetWindowLong(hwnd, GWL_USERDATA);
				
				HWND control = GetDlgItem(hwnd, LOWORD(wParam));
				int noteid= HIWORD(wParam);
				
				switch LOWORD(wParam)
				{
					case IDOK:
						if (d) 
						{
							Edit_GetText( GetDlgItem(hwnd, IDC_NAME), d->name, NAMELEN);
						}

					// fall through ok
					case IDCANCEL:
						DestroyWindow(hwnd);
						return TRUE;
				};
			}
		};

		return FALSE;
	}
};

//---------------------------------------------------------------------------
BOOL CALLBACK ClientDialogProc( HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_INITDIALOG: 
		{
			BOOL32 success=FALSE;

			connectdata->get_connect_data(hinst, NULL);
			
			if (*(connectdata->name))
			{
				SampleGameDesc query_for;

				// server query finished
				if (RendezvousQuery::QueryLAN( query_for, QueryCallback, NULL ))
				{
					// query succeeded
					if (found_desc.addr.type() != NetAddr::INVALID)
					{
						// connected to the found server
						if (link.link_connect( found_desc.addr ))
						{
							// send inital data
							link.systemMsg.WriteString( connectdata->name );
							SetWindowText(hwnd, LocalStr("Client: %s", connectdata->name ));
							success=TRUE;
						}
					}
				}
			}

			if (!success)
			{
				MessageBox(hwnd, "Couldnt connect", "Connect error", MB_OK | MB_SETFOREGROUND );
				DestroyWindow(hwnd);
			}

		}
		return TRUE;

		case WM_COMMAND:
		{
			HWND control = GetDlgItem(hwnd, LOWORD(wParam));
			int noteid= HIWORD(wParam);

			switch (LOWORD(wParam))
			{
			// user asked to send a chat message
				case IDC_CL_SEND:
				{
					char chatmsg[CHATLEN]= "\0";
					Edit_GetText( GetDlgItem( hwnd, IDC_CL_CHAT ), chatmsg, CHATLEN );

					if (*chatmsg)
					{
						link.systemMsg.WriteString( chatmsg );
						add_message( LocalStr( "> %s", chatmsg ));
					}

					Edit_SetText( GetDlgItem( hwnd, IDC_CL_CHAT ), "\0");
				}
				return TRUE;
			};
		}
		break;
		
		case WM_CLOSE:
			DestroyWindow( hwnd );
		break;
	};

	return FALSE;
}

//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nShowCmd)
{
	hinst= hInstance;

	connectdata = new connect_data();
	clientdialog= new modeless_box( hInstance, IDD_CLIENT, ClientDialogProc );

	while (clientdialog->handle_msgs())
	{
		// poll for server messages
		ReadBuffer msg;

		int res=0;
		while ((res=link.PollMessages( &msg )) > 0)
		{
			char chatmsg[CHATLEN];
			msg.ReadString( chatmsg );
			add_message( chatmsg );
		}

		// check for poll error
		if ((link.Status() == LINK_OPEN) && (res < 0))
		{
			MessageBox( clientdialog->hwnd, "Poll Error", "Poll Error", MB_OK );
			break;
		}

		// send messages; check for flush error
		if (!link.FlushMessages())
		{
			if (link.Status() == LINK_OPEN)
			{
				MessageBox( clientdialog->hwnd, "Flush Error", "Flush Error", MB_OK );
				break;
			}
		}
	}

	link.close();

	if (clientdialog)
	{
		delete clientdialog;
		clientdialog= NULL;
	}

	if (connectdata)
	{
		delete connectdata;
		connectdata= NULL;
	}

	return TRUE;
}

