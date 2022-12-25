/***************************************************************************
	FILE: setup.H
		Copyright (C) 1996, Microsoft Corp.

	PURPOSE: Defines buttons and dialogs specific to each launch application

***************************************************************************/

#include "appspecific.h"

extern HINSTANCE g_hResourceInst;
class InstallButton;
extern InstallButton *	pInstall;
class UnInstallButton;
extern UnInstallButton * pUnInstall;
class ReInstallButton;
extern ReInstallButton * pReInstall;
class QuitButton;
extern QuitButton * pExit;
class HelpButton;
extern HelpButton * pHelp;
class PlayButton;
extern PlayButton * pPlay;
class WebLinkButton;
extern WebLinkButton * pWebLink;
class ConfigureButton;
extern ConfigureButton * pConfigure;

extern char lpSetupEnu[];

extern "C" {
int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL SetupInit(HINSTANCE);
long FAR PASCAL SetupWndProc(HWND, WORD, WPARAM, LPARAM);

_declspec(dllexport) BOOL CALLBACK MyCPPDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
}

// 
// cloned to allow sub-classing of the richedit control for selection and indication of focus 
// 
class MyDialog;
extern MyDialog *glpMyDialog;

extern WNDPROC wpOrigEditProc;
extern LRESULT CALLBACK EulaEditSubclassProc(HWND hwnd,	UINT uMsg,	WPARAM wParam,	LPARAM lParam);
DWORD CALLBACK EditStreamCallback(DWORD dwFile, LPBYTE pbBuffer, LONG cbToRead, LONG *pcbRead);
DLGPROC AbortPrintDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CALLBACK DisplayNextBillboard(HWND hwnd, UINT iMsg, UINT iTimerID, DWORD dwTime);

using namespace NGLOBALS;

// 
// cloned to allow sub-classing of the richedit control for selection and indication of focus 
class MyDialog {
private:
	MyDialog *Parent;
protected:
	HWND hDlg;
	char *szTemplate;
	HWND hwndEdit;

	BOOL DlgSendMessage(UINT msg,WPARAM wParam,LPARAM lParam)
	{
		return SendMessage(hDlg,msg,wParam,lParam);
	}
    WORD ReallyQuit(HWND hDlg)
    {
       char str[200];
       LoadString(GetResourceInst(),STR_QUIT_SETUP,str,200);
	   ReplaceStringTokens(str, sizeof(str));
       return MessageBox(hDlg,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2);
    }
public:

	  MyDialog(char *Template) {
	  szTemplate = Template;
	  Parent = glpMyDialog;
	  glpMyDialog = this;
	  }
	  ~MyDialog() { glpMyDialog = Parent; }
	  EBURETCODE start (LPARAM lParam);
	  EBURETCODE start ();
	  virtual BOOL Init(LPARAM lParam) { return FALSE; }
	  virtual BOOL Ok() { EndDialog(hDlg,EBU_OK); return TRUE; }
	  virtual BOOL Cancel() {
		  if(ReallyQuit(hDlg)==IDOK)
		  {
			  EndDialog(hDlg,EBU_ABORT);
		  }
		  return TRUE;
	  }
	  virtual BOOL Command (WORD nId, WORD nNotifyCode, LPARAM lParam) {return FALSE; }
	  virtual BOOL Notify (WPARAM wParam, LPARAM lParam) {return FALSE; }
	  virtual BOOL Help(WORD HelpID) { return FALSE; }
	  virtual BOOL Destroy () { return FALSE; }
	  virtual BOOL Activate(WORD bActivate)  {return FALSE;}
	  BOOL ProcessCommand(HWND hwnd,WORD msg,WPARAM wParam, LPARAM lParam);
};

//
// Install Button
class InstallButton : public PushButton {
public:
	InstallButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		if((fLaunched = HasAppEverBeenLaunched(APP_MUST_LAUNCH)) == TRUE)
		{
			ShowWindow(this->hButWnd, SW_HIDE);
			enable(FALSE);
		}
		else
		{
			enable(TRUE);
			ShowWindow(this->hButWnd, SW_SHOW);
			SetWindowLong(hButWnd,GWL_STYLE,(GetWindowLong(hButWnd,GWL_STYLE) | BS_DEFPUSHBUTTON));
			SetFocus(hButWnd);
		}
	}
	~InstallButton() {}
	void ButtonClicked();
	BOOL fLaunched;
	friend PlayButton;
	friend UnInstallButton;
	friend ReInstallButton;
};

