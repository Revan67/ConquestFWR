//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemoteView.cpp
//
//  Purpose: implementation of the CDbgRemoteView class which provides a single
//  view of a single python source file
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

#include "stdafx.h"
#include "DbgRemote.h"

#include "DbgRemoteDoc.h"
#include "DbgRemoteView.h"

#include "TextFindDlg.h"
#include "TextReplaceDlg.h"
#include "GotoLineDlg.h"
#include "childfrm.h"

#include "SourceControlInterface.h"
#include "BrowseSelDlg.h"

#include "scintilla\scintilla.h"
#include "scintilla\scilexer.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MARKER_CURRENTLINE	3
#define MARKER_BREAKPOINT   2
#define MARKER_BOOKMARK     1

#define BOOKMARK_MASK	(0x00000001 << MARKER_BOOKMARK)
#define BREAKPOINT_MASK (0x00000001 << MARKER_BREAKPOINT)
#define CURLINE_MASK    (0x00000001 << MARKER_CURRENTLINE)
#define MARKER_MASKS	(BOOKMARK_MASK|BREAKPOINT_MASK|CURLINE_MASK)

#define MARGIN_LINENUMBERS	0
#define MARGIN_MARKS		1

#define FOLDFLAG_ABOVEEXPAND	0x02 //2 is draw above if expanded
#define FOLDFLAG_ABOVENOTEXP	0x04 //4 is draw above if not expanded
#define FOLDFLAG_BELOWEXPAND	0x08 //8 is draw below if expanded and 
#define FOLDFLAG_BOLOWNOTEXP	0x10 //16 is draw below if not expanded.
/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteView


IMPLEMENT_DYNCREATE(CDbgRemoteView, CView)


BEGIN_MESSAGE_MAP(CDbgRemoteView, CView)
	//{{AFX_MSG_MAP(CDbgRemoteView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_BOOKMARKS_TOGGLE, OnBookmarksToggle)
	ON_COMMAND(ID_BOOKMARKS_GOTONEXT, OnBookmarksGotonext)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_GOTONEXT, OnUpdateBookmarksGotonext)
	ON_COMMAND(ID_BOOKMARKS_GOTOPREVIOUS, OnBookmarksGotoprevious)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_GOTOPREVIOUS, OnUpdateBookmarksGotoprevious)
	ON_COMMAND(ID_BOOKMARKS_CLEARALL, OnBookmarksClearall)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_CLEARALL, OnUpdateBookmarksClearall)
	ON_COMMAND(ID_DEBUG_BREAKPOINTS_TOGGLE, OnDebugBreakpointsToggle)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_VIEW_EDITOROPTIONS_VIEWWHITESPACE, OnViewWhitespace)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EDITOROPTIONS_VIEWWHITESPACE, OnUpdateViewWhitespace)
	ON_COMMAND(ID_VIEW_SELECTIVEDISPLAY, OnViewSelectivedisplay)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SELECTIVEDISPLAY, OnUpdateViewSelectivedisplay)
	ON_COMMAND(ID_VIEW_LINENUMBERS, OnViewLinenumbers)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINENUMBERS, OnUpdateViewLinenumbers)
	ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
	ON_COMMAND(ID_EDIT_GOTOLINE, OnEditGotoline)
	ON_COMMAND(ID_VIEW_VIEWLINEENDINGS, OnViewViewlineendings)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIEWLINEENDINGS, OnUpdateViewViewlineendings)
	ON_COMMAND(ID_DEBUG_ADDTOWATCH, OnDebugAddtoWatch)
	ON_COMMAND(ID_EDIT_SPACESTOTABS, OnSpacesToTabs)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
	ON_COMMAND(ID_VIEW_ENDOFLINETYPE_CRLF, OnViewEOLTypeCRLF)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENDOFLINETYPE_CRLF, OnUpdateViewEOLTypeCRLF)
	ON_COMMAND(ID_VIEW_ENDOFLINETYPE_CR, OnViewEOLTypeCR)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENDOFLINETYPE_CR, OnUpdateViewEOLTypeCR)
	ON_COMMAND(ID_VIEW_ENDOFLINETYPE_LF, OnViewEOLTypeLF)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENDOFLINETYPE_LF, OnUpdateViewEOLTypeLF)
	ON_COMMAND(ID_EDIT_UNCOMMENTLINE, OnEditUncommentline)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNCOMMENTLINE, OnUpdateEditUncommentline)
	ON_COMMAND(ID_EDIT_COMMENTLINE, OnEditCommentline)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COMMENTLINE, OnUpdateEditCommentline)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_VIEW_EXPANDALL, OnViewExpandAll)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EXPANDALL, OnUpdateViewExpandAll)
	ON_COMMAND(ID_VIEW_COLLAPSEALL, OnViewCollapseAll)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLLAPSEALL, OnUpdateViewCollapseAll)
	ON_COMMAND(ID_EDIT_FINDINFILES, OnEditFindinFiles)
	ON_COMMAND(ID_EDIT_TABSTOSPACES, OnTabsToSpaces)           // Added 10-15-02 Bruce Dickey
	ON_COMMAND(ID_EDIT_INDENT,   OnIndent)                     // Added 10-16-02 Bruce Dickey
	ON_COMMAND(ID_EDIT_UNINDENT, OnUnindent)                   // Added 10-16-02 Bruce Dickey
	//}}AFX_MSG_MAP
	// Standard printing commands
	
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)	
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_COMMAND(ID_EDIT_GOTOTAGDEFINITION, OnEditGototagdefinition)
	ON_UPDATE_COMMAND_UI(ID_EDIT_GOTOTAGDEFINITION, OnUpdateEditGototagdefinition)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_COMMAND(ID_VIEW_SPLITWINDOW, OnViewSplitwindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SPLITWINDOW, OnUpdateViewSplitwindow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteView construction/destruction

CDbgRemoteView::CDbgRemoteView()
	: CView()
{
	m_hwndScintilla = NULL;
	m_bShowLinenumbers = FALSE;
	m_nLineNumberMarginWidth = 30;
	m_nMarkerMarginWidth = 15;

	m_fnEditor = NULL;
	m_ptrEditor = 0;
	m_bShowSelectiveDisplay = FALSE;
}

CDbgRemoteView::~CDbgRemoteView()
{
}


void CDbgRemoteView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	if ( ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetFoldOnOpen())
	{
		//fold the document
		m_bShowSelectiveDisplay = true;
		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold", (WPARAM)"1");
		SendEditor(SCI_CLEARDOCUMENTSTYLE);
		SendEditor(SCI_COLOURISE, 0, -1);
		FoldAll(true);
	}
	else
	{
		//this allows certain file types (xml) to display lexed
		SendEditor(SCI_CLEARDOCUMENTSTYLE);
		SendEditor(SCI_COLOURISE, 0, -1);
	} 

}

/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteView printing

BOOL CDbgRemoteView::OnPreparePrinting(CPrintInfo* pInfo)
{
	dprintf("OnPreparePrinting\n");
	SendEditor(SCI_SETPRINTCOLOURMODE, SC_PRINT_BLACKONWHITE);	
	SendEditor(SCI_SETPRINTMAGNIFICATION, -3);
	
	// default preparation
	return DoPreparePrinting(pInfo);
}


RECT pagesetupMargin;
CFont fontHeader;
CFont fontFooter;
FORMATRANGE frPrint;
int headerLineHeight = 0;
int footerLineHeight = 0;
LONG lengthPrinted = 0;
LONG lengthDoc = 0;

	

void CDbgRemoteView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	dprintf("OnBeginPrinting:\n");

	pInfo->SetMaxPage(-1);
	pInfo->SetMinPage(1);

	
	RECT rectMargins, rectPhysMargins;
	POINT ptPage;
	POINT ptDpi;

	// Get printer resolution
	ptDpi.x = pDC->GetDeviceCaps(LOGPIXELSX);    // dpi in X direction
	ptDpi.y = pDC->GetDeviceCaps(LOGPIXELSY);    // dpi in Y direction


	// Start by getting the physical page size (in device units).
	ptPage.x = pDC->GetDeviceCaps(PHYSICALWIDTH);   // device units
	ptPage.y = pDC->GetDeviceCaps(PHYSICALHEIGHT);  // device units


	// Get the dimensions of the unprintable
	// part of the page (in device units).
	rectPhysMargins.left = pDC->GetDeviceCaps(PHYSICALOFFSETX);
	rectPhysMargins.top = pDC->GetDeviceCaps(PHYSICALOFFSETY);

	// To get the right and lower unprintable area,
	// we take the entire width and height of the paper and
	// subtract everything else.
	rectPhysMargins.right = ptPage.x						// total paper width
	                        - pDC->GetDeviceCaps(HORZRES) // printable width
	                        - rectPhysMargins.left;				// left unprintable margin

	rectPhysMargins.bottom = ptPage.y						// total paper height
	                         - pDC->GetDeviceCaps(VERTRES)	// printable height
	                         - rectPhysMargins.top;				// right unprintable margin


	// At this point, rectPhysMargins contains the widths of the
	// unprintable regions on all four sides of the page in device units.

	// Take in account the page setup given by the user (if one value is not null)
