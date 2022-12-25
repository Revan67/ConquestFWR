//
// Trapezoidal numerical integration scheme. Averages forward and backward
// Euler steps, so is stable and 2nd-order accurate.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdio.h>

#include "fdump.h"

#include "da_heap_utility.h"
#include "tcomponent.h"
#include "ode.h"

//

HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

C8 *interface_name		= "IODESolver";		// Interface name used for registration     
C8 *implementation_name	= "Trapezoidal"; 	// Implementation name.

//

struct DACOM_NO_VTABLE TrapezoidalSolver : public IODESolver
{
	BEGIN_DACOM_MAP_INBOUND(TrapezoidalSolver)
	DACOM_INTERFACE_ENTRY(IODESolver)
	DACOM_INTERFACE_ENTRY2(IID_IODESolver,IODESolver)
	END_DACOM_MAP()

	int		state_len;
	SINGLE *guess;
	SINGLE *prev_state;
	SINGLE *next_state;
	SINGLE *forward_deriv;
	SINGLE *backward_deriv;

	TrapezoidalSolver(void);
	~TrapezoidalSolver(void);

	void free(void)
	{
		delete [] guess;
		guess = NULL;

		delete [] prev_state;
		prev_state = NULL;

		delete [] next_state;
		next_state = NULL;

		delete [] forward_deriv;
		forward_deriv = NULL;

		delete [] backward_deriv;
		backward_deriv = NULL;
	}

	void COMAPI solve(IODE * solvable, SINGLE time_step);
};

//

struct Server : public IComponentFactory
{
	S32	ref_cnt;
   
	Server(void)
	{
		memset(((C8 *) this)+sizeof(U32), 0, sizeof(*this)-sizeof(U32));
		ref_cnt = 1;
	}                            

//
// IComponentFactory methods
//
	DEFMETHOD(QueryInterface)	(const C8 *interface_name, void **instance);
	DEFMETHOD(CreateInstance)	(DACOMDESC *descriptor, void **instance);

	DEFMETHOD_(U32,AddRef)		(void);
	DEFMETHOD_(U32,Release)		(void);
};

//

void SetDllHeapMsg (void)
{
	DWORD dwLen;
	char buffer[260];

	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));

	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	Server *server;

	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			hInstance = hinstDLL;

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			server = new Server;

			if (server == NULL)
			{
				break;
			}

			DACOM = DACOM_Acquire();

		//
		// Register at object-renderer priority
		//
			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
			}

			server->Release();
			break;

	//
	// DLL_PROCESS_DETACH: Release DACOM manager instance
	//
		case DLL_PROCESS_DETACH:

			if (DACOM != NULL)
			{
				DACOM->Release();
				DACOM = NULL;
			}
			break;
	}

	return TRUE;
}

//

U32 Server::AddRef (void)
{
	++ref_cnt;
	return ref_cnt;
}

//

U32 Server::Release(void)
{
	if (ref_cnt > 0)
	{
		--ref_cnt;

		if (ref_cnt == 0)
		{
			delete this;
		}
	}

	return ref_cnt;
}

//

GENRESULT Server::QueryInterface(const C8 *interface_name, void **instance)
{
	GENRESULT result;

	if (strcmp(interface_name, ::interface_name))
	{
		result = GR_INTERFACE_UNSUPPORTED;
	}
	else
	{
		result = GR_OK;
	}

	return result;
}

//

GENRESULT Server::CreateInstance (DACOMDESC *descriptor, void **instance)
{
//
// Assume failure
//
	*instance = NULL;

//
// If unsupported interface or capability requested, fail call
//
	DACOMDESC *info = (DACOMDESC *) descriptor;

	if ((info != NULL) && (info->interface_name != NULL))
	{
		if (info->size != sizeof(*info))
		{
			return GR_INTERFACE_UNSUPPORTED;
		}

		if (strcmp(info->interface_name, interface_name))
		{
			return GR_INTERFACE_UNSUPPORTED;
		}
	}

	GENRESULT result;

	DAODESOLVERDESC * d = (DAODESOLVERDESC *) info;
	if ((d->lpImplementation != NULL) && strcmp(d->lpImplementation, implementation_name))
	{
		result = GR_GENERIC;
	}
	else 
	{
		if ((*instance = new DAComponent<TrapezoidalSolver>) != NULL)
		{
			result = GR_OK;
		}
		else
		{
			result = GR_GENERIC;
		}
	}

	return result;
}

