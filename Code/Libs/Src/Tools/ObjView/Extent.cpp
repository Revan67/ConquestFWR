//
// Extent.cpp
//
//
//

#ifdef DRAWEXTENTS

#include <string.h>
#include <float.h>
#include <windows.h>

//

#include "TSmartPointer.h"	
#include "extent.h"
#include "geom.h"
#include "physics.h"
#include "BaseCam.h"
#include "RPUL.h"

//

#include "CmpndView.h"

//

#define FULL		0x00010000
#define TOP_HALF	0x00020000
#define BOTTOM_HALF	0x00040000

//

void RenderExtents( unsigned long extent_depth );
const BaseExtent* GetExtent(const BaseExtent *extent, ExtentType type);
bool DrawExtents(const BaseExtent *extent, int current_level, int target_level, const Transform & t, float mass);
void DrawBox(const Transform & te, const Box& render_box, int mode, float mass);
void DrawCylinder(const Transform & te, const Cylinder& cylinder, int mode);
void DrawTube(const Transform & te, const Tube& tube, int mode);
void DrawSphere(const Transform & te, const Vector& offset, float radius, int mode, unsigned char color[3], bool depth_tested );
void DrawHull(const Transform & te, const CollisionMesh& cmesh, int mode);

//

static unsigned long extent_depth = 0;

//

void RenderExtents( unsigned long depth )
{
	if( (extent_depth = depth) == 0 ) {
		return;
	}

	RenderPrim->set_render_state( D3DRS_LIGHTING, FALSE );
	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	if( (extent_depth & ~HIGH_BIT) == 1 ) {
		unsigned char color[3] = {255, 255, 255};
		float radius;
		Vector center;

		Engine->get_instance_bounding_sphere( LoadedInstIndices[0], 0, &radius, &center );
		DrawSphere( Engine->get_transform(LoadedInstIndices[0]), center, radius, D3DFILL_WIREFRAME, color, true );
		
		return;
	}
	else {
		COMPTR<IPhysics> physics;
		const BaseExtent *top_extent;
		float mass = 0.0f;
		bool drew_any = false;

		if( Engine->QueryInterface (IID_IPhysics, physics) == GR_OK ) {
			// draw wire frame first
			// draw main extent last
			//
			for( int i = LoadedInstCount-1; i >= 0; i-- )  {

				INSTANCE_INDEX idx = LoadedInstIndices[i];

				if( physics->get_extent(&top_extent, idx) == true ) {

					Transform t( Engine->get_orientation(idx), physics->get_center_of_mass(idx) );

					drew_any |= DrawExtents( top_extent, 1, (extent_depth & ~HIGH_BIT) - 1, t, physics->get_mass(idx) );
				}

				mass += physics->get_mass(idx);
			}
		}

		if( drew_any ) {
			SetRender2D();
			Font.RenderFormattedString( 0, 60, "Mass %.2f", mass );
		}
		else {
			extent_depth = 0;
		}
	}
}

//

