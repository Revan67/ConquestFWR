//////////////////////////////////////////////////////////////////////////////
//  File:    TraceViewBar.cpp
//
//  Purpose: This is a dockable window which displays debug messages from
//  the python program (stderr).
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

// TraceViewBar.cpp : implementation file
//

#include "stdafx.h"
#include "DbgRemote.h"
#include "TraceViewBar.h"
#include "TextFindDlg.h"

#include "scintilla\scintilla.h"
#include "scintilla\scilexer.h"


#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TRACE_BUFFER_SIZE 16384


const int TABINX_STDERR = 0;
const int TABINX_STDOUT = 1;
const int TABINX_BOTH = 2;

// Was 1000 - that's not nearly enough.
const int kLineScroll = 100000;

COLORREF	m_colErrFore = RGB(  0,  0,  0);
COLORREF	m_colErrBack = RGB(255,255,255);

COLORREF	m_colOutFore = RGB(255,255,255);
COLORREF	m_colOutBack = RGB(  0,  0,  0);

COLORREF	m_colMrgFore = RGB(255,255,255);
COLORREF	m_colMrgBack = RGB(  0,  0, 64);



class StopPainting
{
public:
	StopPainting(CWnd& window)
		: m_window(&window)
	{
		// If you use LockWindowUpdate/UnlockWindowUpdate on the edit window
		// then the call to UnlockWindowUpdate causes the entire screen - all
		// applications - to redraw. This is particularly annoying on a multi-mon
		// system where both monitors flicker annoyingly.
		// Using the parent window instead avoids this problem.
		//while (m_window->GetParent())
		//	m_window = m_window->GetParent();
		//m_window->LockWindowUpdate();
		m_window->SetRedraw(false);
	}
	~StopPainting()
	{
		//m_window->UnlockWindowUpdate();
		m_window->SetRedraw(true);
	}
private:
	CWnd* m_window;
};





#define STYLE_DEFAULT_ERR	10
#define STYLE_DEFAULT_OUT	20
#define STYLE_DEFAULT_MRG	30


/////////////////////////////////////////////////////////////////////////////
// CTraceViewBar

CTraceViewBar::CTraceViewBar()
{
	m_MaxLineCount = 100;
}

CTraceViewBar::~CTraceViewBar()
{
}


BEGIN_MESSAGE_MAP(CTraceViewBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CTraceViewBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	//ON_MESSAGE(UM_LINEENTERED, OnLineEntered)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_TRACVWPOP_CLEARALL, OnTracvwpopClearall)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
	ON_COMMAND(ID_TRACVWPOP_CLEARRUN, OnTracvwpopClearrun)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTraceViewBar message handlers
int CTraceViewBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_hwndScintilla = CreateWindowEx(0, 
		"Scintilla","", WS_CHILD|WS_VISIBLE|WS_TABSTOP,
		10,10,500,400,m_hWnd,(HMENU)1492, lpCreateStruct->hInstance,NULL);

	if (m_hwndScintilla == NULL)
	{
		return -1;
	}


	m_Tab.Create(TCS_TABS|WS_CHILD|WS_VISIBLE|WS_BORDER|TCS_BOTTOM|TCS_RIGHTJUSTIFY, CRect(0,0,10,10), this, 1011);
	m_Tab.SetMinTabWidth(0);
	m_Tab.InsertItem(TCIF_TEXT, TABINX_STDERR, "Error", 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, TABINX_STDOUT, "Output", 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, TABINX_BOTH, "Both", 0, 0);
	m_Tab.SetCurSel(TABINX_STDOUT);
	
	int LastSel = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetOutputWndLastSelTab();
	if ((LastSel >= TABINX_STDERR) && (LastSel <= TABINX_BOTH))
		m_Tab.SetCurSel(LastSel);


	if (!m_Font.CreatePointFont(75, "tahoma"))
        return -1;

    m_Tab.SetFont(&m_Font);

	m_fnEditor = reinterpret_cast<SciFnDirect> (::SendMessage(m_hwndScintilla, SCI_GETDIRECTFUNCTION, 0, 0) );
	m_ptrEditor = ::SendMessage(m_hwndScintilla,SCI_GETDIRECTPOINTER, 0, 0);

	InitializeScintilla();

	//initialize the buffers
	Clear();

    return 0;
}

