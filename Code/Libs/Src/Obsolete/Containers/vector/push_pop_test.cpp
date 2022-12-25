// Author: Shaival Varma
// --------------------------------------------------------------------------
#include <iostream>
#include "push_pop_test.h"
#include "da_vector"
// --------------------------------------------------------------------------
using namespace std ;
// --------------------------------------------------------------------------
typedef da_std::vector<int> IntVector;
// --------------------------------------------------------------------------
void push_pop_test()
{
    // Dynamically allocated vector begins with 0 elements.
    IntVector theVector;    // Iterator is used to loop through the vector.

    IntVector::iterator theIterator;
    // Add one element to the end of the vector, an int with the value 42.
    theVector.push_back(42) ;

    // Add two more elements to the end of the vector.
    // theVector will contain [ 42, 1, 109 ].    
	theVector.push_back(1) ;
    theVector.push_back(109) ;    
	
	// Erase last element in vector.
    theVector.pop_back();    
	// Print contents of theVector. Shows [ 42, 1 ]

    cout << "theVector [ " ;

    for (theIterator = theVector.begin(); theIterator != theVector.end(); theIterator++)    
	{
		cout << *theIterator;
        
		if (theIterator != theVector.end()-1)
		{
			cout << ", ";
		}
	}    

	cout << " ]" << endl ;
}
