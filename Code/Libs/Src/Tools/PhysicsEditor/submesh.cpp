//
// <submeshedit.cpp>
//

#include "main.h"
#include "phyedit.h"
#include "submesh.h"
#include "mesh.h"

// misc functions:

void SubMeshEditor::fill_vlist(INSTANCE_INDEX index)
{
	// creates all the faces and etc
	// all vectors are in WORLD SPACE
	// this needs to be recalled when the camera moves
	
    Vector p	= ENGINE->get_position(index);
    Matrix R	= ENGINE->get_orientation(index);

	Mesh * mesh = RENDERER->get_instance_mesh(index);

	if(mesh)
	{
		S32 base_v = vcount;

		// fill vertices

		for (int i = 0; i < mesh->object_vertex_cnt; i++)
		{
			vlist[vcount++] = p + (R * mesh->object_vertex_list[i]);
		}

		// fill faces

		for (i = 0; i < mesh->face_cnt; i++)
		{
			flist[fcount].v[0]		= base_v + mesh->get_face_vertex_index(i, 0);
			flist[fcount].v[1]		= base_v + mesh->get_face_vertex_index(i, 1);
			flist[fcount].v[2]		= base_v + mesh->get_face_vertex_index(i, 2);
			flist[fcount++].normal	= R * (*mesh->get_face_normal(i));
		}

		INSTANCE_INDEX	child	=INVALID_INSTANCE_INDEX;

		child	= ENGINE->get_instance_child_next(index, EN_DONT_RECURSE, child);

		while (child != INVALID_INSTANCE_INDEX)
		{
			fill_vlist(child);
			child = ENGINE->get_instance_child_next(index, EN_DONT_RECURSE, child);
		}
	}
}

void SubMeshEditor::get_closest_vertex(S32 target_x, S32 target_y, INSTANCE_INDEX index, Vector * vertex)
{
    SINGLE sx, sy, depth;
    S32    vtx_index = -1;
    SINGLE cur_dist, min_dist = 100000000.0;

	for (int i = 0; i < vcount; i++)
	{
        if (physicsEditor.camera->point_to_screen(sx, sy, depth, vlist[i]))
        {
            cur_dist = sqrt((sx - target_x) * (sx - target_x) + (sy - target_y) * (sy - target_y));

            if (cur_dist < min_dist)
            {
                min_dist = cur_dist;
                vtx_index = i;
            }
        
        }
        
    }
    
    *vertex = vlist[i];

}

