#ifndef DACOM_H
#define DACOM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DACOM.H                                     //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/

/*
   This header file describes the Digital Anvil Component Object Model 
   and the DA Component Manager.

   DA COM is similar to MS COM, except the QueryInterface() method takes an ASCIIZ
   string that identifies the requested interface, rather than a GUID.

   Components that export the IComponentFactory interface are able to create instances
   of component objects. IComponentFactory inherits from IDAComponent, and adds
   one new method: CreateInstance. 

   CreateInstance is used when a new component object is desired that supports
   a particular interface. QueryInterface is used when you already have a component
   instance, and you want to know if it supports an additional interface.

   The successful completion of CreateInstance is conditional on 

      (1) the current state of the IComponentFactory object, and
      (2) the data items passed in the information parameter.

   The contents of a 'description' structure passed to the object vary 
   depending on the interface definition. Each interface derived from 
   IComponentFactory may define an information structure derived from DACOMDESC.  
   Clients should properly construct this structure, setting the 'size' member 
   to the size of the derived struct. The interface designer may include 
   initialization helper functions to properly set up the structure.

   struct DACOMDESC
   {
      U32 size;            // aggregate size of the structure
      C8 *interface_name;  // ASCIIZ name of requested interface
   };

   The client uses the description structure to describe what type of 
   component (interface) it wants, and (optionally) which component to create
   if there is more than one implementation available.  It may also supply 
   information on how to create the new component instance.  The base 
   structure, DACOMDESC, only describes the interface desired. Any additional
   information would have to be added to a derived struct.

   All DA Component Objects share a common base interface, IDAComponent. 
   IDAComponent defines 3 methods, described below:
      
   //-----------------------------
   // IDAComponent Methods
   //-----------------------------

   //-----------------------------
   //
   GENRESULT QueryInterface (const C8 *interface_name, void **instance);
		INPUT:
			interface_name: ASCIIZ name of the interface requested.
		OUTPUT:
         instance: Address of a pointer to the requested interface.  If the 
            component does not support the interface, *instance will be set to 
            NULL.
		NOTES:
			If successful, the reference count of the interface has already been
			incremented. The calling code should call Release() when it is done
			with the interface pointer.
			Classes that support a single interface may return failure on all 
			interface queries. For example code for single interface class:

			{
				*instance = 0;
				return GR_INTERFACE_UNSUPPORTED;
			}

			For multiple interface classes, every supported interface (even the default interface)
			must be accessible through QueryInterface().

   //-----------------------------
   //
   U32 AddRef (void);
		OUTPUT:
			Increments the reference count on an instance implementing the interface.
		RETURNS:
			The new reference count. This value is for debugging. You should not
			rely on this value.
  
	
   //-----------------------------
   //
   U32 Release(void);
		OUTPUT:
			Decrements the reference count of an instance implementing the interface. If 
			the reference count reaches zero, the instance frees all of its resources
			and removes itself from memory.
		RETURNS:
			The new reference count. This value is for debugging. You should not
			rely on this value.


   IComponentFactory adds one method, described below:
      
   //-----------------------------
   // IComponentFactory Methods
   //-----------------------------

   GENRESULT CreateInstance (DACOMDESC *descriptor, void **instance);
      INPUT:
         descriptor: Address of a component description structure.
         'descriptor' can be NULL if 
			1) The component supports only one interface, and
			2) the interface can create an instance without any parameters.
		 If 'descriptor' is non-NULL, it must point to a valid DACOMDESC structure.
		 (the 'size' member must be properly initialized, and 'interface_name'
		  must point to a valid ASCIIZ name. 'interface_name' can never be NULL!!)

      OUTPUT:
         instance: Address of a pointer to the requested interface.  If the 
            component object cannot be created, *instance will be set to 
            NULL.

      RETURNS:
         GR_OK if DACOM successfully created an instance of the component that
            was requested. The pointer at 'instance' will be set to the 
            address of the new object. The new object will already have a reference
			count of 1.
         Otherwise, an error code from RESULT.H will be returned, with 
            *instance set to NULL.

   //-----------------------------
   // IAggregateComponent Methods
   //-----------------------------

   GENRESULT Initialize (void);
	  RETURNS:
		 GR_OK if everything initialized ok.
		 otherwise it returns some error code.
	  OUTPUT:
		 The component calls QueryInterface() to find interfaces that it might require from
		 other aggregate members. If the component fails to find a required member,
		 it returns an error.
	  NOTES:
	     This method is called by the container object when all members have been loaded.
	     This method may be called more than once. The component should re-initialize any
		 pointers it may have received in a previous call to Initialize().


   //--------------------------------------------------------------------------//
   //------------------------------DA COM Manager------------------------------//
   //--------------------------------------------------------------------------//

  
   Central to the DA COM is the DA COM Manager. This component is responsible
   for registering/unregistering DA COM components, and routing 
   CreateInstance() calls to registered components. There is only one 
   instance of the manager object, so multiple calls to DACOM_Acquire() 
   will always return the same pointer.

   DA COM components are responsible for registering/unregistering themselves
   with the DACOManager object.

   //-----------------------------
   // ICOManager Methods
   //-----------------------------

   GENRESULT CreateInstance (DACOMDESC *descriptor, void **instance);

      OUTPUT:
         Passes the CreateInstance() message to each registered object until
         one returns GR_OK. (See CreateInstance() description above)
         If 'descriptor' is NULL, '*instance' will be set to the CO Manager 
         object, and the function will return GR_OK.

   //--------------------------
   //
   GENRESULT RegisterComponent (struct IComponentFactory *component, C8 *interface_name, U32 priority);

      INPUT:
         component: Pointer to the DA COM object being registered

         interface_name: Address of a statically-defined ASCIIZ name of the 
                    supported interface. This address must remain valid for 
                    the entire life of the object.

         priority: Calling sequence priority, where higher numbers mean
                    more priority, lower numbers mean lower priority.

      RETURNS:
         GR_OK if successfully registered the object, or some error code
               on failure.

	  CALLED BY:
			The startup code in a component DLL. The DLL creates all of the factory instances
			supported by the module, and registers them with the DACOManager.
  
	  NOTES:
         When a client calls the CreateInstance() method of the 
         DACOManager, the object proceeds to call every registered object
         according to its priority until an object returns G_OK. (Highest
         priority objects are called first.)
         Component objects can use the priority setting to override previous
         implementations of an interface when appropriate.

         The reference count of the component is incremented by this method.

   //--------------------------
   //
   GENRESULT UnregisterComponent (struct IComponentFactory *component, C8 *interface_name);

      INPUT:
         component: Pointer to a DA COM object that has been registered.

         interface_name: ASCIIZ string name of the supported implementation.
                    This may be NULL if any matching component will do, or 
                    if the object only supports one interface.

      RETURNS:

         GR_OK if object was successfully unregistered.
         GR_GENERIC on failure.

	  CALLED BY:
			Any code that needs to remove a factory component from the registered list.

	  NOTES:
         The reference count of the component is decremented by this method.

   //--------------------------
   //
   GENRESULT EnumerateComponents (C8 *interface_name, DACOMENUMCALLBACK callback, void *context);
      INPUT:
      interface_name: Pointer to an ASCIIZ name of an interface. 
                      May be NULL if you want to enumerate 
                      all registered objects.

            callback: Address of an enumeration callback function.

             context: Application-defined data item.

      RETURNS:
         GR_OK if no errors.
         GR_INVALID_PARMS if callback was NULL.

      BOOL32 __cdecl DACOMENUMCALLBACK (struct IComponentFactory *component,
                                        C8          *interface_name,
                                        S32          priority,
                                        void        *context);
         INPUT:
            component: Pointer to a registered component.

            interface_name: ASCIIZ name of the interface supported by the 
                       component. This string will match the 'interface_name' 
                       string you passed to the enumerate function, unless 
                       you specified NULL for interface_name.

             priority: Priority value associated with component.

              context: Application data item.

         RETURNS:
                  The callback function should return 0 to force end of 
                  enumeration, non-zero to continue

         NOTES: 
            The reference count for the components is not changed by this 
            function. If you are saving a copy of an instance pointer 
            outside the scope of the enumeration function, you should 
            call the instance's AddRef() method, and then call the 
            instance's Release() method when you are done with the instance.

   //--------------------------
   //
   GENRESULT AddLibrary (C8 *DLL_filename);

      INPUT:
         DLL_filename: ASCIIZ name of DLL file to load.

      RETURNS:
         GR_OK if library was loaded,
         GR_GENERIC if failed for unknown reasons. (File not found?)
         GR_OUT_OF_SPACE if too many libraries have been loaded already

   //--------------------------
   //
   GENRESULT RemoveLibrary (C8 *DLL_filename);
      INPUT:
         DLL_filename: ASCIIZ name of DLL file to unload.

      RETURNS:
         GR_OK if library was unloaded,
         GR_GENERIC if failed for unknown reasons. 

      NOTES: 
         Removes libraries that were loaded via the AddLibrary() method. 
         For each successfull call to AddLibrary() on the same DLL, there 
         should be the same number of calls to RemoveLibrary() to fully 
         remove it from the system.

   //--------------------------
   //
   GENRESULT ShutDown (void);
	   OUTPUT:
		 Unregisters all registered components, unloads all DLL modules.
       NOTES:
		  This method should be called by the application during its exit procedure.
		  After this method is called, dynamicaly loaded components will be unavailable.
		  Windows will NOT automatically unload the DLLs used by the component system.
		  Failure to properly shut down the CO manager will leave DLLs in 
		  memory after the application quits.


   //--------------------------
   //
   GENRESULT SetINIConfig (const C8 *info, U32 flags);
	   INPUT:
		  info: ASCIIZ string, meaning determined by 'flags'. If no flags specified, 'info'
				points to the name of the INI file to open.
		  flags:
			 DACOM_INI_WRITABLE:	Open the INI file with read/write access.
			 DACOM_INI_STRING:		'info' points to a buffer containing INI data. This flag 
									may not be used with DACOM_INI_WRITABLE.

	   RETURNS:
		  GR_OK if INI was parsed correctly
		  GR_INVALID_PARMS if 'info' is not a valid file (and DACOM_INI_STRING was not used.)
	   OUTPUT:
          Sets 'info' to be the data used to initialize DACOM.
          When CreateInstance() is called the first time, the
          DACOM manager parses the INI data, looking for the [Libraries] section.
		  If the section is not found, it loads *.DLL from the directory specified in the
		  DACOM environment path. 
	

*/
//--------------------------------------------------------------------------//


