#ifndef MINSPHERE_H
#define MINSPHERE_H

typedef struct
{
    float x, y, z;
}
Point3;

typedef struct
{
    float x, y, z, r;
}
Sphere;

Sphere MinimalSphere (int n, Point3* p);

#endif
