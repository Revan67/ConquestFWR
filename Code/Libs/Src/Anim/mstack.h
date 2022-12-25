#ifndef MSTACK_H
#define MSTACK_H

//

#include "MNode.h"

//

#define MAX_STACK_ENTRIES	256

//

struct MotionStack
{
	MotionStackNode *	list[MAX_STACK_ENTRIES];
    int					index;

	MotionStack(void)
	{
		memset(this, 0, sizeof(*this));
	}

	void push(MotionStackNode * node);
	MotionStackNode * pop(void);

	void evaluate(void * dst, int data_size);

// Eliminating an object from the stack can result in discontinuous motion.
	void remove(U32 object_idx);

	bool is_empty(void) const
	{
		return (index == 0);
	}
};

//

#endif