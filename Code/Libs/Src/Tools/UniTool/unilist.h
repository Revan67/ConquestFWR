#ifndef UNILIST_H
#define UNILIST_H
//
// UniList.h - A better version of UniList from stddat.h
//

template <typename T> class UniList;

template <typename T> class UniNode
{
	protected:
		
		friend UniList<T>;
		
		UniNode<T>* next;
		UniNode<T>* prev;
		
		UniNode<T> (void)
		{
		}
		
		~UniNode<T> (void)
		{
		}
		
	public:
		
		T object;
};

template <typename T> class UniList
{
	protected:
		
		UniNode<T>* head;
		UniNode<T>* tail;

		bool recycle_nodes;
		UniNode<T>* heap;

		void operator = (const UniList&)
		{
		}
		
		UniList (const UniList&)
		{
		}

		UniNode<T>* alloc_node (void);
		void free_node (UniNode<T>*);

	public:
		
		UniList (bool _recycle_nodes=false);
		
		void prepend (T _obj);
		void append (T _obj);
		
		UniNode<T>* get_head (void) const;
		UniNode<T>* get_tail (void) const;
		
		void insert_after (UniNode<T>* node, T _obj);
		
		//if 'remove' finds 'node' it removes it from the list and 'node' is set
		//to the previous node in the list so that traversal may continue naturally
		
		void remove (UniNode<T>*& node);
		
		bool traverse (UniNode<T>*& current, bool forward = true) const; 
		
		void append (UniList<T>& list);
		
		virtual ~UniList (void);
		
		void free (void);

		//EMAURER only useful if 'recycle_nodes' is true. causes all nodes
		//in the heap to be returned to free store.
		void free_heap (void);
		
		unsigned int count (void) const;
};

template <typename T> inline UniList<T>::UniList (bool _recycle_nodes)
{
	head = tail = heap = NULL;
	recycle_nodes = _recycle_nodes;
}

template <typename T> inline void UniList<T>::free_node (UniNode<T>* old)
{
	if (recycle_nodes && old)
	{
		old->next = heap;
		heap = old;
	}
	else
		delete old;
}

template <typename T> inline UniNode<T>* UniList<T>::alloc_node (void)
{
	UniNode<T>* result = NULL;

	if (recycle_nodes && (result = heap))
		heap = heap->next;

	if (!result)
		result = new UniNode<T>;

	return result;
}

template <typename T> void UniList<T>::prepend (T _obj)
{
	UniNode<T>* node = alloc_node ();

	node->next = head;
	node->prev = NULL;
	
	node->object = _obj;
	
	if (NULL != head)
		head->prev = node;
	else
		tail = node;
	
	head = node;
}

template <typename T> void UniList<T>::append (T _obj)
{
	UniNode<T>* node = alloc_node ();
	node->prev = tail;
	node->next = NULL;
	
	node->object = _obj;
	
	if (tail != NULL)
		tail->next = node;
	else
		head = node;
	
	tail = node;	
}

template <typename T> void UniList<T>::insert_after (UniNode<T>* node, T _obj)
{
	if (NULL == node)
		prepend (_obj);
	else if (tail == node)
		append (_obj);
	else
	{
		UniNode<T>* nn = alloc_node ();
		nn->object = _obj;
		
		nn->next = node->next;
		node->next = nn;
		
		//assert (nn->next);
		nn->next->prev = nn;
	}
}

template <typename T> UniNode<T>* UniList<T>::get_head (void) const
{
	return head;
}

template <typename T> UniNode<T>* UniList<T>::get_tail (void) const
{
	return tail;
}

template <typename T> void UniList<T>::remove (UniNode<T>*& node)
{
	if (NULL != node)
	{
		if (head == node)
		{
			head = node->next;
			
			if (NULL == head)
				tail = NULL;
			else
				head->prev = NULL;
		}
		else if (tail == node)
		{
			tail = tail->prev;
			
			if (NULL == tail)
				head = NULL;
			else
				tail->next = NULL;
		}
		else
		{
			UniNode<T>* prev = node->prev;
			UniNode<T>* next = node->next;
			
			//assert (prev);
			//assert (next);
			
			prev->next = next;
			next->prev = prev;
		}
		
		UniNode<T>* prev = node->prev;
		
		free_node (node);
		node = prev;
	}
}

template <typename T> bool UniList<T>::traverse(UniNode<T>*& current, bool forward) const
{
	if (NULL == current)
		current = (forward) ? head : tail;
	else if (forward)
		current = current->next;
	else
		current = current->prev;
	
	return (NULL != current);
}

template <typename T> void UniList<T>::append (UniList<T> & list)
{
	if (list.head)
	{
		if (tail)	//'this' list is not empty
		{
			tail->next = list.head;
			list.head->prev = tail;
			tail = list.tail;
		}
		else		//'this' list is empty
		{
			head = list.head;
			tail = list.tail;
		}
		
		list.head = list.tail = NULL;
	}
}

template <typename T> unsigned int UniList<T>::count (void) const
{
	unsigned int counter = 0;
	
	UniNode<T>* node = head;
	while (node != NULL)
	{
		++counter;
		node = node->next;
	}
	
	return counter;
}

template <typename T> void UniList<T>::free (void)
{
	UniNode<T>* node = head;
	while (node != NULL)
	{
		UniNode<T>* dead = node;
		node = node->next;
		free_node (dead);
	}
	
	head = tail = NULL;
}

template <typename T> void UniList<T>::free_heap (void)
{
	UniNode<T>* node = heap;

	while (node != NULL)
	{
		UniNode<T>* dead = node;
		node = node->next;
		delete dead;
	}
	
	heap = NULL;		
}

template <typename T> UniList<T>::~UniList (void)
{
	//will not call derived class' free ()
	free ();
	free_heap ();
}

#endif