void DrawHull(const Transform & te, const CollisionMesh& cmesh, int mode)
{
	int i;
	int v_id1, v_id2, v_id3;

	//Matrix m = t.get_transpose();

	SetRenderVolume(&te);

	RenderPrim->set_texture_stage_texture( 0, 0 );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	
	// wire frame
	if(mode == D3DFILL_WIREFRAME )
	{
		RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );

		pb.Color4ub( 0, 255, 0, 0 );

		pb.Begin( PB_LINES );
		for(i=0; i<cmesh.num_edges; i++)
		{
			v_id1 = cmesh.edges[i].v[0];
			v_id2 = cmesh.edges[i].v[1];

			pb.Vertex3f( cmesh.vertices[ v_id1 ].p.x,
						 cmesh.vertices[ v_id1 ].p.y,
						 cmesh.vertices[ v_id1 ].p.z);
			pb.Vertex3f( cmesh.vertices[ v_id2 ].p.x,
						 cmesh.vertices[ v_id2 ].p.y,
						 cmesh.vertices[ v_id2 ].p.z);
		}
		pb.End();
	}
	else
	if(mode == D3DFILL_SOLID)  // faces
	{
		RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		RenderPrim->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		RenderPrim->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );
		RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
		RenderPrim->set_render_state( D3DRS_CULLMODE, D3DCULL_CW );

		pb.Color4ub( 0, 255, 0, 48 );

		// draw transparent box CCW
		pb.Begin(PB_TRIANGLES); 
		for(i=0; i<cmesh.num_triangles; i++)
		{
			v_id1 = cmesh.triangles[i].v[0];
			v_id2 = cmesh.triangles[i].v[1];
			v_id3 = cmesh.triangles[i].v[2];

			pb.Vertex3f(cmesh.vertices[ v_id1 ].p.x,
						cmesh.vertices[ v_id1 ].p.y,
						cmesh.vertices[ v_id1 ].p.z);
			pb.Vertex3f(cmesh.vertices[ v_id2 ].p.x,
						cmesh.vertices[ v_id2 ].p.y,
						cmesh.vertices[ v_id2 ].p.z);
			pb.Vertex3f(cmesh.vertices[ v_id3 ].p.x,
						cmesh.vertices[ v_id3 ].p.y,
						cmesh.vertices[ v_id3 ].p.z);
			
		}
		pb.End();

		RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, TRUE );
		RenderPrim->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
	}
}

//

void DrawSphere(const Transform & te, const Vector& center, float radius, int mode, unsigned char color[3], bool depth_tested )
{
	int slices = 12;
	int stacks = 8;

	float x, y, z;
	int i, j;
   
	float cx = center.x;
	float cy = center.y;
	float cz = center.z;

	static int last_slices = 0;
	static int last_stacks = 0;

	static int max_slices = 0;
	static int max_stacks = 0;

	static float *  cos_theta = NULL;
	static float *  sin_theta = NULL;
	static float *  cos_rho = NULL;
	static float *  sin_rho = NULL;

	if(slices > max_slices)
	{
		max_slices = slices;
		cos_theta = (float*)realloc(cos_theta, (max_slices + 1) * sizeof(float));
		sin_theta = (float*)realloc(sin_theta, (max_slices + 1) * sizeof(float));
	}
	if(stacks > max_stacks)
	{
		max_stacks = stacks;
		cos_rho = (float*)realloc(cos_rho, (max_stacks + 1) * sizeof(float));
		sin_rho = (float*)realloc(sin_rho, (max_stacks + 1) * sizeof(float));
	}

	if(slices != last_slices)
	{
		last_slices = slices;
		float *ct = cos_theta;
		float *st = sin_theta;
		float theta = 0.0f;
		float dtheta = 2.0f * M_PI / (float) slices;
		for(int j=0; j <= last_slices; j++)
		{
			*ct++ = (float)cos(theta);
			*st++ = (float)sin(theta);
			theta += dtheta;
		}
	}

	if(stacks != last_stacks)
	{
		last_stacks = stacks;
		float *ct = cos_rho;
		float *st = sin_rho;
		float rho = 0.0f;
		float drho = M_PI / (float) stacks;
		for(int j=0; j <= last_stacks; j++)
		{
			*ct++ = (float)cos(rho);
			*st++ = (float)sin(rho);
			rho += drho;
		}
	}

	if( (mode & 0x0000ffff) == D3DFILL_WIREFRAME )
	{
      SetRenderVolume(&te);

      RenderPrim->set_texture_stage_texture( 0, 0 );
      RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
      RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
      RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
      RenderPrim->set_render_state( D3DRS_ZENABLE, depth_tested );

	  pb.Color4ub(color[0], color[1], color[2], 0);

	  int stacks_start_offset = 0;
	  int stacks_end_offset = 0;
	  if(mode & TOP_HALF)
	  {
		stacks_start_offset = 0;
		stacks_end_offset = stacks / 2;
	  }
	  else
	  if(mode & BOTTOM_HALF)
	  {
		stacks_start_offset = stacks / 2;
		stacks_end_offset = 0;
	  }

	  /* draw stack lines */
	  for (i = 1 + stacks_start_offset; i < stacks - stacks_end_offset; i++)
	  {
		 float srr = sin_rho[i] * radius;
		 z = cos_rho[i] * radius + cz;
		 pb.Begin( PB_LINE_STRIP );
		 for (j = 0; j <= slices; j++) 
		 {
			x = srr * cos_theta[j] + cx;
			y = srr * sin_theta[j] + cy;
			pb.Vertex3f( x, y, z );
		 }
		 pb.End();
	  }

	  /* draw slice lines */
	  for (j = 0; j < slices; j++) 
	  {
		 float ctr = cos_theta[j] * radius;
		 float str = sin_theta[j] * radius;
		 pb.Begin( PB_LINE_STRIP );
		 for (i = stacks_start_offset; i <= stacks - stacks_end_offset; i++) 
		 {
			x = sin_rho[i] * ctr + cx;
			y = sin_rho[i] * str + cy;
			z = cos_rho[i] * radius + cz;
			pb.Vertex3f( x, y, z);
		 }
		 pb.End();
	  }
	 
	  if(extent_depth & HIGH_BIT)
	  {
		if (TheCamera->point_to_screen(x, y, z, te.translation))
		{
			SetRender2D();
			Font.RenderFormattedString( x, y, "Radius: %.2f", radius);
		}
	  }

	}
}

