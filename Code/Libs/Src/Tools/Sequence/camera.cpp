#include "stdafx.h"
#include "camera.h"
#include "object.h"
#include <stdio.h>
#include "sequence.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern	SequenceApp theApp;

#define DEL(ptr)		\
	if (ptr != NULL) {	\
		delete ptr;		\
	}

#define DELA(ptr)		\
	if (ptr != NULL) {	\
		delete [] ptr;	\
	}

#define DELC(ptr)		\
	if (ptr != NULL) {	\
		delete ptr;		\
		ptr = NULL;		\
	}

#define DELCA(ptr)		\
	if (ptr != NULL) {	\
		delete [] ptr;	\
		ptr = NULL;		\
	}


GameCamera::GameCamera(IEngine *engine, PANE *_pane) : BaseCamera(engine, (ViewRect *)_pane) 
{
	render_obj = NULL;
	obj_attached = NULL;
}

GameCamera::~GameCamera() 
{
	DELC(render_obj);
}

static int turning_step = 0;
void GameCamera::Update(float secs) {
	if (obj_attached != NULL) {
		// Get the position of the object we are following
		Vector obj_pos = obj_attached->get_position();
		Matrix obj_orient = obj_attached->get_orientation();
		Vector reverse_move_orient = obj_last_position - obj_pos;
		if (reverse_move_orient.magnitude() < 0.5f) {
			reverse_move_orient = transform.get_orientation().get_k();
		}

		// Level the orientation off
		Vector bvec = transform.get_position() - obj_pos;
		bvec.y = 0.0f;
		bvec.normalize();

		Vector uvec(0.0f, 1.0f, 0.0f);
		obj_orient.set_j(uvec);
		obj_orient.set_k(bvec);
		Vector rvec = cross_product(uvec, bvec);
		obj_orient.set_i(rvec);

		// Get the desired position of the camera
		// For now we use 3 meters back and one meter above the object we are following
		Vector desired_pos = obj_pos + (obj_orient.get_k() * 3 + obj_orient.get_j());
		Quaternion desired_orient(obj_orient);

		/*if (dot_product(obj_last_orientation.get_k(), obj_orient.get_k()) < 0.995f) {
			// Turning..overturn to look ahead
			Quaternion rotate(Vector(0.0f, 1.0f, 0.0f), 0.4f);
			desired_orient = desired_orient * rotate;
			char buffer[80];
			sprintf(buffer, "Camera overturning %d\n", turning_step);
			turning_step++;
			OutputDebugString(buffer);
		}*/

		// Get the current position
		Vector current_pos = transform.get_position();
		Matrix current_orient = transform.get_orientation();
		desired_orient = slerp(Quaternion(current_orient), desired_orient, 1.0f); // * secs);

		Vector new_pos = current_pos + (desired_pos - current_pos) * 0.25f;

		Vector diff = new_pos - desired_pos;

		while (diff.magnitude() > 5.0f) {
			new_pos = new_pos + (desired_pos - new_pos) * 0.25f;
			diff = new_pos - desired_pos;
		}
		new_pos.y = new_pos.y + (desired_pos.y - new_pos.y) * 0.01f;

		if (diff.magnitude() < 0.005f) {
			new_pos = desired_pos;
		}

		// Assign the camera it's new position & orientation
		theApp.m_ENG->set_position(index, new_pos);
		theApp.m_ENG->set_orientation(index, Matrix(desired_orient));

		obj_last_position = obj_attached->get_position();
		//obj_last_orientation = obj_attached->get_orientation();
	}
}

void GameCamera::Render(BaseCamera* camera) {
	if ((camera != this) && (render_obj != NULL)) {
		render_obj->set_position(get_position());
		render_obj->set_orientation(get_orientation());
		render_obj->Render(camera, RF_FILL);
	}
}

//

void GameCamera::build_view_planes(void)
{
// start with near plane, then far, left, right, bottom, top.
	Vector p, N;
	
	p.set(0, 0, -znear);
	N.set(0, 0, -1);
	planes[4].init(p, N);

// far.
	p.set(0, 0, -zfar);
	N.set(0, 0, 1);
	planes[5].init(p, N);

// left.
	p.x = -half_near_plane_w;
	p.y = 0;
	p.z = -znear;

	float theta = get_fovx() * MUL_DEG_TO_RAD;
	N.x = cos(theta);
	N.y = 0;
	N.z = -sin(theta);

	planes[0].init(p, N);

// right.
	p.x = -p.x;
	N.x = -N.x;
	planes[1].init(p, N);

// bottom.
	p.x = 0;
	p.y = -half_near_plane_h;
	p.z = -znear;

	theta = get_fovy() * MUL_DEG_TO_RAD;
	N.x = 0;
	N.y = cos(theta);
	N.z = -sin(theta);
	planes[2].init(p, N);

// top.
	p.y = -p.y;
	N.y = -N.y;
	planes[3].init(p, N);
}

//