/*	if (pagesetupMargin.left != 0 || pagesetupMargin.right != 0 ||
	        pagesetupMargin.top != 0 || pagesetupMargin.bottom != 0) {
		RECT rectSetup;

		// Convert the hundredths of millimeters (HiMetric) or
		// thousandths of inches (HiEnglish) margin values
		// from the Page Setup dialog to device units.
		// (There are 2540 hundredths of a mm in an inch.)

		char localeInfo[3];
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);

		if (localeInfo[0] == '0') {	// Metric system. '1' is US System
			rectSetup.left   = MulDiv (pagesetupMargin.left, ptDpi.x, 2540);
			rectSetup.top    = MulDiv (pagesetupMargin.top, ptDpi.y, 2540);
			rectSetup.right	 = MulDiv(pagesetupMargin.right, ptDpi.x, 2540);
			rectSetup.bottom = MulDiv(pagesetupMargin.bottom, ptDpi.y, 2540);
		} else {
			rectSetup.left	 = MulDiv(pagesetupMargin.left, ptDpi.x, 1000);
			rectSetup.top	 = MulDiv(pagesetupMargin.top, ptDpi.y, 1000);
			rectSetup.right	 = MulDiv(pagesetupMargin.right, ptDpi.x, 1000);
			rectSetup.bottom = MulDiv(pagesetupMargin.bottom, ptDpi.y, 1000);
		}

		// Dont reduce margins below the minimum printable area
		rectMargins.left	 = max(rectPhysMargins.left, rectSetup.left);
		rectMargins.top	     = max(rectPhysMargins.top, rectSetup.top);
		rectMargins.right	 = max(rectPhysMargins.right, rectSetup.right);
		rectMargins.bottom	 = max(rectPhysMargins.bottom, rectSetup.bottom);
	} else */{
		rectMargins.left	 = rectPhysMargins.left;
		rectMargins.top	     = rectPhysMargins.top;
		rectMargins.right	 = rectPhysMargins.right;
		rectMargins.bottom	 = rectPhysMargins.bottom;
	}



	// rectMargins now contains the values used to shrink the printable
	// area of the page.
	
	// Convert device coordinates into logical coordinates
	pDC->DPtoLP((LPPOINT) &rectMargins, 2);
	pDC->DPtoLP((LPPOINT)&rectPhysMargins, 2);

	
	// Convert page size to logical units and we're done!
	pDC->DPtoLP((LPPOINT) &ptPage, 1);

	TEXTMETRIC tm;
	string headerOrFooter;
	headerOrFooter.assign("", MAX_PATH + 100);	// Usually the path, date et page number

	headerLineHeight = ::MulDiv(9, ptDpi.y, 72);
	fontHeader.CreateFont(headerLineHeight,
	                                0, 0, 0,
	                                FW_NORMAL,
	                                0,
	                                0,
	                                0, 0, 0,
	                                0, 0, 0,
	                                "Arial");
	pDC->SelectObject(&fontHeader);
	pDC->GetTextMetrics(&tm);
	headerLineHeight = tm.tmHeight + tm.tmExternalLeading;

	footerLineHeight = ::MulDiv(9, ptDpi.y, 72);
	fontFooter.CreateFont(footerLineHeight,
	                                0, 0, 0,
	                                FW_NORMAL,
	                                0,
	                                0,
	                                0, 0, 0,
	                                0, 0, 0,
	                                "Arial");
	pDC->SelectObject(&fontFooter);
	pDC->GetTextMetrics(&tm);
	footerLineHeight = tm.tmHeight + tm.tmExternalLeading;

	lengthDoc = SendEditor(SCI_GETLENGTH);
	LONG lengthDocMax = lengthDoc;
	lengthPrinted = 0;

	// Requested to print selection
	/*if (pdlg.Flags & PD_SELECTION) {
		if (startPos > endPos) {
			lengthPrinted = endPos;
			lengthDoc = startPos;
		} else {
			lengthPrinted = startPos;
			lengthDoc = endPos;
		}

		if (lengthPrinted < 0)
			lengthPrinted = 0;
		if (lengthDoc > lengthDocMax)
			lengthDoc = lengthDocMax;
	}*/

	// We must substract the physical margins from the printable area
	frPrint.hdc = pDC->GetSafeHdc();
	frPrint.hdcTarget = pDC->m_hAttribDC;
	frPrint.rc.left = rectMargins.left - rectPhysMargins.left;
	frPrint.rc.top = rectMargins.top - rectPhysMargins.top;
	frPrint.rc.right = ptPage.x - rectMargins.right - rectPhysMargins.left;
	frPrint.rc.bottom = ptPage.y - rectMargins.bottom - rectPhysMargins.top;
	frPrint.rcPage.left = 0;
	frPrint.rcPage.top = 0;
	frPrint.rcPage.right = ptPage.x - rectPhysMargins.left - rectPhysMargins.right - 1;
	frPrint.rcPage.bottom = ptPage.y - rectPhysMargins.top - rectPhysMargins.bottom - 1;
	
	frPrint.rc.top += headerLineHeight + headerLineHeight / 2;
	frPrint.rc.bottom -= footerLineHeight + footerLineHeight / 2;	

} // OnBeginPrinting

void CDbgRemoteView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	dprintf("OnEndPrinting:\n");
	SendEditor(SCI_FORMATRANGE, FALSE, 0);

	fontHeader.DeleteObject();
	fontFooter.DeleteObject();
}

void CDbgRemoteView::OnPrepareDC( CDC* pDC, CPrintInfo* pInfo = NULL )
{
	//dprintf("OnPrepareDC\n");
	CView::OnPrepareDC(pDC, pInfo);

	if(pDC->IsPrinting()) // Is the DC a printer DC.
	{
		pInfo->m_bContinuePrinting = (lengthPrinted < lengthDoc);
	}

}

void CDbgRemoteView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{	
	bool printPage = ((pInfo->m_nCurPage >= pInfo->GetFromPage()) && 
		              (pInfo->m_nCurPage <= pInfo->GetToPage()));

	if (printPage) 
	{
		//left header (filename)
		CString sHeader = GetDocument()->GetFileName();
		
		pDC->SetTextColor(RGB(0,0,0));
		pDC->SetBkColor(RGB(255,255,255));
		CFont* pOldFont = (CFont*)pDC->SelectObject(&fontHeader);
		UINT ta = pDC->SetTextAlign(TA_BOTTOM);
		
		pDC->TextOut(frPrint.rc.left + 5, frPrint.rc.top - headerLineHeight / 2, sHeader);
		
		//right header (date/time)
		sHeader = CTime::GetCurrentTime().Format("%d %b %Y %I:%M %p");
		CSize TextSize = pDC->GetTextExtent(sHeader);

		pDC->TextOut(frPrint.rc.right - 5 - TextSize.cx, frPrint.rc.top - headerLineHeight / 2,
				     sHeader);
		
		pDC->SetTextAlign(ta);
		pDC->SelectObject(pOldFont);
	}

	frPrint.chrg.cpMin = lengthPrinted;
	frPrint.chrg.cpMax = lengthDoc;

	lengthPrinted = SendEditor(SCI_FORMATRANGE,
		                       printPage,
								reinterpret_cast<LPARAM>(&frPrint));
	if (printPage) 
	{
		//print footer (page number) centered
		CString FootBuff;
		FootBuff.Format("Page %d", pInfo->m_nCurPage);

		CSize TextSize = pDC->GetTextExtent(FootBuff);

		pDC->SetTextColor(RGB(0,0,0));
		pDC->SetBkColor(RGB(255,255,255));
		pDC->SelectObject(&fontFooter);
		UINT ta = pDC->SetTextAlign(TA_TOP);
		
		pDC->TextOut((frPrint.rc.left + frPrint.rc.right) / 2 - TextSize.cx / 2, 
			frPrint.rc.bottom + footerLineHeight / 2,
				     FootBuff);
		pDC->SetTextAlign(ta);

	}
} // OnPrint

/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteView diagnostics

#ifdef _DEBUG
void CDbgRemoteView::AssertValid() const
{
	CView::AssertValid();
}

void CDbgRemoteView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CDbgRemoteDoc* CDbgRemoteView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDbgRemoteDoc)));
	return (CDbgRemoteDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteView message handlers



int CDbgRemoteView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_hwndScintilla = CreateWindowEx(0, 
		"Scintilla","", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		10,10,500,400,m_hWnd,(HMENU)1492, lpCreateStruct->hInstance,NULL);

	if (m_hwndScintilla == NULL)
	{
		return -1;
	}
	
	m_fnEditor = reinterpret_cast<SciFnDirect> (::SendMessage(m_hwndScintilla, SCI_GETDIRECTFUNCTION, 0, 0) );
	m_ptrEditor = ::SendMessage(m_hwndScintilla,SCI_GETDIRECTPOINTER, 0, 0);

	CDbgRemoteDoc* pDoc = GetDocument();
	assert(pDoc);
	void* pSciDoc = pDoc->GetSciDocPtr();
	if (pSciDoc)
	{
		SendEditor(SCI_SETDOCPOINTER,0, (LPARAM)pSciDoc);
	}
	else
	{
		pSciDoc = (void*)SendEditor(SCI_GETDOCPOINTER,0, 0);
		pDoc->SetSciDocPtr(pSciDoc);
	}

	InitializeEditor();

	return 0;
}

void CDbgRemoteView::OnDestroy() 
{
	if (m_hwndScintilla)
	{
		::DestroyWindow(m_hwndScintilla);
		m_hwndScintilla = NULL;
		m_fnEditor = 0;
		m_ptrEditor = 0;
	}
	
	CView::OnDestroy();
}

void CDbgRemoteView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_hwndScintilla)
	{
		::MoveWindow(m_hwndScintilla, 0, 0, cx, cy, FALSE);
	}
}

void CDbgRemoteView::SelectLexer(const CString& filename)
{
	const char PythonKeyWords[] = 
		"and assert break class continue def del elif else except exec "
		"finally for from global if import in is lambda not or pass print "
		"raise return try while";

	const char HtmlKeyWords[] = 
		"";


	const char CKeyWords[] = 
		"asm auto bool break case catch char class const const_cast continue "
		"default delete do double dynamic_cast else enum explicit export extern false float for "
		"friend goto if inline int long mutable namespace new operator private protected public "
		"register reinterpret_cast return short signed sizeof static static_cast struct switch "
		"template this throw true try typedef typeid typename union unsigned using "
		"virtual void volatile wchar_t while "
		"import interface readonly attribute component";


	CString TempFilename = filename;
	TempFilename.MakeLower();
	if (TempFilename.Find(".py", 0) > -1)
	{
		SendEditor(SCI_SETLEXER, SCLEX_PYTHON, 0);
		m_docType = "Python";
		SendEditor(SCI_SETKEYWORDS, 0, 
			reinterpret_cast<LPARAM>(PythonKeyWords));

		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold.comment.python", (WPARAM)"0");
		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold.quotes.python", (WPARAM)"0");
		SendEditor(SCI_SETPROPERTY, (LPARAM)"tab.timmy.whinge.level", (WPARAM)"2");


	}
	else if ( (TempFilename.Find(".cpp", 0) > -1) ||
		      (TempFilename.Find(".c", 0) > -1) ||
			  (TempFilename.Find(".h", 0) > -1) ||
			  (TempFilename.Find(".idl", 0) > -1))
	{
		SendEditor(SCI_SETLEXER, SCLEX_CPP, 0);
		m_docType = "C/C++";
		SendEditor(SCI_SETKEYWORDS, 0, 
			reinterpret_cast<LPARAM>(CKeyWords));
	}
	else if (TempFilename.Find(".xml", 0) > -1)
	{
		SendEditor(SCI_SETLEXER, SCLEX_XML, 0);
		m_docType = "html";
		SendEditor(SCI_SETKEYWORDS, 0, 
			reinterpret_cast<LPARAM>(HtmlKeyWords));

		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold.html", (WPARAM)"1");
		
	}
	else if ((TempFilename.Find(".htm", 0) > -1))
	{
		SendEditor(SCI_SETLEXER, SCLEX_HTML, 0);
		m_docType = "html";
		SendEditor(SCI_SETKEYWORDS, 0, 
			reinterpret_cast<LPARAM>(HtmlKeyWords));
		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold.html", (WPARAM)"1");
	}
	else
	{
		SendEditor(SCI_SETLEXER, SCLEX_PYTHON, 0);
		m_docType = "Python";

		SendEditor(SCI_SETKEYWORDS, 0, 
			reinterpret_cast<LPARAM>(PythonKeyWords));
	}

	InitializeEditor();
}

