// DALibs_Decorator.cpp: implementation of the CDALibs_Decorator class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>

#include "dacom.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "rendpipeline.h"
#include "ICamera.h"
#include "extent.h"

#include "IPersistable.h"
#include "IRenderable.h"
#include "ISimulatable.h"
#include "IDecorator.h"
#include "IGeoTransformable.h"
#include "IDACOMEngineInstance.h"
#include "DACOM_Utility.h"

#define DACOM_COMPONENT_NAME CDALibs_Decorator

dacom_component CDALibs_Decorator :  dacom_implements IRenderable,
									 dacom_implements ISimulatable,
									 dacom_implements IDecorator,
									 dacom_implements IComponentFactory
{

	// IRenderable
	DACOM_INTERFACE_METHOD_DECL( Render,			(ILowLevelCamera *IC));
	
	// ISimulatable
	DACOM_INTERFACE_METHOD_DECL( Import,			());
	DACOM_INTERFACE_METHOD_DECL( Update,			());

	// IDecorator
	DACOM_INTERFACE_METHOD_DECL( SetDecorated,		(IDAComponent  *idacomponent));
	DACOM_INTERFACE_METHOD_DECL( GetDecorated,		(IDAComponent **idacomponent));
	DACOM_INTERFACE_METHOD_DECL( Show,				( U32 flag, U32 yesno ));
	DACOM_INTERFACE_METHOD_DECL( ShowExtents,		( ExtentType type, U32 yesno ));


	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// C++ Interface
	CDALibs_Decorator( CLSID_DACOMDESC &creation_info );
	~CDALibs_Decorator();

protected:	// Component Data
	COMPTR<IDAComponent>		m_Decorated;
	COMPTR<IEngine>				m_IEngine;
	COMPTR<ISystemContainer>	m_ISystem;
	U32							m_ShowFlags;
	int							m_RefCnt;
};

//

HRESULT RegisterCDALibs_Decorator( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( "CDALibs_Decorator" );
	if( T_DACOM_CreateInstance<CDALibs_Decorator,CLSID_DACOMDESC>( "CDALibs_Decorator", &desc, (IDAComponent **) &IDAC ) == S_OK ) {
		COMPTR<IComponentFactory> ICF;
		if( SUCCEEDED( IDAC->QueryInterface( IID_IComponentFactory, (void**) &ICF ) ) ) {
			dacom->RegisterComponent( ICF, IID_IDAComponent );	// NOTE: DACOM requires us to register an 'interface 
																// provider' not a component!  EEG
			return S_OK;
		}
	}
	return E_FAIL;
}

//

#define SF_SHOW_EXTENT(type)	(1<<(16+type))

//

DACOM_INTERFACE_METHOD_IMPL( Render,(ILowLevelCamera *IC ))
{
	if( m_Decorated == NULL ) {
		return E_FAIL;
	}

	COMPTR<IRenderPipeline> IRP;
	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPipeline, (void**) &IRP ) ) ) {
		return E_FAIL;
	}

#if 0
	// Render decorated object
	//
	U32 rp_texture;
	if( !(m_ShowFlags & SF_SHOW_TEXTURES) ) {
		IRP->get_pipeline_state( RP_TEXTURE, &rp_texture );	
		IRP->set_pipeline_state( RP_TEXTURE, FALSE );	
	}

	COMPTR<IRenderable> IR;
	if( SUCCEEDED( m_Decorated->QueryInterface( IID_IRenderable, (void**) &IR ) ) ) {
		IR->Render( IC );
	}

	// Render decorator stuff
	//
	COMPTR<IGeoTransformable> IGT;
	COMPTR<IDACOMEngineInstance> IEI;
	Transform object_to_world(0);
	Mesh *object_mesh;

	if( FAILED( m_Decorated->QueryInterface( IID_IGeoTransformable, (void**) &IGT ) ) ) {
		goto id_render_exit_error;
	}

	if( FAILED( m_Decorated->QueryInterface( IID_IDACOMEngineInstance, (void**) &IEI ) ) ) {
		goto id_render_exit_error;
	}

	if( FAILED( IGT->GetTransform( &object_to_world ) ) ) {
		goto id_render_exit_error;
	}
	
	if( FAILED( IEI->GetMesh( &object_mesh ) ) ) {
		goto id_render_exit_error;
	}

	if( m_ShowFlags & SF_SHOW_FACE_NORMALS ) {

	}

	if( m_ShowFlags & SF_SHOW_VERTEX_NORMALS ) {

	}

	if( m_ShowFlags & SF_SHOW_EDGES ) {

	}

	if( m_ShowFlags & SF_SHOW_AXIS ) {

	}

	if( m_ShowFlags & SF_SHOW_NAMES ) {

	}

	// Cleanup
	//
	if( !(m_ShowFlags & SF_SHOW_TEXTURES) ) {
		IRP->set_pipeline_state( RP_TEXTURE, rp_texture );	
	}

	return S_OK;

id_render_exit_error:
	if( !(m_ShowFlags & SF_SHOW_TEXTURES) ) {
		IRP->set_pipeline_state( RP_TEXTURE, rp_texture );	
	}
#endif

	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( Import,(void))
{
	if( m_Decorated ) {
		COMPTR<ISimulatable> IS;
		if( SUCCEEDED( m_Decorated->QueryInterface( IID_ISimulatable, (void**) &IS ) ) ) {
			return IS->Import();
		}
	}
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Update,(void))
{
	if( m_Decorated ) {
		COMPTR<ISimulatable> IS;
		if( SUCCEEDED( m_Decorated->QueryInterface( IID_ISimulatable, (void**) &IS ) ) ) {
			return IS->Update();
		}
	}
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetDecorated,(IDAComponent  *idacomponent))
{
	m_Decorated = idacomponent;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetDecorated,(IDAComponent **idacomponent))
{
	if( m_Decorated ) {
		*idacomponent = m_Decorated;
		return S_OK;
	}
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( Show,( U32 flag, U32 yesno ))
{
	if( yesno ) {
		m_ShowFlags |=  (flag);
	}
	else {
		m_ShowFlags &= ~(flag);
	}
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( ShowExtents,( ExtentType type, U32 yesno ))
{
	if( yesno ) {
		m_ShowFlags |=  (SF_SHOW_EXTENT(type));
	}
	else {
		m_ShowFlags &= ~(SF_SHOW_EXTENT(type));
	}
	return S_OK;
}

//

GENRESULT CDALibs_Decorator::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_Decorator,CLSID_DACOMDESC>( "CDALibs_Decorator", desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_Decorator::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, ISimulatable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDecorator)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_Decorator::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_Decorator::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_Decorator::CDALibs_Decorator( CLSID_DACOMDESC &creation_info )
{
	m_IEngine = creation_info._IEngine;
	m_Decorated = NULL;
	m_ShowFlags = 0;
	m_RefCnt = 0;
}

//

CDALibs_Decorator::~CDALibs_Decorator()
{
}

