//
// registry.h
//
//    8/01/95  AjayJ    Added Win32_HKEY parameter to MyWriteUniversalString

#ifndef __REGISTRY_H
#define __REGISTRY_H

#include "setup.h"
#include "shellapi.h"

//
//*** My function declarations
//
BOOL InitRegistry( BOOLEAN );

LONG MyGetPrivateProfileString( LPSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf );
BOOL MyWritePrivateProfileString( LPCSTR lpszKey, LPCSTR lpszValue );

BOOL MyWriteUniversalRegType(HKEY Win32_HKey, LPCSTR lpszKey, LPSTR lpszValue, DWORD dwDataType);
LONG MyGetUniversalIniString( HKEY Win32Key, LPSTR lpszKey, LPCSTR szValue, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf );
                                  
LONG MyRegDeleteEmptyKey(HKEY hkey, LPSTR szKey, BOOL fIgnoreRegValues = FALSE);
LONG MyRegDeleteSubKeys(HKEY hkey, LPSTR szKey);

#endif //__REGISTRY_H
