// Author: Shaival Varma
// --------------------------------------------------------------------------
#include <iostream>
#include "erase_test.h"
#include "da_vector"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
typedef da_std::vector<int> IntVector;

void show_vector(IntVector &theVector);

void erase_test()
{
	const unsigned int kArraySize = 10;

    // Dynamically allocated vector begins with 0 elements.
    IntVector theVector;

    // Show what's in the vector (actually, nothing).
    show_vector(theVector);

    // Intialize the vector to contain the numbers 0-9.
    for (int idx = 0; idx < kArraySize; idx++)
	{
        theVector.push_back(idx);
	}

    // Output the contents of the dynamic vector of integers.
    show_vector(theVector);

    // Using void iterator erase(iterator Iterator) to
    // delete the 6th element (Index starts with 0).
    theVector.erase(theVector.begin() + 5);

    // Show what's left 
    show_vector(theVector);

    // Using iterator erase(iterator First, iterator Last) to
    // delete a range of elements all at once.
    theVector.erase(theVector.begin(), theVector.end());

    // Show what's left (actually, nothing).
    show_vector(theVector);
}

// Output the contents of the dynamic vector or display a
// message if the vector is empty.
void show_vector(IntVector &theVector)
{
    // First see if there's anything in the vector. Quit if so.
    if (theVector.empty())
    {
        cout << endl << "theVector is empty." << endl;
        return;
    }

    // Iterator is used to loop through the vector.
    IntVector::iterator theIterator;

    // Output contents of theVector.
    cout << endl << "theVector [ " ;

    for (theIterator = theVector.begin(); theIterator != theVector.end(); theIterator++)
    {
        cout << *theIterator;

        if (theIterator != theVector.end()-1) 
		{
			cout << ", "; // cosmetics for the output
		}
    }

    cout << " ]" << endl ;
}

