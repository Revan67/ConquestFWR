#ifndef MINBOX3_H
#define MINBOX3_H

typedef struct
{
    double x, y, z;
}
ExtPoint3;

typedef struct
{
    ExtPoint3 center;
    ExtPoint3 axis[3];
    double extent[3];
	double volume;
}
OBBox3;

OBBox3 MinimalBox3 (int N, ExtPoint3* pt);

#endif
