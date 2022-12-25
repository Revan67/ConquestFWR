// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__820BDDB8_EB23_4F7D_B57F_82FCAF7F41F5__INCLUDED_)
#define AFX_STDAFX_H__820BDDB8_EB23_4F7D_B57F_82FCAF7F41F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>

#include "../p4api/clientapi.h"

#pragma comment(lib, "../p4api/libclient.lib")
#pragma comment(lib, "../p4api/librpc.lib")
#pragma comment(lib, "../p4api/libsupp.lib")


#include <string>
#include <vector>
using namespace std;

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__820BDDB8_EB23_4F7D_B57F_82FCAF7F41F5__INCLUDED_)
