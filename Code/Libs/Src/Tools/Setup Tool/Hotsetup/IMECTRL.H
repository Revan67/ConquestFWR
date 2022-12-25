#ifndef __IMECTRL_H
#define __IMECTRL_H

#include <imm.h>

void changeIMEStatus( HWND hWnd, WORD fIME );
void changeIMEStatus( HWND hWnd, HIMC *hOldIMC, WORD fIME );
void changeIMEStatus( HWND hDlg, HIMC *phIMC, UINT nEdit, WORD fIME );
void clearWndIMEUndeterminedString( HWND hWnd, DWORD dwFlag=CPS_CANCEL );
void clearDlgIMEUndeterminedString( HWND hDlg, UINT nEdit, DWORD dwFlag=CPS_CANCEL );
void hideIMEToolBar( HWND hWnd );
void showIMEToolBar( HWND hWnd );
void IMEInitialize(void);
void IMEUninitialize(void);

#endif //__IMECTRL_H
