//
// <primitive.cpp> - primitive rendering functions
//

#include "primitive.h"
#include "phyedit.h"
#include "main.h"
#include "extent.h"

#define SPHERE_DIVISIONS    12

void dsphere(SINGLE radius, const Vector & extent_p, const Matrix & extent_R, const Vector & camera_p, const Matrix & camera_R, BOOL32 filled, BOOL32 selected)
{
	SINGLE zradius;

    static Vector ij_table[SPHERE_DIVISIONS + 1][SPHERE_DIVISIONS];
    static Vector n_table[SPHERE_DIVISIONS + 1][SPHERE_DIVISIONS];

    S32 i, j;
	Vector w;

	// setup

    for (i = 0; i <= SPHERE_DIVISIONS; i++)
    {
        zradius = radius * sin(i * PI / SPHERE_DIVISIONS);             // 0 to 1 to 0

        for (j = 0; j < SPHERE_DIVISIONS; j++)
        {
            w.x = zradius * cos(j * 2.0 * PI / SPHERE_DIVISIONS);
            w.y = zradius * sin(j * 2.0 * PI / SPHERE_DIVISIONS);
            w.z = radius * cos(i * PI / SPHERE_DIVISIONS);
            
            w = (extent_R * w) + extent_p;
            w = (camera_R * w) + camera_p;

            ij_table[i][j] = w;
        
            n_table[i][j] = w;
            n_table[i][j].normalize();              // ick
        }
    }

	// draw

	Vector lv(0.81, 0.35, 0.47);
    S32 ni, nj;
        
    if (filled)
    {
        glColor4f(0.6, 0.6, 0.6, 0.3);
        glBegin(GL_TRIANGLES);

        for (i = 0; i < SPHERE_DIVISIONS; i++)
        {
            ni = i + 1;

            for (j = 0; j < (SPHERE_DIVISIONS); j++)
            {
                nj = (j + 1) % SPHERE_DIVISIONS;

                static SINGLE diff1, diff2, diff3, diff4;
                
                diff1 = dot_product(n_table[i][j], lv);
                diff2 = dot_product(n_table[ni][j], lv);
                diff3 = dot_product(n_table[ni][nj], lv);
                diff4 = dot_product(n_table[i][nj], lv);
                
                glColor4f(0.4 + diff1, 0.4 + diff1, 0.4 + diff1, 0.3);
                glVertex3f(ij_table[i][j].x,    ij_table[i][j].y,   ij_table[i][j].z);
                
                glColor4f(0.4 + diff2, 0.4 + diff2, 0.4 + diff2, 0.3);
                glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
                
                glColor4f(0.4 + diff4, 0.4 + diff4, 0.4 + diff4, 0.3);
                glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
                
                glColor4f(0.4 + diff2, 0.4 + diff2, 0.4 + diff2, 0.3);
                glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
                
                glColor4f(0.4 + diff3, 0.4 + diff3, 0.4 + diff3, 0.3);
                glVertex3f(ij_table[ni][nj].x,  ij_table[ni][nj].y, ij_table[ni][nj].z);
                
                glColor4f(0.4 + diff4, 0.4 + diff4, 0.4 + diff4, 0.3);
                glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
        
                glColor4f(0.6, 0.6, 0.6, 0.3);
                glVertex3f(ij_table[i][j].x,    ij_table[i][j].y,   ij_table[i][j].z);
                glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
                glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
                glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
                glVertex3f(ij_table[ni][nj].x,  ij_table[ni][nj].y, ij_table[ni][nj].z);
                glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
            }
        }
        glEnd();
    }
    
    // lines
    
    if (!selected)
        glColor4f(0.2, 0.2, 0.2, 1.0);
    else
        glColor4f(0.6, 0.6, 0.6, 1.0);

    glBegin(GL_LINES);

    for (i = 0; i < SPHERE_DIVISIONS; i++)
    {
        ni = i + 1;

        for (j = 0; j < (SPHERE_DIVISIONS); j++)
        {
            nj = (j + 1) % SPHERE_DIVISIONS;

            glVertex3f(ij_table[i][j].x,    ij_table[i][j].y,   ij_table[i][j].z);
            glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
            glVertex3f(ij_table[ni][j].x,   ij_table[ni][j].y,  ij_table[ni][j].z);
            glVertex3f(ij_table[ni][nj].x,  ij_table[ni][nj].y, ij_table[ni][nj].z);
            glVertex3f(ij_table[ni][nj].x,  ij_table[ni][nj].y, ij_table[ni][nj].z);
            glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
            glVertex3f(ij_table[i][nj].x,   ij_table[i][nj].y,  ij_table[i][nj].z);
            glVertex3f(ij_table[i][j].x,    ij_table[i][j].y,   ij_table[i][j].z);
        
        }
    }

    glEnd();
	
}