S32 point_in_poly(SINGLE * xp, SINGLE * yp, float x, float y)
{
    int i, j, c = 0;

    for (i = 0, j = 2; i < 3; j = i++) 
    {
        if ((((yp[i] <= y) && (y < yp[j])) || ((yp[j] <= y) && (y < yp[i]))) && (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
        {
            c = !c;
        }
    }
    return c;
}

void SubMeshEditor::toggle_selected_face(S32 mx, S32 my)
{
    INSTANCE_INDEX index = physicsEditor.object.index;

    Vector tv0, tv1, tv2;
    SINGLE xa[3], ya[3], depth;

    Vector cz = physicsEditor.camera->get_orientation() * Vector(0.0, 0.0, -1.0);

    for (int i = 0; i < fcount; i++)
    {
        tv0 = vlist[flist[i].v[0]];
        tv1 = vlist[flist[i].v[1]];
        tv2 = vlist[flist[i].v[2]];
        
        if (!physicsEditor.camera->point_to_screen(xa[0], ya[0], depth, tv0))
        {
            continue;
        }
        
        if (!physicsEditor.camera->point_to_screen(xa[1], ya[1], depth, tv1))
        {
            continue;
        }
        
        if (!physicsEditor.camera->point_to_screen(xa[2], ya[2], depth, tv2))
        {
            continue;
        }
                       
        if (point_in_poly(xa, ya, mx, my))
        {
            // check dot product

            Vector dp = flist[i].normal;
            
            if (dot_product(dp, cz) > 0.0)
            {
                continue;
            }
            
            // bingo

            BOOL32 found = FALSE;

            for (int j = 0; j < selected_face_count; j++)
            {
                if (selected_faces[j] == i)
                {
                    // remove face
                    found = TRUE;

                    for (int k = j; k < (selected_face_count - 1); k++)
                        selected_faces[k] = selected_faces[k + 1];

                    selected_face_count--;
                }
                    
            }

            if (!found)
                selected_faces[selected_face_count++] = i;

            break;
        }
     
    }

}

void SubMeshEditor::calculate_selected_submesh()
{
    INSTANCE_INDEX index = physicsEditor.object.index;
    
    SINGLE sx, sy;
    SINGLE depth;

    selected_face_count = 0;

    S32 bx0 = __min(mouse_x, box_start_x);
    S32 bx1 = __max(mouse_x, box_start_x);
    S32 by0 = __min(mouse_y, box_start_y);
    S32 by1 = __max(mouse_y, box_start_y);

    // add all selected faces

    Vector w[3];
    S32 count;

    for (int i = 0; i < fcount; i++)
    {
        count = 0;

        for (int j = 0; j < 3; j++)
        {
            if (physicsEditor.camera->point_to_screen(sx, sy, depth, vlist[flist[i].v[j]]))
            {
                if ((sx > bx0) && (sx < bx1) && (sy > by0) && (sy < by1))
                {
                    count++;
                }
            }
        }

        if (count == 3)
        {
            // add face
            selected_faces[selected_face_count++] = i;
        }
    }

}

// submesh functions:

void SubMeshEditor::process_keyboard(KeyboardMessage * message)
{
}

void SubMeshEditor::process_mouse(MouseMessage * message)
{
    switch (message->message)
    {
        case WM_MOUSEMOVE:

            mouse_x = message->x;
            mouse_y = message->y;

            if (creating_box)
            {
                // recalculate selected mesh here
                calculate_selected_submesh();
            }

            break;

        case WM_LBUTTONDOWN:

            if (keyboard->pressed(VK_SHIFT))
            {
                toggle_selected_face(mouse_x, mouse_y);
            }
            else
            {
                creating_box    = TRUE;
                box_start_x     = mouse_x;
                box_start_y     = mouse_y;
            }

            break;

        case WM_LBUTTONUP:

            creating_box    = FALSE;
            break;

        case WM_RBUTTONDOWN:

            creating_box    = FALSE;
            selected_face_count = 0;
            break;
        
    }
}


void SubMeshEditor::render()
{
    if (physicsEditor.object.index == INVALID_INSTANCE_INDEX) return;
    
    INSTANCE_INDEX index = physicsEditor.object.index;

    // perspective draw:

	glDisable(GL_TEXTURE_2D);
    glDepthFunc(GL_ALWAYS);
 
    // draw selected faces

    Vector world;
    
    glColor4f(0.7, 0.0, 0.0, 0.3);
    glEnable(GL_BLEND);
	glDepthMask(FALSE);
    //glDepthFunc(GL_EQUAL);

    glBegin(GL_TRIANGLES);

    for (int i = 0; i < selected_face_count; i++)
    {
        world = vlist[flist[selected_faces[i]].v[0]];
        glVertex3f(world.x, world.y, world.z);
        
        world = vlist[flist[selected_faces[i]].v[1]];
        glVertex3f(world.x, world.y, world.z);
        
        world = vlist[flist[selected_faces[i]].v[2]];
        glVertex3f(world.x, world.y, world.z);
    }

    glEnd();

//	glFlush();

    glDepthFunc(GL_ALWAYS);
    glDisable(GL_BLEND);
    
    // ortho draw:

	Transform t;
    t.set_identity();
    RP->set_modelview(t);

    RP->set_viewport(0, 0, 640, 480);
    RP->set_ortho(0, 640, 480, 0);
	   
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    if (creating_box)
    {
        glBegin(GL_LINES);
        
        glVertex3f(box_start_x, box_start_y, 1.0);
        glVertex3f(mouse_x, box_start_y, 1.0);

        glVertex3f(mouse_x, box_start_y, 1.0);
        glVertex3f(mouse_x, mouse_y, 1.0);

        glVertex3f(mouse_x, mouse_y, 1.0);
        glVertex3f(box_start_x, mouse_y, 1.0);

        glVertex3f(box_start_x, mouse_y, 1.0);
        glVertex3f(box_start_x, box_start_y, 1.0);
        
        glEnd();
    }

	physicsEditor.initialize_matrix();
 
}