//

TrapezoidalSolver::TrapezoidalSolver(void)
{
	state_len		= 0;
	guess			= NULL;
	prev_state		= NULL;
	next_state		= NULL;
	forward_deriv	= NULL;
	backward_deriv	= NULL;
}

//

static int MaxIterations = 0;
static int ForwardSteps = 0;

//

TrapezoidalSolver::~TrapezoidalSolver(void)
{
	free();
}

//
// And now, the one thing that TrapezoidalSolver knows how to do.
//
void TrapezoidalSolver::solve(IODE * solvable, SINGLE time_step)
{
	SINGLE time = solvable->get_t();

	int len = solvable->get_y_length();
	if (len)
	{

	// If we don't have enough room, start over.
		if (state_len < len)
		{
			free();
		}

		if (!prev_state)
		{
		// Assume some growth.
			state_len = (int) floor((SINGLE(len) * 1.5) + 0.5);
			guess			= new SINGLE[state_len];
			prev_state		= new SINGLE[state_len];
			next_state		= new SINGLE[state_len];
			forward_deriv	= new SINGLE[state_len];
			backward_deriv	= new SINGLE[state_len];
		}

	//
	// Get forward Euler derivative.
	//
		solvable->get_y(prev_state, time);
		solvable->get_dydt(forward_deriv, prev_state, time);

	//
	// Iteratively compute backward Euler step.
	//
		memcpy(guess, prev_state, sizeof(SINGLE) * len);

	#define EPSILON (1e-4)

		int iteration_count = 0;
		bool forward_only = false;

		SINGLE *x0, *x1, *f;
#if 1
		forward_only = true;

#else
		bool done = false;
		while (!done)
		{
		// Compute LHS using current guess at state.
			solvable->get_dydt(backward_deriv, guess, time);

			x0	= prev_state;
			x1	= next_state;
			f	= backward_deriv;
			for (int i = 0; i < len; i++, x0++, x1++, f++)
			{
				*x1 = *x0 + time_step * *f;
			}

		// How much did next_state change?
			done = true;
			x0 = next_state;
			x1 = guess;
			for (i = 0; i < len; i++, x0++, x1++)
			{
				if (fabs(*x0 - *x1) >= EPSILON)
				{
					done = false;
					break;
				}
			}

		// Prepare for next iteration.
			if (!done)
			{
				memcpy(guess, next_state, sizeof(SINGLE) * len);
				iteration_count++;
			}

			if (iteration_count > 32)
			{
			// Too many iterations. For some reason we're not converging, possibly
			// a local minimum. Instead of returning some useless result, just
			// ignore the backward step.
				forward_only = true;
				done = true;
			}
		}

		if (iteration_count > MaxIterations)
		{
			MaxIterations = iteration_count;
		}
#endif

		f = forward_deriv;
		SINGLE * b = backward_deriv;
		x0 = prev_state;
		x1 = guess;

		if (forward_only)
		{
		// Backward step didn't converge, use forward step only.
			for (int i = 0; i < len; i++, x0++, x1++, f++)
			{
				*x1 = *x0 + time_step * (*f);
			}

			ForwardSteps++;
		}
		else
		{
		// Average forward and backward steps.
			SINGLE h = time_step * SINGLE(0.5);
			for (int i = 0; i < len; i++, x0++, x1++, f++, b++)
			{
				*x1 = *x0 + h * ((*f) + (*b));
			}
		}

		solvable->set_y(guess);
	}

	time += time_step;
	solvable->set_t(time);
}
