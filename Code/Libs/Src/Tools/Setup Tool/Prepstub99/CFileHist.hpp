/**************************************************************
* CFileHist.hpp: File information History Class
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CFILEHIST_H
#define __CFILEHIST_H
#define ALLOC_HIST 10;

#include "windows.h"
#include <list>
#include <algorithm>

using namespace std ;

typedef struct tagHIST
{
	char	*pszPattern;		// regular expression to match. (wildcards or filename)
	DWORD	dwOSFlags;			// os mask
	DWORD	dwInstallFlags;		// misc install flags
	__int64 cGroup;				// install group mask
} HIST ;


bool HistLessThan (HIST* a, HIST* b);


typedef list <HIST *> HISTLIST;


class CFileHist
{
	public:
		CFileHist();
		~CFileHist();
		bool Create(){ return true;};
		bool AttachHist (HIST *hist);
		HIST *FindHist (const char *szExpression);
		void DumpHist ();

	private:
		HISTLIST m_Hist;
};

#endif