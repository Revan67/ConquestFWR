#ifndef STARTUP_H
#define STARTUP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Startup.h                                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Startup.h 1     9/20/98 1:03p Gboswood $
*/			    
//----------------------------------------------------------------------------
/*
	Initialization scheme for starting up global components.

	//------------------------------
	//
	void Startup (void);
		OUTPUT:
			Create global component. Do all initialization possible without the use
			of other global application components. You may assume that all library 
			components have been loaded, and can be used at this point.

	//------------------------------
	//
	void Initialize (void);
		OUTPUT:
			Do any further initialization needed, using other application components.
  
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct GlobalComponent
{
	GlobalComponent (void)
	{
		AddToGlobalStartupList(*this);
	}

	virtual void Startup (void) = 0;

	virtual void Initialize (void) = 0;
};


//----------------------------------------------------------------------------
//---------------------------End Startup.h------------------------------------
//----------------------------------------------------------------------------
#endif
