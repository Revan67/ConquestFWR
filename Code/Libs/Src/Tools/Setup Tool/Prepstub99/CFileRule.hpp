/**************************************************************
* CFileRule.hpp: File information Rule Class
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CFILERULE_H
#define __CFILERULE_H

#include "windows.h"
#include "RegExMatch.hpp"
#include <list>
#include <algorithm>

using namespace std ;


#define ALLOC_RULE 10

#define	RULE_DEFAULT			0x1
#define	RULE_EXCLUDE			0x2
#define RULE_LOCALIZE_STRING	0x3
#define RULE_DEPARENT_DIR		0x4


typedef struct tagFILERULE
{
	char	*pszPattern;		// regular expression to match. (wildcards or filename)
	DWORD	dwAction;			// action to take if pattern matched.
	DWORD	dwOSFlags;			// os mask
	DWORD	dwInstallFlags;
	__int64 cGroup;				// install group mask
} RULE;


class CFileRule
{
	public:
		CFileRule();
		~CFileRule();
		bool Create(){ return true;};
		bool AttachRule (RULE *rule);
		int FindRule (int iStart, const char *szString);
		RULE *GetRule (int iRuleNumber);
		bool RegExCompare (char *szExp1, char *szExp2);
		int CountRuleType (DWORD dwRuleType);

	private:
		RULE **m_Rule;
		int m_cMaxRule;
		int m_cRule;
};

#endif