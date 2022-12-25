//
// <convex_edit.cpp> - convex mesh editing
//

#include "main.h"
#include "phyedit.h"
#include "editmode.h"
#include "primitive.h"
#include "mingeom.h"
#include "mesh.h"
#include "chull3d.h"

void ConvexMeshEditor::init(BaseExtent * extent, BOOL32 init_extent)
{
    edit_extent = (ConvexMeshExtent *) extent;

    if (init_extent)
    {
        edit_extent->mesh = NULL;
        
    }
    
}

void	ConvexMeshEditor::help_print(Font	*font)
{
	assert(font);

	font->draw_text(8, 8, "Convex Mesh Editor Keys:");
	font->draw_text(8, 24, "O:  Create a convex mesh from selected vertices");
}

void ConvexMeshEditor::keyboard_handler(const KeyboardMessage & kb)
{
    if (kb.pressed)
    {        
        if ((kb.key == 'C') || (kb.key == 'O'))
        {
            // construct convex mesh

            static SINGLE x[1024];
            static SINGLE y[1024];
            static SINGLE z[1024];

            static CTriangle ct[1024];
            int     ct_count = 0;

            Vector world;
            S32 vcount;

            if (kb.key == 'O')
            {
                vcount = selected_vertex_count;

                for (int i = 0; i < vcount; i++)
                {
                    world = physicsEditor.CurObject->mesh->object_vertex_list[selected_vertex_list[i]];
                
                    x[i] = world.x;
                    y[i] = world.y;
                    z[i] = world.z;
                }
            }
/*            else
            {
                vcount = physicsEditor.CurObject->mesh->object_vertex_cnt;

                for (int i = 0; i < vcount; i++)
                {
                    world = physicsEditor.CurObject->mesh->object_vertex_list[i];
                
                    x[i] = world.x;
                    y[i] = world.y;
                    z[i] = world.z;
                }
            }*/

            ConvexHull3D (vcount, x, y, z, ct_count, ct);

            // generate extent
            if (!edit_extent->mesh)
            {
                edit_extent->mesh = new CollisionMesh;
            }
            else
            {
                edit_extent->mesh->free();
            }
            
            CollisionMesh * m = edit_extent->mesh;
 
            m->num_vertices = vcount;
            m->vertices = new Vertex[vcount];

            for (int i = 0; i < vcount; i++)
            {
                m->vertices[i].p.set(x[i], y[i], z[i]);
            }

            m->num_triangles = ct_count;
            m->triangles = new Triangle[ct_count];

            for (i = 0; i < ct_count; i++)
            {
                m->triangles[i].v[0] = ct[i][0];
                m->triangles[i].v[1] = ct[i][1];
                m->triangles[i].v[2] = ct[i][2];
            }

            m->compute_edges();
            m->compute_normals();

            // offset
            edit_extent->xform.translation = -physicsEditor.CurObject->center_of_mass;
        }        
		else if(kb.key == VK_F1)
		{
			physicsEditor.ToggleHelp();
		}
    }
}

void ConvexMeshEditor::add_selected_vertex(S32 target_x, S32 target_y)
{
    INSTANCE_INDEX index = physicsEditor.CurObject->index;
    Vector p = ENGINE->get_position(index);
    Matrix R = ENGINE->get_orientation(index);

    SINGLE sx, sy, depth;
    S32    vtx_index = -1;
    SINGLE cur_dist, min_dist = 10.0;

    Mesh * mesh = physicsEditor.CurObject->mesh;

    if(!mesh)
	{
		return;
	}

    for (int i = 0; i < mesh->object_vertex_cnt; i++)
    {
        Vector world = p + (R * mesh->object_vertex_list[i]);
        Vector n = R * mesh->normal_ABC[mesh->vertex_normal[i]];

        if (n.z > 0.0)
        {
            if (physicsEditor.camera->point_to_screen(sx, sy, depth, world))
            {
                cur_dist = sqrt((sx - target_x) * (sx - target_x) + (sy - target_y) * (sy - target_y));

                if (cur_dist < min_dist)
                {
                    min_dist = cur_dist;
                    vtx_index = i;
                }
            
            }
        }

    }

    // check to see if selected vertex is already selected

    if(vtx_index == -1)
	{
		//try to pick a face instead
		select_verts_by_face();
		return;
	}

    BOOL32 found = FALSE;
    S32    found_idx = 0;

    for (i = 0; i < selected_vertex_count; i++)
    {
        if (selected_vertex_list[i] == vtx_index)
        {
            found = TRUE;
            found_idx = i;
            break;
        }
    }

    if (!found)
    {
        selected_vertex_list[selected_vertex_count++] = vtx_index;
    }
    else
    {
        // remove from selected list
        for (int i = found_idx; i < selected_vertex_count - 1; i++)
        {
            selected_vertex_list[i] = selected_vertex_list[i + 1];
        }

        selected_vertex_count--;
    }

}


