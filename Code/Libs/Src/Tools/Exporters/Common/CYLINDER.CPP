#include <stdio.h>
#include <stdlib.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "sgi_utf.h"
#include "vector.h"
#include "cylinder.h"

void ComputeCylinder(Cylinder *cylinder, const Vector *v, int count)
{

  cylinder->cyl_tr.identity();

  int i;
  float min_x, max_x;
  float min_y, max_y;
  float min_z, max_z;
  float d_x, d_y, d_z;
  
  float *xy, *xz, *yz;

  xy=(float*)Malloc(2*count*sizeof(float));
  xz=(float*)Malloc(2*count*sizeof(float));
  yz=(float*)Malloc(2*count*sizeof(float));

  min_x=max_x=v[0].x;
  min_y=max_y=v[0].y;
  min_z=max_z=v[0].z;
  for(i=0; i<count; i++)
  {
    if(v[i].x<min_x)
      min_x=v[i].x;
    if(v[i].x>max_x)
      max_x=v[i].x;
    if(v[i].y<min_y)
      min_y=v[i].y;
    if(v[i].y>max_y)
      max_y=v[i].y;
    if(v[i].z<min_z)
      min_z=v[i].z;
    if(v[i].z>max_z)
      max_z=v[i].z;

    xy[2*i]=v[i].x;
    xy[2*i+1]=v[i].y;
    xz[2*i]=v[i].x;
    xz[2*i+1]=v[i].z;
    yz[2*i]=v[i].y;
    yz[2*i+1]=v[i].z;
  }
  d_x=max_x-min_x;
  d_y=max_y-min_y;
  d_z=max_z-min_z;

  Circle circle_xy, circle_xz, circle_yz;

  // calc 3 circles
  ComputeCircle(&circle_xy, xy, count);
  ComputeCircle(&circle_xz, xz, count);
  ComputeCircle(&circle_yz, yz, count);


  // fix for flat objects
  if(d_x < 0.00001f)
  {
	  d_y = .001f * circle_yz.r;
  }

  if(d_y < 0.00001f)
  {
	  d_y = .001f * circle_xz.r;
  }

  if(d_z < 0.00001f)
  {
	  d_z = .001f * circle_xy.r;
  }


  // see which of 3 cylinders has smallest volume
  float vol_x, vol_y, vol_z;

  vol_x = (float)M_PI * circle_yz.r*circle_yz.r * d_x;
  vol_y = (float)M_PI * circle_xz.r*circle_xz.r * d_y;
  vol_z = (float)M_PI * circle_xy.r*circle_xy.r * d_z;
  //printf("vol_x vol_y vol_z %f %f %f\n",vol_x,vol_y,vol_z);

  if((vol_x < vol_y) && (vol_x < vol_z)) // along x
  {
	  cylinder->cyl_tr.m = PersistMatrix::rotate_y((float)( (M_PI/180.0) * -90.0));
    // protect against 0 volume cylinder
    if(min_x == max_x){
      min_x -= .005f * circle_yz.r;
      max_x += .005f * circle_yz.r;
      d_x = max_x - min_x;
      vol_x = (float)M_PI * circle_yz.r*circle_yz.r * d_x;
    }
    cylinder->cyl_volume = vol_x;
    cylinder->cyl_r = circle_yz.r;
    cylinder->p1.x = min_x;
    cylinder->p1.y = circle_yz.x;
    cylinder->p1.z = circle_yz.y;
 
    cylinder->p2.x = max_x;
    cylinder->p2.y = circle_yz.x;
    cylinder->p2.z = circle_yz.y;
  }else
  if((vol_y < vol_x) && (vol_y < vol_z)) // along y
  {
	cylinder->cyl_tr.m = PersistMatrix::rotate_x((float)( (M_PI/180.0) * 90.0));
    if(min_y == max_y){ // safety precaution
      min_y -= .005f * circle_xz.r;
      max_y += .005f * circle_xz.r;
      d_y = max_y - min_y;
      vol_y = (float)M_PI * circle_xz.r*circle_xz.r * d_y;
    }
    cylinder->cyl_volume = vol_y;
    cylinder->cyl_r = circle_xz.r;
    cylinder->p1.x = circle_xz.x;
    cylinder->p1.y = min_y;
    cylinder->p1.z = circle_xz.y;
 
    cylinder->p2.x = circle_xz.x;
    cylinder->p2.y = max_y;
    cylinder->p2.z = circle_xz.y;
  }
  else{ // vol_z // along z
    if(min_z == max_z){ // safety precaution
      min_z -= .005f * circle_xy.r;
      max_z += .005f * circle_xy.r;
      d_z = max_z - min_z;
      vol_z = (float)M_PI * circle_xy.r*circle_xy.r * d_z;
    }
    cylinder->cyl_volume = vol_z;
    cylinder->cyl_r = circle_xy.r;
    cylinder->p1.x = circle_xy.x;
    cylinder->p1.y = circle_xy.y;
    cylinder->p1.z = min_z;
 
    cylinder->p2.x = circle_xy.x;
    cylinder->p2.y = circle_xy.y;
    cylinder->p2.z = max_z;
  }

  Free(xy);
  Free(xz);
  Free(yz);

  cylinder->cyl_length = (cylinder->p1 - cylinder->p2).magnitude();
  
  ComputeTube(*cylinder, v, count);
}