void CTraceViewBar::SetReadOnly(bool bRead)
{
	//m_Edit.SetReadOnly(bRead);
}

void CTraceViewBar::Clear()
{
	m_vecErrBuffer.clear();
	m_vecErrBuffer.reserve(TRACE_BUFFER_SIZE);
	m_vecErrBuffer.push_back(0);
	
	m_vecOutBuffer.clear();
	m_vecOutBuffer.reserve(TRACE_BUFFER_SIZE);
	m_vecOutBuffer.push_back(0);
	
	m_vecMrgBuffer.clear();
	m_vecMrgBuffer.reserve(TRACE_BUFFER_SIZE);
	m_vecMrgBuffer.push_back(0);

	SendEditor(SCI_SETTEXT, 0, (LPARAM)"");
}

void CTraceViewBar::OnSize(UINT nType, int cx, int cy) 
{
	CSizingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);

	m_Tab.MoveWindow(rc, FALSE);
	
	m_Tab.AdjustRect(FALSE, &rc);
    //m_Edit.MoveWindow(rc);

	if (m_hwndScintilla)
	{
		::MoveWindow(m_hwndScintilla, rc.left, rc.top, rc.Width(), rc.Height(), TRUE);
	}
}

HBRUSH CTraceViewBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	/*if (nCtlColor == CTLCOLOR_EDIT)
	{
		switch (m_Tab.GetCurSel())
		{
		case TABINX_STDERR:
			pDC->SetBkColor(colErrBack); // change the background color
			pDC->SetTextColor(colErrFore); // change the text color
			return m_hBkBrushErr;

		case TABINX_STDOUT:
			pDC->SetBkColor(colOutBack); // change the background color
			pDC->SetTextColor(colOutFore); // change the text color
			return m_hBkBrushOut;

		case TABINX_BOTH:
			pDC->SetBkColor(colMrgBack); // change the background color
			pDC->SetTextColor(colMrgFore); // change the text color
			return m_hBkBrushMrg;
		}
	}*/
	
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CTraceViewBar::OnClose() 
{
	//DeleteObject(m_hBkBrushErr);
	//DeleteObject(m_hBkBrushOut);
	//DeleteObject(m_hBkBrushMrg); 

	CSizingControlBar::OnClose();
}
void CTraceViewBar::UpdateBuffer(vector<char>& vecMsg)
{
	int TextLen = SendEditor(SCI_GETTEXTLENGTH);
	vecMsg.resize(TextLen + 1);

	SendEditor(SCI_GETTEXT, vecMsg.size(), (LPARAM)&(vecMsg[0]));
}

void CTraceViewBar::AddMessage(const char* szMessage, vector<char>& vecMsg)
{
	// Count the length of the string.
	int AddLength = strlen(szMessage) + 2;
	// Add in how many line feeds there are - that sets the upper
	// limit on how much text will need to be added. This stops the
	// container from continually growing in capacity.
	AddLength += std::count(szMessage, szMessage + AddLength, '\n');

	if (vecMsg.size() + AddLength > vecMsg.capacity())
	{
		dprintf("Capacity is %d.\n", vecMsg.capacity());
		//remove lines until the new message fits
		char* pLineEnd = &(vecMsg[0]);
		while ((pLineEnd - &(vecMsg[0]) < AddLength) && (pLineEnd != NULL))
			pLineEnd = strchr(++pLineEnd, '\n');

		if (pLineEnd == NULL)
		{
			//This must be a very long string clear it out completely
			vecMsg.clear();
		}
		else
		{
			// Skip over the last linefeed found.
			++pLineEnd;
			int EraseCount = pLineEnd - &(vecMsg[0]);
			vecMsg.erase(vecMsg.begin(), vecMsg.begin()+EraseCount);
		}		
	}

	if (vecMsg.size() > 0)
	{
		assert(vecMsg[vecMsg.size() - 1] == 0);
		vecMsg.pop_back();
	}

	//replace \n with \r\n in the new string
	//for (int i = 0; i < AddLength; i++)
	for (const char* ptr = szMessage; *ptr; ++ptr)
	{
		if ((ptr[0] == '\n') && ((ptr == szMessage) || (ptr[-1] != '\r')))
			vecMsg.push_back('\r');
		vecMsg.push_back(ptr[0]);
	}
	//null terminate
	vecMsg.push_back(0);
}

