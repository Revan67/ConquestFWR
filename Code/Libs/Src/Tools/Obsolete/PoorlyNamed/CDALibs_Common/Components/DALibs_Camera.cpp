// DALibs_Camera.cpp: implementation of the CDALibs_Camera class.
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
#include "ICamera.h"
#include "extent.h"
#include "renderer.h"
#include "ITextureLibrary.h"
#include "IProfileParser.h"
#include "IRenderPrimitive.h"
#include "FDump.h"

#include "IPersistable.h"
#include "IPersistableOptions.h"
#include "IRenderable.h"
#include "IGeoTransformable.h"
#include "ISimulatable.h"
#include "ISceneCamera.h"
#include "ILowLevelCamera.h"
#include "IDACOMEngineInstance.h"
#include "DACOM_Utility.h"

#include "Options_inl.cpp"

#ifndef M_PI
#define M_PI 3.141592653F
#endif


#define DACOM_COMPONENT_NAME CDALibs_Camera

dacom_component CDALibs_Camera : dacom_implements IPersistable,
								 dacom_implements IPersistableOptions,
								 dacom_implements IRenderable,
								 dacom_implements ISimulatable,
								 dacom_implements ILowLevelCamera,
								 dacom_implements IGeoTransformable,
								 dacom_implements IDACOMEngineInstance,
								 dacom_implements ISceneCamera,
								 dacom_implements ICamera,
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

	// ILowLevelCamera
	DACOM_INTERFACE_METHOD_DECL( SetHorizontalFieldOfView,		(float fov));
	DACOM_INTERFACE_METHOD_DECL( GetHorizontalFieldOfView,		(float *fov));
	DACOM_INTERFACE_METHOD_DECL( SetVerticalFieldOfView,		(float fov));
	DACOM_INTERFACE_METHOD_DECL( GetVerticalFieldOfView,		(float *fov));
	DACOM_INTERFACE_METHOD_DECL( SetNearClipDistance,			(float dist_z));
	DACOM_INTERFACE_METHOD_DECL( GetNearClipDistance,			(float *dist_z));
	DACOM_INTERFACE_METHOD_DECL( SetFarClipDistance,			(float dist_z));
	DACOM_INTERFACE_METHOD_DECL( GetFarClipDistance,			(float *dist_z));
	DACOM_INTERFACE_METHOD_DECL( SetAspect,						(ILLC_ASPECTTYPE type, float aspect));
	DACOM_INTERFACE_METHOD_DECL( GetAspect,						(float *aspect));
	DACOM_INTERFACE_METHOD_DECL( SetViewport,					(float x, float y, float w, float h));
	DACOM_INTERFACE_METHOD_DECL( GetViewport,					(float *x, float *y, float *w, float *h));

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

	// ISceneCamera
	DACOM_INTERFACE_METHOD_DECL( BeginScene,					( void ));
	DACOM_INTERFACE_METHOD_DECL( EndScene,						( void ));
	DACOM_INTERFACE_METHOD_DECL( SetShot,						( ISCSHOTDESC &shot_desc ));
	DACOM_INTERFACE_METHOD_DECL( GetShot,						( ISCSHOTDESC &shot_desc ));

	// ICamera
	Vector COMAPI get_position( void ) const;
	Transform COMAPI get_transform( void ) const;
	Transform COMAPI get_inverse_transform( void );
	const struct _pane * COMAPI get_pane( void ) const;
	SINGLE COMAPI get_fovx( void ) const;
	SINGLE COMAPI get_fovy( void ) const;
	SINGLE COMAPI get_znear( void ) const;
	SINGLE COMAPI get_zfar( void ) const;
	SINGLE COMAPI get_aspect( void ) const;
	SINGLE COMAPI get_hpc( void ) const;
	SINGLE COMAPI get_vpc( void ) const;
	bool COMAPI point_to_screen( float & screen_x, float & screen_y, float & depth, const Transform & cam2world, const Vector & world_vector ) const;
	void COMAPI screen_to_point( Vector & world_vector, const Transform & cam2world, float screen_x, float screen_y ) const;
	vis_state COMAPI object_visibility( const Vector &view_pos, float radius ) const;

	// IComponentFactory && IDAComponent && IDADispatch
	DEFMETHOD(Invoke) (const C8 *methodName, DACOM_VARIANT parm1 = DACOM_VARIANT() );
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// Interface
	CDALibs_Camera(  CLSID_DACOMDESC &creation_info );
	~CDALibs_Camera();

