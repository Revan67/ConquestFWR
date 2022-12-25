// DarthCamera.h
//
//
//

#ifndef __DarthCamera_h__
#define __DarthCamera_h__

#include <memory>

#include "Engine.h"
#include "DAVariant.h"
#include "Renderer.h"
#include "RendPipeline.h"
#include "BaseCam.h"


class DarthCamera 
{
public:	// Interface

	//

	DarthCamera( float velocity=3.14/8, int x=0, int y=0, int w=640, int h=480, float fov=45.0, float aspect=64/48, float nearp=1.0, float farp=1000.0 )
	{
		DefaultViewport.x0 = x;
		DefaultViewport.y0 = y;
		DefaultViewport.x1 = x + w - 1;
		DefaultViewport.y1 = y + h - 1;
		DefaultFOV = fov;
		DefaultAspect = aspect;
		DefaultNearPlane = nearp;
		DefaultFarPlane = farp;
		DefaultVelocity = velocity;
	}

	//

	~DarthCamera()
	{
	}

	//

	ICamera *GetICamera( void )
	{
		return static_cast<ICamera*>( Camera.get() );
	}

	// 
	
	void ReleaseAll()
	{
		Camera.release();
	}

	//

	void ResetToDefaults( IEngine *Engine, INSTANCE_INDEX Target, float zScale = 3.0f )
	{
		IRenderer *IR = NULL;
		float Radius = 1.0;
		Matrix o;

		if( SUCCEEDED( Engine->QueryInterface( IID_IRenderer, (void**)&IR ) ) ) {
			Vector centroid( 0,0,1 );
			if( IR->get_centroid( Engine->get_archetype( Target ), centroid ) ) {
				SINGLE box[6];
				if( IR->get_3D_bounding_box( Engine->get_archetype( Target ), box ) ) {
					Vector len (max (fabs (box[0] - centroid.x), fabs (box[1] - centroid.x)),
								max (fabs (box[2] - centroid.y), fabs (box[3] - centroid.y)),
								max (fabs (box[4] - centroid.z), fabs (box[5] - centroid.z)));

					Radius = len.magnitude ();
				}
			}

			IR->Release();
			IR = NULL;
		}


		Camera = auto_ptr<BaseCamera>( new BaseCamera( Engine, &DefaultViewport ) );
		Camera->set_near_plane_distance( DefaultNearPlane );
		Camera->set_far_plane_distance( DefaultFarPlane );
		Camera->set_Horizontal_FOV ( DefaultFOV );
		Camera->set_Horizontal_to_vertical_aspect( DefaultAspect );
		
		o.set_identity();
		Camera->set_orientation( -1, o );
		Camera->set_position( -1, o.get_k() * Radius * zScale );
		
	}

	// 

	void SetAnimate( bool _Animate )
	{
		Animate = _Animate;
	}

	//

	void SetViewport( int x, int y, int w, int h )
	{
		DefaultViewport.x0 = x;
		DefaultViewport.y0 = y;
		DefaultViewport.x1 = x + w - 1;
		DefaultViewport.y1 = y + h - 1;
	}

	//

	void Update( IEngine *Engine, INSTANCE_INDEX Target, double dt )
	{
		if( Animate ) {

		}
	}

	//

	void Render( HWND hRender, IRenderPipeline *RenderPipe ) 
	{
		RECT R;
		
		Transform to_view( Camera->get_inverse_transform() );

		if( hRender ) {
			GetClientRect( hRender, &R );
			RenderPipe->set_window( hRender, R.left, R.top, R.right, R.bottom );
			RenderPipe->set_viewport( R.left, R.top, R.right, R.bottom );
		}
		else {
			RenderPipe->set_viewport( DefaultViewport.x0, 
									  DefaultViewport.y0, 
									  DefaultViewport.x1 - DefaultViewport.x0 + 1, 
									  DefaultViewport.y1 - DefaultViewport.y0 + 1 ); 
		}

		RenderPipe->set_modelview( to_view );
		RenderPipe->set_perspective( Camera->get_fovx(), Camera->get_aspect(), Camera->get_znear(), Camera->get_zfar() );
	}

	//

protected:	// Data

	auto_ptr<BaseCamera>	Camera;
	ViewRect				DefaultViewport;
	float					DefaultFOV;
	float					DefaultAspect;
	float					DefaultNearPlane;
	float					DefaultFarPlane;
	bool					Animate;
	float					DefaultVelocity;

};


#endif // eof