void CTraceViewBar::AddDebugMessage(const char* szMessage)
{
	AddMessage(szMessage, m_vecErrBuffer);
	AddMessage(szMessage, m_vecMrgBuffer);


	if (m_Tab.GetCurSel() != TABINX_STDOUT)
	{
		int nLineCount = SendEditor(SCI_GETLINECOUNT);
		int nEndPos = SendEditor(SCI_GETLINEENDPOSITION, nLineCount-1);
		SendEditor(SCI_GOTOLINE, nLineCount);
		SendEditor(SCI_SETCURRENTPOS, nEndPos);
		SendEditor(SCI_SETSELECTIONSTART, nEndPos);
		SendEditor(SCI_SETSELECTIONEND, nEndPos);

		SendEditor(SCI_ADDTEXT, strlen(szMessage), (LPARAM) szMessage);
	}
}


void CTraceViewBar::AddOutputMessage(const char* szMessage)
{
	AddMessage(szMessage, m_vecOutBuffer);
	AddMessage(szMessage, m_vecMrgBuffer);
	
	
	if (m_Tab.GetCurSel() != TABINX_STDERR)
	{
		int nLineCount = SendEditor(SCI_GETLINECOUNT);
		int nEndPos = SendEditor(SCI_GETLINEENDPOSITION, nLineCount-1);
		SendEditor(SCI_GOTOLINE, nLineCount-1);
		SendEditor(SCI_SETCURRENTPOS, nEndPos);
		SendEditor(SCI_SETSELECTIONSTART, nEndPos);
		SendEditor(SCI_SETSELECTIONEND, nEndPos);

		SendEditor(SCI_ADDTEXT,strlen(szMessage), (LPARAM) szMessage);
	}
}