#ifdef WIN32
  #define IS_WIN32 1
#endif

#ifdef _WIN32
  #define IS_WIN32 1
#endif

//
// If compiling DACOM library, use __declspec(dllexport) for both 
// declarations and definitions
//
// If compiling DACOM application, use __declspec(dllimport) for
// declarations -- definitions don't matter
//

#ifdef IS_WIN32

#undef DXDEC
#undef DXDEF

  #ifdef BUILD_DACOM
    #define DXDEC __declspec(dllexport)
    #define DXDEF __declspec(dllexport)
  #else
    #define DXDEC __declspec(dllimport)
  #endif

#else
  #error Must define WIN32 or _WIN32 to use DACOM.H
#endif

//#include <memory.h>

#ifndef US_TYPEDEFS
#include "typedefs.h"      // General type definitions
#endif

#ifndef RESULTS_H
#include "results.h"       // General result codes
#endif

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef COMAPI
#define COMAPI __stdcall
#endif

#ifndef DEFMETHOD
#define DEFMETHOD(method)         virtual GENRESULT COMAPI method
#endif

#ifndef DEFMETHOD_
#define DEFMETHOD_(type,method)   virtual type COMAPI method
#endif

#ifndef DACOM_NO_VTABLE	
  #if (_MSC_VER < 1100)      // 1100 = VC++ 5.0
    #define DACOM_NO_VTABLE
  #else
    #define DACOM_NO_VTABLE __declspec(novtable)
  #endif
