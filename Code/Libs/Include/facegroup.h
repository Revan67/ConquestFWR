//
//
//

#ifndef FACEGROUP_H
#define FACEGROUP_H

//

#include "stddat.h"
#include "FaceProp.h"

struct FaceGroup
{
	int				material;
	int				face_cnt;
	int *			face_vertex_chain;
	int *			face_normal;
	float *			face_D_coefficient;
	FACE_PROPERTY *	face_properties;
	float *			face_area;

	FaceGroup(void)
	{
		material = -1;
		face_cnt = 0;
		face_vertex_chain = NULL;
		face_normal = NULL;
		face_D_coefficient = NULL;
		face_properties = NULL;
		face_area = NULL;
	}

	FaceGroup & operator = (const FaceGroup & grp)
	{
		material = grp.material;
		face_cnt = grp.face_cnt;
		face_vertex_chain = new int[face_cnt * 3];
		memcpy(face_vertex_chain, grp.face_vertex_chain, sizeof(int) * face_cnt * 3);
		face_normal = new int[face_cnt];
		memcpy(face_normal, grp.face_normal, sizeof(int) * face_cnt);
		face_D_coefficient = new float[face_cnt];
		memcpy(face_D_coefficient, grp.face_D_coefficient, sizeof(float) * face_cnt);
		face_properties = new FACE_PROPERTY[face_cnt];
		memcpy(face_properties, grp.face_properties, sizeof(FACE_PROPERTY) * face_cnt);
		face_area = new float[face_cnt];
		memcpy(face_area, grp.face_area, sizeof(float) * face_cnt);

		return *this;
	}

	~FaceGroup(void)
	{
		material = -1;
		face_cnt = 0;

		delete [] face_vertex_chain;
		face_vertex_chain = NULL;

		delete [] face_normal;
		face_normal = NULL;

		delete [] face_D_coefficient;
		face_D_coefficient = NULL;

		delete [] face_properties;
		face_properties = NULL;

		delete [] face_area;
		face_area = NULL;
	}
};

//

#endif