//
// Collision response.
//

#pragma warning( disable : 4786 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "rigid.h"
#include "friction.h"
#include "algebra.h"
#include "rk.h"
#include "Debugprint.h"

//

char work[256];

extern Vector HackPoint1, HackPoint2, HackNormal, HackDV, HackV1, HackV2;
extern bool HackPointsValid, freeze;

//

bool FindClosestPoints(const CollisionMesh & p, const CollisionMesh & q, const Matrix & Rp, const Vector & rp, const Matrix & Rq, const Vector & rq, Vector & v1, Vector & v2, Vector & normal);

//

struct CollisionRecord
{
	const Instance *obj1;
	Vector			p1;		// Absolute contact point in world space.
	Vector			r1;		// Contact point relative to object origin, expressed in world space.

	const Instance *obj2;
	Vector			p2;
	Vector			r2;

	float			vrel;
	Vector			normal;
};

//

#define MAX_COLLISIONS	128

static int NumCollisions = 0;
static CollisionRecord Collisions[MAX_COLLISIONS];

//

static char WorkString[256];

//

inline float square(float n)
{
	return n*n;
}

//

int _matherr( struct _exception *except )
{
	PHYTRACE10("math error.\n");
	return 0;
}

//

extern bool FixJoints;

void svdcmp(float a[3][3], float w[3], float v[3][3]);

//

