#ifndef VBOX_H
#define VBOX_H

//

#include "VMesh.h"

//

struct VBox : public VMesh
{
	int *			tri_face_map;
	float			volume;

	void compute_vregions(void);
	void compute_faces(CollisionMesh * mesh);

	void adjust(float x, float y, float z);

	VBox(float x, float y, float z);
	~VBox(void);
};

//

#endif