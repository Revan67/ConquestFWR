//
// Light.cpp
//
// Functions for creating and manage the ObjView light
//
//

#define D3DLIGHT_RANGE_MAX (sqrt(FLT_MAX))

#ifdef LIGHT

//

#include "CmpndView.h"

//

const float AmbientLightChangeRate = 0.3f;
const float LightAngularVel = 45.0 * MUL_DEG_TO_RAD;
const float LightColorChangeRate = 128.0f;
const U32 MaxLights = 64;

//

bool  AmbientLight_Enabled = true;
float AmbientLight_Default[4] = {0.5, 0.5, 0.5, 1.0};
float AmbientLight_Current[4];

//

BaseLight	*BuiltinLight = NULL;	// The builtin light
Vector		 BuiltinLight_Axis;
bool		 BuiltinLight_Orbit = false;
float		 BuiltinLight_OrbitDir = 1.0f;
bool		 BuiltinLight_Enabled = false;

//

U32				LightList_Count = 0;
INSTANCE_INDEX	LightList_Instances[MaxLights];
bool			LightList_Enables[MaxLights];
ILight		   *LightList_ILights[MaxLights]; 
D3DLIGHT9		LightList_D3DLights[MaxLights];
U32				LightList_D3DLightIndices[MaxLights];

//

void SetLight( ICamera *camera, D3DLIGHT9 *light, ILight *light_data );

//

void DrawSphere( const Transform & te, const Vector& center, float radius, int mode, unsigned char color[3], bool depth_tested );

// ......................................................................

//
// Ambient light
//

//

void AmbientLight_SetDefault( float r, float g, float b )
{
	AmbientLight_Default[0] = _CLAMP( r, 0.0f, 1.0f );
	AmbientLight_Default[1] = _CLAMP( g, 0.0f, 1.0f );
	AmbientLight_Default[2] = _CLAMP( b, 0.0f, 1.0f );
}

//

void AmbientLight_Reset( void )
{
	memcpy( AmbientLight_Current, AmbientLight_Default, sizeof(AmbientLight_Current) );

	AmbientLight_Update( 0.0f );
}

//

void AmbientLight_Enable( bool enabled )
{
	AmbientLight_Enabled = enabled;
}

//

void AmbientLight_Set( float r, float g, float b )
{
	AmbientLight_Current[0] = _CLAMP( r, 0.0f, 1.0f );
	AmbientLight_Current[1] = _CLAMP( g, 0.0f, 1.0f );
	AmbientLight_Current[2] = _CLAMP( b, 0.0f, 1.0f );
}

//

void AmbientLight_Get( float *r, float *g, float *b )
{
	*r = AmbientLight_Current[0];
	*g = AmbientLight_Current[1];
	*b = AmbientLight_Current[2];
}

//

void AmbientLight_SelectColor( void )
{
	if( BuiltinLight == NULL ) {
		return;
	}

	U32 ambient_color = D3DCOLOR_RGBA( int(AmbientLight_Current[0] * 255),
								   int(AmbientLight_Current[1] * 255),
								   int(AmbientLight_Current[2] * 255),
								   0xFF );

	CHOOSECOLOR cc;
	memset( &cc, 0, sizeof(cc) );
	cc.lStructSize = sizeof(cc);
//	cc.Flags = CC_RGBINIT;
	cc.hwndOwner = GetDesktopWindow();
	cc.rgbResult = ambient_color;

	if( ChooseColor( &cc ) ) {
		float r = float((ambient_color>> 0) & 0xFF) / 255.0f;
		float g = float((ambient_color>> 8) & 0xFF) / 255.0f;
		float b = float((ambient_color>>16) & 0xFF) / 255.0f;
		
		AmbientLight_Set( r, g, b );
	}	
}

//

void AmbientLight_Update( float dt )
{
	if( !AmbientLight_Enabled ) {
		return;
	}

	if( LightMan ) {

		LightMan->set_ambient_light( int(AmbientLight_Current[0] * 255), 
									 int(AmbientLight_Current[1] * 255), 
									 int(AmbientLight_Current[2] * 255) );
	}

	if( RenderPipe ) {
		U32 ambient_color;

		ambient_color = D3DCOLOR_RGBA( int(AmbientLight_Current[0] * 255),
								   int(AmbientLight_Current[1] * 255),
								   int(AmbientLight_Current[2] * 255),
								   0xFF );

		RenderPipe->set_render_state( D3DRS_AMBIENT, ambient_color );
	}
}

//
// Light list lights
//

//

