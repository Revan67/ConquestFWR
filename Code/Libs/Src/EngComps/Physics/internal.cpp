//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dacom.h"
#include "filesys.h"

#include "rigid.h"

//

void RigidBodyPhysics::compute_internal_forces(void)
{
// Apply spring & damper type forces.
// Traverse trees, computing spring forces based on current state.

	INSTANCE_INDEX root = -1;
	while ((root = engine->get_root_instance_next(root)) != -1)
	{
		int joint_index = -1;
		while ((joint_index = engine->get_instance_child_next(root, 0, joint_index)) != -1)
		{
			const JointInfo* joint = engine->get_joint_info(joint_index);

			if (joint->type == JT_DAMPED_SPRING)
			{
				Instance * obj1 = GetInstance(engine->get_instance_parent(joint_index));
				Instance * obj2 = GetInstance(joint_index);

				Vector r1 = obj1->get_orientation() * joint->parent_point;
				Vector r2 = obj2->get_orientation() * joint->child_point;

				Vector x1 = obj1->get_position() + r1;
				Vector x2 = obj2->get_position() + r2;

				Vector v1 = obj1->v + cross_product(obj1->w, r1);
				Vector v2 = obj2->v + cross_product(obj2->w, r2);

				Vector l = x1 - x2;
				Vector ldot = v1 - v2;

				SINGLE lm = l.magnitude();

				SINGLE displacement = lm - joint->rest_length;

			// Ignore ridiculously small displacements.
				if (fabs(displacement) > 1e-4)
				{
					SINGLE fm = -(joint->spring_constant * displacement + joint->damping_constant * dot_product(ldot, l) / lm);

					Vector force = l * fm / lm;

					obj1->add_internal_force_at_point(force, x1);
					obj2->add_internal_force_at_point(-force, x2);
				}
			}
		}
	}

// Compute ForceElement forces.

	Vector f, t;
	Link<ForceElement *> * node = forces.get_head();
	while (node)
	{
		ForceElement * fe = node->obj;

		if (fe->object != INVALID_INSTANCE_INDEX)
		{
			Instance * instance = GetInstance(fe->object);
			if (instance)
			{
				engine->set_position(fe->object, instance->get_position());
				engine->set_orientation(fe->object, instance->R);

				fe->compute_force_and_torque(fe->object, f, t);
				//apply_forces_to_tree(fe->object, f, t);
				instance->F_applied += f;
				instance->T_applied += t;
			}
		}
		else
		{
			Link<INSTANCE_INDEX> * inst_node = dynamic_list.get_head();
			while (inst_node)
			{
				INSTANCE_INDEX idx = inst_node->obj;
				Instance * instance = GetInstance( idx );
				if (fe->affects_object(idx))
				{
				// Notify the engine of the object's temporary intermediate position.
					engine->set_position(idx, instance->get_position());
					engine->set_orientation(idx, instance->R);

					fe->compute_force_and_torque(idx, f, t);
					//apply_forces_to_tree(idx, f, t);
					instance->F_applied += f;
					instance->T_applied += t;
				}

				inst_node = inst_node->next;
			}
		}

		node = node->next;
	}


// Solve ABM constraints.

	if (joint_dynamics)
	{
		root = -1;
		while ((root = engine->get_root_instance_next(root)) != -1)
		{
			INSTANCE_INDEX child = engine->get_instance_child_next(root,0,INVALID_INSTANCE_INDEX);
			if (child != INVALID_INSTANCE_INDEX)
			{
				Instance * ri = GetInstance(root);
				compute_ABM_dynamics(ri);
			}
		}
	}
}

//

void RigidBodyPhysics::apply_forces_to_tree(INSTANCE_INDEX root, const Vector & f, const Vector & t)
{
	Instance * instance = GetInstance(root);
	instance->F_internal += f;
	instance->T_internal += t;
	INSTANCE_INDEX c = engine->get_instance_child_next(root, 0, INVALID_INSTANCE_INDEX );
	while (c != INVALID_INSTANCE_INDEX)
	{
		apply_forces_to_tree(c, f, t);
		c = engine->get_instance_child_next(root, 0, c);
	}
}

//