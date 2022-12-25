// ParticleSystemArchetype.h
//
//
//
//

#ifndef __ParticleSystemArchetype_h__
#define __ParticleSystemArchetype_h__

//

#include "ParticleSystemParameters.h"
#include "FDUMP.h"

#include "IParticleSystem.h"
#include "Filesys_Utility.h"
#include "rendpipeline.h"
#include "ITextureLibrary.h"

//

#define PSA_LOAD_OLD_FORMAT 

//

// .............................................................................
//
// ParticleSystemArchetype
//
// Defines a particle system 
//
struct ParticleSystemArchetype : public IParticleSystem
{
protected: // Data
	ParticleSystemParameters parameters;
	ITL_TEXTURE_ID texture_id;
	ITextureLibrary *texturelibrary;

public: // Interface

	ParticleSystemArchetype( void ) ;
	~ParticleSystemArchetype( void ) ;
	ParticleSystemArchetype( const ParticleSystemArchetype &psa ) ;
	ParticleSystemArchetype &operator=( const ParticleSystemArchetype &psa );

	bool load_from_filesystem( IFileSystem *filesys, ITextureLibrary *texturelibray );
	bool load_from_filesystem_old( IFileSystem *filesys, ITextureLibrary *texturelibray );


	// IParticleSystem
	bool COMAPI initialize( void );
	bool COMAPI reset( void ) ;
	bool COMAPI is_active( void );
	bool COMAPI set_parameters( const ParticleSystemParameters *new_parameters );
	bool COMAPI get_parameters( ParticleSystemParameters *out_parameters );

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const C8 *interface_name, void **out_iif );
	U32 COMAPI AddRef( void );
	U32 COMAPI Release( void );

};

//

#endif // EOF
