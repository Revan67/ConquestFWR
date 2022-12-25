/**************************************************************
* RegExMatch.hpp: Regular Expression matching
*
* Chris N. Haddan
* March 31st, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#include "RegExMatch.hpp"


bool RegExMatch (const char *p, const char *s)
{
	if (!p) return false;
	// while we have pattern
	while (*p)
	{
		// if end of string, and still more pattern, fail, except if on wildcard
		if (!*s && *p!='*') 
		{
			return false;
		}

		// compare literal chars, skip ?, and dispatch *
		switch (*p)
		{
			case '?': 
				s++;
				break;

			case '*':
				return RegExMatchWildcard (p, s);

			default:
				if (*p != *s++) 
					return false;
		}
		p++;
	}

	// see if we have string left overs
	if (*s)
	{
		return false;
	}

	// all chars compared, no left overs
	return true; 
}


bool RegExMatchWildcard (const char *p, const char *s)
{	
	// skip multiple wildcards
	while (*p == '*' || *p == '?')
	{
		if (*p=='?' && !*s++)
		{ 
			return false;
		}
		p++;
	}

	// if wildcard is end of pattern, we always have a match.
	if (!*p)
	{
		return true;
	}

	//match what is after the wildcard		
	while (*s)
	{
		if (*p==*s)
		{
			if (RegExMatch (p,s))
			{
				return true;
			}
		}
		s++;
	}

	// check for left over pattern string
	if (*p)
	{
		return false;
	}

	// everything has been compared. no left overs.
	return true;
}