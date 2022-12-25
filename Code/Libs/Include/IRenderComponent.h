#ifndef __IRENDERCOMPONENT_H
#define __IRENDERCOMPONENT_H

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef DAVARIANT_H
#include "DAVariant.h"
#endif

#include "RenderProp.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

typedef int RENDER_ARCHETYPE;
typedef S32 INSTANCE_INDEX;	// defined by Engine.h
#define INVALID_INSTANCE_INDEX  ((INSTANCE_INDEX)-1) // defined by Engine.h
typedef S32 ARCHETYPE_INDEX;
const RENDER_ARCHETYPE INVALID_RENDER_ARCHETYPE = -1;
struct IFileSystem;
enum vis_state;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct RendCompDesc : public DACOMDESC
{
	IDAComponent* system_services;
	IDAComponent* engine_services;

	RendCompDesc (void) : DACOMDESC ("IRenderComponent"), system_services (0), engine_services (0)
	{
		size = sizeof (*this);
	}
};

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IRENDERCOMPONENT_VERSION 6
#define IID_IRenderComponent MAKE_IID("IRenderComponent", 6)

struct DACOM_NO_VTABLE IRenderComponent : public IDAComponent
{

	virtual GENRESULT COMAPI set_render_property( const RenderProp name, DACOM_VARIANT value ) = 0;
	virtual GENRESULT COMAPI get_render_property( const RenderProp name, DACOM_VARIANT out_value ) = 0;
	virtual void COMAPI update( float dt ) = 0;

	virtual bool COMAPI create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys ) = 0;
	virtual bool COMAPI duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index ) = 0;
	virtual void COMAPI destroy_archetype( RENDER_ARCHETYPE render_arch_index ) = 0;
	virtual bool COMAPI split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index ) = 0;
	virtual bool COMAPI get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) = 0;
	virtual bool COMAPI get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) = 0;
	virtual bool COMAPI get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid ) = 0;
	virtual bool COMAPI query_archetype_interface( RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid ) = 0;

	virtual bool COMAPI create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) = 0;
	virtual void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) = 0;
	virtual void COMAPI update_instance( INSTANCE_INDEX inst_index, float dt ) = 0;
	virtual vis_state COMAPI render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr ) = 0;
	virtual bool COMAPI get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) = 0;		
	virtual bool COMAPI query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid ) = 0;


	// These will eventually go away, feel free to implement them in the slowest way
	// possible.
	//
	virtual struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) = 0;
	virtual struct Mesh * COMAPI get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) = 0;
	virtual GENRESULT COMAPI release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) = 0;
	virtual struct Mesh * COMAPI get_archetype_mesh( RENDER_ARCHETYPE render_arch_index ) = 0;
};

#endif 