void LightList_Clear( void )
{
	for( U32 l=0; l<LightList_Count; l++ ) {
		DACOM_RELEASE( LightList_ILights[l] );
		LightList_Instances[l] = INVALID_INSTANCE_INDEX;
	}

	memset( LightList_ILights, 0, MaxLights * sizeof(LightList_ILights[0]) );

	LightList_Count = 0;
}

//

U32	LightList_GetCount( void )
{
	return LightList_Count;
}

//

void LightList_Add( INSTANCE_INDEX inst_index )
{
	if( inst_index == INVALID_INSTANCE_INDEX ) {
		return;
	}

	INSTANCE_INDEX child_inst_index = INVALID_INSTANCE_INDEX;

	while( child_inst_index = Engine->get_instance_child_next( inst_index, 0, child_inst_index ) ) {
		
		if( child_inst_index == INVALID_INSTANCE_INDEX ) {
			break;
		}

		ASSERT( LightList_Count+1 < MaxLights );

		if( SUCCEEDED( Engine->query_instance_interface( child_inst_index, IID_ILight, (IDAComponent**)&LightList_ILights[LightList_Count] ) ) ) {
			LightList_Instances[LightList_Count] = child_inst_index;
			LightList_Count++;
		}
	}
}

//
//

void RenderLight( ICamera *Camera, float LightSize, ILight *Light, INSTANCE_INDEX inst_index, bool active, bool show_range )
{
	ASSERT( Camera );
	ASSERT( Light );
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	// Render the spheres
	//
	Transform		to_world( false );
	LightRGB		color;
	float			range;
	Vector			P;

	Light->GetPosition( P );
	Light->GetTransform( to_world );
	Light->GetColor( color );

	int max_color = _MAX( _MIN( color.r, color.g ), color.b );

	if( max_color > 255 ) {
		float scale = 255.0 / max_color;
	
		color.r = color.r * scale;
		color.g = color.g * scale;
		color.b = color.b * scale;
	}

	range = Light->GetRange();

	// Draw two spheres, the first is a standard size.
	// The second is the size of the range.
	//
	DrawSphere( to_world, 
				Vector(0,0,0), 
				LightSize, 
				D3DFILL_WIREFRAME, 
				(unsigned char*)&color, 
				true );

	if( show_range ) {
		DrawSphere( to_world, 
					Vector(0,0,0), 
					range, 
					D3DFILL_WIREFRAME, 
					(unsigned char*)&color, 
					true );
	}
	
	// Render the label
	//
	char			name[MAX_PATH];
	ARCHETYPE_INDEX	arch_index;

	arch_index = Engine->get_instance_archetype( inst_index );
	ASSERT( arch_index != INVALID_INSTANCE_INDEX );
	Engine->release_archetype( arch_index );

	// Get the name of this light
	//
	strcpy( name, Engine->get_archetype_name( arch_index ) );

	int length = strlen( name );
	if( length > 14 ) {
		name[length - 14] = 0; // strip date and extension
	}

	if( BuiltinLight == Light )	{	
		strcpy( name, "[builtin]");
	}

	if( active ) {
		strcat( name, " [on]" );
	}
	else {
		strcat( name, " [off]" );
	}

	float x,y,z;

	if( Camera->point_to_screen( x, y, z, to_world.translation ) ) {

		SetRender2D();
	
		Font.RenderString( x, y + 20, name );

		float cutoff = Light->GetCutoff();
		if( Light->IsInfinite() ) {
			Font.RenderFormattedString( x, y + 40, "R:Inf C%.1f", cutoff );
		}
		else {
			Font.RenderFormattedString( x, y + 40, "R:%.1f C%.1f", range, cutoff );
		}
	}
}

//

