//
//
//

#include "stddat.h"
#include "pcontact.h"
#include "fdump.h"

//
// BOX HACK!!!
//

inline int NumContainingFeatures(const Feature & f)
{
	int result;
	switch (f.type)
	{
		case Feature::VERT:
			result = 6;
	    	break;
		case Feature::EDGE:
			result = 2;
			break;
		case Feature::FACE:
			result = 0;
			break;
		default:
			result = 0;
			break;
	}
	return result;
}

//

void GetContainingFeatures(Feature ** dst, const Feature & f)
{
	switch (f.type)
	{
		case Feature::VERT:
		{
			VVertex * v = (VVertex *) &f;
			VEdge * e0 = (VEdge *) v->planes[0].neighbor;
			VEdge * e1 = (VEdge *) v->planes[1].neighbor;
			VEdge * e2 = (VEdge *) v->planes[2].neighbor;

			dst[0] = e0;
			dst[1] = e1;
			dst[2] = e2;

			VFace * f0 = (VFace *) e0->planes[2].neighbor;
			VFace * f1 = (VFace *) e0->planes[3].neighbor;
			dst[3] = f0;
			dst[4] = f1;
			if ((e1->planes[2].neighbor == f0) || (e1->planes[2].neighbor == f1))
			{
				dst[5] = e1->planes[3].neighbor;
			}
			else
			{
				dst[5] = e1->planes[2].neighbor;
			}
			break;
		}
		case Feature::EDGE:
		{
			VEdge * e = (VEdge *) &f;
			dst[0] = e->planes[2].neighbor;
			dst[1] = e->planes[3].neighbor;
			break;
		}
		case Feature::FACE:
			break;
	}
}

//

bool OnBoundaryOf(const Feature & f1, const Feature & f2)
{
	bool result;

	VEdge * e;
	VFace * f;

	switch (f2.type)
	{
		case Feature::VERT:
			result = false;	// vertex has no "boundary."
			break;

		case Feature::EDGE:
			switch (f1.type)
			{
				case Feature::VERT:
					e = (VEdge *) &f2;
					result = (e->v[0] == f1.id) || (e->v[1] == f1.id);
					break;
				default:
					result = false;
					break;
			}
			break;

		case Feature::FACE:
			switch (f1.type)
			{
				case Feature::VERT:
					f = (VFace *) &f2;
					result = (f->v[0] == f1.id) || (f->v[1] == f1.id) || (f->v[2] == f1.id) || (f->v[3] == f1.id);
					break;
				case Feature::EDGE:
					f = (VFace *) &f2;
					result = (f->e[0] == f1.id) || (f->e[1] == f1.id) || (f->e[2] == f1.id) || (f->e[3] == f1.id);
					break;
				default:
					result = false;
					break;
			}
			break;
	}
	return result;
}

//

#define MAX_CONTAINING_FEATURES	6

//

bool EdgeEdgeTouch(const VEdge * e0, const VEdge * e1, const Vector & r2, const Matrix & R2)
{
	ASSERT(e0);
	ASSERT(e1);

	Vector e0_v0 = e0->get_v0();
	Vector e0_v1 = e0->get_v1();

	Vector e1_v0 = r2 + R2 * e1->get_v0();
	Vector e1_v1 = r2 + R2 * e1->get_v1();

	Vector edge0 = e0_v1 - e0_v0;
	Vector edge1 = e1_v1 - e1_v0;
	edge0.normalize();
	edge1.normalize();

	float d = dot_product(edge0, edge1);
	bool result = (fabs(d) < 1e-5);
	return result;
}

//

bool PContact::contains(const PContact & c)
{
	bool result;

	bool b1 = OnBoundaryOf(*c.f1, *f1);
	bool b2 = OnBoundaryOf(*c.f2, *f2);

	if (b1 && b2)
	{
		result = true;
	}
	else if (b1 && (*c.f2 == *f2))
	{
		result = true;
	}
	else if ((*c.f1 == *f1) && b2)
	{
		result = true;
	}
	else
	{
		result = false;
	}
	return result;
}

//

void BuildCHG()
{
}

//

