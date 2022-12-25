//$Header: /Libs/Src/TXMLib/TXMLibComponent.cpp 26    8/19/98 4:04p Mkness $
//Copyright 1997 (c) Digital Anvil, Inc.

#include "ITXMLib.h"
#include "TXMLib.h"
#include "TComponent.h"
#include "SysConsumerDesc.h"
#include "fdump.h"

#define USE_HEAP 1

#if USE_HEAP
#include "HeapObj.h"
#endif

struct TXMLibrary : public ITXMLib, public IAggregateComponent
{
	protected:

		TXMLib lib;
		bool bSystemComp;

	public:

	//emaurer:  the order of these statements matters.  The template
	//code that implements CreateInstance () expects the name of the object
	//that is being created to be the first in the list.

	BEGIN_DACOM_MAP_INBOUND(TXMLibrary)
	DACOM_INTERFACE_ENTRY(ITXMLib)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	END_DACOM_MAP()

	~TXMLibrary (void)
	{
		if (bSystemComp==0)		// engine comp
		{
			if (PIPE)
				PIPE->Release();
		}
		PIPE = 0;
	}

	GENRESULT init (SYSCONSUMERDESC* desc);

	GENRESULT init (AGGDESC* desc);

	virtual int COMAPI load_library (IFileSystem* file);

	virtual void COMAPI collect_garbage (void);

	virtual TXM_ID COMAPI get_texture_id (const char* txm_name) const;

	virtual void COMAPI add_ref (const TXM_ID& texture);
	virtual void COMAPI release_texture (const TXM_ID& texture);

	virtual GENRESULT COMAPI Initialize (void);

	virtual void COMAPI set_texture_id (const char* txm_name, TXM_ID id);
    virtual void COMAPI get_texture_name (TXM_ID id, char * buffer);

	virtual void COMAPI enable(int state);
	virtual void COMAPI disable(int state);
	virtual BOOL32 COMAPI is_enabled(int state);

	virtual BOOL32 COMAPI is_indexed(TXM_ID id);

	virtual int COMAPI get_red_bits(TXM_ID id);
	virtual int COMAPI get_green_bits(TXM_ID id);
	virtual int COMAPI get_blue_bits(TXM_ID id);
	virtual int COMAPI get_alpha_bits(TXM_ID id);

	virtual int COMAPI get_width(TXM_ID id, int mipmap_level = 0);
	virtual int COMAPI get_height(TXM_ID id, int mipmap_level = 0);

	virtual TXM_ID COMAPI get_entry_id(int id)const;

	//increments ref count for all textures in the axm, if all found
	//currently: a texture's ref count gets inc'd by the number of times
	// the same number of time the texture is used in the animation
	virtual AXM_HANDLE COMAPI get_axm_handle (const char* axm_name);

	//increments reference count by 1, even if you don't know the axm name.
	virtual void COMAPI add_axm_ref(const AXM_HANDLE& handle);

	//decrements ref count for axm; unloads when count reaches zero
	virtual void COMAPI release_axm (const AXM_HANDLE& axm_handle);

	virtual void COMAPI get_axm_name(AXM_HANDLE handle, char * buffer) const;
	virtual void COMAPI get_axm_frame(AXM_HANDLE handle, int frame, AXM_RECT & rect ) const;

	virtual int COMAPI get_axm_frame_count( AXM_HANDLE handle ) const; 

	virtual int COMAPI get_txm_count( void ) const;
	virtual int COMAPI get_axm_count( void ) const;

	virtual void COMAPI enumerate_txms( TAXM_ENUM_CALLBACK, void *context=NULL ) const;
	virtual void COMAPI enumerate_axms( TAXM_ENUM_CALLBACK, void *context=NULL ) const;

	virtual void reset( void );

	IDAComponent * getBase (void)
	{
		return static_cast<ITXMLib *>(this);
	}
};

// we are an engine component
GENRESULT TXMLibrary::init (SYSCONSUMERDESC* info)
{
	bSystemComp = false;
	GENRESULT result = GR_OK;

	result = info->system->QueryInterface("IRenderPipeline", (void**)&PIPE);

	if( result != GR_OK ) {
#if !USE_NWO
		PIPE = NULL;
		GENERAL_TRACE_1( "TXMLIB(Engine):IRenderPipeline not found, using OpenGL\n" );
		result = GR_OK;
#else
		// This is an error otherwise.
		GENERAL_ERROR("TXMLIB(Engine):IRenderPipeline not found\n" );
		result = GR_GENERIC;
#endif
	}
	else {
		GENERAL_TRACE_1( "TXMLIB(Engine):IRenderPipeline found!!\n" );
	}
	return result;
}

// we are a system component
GENRESULT TXMLibrary::init (AGGDESC* info)
{
	bSystemComp = true;
	return GR_OK;
}

void TXMLibrary::reset (void)
{
	lib.reset ();
}

int TXMLibrary::load_library (IFileSystem* file)
{
	return lib.load_library (file);
}

void TXMLibrary::collect_garbage (void)
{
	lib.collect_garbage ();
}

TXM_ID TXMLibrary::get_texture_id (const char* txm_name) const
{
	return lib.get_texture_id (txm_name);
}

void TXMLibrary::add_ref (const TXM_ID& texture)
{
	lib.add_ref (texture);
}

void TXMLibrary::release_texture (const TXM_ID& texture)
{
	lib.release_texture (texture);
}