BOOL CTraceViewBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pNm = (NMHDR*) lParam;
	static bool bTabIsChanging = false;

	if(pNm->code == TCN_SELCHANGE)
	{
		//update styles sets the scintilla text which sends a SCN_MODIFIED / SC_MOD_DELETETEXT
		//this flag will signal the notify function to ignore the next such message
		bTabIsChanging = true;
		UpdateStyles();
		bTabIsChanging = false;

		((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->SetOutputWndLastSelTab(m_Tab.GetCurSel());
		
	}

	else if (pNm->hwndFrom == m_hwndScintilla)
	{
		SCNotification* pScinNM = reinterpret_cast<SCNotification*>(pNm);

		switch(pNm->code)
		{
			case SCN_MODIFYATTEMPTRO:
				//the user has attempted to type - we will only allow this if we are
				//dprintf("scintilla SCN_MODIFYATTEMPTRO\n");
				//SendEditor(SCI_SETREADONLY, (((CDbgRemoteApp*)AfxGetApp())->GetDebugState() != eDBGSTATE_BREAK));
				break;

			case SCN_UPDATEUI:
				//dprintf("scintilla SCN_UPDATEUI\n");
				//set as read only unless the app is in break..
				//SendEditor(SCI_SETREADONLY, (((CDbgRemoteApp*)AfxGetApp())->GetDebugState() != eDBGSTATE_BREAK));
				break;

			case SCN_DOUBLECLICK:
				OnEditorDoubleClick();
				break;
			case SCN_MODIFIED:
				if (SC_MOD_DELETETEXT & pScinNM->modificationType && !bTabIsChanging)
				{
					//the user has deleted selected text from the window,
					//update the buffer to match.
					
					int CurTab = m_Tab.GetCurSel();
					if (CurTab == TABINX_STDERR)
					{
						UpdateBuffer(m_vecErrBuffer);
					}
					else if (CurTab == TABINX_STDOUT)
					{
						UpdateBuffer(m_vecOutBuffer);
					}
					else
					{
						UpdateBuffer(m_vecMrgBuffer);
					}
				}
				break;

			case SCN_CHARADDED:
				if ('\n' == pScinNM->ch)
				{
					int pos = SendEditor(SCI_GETCURRENTPOS);
					int nLine = SendEditor(SCI_LINEFROMPOSITION, pos) - 1;
					int len = SendEditor(SCI_LINELENGTH, nLine);
					if (len > 0)
					{
						vector<char> buff;
						buff.resize(len+1);
						char* pBuff = &(buff[0]);

						SendEditor(SCI_GETLINE, nLine, (LPARAM)pBuff);

						for (unsigned int i = 0; i < buff.size(); i++)
						{
							if ((buff[i] == '\n') || (buff[i] == '\r'))
							{
								buff[i] = 0;
								break;
							}
						}

						//execute the last entered line
						OnLineEntered((WPARAM)&(buff[0]),0);
					}
				}
				else if ('.' == pScinNM->ch)
				{
					AutoComplete();
				}
				break;



			case SCN_USERLISTSELECTION:
			{
				int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
				if ((SelEnd > 0) && (strlen(pScinNM->text)))
				{
					SendEditor(SCI_INSERTTEXT, SelEnd, (LPARAM)pScinNM->text);
					SendEditor(SCI_SETCURRENTPOS, SelEnd + strlen(pScinNM->text), 0);
					SendEditor(SCI_SETSELECTIONSTART, SelEnd + strlen(pScinNM->text), 0);
				}
			}
			break;

			case SCN_STYLENEEDED:
			{
				int TextLen = SendEditor(SCI_GETTEXTLENGTH);
				SendEditor(SCI_STARTSTYLING, 0, 0xff);
				/*
				int CurTab = m_Tab.GetCurSel();
				if (CurTab == TABINX_STDERR)
					SendEditor(SCI_SETSTYLING, TextLen, STYLE_DEFAULT_ERR);
				else if (CurTab == TABINX_STDOUT)
					SendEditor(SCI_SETSTYLING, TextLen, STYLE_DEFAULT_OUT);
				else
					SendEditor(SCI_SETSTYLING, TextLen, STYLE_DEFAULT_MRG);
*/
				//dprintf("SCN_STYLENEEDED\n");
			}
			break;


		}//switch
	}//else if
	
	return CSizingControlBar::OnNotify(wParam, lParam, pResult);

} // OnNotify

LRESULT CTraceViewBar::OnLineEntered(WPARAM w, LPARAM l)
{
	switch (m_Tab.GetCurSel())
	{
	case TABINX_STDERR:
		UpdateBuffer(m_vecErrBuffer);
		break;
	case TABINX_STDOUT:
		UpdateBuffer(m_vecOutBuffer);
		AddMessage((const char*) w, m_vecMrgBuffer);
		break;
	case TABINX_BOTH:
		UpdateBuffer(m_vecMrgBuffer);
		AddMessage((const char*) w, m_vecOutBuffer);
		break;
	}
	
	
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	if (pApp)
		pApp->RunStatement((const char*) w);
	return 0;
}


LRESULT CTraceViewBar::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam) const
{
	assert(m_fnEditor);
	return m_fnEditor(m_ptrEditor, Msg, wParam, lParam);
}


void CTraceViewBar::InitializeScintilla()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	
	SendEditor(SCI_SETLEXER, SCLEX_NULL, 0);
	//SendEditor(SCI_SETLEXER, SCLEX_CONTAINER, 0);
	SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
	SendEditor(SCI_SETSTYLEBITS, 7);

	//disable the built in popup menu - so that we can replace it with our own.
	SendEditor(SCI_USEPOPUP, 0);

	int nTextDefaultSize = 10;
	switch (m_Tab.GetCurSel())
	{
	case TABINX_STDERR:
		SetAStyle(STYLE_DEFAULT, m_colErrFore, m_colErrBack, nTextDefaultSize, "Courier");
		break;
	case TABINX_STDOUT:
		SetAStyle(STYLE_DEFAULT, m_colOutFore, m_colOutBack, nTextDefaultSize, "Courier");
		break;
	case TABINX_BOTH:
		SetAStyle(STYLE_DEFAULT, m_colMrgFore, m_colMrgBack, nTextDefaultSize, "Courier");
		break;
	}
	SendEditor(SCI_STYLECLEARALL);
	SendEditor(SCI_SETUNDOCOLLECTION, false);


	SendEditor(SCI_SETMARGINWIDTHN, 0, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 2, 0);

	//set the tab width to a reasonable value
	int nTabWidth = pCfg->GetEditorTabWidth();
	if ((nTabWidth < 1) || (nTabWidth > 20))
		nTabWidth = 4;

	SendEditor(SCI_SETTABWIDTH, nTabWidth, 0);

	//sets the indent size in spaces
	SendEditor(SCI_SETINDENT, nTabWidth, 0);

	//true means an indent is a mixure of tabs and spaces, false means that only 
	//spaces are used.
	SendEditor(SCI_SETUSETABS, true, 0);

	//true means the tab key indents, false means it tabs
	SendEditor(SCI_SETTABINDENTS, false, 0);

	//set the caret color
	SendEditor(SCI_SETCARETFORE, RGB(192,192,192));
	
	SendEditor(SCI_SETMODEVENTMASK, SC_MOD_DELETETEXT);

	SendEditor(SCI_SETTEXT, 0, (LPARAM)"");
}

