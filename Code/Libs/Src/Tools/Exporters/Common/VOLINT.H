#ifndef VOLINT_H
#define VOLINT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct POLYHEDRON;

struct P_FACE {
  double norm[3];
  double w; // D coefficient
  int verts[3];
  POLYHEDRON *poly;

  P_FACE(void)
  {
	  memset(this, 0, sizeof(*this));
  }
	  
};

struct POLYHEDRON {
  int numVerts;
  int numFaces;
  double *verts;
  P_FACE *faces;

  union{
	double T0; // volume
	double volume;
  };
  

  double T1[3], T2[3], TP[3];

  double mass;
  double c_o_m[3]; // center of mass
  double i_t[3][3]; //inertia tensor at center of mass

  POLYHEDRON(void)
  {
	  memset(this, 0, sizeof(*this));
  }

  void Release(void)
  {
	  if(verts) { free(verts); }
	  if(faces) { free(faces); }
	  memset(this, 0, sizeof(*this));
  }

}; 



void compVolumeIntegrals(POLYHEDRON *p, double density);
void compFaceIntegrals(P_FACE *f);
void compProjectionIntegrals(P_FACE *f);

#endif