// we are a system component
GENRESULT TXMLibrary::Initialize (void)
{
	GENRESULT result = GR_OK;

	if (bSystemComp==true)
	{
		if ((result= (getBase()->QueryInterface("IRenderPipeline", (void **)&PIPE))) == GR_OK)
			PIPE->Release();		// hold the pointer without a reference

		if( result != GR_OK ) {
#if !USE_NWO
			PIPE = NULL;
			GENERAL_TRACE_1("TXMLIB(System):IRenderPipeline not found, using OpenGL\n" );
			result = GR_OK;
#else
			// This is an error
			GENERAL_ERROR("TXMLIB(System):IRenderPipeline not found\n" );
			result = GR_GENERIC;
#endif
		}
		else {
			GENERAL_TRACE_1( "TXMLIB(System):IRenderPipeline found!!\n" );
		}
	}
	return result;
}

void TXMLibrary::set_texture_id (const char* txm_name, TXM_ID id)
{
	lib.set_texture_id(txm_name, id);
}

void TXMLibrary::get_texture_name (TXM_ID id, char * buffer)
{
    lib.get_texture_name(id, buffer);
}

void TXMLibrary::enable(int state)
{
	lib.enable(state);
}

void COMAPI TXMLibrary::disable(int state)
{
	lib.disable(state);
}


BOOL32 COMAPI TXMLibrary::is_enabled(int state)
{
	return lib.is_enabled(state);
}

//

BOOL32 COMAPI TXMLibrary::is_indexed(TXM_ID id)
{
	return lib.is_indexed(id);
}

//

int COMAPI TXMLibrary::get_red_bits(TXM_ID id)
{
	return lib.get_red_bits(id);
}

//

int COMAPI TXMLibrary::get_green_bits(TXM_ID id)
{
	return lib.get_green_bits(id);
}

//

int COMAPI TXMLibrary::get_blue_bits(TXM_ID id)
{
	return lib.get_blue_bits(id);
}

//

int COMAPI TXMLibrary::get_alpha_bits(TXM_ID id)
{
	return lib.get_alpha_bits(id);
}

//

int COMAPI TXMLibrary::get_width(TXM_ID id, int mipmap_level)
{
	return lib.get_width(id, mipmap_level);
}

//

int COMAPI TXMLibrary::get_height(TXM_ID id, int mipmap_level)
{
	return lib.get_height(id, mipmap_level);
}

//

TXM_ID COMAPI TXMLibrary::get_entry_id(int id)const
{
	return lib.get_entry_id(id);
}

AXM_HANDLE COMAPI TXMLibrary::get_axm_handle (const char* axm_name)
{
	return lib.get_axm_handle( axm_name );
}

void COMAPI TXMLibrary::add_axm_ref(const AXM_HANDLE& handle)
{
	lib.add_axm_ref( handle );
}

void COMAPI TXMLibrary::release_axm (const AXM_HANDLE& handle)
{
	lib.release_axm( handle );
}

void COMAPI TXMLibrary::get_axm_name(AXM_HANDLE handle, char * buffer) const
{
	lib.get_axm_name( handle, buffer );
}

void COMAPI TXMLibrary::get_axm_frame(AXM_HANDLE handle, int frame, AXM_RECT & rect ) const
{
	lib.get_axm_frame( handle, frame, rect );
}

int COMAPI TXMLibrary::get_axm_frame_count( AXM_HANDLE handle ) const
{
	return lib.get_axm_frame_count( handle );
}

int COMAPI TXMLibrary::get_txm_count( void ) const
{
	return lib.get_txm_count();
}

int COMAPI TXMLibrary::get_axm_count( void ) const
{
	return lib.get_axm_count();
}

void COMAPI TXMLibrary::enumerate_txms( TAXM_ENUM_CALLBACK cb, void *context) const
{
	lib.enumerate_txms( cb,context );
}

void COMAPI TXMLibrary::enumerate_axms( TAXM_ENUM_CALLBACK cb, void *context) const
{
	lib.enumerate_axms( cb,context  );
}

//

#if USE_HEAP
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

void main (void)
{
}
#endif

void ShutdownScratchHeap (void);
void StartupScratchHeap (void);

BOOL COMAPI DllMain (HINSTANCE hinstDLL,
						DWORD     fdwReason,
						LPVOID    lpvReserved)
{
	if (DLL_PROCESS_ATTACH == fdwReason)
	{
		#if USE_HEAP
		HEAP = HEAP_Acquire();
		SetDllHeapMsg(hinstDLL);
		#endif

		StartupScratchHeap ();		

		IComponentFactory *server = new DAComponentFactory2<DAComponentAggregate<TXMLibrary>, SYSCONSUMERDESC> ("ITXMLib");

		ASSERT (server);

		//
		// Register at environment-renderer priority
		//

		DACOM_Acquire ()->RegisterComponent(server, "ITXMLib", DACOM_NORMAL_PRIORITY);

		server = new DAComponentFactory2<DAComponentAggregate<TXMLibrary>, AGGDESC> ("ITXMLib");

		DACOM_Acquire ()->RegisterComponent(server, "ITXMLib", DACOM_NORMAL_PRIORITY);

		server->Release();
	}
	else if (DLL_PROCESS_DETACH == fdwReason)
		ShutdownScratchHeap ();

	return TRUE;
}
