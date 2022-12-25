//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

#include "da_heap_utility.h"
#include "tcomponent.h"
#include "ode.h"

//

HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

C8 *interface_name		= "IODESolver";		// Interface name used for registration     
C8 *implementation_name	= "RK4";			// Implementation name.

//

struct DACOM_NO_VTABLE RK4Solver : public IODESolver
{
	BEGIN_DACOM_MAP_INBOUND(RK4Solver)
	END_DACOM_MAP()

	int		state_len;
	SINGLE *state;
	SINGLE *k1;
	SINGLE *k2;
	SINGLE *k3;
	SINGLE *k4;
	SINGLE *work;

	void alloc_buffers(void);
	void free_buffers(void);

	RK4Solver(void);
	~RK4Solver(void);

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
		if ((*instance = new DAComponent<RK4Solver>) != NULL)
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

RK4Solver::RK4Solver(void)
{
	state_len	= 0;
	state		=
	k1			=
	k2			=
	k3			=
	k4			=
	work		= NULL;
}

//
// Allow warning-free conversion from 'const double'
//
#pragma warning(disable : 4244 4305)

//

void RK4Solver::alloc_buffers(void)
{
	state	= new SINGLE[state_len];
	k1		= new SINGLE[state_len];
	k2		= new SINGLE[state_len];
	k3		= new SINGLE[state_len];
	k4		= new SINGLE[state_len];
	work	= new SINGLE[state_len];
}

//

void RK4Solver::free_buffers(void)
{
	delete [] state;
	state = NULL;
	delete [] k1;
	k1 = NULL;
	delete [] k2;
	k2 = NULL;
	delete [] k3;
	k3 = NULL;
	delete [] k4;
	k4 = NULL;
	delete [] work;
	work = NULL;
}

//

RK4Solver::~RK4Solver(void)
{
	if (state)
	{
		free_buffers();
	}
}

//
// And now, the one thing that RK4Solver knows how to do.
//

inline static void ScaleVector(SINGLE * vec, int len, SINGLE scale)
{
	SINGLE * s = vec;
	for (int i = 0; i < len; i++, s++)
	{
		*s *= scale;
	}
}

//

void RK4Solver::solve(IODE * solvable, SINGLE time_step)
{
	SINGLE time = solvable->get_t();

	int len = solvable->get_y_length();
	if (len)
	{

	// If we don't have enough room, start over.
		if (state_len < len)
		{
			free_buffers();
		}

		if (!state)
		{
		// Assume some growth.
			state_len = (int) floor((SINGLE(len) * 1.5) + 0.5);
			alloc_buffers();
		}

		SINGLE half_time_step = time_step / 2.0;

	// Evaluate k1.
		solvable->get_y(state, time);
		solvable->get_dydt(k1, state, time);
		ScaleVector(k1, len, time_step);

	// Evaluate k2.
		for (int i = 0; i < len; i++)
		{
			work[i] = state[i] + 0.5 * k1[i];
		}
		solvable->get_dydt(k2, work, time + half_time_step);
		ScaleVector(k2, len, time_step);

	// Evaluate k3.
		for (i = 0; i < len; i++)
		{
			work[i] = state[i] + 0.5 * k2[i];
		}
		solvable->get_dydt(k3, work, time + half_time_step);
		ScaleVector(k3, len, time_step);

	// Evaluate k4.
		for (i = 0; i < len; i++)
		{
			work[i] = state[i] + k3[i];
		}
		solvable->get_dydt(k4, work, time + time_step);
		ScaleVector(k4, len, time_step);

	// Evaluate new state.
		SINGLE one_sixth = 1.0 / 6.0;
		for (i = 0; i < len; i++)
		{
			work[i] = state[i] + one_sixth * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
		}

	// Set new state.
		solvable->set_y(work);
	}

// Update time.
	time += time_step;
	solvable->set_t(time);
}


