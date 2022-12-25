//
// <tube_edit.cpp> - tube editing
//

#include "main.h"
#include "phyedit.h"
#include "editmode.h"
#include "primitive.h"
#include "mingeom.h"
#include "mesh.h"

void TubeEditor::init(BaseExtent * extent, BOOL32 init_extent)
{
    edit_extent = (TubeExtent *) extent;

    if (init_extent)
    {
        edit_extent->tube.radius = physicsEditor.object.scale;
        edit_extent->xform.translation.zero();
    }
    
}

void	TubeEditor::help_print(Font	*font)
{
	assert(font);

	font->draw_text(8, 8, "Tube Editor Keys:");
	font->draw_text(8, 24, "O:  Create an extent tube from selected faces");
	font->draw_text(8, 40, "Q:  Shrink tube");
	font->draw_text(8, 56, "W:  Grow tube");
}

void TubeEditor::keyboard_handler(const KeyboardMessage & kb)
{
    if (kb.pressed)
    {
        if (kb.key == 'W')
        {
            edit_extent->tube.radius *= 1.01;
        }

        if (kb.key == 'Q')
        {
            edit_extent->tube.radius *= 0.99;
        }

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
                        v = *physicsEditor.object.mesh->get_face_vertex(subMeshEditor.selected_faces[i], j);

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

                compute_optimal_tube(edit_extent, vcount, vlist);
                edit_extent->xform.translation -= physicsEditor.object.center_of_mass;
            }
            else
            {
                compute_optimal_tube(edit_extent, physicsEditor.object.mesh->object_vertex_cnt, physicsEditor.object.mesh->object_vertex_list);
                edit_extent->xform.translation -= physicsEditor.object.center_of_mass;
            }
        }
		else if(kb.key == VK_F1)
		{
			physicsEditor.ToggleHelp();
		}        
    }
}

void TubeEditor::post_render()
{
    INSTANCE_INDEX index    = physicsEditor.object.index;
    Vector position         = PHYSICS->get_center_of_mass(index);
    Matrix orientation      = ENGINE->get_orientation(index);

    draw_tube(edit_extent, position, orientation);
}
    