/**************************************************************
* CFileRule.cpp: File information Rule Class
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#include "CFileRule.hpp"

CFileRule::CFileRule()
{
	ZeroMemory (this, sizeof (CFileRule));
}


CFileRule::~CFileRule()
{
	int i;

	if (m_Rule)
	{
		for (i=0; i<m_cRule; i++)
		{
			if (m_Rule[i])
			{
				if (m_Rule[i]->pszPattern)
				{
					free (m_Rule[i]->pszPattern);
					m_Rule[i]->pszPattern=NULL;
				}
				delete m_Rule[i];
			}
		}
		free (m_Rule);
		m_Rule=NULL;
	}
}


bool CFileRule::AttachRule (RULE *rule)
{
	if (m_cRule == m_cMaxRule)
	{
		m_cMaxRule += ALLOC_RULE;

		if (m_Rule)
		{
			m_Rule = (RULE **)realloc (m_Rule, sizeof (RULE *) * m_cMaxRule);	
		}
		else
		{
			m_Rule = (RULE **)malloc (sizeof (RULE *) * m_cMaxRule);
		}
	}

	if (m_Rule == NULL)
		return false;

	m_Rule[m_cRule++] = rule;
	
	return true;
}


int CFileRule::FindRule (int iStart, const char *szString)
{
	int i;

	if (iStart < 0 || iStart >= m_cRule) 
		return -1;

	if (m_Rule)
	{
		for (i=iStart; i < m_cRule; i++)
		{
			if (RegExMatch (m_Rule[i]->pszPattern, szString))
				return i;
		}
	}
	return -1;
}


RULE *CFileRule::GetRule (int iRuleNumber)
{
	if (iRuleNumber < 0 || iRuleNumber >= m_cRule)
		return NULL;

	return m_Rule[iRuleNumber];
}

int CFileRule::CountRuleType (DWORD dwRuleType)
{
	int nCount = 0;

	if (m_Rule)
	{
		for (int i=0; i < m_cRule; i++)
		{
			if (m_Rule[i]->dwAction == dwRuleType)
			{
				++nCount;
			}
		}
	}

	return nCount;
}

				

	