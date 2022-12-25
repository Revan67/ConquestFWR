// IParticleSystem.h
//
//
//
//

#ifndef __IParticleSystem_h__
#define __IParticleSystem_h__

//
#include "DACOM.h"
#include "ParticleSystemParameters.h"

//

#define IID_IParticleSystem  MAKE_IID( "IParticleSystem", 1 )

//

// ...........................................................................
//
// IParticleSystem
//
//
struct IParticleSystem : public IDAComponent
{
	virtual bool COMAPI initialize( void ) = 0;
	virtual bool COMAPI is_active( void ) = 0;

	virtual bool COMAPI set_parameters( const ParticleSystemParameters *new_parameters ) = 0;
	virtual bool COMAPI get_parameters( ParticleSystemParameters *out_parameters ) = 0;
};



#endif // EOF