void SetLight( ICamera *camera, D3DLIGHT9 *light, ILight *light_data )
{
	Vector	v;
	LightRGB rgb;

	memset( light, 0, sizeof(D3DLIGHT9) );

	if( light_data->IsInfinite() ) {
		light->Type = D3DLIGHT_DIRECTIONAL;
	}
	else { 
		
		light->Theta = light_data->GetCutoff() ;

		if( light->Theta <= 0.0f || light->Theta >= 180.0f ) {
			light->Type = D3DLIGHT_POINT;
			light->Theta *= MUL_DEG_TO_RAD;
			light->Phi = light->Theta;
		}
		else {
			light->Type = D3DLIGHT_SPOT;
			light->Theta *= MUL_DEG_TO_RAD;
			light->Phi = __min( 1.3f * light->Theta, 180.0f * MUL_DEG_TO_RAD );
		}
	}

	light_data->GetColor( rgb );
	light->Diffuse.r = ((float)rgb.r) / 255.0f;
	light->Diffuse.g = ((float)rgb.g) / 255.0f;
	light->Diffuse.b = ((float)rgb.b) / 255.0f;
	light->Diffuse.a = 1.0f;

	light->Falloff = 1.0f;

	float r = light_data->GetRange();

	if( r < 0.0001 ) {
		light->Attenuation1 = 0.0001f;
		light->Range = D3DLIGHT_RANGE_MAX;
	}
	else {
		light->Attenuation0 = 1.0f;
		light->Attenuation1 = 0.0f; //1.0f/(4.00f * r);
		light->Attenuation2 = 1.0f/(0.25f * r * r);
		light->Range = 2.0f * r;
	}
	
	Vector P, D;
	Transform T;
	T.d[2][2] = -1.0f;

	light_data->GetDirection( v );
	D = T * camera->get_inverse_transform() * v;
	light->Direction.x = D.x;
	light->Direction.y = D.y;
	light->Direction.z = D.z;

	light_data->GetPosition( v );
	P = T * camera->get_inverse_transform() * v;
	
	light->Position.x = P.x;
	light->Position.y = P.y;
	light->Position.z = P.z;

#if 0
	P = T * camera->get_inverse_transform() * Vector( 0, 8, 0 );
    ZeroMemory( light, sizeof(D3DLIGHT9) );
    light->Type       = D3DLIGHT_POINT;
    light->Diffuse.r  = 1.0f;
    light->Diffuse.g  = 1.0f;
    light->Diffuse.b  = 1.0f;
    light->Range       = D3DLIGHT_RANGE_MAX;
	light->Position.x  = P.x;
	light->Position.y  = P.y;
	light->Position.z  = P.z;
	light->Attenuation0 = 0;
	light->Attenuation1 = 0.01;
	light->Attenuation2 = 0;
#endif

}

//

void LightList_Update( ICamera *camera, float dt )
{
	if( LightMan == NULL ) {
		return;
	}

	// reactivate the lights for lightmanager
	//
	LightMan->deactivate_all_lights();
	int num_lights = LightMan->get_registered_lights( LightList_ILights );
	LightMan->activate_lights( LightList_ILights, num_lights );
	LightMan->update_lighting( camera );

	// reactivate the lights for hardware lighting
	// we really shouldn't have to do this every frame, but...
	//
	U32 nl;
	
	RenderPipe->get_num_lights( &nl );
	
	if( nl ) {
		RenderPipe->get_lights( 0, nl, LightList_D3DLightIndices );
		for( U32 l=0; l<nl; l++ ) {
			RenderPipe->set_light_enable( LightList_D3DLightIndices[l], FALSE );		
		}
	}

	for( int ll=0; ll<num_lights; ll++ ) {
		SetLight( camera, &LightList_D3DLights[ll], LightList_ILights[ll] );
		RenderPipe->set_light( ll, &LightList_D3DLights[ll] );
		RenderPipe->set_light_enable( ll, TRUE );
	}

}

//

void LightList_Render( ICamera *camera, float light_size )
{
	LightMan->query_lights( LightList_Enables, LightList_ILights, LightList_Count );

	for( U32 l=0; l<LightList_Count; l++ ) {
		RenderLight( camera, light_size, LightList_ILights[l], LightList_Instances[l], LightList_Enables[l], true );
	}
}

//

ILight *LightList_GetILight( U32 list_index ) 
{
	if( list_index >= LightList_Count ) {
		return NULL;
	}

	return LightList_ILights[list_index];
}

//

void LightList_Enable( U32 list_index, bool enabled ) 
{
	if( list_index >= LightList_Count ) {
		return;
	}

	LightList_Enables[list_index] = enabled;
}

//

//
// Builtin light
//


//

bool BuiltinLight_IsValid( void )
{
	return (BuiltinLight == NULL)? false : true;
}

//

void BuiltinLight_Create( float r, float g, float b, float range )
{
	if( BuiltinLight != NULL ) {
		return;
	}

	LightRGB rgb;
	
	BuiltinLight = new BaseLight( Engine, System );

	ASSERT( BuiltinLight );

	rgb.r = r * 255.0f;
	rgb.g = g * 255.0f;
	rgb.b = b * 255.0f;

	BuiltinLight->SetRange( range );
	BuiltinLight->SetInfinite( 0 );
	BuiltinLight->SetCutoff( 180.0f );
	BuiltinLight->SetColor( rgb );
	
	BuiltinLight_Enable( true );

	BuiltinLight_Orbit = false;
	BuiltinLight_OrbitDir = 1.0f;

	return;

}

