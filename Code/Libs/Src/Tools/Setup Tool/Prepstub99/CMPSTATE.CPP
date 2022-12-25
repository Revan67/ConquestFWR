//
// cmpstate.cpp
//
//		Implementation of the CStateStack class - keeps track of state
//		info for the compiler
//
// History:
//
//		11/07/94	KenSh		Created
//

#include "prepstub.h"
#include "textdoc.h"
#include "cmpstate.h"


#define ALLOC_STATES		16


CStateStack::CStateStack()
{
	m_cEntries = 0;
	m_maxEntries = 0;
	m_paMajorStates = NULL;
	m_paMinorStates = NULL;
}

CStateStack::~CStateStack()
{
	if( m_paMajorStates )
	{
		free( m_paMajorStates );
	}
	
	if( m_paMinorStates )
	{
		free( m_paMinorStates );
	}
}

void CStateStack::Push( EMAJOR eMajor, EMINOR eMinor )
{
	//see if we need to expand the array
	if( m_cEntries == m_maxEntries )
	{
		m_maxEntries += ALLOC_STATES;
		
		if( m_paMajorStates )
		{
			ASSERT( m_paMinorStates );
			
			m_paMajorStates = (EMAJOR*)realloc( m_paMajorStates, m_maxEntries * sizeof(EMAJOR) );
			m_paMinorStates = (EMINOR*)realloc( m_paMinorStates, m_maxEntries * sizeof(EMINOR) );
		}
		else
		{
			ASSERT( !m_paMinorStates );
			
			m_paMajorStates = (EMAJOR*)malloc( m_maxEntries * sizeof(EMAJOR) );
			m_paMinorStates = (EMINOR*)malloc( m_maxEntries * sizeof(EMINOR) );
		}
	}
	
	ASSERT( m_paMajorStates );
	ASSERT( m_paMinorStates );
	
	m_paMajorStates[ m_cEntries ] = eMajor;
	m_paMinorStates[ m_cEntries ] = eMinor;
	
	m_cEntries++;
	
	return;
}


BOOL CStateStack::Pop( EMAJOR* pMajor, EMINOR* pMinor )
{
	if( m_cEntries == 0 )
	{
		return FALSE;
	}
	
	m_cEntries--;
	
	*pMajor = m_paMajorStates[ m_cEntries ];
	*pMinor = m_paMinorStates[ m_cEntries ];
	
	return TRUE;
}