void ComputeTube(Cylinder& cyl, const Vector *v, int count)
{
	// x, y, or z oriented
	cyl.tub_tr = cyl.cyl_tr;
	cyl.tub_r = cyl.cyl_r;

	int i;
	Vector p1_current = cyl.p1;
	Vector p2_current = cyl.p2;
	const float radius = cyl.tub_r;
	Vector direction = p2_current - p1_current;
	direction.normalize();

	float length = (p2_current - p1_current).magnitude();
	
	float max_shift = __min(radius, length);

	Vector p1_min = p1_current + max_shift * direction;
	Vector p1_max = p1_current;

	if(max_shift > 0.0f)
	{
		for(i=0; i < 15; i++)
		{
			p1_current = .5f * (p1_min + p1_max);
			Vector center = .5f * (p1_current + p2_current);

			length = (p2_current - p1_current).magnitude(); 

			Vector p1 = -0.5f * length * direction;
			Vector p2 =  0.5f * length * direction;
		
			bool fits = true;
			for(int j=0; j < count; j++)
			{
				Vector vp(v[j] - center);

				Vector vvp = vp - p1;

				float dot = vvp.x * direction.x + vvp.y * direction.y + vvp.z * direction.z;

				Vector dv = dot * direction;
				
				// projected point onto line
				Vector vpp = p1 + dv;
				
				Vector dvp1 = p1 - vpp;
				Vector dvp2 = p2 - vpp;
				
				float d1 = dvp1.magnitude();
				float d2 = dvp2.magnitude();

				// see if we have to clamp against endpoints
				if( (d1 + d2) > 1.000001f * length) // 1.000001 is to avoid numerical imprecission
				{
					if(d1 < d2)
					{
						vpp = p1;
					}
					else
					{
						vpp = p2;
					}
				}

				float dist_to_line = (vp - vpp).magnitude();
				if( dist_to_line > radius )
				{
					fits = false;
					break;
				}
			}

			if(fits)
			{
				p1_max = p1_current;
			}
			else
			{
				p1_min = p1_current;
			}
		}
	}


	length = (p2_current - p1_current).magnitude();
	max_shift = __min(radius, length);

	Vector p2_min = p2_current - max_shift * direction;
	Vector p2_max = p2_current;

	if(max_shift > 0.0f)
	{
		for(i=0; i < 15; i++)
		{
			p2_current = .5f * (p2_min + p2_max);
			Vector center = .5f * (p1_current + p2_current);

			length = (p2_current - p1_current).magnitude(); 

			Vector p1 = -0.5f * length * direction;
			Vector p2 =  0.5f * length * direction;
		
			bool fits = true;
			for(int j=0; j < count; j++)
			{
				Vector vp(v[j] - center);

				Vector vvp = vp - p1;

				float dot = vvp.x * direction.x + vvp.y * direction.y + vvp.z * direction.z;

				Vector dv = dot * direction;
				
				// projected point onto line
				Vector vpp = p1 + dv;
				
				Vector dvp1 = p1 - vpp;
				Vector dvp2 = p2 - vpp;
				
				float d1 = dvp1.magnitude();
				float d2 = dvp2.magnitude();

				// see if we have to clamp against endpoints
				if( (d1 + d2) > 1.000001f * length) // 1.000001 is to avoid numerical imprecission
				{
					if(d1 < d2)
					{
						vpp = p1;
					}
					else
					{
						vpp = p2;
					}
				}

				float dist_to_line = (vp - vpp).magnitude();
				if( dist_to_line > radius )
				{
					fits = false;
					break;
				}
			}

			if(fits)
			{
				p2_max = p2_current;
			}
			else
			{
				p2_min = p2_current;
			}
		}
	}

	cyl.tub_p1 = p1_current;
	cyl.tub_p2 = p2_current;
	cyl.tub_length = (cyl.tub_p2 - cyl.tub_p1).magnitude();

	cyl.tub_volume = (float)M_PI * cyl.tub_r * cyl.tub_r * cyl.tub_length +
					 (4.0f/3.0f) * (float)M_PI * cyl.tub_r * cyl.tub_r * cyl.tub_r;
}
  
