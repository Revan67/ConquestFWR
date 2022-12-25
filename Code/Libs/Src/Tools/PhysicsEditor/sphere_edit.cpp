//
// <sphere_edit.cpp> - sphere editing
//

#include "main.h"
#include "phyedit.h"
#include "editmode.h"
#include "primitive.h"
#include "mingeom.h"
#include "mesh.h"

void SphereEditor::init(BaseExtent * extent, BOOL32 init_extent)
{
    edit_extent = (SphereExtent *) extent;

    if (init_extent)
    {
        edit_extent->sphere.radius = physicsEditor.object.scale;
        edit_extent->xform.translation.zero();
    }
    
}

void	SphereEditor::help_print(Font	*font)
{
	assert(font);

	font->draw_text(8, 8, "Sphere Editor Keys:");
	font->draw_text(8, 24, "O:  Create an extent sphere from selected faces");
	font->draw_text(8, 40, "Q:  Shrink Sphere");
	font->draw_text(8, 56, "W:  Grow Sphere");
}

void SphereEditor::keyboard_handler(const KeyboardMessage & kb)
{
    if (kb.pressed)
    {
        if (kb.key == 'W')
        {
            edit_extent->sphere.radius *= 1.01;
        }

        if (kb.key == 'Q')
        {
            edit_extent->sphere.radius *= 0.99;
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

                compute_optimal_sphere(edit_extent, vcount, vlist);
                edit_extent->xform.translation -= physicsEditor.object.center_of_mass;
            }
            else
            {
                compute_optimal_sphere(edit_extent, physicsEditor.object.mesh->object_vertex_cnt, physicsEditor.object.mesh->object_vertex_list);
                edit_extent->xform.translation -= physicsEditor.object.center_of_mass;
            }
        }
		else if(kb.key == VK_F1)
		{
			physicsEditor.ToggleHelp();
		}        
    }
}

void SphereEditor::post_render()
{
    INSTANCE_INDEX index    = physicsEditor.object.index;
    Vector position         = PHYSICS->get_center_of_mass(index);
    Matrix orientation      = ENGINE->get_orientation(index);

    draw_sphere(edit_extent, position, orientation);
}
    