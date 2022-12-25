#ifndef GAMETYPES_H
#define GAMETYPES_H

#ifdef _PARSER

#include "typedefs.h"

typedef S32 INSTANCE_INDEX;

struct Vector
{
	SINGLE x,y,z;
};

struct Matrix {
	SINGLE element[3][3];
};

struct Transform		// data version of transform class
{
	SINGLE element[3][3];
	Vector position;
};
#else
#define __readonly
#endif

struct GameObjectData {
	char						instance_name[80];
	__readonly INSTANCE_INDEX	index;
	__readonly Vector			position;
	__readonly Matrix			orientation;
	__readonly float			radius;
	int				current_state;
};

struct ColorData {
	int red;
	int green;
	int blue;
};

struct LightData {
	ColorData color;
	float range;
	float half_angle;
};

struct CameraData
{
	float	fovx;
	float	fovy;
	float	znear;
	float	zfar;
};

enum FogType {EXP, EXP2};

struct FogData
{
	bool		enable;
	bool		range_fog;
	ColorData	color;
	FogType		mode;
	float		density;
};

enum ObjectType 
{
	OT_INVALID_OBJ_TYPE = -1,
	OT_BODY,
	OT_BOMB,
	OT_STATIC_GEOMETRY,
	OT_CHARACTER,
	OT_GAME_OBJECT,
	OT_LIGHT,
	OT_EXPLOSION,
	OT_GUN,
	OT_NUM_OBJ_TYPES
};

struct ArcheTypeData {
	char name[80];
	ObjectType object_type;
};

struct TerrainImportScale {
	float terrain_scale;
};

#endif //GAMETYPES_H
