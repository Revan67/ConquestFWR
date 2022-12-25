// DALibs_Light.cpp: implementation of the CDALibs_Light class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <stdio.h>

#include "dacom.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "rendpipeline.h"
#include "ILight.h"
#include "extent.h"
#include "renderer.h"
#include "ITextureLibrary.h"
#include "ICamera.h"
#include "IProfileParser.h"
#include "IRenderPrimitive.h"
#include "ILight.h"
#include "FDump.h"

#include "IPersistable.h"
#include "IPersistableOptions.h"
#include "IRenderable.h"
#include "IGeoTransformable.h"
#include "ISimulatable.h"
#include "IDACOMEngineInstance.h"
#include "DACOM_Utility.h"

#include "Options_inl.cpp"

#ifndef M_PI
#define M_PI 3.141592653F
#endif

//

enum LIGHTTYPE
{
	LT_DIRECTIONAL=1,
	LT_SPOT,
	LT_POINT,

	LT_MAX
};

//
#define _R(clr) (((clr)>>16) & 0xFF)
#define _G(clr) (((clr)>>8)  & 0xFF)
#define _B(clr) (((clr)>>0)  & 0xFF)

//


#define DACOM_COMPONENT_NAME CDALibs_Light

dacom_component CDALibs_Light : dacom_implements IPersistable,
								 dacom_implements IPersistableOptions,
								 dacom_implements IRenderable,
								 dacom_implements ISimulatable,
								 dacom_implements IGeoTransformable,
								 dacom_implements IDACOMEngineInstance,
								 dacom_implements ILight,
								 dacom_implements IComponentFactory
{

	// IPersistable
	DACOM_INTERFACE_METHOD_DECL( LoadFromFileSystem,			(const char * name, IFileSystem *IFS));
	DACOM_INTERFACE_METHOD_DECL( SaveToFileSystem,				(const char * name, IFileSystem *IFS));

	// IPersistableOptions
	DACOM_INTERFACE_METHOD_DECL( LoadFromSection,				(const char * name, IProfileParser *IPP ));
	DACOM_INTERFACE_METHOD_DECL( SaveToSection,					(const char * name, IProfileParser *IPP ));
	
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

	// IDACOMEngineInstance
	DACOM_INTERFACE_METHOD_DECL( GetMesh,						(Mesh **ppmesh));
	DACOM_INTERFACE_METHOD_DECL( GetInstanceIndex,				(INSTANCE_INDEX *pidx));

	// ILight
	DEFMETHOD(GetTransform) (class Transform & transform) const;
	DEFMETHOD(GetPosition) (class Vector & position) const;
	DEFMETHOD(GetColor) (struct LightRGB & color) const;
	DEFMETHOD(GetDirection) (class Vector & direction) const;
	DEFMETHOD_(SINGLE,GetRange) (void) const;
	DEFMETHOD_(BOOL32,IsInfinite) (void) const;
	DEFMETHOD_(SINGLE,GetCutoff) (void) const;
	DEFMETHOD_(U32,GetMap) (void) const;

	// IComponentFactory && IDAComponent && IDADispatch
	DEFMETHOD(Invoke) (const C8 *methodName, DACOM_VARIANT parm1 = DACOM_VARIANT() );
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// Interface
	CDALibs_Light(  CLSID_DACOMDESC &creation_info );
	~CDALibs_Light();

protected:	// Interface
	void Recompute( bool compute_x, bool compute_y );

protected:	// Private Data
	COMPTR<ISystemContainer>	m_ISystem;
	COMPTR<ICOManager>			m_ICOManager;
	COMPTR<IEngine>				m_IEngine;

	INSTANCE_INDEX	m_InstanceIdx;


	LIGHTTYPE		m_Type;
	U32				m_Color;
	Transform		m_LightToWorld;
	float			m_Cutoff;
	float			m_Hotspot;
	float			m_Range;

	int				m_RefCnt;
};

