//
//	RFXLIGHT.CPP - Light map interface
//
//	(C) 1997 Digital Anvil
//
//  asega march 1997
//

// THE BIG PROBLEM LIST

	// PROBLEMS:
	// - excessive bind texture calls
	// - reprojecting all points
	// - not good to keep turning states on and off!
	// - need to turn vertex lights off on object
	// - clipping inefficiency
	// - no direct access to light structures!

	// SOLUTIONS:
	// - INTEGRATE WITH OBJECT RENDERER

#include "renderfx.h"

#include "tsmartpointer.h"
#include "display.h"
#include "engine.h"
#include "3dmath.h"
#include "ITXMLib.h"
#include "basecam.h"
#include "lightman.h"
#include "basemesh.h"
#include "renderer.h"


struct ClipVertex
{
	Vector	world;
	S32		texture_index;
	SINGLE	u, v;
	SINGLE	r, g, b;
};

void clip_polygon(S32 vertex_count, ClipVertex *vertex_list_in, S32 *result_count, S32 x0, S32 y0, S32 x1, S32 y1);

// assumes:
//			- lightmap texture already bound and ready to go

BOOL32 RFX_render_instance_lightmap(IEngine *ENGINE, INSTANCE_INDEX camera_index, INSTANCE_INDEX index, U32 light_count, RFX_LIGHT *light_list)
{
	COMPTR <IRenderer>		RENDER;
	
	if (ENGINE->QueryInterface ("IRenderer", RENDER) != GR_OK) return FALSE;
	
	BaseMesh				*b;
	ARCHETYPE_INDEX			arch		= ENGINE->get_archetype(index);
	Vector					position	= ENGINE->get_position(index);
	Matrix					orientation = ENGINE->get_orientation(index);
	
	static S32				vcount;
	static Vector			vlist[16];
	static Vector			normal;

	static Vector			*vptr;

	if (arch == INVALID_ARCHETYPE_INDEX) return FALSE;

	if (!RENDER->get_statistics((int &) b, arch, 0, ST_MESH_POINTER))
	{
		ENGINE->release_archetype(arch);
		return FALSE;
	}

	for (int i = 0; i < b->face_cnt; i++)
	{
		vcount = b->face_num_vertices[i];

		for (int j = 0; j < vcount; j++)
		{
			vptr = b->object_vertex_list + (b->object_vertex_chain[b->face_vertices[i]]);
			vlist[j] = position + (orientation * (*vptr));
		}
		
		normal = *(b->normal_ABC + b->face_normal[i]);
		normal = orientation * normal;
		RFX_render_polygon_lightmap(vcount, vlist, &normal, light_count, light_list);
	}

	ENGINE->release_archetype(arch);

	return TRUE;
}

