// IDDBackDoor.h
//
//
//


#ifndef IDDBACKDOOR_H
#define IDDBACKDOOR_H

#include <ddraw.h>
#include "dacom.h"

typedef enum 
{
	DDBD_P_DIRECTDRAW,
	DDBD_P_PRIMARYSURFACE,
	DDBD_P_BACKSURFACE
} DDBACKDOORPROVIDER;

//

#define IID_IDDBackDoor "IDDBackDoor"

//

struct DACOM_NO_VTABLE IDDBackDoor : public IDAComponent
{
	// It is up to the caller to release the returned IUnknown interface.
	//
	virtual GENRESULT COMAPI get_dd_provider( DDBACKDOORPROVIDER provider,  IUnknown ** pUnknown ) = 0;
};


#endif // EOF

