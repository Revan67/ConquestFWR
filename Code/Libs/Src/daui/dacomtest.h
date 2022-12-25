#ifndef DACOMTEST_H
#define DACOMTEST_H
//
// DACOMTest.h - Interface definitions for DACOMTest.cpp
//

//
// Include files
//

#include <dacom.h>

//
// Interface definitions
//

struct IFoo : public IDAComponent
{
	virtual GENRESULT COMAPI fooSet (int _foo) = 0;
	virtual int COMAPI fooGet () = 0;
};

struct IBar : public IDAComponent
{
	virtual GENRESULT COMAPI barSet (int _bar) = 0;
	virtual int COMAPI barGet () = 0;
};


#endif