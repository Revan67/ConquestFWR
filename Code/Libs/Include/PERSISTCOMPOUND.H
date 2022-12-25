//$Header: /Conquest/Libs/Include/PERSISTCOMPOUND.H 5     4/28/00 11:57p Rmarr $
//Copyright (c) 1997 Digital Anvil, Inc.

#ifndef PERSISTCOMPOUND_H__
#define PERSISTCOMPOUND_H__

#include "persistmath.h"
#include "typedefs.h"


const int PARTNAME_MAX = 64;
const int PARTFILENAME_MAX = PARTNAME_MAX;

#define COMPOUND_DIR_NAME "Cmpnd"
#define IFS_COMPOUND_DIRECTORY "Cmpnd"
#define CONNECTION_DIR_NAME "Cons"
#define JT_FIXED_NAME "Fix"
#define JT_PRISMATIC_NAME "Pris"
#define JT_REVOLUTE_NAME "Rev"
#define JT_SPRING_NAME "Spr"
#define JT_CYLINDRICAL_NAME "Cyl"
#define JT_SPHERICAL_NAME "Sphere"
#define JT_LOOSE_NAME "Loose"
#define JT_TRANSLATIONAL_NAME "Trans"
#define PART_STEM "Part"
#define ROOT_OBJ_NAME "Root"
#define SPOTS "Spots"

const char OBJECT_NAME[] = "Object name";
const char FILE_NAME[] = "File name";

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

struct PersistSphere
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
