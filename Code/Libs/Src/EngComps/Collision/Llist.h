//
// Linked list class template.
//

#ifndef LLIST_H
#define LLIST_H

//

#include "typedefs.h"
#include <stdlib.h>

//
// struct Link is the wrapper around the class being listed.
//
template <typename T> struct Link
{
	T		obj;
	Link *	prev;
	Link *	next;
};

//

template <typename T> class LinkedList
{
protected:

	Link<T> *	head;
	Link<T> *	tail;
	S32			count;

public:

	LinkedList(void)
	{
		reset();
	}

	~LinkedList(void)
	{
		free();
	}

	void reset(void)
	{
		head = tail = NULL;
		count = 0;
	}

	void free(void)
	{
		Link<T> * node = head;
		while (node)
		{
			Link<T> * next = node->next;
			delete node;
			node = next;
		}

		reset();
	}

	S32 get_count(void) const
	{
		return count;
	}

	bool empty(void) const
	{
		return (head == NULL);
	}

	Link<T> * get_head(void) const
	{
		return head;
	}

	Link<T> * get_tail(void) const
	{
		return tail;
	}

	T operator [] (int index)
	{
	//
	// This does a linear search for now. For linked lists that need indexed 
	// element access, we should create a smarter list with a lookup table...
	//
		int i = 0;
		Link<T> * node = head;
		while (node && i < index)
		{
			node = node->next;
			i++;
		}

		return node->obj;
	}

	void insert(T & t, Link<T> * next = NULL)
	{
		Link<T> * node = new Link<T>;
		node->obj = t;

		Link<T> * prev;

		if (next)
		{
			prev = next->prev;
			next->prev = node;
		}
		else
		{
			prev = tail;
			tail = node;
		}

		if (prev)
		{
			prev->next = node;
		}
		else
		{
			head = node;
		}

		node->next = next;
		node->prev = prev;

		count++;
	}

	inline void add(T & t)
	{
		insert(t, NULL);
	}

	void remove(T & t)
	{
		Link<T> * node = head;
		while (node)
		{
			if (t == node->obj)
			{
				remove(node);
				delete node;
				break;
			}

			node = node->next;
		}
	}

	void remove(Link<T> * node)
	{
		if (node->prev == NULL)
		{
			head = node->next;
		}
		else
		{
			node->prev->next = node->next;
		}

		if (node->next == NULL)
		{
			tail = node->prev;
		}
		else
		{
			node->next->prev = node->prev;
		}

		count--;
	}
};

//

#endif