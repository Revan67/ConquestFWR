//$Header: /Tools/Exporters/common/PersistCompound.h 9     3/05/98 2:07p Mikes $
//Copyright (c) 1997 Digital Anvil, Inc.

#ifndef PERSISTCOMPOUND_H__
#define PERSISTCOMPOUND_H__

#include "persistmath.h"
#include "typedefs.h"

const int PARTNAME_MAX = 64;
const int PARTFILENAME_MAX = PARTNAME_MAX;

#define COMPOUND_DIR_NAME "Cmpnd"
#define CONNECTION_DIR_NAME "Cons"
#define JT_FIXED_NAME "Fix"
#define JT_PRISMATIC_NAME "Pris"
#define JT_REVOLUTE_NAME "Rev"
#define JT_SPRING_NAME "Spr"
#define JT_CYLINDRICAL_NAME "Cyl"
#define PART_STEM "Part"
#define ROOT_OBJ_NAME "Root"
#define SPOTS "Spots"

const char OBJECT_NAME[] = "Object name";
const char FILE_NAME[] = "File name";

const char JT_SPHERICAL_NAME[] = "Sphere";
const char JT_LOOSE_NAME[] = "Loose";
const char JT_TRANSLATIONAL_NAME[] = "Trans";

struct Fix
{
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];
	PersistVector pos;
	PersistMatrix orient;
};

typedef Fix Loose;
typedef Fix Trans;

struct Rev
{
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];
	PersistVector parent_point;
	PersistVector child_point;
	PersistMatrix rel_orientation;
	PersistVector axis;
	SINGLE min; // Radians
	SINGLE max;
};

typedef Rev Pris;

struct Cyl
{
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];
	PersistVector parent_point;
	PersistVector child_point;
	PersistMatrix rel_orientation;
	PersistVector axis;
	SINGLE min_trans; 
	SINGLE max_trans;
	SINGLE min_rot;
	SINGLE max_rot;
};

struct Spring
{
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];
	PersistVector parent_point;
	PersistVector child_point;
	SINGLE spring_constant;
	SINGLE damping_constant;
	SINGLE rest_length;
};

struct Sphere
{
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];

	PersistVector parent_point;
	PersistVector child_point;

	PersistMatrix rel_orientation;

	SINGLE min_about_i;
	SINGLE max_about_i;

	SINGLE min_about_j;
	SINGLE max_about_j;

	SINGLE min_about_k;
	SINGLE max_about_k;
};

#endif