void CTraceViewBar::UpdateStyles()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CUserCfg::SEditorStyle traceStyle;				
		
	switch (m_Tab.GetCurSel())
	{
		case TABINX_STDERR:
			{
				pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_TRACEVW_ERR, &traceStyle);
				
				SendEditor(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(&(m_vecErrBuffer[0])));
				
				int nLineCount = SendEditor(SCI_GETLINECOUNT);
				int nEndPos = SendEditor(SCI_GETLINEENDPOSITION);
				SendEditor(SCI_GOTOLINE, nLineCount);
				SendEditor(SCI_SETCURRENTPOS, nEndPos);
				SendEditor(SCI_SETSELECTIONSTART, nEndPos);
				SendEditor(SCI_SETSELECTIONEND, nEndPos);

				SetAStyle(STYLE_DEFAULT, traceStyle.ForeCol, traceStyle.BackCol, traceStyle.PointSize, traceStyle.Facename);
				SendEditor(SCI_STYLECLEARALL);

				break;
			}
		case TABINX_STDOUT:
			{
				pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_TRACEVW_OUT, &traceStyle);
				
				SendEditor(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(&(m_vecOutBuffer[0])));
				int nLineCount = SendEditor(SCI_GETLINECOUNT);
				int nEndPos = SendEditor(SCI_GETLINEENDPOSITION);
				SendEditor(SCI_GOTOLINE, nLineCount);
				SendEditor(SCI_SETCURRENTPOS, nEndPos);
				SendEditor(SCI_SETSELECTIONSTART, nEndPos);
				SendEditor(SCI_SETSELECTIONEND, nEndPos);

				SetAStyle(STYLE_DEFAULT, traceStyle.ForeCol, traceStyle.BackCol, traceStyle.PointSize, traceStyle.Facename);
				SendEditor(SCI_STYLECLEARALL);
				break;
			}
		case TABINX_BOTH:
			{
				pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_TRACEVW_MRG, &traceStyle);
				
				SendEditor(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(&(m_vecMrgBuffer[0])));
				int nLineCount = SendEditor(SCI_GETLINECOUNT);
				int nEndPos = SendEditor(SCI_GETLINEENDPOSITION);
				SendEditor(SCI_GOTOLINE, nLineCount);
				SendEditor(SCI_SETCURRENTPOS, nEndPos);
				SendEditor(SCI_SETSELECTIONSTART, nEndPos);
				SendEditor(SCI_SETSELECTIONEND, nEndPos);

				SetAStyle(STYLE_DEFAULT, traceStyle.ForeCol, traceStyle.BackCol, traceStyle.PointSize, traceStyle.Facename);
				SendEditor(SCI_STYLECLEARALL);
				break;
			}
		}
}

void CTraceViewBar::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void CTraceViewBar::OnDestroy() 
{
	if (m_hwndScintilla)
	{
		::DestroyWindow(m_hwndScintilla);
		m_hwndScintilla = NULL;
		m_fnEditor = 0;
		m_ptrEditor = 0;
	}
		
	CSizingControlBar::OnDestroy();
}

BOOL CTraceViewBar::OnEraseBkgnd(CDC* pDC) 
{	
	//return CSizingControlBar::OnEraseBkgnd(pDC);
	return TRUE;
}


