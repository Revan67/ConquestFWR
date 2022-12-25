// Author: Shaival Varma
// --------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <algorithm>
#include "equal_and_compare_test.h"
#include "da_vector"
// --------------------------------------------------------------------------
using namespace std;
using namespace std::rel_ops;
// --------------------------------------------------------------------------
// The ID class is used for team scoring. It holds each player's name
// and score.
class ID
{
	public:    
		string Name;    
		int Score;

		ID()
		: Name(""), Score(0) 
		{
		}
    
		ID(string NewName, int NewScore) 
		: Name(NewName), Score(NewScore) 
		{
		}
};
// --------------------------------------------------------------------------
// An ID is equivalent only if both name and score match.
bool operator==(const ID& x, const ID& y)
{
    return (x.Name == y.Name) && (x.Score == y.Score);
}
// --------------------------------------------------------------------------
// IDs will be sorted by Score, not by Name.
bool operator<(const ID& x, const ID& y)
{    
	return x.Score < y.Score;
}
// --------------------------------------------------------------------------
typedef da_std::vector<ID> NameVector;
// --------------------------------------------------------------------------
void equal_and_compare_test()
{
    // Declare 3 dynamically allocated vectors of names.
    NameVector Vector1, Vector2, Vector3;    // Create 3 short vectors of names.

    Vector1.push_back(ID("Karen Palmer", 2));
    Vector1.push_back(ID("Ada Campbell", 1));
    
	Vector2.push_back(ID("John Woloschuk", 3));
    Vector2.push_back(ID("Grady Leno", 2));
    
	Vector3.push_back(ID("Karen Palmer", 2));
    Vector3.push_back(ID("Ada Campbell", 1));
    
	// Compare Vector1 to Vector2 and show whether they're equivalent.
    Vector1 == Vector2 ? cout << "Vector1 == Vector2" : cout << "Vector1 != Vector2";    
	cout << endl;
    // Compare Vector1 to Vector3 and show whether they're equivalent.
    Vector1 == Vector3 ? cout << "Vector1 == Vector3" : cout << "Vector1 != Vector3";
    cout << endl;
}
// --------------------------------------------------------------------------
