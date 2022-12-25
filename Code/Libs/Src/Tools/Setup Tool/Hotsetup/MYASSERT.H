//
// myassert.h
//

#ifndef __MYASSERT_H
#define __MYASSERT_H


#define assert ASSERT
#define verify VERIFY

#ifdef 	_DEBUG

	#define ASSERT(f)			((f) ? (void)0 : AssertFailedLine(__FILE__, __LINE__))
	#define VERIFY(f)			(ASSERT(f))

	void AssertFailedLine(LPCSTR lpszFileName, int nLine);

#else	// _DEBUG

	#define ASSERT(f)			((void)0)
	#define VERIFY(f)			(f)

#endif	// _DEBUG



#endif //__MYASSERT_H