void ComputeCircle(Circle *circle, const float *xy, int count)
{
  int i,j;
  float dx, dy;
  float r;
  float cx, cy;
  float max_dist;

  // find the two most distant points
  dx = xy[2*0+0] - xy[2*1+0];
  dy = xy[2*0+1] - xy[2*1+1];
  max_dist = dx*dx + dy*dy;

  cx = (xy[2*0+0] + xy[2*1+0]) / 2.0f;
  cy = (xy[2*0+1] + xy[2*1+1]) / 2.0f;
  r = (float)sqrt(max_dist) / 2.0f;
  for(j=0; j<count; j++){
    for(i=j; i<count; i++){
      dx = xy[2*j+0] - xy[2*i+0];
      dy = xy[2*j+1] - xy[2*i+1];
      if((dx*dx + dy*dy) > max_dist){
        max_dist = dx*dx + dy*dy;
        cx = (xy[2*j+0] + xy[2*i+0]) / 2.0f;
        cy = (xy[2*j+1] + xy[2*i+1]) / 2.0f;
        r = (float)sqrt(max_dist) / 2.0f;
      }

    }
  }

  //printf("cx cy r %f %f %f\n",cx,cy,r);

  // update circle
  float old_to_p, old_to_new;
  float x,y;
  for(i=0; i<count; i++){

    x = xy[2*i];
    y = xy[2*i+1];

    dx = x - cx;
    dy = y - cy;

    old_to_p = (float)sqrt(dx*dx + dy*dy);

    if(old_to_p > r){

      // save old
	  float min_r = r;
	  float min_cx = cx;
	  float min_cy = cy;

	  // update
      r  = (r + old_to_p)/2.0f;

      old_to_new = old_to_p - r;

      cx = (r*cx + old_to_new*x)/old_to_p;
      cy = (r*cy + old_to_new*y)/old_to_p;

	  // save new
	  float max_r = r;
	  float max_cx = cx;
	  float max_cy = cy;

	  // move min center past new cener
	  min_cx = cx + (cx - min_cx);
	  min_cy = cy + (cy - min_cy);

	  // try to back up
	  for(j=0; j<15; j++)
	  {
		  float tmp_r = .5f * (min_r + max_r);
		  float tmp_cx = .5f * (min_cx + max_cx);
		  float tmp_cy = .5f * (min_cy + max_cy);

		  int min_flag = 0;

		  for(int k=0; k<i; k++)
		  {
			  float tmp_dx = xy[2*k] - tmp_cx;
			  float tmp_dy = xy[2*k+1] - tmp_cy;
			  float tmp_old_to_p = (float)sqrt(tmp_dx*tmp_dx + tmp_dy*tmp_dy);

			  if(tmp_old_to_p > tmp_r)
			  {
			  	  min_cx = tmp_cx;
				  min_cy = tmp_cy;
				  min_r = tmp_r;
				  min_flag = 1;
				  break;
			  }
		
			  if(!min_flag)
			  {
			  	  max_cx = tmp_cx;
				  max_cy = tmp_cy;
				  max_r = tmp_r;
			  }
		  }
	  }

    }
  }

  //printf("cx cy r %f %f %f\n",cx,cy,r);

  circle->x = cx;
  circle->y = cy;

  // pad to avoid numerical accuracy problems
  circle->r = 1.00001f * r;
}