//
//	Play Button
class PlayButton : public PushButton {
public:
	PlayButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		if((fLaunched = HasAppEverBeenLaunched(APP_MUST_LAUNCH)) == TRUE)
		{
			enable(TRUE);
			ShowWindow(this->hButWnd, SW_SHOW);
			SetFocus(hButWnd);
		}
		else
		{
			ShowWindow(this->hButWnd, SW_HIDE);
			SetWindowLong(hButWnd,GWL_STYLE,(GetWindowLong(hButWnd,GWL_STYLE) | BS_DEFPUSHBUTTON));
			enable(FALSE);
		}
	}
   ~PlayButton() {}
	void ButtonClicked();
	BOOL fLaunched;
	friend InstallButton;
	friend UnInstallButton;
	friend ReInstallButton;
};

//
//	Uninstall Button
class UnInstallButton : public PushButton {
public:
	UnInstallButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		if(HasAppEverBeenLaunched(APP_MUST_LAUNCH) == TRUE)
		{
		    enable(TRUE);
		}
		else
		{
			enable(FALSE);
		}
	}
	~UnInstallButton() {}
	void ButtonClicked();
	friend InstallButton;
	friend PlayButton;
	friend ReInstallButton;
};


//
//	Quit Button
class QuitButton : public PushButton {
public:
	QuitButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		;
	}
	~QuitButton() {
      FreeLibrary(GetResourceInst());
      DeleteFile(lpSetupEnu);
	}
	void ButtonClicked();
	friend InstallButton;
	friend ReInstallButton;
    WORD ReallyQuit(HWND hWindow)
    {
       char str[200];
       LoadString(GetResourceInst(),STR_QUIT_SETUP,str,200);
	   ReplaceStringTokens(str, sizeof(str));
       return MessageBox(hWindow,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2);
    }
};

//
//	Reinstall Button
class ReInstallButton : public PushButton {
public:
	ReInstallButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		if(HasAppEverBeenLaunched(APP_MUST_LAUNCH))
		{
            enable(TRUE);
		}
		else
		{
			enable(FALSE);
		}
	}
	~ReInstallButton() {}
	void ButtonClicked();
	friend PlayButton;
	friend InstallButton;
};

//
//	Web link Button
class WebLinkButton : public PushButton {
public:
	WebLinkButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		enable(IsBrowserInstalled());
		ShowWindow(this->hButWnd, SW_SHOW);
	}
   ~WebLinkButton() {}
	void ButtonClicked();
};

//
//	Configure Button
class ConfigureButton : public PushButton {
public:
	ConfigureButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		if(HasAppEverBeenLaunched(APP_MUST_LAUNCH) == TRUE)
		{
			    enable(TRUE);
		}
		else
		{
			enable(FALSE);
		}
	}
   ~ConfigureButton() {}
	void ButtonClicked();
};

//
//	Web link Button
class ReadMeButton : public PushButton {
public:
	ReadMeButton(HWND hWnd,BUTTONRECT *br,char *sndfile) : PushButton(hWnd,br,sndfile)
	{
		hInst = 0;
		enable(TRUE);
		ShowWindow(this->hButWnd, SW_SHOW);
	}
   ~ReadMeButton() {}
	void ButtonClicked();
};

//
// Deriving from MyDialog instead of Dialog to allow for subclassing of the RTF control
// which requires clean-up code in the non-virtual processcommand method of Dialog.
//
class EULADlg : public MyDialog {
public:
	EULADlg() : MyDialog(MAKEINTRESOURCE(IDD_EULA)) {}
	~EULADlg() {}
    BOOL Init(LPARAM lParam);
	BOOL Command (WORD nId, WORD nNotifyCode, LPARAM lParam);
	BOOL Cancel()
	{
		if (IDOK == ReallyQuit(hDlg))
		{
			EndDialog(hDlg,IDCANCEL);
			return (TRUE);
		}
		return (TRUE);
	}

	BOOL Ok()
	{
		EndDialog(hDlg,IDOK);
		return(TRUE);
	}
	void PrintTheContents (HWND hWndRichEdit);
	BOOL InitEULARTF(HWND hWndRichEdit, UINT nEULAPathResID);
};

// 
// Custom version of MkRoot handles the special case when the root is initially null on ID_OK command.
//
class MyMkRootDlg : public MyDialog {
public:
	MyMkRootDlg() : MyDialog(MAKEINTRESOURCE(GETROOTDLG)) {}
	~MyMkRootDlg() {}
	BOOL Init(LPARAM lParam);
	BOOL Ok();
	BOOL Command(WORD nId,WORD nNotify, LPARAM lParam);
private:
	LPMKROOTDATA mk;
	Browse();
};
