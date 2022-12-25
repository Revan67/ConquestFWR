//
//
//

#ifndef STACK_H
#define STACK_H

//

//

template <class T, int SIZE> class Stack
{
protected:

	T *	list[SIZE];
	int	size;
	int	top;

public:

	Stack(void)
	{
		size	= SIZE;
		top		= -1;
	}

	bool is_empty(void) const
	{
		return (top < 0);
	}

	void clear(void)
	{
		top = -1;
	}

	void push(T * ptr)
	{
#ifdef ASSERT
		ASSERT(top < size);
#endif
		list[++top] = ptr;
	}

	T * pop(void)
	{
#ifdef ASSERT
		ASSERT(top >= 0);
#endif
		return list[top--];
	}

};

//

#endif