void draw_sphere(SphereExtent *sphere, const Vector & position, const Matrix & orientation)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
//    glHint(GL_VOLUME_CLIPPING_HINT, GL_NICEST);
   
    BOOL32 fill;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
   
    if (physicsEditor.flags & PEC_SOLID_EXTENTS)
    {   
        fill = TRUE;
    }
    else
    {
        fill = FALSE;
    }
   
    BOOL32 selected = (physicsEditor.selected_extent == sphere) ? TRUE : FALSE;
    
	dsphere(sphere->sphere.radius, sphere->xform.get_position(), sphere->xform.get_orientation(), position, orientation, fill, selected);
    
}

#define LINE_DRAW(a, b) { glVertex3f(p[a].x, p[a].y, p[a].z); glVertex3f(p[b].x, p[b].y, p[b].z); }

#define TRI_DRAW(a, b, c) { glVertex3f(p[a].x, p[a].y, p[a].z); \
                            glVertex3f(p[b].x, p[b].y, p[b].z); \
                            glVertex3f(p[c].x, p[c].y, p[c].z); }

void draw_box(BoxExtent *box_extent, const Vector & position, const Matrix & orientation)
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
//    glHint(GL_VOLUME_CLIPPING_HINT, GL_NICEST);

    BOOL32 fill;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
   
    if (physicsEditor.flags & PEC_SOLID_EXTENTS)
    {   
        fill = TRUE;
    }
    else
    {
        fill = FALSE;
    }
   
    BOOL32 selected = (physicsEditor.selected_extent == box_extent) ? TRUE : FALSE;
    
    static Vector p[8];

    Box *box = &box_extent->box;
    Vector center = box_extent->xform.translation;

    p[0].x = -box->half_x; p[0].y =  box->half_y; p[0].z = -box->half_z;
    p[1].x =  box->half_x; p[1].y =  box->half_y; p[1].z = -box->half_z;
    p[2].x = -box->half_x; p[2].y =  box->half_y; p[2].z =  box->half_z;
    p[3].x =  box->half_x; p[3].y =  box->half_y; p[3].z =  box->half_z;
    p[4].x = -box->half_x; p[4].y = -box->half_y; p[4].z = -box->half_z;
    p[5].x =  box->half_x; p[5].y = -box->half_y; p[5].z = -box->half_z;
    p[6].x = -box->half_x; p[6].y = -box->half_y; p[6].z =  box->half_z;
    p[7].x =  box->half_x; p[7].y = -box->half_y; p[7].z =  box->half_z;

    Matrix mo = box_extent->xform.get_orientation();

    for (int i = 0; i < 8; i++)
    {
       // orient box
       p[i] = (mo * p[i]) + center;

       // orient to object
       p[i] = (orientation * p[i]) + position;
    }

    if (fill)
    {
        glColor4f(0.7, 0.5, 1.0, 0.3);
        glBegin(GL_TRIANGLES);

        TRI_DRAW(0, 1, 2); TRI_DRAW(1, 3, 2);
        TRI_DRAW(3, 1, 7); TRI_DRAW(1, 5, 7);
        TRI_DRAW(2, 3, 6); TRI_DRAW(3, 7, 6);
        TRI_DRAW(0, 4, 1); TRI_DRAW(4, 5, 1);
        TRI_DRAW(2, 0, 6); TRI_DRAW(0, 4, 6);
        TRI_DRAW(4, 6, 5); TRI_DRAW(6, 7, 5);

        glEnd();
    }

    if (!selected)
    {
        glColor4f(0.7, 0.5, 1.0, 0.7);
    }
    else
    {
        glColor4f(0.8, 0.8, 1.0, 1.0);
    }

    glBegin(GL_LINES);

    LINE_DRAW(0, 2); LINE_DRAW(2, 3); LINE_DRAW(3, 1); LINE_DRAW(1, 0);
    LINE_DRAW(4, 6); LINE_DRAW(6, 7); LINE_DRAW(7, 5); LINE_DRAW(5, 4);
    LINE_DRAW(2, 6); LINE_DRAW(3, 7); LINE_DRAW(0, 4); LINE_DRAW(1, 5);

    glEnd();
    
}

