//
// <box_edit.cpp> - box editing
//

#include "main.h"
#include "phyedit.h"
#include "editmode.h"
#include "primitive.h"
#include "mingeom.h"
#include "mesh.h"

void BoxEditor::init(BaseExtent * extent, BOOL32 init_extent)
{
    edit_extent = (BoxExtent *) extent;

    if (init_extent)
    {
        edit_extent->box.half_x = physicsEditor.object.scale * 0.1;
        edit_extent->box.half_y = physicsEditor.object.scale * 0.1;
        edit_extent->box.half_z = physicsEditor.object.scale * 0.1;
        
        edit_extent->xform.translation.zero();
    }
    
}

void	BoxEditor::help_print(Font	*font)
{
	assert(font);

	font->draw_text(8, 8, "Box Editor Keys:");
	font->draw_text(8, 24, "O:     Create an extent box from selected faces");
	font->draw_text(8, 40, "Q, W:  Shrink & Grow Box");
	font->draw_text(8, 56, "XYZ:   Shrink & Grow on a Box axis (Shift grows)");
	font->draw_text(8, 72, "R, E:  Rotate (Shift slows rotation)");
}

void BoxEditor::keyboard_handler(const KeyboardMessage & kb)
{
    if (kb.pressed)
    {        
        if (kb.key == 'O')
        {
            if (subMeshEditor.selected_face_count > 0)
            {
                // insert all unique vertices in selected faces into vlist and compute new extent

                S32 vcount = 0;
                Vector vlist[4096];
                Vector v;
                BOOL32 same;
				
				Vector p = ENGINE->get_position(physicsEditor.object.index);
				Matrix R = ENGINE->get_orientation(physicsEditor.object.index);

                for (int i = 0; i < subMeshEditor.selected_face_count; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        same = FALSE;
                        v = subMeshEditor.vlist[subMeshEditor.flist[subMeshEditor.selected_faces[i]].v[j]];

						// rotate v back into object space
						v = R.get_transpose() * (v - p);
                        
						for (int k = 0; k < vcount; k++)
                        {
                            if (vlist[k].equal(v, 0.000001))
                            {
                                same = TRUE;
                                break;
                            }
                        }
                        
                        if (!same)
                            vlist[vcount++] = v;
                        
                    }
				}
				
				compute_optimal_box(edit_extent, vcount, vlist);
				edit_extent->xform.translation	-=physicsEditor.object.center_of_mass;
            }
            else
            {
                compute_optimal_box(edit_extent, physicsEditor.CurObject->mesh->object_vertex_cnt, physicsEditor.CurObject->mesh->object_vertex_list);
                edit_extent->xform.translation -= physicsEditor.CurObject->center_of_mass;
            }
        }
        else
        if (kb.key == 'Q')
        {
            edit_extent->box.half_x *= 0.98;
            edit_extent->box.half_y *= 0.98;
            edit_extent->box.half_z *= 0.98;
        }
        else
        if (kb.key == 'W')
        {
            edit_extent->box.half_x *= 1.02;
            edit_extent->box.half_y *= 1.02;
            edit_extent->box.half_z *= 1.02;
        }
        else
        if (kb.key == 'X')
        {
            if (keyboard->pressed(VK_SHIFT))
                edit_extent->box.half_x *= 1.02;
            else
                edit_extent->box.half_x *= 0.98;
        }
        else
        if (kb.key == 'Y')
        {
            if (keyboard->pressed(VK_SHIFT))
                edit_extent->box.half_y *= 1.02;
            else
                edit_extent->box.half_y *= 0.98;
        }
        else
        if (kb.key == 'Z')
        {
            if (keyboard->pressed(VK_SHIFT))
                edit_extent->box.half_z *= 1.02;
            else
                edit_extent->box.half_z *= 0.98;
        }
		else
		if (kb.key == 'R')
		{
			SINGLE ang = physicsEditor.keyboard_dt * -3.0;

			if (keyboard->pressed(VK_SHIFT)) ang *= 0.1;

            Quaternion q(Vector(0.0, 0.0, 1.0), ang);
            edit_extent->xform.set_orientation(Matrix(q) * edit_extent->xform.get_orientation());
		}
		else
		if (kb.key == 'E')
		{
			SINGLE ang = physicsEditor.keyboard_dt * 3.0;

			if (keyboard->pressed(VK_SHIFT)) ang *= 0.1;

            Quaternion q(Vector(0.0, 0.0, 1.0), ang);
            edit_extent->xform.set_orientation(Matrix(q) * edit_extent->xform.get_orientation());
		}
		else if(kb.key == VK_F1)
		{
			physicsEditor.ToggleHelp();
		}
    }
}

void	BoxEditor::post_render(void)
{
	INSTANCE_INDEX	index		=physicsEditor.object.index;
	Vector			position	=PHYSICS->get_center_of_mass(index);
	Matrix			orientation	=ENGINE->get_orientation(index);
	
	draw_box(edit_extent, position, orientation);
}
    