void	ConvexMeshEditor::mouse_handler(const MouseMessage & message)
{
	switch(message.message)
	{
		case	WM_MOUSEMOVE:
			mouse_x	=message.x;
			mouse_y	=message.y;
			break;
		
		case	WM_LBUTTONDOWN:
			add_selected_vertex(mouse_x, mouse_y);
            break;

		case	WM_LBUTTONUP:
			break;
		
		case	WM_RBUTTONDOWN:
			selected_vertex_count	=0;
			break;
	}
}

extern	S32	point_in_poly(SINGLE * xp, SINGLE * yp, float x, float y);

void ConvexMeshEditor::select_verts_by_face(void)
{
	Vector			tv0, tv1, tv2;
    SINGLE			xa[3], ya[3], depth;
	INSTANCE_INDEX	index	=physicsEditor.CurObject->index;
	Vector			cz		=physicsEditor.camera->get_orientation() * Vector(0.0, 0.0, -1.0);
	Mesh			*mesh	=physicsEditor.CurObject->mesh;
	Vector			p		=ENGINE->get_position(index);
	Matrix			R		=ENGINE->get_orientation(index);
	int				i, j;
//	int				k, m;	disable deselect
	
	if(!mesh)	//cant do mesh stuff if there is no mesh
	{
		return;
	}
	
	for(i=0;i < mesh->face_cnt;i++)
	{
		tv0	=p + (R * mesh->object_vertex_list[mesh->get_face_vertex_index(i, 0)]);
		tv1	=p + (R * mesh->object_vertex_list[mesh->get_face_vertex_index(i, 1)]);
		tv2	=p + (R * mesh->object_vertex_list[mesh->get_face_vertex_index(i, 2)]);
		
		if(!physicsEditor.camera->point_to_screen(xa[0], ya[0], depth, tv0))
		{
			continue;
		}
        
        if(!physicsEditor.camera->point_to_screen(xa[1], ya[1], depth, tv1))
		{
			continue;
		}
		
		if(!physicsEditor.camera->point_to_screen(xa[2], ya[2], depth, tv2))
		{
			continue;
		}
		if(point_in_poly(xa, ya, mouse_x, mouse_y))
		{
			//check dot product
			Vector	dp	=R * (*mesh->get_face_normal(i));
			
			if(dot_product(dp, cz) > 0.0)
			{
				continue;
			}

			//probably a bad idea to deselect here
/*
			BOOL32	found[3]		={ FALSE, FALSE, FALSE };
			for(j=0;j < selected_vertex_count;j++)
			{
				for(k=0;k < 3;k++)
				{
					if(selected_vertex_list[j] == (mesh->get_face_vertex_index(i, k)))
					{
						found[k]	=TRUE;

						//remove from selected list
						for(m=j;m < selected_vertex_count - 1;m++)
						{
							selected_vertex_list[m]	=selected_vertex_list[m + 1];
						}
						selected_vertex_count--;
					}
				}
			}*/
			for(j=0;j < 3;j++)
			{
//				if(!found[j])
				{
					selected_vertex_list[selected_vertex_count++]	=(mesh->get_face_vertex_index(i, j));
				}
			}
			break;
		}
	}		
}

