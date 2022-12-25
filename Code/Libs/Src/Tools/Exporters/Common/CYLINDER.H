#ifndef CYLINDER_H
#define CYLINDER_H

#include <stdio.h>
#include "persistmath.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Circle
{
public:
  float x,y;
  float r;

  Circle()
  {
    x=y=0.0f;
    r=0.0f;
  }
};

struct Cylinder
{
public:
  char cyl_name[64];
  char tub_name[64];
  float cyl_r;
  float tub_r;	// can't reuse one radius because byte swapping would happen twice on same data
  Vector p1, p2;
  Vector tub_p1, tub_p2;
  float cyl_length;
  float tub_length;
  float cyl_volume;
  float tub_volume;
  PersistTransform cyl_tr;
  PersistTransform tub_tr;

  void Init(void)
  {
	cyl_name[0] = 0;
	tub_name[0] = 0;
    p1.x=p1.y=p1.z=0.0f;
    p2.x=p2.y=p2.z=0.0f;
	tub_p1.x=tub_p1.y=tub_p1.z=0.0f;
    tub_p2.x=tub_p2.y=tub_p2.z=0.0f;
    cyl_r=tub_r=0.0f;
	cyl_length=0.0f;
	tub_length=0.0f;
    cyl_volume=0.0f;
	tub_volume=0.0f;
	cyl_tr.identity();
	tub_tr.identity();
  }

  Cylinder()
  {
    Init();
  }
};

void ComputeCylinder(Cylinder *cylinder, const Vector *v, int count);
void ComputeCircle(Circle *circle, const float *xy, int count);
void ComputeTube(Cylinder& cyl, const Vector *v, int count);

#endif