#endif

//
// Interface versioning macro
//
#define MAKE_IID(name,ver) name "__" #ver

//--------------------------------------------------------------------------//
//----------------------------IDAComponent Interface------------------------//
//--------------------------------------------------------------------------//

//
// Self-initializing structure used by clients to request a desired DA COM
// implementation
//

struct DACOMDESC
{
	U32 size;                  // size of the structure + any extension
	const C8 *interface_name;  // ASCIIZ name of requested interface
	
	DACOMDESC (const C8 * _interface_name = 0)
	{
		size = sizeof(*this);
		interface_name = _interface_name;
	}
};

//
// Self-initializing structure used by clients to request a desired DA COM
// implementation. AGGDESC is used by components that support aggregation.
// A successfull call to CreateInstance() using an AGGDESC will return a pointer
// to an IAggregateComponent interface. Use QueryInterface() to retrieve more
// specific interface pointers. 
//

struct AGGDESC : public DACOMDESC
{
	struct IDAComponent *  outer;
	struct IDAComponent ** inner;		
	const  C8           *  description;
	
	AGGDESC (const C8 *_interfaceName = 0,
		const C8 * _description = 0) : DACOMDESC(_interfaceName)
	{
		outer = 0;
		inner = 0;
		description = _description;
		size = sizeof(*this);
	}
};