//

void DrawTube(const Transform & te, const Tube& tube, int mode)
{
	int slices = 12;
	int stacks = 6;

	double baseRadius = tube.radius;
	double topRadius = tube.radius;
	double height = tube.length;

	double da, r, dr, dz;
	float x, y, z, nz; // nsign;
	int i, j;

	da = 2.0*M_PI / slices;
	dr = (topRadius - baseRadius) / stacks;
	dz = height / stacks;
	nz = (baseRadius - topRadius) / height;  /* Z component of normal vectors */

	SetRenderVolume(&te);

	RenderPrim->set_texture_stage_texture( 0, 0 );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
	RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );

	pb.Color4ub(255, 0, 255, 0);

   if(mode == D3DFILL_WIREFRAME) 
   {
      /* Draw rings */
	 z = -height/2.0;
	 r = baseRadius;
	 for (j=0; j <= stacks; j++)
	 {
	    pb.Begin( PB_LINE_LOOP );
	    for (i=0; i < slices; i++)
		{
	       x = cos(i*da);
	       y = sin(i*da);
		   pb.Vertex3f(x*r, y*r, z);
	    }
	    pb.End();
	    z += dz;
	    r += dr;
	 }

      /* draw length lines */
      pb.Begin( PB_LINES );
      for (i=0; i < slices; i++) 
	  {
		 x = cos(i*da);
		 y = sin(i*da);
		 pb.Vertex3f(x*baseRadius, y*baseRadius, .5f * -height);
		 pb.Vertex3f(x*topRadius,  y*topRadius,   .5f * height);
      }
      pb.End();
   }

   unsigned char color[3] = {255, 0, 255};
   DrawSphere(te, Vector(0, 0,  tube.length/2.0), tube.radius, mode | TOP_HALF, color, true );
   DrawSphere(te, Vector(0, 0, -tube.length/2.0), tube.radius, mode | BOTTOM_HALF, color, true );
}

//

void DrawCylinder(const Transform & te, const Cylinder& cylinder, int mode)
{
	int slices = 8;
	int stacks = 4;

	double baseRadius = cylinder.radius;
	double topRadius = cylinder.radius;
	double height = cylinder.length;

	double da, r, dr, dz;
	float x, y, z, nz; // nsign;
	int i, j;

	da = 2.0*M_PI / slices;
	dr = (topRadius - baseRadius) / stacks;
	dz = height / stacks;
	nz = (baseRadius - topRadius) / height;  /* Z component of normal vectors */

	SetRenderVolume(&te);

	RenderPrim->set_texture_stage_texture( 0, 0 );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );
	RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );

	pb.Color4ub(255, 255, 0, 0);

   if(mode == D3DFILL_WIREFRAME) 
   {
      /* Draw rings */
	 z = -height/2.0;//0.0;
	 r = baseRadius;
	 for (j=0; j <= stacks; j++)
	 {
	    pb.Begin( PB_LINE_LOOP );
	    for (i=0; i < slices; i++)
		{
	       x = cos(i*da);
	       y = sin(i*da);
		   pb.Vertex3f(x*r, y*r, z);
	    }
	    pb.End();
	    z += dz;
	    r += dr;
	 }
      /* draw length lines */
      pb.Begin( PB_LINES );
      for (i=0;i<slices;i++) 
	  {
		 x = cos(i*da);
		 y = sin(i*da);
		 pb.Vertex3f(x*baseRadius, y*baseRadius, -height/2.0);
		 pb.Vertex3f(x*topRadius,  y*topRadius,   height/2.0);
      }
      pb.End();
   }
}

