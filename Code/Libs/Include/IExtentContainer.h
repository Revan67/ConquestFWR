// IExtentContainer.h
//
//
//
//

#ifndef __IExtentContainer_h__
#define __IExtentContainer_h__

//

#include "DACOM.h"
#include "Extent.h"

//

#define IID_IExtentContainer  MAKE_IID( "IExtentContainer", 1 )

//

// ...........................................................................
//
// IExtentContainer
//
//
struct IExtentContainer : public IDAComponent
{
	virtual bool COMAPI get_extents_tree( BaseExtent **out_Extent ) = 0;
	virtual bool COMAPI set_extents_tree( BaseExtent *Extent ) = 0;

	// TODO: add intersection stuff
};



#endif // EOF