void CTraceViewBar::OnEditorDoubleClick()
{
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int nLine = SendEditor(SCI_LINEFROMPOSITION, pos);
	int len = SendEditor(SCI_LINELENGTH, nLine);
	if (len > 0)
	{
		vector<char> buff;
		buff.resize(len+1);
		char* pBuff = &(buff[0]);

		SendEditor(SCI_GETLINE, nLine, (LPARAM)pBuff);

		char* pFilenm = strstr(pBuff, "File ");
		if (pFilenm)
		{		
			//find the start of the filename
			pFilenm = strchr(pFilenm, '"');
			if (pFilenm)
			{
				pFilenm++;
				//find the end of the filename
				char* pFileEnd = strchr(pFilenm, '"');
				if (pFileEnd)
				{
					//null terminate the filename
					*pFileEnd = 0;
					pFileEnd++;
					if (strcmp("<string>", pFilenm) == 0)
					{
						//if the filename is <string> then assume we
						//want the active view
						pFilenm = "";
					}
					
					//find the line number
					int nLineNumber = 0;
					char* pLine = strstr(pFileEnd, "line ");	
					if (pLine)
					{
						pLine += strlen("line ");
						nLineNumber = strtol(pLine, 0, 10);
					}
					
					if (nLineNumber > 0)
					{
						((CDbgRemoteApp*)AfxGetApp())->DisplayFileLine(pFilenm, nLineNumber);
					}
				}
			}
		}
	}
} // OnEditorDoubleClick

void CTraceViewBar::AutoComplete()
{
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int nLine = SendEditor(SCI_LINEFROMPOSITION, pos);
	int len = SendEditor(SCI_LINELENGTH, nLine);
	if (len > 0)
	{
		vector<char> buff;
		buff.resize(len+1);
		char* pBuff = &(buff[0]);

		SendEditor(SCI_GETLINE, nLine, (LPARAM)pBuff);

		char* pEnd = strrchr(&(buff[0]), '.');
		if (pEnd)
		{
			*pEnd = 0;

			if (strlen(&(buff[0])))
			{
				((CDbgRemoteApp*)AfxGetApp())->GetAutoComplete(&(buff[0]), eDBGCLIENT_OUTPUT);
			}
		}
	}	
} // AutoComplete

void CTraceViewBar::DisplayAutoComplete(const char* cszMessage)
{
	const char* cszCurItem = cszMessage;
	string List;
	List = "";
	SendEditor(SCI_AUTOCSETSEPARATOR, '\n');

	int nItems = 0;
	while (*cszCurItem)
	{
		//find leading '
		if (cszCurItem[0] == 0x27)
		{
			char* pEnd = strchr(cszCurItem+1, 0x27);
			if (pEnd)
			{
				//move past the leading '
				cszCurItem++;
				while (cszCurItem < pEnd)
				{
					//copy the characters
					List += cszCurItem[0];
					cszCurItem++;
				}
				//add a delimiter
				List += '\n';
				nItems++;
			}
			else
				break;
		}
		
		cszCurItem++;
	}

	if (nItems)
	{
		SendEditor(SCI_USERLISTSHOW, List.size(), (WPARAM)List.c_str());
	}
} // DisplayAutoComplete

void CTraceViewBar::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// Load the menu resource. 	
	CMenu menu;
	menu.LoadMenu(IDR_TRACEVW_POPUP);

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	CMenu* pSubMenu = menu.GetSubMenu(0); 

	if (SendEditor(SCI_CANUNDO, 0, 0) == 0)
		pSubMenu->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND|MF_GRAYED);
	if (SendEditor(SCI_CANPASTE, 0, 0) == 0)
		pSubMenu->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND|MF_GRAYED);

	if (((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetClearOutput() )
		pSubMenu->CheckMenuItem(ID_TRACVWPOP_CLEARONRUN, MF_BYCOMMAND|MF_CHECKED);

	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); 
}

void CTraceViewBar::OnEditCut()
{
	SendEditor(SCI_CUT, 0, 0);
}

void CTraceViewBar::OnEditPaste()
{
	SendEditor(SCI_PASTE, 0, 0);
}

void CTraceViewBar::OnEditCopy()
{
	SendEditor(SCI_COPY, 0, 0);
}

void CTraceViewBar::OnEditUndo()
{
	SendEditor(SCI_UNDO, 0, 0);
}


void CTraceViewBar::OnTracvwpopClearall()
{
	Clear();
}

