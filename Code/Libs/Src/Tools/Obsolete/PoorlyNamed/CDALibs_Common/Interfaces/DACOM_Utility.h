// DACOM_Utility.h
//
//
//

#ifndef DACOM_UTILITY_H
#define DACOM_UTILITY_H

//

#include "dacom.h"
#include "fdump.h"
#include "engine.h"
#include "system.h"

// define some convienence macros

#define DACOM_INTERFACE_METHOD(name,params) virtual HRESULT COMAPI name params = 0;
#define DACOM_INTERFACE_METHOD_DECL(name,params) HRESULT COMAPI name params;
#define DACOM_INTERFACE_METHOD_IMPL(name,params) HRESULT COMAPI DACOM_COMPONENT_NAME :: name params

#define DACOM_QUERYINTERFACE_BEGIN(out_p,supported_if)	\
	if( strcmp( IID_IDAComponent, interface_name ) == 0 ) { \
		*out_p = static_cast<IDAComponent*>( static_cast<supported_if*>(this) );\
		AddRef();\
		return GR_OK;\
	}
		
#define DACOM_QUERYINTERFACE_ENTRY(out_p,iff) \
	else if( strcmp( IID_ ## iff, interface_name ) == 0 ) { \
		*out_p = static_cast<iff*>(this); \
		AddRef(); \
		return GR_OK; \
	}

#define DACOM_QUERYINTERFACE_END(out_p) \
	*out_p = NULL; \
	return GR_INTERFACE_UNSUPPORTED;
 

#define dacom_implements			public
#define dacom_interface(ifname)		struct DACOM_NO_VTABLE ifname : public IDAComponent
#define dacom_component				struct


#define RELEASE(iff) if( (iff) ) { (iff)->Release(); iff = NULL; }

// define some useful errors
//
#define DACOM_FACILITY 0xD0A
#define MAKE_DACOM_HRESULT( code )	MAKE_HRESULT(1,DACOM_FACILITY,code)

#define E_DACOM_READONLY	MAKE_DACOM_HRESULT( 0x100 )

//

struct CLSID_DACOMDESC : public DACOMDESC
{
	const C8 *component_name;
	COMPTR<ISystemContainer> _ISystem;
	COMPTR<IEngine> _IEngine;
	struct IDAComponent *  outer;
	struct IDAComponent ** inner;		
	
	CLSID_DACOMDESC( const C8 *_component_name, 
					 ISystemContainer *isystem=NULL, 
					 IEngine *iengine=NULL,
					 IDAComponent *_outer = NULL,
					 IDAComponent **_inner = NULL ) : DACOMDESC( "IDAComponent" )
	{
		component_name = _component_name;
		_ISystem = isystem;
		if( isystem == NULL ) {
			GENERAL_WARNING( "System pointer is null in CLSID_DACOMDESC" );
		}
		_IEngine = iengine;
		if( iengine == NULL ) {
			GENERAL_WARNING( "Engine pointer is null in CLSID_DACOMDESC" );
		}

		outer = _outer;
		inner = _inner;

		size = sizeof(*this);
	}
};

//

template< class TypeToMake, class TypeDescriptorType > 
static GENRESULT T_DACOM_CreateInstance( const char *TypeComponentClass, DACOMDESC *desc, IDAComponent **out_instance, TypeToMake *dummy_fucking_shit=NULL )
{
	TypeToMake *new_instance = NULL;

	*out_instance = NULL;

	if( desc->size != sizeof(TypeDescriptorType) || 
		desc->size < sizeof(CLSID_DACOMDESC) || 
		strcmp( ((CLSID_DACOMDESC*)desc)->component_name, TypeComponentClass )!=0 ) {
		return GR_GENERIC;
	}

	CLSID_DACOMDESC *clsid = reinterpret_cast<CLSID_DACOMDESC*>(desc);

	// Create an instance 
	//
	if( (new_instance = new TypeToMake( *clsid ) ) == NULL ) {
		return GR_OUT_OF_MEMORY;
	}

	if( FAILED( new_instance->QueryInterface( desc->interface_name, (void **) out_instance ) ) ) {
		delete new_instance;
		return GR_INTERFACE_UNSUPPORTED;
	}

	return GR_OK;
}

//

static const char *IID_IDAComponent = "IDAComponent";
static const char *IID_IComponentFactory = "IComponentFactory";
static const char *IID_ISystemContainer = "ISystemContainer";
static const char *IID_IProfileParser = "IProfileParser";
static const char *IID_ILightManager = "ILightManager";
static const char *IID_ILight = "ILight";
static const char *IID_ITXMLib = "ITXMLib";
//static const char *IID_IModel = "IModel";
//static const char *IID_IRenderPipeline = "IRenderPipeline";
//static const char *IID_IRenderPrimitive = "IRenderPrimitive";
//static const char *IID_IEngine = "IEngine";
//static const char *IID_ICamera = "ICamera";
//static const char *IID_IRenderer = "IRenderer";
//static const char *IID_ = "";

#endif
