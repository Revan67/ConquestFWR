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
C8 *implementation_name	= "Euler";			// Implementation name.

//

struct DACOM_NO_VTABLE EulerSolver : public IODESolver
{
	BEGIN_DACOM_MAP_INBOUND(EulerSolver)
	END_DACOM_MAP()

	int		state_len;
	SINGLE *state;
	SINGLE *deriv;

	EulerSolver(void);
	~EulerSolver(void);

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
		if ((*instance = new DAComponent<EulerSolver>) != NULL)
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

EulerSolver::EulerSolver(void)
{
	state_len	= 0;
	state		= NULL;
	deriv		= NULL;
}

//

EulerSolver::~EulerSolver(void)
{
	if (state)
	{
		delete [] state;
		state = NULL;
		delete [] deriv;
		deriv = NULL;
	}
}

//
// And now, the one thing that EulerSolver knows how to do.
//

void EulerSolver::solve(IODE * solvable, SINGLE time_step)
{
	SINGLE time = solvable->get_t();

	int len = solvable->get_y_length();
	if (len)
	{

	// If we don't have enough room, start over.
		if (state_len < len)
		{
			delete [] state;
			state = NULL;
			delete [] deriv;
			deriv = NULL;
		}

		if (!state)
		{
		// Assume some growth.
			state_len = (int) floor((SINGLE(len) * 1.5) + 0.5);
			state = new SINGLE[state_len];
			deriv = new SINGLE[state_len];
		}

		solvable->get_y(state, time);
		solvable->get_dydt(deriv, state, time);

		SINGLE * s = state;
		SINGLE * d = deriv;
		for (int i = 0; i < len; i++, s++, d++)
		{
			*s += *d * time_step;
		}

		solvable->set_y(state);
	}

	time += time_step;
	solvable->set_t(time);
}
