
/*
    print.h - Copyright (c) Microsoft Corp. 1991

    Created by Gavin Jancke

*/
#ifndef print_h
#define print_h



extern "C"
{
#include "commdlg.h"

extern _declspec(dllexport) BOOL WINAPI AbortProc (HDC hPr, int reserved);
extern _declspec(dllexport) int WINAPI AbortDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
}

/*@Introduction
   This object is the interface to the common dialog print routines in sqltools.dll
and the printer capability of Windows.  It will allow for printing from several
different types of MDI library objects including ColumnListBox, Dialogs with
Column Listbox, and Reports.  Each object has it\'s own member function to call
that will properly format and print out the contents of the calling object.
This object has two of it\'s own message processing loops, one is for a
abort dialog box, the other is similar but for abort dialog inside of a dialog.
This object is a global object and so should not be created by anyone but
the global object startup.  It is declared in the global.cpp file of the
library.
*/
class Printer {
public :
/*@Public_Methods Public Methods */
/**/
/*@Public_Variables Public Variables */
/**/
/*@Variable This flag is set if the user decides to about printing. */
    boolean    bAbort;
/*@Variable This is the handle to the abort print dialog box. */
    HWND       hwndAbortDlg;
	HWND hwndParent;

/*@Variable This structure contains the information needed for the common dialog
to put up it\'s print setup dialog and for use during printing. */
    PRINTDLG   PD;
/*@Section Constructors */
/**/
/*@Method  Get parent window handle. */
    Printer (HWND hWnd ){ 
		hwndParent = hWnd;
		ZeroMemory(&PD, sizeof(PRINTDLG));
	}
/*@Method  This destructor is a do nothing. */
    ~Printer (){}
/*@Section Services */
/**/
    boolean PrintSetup ();
    boolean PrintStart ();
    boolean PrintStop ();
    boolean PrintText (char *);
    boolean PrintMultiLine(char *);
    void *ReturnPDStruct();
	boolean PrintFile(char *);
    int abortDlgProc (HWND, UINT, WPARAM, LPARAM);
    BOOL IsDefaultPrinter();


protected :
/*@Protected_Variables Protected Variables */
/**/
/*@Variable This Printer is to the print buffer for the current line. */
    char *lpPrintBuffer;
/*@Variable This pointer contains the title of the document printed. */
    char *pszTitle;
/*@Variable A far proc pointer to the about print proc abort handler that windows
uses as a message pump for the print routines. */
    ABORTPROC    lpfnAbortPrinterProc;
/*@Variable A far proc pointer to the about print dialog message handler. */
    DLGPROC  lpfnAbortProc;
/*@Variable A display context for the printer surface. */
    HDC        hPr;
/*@Variable Text metrics used during the printing process. */
    TEXTMETRIC TextMetric;
/*@Variable Line spacing value. */
    DWORD       LineSpace;
/*@Variable Page Size. */
    DWORD       nPageSize;
/*@Variable Lines per page. */
    DWORD       LinesPerPage;
/*@Variable The current line that is printing. */
    DWORD       CurrentLine;
/*@Variable The maximum length a full path/filename can be. */
    DWORD       wMaxFileLength;
/*@Variable Status of the printing. */
    DWORD       IOStatus;
/*@Variable Mode of printing (MDI, REPORT, etc). */
    DWORD       wPrintMode;
/*@Protected_Methods Protected Methods */
/**/
    HDC  GetPrintDC (void);
    void DestroyAbortWnd ();
    char *Printer::GetPrintFile(char *);

};
#endif