void ConvexMeshEditor::post_render(void)
{
	INSTANCE_INDEX	index	=physicsEditor.CurObject->index;
	Vector			p		=ENGINE->get_position(index);
	Matrix			R		=ENGINE->get_orientation(index);
	Mesh			*mesh	=physicsEditor.CurObject->mesh;
	
	if(!mesh)	//cant do mesh stuff if there is no mesh
	{
		return;
	}
	
	//look for mouseover hits
	Vector	world, worldofs;
	SINGLE	cur_dist, min_dist, sx, sy, depth;
	SINGLE	dz		=physicsEditor.CurObject->scale * 0.004;
	Vector	cdir	=physicsEditor.camera->get_orientation() * Vector(0.0, 0.0, dz);
	int		i, vtx_index	=0;
	
	for(i=0,min_dist=10.0f;i < mesh->object_vertex_cnt;i++)
	{
		Vector	n	=R * mesh->normal_ABC[mesh->vertex_normal[i]];
		if(*((U32 *)&n.z) & 0x80000000)	//if(n.z < 0)
		{
			continue;
		}
		
		world	=p + (R * mesh->object_vertex_list[i]);

		if(physicsEditor.camera->point_to_screen(sx, sy, depth, world))
		{
			cur_dist	=sqrt((sx - mouse_x) * (sx - mouse_x) + (sy - mouse_y) * (sy - mouse_y));
			
			if(cur_dist < min_dist)
			{
				min_dist	=cur_dist;
				vtx_index	=i;
			}
		}
	}

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glColor4f(1.0, 0.7, 1.0, 1.0);
    glBegin(GL_POINTS);

    for(i = 0; i < mesh->object_vertex_cnt; i++)
    {
		world = p + (R * mesh->object_vertex_list[i]);
        world += cdir;                                              // offset to avoid zbuffer prob

		if(vtx_index != i)
		{
			glVertex3f(world.x, world.y, world.z);
		}
    }

	glEnd();

    glDepthFunc(GL_ALWAYS);

	if(vtx_index)
	{
		//get a good idea for a size in screenspace
		world	=p + (R * mesh->object_vertex_list[vtx_index]);
		world	+=cdir;

		//find the spot on the screen
		physicsEditor.camera->point_to_screen(sx, sy, depth, world);

		//adjust the near plane so the next call lands planar to world
		SINGLE	nearp	=physicsEditor.camera->znear;

		physicsEditor.camera->set_near_plane_distance(depth);

		//move over 5 pixies
		physicsEditor.camera->screen_to_point(worldofs, sx+2, sy+2);
		worldofs.z	=world.z;

		physicsEditor.camera->set_near_plane_distance(nearp);

		SINGLE	vscale	=worldofs.z - world.z;

		Vector	dist	=world - worldofs;

		SINGLE	len	=dist.magnitude();

		glBegin(GL_TRIANGLES);

		glColor4f(0.6, 0.0, 0.6, 0.25);
		
		glVertex3f(world.x+len, world.y+len, world.z);
		glVertex3f(world.x-len, world.y+len, world.z);
		glVertex3f(world.x-len, world.y-len, world.z);
		glVertex3f(world.x-len, world.y-len, world.z);
		glVertex3f(world.x+len, world.y-len, world.z);
		glVertex3f(world.x+len, world.y+len, world.z);
		
		glVertex3f(world.x, world.y+len, world.z+len);
		glVertex3f(world.x, world.y+len, world.z-len);
		glVertex3f(world.x, world.y-len, world.z-len);
		glVertex3f(world.x, world.y-len, world.z-len);
		glVertex3f(world.x, world.y-len, world.z+len);
		glVertex3f(world.x, world.y+len, world.z+len);
		
		glVertex3f(world.x+len, world.y, world.z+len);
		glVertex3f(world.x-len, world.y, world.z+len);
		glVertex3f(world.x-len, world.y, world.z-len);
		glVertex3f(world.x-len, world.y, world.z-len);
		glVertex3f(world.x+len, world.y, world.z-len);
		glVertex3f(world.x+len, world.y, world.z+len);
		
		glColor4f(1.0, 0.7, 1.0, 1.0);

		glEnd();
	}

    // draw all selected vertices
    
    glBegin(GL_LINES);


    for (i = 0; i < selected_vertex_count; i++)
    {
        world	=p + (R * mesh->object_vertex_list[selected_vertex_list[i]]);

        glColor4f(1.0, 1.0, 0.6, 1.0);
        
        glVertex3f(world.x - dz, world.y, world.z);
        glVertex3f(world.x + dz, world.y, world.z);

        glColor4f(1.0, 1.0, 0.7, 1.0);
        
        glVertex3f(world.x, world.y - dz, world.z);
        glVertex3f(world.x, world.y + dz, world.z);

        glColor4f(0.9, 0.9, 0.5, 1.0);
        
        glVertex3f(world.x, world.y, world.z - dz);
        glVertex3f(world.x, world.y, world.z + dz);

    }

    glEnd();

    // draw the convex mesh

    draw_mesh(edit_extent, PHYSICS->get_center_of_mass(index), R);
}

void ConvexMeshEditor::font_render(Font * font)
{
    if (edit_extent->mesh)
    {
        font->print_var(10, 30, "Convex mesh stats: ", "%d vertices, %d faces, %d normals", edit_extent->mesh->num_vertices, edit_extent->mesh->num_triangles, edit_extent->mesh->num_normals);
    }
}