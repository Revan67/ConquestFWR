//
// cmpstate.h
//
//		Definition of the CStateStack class
//
// History:
//
//		11/06/94	KenSh		Created
//

#ifndef __CMPSTATE_H
#define __CMPSTATE_H

class CStateStack
{

private:
	int		m_cEntries;
	int		m_maxEntries;
	EMAJOR*	m_paMajorStates;
	EMINOR*	m_paMinorStates;
	
public:
	CStateStack();
	~CStateStack();
	
	void	Push( EMAJOR eMajor, EMINOR eMinor );
	BOOL	Pop( EMAJOR* pMajor, EMINOR* pMinor );
	void	Empty( void )
				{ m_cEntries = 0; }
};

#endif //__CMPSTATE_H