protected:	// Interface
	void Recompute( bool compute_x, bool compute_y );

protected:	// Private Data
	COMPTR<ISystemContainer>	m_ISystem;
	COMPTR<ICOManager>			m_ICOManager;
	COMPTR<IEngine>				m_IEngine;

	INSTANCE_INDEX	m_InstanceIdx;
	Transform		m_CameraToWorld;
	PANE			m_Viewport;
	float			m_ViewportWidth;
	float			m_ViewportHeight;
	float			m_FOV_X;			// 1/2 of horizontal FOV
	float			m_FOV_Y;			// 1/2 of vertical FOV
	float			m_NearPlane;
	float			m_FarPlane;
	float			m_Aspect;

	Vector			m_RightPlaneNormal;
	Vector			m_TopPlaneNormal;
	float			m_HPC;
	float			m_VPC;
	float			m_HalfNearPlaneWidth;
	float			m_HalfNearPlaneHeight;
	U32				m_ClearColor;

	ISCSHOTDESC		m_LastShot;

	int				m_RefCnt;
};

//

HRESULT RegisterCDALibs_Camera( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( "CDALibs_Camera" );
	if( T_DACOM_CreateInstance<CDALibs_Camera,CLSID_DACOMDESC>( "CDALibs_Camera", &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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
		Vector pos, target, up;
		float fovx, aspect, nearz, farz;
		float x,y,w,h;
		U32 cc;
		COMPTR<IProfileParser> IPP;
		char path[1024+1];
		char file[1024+1];

		if( name ) {
			strcpy( path, name );
		}
		else {
			strcpy( path, "DefaultCamera" );
		}

		opt_get_vector( m_ICOManager, _IPP, path, "Position",		Vector(0,0,0),	pos		);
		opt_get_vector( m_ICOManager, _IPP, path, "Target",			Vector(0,0,-1), target	);
		opt_get_vector( m_ICOManager, _IPP, path, "Up",				Vector(0,1,0),	up		);
		opt_get_float(	m_ICOManager, _IPP, path, "HorizontalFOV",	60.0f,			&fovx	);
		opt_get_float(	m_ICOManager, _IPP, path, "Aspect",			4.0/3.0,		&aspect	);
		opt_get_float(	m_ICOManager, _IPP, path, "NearPlane",		1.0,			&nearz	);
		opt_get_float(	m_ICOManager, _IPP, path, "FarPlane",		1000.0,			&farz	);
		opt_get_float(	m_ICOManager, _IPP, path, "ScreenX",		0,				&x		);
		opt_get_float(	m_ICOManager, _IPP, path, "ScreenY",		0,				&y		);
		opt_get_float(	m_ICOManager, _IPP, path, "Width",			640,			&w		);
		opt_get_float(	m_ICOManager, _IPP, path, "Height",			480,			&h		);
		opt_get_u32(	m_ICOManager, _IPP, path, "ClearColor",		0,				&cc		);
		
		opt_get_string(	m_ICOManager, _IPP, path, "VisualRepresentation", "OV_Camera.3db", file, 1024 );

		m_ClearColor = cc;

		//
		Vector j = up.normalize();
		Vector k = pos - target;	
		k.normalize();
		Vector i = cross_product( j, k );
		i.normalize();
		j = cross_product( k, i );
		j.normalize();

		// Now we have our basis vectors for the camera's orientation:
		SetIdentity();
		SetBasisI( &i );
		SetBasisJ( &j );
		SetBasisK( &k );
		SetTranslation( &pos );
		SetNearClipDistance( nearz );
		SetFarClipDistance( farz );
		SetHorizontalFieldOfView( fovx );
		SetAspect( ILLC_ASPECT_H2V, aspect );
		SetViewport( x, y, w, h );

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

DACOM_INTERFACE_METHOD_IMPL( SetHorizontalFieldOfView,(float fov))
{
	m_FOV_X = fov * 0.5F;
	m_HalfNearPlaneWidth = m_NearPlane * tan( m_FOV_X * M_PI / 180.0 );
	Recompute( true, false );
	m_Aspect = m_HalfNearPlaneWidth / m_HalfNearPlaneHeight;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetHorizontalFieldOfView,(float *fov))
{
	*fov = m_FOV_X * 2.0;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetVerticalFieldOfView,(float fov))
{
	m_FOV_Y = fov * 0.5F;
	m_HalfNearPlaneHeight = m_NearPlane * tan( m_FOV_Y * M_PI / 180.0 );
	Recompute( false, true );
	m_Aspect = m_HalfNearPlaneWidth / m_HalfNearPlaneHeight;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetVerticalFieldOfView,(float *fov))
{
	*fov = m_FOV_Y * 2.0;
	return S_OK;

}
//

DACOM_INTERFACE_METHOD_IMPL( SetNearClipDistance,(float dist_z))
{
	m_NearPlane = dist_z;
	SetHorizontalFieldOfView( m_FOV_X * 2.0 );
	SetVerticalFieldOfView( m_FOV_Y * 2.0 );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetNearClipDistance,(float *dist_z))
{
	*dist_z = m_NearPlane;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetFarClipDistance,(float dist_z))
{
	m_FarPlane = dist_z;
	SetHorizontalFieldOfView( m_FOV_X * 2.0 );
	SetVerticalFieldOfView( m_FOV_Y * 2.0 );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetFarClipDistance,(float *dist_z))
{
	*dist_z = m_FarPlane;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetAspect,( ILLC_ASPECTTYPE type, float aspect))
{
	switch( type ) {
	case ILLC_ASPECT_H2V:	
		m_Aspect = aspect;
		m_HalfNearPlaneHeight = m_HalfNearPlaneWidth / m_Aspect;
		Recompute( false, true );
		break;
	case ILLC_ASPECT_V2H:	
		m_Aspect = 1.0/aspect;
		m_HalfNearPlaneWidth = m_HalfNearPlaneHeight / m_Aspect;	
		Recompute( true, false );
		break;
	}
	
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetAspect,(float *aspect))
{
	*aspect = m_Aspect;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetViewport,(float x, float y, float w, float h))
{
	m_Viewport.x0 = x;
	m_Viewport.y0 = y;
	m_Viewport.x1 = x+w;
	m_Viewport.y1 = y+h;

	m_ViewportWidth  = w;
	m_ViewportHeight = h;

	m_HPC = w / m_HalfNearPlaneWidth;
	m_VPC = h / m_HalfNearPlaneHeight;

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetViewport,(float *x, float *y, float *w, float *h))
{
	*x = m_Viewport.x0;
	*y = m_Viewport.y0;
	*w = m_ViewportWidth;
	*h = m_ViewportHeight;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( BeginScene,( void ))
{
	COMPTR<IRenderPrimitive> IBATCH;
	COMPTR<IRenderPipeline> IP;

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPrimitive, (void**) &IBATCH ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPipeline, (void**) &IP ) ) ) {
		return E_FAIL;
	}

	IBATCH->set_state( RPR_BATCH, TRUE );

	IBATCH->set_viewport( m_Viewport.x0, m_Viewport.y0, m_ViewportWidth, m_ViewportHeight );
	IBATCH->set_modelview( m_CameraToWorld.get_inverse() );
	IBATCH->set_perspective( m_FOV_Y, m_Aspect, m_NearPlane, m_FarPlane );
	IP->set_pipeline_state( RP_CLEAR_COLOR, m_ClearColor );
	IP->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( EndScene,( void ))
{
	COMPTR<IRenderPrimitive> IBATCH;
	if( SUCCEEDED( m_ISystem->QueryInterface( IID_IRenderPrimitive, (void**) &IBATCH ) ) ) {
		IBATCH->flush( RP_OPAQUE | RP_TRANSLUCENT_DEPTH_SORTED );
		IBATCH->set_state( RPR_BATCH, FALSE );
	}
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetShot,( ISCSHOTDESC &shot_desc ))
{
	shot_desc = m_LastShot;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetShot,( ISCSHOTDESC &shot_desc ))
{
	m_LastShot = shot_desc;

	switch( shot_desc.shot_type ) {
	case ISC_ST_INTERNAL:
		{
			COMPTR<IDACOMEngineInstance> II;

			INSTANCE_INDEX idx = -1;
			if( SUCCEEDED( shot_desc.actors[0]->QueryInterface( IID_IDACOMEngineInstance, (void**) &II ) ) ) {
				II->GetInstanceIndex( &idx );
			}

			Matrix o;
			float max_radius = m_IEngine->get_radius( idx );
			float dist = 5 * (max_radius * 2.0f > 1.5f) ? max_radius * 2.0f : 1.5f;
			o.set_identity ();

			SetOrientationFromMatrix( &o );
			Vector v = o.get_k() * dist;
			SetTranslation( &v );
		}
		break;

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

DACOM_INTERFACE_METHOD_IMPL( Render,(ILowLevelCamera *IC ))
{
	COMPTR<IDAComponent> us;
	COMPTR<IDAComponent> them;

	this->QueryInterface( IID_IDAComponent, (void**) &us );
	IC->QueryInterface( IID_IDAComponent, (void**) &them );

	if( us == them ) {
		// Don't render ourselves in our own camera
		//
		return S_OK;
	}

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
	m_CameraToWorld.set_identity();
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Multiply,(const Transform *T))
{
	m_CameraToWorld.multiply( *T );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranspose,(Transform *out_T))
{
	*out_T = m_CameraToWorld.get_transpose();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetInverse,(Transform *out_T))
{
	*out_T = m_CameraToWorld.get_inverse();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTransform,(const Transform *Transform))
{
	m_CameraToWorld = *Transform;
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTransform,(Transform *out_Transform))
{
	*out_Transform = m_CameraToWorld;
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetTranslation,(const Vector *translation))
{
	m_CameraToWorld.set_position( *translation );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetTranslation,(Vector *out_translation))
{
	*out_translation = m_CameraToWorld.get_position();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromMatrix,(const Matrix *M))
{
	m_CameraToWorld.set_orientation( *M );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromTransform,(const Transform *T))
{
	m_CameraToWorld.d[0][0] = T->d[0][0];
	m_CameraToWorld.d[0][1] = T->d[0][1];
	m_CameraToWorld.d[0][2] = T->d[0][2];
	m_CameraToWorld.d[1][0] = T->d[1][0];
	m_CameraToWorld.d[1][1] = T->d[1][1];
	m_CameraToWorld.d[1][2] = T->d[1][2];
	m_CameraToWorld.d[2][0] = T->d[2][0];
	m_CameraToWorld.d[2][1] = T->d[2][1];
	m_CameraToWorld.d[2][2] = T->d[2][2];

	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetOrientationFromQuaternion,(const Quaternion *Q))
{
	m_CameraToWorld.set_orientation( *Q );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
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
	*out_M = m_CameraToWorld.get_orientation();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisI,(const Vector *in_V))
{
	m_CameraToWorld.set_i( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisI,(Vector *out_V))
{
	*out_V = m_CameraToWorld.get_i();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisJ,(const Vector *in_V))
{
	m_CameraToWorld.set_j( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisJ,(Vector *out_V))
{
	*out_V = m_CameraToWorld.get_j();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( SetBasisK,(const Vector *in_V))
{
	m_CameraToWorld.set_k( *in_V );
	m_IEngine->set_transform( m_InstanceIdx, m_CameraToWorld );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetBasisK,(Vector *out_V))
{
	*out_V = m_CameraToWorld.get_k();
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Rotate,(const Vector *V, Vector *out_V))
{
	*out_V = m_CameraToWorld.rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslate,(const Vector *V, Vector *out_V))
{
	*out_V = m_CameraToWorld.rotate_translate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_CameraToWorld.inverse_rotate( *V );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( RotateAndTranslateByInverse,(const Vector *V, Vector *out_V))
{
	*out_V = m_CameraToWorld.inverse_rotate_translate( *V );
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

Vector COMAPI CDALibs_Camera::get_position (void)  const
{
	return m_CameraToWorld.get_position();
}

//

Transform COMAPI CDALibs_Camera::get_transform (void)  const
{
	return m_CameraToWorld;
}

//

Transform COMAPI CDALibs_Camera::get_inverse_transform( void )
{
	return m_CameraToWorld.get_inverse();
}

//

const PANE *COMAPI CDALibs_Camera::get_pane (void) const
{
	return &m_Viewport;
}

//

SINGLE COMAPI CDALibs_Camera::get_fovx (void)  const
{
	return m_FOV_X;
}

//

SINGLE COMAPI CDALibs_Camera::get_fovy (void)  const
{
	return m_FOV_Y;
}

//

SINGLE COMAPI CDALibs_Camera::get_znear (void)  const
{
	return m_NearPlane;
}

//

SINGLE COMAPI CDALibs_Camera::get_zfar (void)  const
{
	return m_FarPlane;
}

//

SINGLE COMAPI CDALibs_Camera::get_aspect (void)  const
{
	return m_Aspect;
}

//

SINGLE COMAPI CDALibs_Camera::get_hpc (void)  const
{
	return m_HPC;
}

//

SINGLE COMAPI CDALibs_Camera::get_vpc (void)  const
{
	return m_VPC;
}

//

bool COMAPI CDALibs_Camera::point_to_screen (float & screen_x, float & screen_y, float & depth, const Transform & cam2world, const Vector & world_vector)  const
{
	Vector view_vector = cam2world.inverse_rotate_translate( world_vector );
	if( view_vector.z <= -m_NearPlane ) {
		float w = -1.0f / view_vector.z;
		screen_x = m_Viewport.x0 + m_ViewportWidth  + view_vector.x * w * m_HPC;
		screen_y = m_Viewport.y0 + m_ViewportHeight + view_vector.y * w * m_VPC;
		depth = -view_vector.z;
		return true;
	}
	return false;
}

//

void COMAPI CDALibs_Camera::screen_to_point (Vector & world_vector, const Transform & cam2world, float screen_x, float screen_y)  const
{
	// Scale point into perspective space.
	float sx = screen_x - (m_Viewport.x0 + m_ViewportWidth);
	float sy = screen_y - (m_Viewport.y0 + m_ViewportHeight);

	sx /= m_HPC;
	sy /= m_VPC;

	world_vector = sx * cam2world.get_i() + sy * cam2world.get_j() - cam2world.get_k() * m_NearPlane;
	return;
}

//

vis_state COMAPI CDALibs_Camera::object_visibility (const Vector &view_pos, float radius)  const
{
		vis_state result;

		// Check near plane
		if( view_pos.z > (radius - m_NearPlane) ) {
			return VS_NOT_VISIBLE;
		}
		else if( (view_pos.z + radius) < -m_NearPlane )	{
			result = VS_FULLY_VISIBLE;
		}
		else {
			result = VS_PARTIALLY_VISIBLE;
		}

		// Check far plane
		if( (view_pos.z + radius) < -m_FarPlane ) {
			return VS_NOT_VISIBLE;
		}
		else if( view_pos.z > (radius - m_FarPlane) )
		{
			result = VS_FULLY_VISIBLE;
		}
		else {
			result = VS_PARTIALLY_VISIBLE;
		}

		// Check X planes
		// Effectively: dot_product(view_pos,h_norm);
		float rx = (float) fabs( view_pos.x ) * m_RightPlaneNormal.x + view_pos.z * m_RightPlaneNormal.z;
		if( rx > radius ) {
			return VS_NOT_VISIBLE;
		}
		else if( rx < -radius ) {
			// Fully inside view plane; leave previous result alone.
		}
		else {
			result = VS_PARTIALLY_VISIBLE;
		}

		// Check Y planes
		// Effectively: dot_product(view_pos,v_norm);
		float ry = (float) fabs(view_pos.y) * m_TopPlaneNormal.y + view_pos.z * m_TopPlaneNormal.z;
		if( ry > radius ) {
			return VS_NOT_VISIBLE;
		}
		else if( ry < -radius ) {
			// Inside view plane; leave previous result alone.
		}
		else
		{
			result = VS_PARTIALLY_VISIBLE;
		}

		return result;
}

//

void CDALibs_Camera::Recompute( bool compute_x, bool compute_y )
{
	if( compute_x ) {
		m_FOV_X = atan( m_HalfNearPlaneWidth / m_NearPlane ) * 180.0 / M_PI;
		m_RightPlaneNormal.set( 1.0, 0, m_HalfNearPlaneWidth );	// points away from view volume
		m_RightPlaneNormal.normalize();
		m_HPC = m_ViewportWidth / m_HalfNearPlaneWidth;
	}

	if( compute_y ) {
		m_FOV_Y = atan( m_HalfNearPlaneHeight / m_NearPlane ) * 180.0 / M_PI;
		m_TopPlaneNormal.set( 1.0, 0, m_HalfNearPlaneHeight );	// points away from view volume
		m_TopPlaneNormal.normalize();
		m_HPC = - m_ViewportHeight / m_HalfNearPlaneWidth;
	}

	return;
}

//

GENRESULT CDALibs_Camera::Invoke( const C8 *methodName, DACOM_VARIANT parm1 )
{
	return GR_GENERIC;
}

//

GENRESULT CDALibs_Camera::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_Camera,CLSID_DACOMDESC>( "CDALibs_Camera", desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_Camera::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IRenderable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IPersistable)
	DACOM_QUERYINTERFACE_ENTRY(instance, ISimulatable)
	DACOM_QUERYINTERFACE_ENTRY(instance, ILowLevelCamera)
	DACOM_QUERYINTERFACE_ENTRY(instance, IGeoTransformable)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDACOMEngineInstance)
	DACOM_QUERYINTERFACE_ENTRY(instance, ISceneCamera)
	DACOM_QUERYINTERFACE_ENTRY(instance, ICamera)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_Camera::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_Camera::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_Camera::CDALibs_Camera( CLSID_DACOMDESC &creation_info )
{
	m_InstanceIdx = INVALID_INSTANCE_INDEX;
	m_IEngine = creation_info._IEngine;
	m_ISystem = creation_info._ISystem;
	m_ICOManager = DACOM_Acquire();

	m_FOV_Y = 60.0;
	m_NearPlane = 1.0;
	m_FarPlane = 1000.0;
	m_Aspect = 4/3;

	m_HPC =  1.0;
	m_VPC = -1.0;

	m_HalfNearPlaneWidth = 1.0;
	m_HalfNearPlaneHeight = m_HalfNearPlaneWidth / m_Aspect;

	SetViewport( 0,0,100,100 );

	Recompute( true, true );

	m_RefCnt = 0;
}

//

CDALibs_Camera::~CDALibs_Camera()
{
	if( m_InstanceIdx != INVALID_INSTANCE_INDEX ) {
		m_IEngine->destroy_instance( m_InstanceIdx );
	}
}

