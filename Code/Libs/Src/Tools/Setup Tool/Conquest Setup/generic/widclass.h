#include "hotsetuprc.h"
#define PUSHBUTTON   1
extern "C" {
_declspec(dllexport) BOOL CALLBACK CPPDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
}

using namespace NGLOBALS;

HBITMAP LoadResourceBitmap(HINSTANCE hInstance, HWND hWnd,LPSTR lpString, HPALETTE * lphPalette);
HPALETTE CreateDIBPalette (HDC hdc,LPBITMAPINFO lpbmi, LPINT lpiNumColors);
typedef struct Buttontag {
	RECT ButtonRect;
	WORD BitID;
}BUTTONRECT;

class Object;
extern HINSTANCE hInst;
extern HINSTANCE g_hInst;
extern HWND ghWnd;
class Dialog;
extern Dialog *glpDialog;
class Container {
public :
	Container();
	~Container();
	BOOL Add(Object *Obj);
	BOOL Remove(Object *Obj);
	BOOL ProcessMessage(WORD message,WPARAM wParam, LPARAM lParam);
private:
	Object **ObjArray;
	int ObjCount;
};

class Object {
public:
	Object() {}
	~Object() {}
	BOOL ProcessMessage(WORD message,WPARAM wParam, LPARAM lParam);
protected:
	friend Container;
	int objType;
};

class PushButton : public Object {
public:
	PushButton(HWND hWnd,BUTTONRECT *br,char *sndfile);
	~PushButton() {}
	virtual void ButtonClicked() {}
	void enable(BOOL bEnable)
	{
		bEnabled = bEnable;
		EnableWindow(hButWnd,bEnabled);
	}
	void setTitle(int title)
	{
		char buf[100];
		butrect.BitID = title;
		LoadString(NGLOBALS::GetResourceInst(),title,buf,100);
		SetWindowText(hButWnd,buf);
	}
	void Focus()
	{
		SetFocus(hButWnd);
	}
	void show(int Show)
	{
		ShowWindow(this->hButWnd, Show);
	}
protected:
	HWND hButWnd;
	BUTTONRECT butrect;
	HWND parenthWnd;
	char SoundFile[50];
	DRAWITEMSTRUCT DIS;
   BOOL bEnabled;
private:
	void ProcessCommand(WORD nCode);
	BOOL Paint();
	BOOL Draw(LPDRAWITEMSTRUCT);
	BOOL Measure(LPMEASUREITEMSTRUCT) { return FALSE;}
	HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPSTR lpString);
	friend Container;
   WORD baseID;
};
class Dialog {
private:
	Dialog *Parent;
protected:
	HWND hDlg;
	char *szTemplate;

	BOOL DlgSendMessage(UINT msg,WPARAM wParam,LPARAM lParam)
	{
		return SendMessage(hDlg,msg,wParam,lParam);
	}
    WORD ReallyQuit(HWND hDlg)
    {
       char str[200];
       LoadString(NGLOBALS::GetResourceInst(),STR_QUIT_SETUP,str,200);
	   ReplaceStringTokens(str, sizeof(str));
       return MessageBox(hDlg,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2);
    }
public:
	  Dialog(char *Template) {
	  szTemplate = Template;
	  Parent = glpDialog;
	  glpDialog = this;
	  }
	  ~Dialog() { glpDialog = Parent; }
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


