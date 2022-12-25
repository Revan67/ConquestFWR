#ifndef _PERSISTHARDPOINT_H_
#define _PERSISTHARDPOINT_H_

#include "persistmath.h"

const char HP_ROOT_NAME[] = "Hardpoints";
const char HP_FIXED_NAME[] = "Fixed";
const char HP_REVOLUTE_NAME[] = "Revolute";
const char HP_PRISMATIC_NAME[] = "Prismatic";

const unsigned int PERSIST_MAX_HARDPOINT_NAME = 64;

struct PersistHPSpot
{
	char name[PERSIST_MAX_HARDPOINT_NAME];
	PersistVector point;
	PersistMatrix orientation;
};

typedef PersistHPSpot PersistHPFixed;

struct PersistHPPrismatic
{
	PersistHPSpot spot;

	PersistVector axis;
	SINGLE min;
	SINGLE max;
};

typedef PersistHPPrismatic PersistHPRevolute;

#endif
