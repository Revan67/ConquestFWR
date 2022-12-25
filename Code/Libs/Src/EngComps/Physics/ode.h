//
// ODE.H - Ordinary Differential Equation solver and related interfaces.
//

#ifndef ODE_H
#define ODE_H

//

#include <memory.h>
#include "dacom.h"

//
// This is the "outgoing" interface for IODESolver. Any class that is
// solvable needs to implement this interface.
//
// Note that although y and ydot are technically functions of time, we will
// rarely encounter ODEs with explicit time dependence in simulation.
//
#define IID_IODE MAKE_IID("IODE",1)

struct IODE
{
// Return length of state vector.
	virtual S32		get_y_length(void) = 0;

// Return state vector at time t.
	virtual void	get_y(SINGLE * dst, SINGLE t) = 0;

// Return time derivative of state vector at time t.
	virtual void	get_dydt(SINGLE * dst, SINGLE * y, SINGLE t) = 0;

// Set state vector.
	virtual void	set_y(SINGLE * src) = 0;

// Each ODE has to keep track of its own timeline.
	virtual SINGLE	get_t(void) const = 0;
	virtual void	set_t(SINGLE time) = 0;

};

//

struct DAODESOLVERDESC : public DACOMDESC
{
	const C8 *	lpImplementation;

	DAODESOLVERDESC(const C8 * _implementation = NULL, const C8 *_interface_name = "IODESolver") : DACOMDESC(_interface_name)
	{
		memset(((C8 *)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));
		lpImplementation = _implementation;
	}
};

//

#define IID_IODESolver MAKE_IID("IODESolver",1)

struct IODESolver : public IDAComponent
{
	virtual void COMAPI solve(IODE * solvable, SINGLE time_step) = 0;
};

//

#endif