void draw_mesh(ConvexMeshExtent * mesh, const Vector & p, const Matrix & R)
{
	int	i;

    if (mesh->mesh == NULL) return;

    BOOL32 fill = (physicsEditor.flags & PEC_SOLID_EXTENTS) ? TRUE : FALSE;

    glDepthFunc(GL_LEQUAL);
 
    Vector world;
    
    if(physicsEditor.selected_extent != mesh)
	{
		glColor4f(0.7, 0.4, 1.0, 0.7);
	}
	else
	{
        glColor4f(0.6, 0.6, 0.6, 1.0);
	}

    glEnable(GL_BLEND);

    if (fill)
        glBegin(GL_TRIANGLES);
    else
        glBegin(GL_LINES);

    CollisionMesh * m = mesh->mesh;

    Vector v1, v2, v3;

    // faces
    Vector center   = mesh->xform.translation;
    
    for (i = 0; i < m->num_triangles; i++)
    {
        v1 = p + (R * (center + m->vertices[m->triangles[i].v[0]].p));
        v2 = p + (R * (center + m->vertices[m->triangles[i].v[1]].p));
        v3 = p + (R * (center + m->vertices[m->triangles[i].v[2]].p));
        
        if (fill)
        {
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v3.x, v3.y, v3.z);
        }
        else
        {
            glVertex3f(v1.x, v1.y, v1.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v2.x, v2.y, v2.z);
            glVertex3f(v3.x, v3.y, v3.z);
            glVertex3f(v3.x, v3.y, v3.z);
            glVertex3f(v1.x, v1.y, v1.z);
        }

    }

    glEnd();

    // normals
    // vertex normals, face normals, edge normals

    Vector ns, ne;

    glColor4f(0.9, 0.9, 1.0, 0.7);

    glBegin(GL_LINES);
    
    for (i = 0; i < m->num_vertices; i++)
    {
        ns	=p + (R * (center + m->vertices[i].p));
		ne	=ns + (R * (m->normals[m->vertices[i].n] * physicsEditor.object.scale * 0.05));

        glVertex3f(ns.x, ns.y, ns.z);
        glVertex3f(ne.x, ne.y, ne.z);

    }
    glEnd();
}

void draw_tube(TubeExtent * tube, const Vector & p, const Matrix & R)
{
	// tube is a cylinder with half-spheres as end-caps

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
//    glHint(GL_VOLUME_CLIPPING_HINT, GL_NICEST);
   
    BOOL32 fill;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
   
	if (physicsEditor.flags & PEC_SOLID_EXTENTS)
    {   
        fill = TRUE;
    }
    else
    {
        fill = FALSE;
    }
   
    BOOL32 selected = (physicsEditor.selected_extent == tube) ? TRUE : FALSE;
}


void render_extent(BaseExtent * extent, const Vector & com, const Matrix & R)
{
	switch (extent->type)
	{
		case ET_SPHERE:
			draw_sphere((SphereExtent *) extent, com, R);
			break;

		case ET_TUBE:
			draw_tube((TubeExtent *) extent, com, R);
			break;

		case ET_BOX:
			draw_box((BoxExtent *) extent, com, R);
			break;

		case ET_CONVEX_MESH:
			draw_mesh((ConvexMeshExtent *) extent, com, R);
			break;
	}
}
