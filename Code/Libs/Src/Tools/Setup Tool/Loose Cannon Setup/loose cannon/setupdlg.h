#ifndef _SETUPDLG_H
#define _SETUPDLG_H
extern "C" {
_declspec(dllexport) BOOL CALLBACK GetPIDDlgProc1 (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
_declspec(dllexport) BOOL CALLBACK GetNameDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
_declspec(dllexport) BOOL CALLBACK CDSpeedDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
_declspec(dllexport) BOOL CALLBACK DoneCDDlgProc(HWND hDlg, WORD message, WPARAM wParam, LPARAM lParam);
_declspec(dllexport) BOOL CALLBACK CDWndProc(HWND hDlg, WORD message, WPARAM wParam, LPARAM lParam);
_declspec(dllexport) BOOL CALLBACK StartCDDlgProc(HWND hDlg, WORD message, WPARAM wParam, LPARAM lParam);
EBURETCODE WINAPI MkRootFn(LPMKROOTDATA mk);
EBURETCODE WINAPI GetGroupFn(LPGETGROUPDATA group);
EBURETCODE WINAPI GetNameFn(LPGETNAMEDATA UserName);
EBURETCODE WINAPI CopyFileFn(LPFILECOPYSTATUS lpfs);
EBURETCODE WINAPI GetPidNumFn(LPGETPIDDATA pid);
EBURETCODE WINAPI GetPidFn(LPGETPIDDATA pid);
EBURETCODE WINAPI CDFailedFn(LPCDSPEEDDATA cd);
EBURETCODE WINAPI StartCDFn();
EBURETCODE WINAPI CDDoneFn(LPCDSPEEDDATA cd);
EBURETCODE WINAPI CreateCDSpeedWindow(LPCDSPEEDDATA cd);
EBURETCODE WINAPI InstallDirectX(LPUPDATEARRAY Updates);
EBURETCODE WINAPI DetectDirectX(DIRECT_X_VERSION * pver);
EBURETCODE WINAPI InstIconFn(LPINSTICONDATA lpi);
EBURETCODE WINAPI InstallDPlay();

#define WM_UNINSTALL WM_USER+1000
#define WM_START_BILLBOARDS WM_UNINSTALL+2
#define WM_STOP_BILLBOARDS WM_START_BILLBOARDS+2

extern BOOL bDXSkipped;

void NukeDPlayRemants(TCHAR *,ULONG);
void DeleteDirectPlayFile(TCHAR *szFile);
}
WORD ReallyQuit(HWND);
BOOL ForwardMyMessages();
// extern VOID ReplaceStringTokens(char *sz, size_t wBuf);

//void InstallAMovie();

class GetGroupDlg : public Dialog {
public:
	GetGroupDlg() : Dialog("GETGROUPDLG") {}
	~GetGroupDlg() {}
	BOOL Init(LPARAM lParam);
	BOOL Command(WORD nId,WORD nNotify, LPARAM lParam);
private:
	LPGETGROUPDATA group;

};
class MkRootDlg : public Dialog {
public:
	MkRootDlg() : Dialog(MAKEINTRESOURCE(GETROOTDLG)) {}
	~MkRootDlg() {}
	BOOL Init(LPARAM lParam);
	BOOL Ok();
	BOOL Command(WORD nId,WORD nNotify, LPARAM lParam);
private:
	LPMKROOTDATA mk;
	Browse();

};

class GetPIDDlg : public Dialog {
public:
#ifdef SETUPOEM
	GetPIDDlg() : Dialog("GETOEMPIDDLG") {}
#else
	GetPIDDlg() : Dialog("GETPIDDLG"){}
#endif
	~GetPIDDlg() {}
    BOOL Init(LPARAM lParam);
    BOOL Command(WORD nID, WORD nNotify,LPARAM lParam);
    BOOL Ok();
private:
	LPGETPIDDATA pid;
};

class GetNameDlg : public Dialog {
public:
	GetNameDlg() : Dialog("GETNAMEDLG") {}
	~GetNameDlg() {}
    BOOL Init(LPARAM lParam);
    BOOL Ok();
private:
	LPSTR UserName;
};
class ShowPIDDlg : public Dialog {
public:
#ifdef SETUPOEM
    ShowPIDDlg() : Dialog("GETOEMPIDDLG1") {}
#else
    ShowPIDDlg() : Dialog("GETPIDDLG1") {}
#endif
	~ShowPIDDlg() {}
    BOOL Init(LPARAM lParam);
};

class CDFailedDlg : public Dialog {
private:
	LPCDSPEEDDATA cd;
public:
	CDFailedDlg() : Dialog("CDSPEEDDLG") {}
	~CDFailedDlg() {}
    BOOL Init(LPARAM lParam);
};
class CDDoneDlg : public Dialog {
private:
	LPCDSPEEDDATA cd;
public:
	CDDoneDlg() : Dialog("CDTESTRESULTS") {}
	~CDDoneDlg() {}
    BOOL Init(LPARAM lParam);
};
class StartCDDlg : public Dialog {
public:
	StartCDDlg() : Dialog("STARTCDTEST") {}
	~StartCDDlg() {}
    BOOL Init(LPARAM lParam);
	BOOL Cancel();
};

class DXSetupDlg : public Dialog {
public:
	DXSetupDlg() : Dialog("BATCHDX") {}
	~DXSetupDlg() {}
    BOOL Init(LPARAM lParam);
	BOOL Notify(WPARAM wParam, LPARAM lParam);
	BOOL Cancel();
	BOOL Ok();
	BOOL Command(WORD nID, WPARAM wParam, LPARAM lParam);

private:
	WORD GetDefaultButtonResponse(DWORD dwMsgType);
	void UpdateButton(HWND hDlg,int Id, char *szName,BOOL bRecommend);
	LPUPDATEARRAY Updates;
};

class Restart : public Dialog {
public:
	Restart() : Dialog("RESTART") {}
	~Restart() {}
    BOOL Init(LPARAM lParam);
	BOOL Cancel();
};

#endif