//

void DrawBox(const Transform & te, const Box& box, int mode, float mass)
{
		Vector p0( -box.half_x, -box.half_y,  +box.half_z);
		Vector p1( +box.half_x, -box.half_y,  +box.half_z);
		Vector p2( +box.half_x, +box.half_y,  +box.half_z);
		Vector p3( -box.half_x, +box.half_y,  +box.half_z);

		Vector p4( -box.half_x,  -box.half_y, -box.half_z);
		Vector p5( +box.half_x,  -box.half_y, -box.half_z);
		Vector p6( +box.half_x,  +box.half_y, -box.half_z);
		Vector p7( -box.half_x,  +box.half_y, -box.half_z);
		
		SetRenderVolume(&te);

		RenderPrim->set_texture_stage_texture( 0, 0 );
		RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
		RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );

		if(mode == D3DFILL_WIREFRAME)
		{
			RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
		
			pb.Color4ub(0, 0, 255, 0);

			pb.Begin(PB_LINE_STRIP);
				// top
				pb.Vertex3f(p3.x, p3.y, p3.z);
				pb.Vertex3f(p2.x, p2.y, p2.z);
				pb.Vertex3f(p6.x, p6.y, p6.z);
				pb.Vertex3f(p7.x, p7.y, p7.z);
				pb.Vertex3f(p3.x, p3.y, p3.z);
			pb.End();
			
			pb.Begin(PB_LINE_STRIP);
				// bottom
				pb.Vertex3f(p0.x, p0.y, p0.z);
				pb.Vertex3f(p4.x, p4.y, p4.z);
				pb.Vertex3f(p5.x, p5.y, p5.z);
				pb.Vertex3f(p1.x, p1.y, p1.z);
				pb.Vertex3f(p0.x, p0.y, p0.z);
			pb.End();
			
			// sides
			pb.Begin(PB_LINES);
				pb.Vertex3f(p0.x, p0.y, p0.z);
				pb.Vertex3f(p3.x, p3.y, p3.z);

				pb.Vertex3f(p1.x, p1.y, p1.z);
				pb.Vertex3f(p2.x, p2.y, p2.z);

				pb.Vertex3f(p4.x, p4.y, p4.z);
				pb.Vertex3f(p7.x, p7.y, p7.z);

				pb.Vertex3f(p5.x, p5.y, p5.z);
				pb.Vertex3f(p6.x, p6.y, p6.z);
			pb.End();

			// draw dimensions and mass
			if(extent_depth & HIGH_BIT){
				float x,y,z;
				if (TheCamera->point_to_screen(x, y, z, te.translation))
				{
					SetRender2D();
					Font.RenderFormattedString( x, y, "X: %.2f, Y: %.2f, Z: %.2f",2*box.half_x, 2*box.half_y, 2*box.half_z);
					Font.RenderFormattedString( x, y+20, "Mass: %.2f", mass);
				}
			}
		}
		else
		if(mode == D3DFILL_SOLID)
		{
			RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			RenderPrim->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			RenderPrim->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
			RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
			RenderPrim->set_render_state( D3DRS_CULLMODE, D3DCULL_CW );

			pb.Color4ub(0,0,255,48);

			// draw transparent box CCW
			pb.Begin(PB_QUADS); 

			// top
			pb.Vertex3f(p3.x, p3.y, p3.z);
			pb.Vertex3f(p2.x, p2.y, p2.z);
			pb.Vertex3f(p6.x, p6.y, p6.z);
			pb.Vertex3f(p7.x, p7.y, p7.z);
			
			// bottom
			pb.Vertex3f(p0.x, p0.y, p0.z);
			pb.Vertex3f(p4.x, p4.y, p4.z);
			pb.Vertex3f(p5.x, p5.y, p5.z);
			pb.Vertex3f(p1.x, p1.y, p1.z);
			
			// front
			pb.Vertex3f(p0.x, p0.y, p0.z);
			pb.Vertex3f(p1.x, p1.y, p1.z);
			pb.Vertex3f(p2.x, p2.y, p2.z);
			pb.Vertex3f(p3.x, p3.y, p3.z);

			// back
			pb.Vertex3f(p4.x, p4.y, p4.z);
			pb.Vertex3f(p7.x, p7.y, p7.z);
			pb.Vertex3f(p6.x, p6.y, p6.z);
			pb.Vertex3f(p5.x, p5.y, p5.z);

			// right
			pb.Vertex3f(p1.x, p1.y, p1.z);
			pb.Vertex3f(p5.x, p5.y, p5.z);
			pb.Vertex3f(p6.x, p6.y, p6.z);
			pb.Vertex3f(p2.x, p2.y, p2.z);

			// left
			pb.Vertex3f(p0.x, p0.y, p0.z);
			pb.Vertex3f(p3.x, p3.y, p3.z);
			pb.Vertex3f(p7.x, p7.y, p7.z);
			pb.Vertex3f(p4.x, p4.y, p4.z);

			pb.End();

			RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, TRUE );
		}
}

