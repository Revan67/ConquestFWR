// RenderArchetype.h
//
//
//


#ifndef __RenderArchetype_h__
#define __RenderArchetype_h__

//

#include <float.h>

//

struct RenderArchetypeLod
{
	RENDER_ARCHETYPE		 render_arch_index;		// The render archetype index of this lod.
	IRenderComponent		*render_component;		// The render component that created this lod.
	float					 switch_in_fraction;	// The lod 'fraction' to start rendering this lod.

	RenderArchetypeLod();
	~RenderArchetypeLod();
};

//

RenderArchetypeLod::RenderArchetypeLod()
{
	render_component = NULL;
	render_arch_index = INVALID_RENDER_ARCHETYPE;
	switch_in_fraction = 0.0f;
}

//

RenderArchetypeLod::~RenderArchetypeLod()
{
	render_component = NULL;
}

//

struct RenderArchetype
{
public: // Interface

	RenderArchetype();
	RenderArchetype( const RenderArchetype &ra );
	~RenderArchetype();
	RenderArchetype &operator=( const RenderArchetype &ra );

	int get_num_lod_levels( void ) const;
	bool set_num_lod_levels( int _num_lod_levels );

	const RenderArchetypeLod *get_lod_at_level( int lod_level ) const;

	bool set_render_component_at_level( int lod_level, IRenderComponent *render_component );
	bool set_switch_in_fraction_at_level( int lod_level, float switch_in_fraction );
	bool set_render_archetype_at_level( int lod_level, RENDER_ARCHETYPE render_arch_index );

	const RenderArchetypeLod *get_lod_at_fraction( float lod_fraction ) const;

	float get_minimum_render_distance( void ) const;
	bool set_minimum_render_distance( float min_fraction );

	float get_maximum_render_distance( void ) const;
	bool set_maximum_render_distance( float max_fraction );

public:	// Data

	RenderArchetypeLod *lod_levels;
	int num_lod_levels;

	float inv_lod_fraction_span;

	// These controll when an object becomes visible and when it dissappears again.
	// there is only a single pair of these numbers regardless of the number of
	// dicrete lod levels or if it's continuous.
	// 0 to FLT_MAX is the default
	SWITCH_DISTANCE_TYPE min_render_distance;
	SWITCH_DISTANCE_TYPE max_render_distance;

	SINGLE lod_start_dist;	// when the first LOD switch occurs (continuous or not)
	SINGLE lod_stop_dist;	// when the last LOD switch occurs (continuous or not)
};

//

RenderArchetype::RenderArchetype()
{
	lod_levels = NULL;
	num_lod_levels = 0;

	inv_lod_fraction_span = 1.0f;

	min_render_distance = 0.0f;
	max_render_distance = FLT_MAX;

	lod_start_dist = 0.0f;
	lod_stop_dist = FLT_MAX;
}

//

RenderArchetype::~RenderArchetype()
{
	set_num_lod_levels( 0 );
}

//

RenderArchetype::RenderArchetype( const RenderArchetype &ra )
{
	lod_levels = NULL;
	num_lod_levels = 0;

	min_render_distance = 0.0f;
	max_render_distance = 100000.0f;

	operator=( ra );
}

//

RenderArchetype & RenderArchetype::operator=( const RenderArchetype &ra )
{
	set_minimum_render_distance( ra.get_minimum_render_distance() );
	set_maximum_render_distance( ra.get_maximum_render_distance() );
	
	lod_start_dist = ra.lod_start_dist;
	lod_stop_dist = ra.lod_stop_dist;
	set_num_lod_levels( ra.get_num_lod_levels() );

	const RenderArchetypeLod *level;
	for( int lod_level = 0 ; lod_level < num_lod_levels; lod_level ++ ) {
		level = ra.get_lod_at_level( lod_level );
		set_render_component_at_level( lod_level, level->render_component );
		set_switch_in_fraction_at_level( lod_level, level->switch_in_fraction );
		set_render_archetype_at_level( lod_level, level->render_arch_index );
	}

	return *this;
}

//

int RenderArchetype::get_num_lod_levels( void ) const
{
	return num_lod_levels;
}

//

bool RenderArchetype::set_num_lod_levels( int _num_lod_levels )
{
	delete[] lod_levels;
	lod_levels = NULL;

	if( _num_lod_levels && (lod_levels = new RenderArchetypeLod[ _num_lod_levels ]) == NULL ) {
		return false;	
	}

	num_lod_levels = _num_lod_levels;
	return true;
}

//

const RenderArchetypeLod *RenderArchetype::get_lod_at_level( int lod_level ) const
{
	ASSERT( lod_level >= 0 );
	ASSERT( lod_level < num_lod_levels );
	ASSERT( lod_levels != NULL );

	return &lod_levels[ lod_level ];
}

//

bool RenderArchetype::set_render_component_at_level( int lod_level, IRenderComponent *render_component )
{
	ASSERT( lod_level >= 0 );
	ASSERT( lod_level < num_lod_levels );
	ASSERT( lod_levels != NULL );
	ASSERT( render_component != NULL );

	lod_levels[lod_level].render_component = render_component;

	return true;
}

//

bool RenderArchetype::set_switch_in_fraction_at_level( int lod_level, float switch_in_fraction )
{
	ASSERT( lod_level >= 0 );
	ASSERT( lod_level < num_lod_levels );
	ASSERT( lod_levels != NULL );
	ASSERT( switch_in_fraction >= 0.0f );

	lod_levels[lod_level].switch_in_fraction = switch_in_fraction ;

	return true;
}

//

bool RenderArchetype::set_render_archetype_at_level( int lod_level, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( lod_level >= 0 );
	ASSERT( lod_level < num_lod_levels );
	ASSERT( lod_levels != NULL );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	lod_levels[lod_level].render_arch_index = render_arch_index;

	return true;
}

//

const RenderArchetypeLod *RenderArchetype::get_lod_at_fraction( float lod_fraction ) const
{
	ASSERT( lod_levels != NULL );
	
	if( lod_fraction < 0.0f ) {
		return NULL;
	}

	for( int lod_level = 0; lod_level < num_lod_levels; lod_level++ ) {
		if( lod_levels[ lod_level ].switch_in_fraction <= lod_fraction ) {
			return &lod_levels[ lod_level ];
		}
	}

	return NULL;
}

//

SWITCH_DISTANCE_TYPE RenderArchetype::get_minimum_render_distance( void ) const
{
	return min_render_distance;
}

//

bool RenderArchetype::set_minimum_render_distance( SWITCH_DISTANCE_TYPE min_distance )
{
	min_render_distance = min_distance;
	return true;
}

//

SWITCH_DISTANCE_TYPE RenderArchetype::get_maximum_render_distance( void ) const
{
	return max_render_distance;
}

//

bool RenderArchetype::set_maximum_render_distance( SWITCH_DISTANCE_TYPE max_distance )
{
	max_render_distance = max_distance;
	return true;
}

//


#endif //EOF
