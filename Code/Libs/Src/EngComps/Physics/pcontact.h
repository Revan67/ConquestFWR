#ifndef PCONTACT_H
#define PCONTACT_H

//

#include "vbox.h"

//

bool EdgeEdgeTouch(const VEdge * e0, const VEdge * e1, const Vector & r2, const Matrix & R2);

//

struct PContact
{
	enum 
	{
		FACE_FACE,
		EDGE_FACE,
		VERTEX_FACE,
		EDGE_CROSS,
		EDGE_TOUCH,
		VERTEX_EDGE,
		VERTEX_VERTEX
	} type;

	VBox *			v1;
	Feature *		f1;
	Vector			r1;
	Matrix			R1;

	VBox *			v2;
	Feature	*		f2;
	Vector			r2;
	Matrix			R2;

	PContact *		parent;
	PContact *		sibling;
	PContact *		child;

	PContact(void)
	{
		init();
	}

	PContact(Feature * _f1, const Vector & _r1, const Matrix & _R1, Feature * _f2, const Vector & _r2, const Matrix & _R2)
	{
		init();
		f1 = _f1;
		r1 = _r1;
		R1 = _R1;
		f2 = _f2;
		r2 = _r2;
		R2 = _R2;

		switch ((f1->type << 2) + f2->type)
		{
		case (Feature::FACE << 2) + Feature::FACE:
			type = FACE_FACE;
			break;
		case (Feature::FACE << 2) + Feature::EDGE:
		case (Feature::EDGE << 2) + Feature::FACE:
			type = EDGE_FACE;
			break;
		case (Feature::VERT << 2) + Feature::FACE:
		case (Feature::FACE << 2) + Feature::VERT:
			type = VERTEX_FACE;
			break;
		case (Feature::EDGE << 2) + Feature::EDGE:
		{
		// check cross vs. touch.
			if (EdgeEdgeTouch((VEdge *) f1, (VEdge *) f2, r2, R2))
			{
				type = EDGE_TOUCH;
			}
			else
			{
				type = EDGE_CROSS;
			}
			break;
		}
		case (Feature::EDGE << 2) + Feature::VERT:
		case (Feature::VERT << 2) + Feature::EDGE:
			type = VERTEX_EDGE;
			break;
		case (Feature::VERT << 2) + Feature::VERT:
			type = VERTEX_VERTEX;
			break;
		}
	}

	void init(void)
	{
		parent = child = sibling = NULL;
	}

	bool contains(const PContact & c);

	void add_child(PContact * c)
	{
		if (!child)
		{
			child = c;
			c->parent = this;
		}
		else
		{
			PContact * prev = NULL;
			PContact * ch = child;
			while (ch)
			{
				prev = ch;
				ch = ch->sibling;
			}

			prev->sibling = c;
			c->parent = this;
		}
	}
};

//

void FindPrincipalContact(PContact & ec0);

//

#endif