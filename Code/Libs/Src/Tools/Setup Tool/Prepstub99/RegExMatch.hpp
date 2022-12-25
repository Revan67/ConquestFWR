/**************************************************************
* RegExMatch.hpp: Regular Expression matching
*
* Chris N. Haddan
* March 31st, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __REGEXMATCH_H
#define __REGEXMATCH_H

bool RegExMatchWildcard (const char *p, const char *s);
bool RegExMatch (const char *p, const char *s);

#endif