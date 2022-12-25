#include "stdafx.h"
#include "attach.h"
#include "character.h"
#include "physobj.h"
#include "bomb.h"
#include "sequence.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern	SequenceApp theApp;

void Transfer::execute(Character * c) const
{
	Attachment * s = c->attachments + src;

	BaseObject * obj = (BaseObject *) theApp.m_ENG->get_user_data(s->child);

	theApp.m_MODEL->disconnect(s->parent, s->child);
	s->active = false;

	if (s->arch->select)
	{
		if (obj->get_type() == OT_GUN)
		{
		 	Gun * gun = (Gun *) obj;
			if (gun == c->selected_gun)
			{
				c->selected_gun = NULL;
			}
		}
	}

	if (dst == -1)
	{
	// disconnecting object, set collision flag.
    	PhysicalObject * phy = (PhysicalObject *) obj;
		phy->set_tangible(true);
	// be sure orientation quaternion is synced
		phy->set_orientation(phy->get_orientation_mat());
		phy->wake_up();

		if (obj->get_type() == OT_BOMB)
		{
		// we're throwing a bomb. Could be dropping, need a way to distinguish (MT_THROW).

			CBomb * bomb = (CBomb *) obj;
			bomb->pull_pin();

			Matrix R = c->get_orientation();
			Quaternion q = c->deform->get_heading_quaternion(R);
			R = q;

			Vector v = -10.0f * R.get_k();
			v.y += 5;

			bomb->set_velocity(v);
			bomb->set_angular_velocity(Vector(1, 1, 1));
		}
	}
	else
	{
		c->attach(dst, s->child);
	}

	s->child = INVALID_INSTANCE_INDEX;
}

//