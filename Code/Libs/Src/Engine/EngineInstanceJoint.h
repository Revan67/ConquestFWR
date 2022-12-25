// EngineInstanceJoint
//
//
//

#ifndef __EngineInstanceJoint_h__
#define __EngineInstanceJoint_h__

//

#include "JointInfo.h"
#include "JointState.h"

//

struct EngineInstanceJoint 
{
	struct EngineInstance *child_instance;
	JointInfo			   info;
	JointState			   state;
	
	//

	EngineInstanceJoint() 
	{
		child_instance = NULL;

		memset( &info, 0, sizeof(info) );
		memset( &state, 0, sizeof(state) );
		state.w = 1.0f;
	}

	//

	EngineInstanceJoint( const EngineInstanceJoint &eij )
	{
		operator=(eij);
	}

	//

	const EngineInstanceJoint &operator=( const EngineInstanceJoint &eij ) 
	{
		memcpy( &info, &eij.info, sizeof( info ) );
		memcpy( &state, &eij.state, sizeof( state ) );
		child_instance = eij.child_instance;
		return *this;
	}

	//

	void set_state_vector_derivatives( const float *values ) 
	{
		switch( info.type ) {

		case JT_FIXED:
		case JT_LOOSE:
		case JT_TRANSLATIONAL:
		case JT_SPHERICAL:
			GENERAL_TRACE_1( "Attempt to set state vector derivative for unsupported joint type.\n" );
			break;

		case JT_CYLINDRICAL:
			state.p_dot = values[0];
			state.r_dot = values[1];
			break;

		case JT_REVOLUTE:
		case JT_PRISMATIC:
			state.q_dot = values[0];
			break;

		default:
			break;
		}
	}

	//

	void set_state_vector( const float *values ) 
	{
		switch( info.type ) {

		case JT_FIXED:
			break;

		case JT_REVOLUTE:
		case JT_PRISMATIC:
			state.q = (info.min0 > values[0]) ? info.min0 : (info.max0 < values[0]) ? info.max0 : values[0] ;
			break;

		case JT_CYLINDRICAL:
			state.p = values[0];
			state.r = values[1];
			break;

		case JT_SPHERICAL:
			state.w = values[0];
			state.x = values[1];
			state.y = values[2];
			state.z = values[3];
			break;

		case JT_TRANSLATIONAL:
			state.px = values[0];
			state.py = values[1];
			state.pz = values[2];
			break;

		case JT_LOOSE:
			state.px = values[0];
			state.py = values[1];
			state.pz = values[2];
			state.w = values[3];
			state.x = values[4];
			state.y = values[5];
			state.z = values[6];
			break;

		default:
			break;
		}
		return;
	}

	//

	void get_state_vector_derivatives( float *out_values ) 
	{
		switch( info.type ) {

		case JT_FIXED:
		case JT_LOOSE:
		case JT_TRANSLATIONAL:
		case JT_SPHERICAL:
			out_values[0] = 0.0f;
			break;

		case JT_CYLINDRICAL:
			out_values[0] = state.p_dot;
			out_values[1] = state.r_dot;
			break;

		case JT_REVOLUTE:
		case JT_PRISMATIC:
			out_values[0] = state.q_dot;
			break;

		default:
			break;
		}
	}

	//

	void get_state_vector( float *out_values )
	{
		switch( info.type ) {

		case JT_REVOLUTE:
		case JT_PRISMATIC:
			out_values[0] = state.q;
			break;

		case JT_CYLINDRICAL:
			out_values[0] = state.p;
			out_values[1] = state.r;
			break;

		case JT_SPHERICAL:
			out_values[0] = state.w;
			out_values[1] = state.x;
			out_values[2] = state.y;
			out_values[3] = state.z;
			break;

		case JT_TRANSLATIONAL:
			out_values[0] = state.px;
			out_values[1] = state.py;
			out_values[2] = state.pz;
			break;

		case JT_LOOSE:
			out_values[0] = state.px;
			out_values[1] = state.py;
			out_values[2] = state.pz;
			out_values[3] = state.w;
			out_values[4] = state.x;
			out_values[5] = state.y;
			out_values[6] = state.z;
			break;

		default:
			break;
		}

		return;
	}

};

//

typedef std::list<EngineInstanceJoint> EngineInstanceJointList;

//

#endif // EOF