void CTraceViewBar::OnTracvwpopClearrun()
{
	bool bClear = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetClearOutput();
	((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->SetClearOutput( ! bClear );
}





BOOL CTraceViewBar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CSizingControlBarG::OnCommand(wParam, lParam);
}




void CTraceViewBar::OnEditFind()
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos < 0)
		CurPos = 0;
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	
	CTextFindDlg dlg;

	if (SelEnd > CurPos)
	{
		TEXTRANGE tr;
		tr.chrg.cpMin = CurPos;
		tr.chrg.cpMax = SelEnd;
		vector<char> vecText;
		vecText.resize(SelEnd - CurPos+1, 0);
		tr.lpstrText = &vecText[0];
		
		SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
		dlg.m_FindText = tr.lpstrText;
	}

	dlg.m_MatchCase  = m_FindParams.nFlags & SCFIND_MATCHCASE;
	dlg.m_MatchStart = m_FindParams.nFlags & SCFIND_WORDSTART;
	dlg.m_MatchWord  = m_FindParams.nFlags & SCFIND_WHOLEWORD;
	dlg.m_UseRegExpr = m_FindParams.nFlags & SCFIND_REGEXP;

	
	if (dlg.DoModal() == IDOK)
	{
		m_FindParams.nFlags = 0;
		if (dlg.m_MatchCase)
			m_FindParams.nFlags |= SCFIND_MATCHCASE;
		if (dlg.m_MatchStart)
			m_FindParams.nFlags |= SCFIND_WORDSTART;
		if (dlg.m_MatchWord)
			m_FindParams.nFlags |= SCFIND_WHOLEWORD;
		if (dlg.m_UseRegExpr)
			m_FindParams.nFlags |= SCFIND_REGEXP;
		
		m_FindParams.FindStr = dlg.m_FindText;
		m_FindParams.ttf.lpstrText = (char*)((const char*)m_FindParams.FindStr);
		m_FindParams.ttf.chrg.cpMin = CurPos;
		
		int nLineCount = SendEditor(SCI_GETLINECOUNT, 0, 0);
		int nEndPos = SendEditor(SCI_GETLINEENDPOSITION, nLineCount, 0);
		m_FindParams.ttf.chrg.cpMax = nEndPos;

		
		int Pos = SendEditor(SCI_FINDTEXT, m_FindParams.nFlags, (LPARAM)&(m_FindParams.ttf));

		if (Pos > -1)
		{
			int NewLine = SendEditor(SCI_LINEFROMPOSITION, Pos, 0);
			SendEditor(SCI_ENSUREVISIBLE, NewLine);
			SendEditor(SCI_GOTOLINE, NewLine);
			SendEditor(SCI_SETSEL, m_FindParams.ttf.chrgText.cpMin, m_FindParams.ttf.chrgText.cpMax);
			::SetFocus(m_hwndScintilla);
		}
	}
}

void CTraceViewBar::OnEditFindnext()
{
	if (m_FindParams.FindStr.GetLength() == 0)
	{
		OnEditFind();
		return;
	}


	m_FindParams.ttf.chrg.cpMin = SendEditor(SCI_GETCURRENTPOS, 0, 0);;
		
	int nLineCount = SendEditor(SCI_GETLINECOUNT, 0, 0);
	int nEndPos = SendEditor(SCI_GETLINEENDPOSITION, nLineCount, 0);
	m_FindParams.ttf.chrg.cpMax = nEndPos;

	int Pos = SendEditor(SCI_FINDTEXT, m_FindParams.nFlags, (LPARAM)&(m_FindParams.ttf));
	if (Pos < 0)
	{
		m_FindParams.ttf.chrg.cpMax = m_FindParams.ttf.chrg.cpMin;
		m_FindParams.ttf.chrg.cpMin = 0;
		Pos = SendEditor(SCI_FINDTEXT, m_FindParams.nFlags, (LPARAM)&(m_FindParams.ttf));
	}

	if (Pos > -1)
	{
		int NewLine = SendEditor(SCI_LINEFROMPOSITION, Pos, 0);
		SendEditor(SCI_ENSUREVISIBLE, NewLine);
		SendEditor(SCI_GOTOLINE, NewLine);
		SendEditor(SCI_SETSEL, m_FindParams.ttf.chrgText.cpMin, m_FindParams.ttf.chrgText.cpMax);
		::SetFocus(m_hwndScintilla);
	}
}




