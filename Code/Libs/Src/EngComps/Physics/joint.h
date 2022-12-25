//
//
//

#ifndef JOINT_H
#define JOINT_H

//

#include "stddat.h"

//

struct Instance;	// Forward reference.

//
// Joint is the structure that holds the connectivity details as a list
// of connection type and their relevant associated data.
//
struct Joint
{
	JointType	type;
	Instance *	inboard_link;
	Instance *	outboard_link;

// HashPool stuff.
	U32		hash_key;
	Joint *	hash_next;   
	Joint *	hash_prev;  
	Joint *	next;          
	Joint *	prev;       
	S32		index;

	union _data
	{
	// Fixed child.
		struct 
		{
			Vector		offset;
			Quaternion	orient;
		};

	// Revolute joint.
		struct
		{
			Vector		axis;
			Vector		d;
		};
	} data;

// More HashPool stuff.
	static U32 hash(const void * joint);
	BOOL32 compare(const void * joint);

	void initialize(const void * joint)
	{
		Joint * j = (Joint *) joint;
		type			= j->type;
		inboard_link	= j->inboard_link;
		outboard_link	= j->outboard_link;
		memcpy(&data, &(j->data), sizeof(data));
	}

	void shutdown(void) {}
	void display(void) {}


};

//

typedef HashPool<Joint, 256>	JointPool;

//

struct JointNode
{
	Joint *		joint;
	JointNode *	prev;
	JointNode *	next;
};

//

typedef LList<JointNode> JointList;

//

#endif