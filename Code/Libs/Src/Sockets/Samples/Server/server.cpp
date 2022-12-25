//---------------------------------------------------------------------------
// SERVER
//---------------------------------------------------------------------------
#include "..\common.h"

HINSTANCE hinst=NULL;

modeless_box * serverdialog=NULL;		// dialog handler
RendezvousPoint * connectpoint= NULL;		// place where clients connect to

// never provided a context for the netlinks (sorry... tlp)
struct Client 
{
	Client(void) { 
		inited= FALSE; *name=0;
	}

	BOOL32 inited;
	char name[NAMELEN];
};

DynamicArray<Client> clients;

//---------------------------------------------------------------------------
// add a message to the chat list part of the dialog
inline void add_message( const char * string )
{
	if (serverdialog)
		ListBox_InsertString( GetDlgItem( serverdialog->hwnd,  IDC_SV_CHATLIST ), -1, string );
}

//---------------------------------------------------------------------------
inline void add_client( int id, const char * name )
{
	if (serverdialog)
		ListBox_InsertString( GetDlgItem( serverdialog->hwnd, IDC_SV_CLIENTLIST), id, name );
}

inline void rub_client( int id )
{
	if (serverdialog)
		ListBox_DeleteString( GetDlgItem( serverdialog->hwnd, IDC_SV_CLIENTLIST), id );
}

//---------------------------------------------------------------------------
inline void broadcast( int id, const char * string )
{
	add_message( string );

	// send to all existing links (clients)
	for (int i=0; i< connectpoint->numlinks(); i++) {
		if (i!= id)
			connectpoint->link(i)->systemMsg.WriteString( string );
	}
}

//---------------------------------------------------------------------------
BOOL CALLBACK ServerDialogProc( HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_INITDIALOG:
		return TRUE;

		case WM_COMMAND:
		{
			HWND control = GetDlgItem(hwnd, LOWORD(wParam));
			int noteid= HIWORD(wParam);

			switch (LOWORD(wParam))
			{
			// server asked to send a chat message
				case IDC_SV_SEND:
				{
					char chatmsg[CHATLEN]= "\0";
					Edit_GetText( GetDlgItem( hwnd, IDC_SV_CHAT ), chatmsg, CHATLEN );

					if (*chatmsg) {
						char msg[CHATLEN]="server >";
						strncat( msg, chatmsg, CHATLEN );
					
						broadcast( -1, msg );
					}

					Edit_SetText( GetDlgItem( hwnd, IDC_SV_CHAT ), "\0");
				}
				return TRUE;

				case IDC_SV_CHAT:
				return TRUE;
			};
		}
		break;

		case WM_CLOSE:
			DestroyWindow( hwnd );
		return TRUE;
	};

	return FALSE;
}

WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nShowCmd)
{
	hinst= hInstance;

	SampleGameDesc desc;

	connectpoint= new RendezvousPoint(desc);

	connectpoint->OpenRendezvous();

	serverdialog= new modeless_box( hInstance, IDD_SERVER, ServerDialogProc );

	while (serverdialog->handle_msgs())
	{
		// link is now open;
		// can send game specific data
		NetLink * newlink;
		if (connectpoint->PollIncoming( &newlink ) > 0)
		{
		}

		// process all existing links (clients)
		for (int i=0; i< connectpoint->numlinks(); i++)
		{
			NetLink * link= connectpoint->link( i );

			ReadBuffer msg;

		// get new messages
			int res=0;
			
			while ((res=link->PollMessages(&msg)) > 0)
			{
				// expecting first message to be a name
				if (!clients[i].inited)
				{
					msg.ReadString( clients[i].name );
					add_client( i, clients[i].name );
					clients[i].inited= TRUE;
				}
				// expecting all other messages to be chat strings
				else
				{
					char chatmsg[CHATLEN];
					msg.ReadString( chatmsg );
				
					broadcast( i, LocalStr("client %s > %s", clients[i].name, chatmsg ) );
				}
			}

		// check for poll error;
		// disconnect client
			if (res < 0)
			{
				if (clients[i].inited)
				{
					clients[i].inited= FALSE;
					broadcast( i, LocalStr("client %s left", clients[i].name ));
					rub_client( i );
					link->close();
				}
			}

		// send messages
			link->FlushMessages();
		}
	}

	connectpoint->CloseRendezvous();

	return TRUE;
}

