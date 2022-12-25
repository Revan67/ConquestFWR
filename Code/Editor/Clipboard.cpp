//
// Clipboard.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "Clipboard.h"
#include "CQTrace.h"

#include <afxpriv.h>
#include <afxole.h>

#include <TComponent.h>
#include <startup.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------

struct Clipboard : IClipboard
{
	BEGIN_DACOM_MAP_INBOUND(Clipboard)
		DACOM_INTERFACE_ENTRY(IClipboard)
	END_DACOM_MAP()

	virtual bool Copy( IClipboardObject& _object );
	virtual bool Append( IClipboardObject& _object );
	virtual bool Paste( IClipboardObject& _object );

	virtual bool HasDataForType( const char* _type )
	{
		UINT clipboardFormat = ::RegisterClipboardFormat( _type );

		COleDataObject obj;

		if( obj.AttachClipboard() ) 
		{
			if (obj.IsDataAvailable(clipboardFormat)) 
			{
				return true;
			}
		}

		return false;
	}
};

//----------------------------------------------------------------------------------------------

bool Clipboard::Copy( IClipboardObject& _object )
{
	UINT clipboardFormat = ::RegisterClipboardFormat( _object.GetType() );

	CSharedFile	sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);

	if( _object.Copy(sf) )
	{
		COleDataSource*	pSource = new COleDataSource();

		HGLOBAL hMem = sf.Detach();
		if( hMem )
		{
			pSource->CacheGlobalData(clipboardFormat, hMem);
			pSource->SetClipboard();
			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------

bool Clipboard::Append( IClipboardObject& _object )
{
	return true;
}

//----------------------------------------------------------------------------------------------

bool Clipboard::Paste( IClipboardObject& _object )
{
	UINT clipboardFormat = ::RegisterClipboardFormat( _object.GetType() );

	COleDataObject obj;

	bool ret = false;

	if( obj.AttachClipboard() ) 
	{
		if (obj.IsDataAvailable(clipboardFormat)) 
		{
			HGLOBAL hmem = obj.GetGlobalData(clipboardFormat);
			if( hmem )
			{
				CSharedFile	sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);
				sf.Write( (BYTE*) ::GlobalLock(hmem), ::GlobalSize(hmem));
				sf.SeekToBegin();

				ret = _object.Paste( sf );

				::GlobalUnlock(hmem);
			}
		}
		obj.Detach();
	}

	return ret;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _Clipboard : GlobalComponent
{
	Clipboard * clipboard;

	virtual void Startup (void)
	{
		clipboard = new DAComponent<Clipboard>;
		AddToGlobalCleanupList((IDAComponent **) &clipboard);
		CLIPBOARD = clipboard;
	}

	virtual void Initialize (void)
	{
		::AfxOleInit();
	}
};
static _Clipboard __Clipboard;