bool GameCamera::box_intersects_frustum(const Vector v[8]) const
{
	bool result = true;

	struct vertex
	{
		Vector p;
		float d;
		bool active:1;
		bool inside:1;
	};

	vertex verts[64];
	int num_verts = 8;

	for (int i = 0; i < num_verts; i++)
	{
		verts[i].p = get_transform().inverse_rotate_translate(v[i]);
		verts[i].active = true;
	}

	struct edge
	{
		int v0, v1;
		bool active;

		edge(void)
		{
			active = false;
		}
	};

	edge edges[64];
	edges[0].v0 = 0;
	edges[0].v1 = 1;
	edges[1].v0 = 1;
	edges[1].v1 = 2;
	edges[2].v0 = 2;
	edges[2].v1 = 3;
	edges[3].v0 = 3;
	edges[3].v1 = 0;

	edges[4].v0 = 4;
	edges[4].v1 = 5;
	edges[5].v0 = 5;
	edges[5].v1 = 6;
	edges[6].v0 = 6;
	edges[6].v1 = 7;
	edges[7].v0 = 7;
	edges[7].v1 = 4;

	edges[ 8].v0 = 0;
	edges[ 8].v1 = 4;
	edges[ 9].v0 = 1;
	edges[ 9].v1 = 5;
	edges[10].v0 = 2;
	edges[10].v1 = 6;
	edges[11].v0 = 3;
	edges[11].v1 = 7;

	int num_edges = 12;
	for (i = 0; i < num_edges; i++)
	{
		edges[i].active = true;
	}

	const ViewPlane * p = planes;
	for (i = 0; i < 6; i++, p++)
	{
		int inside = 0;
		bool all_inside = true;
		vertex * vtx = verts;
		for (int j = 0; j < num_verts; j++, vtx++)
		{
			if (vtx->active)
			{
				float d = dot_product(p->N, vtx->p) + p->D;

			// DO WE WANT A TOLERANCE HERE?
				if (d >= 0)
				{
					vtx->d = d;
					vtx->inside = true;
					inside++;
				}
				else
				{
					vtx->inside = false;
					all_inside = false;
				}
			}
		}

		if (all_inside)
		{
			continue;
		}

		if (inside)
		{
		// check edges.

			int new_edge_idx = num_edges;

			edge * e = edges;
			for (int j = 0; j < num_edges; j++, e++)
			{
				if (e->active)
				{
					vertex * v0 = verts + e->v0;
					vertex * v1 = verts + e->v1;

					if (v0->inside)
					{
						if (v1->inside)
						{
						// both in, do nothing.
						}
						else
						{
						// v0 in, v1 out. Create new edge.
							Vector lb = v0->p;
							Vector ld = v1->p - v0->p;

							float denom = dot_product(ld, p->N);
							float t = -v0->d / denom;
							assert(t > -1e-5);

							verts[num_verts].p = lb + t * ld;
							verts[num_verts].active = true;

							edges[new_edge_idx].v0 = e->v0;
							edges[new_edge_idx].v1 = num_verts++;
							edges[new_edge_idx].active = true;
							new_edge_idx++;

							v1->active = false;
							e->active = false;
						}
					}
					else if (v1->inside)
					{
					// v0 out, v1 in.
						Vector lb = v1->p;
						Vector ld = v0->p - v1->p;

						float denom = dot_product(ld, p->N);
						float t = -v1->d / denom;
						assert(t > -1e-5);

						verts[num_verts].p = lb + t * ld;
						verts[num_verts].active = true;

						edges[new_edge_idx].v0 = num_verts++;
						edges[new_edge_idx].v1 = e->v1;
						edges[new_edge_idx].active = true;
						new_edge_idx++;

						v0->active = false;
						e->active = false;
					}
					else
					{
					// both out, forget about this edge.
						e->active = false;
					}
				}
			}

			num_edges = new_edge_idx;

			assert(num_verts < 64);
			assert(num_edges < 64);
		}
		else
		{
		// we're done.
			result = false;
			break;
		}
	}

	return result;
}

//
bool GameCamera::edge_intersects_frustum(Vector& v1, Vector& v2) const {
	bool result = true;

	struct vertex
	{
		Vector p;
		float d;
		bool inside:1;
	};

	vertex verts[2];

	verts[0].p = v1;
	verts[1].p = v2;

	const ViewPlane * p = planes;
	for (int i = 0; i < 4; i++, p++) {
		int inside = 0;
		vertex * vtx = verts;

		float d = dot_product(p->N, vtx->p) + p->D;

		// DO WE WANT A TOLERANCE HERE?
		if (d >= 0) {
			vtx->d = d;
			vtx->inside = true;
			inside++;
		} else {
			vtx->inside = false;
		}

		vtx++;
		d = dot_product(p->N, vtx->p) + p->D;
		if (d >= 0) {
			vtx->d = d;
			vtx->inside = true;
			inside++;
		} else {
			vtx->inside = false;
		}

		if (inside == 2) {
			continue;
		}

		if (inside) {
			if (verts[0].inside) {
				// v0 in, v1 out. Create new edge.
				Vector lb = verts[0].p;
				Vector ld = verts[1].p - verts[0].p;

				float denom = dot_product(ld, p->N);
				float t = -verts[0].d / denom;
				assert(t > -1e-5);

				verts[0].p = lb + t * ld;
			} else {
				// v0 out, v1 in.
				Vector lb = verts[1].p;
				Vector ld = verts[0].p - verts[1].p;

				float denom = dot_product(ld, p->N);
				float t = -verts[1].d / denom;
				assert(t > -1e-5);

				verts[1].p = lb + t * ld;
			}
		} else {
			// we're done.
			result = false;
			break;
		}
	}

	return result;
}
