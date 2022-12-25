// Author: Shaival Varma
// --------------------------------------------------------------------------
#include <iostream>
#include "front_back_test.h"
#include "da_vector"
// --------------------------------------------------------------------------
using namespace std ;
// --------------------------------------------------------------------------
typedef da_std::vector<int> IntVector;
// --------------------------------------------------------------------------
void front_back_test()
{
    // Dynamically allocated vector begins with 0 elements.
    IntVector theVector;

	const unsigned int kArraySize = 4;

    // Intialize the array to contain the members [100, 200, 300, 400]
    for (int cEachItem = 0; cEachItem < kArraySize; cEachItem++)
	{
        theVector.push_back((cEachItem + 1) * 100);
	}

	// Show the state
    cout << "First element: " << theVector.front() << endl;
    cout << "Last element: " << theVector.back() << endl;
    cout << "Elements in vector: " << theVector.size() << endl;
    
	// Delete the last element of the vector. Remember that the vector
    // is 0-based, so theVector.end() actually points 1 element beyond
    // the end.    
	theVector.erase(theVector.end() - 1);
    cout << endl << "After erasing last element, new last element is: " << theVector.back() << endl;
    
	// Delete the first element of the vector.
    theVector.erase(theVector.begin());
    cout << "After erasing first element, new first element is: " << theVector.front() << endl;
    cout << "Elements in vector: " << theVector.size() << endl;
}