BOOL32 RFX_render_polygon_lightmap(S32 vcount, Vector *vlist, Vector *normal, U32 light_count, RFX_LIGHT *light_list)
{
	#define LIGHT_BASE_ALPHA	0.5
	#define DIST_3D(a, b)		sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));

	static Vector		vert_points[4];					// Vertex points of polygon
	static Vector		proj_points[4];					// Vertex points in light's coordinate frame
	
	static ClipVertex	clip_list[16];					// Resultant vertices after clipping
	static S32			clip_result;					// Resultant vertex count

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	
	SINGLE	one_over_range;								// 1 / (spotlight range)
	SINGLE  half_range;									// 1/2 spotlight range
	Vector	l2v;
	Matrix	inv_rot;

	for (U32 i = 0; i < light_count; i++)
	{
		RFX_LIGHT *current_spotlight = light_list + i;
		
		SINGLE nr = (SINGLE) current_spotlight->r / 256.0;
		SINGLE ng = (SINGLE) current_spotlight->g / 256.0;
		SINGLE nb = (SINGLE) current_spotlight->b / 256.0;

		one_over_range = 1.0 / (SINGLE) current_spotlight->range;
		half_range = current_spotlight->range * 0.5;

		if (current_spotlight != NULL)
		{
			// Perform render:

			// Calculate texture map application for each face:
			// We should probably optimize this for some point for static lights, since we do a lot of math
			// in this loop.
		
			// Find face normal:
			Vector plane_ABC = *normal;

			// Figure out plane equation of face in world space:
			SINGLE plane_D = -dot_product(*vlist, plane_ABC);
			
			// Center of light map is closest point on plane to light position:
			// (inefficient? lots of muls)
			SINGLE t = -dot_product(current_spotlight->position, plane_ABC) + plane_D;

			// back face if dt < 0
			if (t < 0.0)
			{
				// t intercept
				// dot_product(x, x) == 1 if x is normalized!
				
				// Find center of plane:
				Vector plane_center = current_spotlight->position + (plane_ABC * t);

				// Each vertex now maps onto the light map plane:
				Vector tex_vector;
				Vector *pt;

				for (int j = 0; j < vcount; j++)
				{
					pt = vlist + j;

					tex_vector = *pt - plane_center;
					tex_vector.normalize();

					// Scale by distance of point to spotlight to 0.0 - 1.0 range:
					Vector tv = *pt - current_spotlight->position;
					SINGLE mag = tv.magnitude();

					tex_vector *= mag / current_spotlight->range;

					tex_vector.x += 0.5;
					tex_vector.y += 0.5;
					
					clip_list[j].world			= vlist[j];			// clip in light space
					clip_list[j].u				= tex_vector.x;
					clip_list[j].v				= tex_vector.y;
					clip_list[j].r = nr; clip_list[j].g = ng; clip_list[j].b = nb;

				}

				// Clip polygon so that we only draw the portion (rectangular window) that projects onto the polygon to save time:
				clip_polygon(vcount, clip_list, &clip_result, -half_range + plane_center.x, -half_range + plane_center.y, half_range + plane_center.x, half_range + plane_center.y);

				// Tesselate and draw clipped polygon:

				glBegin(GL_TRIANGLES);
				
				for (j = 1; j < (clip_result - 1); j++)
				{
					glColor4f(clip_list[0].r, clip_list[0].g, clip_list[0].b, LIGHT_BASE_ALPHA);
					glTexCoord2f(clip_list[0].u, clip_list[0].v);
					glVertex3f(clip_list[0].world.x, clip_list[0].world.y, clip_list[0].world.z + 0.15);

					glColor4f(clip_list[j].r, clip_list[j].g, clip_list[j].b, LIGHT_BASE_ALPHA);
					glTexCoord2f(clip_list[j].u, clip_list[j].v);
					glVertex3f(clip_list[j].world.x, clip_list[j].world.y, clip_list[j].world.z + 0.15);

					glColor4f(clip_list[j + 1].r, clip_list[j + 1].g, clip_list[j + 1].b, LIGHT_BASE_ALPHA);
					glTexCoord2f(clip_list[j + 1].u, clip_list[j + 1].v);
					glVertex3f(clip_list[j + 1].world.x, clip_list[j + 1].world.y, clip_list[j + 1].world.z + 0.15);
				}

				glEnd();

			}

		}
	}

	glDepthMask(GL_TRUE);

	return FALSE;

}

// Polygon clipper:
// make sure vertex_list_in has enough space for final list

BOOL32 __fastcall cp_inside(ClipVertex *p, S32 x0, S32 y0, S32 x1, S32 y1, U32 side)
{
	if (side == 0)
		if (p->world.x > x0) return TRUE; else return FALSE;
	else 
	if (side == 1) 
		if (p->world.x < x1) return TRUE; else return FALSE;
	else
	if (side == 2)
		if (p->world.y > y0) return TRUE; else return FALSE;
	else
		if (p->world.y < y1) return TRUE; else return FALSE;
}

#define MAX_CP_VERTICES		16

