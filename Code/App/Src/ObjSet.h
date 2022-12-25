#ifndef OBJSET_H
#define OBJSET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ObjSet.h                                  //
//																			//
//                   COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjSet.h 6     9/22/00 12:45p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

const DWORD * __fastcall dwordsearch (DWORD len, DWORD value, const DWORD * buffer);

struct ObjSet
{
	U32 numObjects;
	U32 objectIDs[MAX_SELECTED_UNITS];

	ObjSet (void)
	{
		numObjects = 0;
	}

	ObjSet (const ObjSet & set)
	{
		*this = set;
	}

	//
	// operators
	//

	bool operator == (const ObjSet & set) const
	{
		return (numObjects == set.numObjects &&
				(*this & set).numObjects == numObjects);
	}

	bool operator != (const ObjSet & set) const
	{
		return (numObjects != set.numObjects ||
				(*this & set).numObjects != numObjects);
	}

	bool contains (U32 object) const
	{
		return (dwordsearch(numObjects, object, objectIDs) != NULL);
	}

	bool isSubsetOf (const ObjSet & set) const
	{
		bool result = true;
		U32 i = 0;
		const U32 * ptr;

		while (i < numObjects)
		{
			if ((ptr = dwordsearch(set.numObjects, objectIDs[i], set.objectIDs)) != 0)
			{
				if (ptr + (numObjects-i) <= set.objectIDs + MAX_SELECTED_UNITS)
				{
					if (memcmp(objectIDs+i+1, ptr+1, numObjects-(i+1)) == 0)
						break;
				}

				i++;
			}
			else
			{
				result = false;
				break;
			}
		}

		return result;
	}

	ObjSet & operator += (const ObjSet & set)
	{
		U32 i;

		for (i = 0; i < set.numObjects; i++)
		{
			if (contains(set.objectIDs[i]) == false)
			{
				if (numObjects < MAX_SELECTED_UNITS)
					objectIDs[numObjects++] = set.objectIDs[i];
				else
					break;  // no room at the inn
			}
		}

		return *this;
	}
	
	ObjSet operator + (const ObjSet & set) const
	{
		ObjSet result = *this;

		result += set;
		return result;
	}

	bool removeObject (U32 object)
	{
		U32 * match = const_cast<U32 *>(dwordsearch(numObjects, object, objectIDs));

		if (match)
		{
			*match = objectIDs[--numObjects];
			return true;
		}
		else
			return false;
	}

	bool addObject (U32 object)
	{
		// return false if we are already full
		if (numObjects < MAX_SELECTED_UNITS)
		{
			U32 * match = const_cast<U32 *>(dwordsearch(numObjects, object, objectIDs));

			if (match)
			{
				// the object is already in the list
				return false;
			}
			else
			{
				objectIDs[numObjects++] = object;
				return true;
			}
		}
		return false;
	}

	ObjSet & operator -= (const ObjSet & set)
	{
		U32 i;

		if (numObjects)
		for (i = 0; i < set.numObjects; i++)
		{
			U32 * match = const_cast<U32 *>(dwordsearch(numObjects, set.objectIDs[i], objectIDs));

			if (match)
				*match = objectIDs[--numObjects];
		}

		return *this;
	}

	ObjSet operator - (const ObjSet & set) const
	{
		ObjSet result = *this;

		result -= set;
		return result;
	}

	ObjSet & operator &= (const ObjSet & set)
	{
		U32 i=numObjects;

		while (i-- > 0)
		{
			if (dwordsearch(set.numObjects, objectIDs[i], set.objectIDs) == 0)
			{
				objectIDs[i] = objectIDs[--numObjects];
			}
		}

		return *this;
	}

	ObjSet operator & (const ObjSet & set) const
	{
		ObjSet result = *this;

		result &= set;
		return result;
	}


};



#endif
