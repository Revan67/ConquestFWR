// CCommonDialog.h
//
//
//



#ifndef CCOMMONDIALOG_H
#define CCOMMONDIALOG_H

#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>

class CCommonDialog
{
public:
	CCommonDialog( HWND hOwner=NULL, const char *szFilter="All Files (*.*)\0*.*\0\0", DWORD Flags=OFN_HIDEREADONLY, BOOL bFileOpen=TRUE )
	{
//		InitCommonControls();

		memset( &m_OFN, 0, sizeof(OPENFILENAME) );
		m_OFN.lStructSize = sizeof(OPENFILENAME);
		m_OFN.hwndOwner = hOwner;
		m_OFN.lpstrFilter = szFilter;
		m_OFN.Flags = Flags;
		m_bFileOpen = bFileOpen;
		m_szFilename[0] = 0;
	}

	~CCommonDialog()
	{
	}

	HRESULT DoModal()
	{
		m_OFN.nMaxFile = _MAX_PATH;
		m_OFN.lpstrFile = &m_szFilename[0];

		if( m_bFileOpen ) {
			if( GetOpenFileName( &m_OFN ) ) {
				return S_OK;
			}
		}
		else {
			if( GetSaveFileName( &m_OFN ) ) {
				return S_OK;
			}
		}

		return E_FAIL;
	}

	HRESULT GetPathName( char *szBuffer, U32 nBufferSize )
	{
		if( nBufferSize < (1+strlen(m_szFilename)) ) {
			return E_FAIL;
		}

		strcpy( szBuffer, m_szFilename );

		return S_OK;
	}
	
protected:
	BOOL m_bFileOpen;
	char m_szFilename[_MAX_PATH];
	OPENFILENAME m_OFN;
};


#endif
