// DALibs_Renderable.cpp: implementation of the CDALibs_Renderable class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>

#include "dacom.h"
#include "3dmath.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "renderer.h"
#include "ICamera.h"
#include "ITextureLibrary.h"
#include "ITXMLib.h"
#include "extent.h"

#include "IPersistable.h"
#include "IRenderable.h"
#include "IGeoTransformable.h"
#include "ISimulatable.h"
#include "INamedProperty.h"
#include "IDACOMEngineInstance.h"
#include "DACOM_Utility.h"
#include "SymbolTable.h"


#define DACOM_COMPONENT_NAME CDALibs_Renderable

dacom_component CDALibs_Renderable : dacom_implements IPersistable,
									 dacom_implements IRenderable,
									 dacom_implements ISimulatable,
									 dacom_implements IGeoTransformable,
									 dacom_implements INamedProperty,
									 dacom_implements IDACOMEngineInstance,
									 dacom_implements IComponentFactory
{

	// IPersistable
	DACOM_INTERFACE_METHOD_DECL( LoadFromFileSystem,			(const char * name, IFileSystem *IFS));
	DACOM_INTERFACE_METHOD_DECL( SaveToFileSystem,				(const char * name, IFileSystem *IFS));
																
	// IRenderable												
	DACOM_INTERFACE_METHOD_DECL( Render,						(ILowLevelCamera *IC));
																
	// ISimulatable												
	DACOM_INTERFACE_METHOD_DECL( Import,						());
	DACOM_INTERFACE_METHOD_DECL( Update,						());

	// IGeoTransformable
	DACOM_INTERFACE_METHOD_DECL( SetIdentity,					(void));
	DACOM_INTERFACE_METHOD_DECL( Multiply,						(const Transform *T));
	DACOM_INTERFACE_METHOD_DECL( GetTranspose,					(Transform *out_T));
	DACOM_INTERFACE_METHOD_DECL( GetInverse,					(Transform *out_T));
	DACOM_INTERFACE_METHOD_DECL( SetTransform,					(const Transform *Transform));
	DACOM_INTERFACE_METHOD_DECL( GetTransform,					(Transform *out_Transform));
	DACOM_INTERFACE_METHOD_DECL( SetTranslation,				(const Vector *translation));
	DACOM_INTERFACE_METHOD_DECL( GetTranslation,				(Vector *out_translation));
	DACOM_INTERFACE_METHOD_DECL( SetOrientationFromMatrix,		(const Matrix *M));
	DACOM_INTERFACE_METHOD_DECL( SetOrientationFromTransform,	(const Transform *T));
	DACOM_INTERFACE_METHOD_DECL( SetOrientationFromQuaternion,	(const Quaternion *Q));
	DACOM_INTERFACE_METHOD_DECL( SetOrientationFromAxisAngle,	(const Vector *axis, const float angle_rad));
	DACOM_INTERFACE_METHOD_DECL( GetOrientation,				(Matrix *out_M));
	DACOM_INTERFACE_METHOD_DECL( SetBasisI,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD_DECL( GetBasisI,						(Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( SetBasisJ,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD_DECL( GetBasisJ,						(Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( SetBasisK,						(const Vector *in_V));
	DACOM_INTERFACE_METHOD_DECL( GetBasisK,						(Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( Rotate,						(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( RotateAndTranslate,			(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( RotateByInverse,				(const Vector *V, Vector *out_V));
	DACOM_INTERFACE_METHOD_DECL( RotateAndTranslateByInverse,	(const Vector *V, Vector *out_V));

	// INamedProperty
	DACOM_INTERFACE_METHOD_DECL( SetPropertyFromString,	(const char *name, const char *value));
	DACOM_INTERFACE_METHOD_DECL( SetPropertyFromInt,	(const char *name, const U32 value));
	DACOM_INTERFACE_METHOD_DECL( SetPropertyFromReal,	(const char *name, const float value));
	DACOM_INTERFACE_METHOD_DECL( SetPropertyFromUnknown,(const char *name, const void *value));
	DACOM_INTERFACE_METHOD_DECL( GetPropertyAsString,	(const char *name, char *value, U32 max_len));
	DACOM_INTERFACE_METHOD_DECL( GetPropertyAsInt,		(const char *name, U32 *value));
	DACOM_INTERFACE_METHOD_DECL( GetPropertyAsReal,		(const char *name, float *value));
	DACOM_INTERFACE_METHOD_DECL( GetPropertyAsUnknown,	(const char *name, void **value));

	// IDACOMEngineInstance
	DACOM_INTERFACE_METHOD_DECL( GetMesh,						(Mesh **ppmesh));
	DACOM_INTERFACE_METHOD_DECL( GetInstanceIndex,				(INSTANCE_INDEX *pidx));
													
	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// Interface
	CDALibs_Renderable( CLSID_DACOMDESC &creation_info );
	~CDALibs_Renderable();

protected:	// Private Data
	INSTANCE_INDEX	m_InstanceIdx;
	Transform		m_ObjectToWorld;
	CSymbolTable	m_NamedProperties;

	COMPTR<IEngine>				m_IEngine;
	COMPTR<ISystemContainer>	m_ISystem;
	int							m_RefCnt;

};

//

HRESULT RegisterCDALibs_Renderable( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( "CDALibs_Renderable" );
	if( T_DACOM_CreateInstance<CDALibs_Renderable,CLSID_DACOMDESC>( "CDALibs_Renderable", &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

DACOM_INTERFACE_METHOD_IMPL( LoadFromFileSystem,(const char * name, IFileSystem *_IFS))
{
	if( name == NULL ) {
		return E_FAIL;
	}

	COMPTR<IFileSystem> IFS;
	if( _IFS ) {
		IFS = _IFS;
	}
	else {
		if( FAILED( m_IEngine->create_file_system( name, (IFileSystem**) &IFS ) ) ) {
			return E_FAIL;
		}
	}

	COMPTR<ITextureLibrary> ITL;
	COMPTR<ITXMLib> ITXM;

	if( SUCCEEDED( m_IEngine->QueryInterface( IID_ITXMLib, ITXM ) ) ) {
		ITXM->load_library( IFS );
	}
	else if( SUCCEEDED( m_ISystem->QueryInterface( IID_ITextureLibrary, ITL ) ) ) {
		ITL->load_library( IFS );
	}

	if( (m_InstanceIdx = m_IEngine->create_instance( name, IFS )) == INVALID_INSTANCE_INDEX ) {
		return E_FAIL;
	}
	
	SetIdentity();

	const char *sz = strrchr( name, '\\' );
	if( sz == NULL ) {
		sz = name;
	}
	else {
		sz++;
	}
	
	m_NamedProperties.SetSymbolValueFromString( "Name", sz );
	m_NamedProperties.SetSymbolValueFromString( "Path", name );
	
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SaveToFileSystem,(const char * name, IFileSystem *IFS))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( Render,(ILowLevelCamera *IC ))
{
	if( m_InstanceIdx != INVALID_INSTANCE_INDEX ) {
		COMPTR<ICamera> IEC;
		if( SUCCEEDED( IC->QueryInterface( IID_ICamera, (void**) &IEC ) ) ) {
			m_IEngine->render_instance( IEC, m_InstanceIdx, RF_FILL );
		}
	}

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Import,(void))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( Update,(void))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetPropertyFromString,(const char *name, const char *value))
{
	return m_NamedProperties.SetSymbolValueFromString( name, value );
}

//

DACOM_INTERFACE_METHOD_IMPL( SetPropertyFromInt,(const char *name, const U32 value))
{
	return m_NamedProperties.SetSymbolValueFromU32( name, value );
}

//

DACOM_INTERFACE_METHOD_IMPL( SetPropertyFromReal,(const char *name, const float value))
{
	return m_NamedProperties.SetSymbolValueFromFloat( name, value );
}

//

DACOM_INTERFACE_METHOD_IMPL( SetPropertyFromUnknown,(const char *name, const void *value))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetPropertyAsString,(const char *name, char *value, U32 max_len))
{
	return m_NamedProperties.GetSymbolValueAsString( name, value, max_len );
}

//

DACOM_INTERFACE_METHOD_IMPL( GetPropertyAsInt,(const char *name, U32 *value))
{
	return m_NamedProperties.GetSymbolValueAsU32( name, value );
}

//

DACOM_INTERFACE_METHOD_IMPL( GetPropertyAsReal,(const char *name, float *value))
{
	return m_NamedProperties.GetSymbolValueAsFloat( name, value );
}

//

DACOM_INTERFACE_METHOD_IMPL( GetPropertyAsUnknown,(const char *name, void **value))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetIdentity,(void))
{
	m_ObjectToWorld.set_identity();
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Multiply,(const Transform *T))
{
	m_ObjectToWorld.multiply( *T );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranspose,(Transform *out_T))
{
	*out_T = m_ObjectToWorld.get_transpose();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetInverse,(Transform *out_T))
{
	*out_T = m_ObjectToWorld.get_inverse();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTransform,(const Transform *Transform))
{
	m_ObjectToWorld = *Transform;
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTransform,(Transform *out_Transform))
{
	*out_Transform = m_ObjectToWorld;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTranslation,(const Vector *translation))
{
	m_ObjectToWorld.set_position( *translation );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranslation,(Vector *out_translation))
{
	*out_translation = m_ObjectToWorld.get_position();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromMatrix,(const Matrix *M))
{
	m_ObjectToWorld.set_orientation( *M );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromTransform,(const Transform *T))
{
	m_ObjectToWorld.d[0][0] = T->d[0][0];
	m_ObjectToWorld.d[0][1] = T->d[0][1];
	m_ObjectToWorld.d[0][2] = T->d[0][2];
	m_ObjectToWorld.d[1][0] = T->d[1][0];
	m_ObjectToWorld.d[1][1] = T->d[1][1];
	m_ObjectToWorld.d[1][2] = T->d[1][2];
	m_ObjectToWorld.d[2][0] = T->d[2][0];
	m_ObjectToWorld.d[2][1] = T->d[2][1];
	m_ObjectToWorld.d[2][2] = T->d[2][2];

	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromQuaternion,(const Quaternion *Q))
{
	m_ObjectToWorld.set_orientation( *Q );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromAxisAngle,(const Vector *axis, const float angle_rad))
{
	GENERAL_TRACE_1( "CDALibs_Renderable: SetOrientationFromAxisAngle: unsupported\n" );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetOrientation,(Matrix *out_M))
{
	*out_M = m_ObjectToWorld.get_orientation();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisI,(const Vector *in_V))
{
	m_ObjectToWorld.set_i( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisI,(Vector *out_V))
{
	*out_V = m_ObjectToWorld.get_i();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisJ,(const Vector *in_V))
{
	m_ObjectToWorld.set_j( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisJ,(Vector *out_V))
{
	*out_V = m_ObjectToWorld.get_j();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisK,(const Vector *in_V))
{
	m_ObjectToWorld.set_k( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_ObjectToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisK,(Vector *out_V))
{
	*out_V = m_ObjectToWorld.get_k();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Rotate,(const Vector *V, Vector *out_V))
{
	*out_V = m_ObjectToWorld.rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslate,(const Vector *V, Vector *out_V))
{
	*out_V = m_ObjectToWorld.rotate_translate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_ObjectToWorld.inverse_rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_ObjectToWorld.inverse_rotate_translate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetMesh,(Mesh **ppmesh))
{
	COMPTR<IRenderer> IR;
	if( SUCCEEDED( m_IEngine->QueryInterface( IID_IRenderer, (void**) &IR ) ) ) {
		*ppmesh = IR->get_instance_mesh( m_InstanceIdx );
		return S_OK;
	}
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetInstanceIndex,(INSTANCE_INDEX *pidx))
{
	*pidx = m_InstanceIdx;
	return E_FAIL;
}

//


GENRESULT CDALibs_Renderable::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_Renderable,CLSID_DACOMDESC>( "CDALibs_Renderable", desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_Renderable::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IPersistable)
	DACOM_QUERYINTERFACE_ENTRY(instance, ISimulatable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IGeoTransformable)
	DACOM_QUERYINTERFACE_ENTRY(instance, INamedProperty)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDACOMEngineInstance)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_Renderable::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_Renderable::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_Renderable::CDALibs_Renderable( CLSID_DACOMDESC &creation_info )
{
	m_InstanceIdx = INVALID_INSTANCE_INDEX;
	m_IEngine = creation_info._IEngine;
	m_ISystem = creation_info._ISystem;

	m_NamedProperties.AddSymbol( "Name", ST_STRING, "Unnamed" );
	m_NamedProperties.AddSymbol( "Path", ST_STRING, "Unknown" );

	m_RefCnt = 0;
}

//

CDALibs_Renderable::~CDALibs_Renderable()
{
	if( m_InstanceIdx != INVALID_INSTANCE_INDEX ) {
		m_IEngine->destroy_instance( m_InstanceIdx );
	}
}