void CDbgRemoteView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int firstVis = 0;
	int curPos = 0;
	int bookLine, breakLine;
	vector<int> bookmarks;
	vector<int> breakpoints;
	vector<int> foldlevels;

	switch (lHint)
	{
	case CDbgRemoteDoc::HINT_UPDATESTYLE:

		InitializeEditor();
		break;

	case CDbgRemoteDoc::HINT_DOC_REOPENED:
		//store the current position as well as breakpoint and bookmark
		//position indicators
		firstVis = SendEditor(SCI_GETFIRSTVISIBLELINE);
		curPos = SendEditor(SCI_GETCURRENTPOS);
		
		bookmarks.clear();
		bookLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
		while (bookLine > 0)
		{
			bookmarks.push_back(bookLine);
			bookLine = SendEditor(SCI_MARKERNEXT, ++bookLine, BOOKMARK_MASK);
		}

		breakpoints.clear();
		breakLine = SendEditor(SCI_MARKERNEXT, 0, BREAKPOINT_MASK);
		while (breakLine > 0)
		{
			breakpoints.push_back(breakLine);
			breakLine = SendEditor(SCI_MARKERNEXT, ++breakLine, BREAKPOINT_MASK);
		}
		for (int line = 0; line < SendEditor(SCI_GETLINECOUNT); ++line)
			foldlevels.push_back(SendEditor(SCI_GETFOLDEXPANDED, line));
		//intentional pass through



	case CDbgRemoteDoc::HINT_DOC_OPENED:
		{
			const char* pszInitText = reinterpret_cast<const char*>(pHint);
			
			if (pszInitText)
				SendEditor(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(pszInitText));
				
			if (m_hwndScintilla)
			{				
				SendEditor(SCI_SETREADONLY, 0);
				SendEditor(SCI_EMPTYUNDOBUFFER); // don't allow document open to be undone
				
				CString DocFilename = GetDocument()->GetFileName();
				SelectLexer(DocFilename);
				SendEditor(SCI_COLOURISE, 0, -1);
				SendEditor(SCI_SETSEL);
				SendEditor(SCI_SETREADONLY, GetDocument()->ReadOnly());
				
				if (pszInitText)
				{
					//the next few lines should help us straighten out some problems with files
					//that have inconsistant CRLF's
					SendEditor(SCI_CONVERTEOLS, SC_EOL_CRLF);
					if (SendEditor(SCI_GETMODIFY))
					{
						AfxGetMainWnd()->PostMessage(WM_COMMAND, MAKELONG(ID_FILE_SAVE, 1), 0);
					}
				}
			}
		}
		break;

	case CDbgRemoteDoc::HINT_DOC_SAVED:

		//store and restore the current position - cleared by the recoloring
		firstVis = SendEditor(SCI_GETFIRSTVISIBLELINE);
		curPos = SendEditor(SCI_GETCURRENTPOS);

		SendEditor(SCI_SETSAVEPOINT);
		
		CString DocFilename = GetDocument()->GetFileName();
		SelectLexer(DocFilename);
		SendEditor(SCI_COLOURISE, 0, -1);
		SendEditor(SCI_SETREADONLY, GetDocument()->ReadOnly());

		SendEditor(SCI_ENSUREVISIBLE, firstVis);
		SendEditor(SCI_GOTOLINE, firstVis);
		
		SendEditor(SCI_SETCURRENTPOS, curPos);
		SendEditor(SCI_SETSELECTIONSTART, curPos);
		SendEditor(SCI_SETSELECTIONEND, curPos);

		break;
	}





	if (lHint == CDbgRemoteDoc::HINT_DOC_REOPENED)
	{
		//reset the state
		SendEditor(SCI_ENSUREVISIBLE, firstVis);
		SendEditor(SCI_GOTOLINE, firstVis);
		//SendEditor(SCI_LINESCROLL, 0, firstVis-1);
		
		SendEditor(SCI_SETCURRENTPOS, curPos);
		SendEditor(SCI_SETSELECTIONSTART, curPos);
		SendEditor(SCI_SETSELECTIONEND, curPos);
				
		for (vector<int>::iterator itr = bookmarks.begin(); itr != bookmarks.end(); itr++)
		{
			SendEditor(SCI_MARKERADD, *itr, MARKER_BOOKMARK);
		}
		for (vector<int>::iterator itr = breakpoints.begin(); itr != breakpoints.end(); itr++)
		{
			SendEditor(SCI_MARKERADD, *itr, MARKER_BREAKPOINT);
		}

		for (unsigned int line = 0; line < foldlevels.size(); ++line)
			SendEditor(SCI_SETFOLDEXPANDED, line, foldlevels[line]);
	}
}

LRESULT CDbgRemoteView::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam) const
{
	assert(m_fnEditor);
	return m_fnEditor(m_ptrEditor, Msg, wParam, lParam);
}


void CDbgRemoteView::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void CDbgRemoteView::DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back) 
{
	SendEditor(SCI_MARKERDEFINE, marker, markerType);
	SendEditor(SCI_MARKERSETFORE, marker, fore);
	SendEditor(SCI_MARKERSETBACK, marker, back);
}




void CDbgRemoteView::InitializeEditor() 
{
	SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
	SendEditor(SCI_SETSTYLEBITS, 7);


	int nTextDefaultSize = 11;
	
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, RGB(0,0,0), RGB(255,255,255), nTextDefaultSize, "Courier");
	SendEditor(SCI_STYLECLEARALL);	// Copies global style to all others
	
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();


	

	int i = 0;
	const CUserCfg::SEditorStyle* pStyle = 0;
	do
	{
		pStyle = pCfg->GetEditorStyle(i++);
		if (pStyle && (strcmp(pStyle->Languagename, m_docType.c_str()) == 0))
		{
			
			SetAStyle(pStyle->StyleId, pStyle->ForeCol, pStyle->BackCol, pStyle->PointSize, pStyle->Facename);
			
			SendEditor(SCI_STYLESETBOLD, pStyle->StyleId, pStyle->Bold);
			SendEditor(SCI_STYLESETITALIC, pStyle->StyleId, pStyle->Italic);
			SendEditor(SCI_STYLESETUNDERLINE, pStyle->StyleId, pStyle->Underline);

			//the default val should be the first one
			if ((STYLE_DEFAULT == pStyle->StyleId) && (i == 0))
				SendEditor(SCI_STYLECLEARALL);
		}
		
	}while (pStyle);


	//set the properties for the marks margin (always visible)
	SendEditor(SCI_SETMARGINMASKN, MARGIN_MARKS,  SC_MASK_FOLDERS|MARKER_MASKS);
	
	SendEditor(SCI_SETMARGINWIDTHN, MARGIN_MARKS, m_nMarkerMarginWidth);
	SendEditor(SCI_SETMARGINSENSITIVEN, MARGIN_MARKS, 1);
	SendEditor(SCI_SETMARGINTYPEN, MARGIN_MARKS, SC_MARGIN_SYMBOL);
	
	//create the marker for the bookmark indicator: rounded rectangle, cyan on cyan
	DefineMarker(MARKER_BOOKMARK, SC_MARK_SMALLRECT, RGB(0,0,0), RGB(0,128,192));
	
	//create the marker for the breakpoint indicator: circle, red on red
	DefineMarker(MARKER_BREAKPOINT, SC_MARK_CIRCLE, RGB(128,64,64), RGB(128,64,64));
	
	//Create the marker for the current line indicator:arrow, yellow on yellow
	DefineMarker(MARKER_CURRENTLINE, SC_MARK_ARROW, RGB(255,255,0), RGB(255,255,0));

	//set the tab width to a reasonable value
	int nTabWidth = pCfg->GetEditorTabWidth();
	if ((nTabWidth < 1) || (nTabWidth > 20))
		nTabWidth = 4;

	SendEditor(SCI_SETTABWIDTH, nTabWidth, 0);

	//sets the indent size in spaces
	SendEditor(SCI_SETINDENT, nTabWidth, 0);

	//true means an indent is a mixure of tabs and spaces, false means that only 
	//spaces are used.
	SendEditor(SCI_SETUSETABS, pCfg->GetEditorUseTabs(), 0);

	//true means the tab key indents, false means it tabs
	SendEditor(SCI_SETTABINDENTS, false, 0);

	//set the caret color
	SendEditor(SCI_SETCARETFORE, pCfg->GetEditorCaretColor(), 0);
	
	SendEditor(SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD | SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
	
	DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, RGB(0, 0, 0), RGB(0, 0, 0));
	DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, RGB(0, 0, 0), RGB(0, 0, 0));


	//set the delay for tips...
	SendEditor(SCI_SETMOUSEDWELLTIME, 1000);

	//disable the built in popup menu - so that we can replace it with our own.
	SendEditor(SCI_USEPOPUP, 0);


} // InitializeEditor


void CDbgRemoteView::SetCurrentLine(int nLineNumber)
{
	SendEditor(SCI_MARKERDELETEALL, MARKER_CURRENTLINE, 0);
	if (nLineNumber > -1)
	{
		//the markers use a 0 based line number where python, and the 
		//line number display use a 1 based - go figure.
		nLineNumber -= 1;
		
		SendEditor(SCI_MARKERADD, nLineNumber, MARKER_CURRENTLINE);

		int NewPos = SendEditor(SCI_POSITIONFROMLINE, nLineNumber, 0);
		SendEditor(SCI_ENSUREVISIBLE, nLineNumber);
		SendEditor(SCI_SETCURRENTPOS, NewPos, 0);
		SendEditor(SCI_SETSELECTIONSTART, NewPos, 0);
		SendEditor(SCI_GOTOLINE, nLineNumber);
	}
}

int  CDbgRemoteView::GetCurrentLine()const
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos > -1)
	{
		int CurLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
		if (CurLine > -1)
			//convert to 1 based for display
			return CurLine+1;
	}
	return 0;
}

int CDbgRemoteView::GetCurrentCol()const
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos > -1)
	{
		int CurCol = SendEditor(SCI_GETCOLUMN, CurPos, 0);
		if (CurCol > -1)
			//convert to 1 based for display
			return CurCol+1;
	}
	return 0;
}

void CDbgRemoteView::SetBreakpoint(int nLineNumber, BOOL bSet)
{
	if (bSet)
		SendEditor(SCI_MARKERADD, nLineNumber, MARKER_BREAKPOINT);
	else
		SendEditor(SCI_MARKERDELETE, nLineNumber, MARKER_BREAKPOINT);
}

void CDbgRemoteView::ClearBreakpoints()
{
	SendEditor(SCI_MARKERDELETEALL, MARKER_BREAKPOINT, 0);
}

