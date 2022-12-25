//
// DACOMTest.cpp - A simple test of the DACOM library. It also tests the <tcomponent.h> header
//

//
// Include files
//

#include "stdafx.h"
#include <dacom.h>
#include <tcomponent.h>
#include <tsmartpointer.h>
#include <tchar.h>
#include "dacomtest.h"

//
// Compile switches
//

// Set to non-zero to build as DLL.
#define MAKE_DLL 0

//
// Concrete class definitions
//

struct TestComponent : public IFoo, public IBar
{
	// Create a table of interface offsets
	BEGIN_DACOM_MAP_INBOUND(TestComponent)
	DACOM_INTERFACE_ENTRY(IFoo)
	DACOM_INTERFACE_ENTRY(IBar)
	END_DACOM_MAP()

	// Data members
	int foo;
	int bar;

	// Local methods

	TestComponent ();
	virtual ~TestComponent ();

	// Methods required for DAAggregateComponent and DAComponent templates
	GENRESULT init (DACOMDESC * desc);

	// IFoo methods
	virtual GENRESULT COMAPI fooSet (int _foo);
	virtual int COMAPI fooGet ();

	// IBar methods
	virtual GENRESULT COMAPI barSet (int _bar);
	virtual int COMAPI barGet ();

	// NOTE: IDAComponent methods are not implemented here. This means that TestComponent cannot
	// be instantiated. The IDAComponent methods will be added by template code when the factory is registered.
};


TestComponent::TestComponent ()
{
	foo = bar = 0;
}

TestComponent::~TestComponent ()
{
	// Nothing to do
}

GENRESULT COMAPI TestComponent::fooSet (int _foo)
{
	OutputDebugString (_T("Foo set\n"));
	foo = _foo;
	return GR_OK;
}

int COMAPI TestComponent::fooGet ()
{
	OutputDebugString (_T("Foo get\n"));
	return foo;
}

GENRESULT COMAPI TestComponent::barSet (int _bar)
{
	OutputDebugString (_T("Bar set\n"));
	bar = _bar;
	return GR_OK;
}

int COMAPI TestComponent::barGet ()
{
	OutputDebugString (_T("Bar get\n"));
	return bar;
}

GENRESULT TestComponent::init (DACOMDESC * desc)
{
	// Always allow the component to be created.
	return GR_OK;
}

//
// Testing functions
//

bool test_dacom()
{
	// The COMPTR<> template automatically releases the reference when it falls out of scope
	COMPTR<IFoo> testObject;

	ICOManager *DACOM = DACOM_Acquire();
	if (DACOM == NULL)
	{
		return false;
	}

	DACOMDESC desc(_T("IFoo"));

	if (DACOM->CreateInstance (&desc, testObject) != GR_OK)
	{
		return false;
	}

	COMPTR<IBar> testBar;
	if (testObject->QueryInterface(_T("IBar"), testBar) != GR_OK)
	{
		return false;
	}

	if (testObject->fooSet (100) != GR_OK)
	{
		return false;
	}

	if (testObject->fooGet () != 100)
	{
		return false;
	}

	if (testBar->barSet (50) != GR_OK)
	{
		return false;
	}

	if (testBar->barGet () != 50)
	{
		return false;
	}

	return true;
}

//
// DACOM Registration functions.
//

// When a component is stored in a DLL, its DLLMain function will handle the registering of the components.
// Additionally, it will indicate to the heap manager the name of the module, so that the heap knows which
// DLLs are allocating what memory.
// The heap stuff is not done here, but the code is below. To compile as a DLL, change the compile switch
// definition at the top of the file.

void dacomtest_startup()
{
	// Get the DACOM interface. We need this to register the factory
	ICOManager *DACOM = DACOM_Acquire();

	// Create a factory for the template class DAComponent<TestComponent>.
	// DAComponent<> provides the IDAComponent methods.

	IComponentFactory *server;
	server = new DAComponentFactory<DAComponent<TestComponent>,DACOMDESC>(_T("IFoo"));
	DACOM->RegisterComponent(server, _T("IFoo"), DACOM_LOW_PRIORITY);
	server->Release();
}

#if MAKE_DLL
#include <HeapObj.h>

void SetDllHeapMsg (HINSTANCE hInstance)
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

//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL COMAPI DllMain(HINSTANCE hinstDLL,  //)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
   switch (fdwReason)
      {
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:
			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);
			dacomtest_startup ();
			break;

      case DLL_PROCESS_DETACH:
         break;
      }

   return TRUE;
}
#endif
