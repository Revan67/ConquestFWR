// Light.h
//
//
//

#ifndef __LIGHT_H
#define __LIGHT_H

#ifdef LIGHT

//

#if 0
typedef U32 light_bit;
const light_bit LIGHT_CURRENT	= (1<<0);		
const light_bit LIGHT_EXPORTED	= (1<<1);
const light_bit LIGHT_BUILTIN	= (1<<2);
const light_bit LIGHT_ALL		= LIGHT_CURRENT|LIGHT_EXPORTED|LIGHT_BUILTIN;

const light_bit LIGHT_RANGE		= (1<<3);
const light_bit LIGHT_AMBIENT	= (1<<4);
const light_bit LIGHT_R			= (1<<5);
const light_bit LIGHT_G			= (1<<6);
const light_bit LIGHT_B			= (1<<7);
#endif

//


// The following operate on the ambient light
//
void AmbientLight_SetDefault( float r, float g, float b );
void AmbientLight_Set( float r, float g, float b );
void AmbientLight_Get( float *r, float *g, float *b );
void AmbientLight_Reset( void );
void AmbientLight_Enable( bool enabled );
void AmbientLight_Update( float dt );
void AmbientLight_SelectColor( void );

//

void	LightList_Clear( void );
void	LightList_Add( INSTANCE_INDEX inst_index );
void	LightList_Update( ICamera *camera, float dt );
void	LightList_Render( ICamera *camera, float light_size );
U32		LightList_GetCount( void );
ILight *LightList_GetILight( U32 list_index ); 
void	LightList_Enable( U32 list_index, bool enabled ); 

//

bool BuiltinLight_IsValid( void );
void BuiltinLight_Create( float r, float g, float b, float range );
void BuiltinLight_Destroy( void );
void BuiltinLight_Enable( bool enabled );
void BuiltinLight_Update( ICamera *camera, float dt );
void BuiltinLight_Render( ICamera *camera, float light_size );
void BuiltinLight_Attach( INSTANCE_INDEX parent_inst_index );
void BuiltinLight_Detach( void );
void BuiltinLight_EnableOrbit( bool enabled );
void BuiltinLight_ToggleOrbitDirection( void );
void BuiltinLight_SelectColor( void );

//

#endif

#endif // EOF