void CDbgRemoteView::OnBookmarksToggle() 
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos > -1)
	{
		int CurLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
		if (CurLine > -1)
		{
			int LineMarkers = SendEditor(SCI_MARKERGET, CurLine, 0);
			if (LineMarkers & BOOKMARK_MASK)
			{
				SendEditor(SCI_MARKERDELETE, CurLine, MARKER_BOOKMARK);
			}
			else
			{
				SendEditor(SCI_MARKERADD, CurLine, MARKER_BOOKMARK);
			}
		}
	}
}

void CDbgRemoteView::OnBookmarksGotonext() 
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	int CurLine = 0;
	if (CurPos > -1)
		CurLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
	if (CurLine < 0)
		CurLine = 0;

	int NextMarkLine = SendEditor(SCI_MARKERNEXT, CurLine+1, BOOKMARK_MASK);
	if ((NextMarkLine < 0) && (CurLine > 0))
		NextMarkLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
	
	if (NextMarkLine > -1)
	{
		int NewPos = SendEditor(SCI_POSITIONFROMLINE, NextMarkLine, 0);
		SendEditor(SCI_SETCURRENTPOS, NewPos, 0);
		SendEditor(SCI_SETSELECTIONSTART, NewPos, 0);
		SendEditor(SCI_ENSUREVISIBLE, NextMarkLine);
		SendEditor(SCI_GOTOLINE, NextMarkLine);
	}
}

void CDbgRemoteView::OnUpdateBookmarksGotonext(CCmdUI* pCmdUI) 
{
	int NextMarkLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
	pCmdUI->Enable(NextMarkLine > -1);
}

void CDbgRemoteView::OnBookmarksGotoprevious() 
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	int CurLine = 0;
	if (CurPos > -1)
		CurLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
	if (CurLine < 0)
		CurLine = 0;

	int PrevMarkLine = SendEditor(SCI_MARKERPREVIOUS, CurLine-1, BOOKMARK_MASK);
	
	if (PrevMarkLine < 0)
	{
		int MaxLine = SendEditor(SCI_GETLINECOUNT, 0, 0);
		PrevMarkLine = SendEditor(SCI_MARKERPREVIOUS, MaxLine, BOOKMARK_MASK);
	}

	if (PrevMarkLine > -1)
	{
		int NewPos = SendEditor(SCI_POSITIONFROMLINE, PrevMarkLine, 0);
		SendEditor(SCI_SETCURRENTPOS, NewPos, 0);
		SendEditor(SCI_SETSELECTIONSTART, NewPos, 0);
		SendEditor(SCI_ENSUREVISIBLE, PrevMarkLine);
		SendEditor(SCI_GOTOLINE, PrevMarkLine);
	}
}

void CDbgRemoteView::OnUpdateBookmarksGotoprevious(CCmdUI* pCmdUI) 
{
	int NextMarkLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
	pCmdUI->Enable(NextMarkLine > -1);
}

void CDbgRemoteView::OnBookmarksClearall() 
{
	SendEditor(SCI_MARKERDELETEALL, MARKER_BOOKMARK, 0);
}

void CDbgRemoteView::OnUpdateBookmarksClearall(CCmdUI* pCmdUI) 
{
	int NextMarkLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
	pCmdUI->Enable(NextMarkLine > -1);
}

void CDbgRemoteView::OnDraw(CDC* pDC) 
{	
}

void CDbgRemoteView::OnDebugBreakpointsToggle() 
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos > -1)
	{
		int CurLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
		if (CurLine > -1)
		{
			CDbgRemoteDoc* pDoc = GetDocument();
			pDoc->ToggleBreakpoint(CurLine);


			int LineMarkers = SendEditor(SCI_MARKERGET, CurLine, 0);
			if (LineMarkers & BREAKPOINT_MASK)
			{
				SendEditor(SCI_MARKERDELETE, CurLine, MARKER_BREAKPOINT);
			}
			else
			{
				SendEditor(SCI_MARKERADD, CurLine, MARKER_BREAKPOINT);
			}
		}
	}
}

BOOL CDbgRemoteView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pnmh = reinterpret_cast<NMHDR*>(lParam);

	if (pnmh->hwndFrom == m_hwndScintilla)
	{
		SCNotification* pScinNM = reinterpret_cast<SCNotification*>(pnmh);

		switch(pnmh->code)
		{
		case SCN_MODIFYATTEMPTRO:
			OnAttemptModify();
			break;

		case SCN_MODIFIED:

			if (pScinNM->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
			{
				if (!GetDocument()->IsModified())
				{
					GetDocument()->SetModifiedFlag();
				}
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
				break;
			}
		
		case SCN_CHARADDED:

			if ('\n' == pScinNM->ch)
			{
				AutoIndent();
			}
			else if ('.' == pScinNM->ch)
			{
				AutoComplete();
			}
			else if ('(' == pScinNM->ch)
			{
				AutoHint();
			}
			else
				MatchBraces();

			break;
		
		case SCN_UPDATEUI:

			MatchBraces();
			break;

		case SCN_SAVEPOINTREACHED:

			GetDocument()->SetModifiedFlag(false);
			break;

		case SCN_SAVEPOINTLEFT:

			GetDocument()->SetModifiedFlag();
			break;

		case SCN_MARGINCLICK:
			{
				int lineClick = SendEditor(SCI_LINEFROMPOSITION, pScinNM->position);

				if (m_bShowSelectiveDisplay)
				{
					FoldLine(lineClick, pScinNM->modifiers);
				}
			}
			break;

		case SCN_DWELLSTART:
			{
				StartAutoWatch();
			}
			break;
		case SCN_DWELLEND:
			{
				EndAutoWatch();
			}
			break;
		}
	}	
	
	return CView::OnNotify(wParam, lParam, pResult);
}

void CDbgRemoteView::FoldLine(int nLine, int Modifiers/* = 0*/)
{
	int levelClick = SendEditor(SCI_GETFOLDLEVEL, nLine);
	if (levelClick & SC_FOLDLEVELHEADERFLAG) 
	{
		if (Modifiers & SCMOD_SHIFT) 
		{
			// Ensure all children visible
			SendEditor(SCI_SETFOLDEXPANDED, nLine, 1);
			Expand(nLine, true, true, 100, levelClick);
		} 
		else if (Modifiers & SCMOD_CTRL) 
		{
			if (SendEditor(SCI_GETFOLDEXPANDED, nLine)) 
			{
				// Contract this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, nLine, 0);
				Expand(nLine, false, true, 0, levelClick);
			} 
			else 
			{
				// Expand this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, nLine, 1);
				Expand(nLine, true, true, 100, levelClick);
			}
		} 
		else
		{
			// Toggle this line
			SendEditor(SCI_TOGGLEFOLD, nLine);
		}
	}
}

void CDbgRemoteView::AutoIndent()
{
	vector<char> linebuff;
	
	int nCurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	int nCurLine = SendEditor(SCI_LINEFROMPOSITION, nCurPos);

	int  lineLength  =  SendEditor(SCI_LINELENGTH,  nCurLine);
	linebuff.resize(lineLength+2);

	if ( (nCurLine > 0) /*&& (lineLength <= 2)*/ )
	{
		int prevLineLength = SendEditor(SCI_LINELENGTH, nCurLine - 1);
		if (prevLineLength > lineLength)
			linebuff.resize(prevLineLength + 2);

			
		WORD buflen = linebuff.size();
		memcpy(&(linebuff[0]),  &buflen,  sizeof(buflen));
		SendEditor(SCI_GETLINE,  nCurLine - 1,reinterpret_cast<LPARAM>(&(linebuff[0])));
		linebuff[prevLineLength]  =  '\0';

		BOOL bAddExtraTab = ((linebuff[prevLineLength-2] == ':') || (linebuff[prevLineLength-3] == ':'));
		
		int pos = 0;
		BOOL bTabsOnly = TRUE;
		for (pos = 0; linebuff[pos]; pos++)  
		{
			if ((linebuff[pos] != ' ') && (linebuff[pos] != '\t'))
			{
				if ((linebuff[pos] != '\r') && (linebuff[pos] != '\n'))
					bTabsOnly = FALSE;
				
				linebuff[pos] = '\0';
				
				break;
			}
		}

		if (bAddExtraTab)
		{
			//the previous line ends with the : character indicating a block, add one level of indent
			linebuff[pos] = '\t';
			linebuff[pos+1] = '\0';
		}
		
		SendEditor(SCI_REPLACESEL,  0,  reinterpret_cast<LPARAM>(&(linebuff[0])));
	
	}
}

void CDbgRemoteView::OnEditCopy() 
{
	SendEditor(SCI_COPY, 0, 0);
}

void CDbgRemoteView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
}

void CDbgRemoteView::OnEditCut() 
{
	OnAttemptModify();
	SendEditor(SCI_CUT, 0, 0);
}

void CDbgRemoteView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
}

void CDbgRemoteView::OnEditPaste() 
{
	OnAttemptModify();
	SendEditor(SCI_PASTE, 0, 0);	
}

void CDbgRemoteView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(SendEditor(SCI_CANPASTE, 0, 0) != 0);
}

void CDbgRemoteView::OnEditUndo() 
{
	SendEditor(SCI_UNDO, 0, 0);
}

void CDbgRemoteView::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(SendEditor(SCI_CANUNDO, 0, 0) != 0);
}

void CDbgRemoteView::OnEditRedo() 
{
	SendEditor(SCI_REDO, 0, 0);	
}

void CDbgRemoteView::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(SendEditor(SCI_CANREDO, 0, 0) != 0);
}

string CDbgRemoteView::GetCurrentText()
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos < 0)
		CurPos = 0;
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	int SelStart = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	if (SelEnd == SelStart)
	{
		//find the start and end of the current word
		GetCurrentWordPos(SelStart, SelEnd);
	}

	if (SelStart > SelEnd)
	{
		SelEnd = SelStart;
	}

	TEXTRANGE tr;
	tr.chrg.cpMin = SelStart;
	tr.chrg.cpMax = SelEnd;
	vector<char> vecText;
	vecText.resize(SelEnd - SelStart+1, 0);
	tr.lpstrText = &vecText[0];
	
	SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
	return string(tr.lpstrText);

}