void COMAPI RigidBodyPhysics::compute_collision_response(const CollisionData & data, INSTANCE_INDEX i1, INSTANCE_INDEX i2)
{
	Instance * obj1 = (i1 == INVALID_INSTANCE_INDEX) ? NULL : GetInstance(i1);
	Instance * obj2 = (i2 == INVALID_INSTANCE_INDEX) ? NULL : GetInstance(i2);

// Check vrel.
	Vector rel1, rel2;
	Vector v1, v2;
	if (obj1)
	{
		rel1 = data.contact - obj1->x;
		v1 = obj1->v + cross_product(obj1->w, rel1);
	}
	else
	{
		rel1.zero();
		v1.zero();
	}

	if (obj2)
	{
		rel2 = data.contact - obj2->x;
		v2 = obj2->v + cross_product(obj2->w, rel2);
	}
	else
	{
		rel2.zero();
		v2.zero();
	}

	Vector dv = v1 - v2;
	float vrel = dot_product(dv, data.normal);

	const float tolerance = 1e-5;

	if (vrel < 0) // -tolerance
	{
		if ((collision_func == NULL) || collision_func(i1, i2, &data))
		{
			SINGLE d1, d2;
			Matrix Iinv1, Iinv2;
			if (obj1 && obj1->dynamic == DS_DYNAMIC)
			{
				d1 = 1.0f / obj1->mass;
				Iinv1 = obj1->Iinv;
			}
			else
			{
				d1 = 0;
				Iinv1.zero();
			}
			SINGLE d3 = dot_product(data.normal, cross_product(Iinv1 * cross_product(rel1, data.normal), rel1));

			if (obj2 && obj2->dynamic == DS_DYNAMIC)
			{
				d2 = 1.0f / obj2->mass;
				Iinv2 = obj2->Iinv;
			}
			else
			{
				d2 = 0;
				Iinv2.zero();
			}

			SINGLE d4 = dot_product(data.normal, cross_product(Iinv2 * cross_product(rel2, data.normal), rel2));

			if (collision_friction)
			{
				FixJoints = false;

			//
			// Create collision frame:
			//
				Vector i(data.normal.y, -data.normal.x, 0.0f);
				if (fabs(i.x) < 1e-4 && fabs(i.y) < 1e-4)
				{
					i.x = 1.0f;
				}
				i.normalize();

				Vector k(data.normal);

				Vector j = cross_product(k, i);
				j.normalize();

				Vector rc = data.contact;

				Matrix Rc;
				Rc.set_i(i);
				Rc.set_j(j);
				Rc.set_k(k);

				Vector u0 = dv * Rc;

				Vector r1 = rel1 * Rc;
				Vector r2 = rel2 * Rc;

				Matrix J1 = Rc.get_transpose() * Iinv1 * Rc;
				Matrix J2 = Rc.get_transpose() * Iinv2 * Rc;

				INSTANCE_INDEX p1 = engine->get_instance_parent(i1);
				INSTANCE_INDEX c1 = engine->get_instance_child_next(i1,EN_DONT_RECURSE,INVALID_INSTANCE_INDEX);
				INSTANCE_INDEX p2 = engine->get_instance_parent(i2);
				INSTANCE_INDEX c2 = engine->get_instance_child_next(i2,EN_DONT_RECURSE, INVALID_INSTANCE_INDEX);
				bool tree1 = (p1 != INVALID_INSTANCE_INDEX) || (c1 != INVALID_INSTANCE_INDEX);
				bool tree2 = (p2 != INVALID_INSTANCE_INDEX) || (c2 != INVALID_INSTANCE_INDEX);

				Matrix I; I.set_identity();
				Matrix K;
				Instance * root1, * root2;

				if ((tree1 || tree2) && joint_dynamics)
				{
					Matrix K1, K2;
					if (tree1)
					{
						INSTANCE_INDEX next = (p1 == INVALID_INSTANCE_INDEX) ? i1 : p1;
						INSTANCE_INDEX root = i1;
						while (next != INVALID_INSTANCE_INDEX)
						{
							root = next;
							next = engine->get_instance_parent(next);
						}
						root1 = GetInstance(root);
						compute_ABM_collision_matrix(K1, root1, obj1, Rc, rc, 0);
					}
					else
					{
						root1 = NULL;
						K1 = d1 * I - dual(r1) * J1 * dual(r1);
					}

					if (tree2)
					{
						INSTANCE_INDEX next = (p2 == INVALID_INSTANCE_INDEX) ? i2 : p2;
						INSTANCE_INDEX root = i2;
						while (next != INVALID_INSTANCE_INDEX)
						{
							root = next;
							next = engine->get_instance_parent(next);
						}
						root2 = GetInstance(root);
						compute_ABM_collision_matrix(K2, root2, obj2, Rc, rc, 1);
					}
					else
					{
						root2 = NULL;
						K2 = d2 * I - dual(r2) * J2 * dual(r2);
						//K2.zero();
					}

					K = K1 + K2;
				}
				else
				{
					Matrix I; I.set_identity();
					K = (d1 + d2) * I - (dual(r1) * J1 * dual(r1) + dual(r2) * J2 * dual(r2));
				}


				Matrix Kinv;

				if (tree1 || tree2)
				{

				//
				// K can be rank-deficient. Use SVD to invert it.
				// TODO: TRY GAUSSIAN elimination first, switch to SVD if necessary.
				//
					float a[3][3];
					memcpy(a, K.d, sizeof(float) * 9);
					float w[3], v[3][3];
					svdcmp(a, w, v);

					Matrix U; memcpy(U.d, a, sizeof(float) * 9);
					Matrix W; W.set_identity(); 
					W.d[0][0] = (w[0] < 1e-5) ? 0 : 1.0f / w[0];
					W.d[1][1] = (w[1] < 1e-5) ? 0 : 1.0f / w[1];
					W.d[2][2] = (w[2] < 1e-5) ? 0 : 1.0f / w[2];

					Matrix V; memcpy(V.d, v, sizeof(float) * 9);

				// OPTIMIZE THIS. W is diagonal.
					Kinv = V * W * U.get_transpose();
				}
				else
				{
				// Use gauss-jordan elimination.
				 	Kinv = K.get_inverse();
				}

				Vector p;
				float er = data.coeff;

				float coeff = data.coeff;
				if (-u0.z < sqrt(2 * 30 * 0.1))
				{
					coeff *= 2.0f;
				}

				Vector n(0, 0, 1);						// by definition of the collision frame.
				Vector V_i = dv * Rc;					// relative velocity in collision frame.

				float denom = dot_product(n, K * n);
				Vector P_I(0, 0, -V_i.z / denom);

				Vector P_II = -(Kinv * V_i);

				Vector dP = P_II - P_I;

				const float e_t = 0;
				Vector P = (1 + coeff) * P_I + (1 + e_t) * dP;

				float nP = dot_product(n, P);
				Vector check = P - nP * n;
				float cmag = check.magnitude();
				if (cmag > data.mu * nP)
				{
					float lnum = data.mu * (1.0f + coeff) * dot_product(n, P_I);
					Vector pdenom = P_II - dot_product(n, P_II) * n;
					float ldenom = pdenom.magnitude() - data.mu * dot_product(n, dP);
					float lambda = lnum / ldenom;
					p = (1 + coeff) * P_I + lambda * dP;
				}
				else
				{
					p = P;
				}

				float pmag = p.magnitude();

				if ((tree1 || tree2) && joint_dynamics)
				{
					if (tree1)
					{
						propagate_impulse(root1, obj1, p, Rc, rc, 0);
					}
					else if (obj1)
					{
						add_impulse_at_point(i1, Rc * p, data.contact);
					}
					if (tree2)
					{
						propagate_impulse(root2, obj2, -p, Rc, rc, 1);
					}
					else if (obj2)
					{
						add_impulse_at_point(i2, -(Rc * p), data.contact);
					}
				}
				else
				{
					p = Rc * p;
					if (obj1)
					{
						add_impulse_at_point(i1, p, data.contact);
					}
					if (obj2)
					{
						add_impulse_at_point(i2, -p, data.contact);
					}
				}
			}
			else
			{
				SINGLE numerator = -(1.0f + data.coeff) * vrel;
				SINGLE denominator = d1 + d2 + d3 + d4;

				const float tolerance = 1e-5;
				if (fabs(denominator) > tolerance)
				{
					SINGLE imag = numerator / denominator;
					Vector impulse = data.normal * imag;

					if (obj1)
					{
						add_impulse_at_point(i1,  impulse, data.contact);
					}
					if (obj2)
					{
						add_impulse_at_point(i2, -impulse, data.contact);
					}
				}
			}
		}
	}
	else if (vrel < tolerance)
	{
	/*
		Contact c;
		c.a = obj1;
		c.b = obj2;
		c.p = data.contact;
		c.N = data.normal;
		compute_contact_forces(&c, 1);
	*/
	}
}

