#ifndef THASHLIST_H
#define THASHLIST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                THashList.h                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/include/THashList.h 3     9/24/03 10:08a Tmauer $
*/			    
//--------------------------------------------------------------------------//

//#ifndef CQTRACE_H
//#include "CQTrace.h"
//#endif

//--------------------------------------------------------------------------//
//
template <class Type, class HashType, S32 hashSize=256>
struct HashList
{
	Type * table[hashSize];   // Hash bucket array for fast searches
	
	HashList (void)
	{
		memset(this, 0, sizeof(*this));
	}
	
	~HashList (void)
	{
		reset ();
	}
	
private:
	void resetHash (S32 index)
	{
		Type * node, * tmp;

		if ((node = table[index]) != 0)
		{
			table[index] = 0;
			while (node)
			{
				tmp = node;
				node = node->pNext;
				delete tmp;
			}
		}
	}
	void printHash (S32 index) const
	{
		Type * node=table[index];
		while (node)
		{
			node->print();
			node = node->pNext;
		}
	}
	template <class CmpType> 
	Type * findNode (const CmpType & value, S32 index)
	{
		Type * node=table[index];
		while (node)
		{
			if (node->compare(value))
				break;
			node = node->pNext;
		}

		return node;
	}

public:
	
	void reset(void)
	{
		S32 i;
		for (i = 0; i < hashSize; i++)
			resetHash(i);
	}

	void addNode (Type * node)
	{
		U32 key = node->hash() % hashSize;

		//CQASSERT(node->pNext==0);
		node->pNext = table[key];
		table[key] = node;
	}

	void removeNode (Type * node)
	{
		U32 key = node->hash() % hashSize;
		Type * prev = table[key];

		if (node == prev)
			table[key] = prev->pNext;
		else
		if (prev)
		{
			while (prev->pNext && prev->pNext != node)
				prev = prev->pNext;
			if (prev->pNext)
				prev->pNext = node->pNext;
		}
	}

	Type * findHashedNode (const HashType & cmp)
	{
		const U32 key = Type::hash(cmp) % hashSize;
		Type * node = table[key];
		Type * prev = 0;

		while (node)
		{
			if (node->compare(cmp))
			{
				// move node back to the start of the list
				if (prev)
				{
					prev->pNext = node->pNext;
					node->pNext = table[key];
					table[key] = node;
				}
				break;
			}
			prev = node;
			node = node->pNext;
		}

		return node;
	}

	template <class CmpType> 
	Type * findNode (const CmpType & value)
	{
		S32 i;
		Type * node = 0;
		for (i = 0; i < hashSize; i++)
			if ((node = findNode(value, i)) != 0)
				break;

		return node;
	}

	void print (void) const
	{
		S32 i;
		for (i = 0; i < hashSize; i++)
			printHash(i);
	}

	Type * getFirstNode()
	{
		S32 i;
		for (i = 0; i < hashSize; i++)
		{
			if(table[i])
				return table[i];
		}
		return NULL;
	}

	template <class CmpType> 
	Type * findNextNode(const CmpType & current)
	{
		S32 i;
		for (i = 0; i < hashSize; i++)
		{
			Type * node=table[i];
			while (node)
			{
				if (node->compare(current))
				{
					if(node->pNext)
						return node->pNext;
					else
					{
						++i;
						for (; i < hashSize; i++)
						{
							if(table[i])
								return table[i];
						}
						return getFirstNode();
					}
				}
				node = node->pNext;
			}
		}
		return NULL;
	}

	template <class CmpType> 
	Type * findPrevNode(const CmpType & current)
	{
		S32 i;
		Type * prev = NULL;
		for (i = 0; i < hashSize; i++)
		{
			Type * node=table[i];
			while (node)
			{
				if (node->compare(current))
				{
					if(prev)
					{
						return prev;
					}
					else
					{
						for (i = hashSize-1; i >= 0; i--)
						{
							if(table[i])
							{
								node = table[i];
								while(node->pNext)
								{
									node = node->pNext;
								}
								return node;
							}
						}
					}
					return NULL;
				}
				prev = node;
				node = node->pNext;
			}
		}
		return NULL;
	}

	U32 nodeCount()
	{
		S32 count = 0;
		S32 i;
		for (i = 0; i < hashSize; i++)
		{
			Type * node = table[i];
			while(node)
			{
				++count;
				node = node->pNext;
			}
		}
		return count;
	}
};


//--------------------------------------------------------------------------//
//------------------------------End THashList.h-----------------------------//
//--------------------------------------------------------------------------//
#endif