void CDbgRemoteView::OnEditFind() 
{
	int CurPos = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	if (CurPos < 0)
		CurPos = 0;
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	if (SelEnd == CurPos)
	{
		//find the start and end of the current word
		GetCurrentWordPos(CurPos, SelEnd);
	}

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


void CDbgRemoteView::OnEditFindnext() 
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

void CDbgRemoteView::OnEditReplace() 
{
	string Text = GetCurrentText();

	CTextReplaceDlg dlg;
	dlg.m_strReplaceWhat = Text.c_str();
	dlg.m_hScintillaHwnd = m_hwndScintilla;
	dlg.DoModal();
}



void CDbgRemoteView::GetEditorText(char_vector& editorData)
{
	int dataSize = SendEditor(SCI_GETTEXTLENGTH, 0, 0);
	
	editorData.resize(dataSize + 1);
	editorData[dataSize] = '\0';
	
	if (dataSize)
	{
		SendEditor(SCI_GETTEXT, dataSize + 1, reinterpret_cast<LPARAM>(&editorData[0]));
	}
}

void CDbgRemoteView::GetCurrentWordPos(int& CurStart, int& CurEnd)
{
	int nLineNum = SendEditor(SCI_LINEFROMPOSITION, CurStart);
	if (nLineNum < 0)
		return;

	int nLineLen = SendEditor(SCI_LINELENGTH, nLineNum);
	if (nLineLen <= 1)
		return;
	
	int nLineEndPos = SendEditor(SCI_GETLINEENDPOSITION, nLineNum);
	int nLineStartPos = nLineEndPos - nLineLen;
	
	//const char* cszDelimiters = " .,!()[]:=\t\"\n\r";
	const char* cszDelimiters = " ,!()[]:=\t\"\n\r";

	while ((CurStart > 0) && (CurStart-1 >= nLineStartPos))
	{
		char CurChar = static_cast<char>(SendEditor(SCI_GETCHARAT, CurStart-1));
		if (strchr(cszDelimiters, CurChar))
			break;
		CurStart--;
	}

	cszDelimiters = " .,!()[]:=\t\"\n\r";

	while (CurEnd < nLineEndPos)
	{
		char CurChar = static_cast<char>(SendEditor(SCI_GETCHARAT, CurEnd));
		if (strchr(cszDelimiters, CurChar))
			break;
		CurEnd++;
	}

}//GetCurrentWord

BOOL CDbgRemoteView::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	if (nStartPos == nEndPos)
	{
		//find the start and end of the current word
		GetCurrentWordPos(nStartPos, nEndPos);
	}

#ifdef _PLATFORM_SDK
	if (nEndPos > nStartPos)
	{
		TEXTRANGE tr;
		tr.chrg.cpMin = nStartPos;
		tr.chrg.cpMax = nEndPos;
		vector<char> vecText;
		vecText.resize(nEndPos - nStartPos+1, 0);
		tr.lpstrText = &vecText[0];
		
		SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);


		HH_AKLINK hhData;
		hhData.cbStruct = sizeof(HH_AKLINK);
		hhData.fReserved = 0;
		hhData.pszKeywords = tr.lpstrText;
		hhData.pszUrl = "";       
		hhData.pszMsgText = "Unable to locate selected word";            
		hhData.pszMsgTitle = "No help found"; 
		hhData.pszWindow = ""; 
		hhData.fIndexOnFail = 1;     


		const char* cszHelpFile = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetPythonHelpFile();
		if (strlen(cszHelpFile) > 0)
		{
			CString HelpPath;
				
			//if the help file has just a relative path, get the module path to make a full path
			if (strchr(cszHelpFile, ':') == 0)
			{
				char* pBuff = HelpPath.GetBufferSetLength(_MAX_PATH);
				if (GetModuleFileName(0, pBuff, _MAX_PATH) > 0)
				{
					char* slash = strrchr(pBuff, '\\');
					if (slash)
						slash[1] = 0;
				}
				HelpPath.ReleaseBuffer();
			}
			
			HelpPath += cszHelpFile;
			::HtmlHelp(0, HelpPath, HH_KEYWORD_LOOKUP, (DWORD)&hhData);
		
		}
	}
#else
	((CDbgRemoteApp*)AfxGetApp())->OnHelpPythonHelp();

#endif


	return CView::OnHelpInfo(pHelpInfo);
}

void CDbgRemoteView::OnViewWhitespace() 
{
	if (SendEditor(SCI_GETVIEWWS, 0, 0) == SCWS_INVISIBLE)
		SendEditor(SCI_SETVIEWWS, SCWS_VISIBLEALWAYS, 0);
	else
		SendEditor(SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
}

void CDbgRemoteView::OnUpdateViewWhitespace(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(SendEditor(SCI_GETVIEWWS, 0, 0) != SCWS_INVISIBLE);
}

void CDbgRemoteView::OnViewSelectivedisplay() 
{
	if (m_bShowSelectiveDisplay)
	{
		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold", (WPARAM)"0");
		//SendEditor(SCI_SETFOLDFLAGS, 0);
		FoldAll();
	}
	else
	{
		SendEditor(SCI_SETPROPERTY, (LPARAM)"fold", (WPARAM)"1");
		//SendEditor(SCI_SETFOLDFLAGS, FOLDFLAG_ABOVEEXPAND|FOLDFLAG_BOLOWNOTEXP);
	}
	SendEditor(SCI_CLEARDOCUMENTSTYLE);
	SendEditor(SCI_COLOURISE, 0, -1);
	m_bShowSelectiveDisplay = !m_bShowSelectiveDisplay;
}

void CDbgRemoteView::OnUpdateViewSelectivedisplay(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bShowSelectiveDisplay);
}

void CDbgRemoteView::OnViewLinenumbers() 
{
	m_bShowLinenumbers = !m_bShowLinenumbers;
	SendEditor(SCI_SETMARGINWIDTHN, MARGIN_LINENUMBERS, m_bShowLinenumbers ? m_nLineNumberMarginWidth : 0);	
}

void CDbgRemoteView::OnUpdateViewLinenumbers(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bShowLinenumbers);
}


void CDbgRemoteView::OnEditGotoline() 
{
	CGotoLineDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		ShowLine(dlg.m_LineNumber);
	}
}

void CDbgRemoteView::ShowLine(int nLineNumber)
{
	//our (and pythons) line numbers are 1 based, scintilla's are 0 based
	nLineNumber--;
	
	int NewPos = SendEditor(SCI_POSITIONFROMLINE, nLineNumber, 0);
	SendEditor(SCI_SETCURRENTPOS, NewPos, 0);
	SendEditor(SCI_SETSELECTIONSTART, NewPos, 0);
	SendEditor(SCI_GOTOLINE, nLineNumber);
	::SetFocus(m_hwndScintilla);
}

void CDbgRemoteView::OnViewViewlineendings() 
{
	int i = SendEditor(SCI_GETVIEWEOL, 0, 0);

	SendEditor(SCI_SETVIEWEOL, (LPARAM) (i == 0), 0);
}

void CDbgRemoteView::OnUpdateViewViewlineendings(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(SendEditor(SCI_GETVIEWEOL, 0, 0) != 0);
}

void CDbgRemoteView::OnDebugAddtoWatch() 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	
	if (nStartPos == nEndPos)
	{
		//find the start and end of the current word
		GetCurrentWordPos(nStartPos, nEndPos);
	}
	
	if (nEndPos > nStartPos)
	{
		TEXTRANGE tr;
		tr.chrg.cpMin = nStartPos;
		tr.chrg.cpMax = nEndPos;
		tr.lpstrText = new char[nEndPos - nStartPos+1];
		
		SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
		
		((CDbgRemoteApp*)AfxGetApp())->AddWatchItem(tr.lpstrText);
	}
}


void CDbgRemoteView::OnSpacesToTabs() 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos   = SendEditor(SCI_GETSELECTIONEND, 0, 0);

	if (nEndPos < nStartPos)
	{
		int temp = nStartPos;
		nStartPos = nEndPos;
		nEndPos = temp;
	}

	SendEditor(SCI_BEGINUNDOACTION, 0, 0);

	char_vector m_LineBuff;

	int SpacesPerTab = SendEditor(SCI_GETTABWIDTH, 0, 0);

	int nLineStart = SendEditor(SCI_LINEFROMPOSITION, nStartPos, 0);
	int nLineEnd   = SendEditor(SCI_LINEFROMPOSITION, nEndPos, 0);

	for (int nCurLine = nLineStart; nCurLine <= nLineEnd; nCurLine++)
	{
		int LineStartPos = SendEditor(SCI_POSITIONFROMLINE, nCurLine, 0);
		int LineEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nCurLine, 0);

		int LineLen = LineEndPos - LineStartPos;
		if (LineLen > SpacesPerTab)
		{
			SendEditor(SCI_SETSEL, LineStartPos, LineEndPos);

			m_LineBuff.resize(LineEndPos - LineStartPos + 1);
			SendEditor(SCI_GETSELTEXT, 0, (LPARAM)&m_LineBuff[0]);
			
			//count leading spaces
			int SpaceCount = 0;
         int TabCount = 0;                         // Added 10-15-02 Bruce Dickey
         int BufIdx = 0;                           // Added 10-15-02 Bruce Dickey

			//while (m_LineBuff[SpaceCount] == ' ')
			while (    (m_LineBuff[BufIdx] == ' ')    // space
                 || (m_LineBuff[BufIdx] == '.')    // space when view whitespace is on
                 || (m_LineBuff[BufIdx] == '\t')   // tab
               )
         {
            //SpaceCount++;
            if (   (m_LineBuff[BufIdx] == ' ')
                || (m_LineBuff[BufIdx] == '.')    // space when view whitespace is on
               )
            {
               SpaceCount++;
            }

            if (m_LineBuff[BufIdx] == '\t')
            {
               TabCount++;
            }

            BufIdx++;
         }

			//int TabCount = SpaceCount / SpacesPerTab;

         int OrigTabCount = TabCount;
			TabCount += SpaceCount / SpacesPerTab;    // Changed 10-15-02 Bruce Dickey

			if (TabCount > 0)
			{
            // Changed 10-15-02 Bruce Dickey
				//m_LineBuff.erase (m_LineBuff.begin(), m_LineBuff.begin() + (TabCount * SpacesPerTab) );
				m_LineBuff.erase (m_LineBuff.begin(), m_LineBuff.begin() + (OrigTabCount + SpaceCount) );
				m_LineBuff.insert(m_LineBuff.begin(), TabCount, '\t');
				
				SendEditor(SCI_REPLACESEL, 0, (LPARAM)&m_LineBuff[0]);
			}
		}
	}

	//end the 'batch' undo
	SendEditor(SCI_ENDUNDOACTION, 0, 0);

	//reset selection
	nStartPos = SendEditor(SCI_POSITIONFROMLINE, nLineStart, 0);
	nEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nLineEnd, 0);
	
	SendEditor(SCI_SETSEL, nStartPos, nEndPos);
}


