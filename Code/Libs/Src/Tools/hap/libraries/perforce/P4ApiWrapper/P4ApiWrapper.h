#ifndef _P4APIWRAPPER_H_
#define _P4APIWRAPPER_H_

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the P4APIWRAPPER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// P4APIWRAPPER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef P4APIWRAPPER_EXPORTS
#define P4APIWRAPPER_API __declspec(dllexport)
#else
#define P4APIWRAPPER_API __declspec(dllimport)
#endif


struct SScFileStatus
{
	bool bControlled;
	bool bOpenForEdit;
	bool bOpenByOther;
	bool bNewerInSc;
};

#define NM_SENDINFO		WM_APP + 22
#define NM_SENDERROR	WM_APP + 23


extern "C"
{

P4APIWRAPPER_API int SetNotifyHwnd(HWND hwndNotify);
typedef int (*TFnSetNotifyWnd)(HWND);


P4APIWRAPPER_API int GetFileStatus(const char* cszFilename, SScFileStatus* pFs);
typedef int (*TFnGetFileStatus)(const char*, SScFileStatus*);


P4APIWRAPPER_API int CheckFileOut(const char* cszFilename);
typedef int (*TFnCheckFileOut)(const char*);

P4APIWRAPPER_API int RevertFile(const char* cszFilename);
typedef int (*TFnRevertFile)(const char*);

P4APIWRAPPER_API int CheckFileIn(const char* cszFilename);
typedef int (*TFnCheckFileIn)(const char*);

P4APIWRAPPER_API int AddFile(const char* cszFilename);
typedef int (*TFnAddFile)(const char*);

P4APIWRAPPER_API int DiffFile(const char* cszFilename);
typedef int (*TFnDiffFile)(const char*);

P4APIWRAPPER_API int SyncFile(const char* cszFilename);
typedef int (*TFnSyncFile)(const char*);










};

#endif //_P4APIWRAPPER_H_