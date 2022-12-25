#ifndef ISPLINEPATH_H
#define ISPLINEPATH_H

#ifndef DACOM_H
#include <DACOM.h>
#endif

class Vector;

typedef enum PathType
{
	K_CR_SPLINE,
	K_HERMITE_SPLINE
};

// ISplinePath functions

#define ISpline_VERSION 1
#define IID_ISpline MAKE_IID("ISpline", 1)
struct ISpline: IDAComponent
{
public:
	virtual GENRESULT COMAPI get_position(SINGLE t, Vector &pos) = 0;
	virtual GENRESULT COMAPI get_position_untransformed(SINGLE t, Vector &pos) = 0;
	virtual GENRESULT COMAPI get_tangent(SINGLE t, Vector &tangent) = 0;
	virtual GENRESULT COMAPI get_tangent_untransformed(SINGLE t, Vector &tangent) = 0;
	virtual SINGLE COMAPI get_length() = 0;
	virtual GENRESULT COMAPI set_control_points(Vector pointList[], U32 count) = 0;
	virtual U32 COMAPI get_num_control_points() = 0;
	virtual GENRESULT COMAPI set_control_point(U32 index, Vector &point) = 0;
	virtual GENRESULT COMAPI get_control_point(U32 index, Vector &point) = 0;
	virtual GENRESULT COMAPI calculate_position(U32 type, Vector *p0, Vector *p1, Vector *p2, Vector *p3, SINGLE t, Vector *result) = 0;
	virtual GENRESULT COMAPI set_transform(Transform &xform) = 0;
	virtual GENRESULT COMAPI get_transform(Transform &xform) = 0;
	virtual GENRESULT COMAPI get_inverse_transform(Transform &xform) = 0;
};

#endif //ISPLINEPATH_H