// Added 10-15-02 Bruce Dickey
void CDbgRemoteView::OnTabsToSpaces() 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos   = SendEditor(SCI_GETSELECTIONEND, 0, 0);

	if (nEndPos < nStartPos)
	{
		int temp = nStartPos;
		nStartPos = nEndPos;
		nEndPos = temp;
	}

	SendEditor(SCI_BEGINUNDOACTION, 0, 0);

	char_vector m_LineBuff;

	int SpacesPerTab = SendEditor(SCI_GETTABWIDTH, 0, 0);

	int nLineStart = SendEditor(SCI_LINEFROMPOSITION, nStartPos, 0);
	int nLineEnd   = SendEditor(SCI_LINEFROMPOSITION, nEndPos, 0);

	for (int nCurLine = nLineStart; nCurLine <= nLineEnd; nCurLine++)
	{
		int LineStartPos = SendEditor(SCI_POSITIONFROMLINE, nCurLine, 0);
		int LineEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nCurLine, 0);

		int LineLen = LineEndPos - LineStartPos;
		if (LineLen > SpacesPerTab)
		{
			SendEditor(SCI_SETSEL, LineStartPos, LineEndPos);

			m_LineBuff.resize(LineEndPos - LineStartPos + 1);
			SendEditor(SCI_GETSELTEXT, 0, (LPARAM)&m_LineBuff[0]);
			
			//count leading spaces
			int SpaceCount = 0;
         int TabCount = 0;                         // Added 10-15-02 Bruce Dickey
         int BufIdx = 0;                           // Added 10-15-02 Bruce Dickey

			//while (m_LineBuff[SpaceCount] == ' ')
			while (    (m_LineBuff[BufIdx] == ' ')    // space
                 || (m_LineBuff[BufIdx] == '.')    // space when view whitespace is on
                 || (m_LineBuff[BufIdx] == '\t')   // tab
               )
         {
            //SpaceCount++;
            if (   (m_LineBuff[BufIdx] == ' ')
                || (m_LineBuff[BufIdx] == '.')    // space when view whitespace is on
               )
            {
               SpaceCount++;
            }

            if (m_LineBuff[BufIdx] == '\t')
            {
               TabCount++;
            }

            BufIdx++;
         }

			//int TabCount = SpaceCount / SpacesPerTab;

         int OrigSpaceCount = SpaceCount;
			SpaceCount += TabCount * SpacesPerTab;    // Changed 10-15-02 Bruce Dickey

			if (TabCount > 0)
			{
            // Changed 10-15-02 Bruce Dickey
				//m_LineBuff.erase (m_LineBuff.begin(), m_LineBuff.begin() + (TabCount * SpacesPerTab) );
				m_LineBuff.erase (m_LineBuff.begin(), m_LineBuff.begin() + (TabCount + OrigSpaceCount) );
				m_LineBuff.insert(m_LineBuff.begin(), SpaceCount, ' ');
				
				SendEditor(SCI_REPLACESEL, 0, (LPARAM)&m_LineBuff[0]);
			}
		}
	}

	//end the 'batch' undo
	SendEditor(SCI_ENDUNDOACTION, 0, 0);

	//reset selection
	nStartPos = SendEditor(SCI_POSITIONFROMLINE, nLineStart, 0);
	nEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nLineEnd, 0);
	
	SendEditor(SCI_SETSEL, nStartPos, nEndPos);
}


// Added 10-16-02 Bruce Dickey
void CDbgRemoteView::OnIndent() 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos   = SendEditor(SCI_GETSELECTIONEND, 0, 0);

	if (nEndPos < nStartPos)
	{
		int temp = nStartPos;
		nStartPos = nEndPos;
		nEndPos = temp;
	}

	SendEditor(SCI_BEGINUNDOACTION, 0, 0);

	int nLineStart = SendEditor(SCI_LINEFROMPOSITION, nStartPos, 0);
	int nLineEnd   = SendEditor(SCI_LINEFROMPOSITION, nEndPos, 0);

	for (int nCurLine = nLineStart; nCurLine <= nLineEnd; nCurLine++)
	{
   	nStartPos = SendEditor(SCI_POSITIONFROMLINE,   nCurLine, 0);
	   nEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nCurLine, 0);
	
   	SendEditor(SCI_SETSEL, nStartPos, nStartPos);
      SendEditor(SCI_TAB, 0, 0);
	}

	//end the 'batch' undo
	SendEditor(SCI_ENDUNDOACTION, 0, 0);

	//reset selection
	nStartPos = SendEditor(SCI_POSITIONFROMLINE, nLineStart, 0);
	nEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nLineEnd, 0);
	
	SendEditor(SCI_SETSEL, nStartPos, nStartPos);
}


// Added 10-16-02 Bruce Dickey
void CDbgRemoteView::OnUnindent() 
{
	int nStartPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int nEndPos   = SendEditor(SCI_GETSELECTIONEND, 0, 0);

	if (nEndPos < nStartPos)
	{
		int temp = nStartPos;
		nStartPos = nEndPos;
		nEndPos = temp;
	}

	SendEditor(SCI_BEGINUNDOACTION, 0, 0);

	char_vector m_LineBuff;

	int nLineStart = SendEditor(SCI_LINEFROMPOSITION, nStartPos, 0);
	int nLineEnd   = SendEditor(SCI_LINEFROMPOSITION, nEndPos, 0);

	for (int nCurLine = nLineStart; nCurLine <= nLineEnd; nCurLine++)
	{
		int LineStartPos = SendEditor(SCI_POSITIONFROMLINE, nCurLine, 0);
		int LineEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nCurLine, 0);

		SendEditor(SCI_SETSEL, LineStartPos, LineEndPos);

		m_LineBuff.resize(LineEndPos - LineStartPos + 1);
		SendEditor(SCI_GETSELTEXT, 0, (LPARAM)&m_LineBuff[0]);
		
		// Set position to end of last leading whitespace char
		int nBufPos = 0;

		while (    (m_LineBuff[nBufPos] == ' ')    // space
              || (m_LineBuff[nBufPos] == '.')    // space when view whitespace is on
              || (m_LineBuff[nBufPos] == '\t')   // tab
            )
      {
         nBufPos++;
      }

      nBufPos += LineStartPos;

   	SendEditor(SCI_SETSEL, nBufPos, nBufPos);
      SendEditor(SCI_DELETEBACKNOTLINE, 0, 0);
	}

	//end the 'batch' undo
	SendEditor(SCI_ENDUNDOACTION, 0, 0);

	//reset selection
	nStartPos = SendEditor(SCI_POSITIONFROMLINE, nLineStart, 0);
	nEndPos   = SendEditor(SCI_GETLINEENDPOSITION, nLineEnd, 0);
	
	SendEditor(SCI_SETSEL, nStartPos, nStartPos);
}

void CDbgRemoteView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	//dprintf("SetFocus: \n");	

	::SetFocus(m_hwndScintilla);
}




void CDbgRemoteView::Expand(int &line, bool doExpand, bool force, int visLevels, int level) 
{
	int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK);
	line++;
	while (line <= lineMaxSubord) {
		if (force) {
			if (visLevels > 0)
				SendEditor(SCI_SHOWLINES, line, line);
			else
				SendEditor(SCI_HIDELINES, line, line);
		} else {
			if (doExpand)
				SendEditor(SCI_SHOWLINES, line, line);
		}
		int levelLine = level;
		if (levelLine == -1)
			levelLine = SendEditor(SCI_GETFOLDLEVEL, line);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			if (force) {
				if (visLevels > 1)
					SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				else
					SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				Expand(line, doExpand, force, visLevels - 1);
			} else {
				if (doExpand) {
					if (!SendEditor(SCI_GETFOLDEXPANDED, line))
						SendEditor(SCI_SETFOLDEXPANDED, line, 1);
					Expand(line, true, force, visLevels - 1);
				} else {
					Expand(line, false, force, visLevels - 1);
				}
			}
		} else {
			line++;
		}
	}
}

void CDbgRemoteView::FoldAll(bool bFold) 
{
	SendEditor(SCI_COLOURISE, 0, -1);
	int maxLine = SendEditor(SCI_GETLINECOUNT);
	bool expanding = !bFold;
	/*for (int lineSeek = 0; lineSeek < maxLine; lineSeek++) {
		if (SendEditor(SCI_GETFOLDLEVEL, lineSeek) & SC_FOLDLEVELHEADERFLAG) {
			expanding = !SendEditor(SCI_GETFOLDEXPANDED, lineSeek);
			break;
		}
	}*/
	for (int line = 0; line < maxLine; line++) {
		int level = SendEditor(SCI_GETFOLDLEVEL, line);
		if ((level & SC_FOLDLEVELHEADERFLAG) &&
		        (SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
			if (expanding) {
				SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				Expand(line, true, false, 0, level);
				line--;
			} else {
				int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, -1);
				SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				if (lineMaxSubord > line)
					SendEditor(SCI_HIDELINES, line + 1, lineMaxSubord);
			}
		}
	}
}

void CDbgRemoteView::MatchBraces()
{
	static bool bHighlightOn = false;

	int nCurPos = SendEditor(SCI_GETCURRENTPOS);

	if (nCurPos > 0)
	{
		char ch = static_cast<char>(SendEditor(SCI_GETCHARAT, nCurPos - 1));

		switch (ch)
		{
		case '(':	case ')':
		case '[':	case ']':
		case '{':	case '}':
		case '<':	case '>':
			{
				bHighlightOn = true;

				int MatchPos = SendEditor(SCI_BRACEMATCH, nCurPos - 1);
				if (MatchPos != INVALID_POSITION)
					SendEditor(SCI_BRACEHIGHLIGHT, nCurPos - 1, MatchPos);
				else
					SendEditor(SCI_BRACEBADLIGHT, nCurPos - 1);
			}
			break;

		default:

			if (bHighlightOn)
			{
				SendEditor(SCI_BRACEHIGHLIGHT, INVALID_POSITION, INVALID_POSITION);
				bHighlightOn = FALSE;
			}

			break;
		}
	}
}


void CDbgRemoteView::OnViewEOLTypeCRLF() 
{
	SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
	SendEditor(SCI_CONVERTEOLS, SC_EOL_CRLF);
}

void CDbgRemoteView::OnUpdateViewEOLTypeCRLF(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(SendEditor(SCI_GETEOLMODE) == SC_EOL_CRLF);
}

void CDbgRemoteView::OnViewEOLTypeCR() 
{
	SendEditor(SCI_SETEOLMODE, SC_EOL_CR);
	SendEditor(SCI_CONVERTEOLS, SC_EOL_CR);
}

void CDbgRemoteView::OnUpdateViewEOLTypeCR(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(SendEditor(SCI_GETEOLMODE) == SC_EOL_CR);
}

void CDbgRemoteView::OnViewEOLTypeLF() 
{
	SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
	SendEditor(SCI_CONVERTEOLS, SC_EOL_LF);
}

void CDbgRemoteView::OnUpdateViewEOLTypeLF(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(SendEditor(SCI_GETEOLMODE) == SC_EOL_LF);
}