//

void COMAPI RigidBodyPhysics::compute_hierarchy_collision_response(const CollisionData & data, INSTANCE_INDEX i1, INSTANCE_INDEX i2)
{
	Instance * obj1 = (i1 == INVALID_INSTANCE_INDEX) ? NULL : GetInstance(i1);
	Instance * obj2 = (i2 == INVALID_INSTANCE_INDEX) ? NULL : GetInstance(i2);

	INSTANCE_INDEX p1 = engine->get_instance_parent(i1);
	INSTANCE_INDEX c1 = engine->get_instance_child_next(i1,EN_DONT_RECURSE,INVALID_INSTANCE_INDEX);
	INSTANCE_INDEX p2 = engine->get_instance_parent(i2);
	INSTANCE_INDEX c2 = engine->get_instance_child_next(i2,EN_DONT_RECURSE,INVALID_INSTANCE_INDEX);

	bool tree1 = obj1 && ((p1 != INVALID_INSTANCE_INDEX) || (c1 != INVALID_INSTANCE_INDEX));
	bool tree2 = obj2 && ((p2 != INVALID_INSTANCE_INDEX) || (c2 != INVALID_INSTANCE_INDEX));
	
	if (tree1 || tree2)
	{
	// 
	// at least one hierarchical object.
	//

		FixJoints = true;

	// Check vrel.
		Vector rel1, rel2;
		Vector v1, v2;
		if (obj1)
		{
			rel1 = data.contact - obj1->x;
			v1 = obj1->v + cross_product(obj1->w, rel1);
		}
		else
		{
			rel1.zero();
			v1.zero();
		}

		if (obj2)
		{
			rel2 = data.contact - obj2->x;
			v2 = obj2->v + cross_product(obj2->w, rel2);
		}
		else
		{
			rel2.zero();
			v2.zero();
		}

		Vector dv = v1 - v2;
		float vrel = dot_product(dv, data.normal);

		if (vrel < 0.0f)
		{
			if ((collision_func == NULL) || collision_func(i1, i2, &data))
			{
				SINGLE d1, d2;
				Matrix Iinv1, Iinv2;
				if (obj1 && obj1->dynamic == DS_DYNAMIC)
				{
					d1 = 1.0f / obj1->mass;
					Iinv1 = obj1->Iinv;
				}
				else
				{
					d1 = 0.0f;
					Iinv1.zero();
				}
				SINGLE d3 = dot_product(data.normal, cross_product(Iinv1 * cross_product(rel1, data.normal), rel1));

				if (obj2 && obj2->dynamic == DS_DYNAMIC)
				{
					d2 = 1.0f / obj2->mass;
					Iinv2 = obj2->Iinv;
				}
				else
				{
					d2 = 0.0f;
					Iinv2.zero();
				}

				SINGLE d4 = dot_product(data.normal, cross_product(Iinv2 * cross_product(rel2, data.normal), rel2));

				if (collision_friction)
				{
				//
				// Create collision frame:
				//
					Vector i(data.normal.y, -data.normal.x, 0.0f);
					if (fabs(i.x) < 1e-4 && fabs(i.y) < 1e-4)
					{
						i.x = 1.0f;
					}
					i.normalize();

					Vector k(data.normal);

					Vector j = cross_product(k, i);
					j.normalize();

					Vector rc = data.contact;

					Matrix Rc;
					Rc.set_i(i);
					Rc.set_j(j);
					Rc.set_k(k);

					Vector u0 = dv * Rc;

					Vector r1 = rel1 * Rc;
					Vector r2 = rel2 * Rc;

					Matrix J1 = Rc.get_transpose() * Iinv1 * Rc;
					Matrix J2 = Rc.get_transpose() * Iinv2 * Rc;

					Matrix I; I.set_identity();
					Matrix K;
					Instance * root1, * root2;

					Matrix K1, K2;

					bool k1_computed = false, k2_computed = false;

					if (tree1)
					{
						INSTANCE_INDEX next = (p1 == INVALID_INSTANCE_INDEX) ? i1 : p1;
						INSTANCE_INDEX root = i1;
						while (next != INVALID_INSTANCE_INDEX)
						{
							root = next;
							next = engine->get_instance_parent(next);
						}
						root1 = GetInstance(root);
						k1_computed = compute_ABM_collision_matrix(K1, root1, obj1, Rc, rc, 0);
					}

					if (!k1_computed)
					{
						root1 = NULL;
						K1 = d1 * I - dual(r1) * J1 * dual(r1);
						tree1 = false;
					}

					if (tree2)
					{
						INSTANCE_INDEX next = (p2 == INVALID_INSTANCE_INDEX) ? i2 : p2;
						INSTANCE_INDEX root = i2;
						while (next != INVALID_INSTANCE_INDEX)
						{
							root = next;
							next = engine->get_instance_parent(next);
						}
						root2 = GetInstance(root);
						k2_computed = compute_ABM_collision_matrix(K2, root2, obj2, Rc, rc, 1);
					}

					if (!k2_computed)
					{
						root2 = NULL;
						K2 = d2 * I - dual(r2) * J2 * dual(r2);
						tree2 = false;
					}

					K = K1 + K2;

					Matrix Kinv;
				//
				// K can be rank-deficient. Use SVD to invert it.
				// TODO: TRY GAUSSIAN elimination first, switch to SVD if necessary.
				//
					float a[3][3];
					memcpy(a, K.d, sizeof(float) * 9);
					float w[3], v[3][3];
					svdcmp(a, w, v);

					Matrix U; memcpy(U.d, a, sizeof(float) * 9);
					Matrix W; W.set_identity(); 
					W.d[0][0] = (w[0] < 1e-5) ? 0 : 1.0f / w[0];
					W.d[1][1] = (w[1] < 1e-5) ? 0 : 1.0f / w[1];
					W.d[2][2] = (w[2] < 1e-5) ? 0 : 1.0f / w[2];

					Matrix V; memcpy(V.d, v, sizeof(float) * 9);

				// OPTIMIZE THIS. W is diagonal.
					Kinv = V * W * U.get_transpose();

					Vector p;
					float er = data.coeff;

					float coeff = data.coeff;
					if (-u0.z < sqrt(2 * 30 * 0.1))
					{
						coeff *= 2.0f;
					}

					Vector n(0.0f, 0.0f, 1.0f);		// by definition of the collision frame.
					Vector V_i = dv * Rc;			// relative velocity in collision frame.

					float denom = dot_product(n, K * n);
					Vector P_I(0.0f, 0.0f, -V_i.z / denom);

					Vector P_II = -(Kinv * V_i);

					Vector dP = P_II - P_I;

					const float e_t = 0;
					Vector P = (1 + coeff) * P_I + (1 + e_t) * dP;

					float nP = dot_product(n, P);
					Vector check = P - nP * n;
					float cmag = check.magnitude();
					if (cmag > data.mu * nP)
					{
						float lnum = data.mu * (1 + coeff) * dot_product(n, P_I);
						Vector pdenom = P_II - dot_product(n, P_II) * n;
						float ldenom = pdenom.magnitude() - data.mu * dot_product(n, dP);
						float lambda = lnum / ldenom;
						p = (1.0f + coeff) * P_I + lambda * dP;
					}
					else
					{
						p = P;
					}

					float pmag = p.magnitude();

					if (tree1)
					{
						propagate_impulse(root1, obj1, p, Rc, rc, 0);
					}
					else if (INVALID_INSTANCE_INDEX != i1)
					{
						add_impulse_at_point(i1, Rc * p, data.contact);
					}

					if (tree2)
					{
						propagate_impulse(root2, obj2, -p, Rc, rc, 1);
					}
					else if (INVALID_INSTANCE_INDEX != i2)
					{
						add_impulse_at_point(i2, -(Rc * p), data.contact);
					}
				}
				else
				{
					SINGLE numerator = -(1.0f + data.coeff) * vrel;
					SINGLE denominator = d1 + d2 + d3 + d4;

					const float tolerance = 1e-5;
					if (fabs(denominator) > tolerance)
					{
						SINGLE imag = numerator / denominator;
						Vector impulse = data.normal * imag;

						if (obj1)
						{
							add_impulse_at_point(i1,  impulse, data.contact);
						}
						if (obj2)
						{
							add_impulse_at_point(i2, -impulse, data.contact);
						}
					}
				}
			}
		}
		
	}
	else
	{
	//
	// no hierarchical objects.
	//
		compute_collision_response(data, i1, i2);
	}

	FixJoints = false;
}

//
