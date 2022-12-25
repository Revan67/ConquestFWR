#ifndef __SYSCONSUMERDESC_H
#define __SYSCONSUMERDESC_H

#include "DACOM.h"

struct SYSCONSUMERDESC : public AGGDESC
{
	struct IDAComponent * system;		// system component provider	

	SYSCONSUMERDESC (const char* _interface_name = NULL) : AGGDESC(_interface_name)
	{
		size = sizeof(*this);
	}
};

#endif