//
// Abstract base class from which all DA component classes must be derived
//

#define IID_IDAComponent "IDAComponent"

struct DACOM_NO_VTABLE IDAComponent
{
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance) = 0;
	
	DEFMETHOD_(U32,AddRef)           (void) = 0;
	
	DEFMETHOD_(U32,Release)          (void) = 0;
	
	inline GENRESULT QueryOutgoingInterface (const C8 *connectionName, struct IDAConnectionPoint **connection);
};

//

#define DACOM_RELEASE(x) if((x)) { (x)->Release(); (x) = NULL; }

//

//
// Abstract class from which all class factories inherit
//

#define IID_IComponentFactory "IComponentFactory"

struct DACOM_NO_VTABLE IComponentFactory : public IDAComponent
{
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance) = 0;
};

//
// Abstract class from which all aggregatable classes inherit
//

#define IID_IAggregateComponent "IAggregateComponent"

struct DACOM_NO_VTABLE IAggregateComponent : public IDAComponent
{
	DEFMETHOD(Initialize) (void) = 0;
};

//--------------------------------------------------------------------------//
//---------------------------ICOManager Interface---------------------------//
//--------------------------------------------------------------------------//

//-------------------------------------
// Priorities for RegisterComponent
//-------------------------------------
#define DACOM_HIGH_PRIORITY   0xC0000000
#define DACOM_NORMAL_PRIORITY 0x80000000
#define DACOM_LOW_PRIORITY    0x40000000

//-------------------------------------
// callback prototype for EnumerateComponents
//-------------------------------------
typedef BOOL32 (__cdecl * DACOMENUMCALLBACK)(IComponentFactory *component,
                                             const C8    *interface_name,
                                             U32           priority,
                                             void         *context);

//-------------------------------------
// flags for SetINIConfig
//-------------------------------------
#define DACOM_INI_WRITABLE	0x00000001
#define DACOM_INI_STRING	0x00000002


struct DACOM_NO_VTABLE ICOManager : public IComponentFactory
{
	DEFMETHOD(RegisterComponent)	  (IComponentFactory *component, 
		const C8     *interface_name, 
		U32           priority = DACOM_NORMAL_PRIORITY) = 0;
	
	DEFMETHOD(UnregisterComponent) (IComponentFactory *component, 
		const C8     *interface_name = 0) = 0;
	
	DEFMETHOD(EnumerateComponents) (const C8     *interface_name, 
		DACOMENUMCALLBACK callback, 
		void             *context = 0) = 0;
	
	DEFMETHOD(AddLibrary)          (const C8 *DLL_filename) = 0;
	
	DEFMETHOD(RemoveLibrary)       (const C8 *DLL_filename) = 0;
	
	DEFMETHOD(ShutDown)            (void) = 0;
	
	DEFMETHOD(SetINIConfig)        (const C8 *info, U32 flags=0) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifdef __cplusplus
extern "C" 
{
#endif
	
	//
	// All clients of DACOM (including component objects as well as the
	// application itself) must call DACOM_Acquire() to obtain an instance
	// pointer to the DA Component Manager
	//
	
	DXDEC ICOManager * __cdecl DACOM_Acquire(void);

	// This allows clients to retrieve the version information about a DLL.
	// The version information is pulled out of the product version resource in the DLL and
	// returned.
	//
	DXDEC GENRESULT DACOM_GetDllVersion( const char *dll_name, U32 *out_major, U32 *out_minor, U32 *out_build );

	// This allows clients to retrieve DACOM information before acquiring/initializing
	// DACOM itself.  This loads the version out of the product version resource of the 
	// DACOM.dll that will be/would be/was used when the application calls DACOM_Acquire()
	//
	DXDEC GENRESULT DACOM_GetVersion( U32 *out_major, U32 *out_minor, U32 *out_build );

	
#ifdef __cplusplus
}
#endif

#endif

//--------------------------------------------------------------------------//
//-------------------------------END DACOM.H--------------------------------//
//--------------------------------------------------------------------------//