//

bool DrawExtents(const BaseExtent *extent, int current_level, int target_level, const Transform& t, float mass)
{
	if(extent == NULL) return false;

	bool result = false;

	if(current_level == target_level)
	{
		Transform te = t * extent->xform;
	
		switch(extent->type)
		{
			case ET_LINE_SEGMENT:
			case ET_INFINITE_PLANE:
			case ET_NONE:
				{
					break;
				}
			case ET_SPHERE:
				{
					const Sphere *gp = (Sphere*)extent->get_primitive();

					unsigned char color[3] = {255, 0, 0};
					DrawSphere(te, Vector(0,0,0), gp->radius, D3DFILL_WIREFRAME | FULL, color, true );
					break;
				}
			case ET_BOX:
				{
					const Box *box = (Box*)extent->get_primitive();
					DrawBox(te, *box, D3DFILL_WIREFRAME, mass);
					DrawBox(te, *box, D3DFILL_SOLID, mass);
					break;
				}
			case ET_TUBE:
				{
					const Tube *tube = (Tube*)extent->get_primitive();
					DrawTube(te, *tube, D3DFILL_WIREFRAME);
					break;
				}
			case ET_CYLINDER:
				{
					const Cylinder *cylinder = (Cylinder*)extent->get_primitive();
					DrawCylinder(te, *cylinder, D3DFILL_WIREFRAME);
					break;
				}
			case ET_CONVEX_MESH:
			case ET_GENERAL_MESH:
				{
					const CollisionMesh *cmesh = (CollisionMesh*)extent->get_primitive();
					DrawHull(te, *cmesh, D3DFILL_WIREFRAME);
					DrawHull(te, *cmesh, D3DFILL_SOLID);
					break;
				}
			default:
				GENERAL_WARNING( "Unknown joint type!\n" );
		}

		result = true;
	}

	result |= DrawExtents(extent->next, current_level, target_level, t, mass);

	if(current_level < target_level)
	{
		result |= DrawExtents(extent->child, current_level + 1, target_level, t, mass);
	}

	return result;
}

//

const BaseExtent* GetExtent(const BaseExtent *extent, ExtentType type)
{
	if(extent == NULL) return NULL;

	if(extent->type == type)
	{
		return extent;
	}

	// depth first
	const BaseExtent *child = GetExtent(extent->child, type);
	if(child)
	{
		return child;
	}
	
	return GetExtent(extent->next, type);
}

//

#endif // EOF