void clip_polygon(S32 vertex_count, ClipVertex *vertex_list_in, S32 *result_count, S32 x0, S32 y0, S32 x1, S32 y1)
{
	#define CALC_SLOPES(q, iq, x1, y1, x2, y2)		q = (x1 - x2); if (q != 0.0) { q = (y1 - y2) / q; iq = 1.0 / q; } else { iq = 0.0; }
	
	SINGLE qslope, iqslope;			// vertex slope
	SINGLE c_factor;				// clip percentage 0.0 - 1.0

	static ClipVertex vertex_list_out[MAX_CP_VERTICES];

	ClipVertex *p, *s, *result;

	for (S32 side = 0; side < 4; side++)
	{
		// Compute resultant vertices:

		*result_count = 0;
		s = vertex_list_in + (vertex_count - 1);

		for (int i = 0; i < vertex_count; i++)
		{
			p = vertex_list_in + i;

			if (cp_inside(p, x0, y0, x1, y1, side))
			{
				if (cp_inside(s, x0, y0, x1, y1, side))
				{
					memcpy(&vertex_list_out[(*result_count)++], p, sizeof(ClipVertex));
				}
				else
				{
					// output i
					// macro-ize this -----------------------------
					result = &vertex_list_out[*result_count];
					
					CALC_SLOPES(qslope,  iqslope,  p->world.x, p->world.y, s->world.x, s->world.y);
					
					switch (side)
					{
						case 0:												// left
							result->world.x = x0;
							result->world.y = qslope * (x0 - p->world.x) + p->world.y;
							
							c_factor = (x0 - p->world.x) / (s->world.x - p->world.x);
						
							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 1:												// right
							result->world.x = x1;
							result->world.y = qslope * (x1 - p->world.x) + p->world.y;

							c_factor = (x1 - p->world.x) / (s->world.x - p->world.x);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 2:												// top
							result->world.x = p->world.x + (y0 - p->world.y) * iqslope;
							result->world.y = y0;

							c_factor = (y0 - p->world.y) / (s->world.y - p->world.y);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 3:												// bottom
							result->world.x = p->world.x + (y1 - p->world.y) * iqslope;
							result->world.y = y1;

							c_factor = (y1 - p->world.y) / (s->world.y - p->world.y);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;
					}

					(*result_count)++;

					// ------------------------------------------------

					// output p
					memcpy(&vertex_list_out[(*result_count)++], p, sizeof(ClipVertex));
										
				}
			}
			else
			{
				if (cp_inside(s, x0, y0, x1, y1, side))
				{
					// macro-ize this -----------------------------
					result = &vertex_list_out[*result_count];
	
					CALC_SLOPES(qslope, iqslope, p->world.x, p->world.y, s->world.x, s->world.y);
					
					switch (side)
					{
						case 0:												// left
							result->world.x = x0;
							result->world.y = qslope * (x0 - p->world.x) + p->world.y;
						
							c_factor = (x0 - p->world.x) / (s->world.x - p->world.x);
						
							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 1:												// right
							result->world.x = x1;
							result->world.y = qslope * (x1 - p->world.x) + p->world.y;

							c_factor = (x1 - p->world.x) / (s->world.x - p->world.x);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 2:												// top
							result->world.x = p->world.x + (y0 - p->world.y) * iqslope;
							result->world.y = y0;

							c_factor = (y0 - p->world.y) / (s->world.y - p->world.y);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;

						case 3:												// bottom
							result->world.x = p->world.x + (y1 - p->world.y) * iqslope;
							result->world.y = y1;

							c_factor = (y1 - p->world.y) / (s->world.y - p->world.y);

							result->world.z = p->world.z + (s->world.z - p->world.z) * c_factor;
							result->u = p->u + (s->u - p->u) * c_factor; result->v = p->v + (s->v - p->v) * c_factor;
							result->r = p->r + (s->r - p->r) * c_factor; result->g = p->g + (s->g - p->g) * c_factor;
							result->b = p->b + (s->b - p->b) * c_factor;

							break;
					}

					(*result_count)++;

					// ------------------------------------------------
				
				}
			}

			s = p;
		}

		// Copy 'new' vertices to old vertices
		for (i = 0; i < *result_count; i++)
		{
			memcpy(vertex_list_in + i, &vertex_list_out[i], sizeof(ClipVertex));
		}

		vertex_count = *result_count;

	}
	
}
