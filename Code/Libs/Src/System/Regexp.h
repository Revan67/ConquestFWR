#ifndef REGEXP_H
#define REGEXP_H
//
// REGEXP.H - Regular expression parser class
//

//
// This class makes using the above code easier under C++
// WARNING: All returned pointers point into this class's static
// string buffer.  DO NOT store the returned pointer for later use.
// This also makes the class thread non-safe.
//

struct RegExp
{
	static char buffer [80];
	void *compExp;           // compiled expression
	int b;
	char *start;

	RegExp() { }
	RegExp( char *regex );
		// Feed the constructor the regular expression, which
		// gets compiled for speed purposes

	void set( char *regex );
		// Change the regular expression to the one listed.
		// WARNING: The expression gets compiled.

	~RegExp();

	int test( char *string );
		// Feed the string to compare to.  Returns if anything
		// was found or not.  If anything was found, then
		// use the get() to extract the parenthesized subparts
		// starting at offset 1.  0 is used for the whole matching string
		// You can ignore the return value if you want

	char *get( int i, char *dst=(char*)0, int dstSize=80, int skip=0, int retNULL=1 );
		// Get subpart, 0 = whole matching, 1 = first parenthesis
		// If you don't supply a dst buffer, the static one
		// will be used.
		// This will null terminate the string for you
		// If you specify skip, it will skip over that many chars
		// at the head of the string.  Useful when you are
		// extracting words that are optionally delimited
		// if retNULL is set, returns nul on empty string, otherwise returns ""

	int getPos( int i );
		// Gives the position in the string of the ith element (see above)
		// WARNING! This can return -1 if it wasn't found

	int getRealPos( int i );
		// Gives the position relative to the whole string
};

#endif