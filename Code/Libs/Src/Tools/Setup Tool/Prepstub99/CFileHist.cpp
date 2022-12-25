/**************************************************************
* CFileHist.cpp: File information History Class
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CFileHist.hpp"


CFileHist::CFileHist()
{

}


CFileHist::~CFileHist()
{
	HISTLIST::iterator i;

	for (i=m_Hist.begin(); i != m_Hist.end(); ++i)
	{
		if (static_cast<HIST *>(*i)->pszPattern)
		{
			free (static_cast<HIST *>(*i)->pszPattern);
			static_cast<HIST *>(*i)->pszPattern=NULL;
		}

		delete (static_cast<HIST *>(*i));
	}

	if (!m_Hist.empty()) 
	{
		m_Hist.clear();
	}
}


void CFileHist::DumpHist ()
{
	HISTLIST::iterator i;
	char buffer[255];

	for (i = m_Hist.begin(); i != m_Hist.end(); ++i)
	{
		wsprintf (buffer, "[%s] group:%d  flags:%d\n",(*i)->pszPattern,(*i)->dwOSFlags, (*i)->cGroup);
		OutputDebugString (buffer);
	}
}


bool CFileHist::AttachHist (HIST *pHist)
{
	HISTLIST::iterator start, end;
	if (!pHist)
	{
		return false;
	}

	// bounds checker will flag uninitialzed memory in lower_bound. false warning.
	m_Hist.insert (lower_bound (m_Hist.begin(), m_Hist.end(), pHist, HistLessThan), pHist);

	return true;
}


bool HistLessThan (HIST* a, HIST* b)
{
	return (strcmpi (a->pszPattern,b->pszPattern) < 0);
}


HIST *CFileHist::FindHist (const char *szString)
{
	HISTLIST::iterator i;
	HIST hi;

	if (!szString) 
	{
		return NULL;
	}

	hi.pszPattern = (char *)malloc (lstrlen (szString)+1);

	lstrcpy (hi.pszPattern, szString);
	
	i = lower_bound (m_Hist.begin(), m_Hist.end(), &hi, HistLessThan);

	if (i == m_Hist.end() || (strcmpi ((*i)->pszPattern, hi.pszPattern) !=0))
	{
		free (hi.pszPattern);
		hi.pszPattern=NULL;
		return NULL;
	}
	else
	{
		free (hi.pszPattern);
		hi.pszPattern=NULL;
		return (static_cast<HIST *>(*i));
	}
}