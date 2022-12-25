#ifndef RANDOMNUM_H
#define RANDOMNUM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              RandomNum.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RandomNum.h 2     12/15/98 7:00p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

struct RandomNumGenerator
{
	int *randomIndex;			// randomIndex[numRandomEntries]
	int numRandomEntries;


	RandomNumGenerator (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~RandomNumGenerator (void)
	{
		delete [] randomIndex;
	}

	void init (int numEntries)
	{
		delete [] randomIndex;

		randomIndex = new int[numEntries];

		numRandomEntries = numEntries;
		while (numEntries-- > 0)
			randomIndex[numEntries] = numEntries;
	}

	int rand (void)    // returns -1 when out of numbers
	{
		if (numRandomEntries <= 0)
			return -1;

		int index = ::rand();

		if (numRandomEntries > RAND_MAX/2)
			index *= ::rand();

		index = index % numRandomEntries;
		int result = randomIndex[index];

		randomIndex[index] = randomIndex[--numRandomEntries];

		return result;
	}
};


#endif