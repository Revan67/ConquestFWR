#ifndef FERROR_H
#define FERROR_H

#ifdef __cplusplus
extern "C" {
#endif

void Fatal(char *fmt, ...);
void FatalwID(long dwStringResource);
void WarningBox(HWND hwnd, long dwStringResource, ...);

extern char szAppName[40];
extern char szRecName[64];
extern HINSTANCE hInstance;
extern HWND hMainWindow;

#ifdef __cplusplus
}
#endif

#endif