void CDbgRemoteView::OnEditUncommentline() 
{
	SendEditor(SCI_BEGINUNDOACTION, 0, 0);
	
	int CurPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int StartLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	int EndLine = SendEditor(SCI_LINEFROMPOSITION, SelEnd, 0);
	if (EndLine < StartLine)
	{
		int a = EndLine;
		EndLine = StartLine;
		StartLine = a;
	}
		
	for (int i = StartLine; i <= EndLine; i++)
	{
		int CurPos = SendEditor(SCI_POSITIONFROMLINE, i, 0);
		int LineEndPos = SendEditor(SCI_GETLINEENDPOSITION, i, 0);
		while (CurPos < LineEndPos)
		{
			char CurChar = static_cast<char>(SendEditor(SCI_GETCHARAT, CurPos));
			if ( (CurChar != ' ') && (CurChar != '\t') && (CurChar != '#'))
				break;
			if (CurChar == '#')
			{
				SendEditor(SCI_SETSEL, CurPos, CurPos+1);		
				SendEditor(SCI_REPLACESEL, 0, (LPARAM)"");
				// Break so that exisiting comments that have been commented out are
				// not also un-commented.
				break;      // Added 10-21-02 Bruce Dickey
			}
			CurPos++;

		}
	}
	SendEditor(SCI_ENDUNDOACTION, 0, 0);
}

void CDbgRemoteView::OnUpdateEditUncommentline(CCmdUI* pCmdUI) 
{
	
}

void CDbgRemoteView::OnEditCommentline() 
{
	SendEditor(SCI_BEGINUNDOACTION, 0, 0);
	int CurPos = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	int StartLine = SendEditor(SCI_LINEFROMPOSITION, CurPos, 0);
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	int EndLine = SendEditor(SCI_LINEFROMPOSITION, SelEnd, 0);
	if (EndLine < StartLine)
	{
		int a = EndLine;
		EndLine = StartLine;
		StartLine = a;
	}
	for (int i = StartLine; i <= EndLine; i++)
	{
		int CurPos = SendEditor(SCI_POSITIONFROMLINE, i, 0);
		SendEditor(SCI_INSERTTEXT, CurPos, (LONG)"#");
	}
	SendEditor(SCI_ENDUNDOACTION, 0, 0);
}

void CDbgRemoteView::OnUpdateEditCommentline(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

BOOL CDbgRemoteView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	
}




int CountTabs(const char* cszStr)
{
	assert(cszStr);

	int count = 0;
	while (*cszStr == '\t')
	{
		++count;
		++cszStr;
	}
	return count;
}

void TrimLine(char* szStr)
{
	for (size_t j = 0; j < strlen(szStr); j++)
	{
		if ( ( ! isprint(szStr[j])) && (szStr[j] != '\t'))
		{
			szStr[j] = 0;
			break;
		}
	}
}

//#include <python.h>
//#include <ceval.h>

void CDbgRemoteView::AutoComplete()
{
#if 0 // this stuff just isn't ready for primetime
	dprintf("AutoComplete:\n");
	//initialize the python engine
	Py_Initialize();


	//create a dictionary to use for our "locals" and add the builtins to it
	PyObject* pDict = PyDict_New();

	PyObject* pBuil = PyEval_GetBuiltins();


	PyDict_SetItemString(pDict, "__builtins__", pBuil);
	//PyDict_SetItemString(pDict, "__name__", PyString_FromString("__main__") );

	//go through the file and add any imports declarations to the main workspace
	int LineCount = SendEditor(SCI_GETLINECOUNT);
	char_vector vecLine;
	
	for (int i = 0; i < LineCount; i++)
	{
		vecLine.resize(SendEditor(SCI_LINELENGTH, i)+1);
		SendEditor(SCI_GETLINE, i, (LPARAM)&(vecLine[0]));
		
		TrimLine(&(vecLine[0]));

		string line = &(vecLine[0]);

		if (strstr(&(vecLine[0]), "import"))
		{
			int tabInx = 0; 
			while (vecLine[tabInx] && vecLine[tabInx] == '\t')
				tabInx++;

			line = &(vecLine[tabInx]);

			dprintf("Running (import): %s\n", line.c_str());
			//PyRun_String((char*)line.c_str(), Py_file_input, pDict, pDict);
			PyRun_String((char*)line.c_str(), Py_file_input, pDict, pDict);
			//PyObject* pObj = PyRun_String((char*)line.c_str(), Py_file_input, pDict, pDict);
			//if (pObj)
			//	Py_XDECREF(pObj);
		}
/*		else if (strstr(&(vecLine[0]), "class"))
		{
			int DefIndent = CountTabs(&(vecLine[0]));
			line += "\n";
			while (++i < LineCount)
			{
				vecLine.resize(SendEditor(SCI_LINELENGTH, i)+1);
				SendEditor(SCI_GETLINE, i, (LPARAM)&(vecLine[0]));
				
				if (strstr(&(vecLine[0]), "def"))
				{
					TrimLine(&(vecLine[0]));
					line += &(vecLine[0]);
					line += "\n";
					
					for (int j = 0; j <= DefIndent+1; j++)
						line += "\t";
					line += "pass\n";
					
				}
				int nextIndent = CountTabs(&(vecLine[0]));
				if (nextIndent <= DefIndent)
					break;
			}

			dprintf("Running (class): \n%s\n", line.c_str());

 			PyObject* pObj = PyRun_String((char*)line.c_str(), Py_file_input, pDict, pDict);
			if (pObj)
				Py_DECREF(pObj);

		}
		else if ( (strstr(&(vecLine[0]), "class")) ||
				  (strstr(&(vecLine[0]), "def")) )
		{
			line += "\n\tpass\n";
			dprintf("Running (def): %s\n", line.c_str());
			PyObject* pObj = PyRun_String((char*)line.c_str(), Py_file_input, pDict, pDict);
			if (pObj)
				Py_DECREF(pObj);
		}*/
	}

	PyObject* pMods= PyImport_GetModuleDict();
	//PyObject* pBuil = PyEval_GetBuiltins();
	
	int nCurCol = GetCurrentCol();
	int nCurLine = GetCurrentLine();
	vecLine.resize(SendEditor(SCI_LINELENGTH, nCurLine)+1);
	SendEditor(SCI_GETLINE, nCurLine-1, (LPARAM)&(vecLine[0]));
	vecLine[nCurCol-2] = 0;

	PyObject* pItem = 0;
	string CurLine = &(vecLine[0]);
	do
	{
		string CurItem;
		if (CurLine.find('.') != -1)
			CurItem = CurLine.substr(0, CurLine.find('.'));
		else
			CurItem = CurLine;
		
		char* szCurItem = (char*)CurItem.c_str();
		if (!pItem)
		{
			pItem = PyDict_GetItemString(pMods, szCurItem);
			if (!pItem || pItem == Py_None)
			{
				//pItem = PyDict_GetItemString(pDict, szCurItem);
				//if (!pItem || pItem == Py_None)
				{
					pItem = PyDict_GetItemString(pBuil, szCurItem);
				}
			}
		}
		else
		{
			PyObject* pNext = PyObject_GetAttrString(pItem, szCurItem);
			if (!pNext)
			{
				goto NOTFOUND;
			}
			
			Py_XDECREF(pItem);
			pItem = pNext;
		}
		if (CurLine.find('.') != -1)
			CurLine = CurLine.substr(CurLine.find('.')+1, CurLine.length());
		else
			CurLine = "";
	}
	while (CurLine.length() > 0);

	if (pItem && (pItem != Py_None))
	{
		PyObject* pCmdList = PyObject_Dir(pItem);
		if (pCmdList && PyList_Check(pCmdList) && PyList_Size(pCmdList))
		{
			dprintf("DirResults: \n");
			static string ShowStr;
			ShowStr = "";
			for (int i = 0; i < PyList_Size(pCmdList); i++)
			{
				//dprintf("   %s\n", PyString_AsString(PyList_GetItem(pCmdList, i)));
				if (i > 0)
					ShowStr += ' ';
				ShowStr += PyString_AsString(PyList_GetItem(pCmdList, i));				
			}
			Py_XDECREF(pCmdList);

			const char* ShowCStr = ShowStr.c_str();
			//static char* ShowCStr = "test test test test";
			//SendEditor(SCI_AUTOCSHOW, strlen(ShowCStr), (WPARAM)ShowCStr);
			SendEditor(SCI_USERLISTSHOW, strlen(ShowCStr), (WPARAM)ShowCStr);
		}
		Py_XDECREF(pItem);
	}

	Py_XDECREF(pBuil);
	//Py_DECREF(pDict);
	//Py_DECREF(pMods);
	
NOTFOUND:

	Py_XDECREF(pDict);


	Py_Finalize();
#endif // 0
}


void CDbgRemoteView::AutoHint()
{
}


void CDbgRemoteView::StartAutoWatch()
{
	//dprintf("StartAutoWatch()\n");
	if (! ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetAutoWatch())
		return;

	//get the location of the mouse cursor
	POINT p;
	if (!GetCursorPos(&p))
		return;
	ScreenToClient(&p);

	//Convert to a document position
	int CurPos = SendEditor(SCI_POSITIONFROMPOINT, p.x, p.y);

	//get the begin and end of the current selection
	// Was using SCI_GETCURRENTPOS for some reason.
	//int SelBegin = SendEditor(SCI_GETCURRENTPOS, 0, 0);
	int SelBegin = SendEditor(SCI_GETSELECTIONSTART, 0, 0);
	if (SelBegin < 0)
		SelBegin = 0;
	int SelEnd = SendEditor(SCI_GETSELECTIONEND, 0, 0);
	
	//make it simple and swap if we're reverse selected.
	//dprintf("Selection is %d, %d.\n", SelBegin, SelEnd);
	if (SelBegin > SelEnd)
	{
		swap(SelBegin, SelEnd);
	}

	if ((CurPos >= SelBegin) && (CurPos <= SelEnd))
	{
		//the user is hovering over a selection - use that	
		//dprintf("Use selection.\n");
	}
	else
	{		
		//the user is hovering someplace else, grab the word
		//nearest the cursor (ignore the current selection)
		SelBegin = CurPos;
		SelEnd = CurPos;	
		
		//find the start and end of the current word
		GetCurrentWordPos(SelBegin, SelEnd);
	}

	if (SelEnd > SelBegin)
	{
		TEXTRANGE tr;
		tr.chrg.cpMin = SelBegin;
		tr.chrg.cpMax = SelEnd;
		vector<char> vecText;
		vecText.resize(SelEnd - SelBegin+1, 0);
		tr.lpstrText = &vecText[0];
		
		SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

		//just to be on the safe side - lets just disallow assignment
		if ( ! strchr(tr.lpstrText, '=') )
		{
			dprintf("Sending Autowatch: %s\n", tr.lpstrText);
			//ask the app to get the watch item for the selected word
			((CDbgRemoteApp*)AfxGetApp())->GetAutowatchItem(tr.lpstrText);

			//store the watch position
			m_nAutoWatchPos = CurPos;
			m_autoWatchExpression = tr.lpstrText;
		}
		else
			dprintf("Not Sending autowatch(=): %s\n", tr.lpstrText);
	}
}

void CDbgRemoteView::EndAutoWatch()
{
	dprintf("EndAutoWatch()\n");

	SendEditor(SCI_CALLTIPCANCEL);
	//flag the watch position so we don't display watch if we move before
	//we receive the watch results
	m_nAutoWatchPos = 0;
}