//

void BuiltinLight_Destroy( void )
{
	BuiltinLight_Orbit = false;
	
	BuiltinLight_Detach();

//	DACOM_RELEASE( LightBuiltin->Release() );
	delete BuiltinLight;
	BuiltinLight = NULL;
}

//

void BuiltinLight_Enable( bool enabled )
{
	BuiltinLight->set_On( enabled );
	BuiltinLight_Enabled = enabled;
}

//

void BuiltinLight_Render( ICamera *camera, float light_size )
{
//	U32 color = 0xFFFFFFFF;
//	DrawSphere( Transform(), Vector( 0,8,0 ), light_size, D3DFILL_WIREFRAME, (unsigned char*)&color, true );

	RenderLight( camera, light_size, BuiltinLight, BuiltinLight->index, BuiltinLight_Enabled, false );
}

//

void BuiltinLight_Attach( INSTANCE_INDEX parent_inst_index )
{
	if( BuiltinLight == NULL ) {
		return;
	}

	if( parent_inst_index == INVALID_INSTANCE_INDEX ) {
		GENERAL_WARNING( "Attempted to attach built-in light to invalid instance" );
		return;
	}

	if( Engine->get_instance_root( parent_inst_index ) == Engine->get_instance_root( BuiltinLight->index ) ) {
		return;
	}

	JointInfo j;
	Matrix m;

	m.set_identity ();

	j.type = JT_FIXED;
	j.rel_position = Vector (0, 0, 0);
	j.rel_orientation = m;

	Engine->create_joint( parent_inst_index, BuiltinLight->index, &j );
}

//

void BuiltinLight_Detach( void )
{
	if( BuiltinLight == NULL ) {
		return;
	}

	INSTANCE_INDEX parent_inst_index;

	parent_inst_index = Engine->get_instance_parent( BuiltinLight->index );

	if( Engine->get_instance_root( parent_inst_index ) != Engine->get_instance_root( BuiltinLight->index ) ) {
		return;
	}

	Engine->destroy_joint( parent_inst_index, BuiltinLight->index );
}

//

void BuiltinLight_EnableOrbit( bool enabled )
{
	if( BuiltinLight == NULL ) {
		return;
	}

	BuiltinLight_Orbit = enabled;

	if( BuiltinLight_Orbit ) {

		BuiltinLight_Detach();

		//compute light axis

		Vector look( Engine->get_position( BuiltinLight->index ) );

		BuiltinLight_Axis = Vector( look.y, -look.x, 0 );

		if( fabs (BuiltinLight_Axis.x) < NEAR_ZERO && fabs (BuiltinLight_Axis.y) < NEAR_ZERO ) {
			BuiltinLight_Axis.x = 1.0f;
		}

		BuiltinLight_Axis.normalize ();
	}

}

//

void BuiltinLight_ToggleOrbitDirection( void )
{
	BuiltinLight_OrbitDir = 1.0f - BuiltinLight_OrbitDir;
}

//

void BuiltinLight_Update( ICamera *camera, float dt )
{
	if( BuiltinLight == NULL ) {
		return ;
	}

	// Update the light position
	//
	if( BuiltinLight_Orbit ) {
		
		Vector lpos( Engine->get_position( BuiltinLight->index ) );

		float magn = lpos.magnitude ();

		lpos.normalize ();

		Matrix ornt (lpos, BuiltinLight_Axis, cross_product (lpos, BuiltinLight_Axis));

		Quaternion q (BuiltinLight_Axis, LightAngularVel * BuiltinLight_OrbitDir * dt );

		Matrix rot (q);

		ornt = rot * ornt;

		Engine->set_position( BuiltinLight->index, ornt.get_i () * magn );
	}
}

//

void BuiltinLight_SelectColor( void )
{
	if( BuiltinLight == NULL ) {
		return;
	}

	LightRGB col;

	BuiltinLight->GetColor( col );
	
	CHOOSECOLOR cc;
	memset( &cc, 0, sizeof(cc) );
	cc.lStructSize = sizeof(cc);
	cc.Flags = CC_ANYCOLOR | CC_RGBINIT;
	cc.rgbResult = D3DCOLOR_XRGB( col.r, col.g, col.b );

	if( ChooseColor( &cc ) ) {
		col.r = (cc.rgbResult>> 0) & 0xFF;
		col.g = (cc.rgbResult>> 8) & 0xFF;
		col.b = (cc.rgbResult>>16) & 0xFF;
		BuiltinLight->SetColor( col );
	}
}

//

#endif //LIGHT

// EOF