void FindPrincipalContact(PContact & ec0)
{
//
// Classify ec0.
//
	bool EET = false;
    if ((ec0.f1->type == Feature::EDGE) && (ec0.f2->type == Feature::EDGE))
	{
		VEdge * e0 = (VEdge *) ec0.f1;
		VEdge * e1 = (VEdge *) ec0.f2;

		EET = EdgeEdgeTouch(e0, e1, ec0.r2, ec0.R2);
	}
//
// Compute HECs from ec0.
//
	int num_Ta = NumContainingFeatures(*ec0.f1);
	int num_Tb = NumContainingFeatures(*ec0.f2);
	Feature * Ta[MAX_CONTAINING_FEATURES+1];
	Feature * Tb[MAX_CONTAINING_FEATURES+1];

	Ta[0] = ec0.f1;
	Tb[0] = ec0.f2;
	GetContainingFeatures(Ta+1, *ec0.f1);
	GetContainingFeatures(Tb+1, *ec0.f2);

	int num_H = 0;
	DynamicArray<PContact> H;

	for (int i = 0; i < num_Ta; i++)
	{
		Feature * ua = Ta[i];
		for (int j = 0; j < num_Tb; j++)
		{
			Feature * ub = Tb[j];
			if ((ua->type == Feature::EDGE) && (ub->type == Feature::EDGE))
			{
			// ¿¿¿ check cross v. touch ???
				H[num_H].f1 = ua;
				H[num_H].r1 = ec0.r1;
				H[num_H].R1 = ec0.R1;

				H[num_H].f2 = ub;
				H[num_H].r2 = ec0.r2;
				H[num_H].R2 = ec0.R2;

				num_H++;
			}
			else if (
					 (
					  ((ua->type == Feature::FACE) && (ub->type == Feature::EDGE)) ||
					  ((ua->type == Feature::EDGE) && (ub->type == Feature::FACE))
					 ) && EET
					)
			{
				continue;
			}
			else
			{
				H[num_H].f1 = ua;
				H[num_H].r1 = ec0.r1;
				H[num_H].R1 = ec0.R1;

				H[num_H].f2 = ub;
				H[num_H].r2 = ec0.r2;
				H[num_H].R2 = ec0.R2;

				num_H++;
			}
		}
	}

//
// Build contact hierarchy graph.
//
	for (int e = 0; e < num_H; e++)
	{
		PContact * ec = &H[e];

		int num_containing = 0;
		DynamicArray<PContact *> containing;

		for (i = 0; i < num_H; i++)
		{
			PContact * hec = &H[i];
			if (hec->contains(*ec))
			{
				containing[num_containing++] = hec;
			}
		}

		for (i = 0; i < num_containing; i++)
		{
			PContact * eci = containing[i];

			bool direct = true;

			for (int j = 0; j < num_containing; j++)
			{
				if (i == j) continue;
				PContact * ecj = containing[i];

				if (eci->contains(*ecj))
				{
				// violates directly-containing critera.
					direct = false;
					break;
				}
			}

			if (direct)
			{
			// eci directly contains ec, add node to graph.
				ec->add_child(eci);
			}
		}
	}

//
// Search CHG for PC.
//
	PContact * ecd = &ec0;
	while (ecd)
	{
		PContact * hec = ecd->child;
		while (hec)
		{
//			if (hec is valid EC)
			{
				ecd = hec;
				break;
			}
			hec = hec->sibling;
		}
	}
}

//

int GetHECs(PContact * dst, const PContact & EC)
{
	int result = 0;

	switch (EC.type)
	{
	case PContact::FACE_FACE:
		break;
	case PContact::EDGE_FACE:
		{
			ASSERT(dst);

			result = 2;
			VEdge * e;
			VFace * f;
			if (EC.f1->type == Feature::EDGE)
			{
				e = (VEdge *) EC.f1;
				f = (VFace *) EC.f2;
			}
			else
			{
				f = (VFace *) EC.f1;
				e = (VEdge *) EC.f2;
			}

			dst[0].type = PContact::FACE_FACE;
			dst[0].v1 = EC.v1;
			dst[0].f1 = EC.v1->faces + e->t[0];
			dst[0].r1 = EC.r1;
			dst[0].R1 = EC.R1;
			dst[0].v2 = EC.v2;
			dst[0].f2 = f;
			dst[0].r2 = EC.r2;
			dst[0].R2 = EC.R2;


			dst[1].type = PContact::FACE_FACE;
			dst[1].v1 = EC.v1;
			dst[1].f1 = EC.v1->faces + e->t[1];
			dst[1].r1 = EC.r1;
			dst[1].R1 = EC.R1;
			dst[1].v2 = EC.v2;
			dst[1].f2 = f;
			dst[1].r2 = EC.r2;
			dst[1].R2 = EC.R2;
		}
		break;
	case PContact::VERTEX_FACE:
		{
		}
	case PContact::EDGE_CROSS:
		{
		}
		break;
	case PContact::EDGE_TOUCH:
		{
		}
		break;
	case PContact::VERTEX_EDGE:
		{
		}
		break;
	case PContact::VERTEX_VERTEX:
		{
		}
		break;
	}

	return result;
}

//