void CDbgRemoteView::DisplayAutoWatch(const char* cszWatchStr)
{
	dprintf("Autowatch Received: %s\n", cszWatchStr);
	//if we have a non zero watch position, show the watch tip at the right spot.
	if (m_nAutoWatchPos)
	{
		// Must be declared out here so it remains valid long enough to be used.
		string text(cszWatchStr);
		// Too many lines causes the top of the list to disappear off the top of screen
		// This has been fixed, but huge tooltips are probably still a bad idea.
		const int kMaxLines = 50;
		// Breaking lists/tuples/dictionaries into multiple lines added by Bruce Dawson, July 15, 2002.
		if (text.size() > 2 && text[0] == ' ' && (text[1] == '[' || text[1] == '{' || text[1] == '('))
		{
			string temp = text;
			text = "";
			bool inQuotes = false;
			int inBrackets = 0;
			int inParentheses = 0;
			int inBraces = 0;
			int numLines = 0;
			for (size_t i = 0; i < temp.size(); ++i)
			{
				text += temp[i];
				if (temp[i] == '\"')
					inQuotes = !inQuotes;
				if (!inQuotes)
				{
					// The idea is that if we come across a comma and we aren't inside
					// quotes and if all braces/brackets/parentheses (except for the
					// opening one) are matched then we should make a new line. This
					// has the effect of splitting the outermost list/dictionary/tuple
					// into elements, thus making them easier to read. I'm not sure
					// whether this parsing code is complete, and it would be nice to
					// somehow break-up sub-objects also, but that's a bit ambitious
					// for a tool-tip.
					// Ignore the first bracket/parentheses/brace...
					if (i > 1)
					{
						if (temp[i] == '(')
							++inBrackets;
						if (temp[i] == ')')
							--inBrackets;
						if (temp[i] == '{')
							++inParentheses;
						if (temp[i] == '}')
							--inParentheses;
						if (temp[i] == '[')
							++inBraces;
						if (temp[i] == ']')
							--inBraces;
					}
					if (!inBrackets && !inParentheses && !inBraces && temp[i] == ',')
					{
						++numLines;
						if (numLines > kMaxLines)
						{
							text += "...";
							break;
						}
						text += "\n";
					}
				}
			}
		}
		string display = m_autoWatchExpression + " =" + text;
		SendEditor(SCI_CALLTIPSHOW, m_nAutoWatchPos, (long)display.c_str());
		//SendEditor(SCI_CALLTIPSHOW, m_nAutoWatchPos, (long)cszWatchStr);
	}
}





void CDbgRemoteView::OnViewExpandAll() 
{
	for (int line = 0; line < SendEditor(SCI_GETLINECOUNT); ++line)
	{
		if ( ! SendEditor(SCI_GETFOLDEXPANDED, line))
			SendEditor(SCI_TOGGLEFOLD, line);
	}
}

void CDbgRemoteView::OnUpdateViewExpandAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bShowSelectiveDisplay);
}

void CDbgRemoteView::OnViewCollapseAll() 
{
	for (int line = 0; line < SendEditor(SCI_GETLINECOUNT); ++line)
	{
		if (SendEditor(SCI_GETFOLDEXPANDED, line))
			SendEditor(SCI_TOGGLEFOLD, line);
	}
}

void CDbgRemoteView::OnUpdateViewCollapseAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bShowSelectiveDisplay);
}

void CDbgRemoteView::OnEditFindinFiles() 
{
	string Text = GetCurrentText();

	((CDbgRemoteApp*)AfxGetApp())->FindInFiles(Text.c_str());
}


void CDbgRemoteView::OnAttemptModify()
{
	//checking if the doc is still read only
	if (GetDocument()->ReadOnly())
	{
		CString docFilename = GetDocument()->GetPathName();
		CSourceControlInterface* psci = ((CDbgRemoteApp*)AfxGetApp())->GetSci();
		if (psci && docFilename.GetLength())
		{
			
			DWORD dwcurTick = GetTickCount();
			SScFileStatus stat = psci->GetFileStatus(docFilename);
			dprintf ("GetFileStatus took: %d ms\n", GetTickCount() - dwcurTick);
			if (stat.bControlled)
			{
				if (stat.bOpenForEdit == false)
				{
					string prompt = stringprintf("File is not opened for edit, check out file: %s?", docFilename);
					if (IDYES == AfxMessageBox(prompt.c_str(), MB_YESNO))
					{
						bool reload = false;
						if (stat.bNewerInSc)
						{
							if (IDYES == AfxMessageBox("A newer version is available, Sync first?", MB_YESNO))
							{
								
								dwcurTick = GetTickCount();
								psci->SyncFile(docFilename);
								dprintf ("SyncFile took: %d ms\n", GetTickCount() - dwcurTick);
								reload = true;
							}
						}
						dwcurTick = GetTickCount();
						psci->CheckFileOut(docFilename);
						dprintf ("CheckFileOut took: %d ms\n", GetTickCount() - dwcurTick);

						if (reload)
							((CDbgRemoteApp*)AfxGetApp())->ReloadDocumentFile(docFilename);

						dwcurTick = GetTickCount();
						((CDbgRemoteApp*)AfxGetApp())->UpdateFileStatus(docFilename);
						dprintf ("UpdateFileStatus took: %d ms\n", GetTickCount() - dwcurTick);
					}
				}
			}
			else
			{
				if (IDYES == AfxMessageBox("File is read-only - change to readwrite?", MB_YESNO))
				{
					DWORD dwAttr = GetFileAttributes(docFilename);
					dwAttr = dwAttr & (~FILE_ATTRIBUTE_READONLY);
					SetFileAttributes(docFilename, dwAttr); 
				}
			}
		}
	}
	
	SendEditor(SCI_SETREADONLY, GetDocument()->ReadOnly());	
}

void CDbgRemoteView::OnEditGototagdefinition()
{
	string sel = GetCurrentText();
	
	const CTags* pTagDb = ((CDbgRemoteApp*)AfxGetApp())->GetBrowseTags();
	
	vector<string> files;
	vector<int> lines;

	if (pTagDb->FindTagInfo(sel.c_str(), files, lines) )
	{
		if (files.size() > 1)
		{
			CBrowseSelDlg dlg;
			for (unsigned int i = 0; i < files.size(); i++)
				dlg.AddBrowseItem(files[i].c_str(), lines[i]);
			
			int sel = dlg.SelectBrowseItem();
			if (sel > -1)
			{
				assert(sel < (int) files.size());
				((CDbgRemoteApp*)AfxGetApp())->DisplayFileLine(files[sel].c_str(), lines[sel]);
			}
		}
		else
			((CDbgRemoteApp*)AfxGetApp())->DisplayFileLine(files[0].c_str(), lines[0]);
	}
}

void CDbgRemoteView::OnUpdateEditGototagdefinition(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( ((CDbgRemoteApp*)AfxGetApp())->GetBrowseTags()->HasData() );
}



void CDbgRemoteView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// Load the menu resource. 	
	CMenu menu;
	menu.LoadMenu(IDR_EDITVW_POPUP);

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	CMenu* pSubMenu = menu.GetSubMenu(0); 

	if (SendEditor(SCI_CANUNDO, 0, 0) == 0)
		pSubMenu->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND|MF_GRAYED);
	if (SendEditor(SCI_CANPASTE, 0, 0) == 0)
		pSubMenu->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND|MF_GRAYED);
	if (SendEditor(SCI_CANREDO, 0, 0) == 0)
		pSubMenu->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND|MF_GRAYED);
	CMenu* pMarkerSubMenu = 0;
	int i = 0;
	while (pMarkerSubMenu == 0)
	{
		pMarkerSubMenu = pSubMenu->GetSubMenu(i++);
	}
	if (pMarkerSubMenu)
	{
		int NextMarkLine = SendEditor(SCI_MARKERNEXT, 0, BOOKMARK_MASK);
		if (!(NextMarkLine > -1))
		{
			pMarkerSubMenu->EnableMenuItem(ID_BOOKMARKS_GOTOPREVIOUS, MF_BYCOMMAND|MF_GRAYED);
			pMarkerSubMenu->EnableMenuItem(ID_BOOKMARKS_GOTONEXT, MF_BYCOMMAND|MF_GRAYED);
			pMarkerSubMenu->EnableMenuItem(ID_BOOKMARKS_CLEARALL, MF_BYCOMMAND|MF_GRAYED);
		}
	}


	if (SendEditor(SCI_GETVIEWWS, 0, 0) != SCWS_INVISIBLE)
		pSubMenu->CheckMenuItem(ID_VIEW_EDITOROPTIONS_VIEWWHITESPACE, MF_BYCOMMAND|MF_CHECKED);
	if (m_bShowSelectiveDisplay)
		pSubMenu->CheckMenuItem(ID_VIEW_SELECTIVEDISPLAY, MF_BYCOMMAND|MF_CHECKED);
	if (m_bShowLinenumbers)
		pSubMenu->CheckMenuItem(ID_VIEW_LINENUMBERS, MF_BYCOMMAND|MF_CHECKED);
	if (SendEditor(SCI_GETVIEWEOL, 0, 0) != 0)
		pSubMenu->CheckMenuItem(ID_VIEW_VIEWLINEENDINGS, MF_BYCOMMAND|MF_CHECKED);

	if (m_bShowSelectiveDisplay)
		pSubMenu->CheckMenuItem(ID_VIEW_SELECTIVEDISPLAY, MF_BYCOMMAND|MF_CHECKED);
	else
	{
		pSubMenu->EnableMenuItem(ID_VIEW_EXPANDALL, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_VIEW_COLLAPSEALL, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_EDIT_REDO, MF_BYCOMMAND|MF_GRAYED);
	}

	CSplitterWnd* pSplit = (CSplitterWnd*)GetParent();
	if (pSplit->GetRowCount() != 1)
		pSubMenu->CheckMenuItem(ID_VIEW_SPLITWINDOW, MF_BYCOMMAND|MF_CHECKED);

	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); 
}

void CDbgRemoteView::OnFileProperties()
{
	GetDocument()->ShowProperties();
}

void CDbgRemoteView::OnViewSplitwindow()
{
	CSplitterWnd* pSplit = (CSplitterWnd*)GetParent();
	int rows = pSplit->GetRowCount();
	if (rows == 1)
	{
		pSplit->SplitRow(20);
	}
	else
	{
		int activeRow = 0;
		pSplit->GetActivePane(&activeRow);
		if (activeRow == 0)
			pSplit->DeleteRow(1);
		else
			pSplit->DeleteRow(0);
	}
	pSplit->RecalcLayout();
}

void CDbgRemoteView::OnUpdateViewSplitwindow(CCmdUI *pCmdUI)
{
	CSplitterWnd* pSplit = (CSplitterWnd*)GetParent();
	int rows = pSplit->GetRowCount();
	pCmdUI->SetCheck(rows != 1);
}
