#ifndef CAMERA_H
#define CAMERA_H

#include "Basecam.h"

//

struct ViewPlane
{
	Vector	N;
	float	D;

	ViewPlane(void) {}
	ViewPlane(const Vector & p, const Vector & n)
	{
		init(p, n);
	}

	void init(const Vector & p, const Vector & n)
	{
		N = n;
		D = -dot_product(N, p);
	}
};

//
typedef struct	PANETAG	PANE;

class GameCamera : public BaseCamera 
{
	protected:

		ViewPlane planes[6];

		Vector obj_last_position;
		Matrix obj_last_orientation;

	public:
		GameCamera(IEngine* engine, PANE *_pane);
		~GameCamera();

		void Update(float secs);
		void Render(BaseCamera* camera);

		class BaseObject* render_obj;			// The object to render
		class BaseObject* obj_attached;		// The object we are attached to

		void build_view_planes(void);

		bool box_intersects_frustum(const Vector v[8]) const;
		bool edge_intersects_frustum(Vector& v1, Vector& v2) const;
};

#endif //CAMERA_H
