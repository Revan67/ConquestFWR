#ifndef VCYL_H
#define VCYL_H

//

#include "VMesh.h"

//

struct VCyl : public VMesh
{
	float length, radius;

	VCyl(float len, float rad);

	void adjust(float len, float rad);
};

//

#endif