//

HRESULT RegisterCDALibs_Light( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( "CDALibs_Light" );
	if( T_DACOM_CreateInstance<CDALibs_Light,CLSID_DACOMDESC>( "CDALibs_Light", &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

DACOM_INTERFACE_METHOD_IMPL( LoadFromSection,(const char * name, IProfileParser *_IPP))
{
	if( name != NULL ) {

		// read parameters
		//
		Vector pos, target, up, color;
		
		COMPTR<IProfileParser> IPP;
		char type[255+1];
		char path[1024+1];
		char file[1024+1];

		if( name ) {
			strcpy( path, name );
		}
		else {
			strcpy( path, "DefaultLight" );
		}

#define BIG_RANGE (1E9)

		opt_get_string( m_ICOManager, _IPP, path, "Type",			"Point",			type, 255	);

		opt_get_vector( m_ICOManager, _IPP, path, "Position",		Vector(0,0,0),		pos			);
		opt_get_vector( m_ICOManager, _IPP, path, "Target",			Vector(0,0,-1),		target		);
		opt_get_vector(	m_ICOManager, _IPP, path, "Color",			Vector(255,255,255),color		);

		opt_get_float( 	m_ICOManager, _IPP, path, "Range",			BIG_RANGE,			&m_Range	);
		opt_get_float( 	m_ICOManager, _IPP, path, "Cutoff",			180.0,				&m_Cutoff	);
		opt_get_float( 	m_ICOManager, _IPP, path, "Hotspot",		180.0,				&m_Hotspot	);
		
		opt_get_string(	m_ICOManager, _IPP, path, "VisualRepresentation", "OV_Light.3db", file, 1024 );

		// TODO: position and orient the light


		// create 3db representation
		//
		COMPTR<ITextureLibrary> ITL;
		if( SUCCEEDED( m_ISystem->QueryInterface( IID_ITextureLibrary, (void**) &ITL ) ) ) {
			
			COMPTR<IFileSystem> IFS;
			if( SUCCEEDED( m_IEngine->create_file_system( file, (IFileSystem**) &IFS ) ) ) {
			
				ITL->load_library( IFS );
				if( (m_InstanceIdx = m_IEngine->create_instance( file, IFS )) == INVALID_INSTANCE_INDEX ) {
					return E_FAIL;
				}
			}
		}
	}

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SaveToSection,(const char * name, IProfileParser *_IPP))
{
	return E_FAIL;
}

//

DACOM_INTERFACE_METHOD_IMPL( LoadFromFileSystem,(const char * name, IFileSystem *IFS))
{
	return LoadFromSection( name, NULL );
}

//

DACOM_INTERFACE_METHOD_IMPL( SaveToFileSystem,(const char * name, IFileSystem *IFS))
{
	return SaveToSection( name, NULL );
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

DACOM_INTERFACE_METHOD_IMPL( SetIdentity,(void))
{
	m_LightToWorld.set_identity();
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Multiply,(const Transform *T))
{
	m_LightToWorld.multiply( *T );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranspose,(Transform *out_T))
{
	*out_T = m_LightToWorld.get_transpose();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetInverse,(Transform *out_T))
{
	*out_T = m_LightToWorld.get_inverse();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTransform,(const Transform *Transform))
{
	m_LightToWorld = *Transform;
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTransform,(Transform *out_Transform))
{
	*out_Transform = m_LightToWorld;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTranslation,(const Vector *translation))
{
	m_LightToWorld.set_position( *translation );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranslation,(Vector *out_translation))
{
	*out_translation = m_LightToWorld.get_position();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromMatrix,(const Matrix *M))
{
	m_LightToWorld.set_orientation( *M );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromTransform,(const Transform *T))
{
	m_LightToWorld.d[0][0] = T->d[0][0];
	m_LightToWorld.d[0][1] = T->d[0][1];
	m_LightToWorld.d[0][2] = T->d[0][2];
	m_LightToWorld.d[1][0] = T->d[1][0];
	m_LightToWorld.d[1][1] = T->d[1][1];
	m_LightToWorld.d[1][2] = T->d[1][2];
	m_LightToWorld.d[2][0] = T->d[2][0];
	m_LightToWorld.d[2][1] = T->d[2][1];
	m_LightToWorld.d[2][2] = T->d[2][2];

	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromQuaternion,(const Quaternion *Q))
{
	m_LightToWorld.set_orientation( *Q );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromAxisAngle,(const Vector *axis, const float angle_rad))
{
	GENERAL_TRACE_1( "CDALibs_Light: SetOrientationFromAxisAngle: unsupported\n" );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetOrientation,(Matrix *out_M))
{
	*out_M = m_LightToWorld.get_orientation();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisI,(const Vector *in_V))
{
	m_LightToWorld.set_i( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisI,(Vector *out_V))
{
	*out_V = m_LightToWorld.get_i();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisJ,(const Vector *in_V))
{
	m_LightToWorld.set_j( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisJ,(Vector *out_V))
{
	*out_V = m_LightToWorld.get_j();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisK,(const Vector *in_V))
{
	m_LightToWorld.set_k( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_LightToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisK,(Vector *out_V))
{
	*out_V = m_LightToWorld.get_k();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Rotate,(const Vector *V, Vector *out_V))
{
	*out_V = m_LightToWorld.rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslate,(const Vector *V, Vector *out_V))
{
	*out_V = m_LightToWorld.rotate_translate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_LightToWorld.inverse_rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_LightToWorld.inverse_rotate_translate( *V );
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

GENRESULT CDALibs_Light::Invoke( const C8 *methodName, DACOM_VARIANT parm1 )
{
	return GR_GENERIC;
}


//

GENRESULT CDALibs_Light::GetTransform( class Transform & transform) const
{
	transform = m_LightToWorld;
	return GR_OK;
}

//

GENRESULT CDALibs_Light::GetPosition( class Vector & position) const
{
	position = m_LightToWorld.get_position();
	return GR_OK;
}

//

GENRESULT CDALibs_Light::GetColor( struct LightRGB & _color ) const
{
	_color.r = _R(m_Color);
	_color.g = _G(m_Color);
	_color.b = _B(m_Color);
	return GR_OK;
}

//

GENRESULT CDALibs_Light::GetDirection(class Vector & _direction)	const
{
	_direction = m_LightToWorld.get_k();
	return GR_OK;
}

//

SINGLE CDALibs_Light::GetRange(void) const 
{
	return m_Range;
}

//

BOOL32 CDALibs_Light::IsInfinite(void) const
{
	return (m_Type == LT_DIRECTIONAL);
}

//

SINGLE CDALibs_Light::GetCutoff(void) const
{
	return m_Cutoff;
}

//

U32 CDALibs_Light::GetMap(void) const
{
	return 0;
}

//






//

GENRESULT CDALibs_Light::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_Light,CLSID_DACOMDESC>( "CDALibs_Light", desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_Light::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IPersistable)
	DACOM_QUERYINTERFACE_ENTRY(instance, ISimulatable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IGeoTransformable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDACOMEngineInstance)
	DACOM_QUERYINTERFACE_ENTRY(instance, ILight)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_Light::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_Light::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_Light::CDALibs_Light( CLSID_DACOMDESC &creation_info )
{
	m_InstanceIdx = INVALID_INSTANCE_INDEX;
	m_IEngine = creation_info._IEngine;
	m_ISystem = creation_info._ISystem;
	m_ICOManager = DACOM_Acquire();

	m_RefCnt = 0;
}

//

CDALibs_Light::~CDALibs_Light()
{
	if( m_InstanceIdx != INVALID_INSTANCE_INDEX ) {
		m_IEngine->destroy_instance( m_InstanceIdx );
	}
}

