//
// <object.cpp>
//

#include "object.h"
#include "main.h"
#include "filesys.h"
#include "primitive.h"
#include "treebar.h"
#include "mesh.h"
#include "phyedit.h"


static	Vector		intersect, normal, clickpos, clickdir;
static	BaseExtent	*got;


//global set before extent selection
SINGLE		closestlen;

void Object::initialize(void)
{
    index = INVALID_INSTANCE_INDEX;
    read_only = FALSE;
    memset(name, 0, 256);
    memset(filename, 0, 256);
    mass = 0.0;
}

void	Object::reset(void)
{
	if(index != INVALID_INSTANCE_INDEX)
	{
		ENGINE->destroy_instance(index);
		index	=INVALID_INSTANCE_INDEX;
	}
	
	memset(name, 0, 256);
	memset(filename, 0, 256);
	Ibody.zero();
	center_of_mass.zero();

	mass	=0.0;
	tree	=NULL;
	mesh	=NULL;
}

void	mesh_render(BaseCamera *camera, INSTANCE_INDEX index, SINGLE r, SINGLE g, SINGLE b)
{
	Vector	p		=ENGINE->get_position(index);
	Matrix	R		=ENGINE->get_orientation(index);
	Mesh	*mesh	=RENDERER->get_instance_mesh(index, camera);
	Vector	vp[3];
	Vector	lvec(0.6, 0.3, 0.6);
	int		i;

	if(!mesh)	//some objects won't have meshes (nurbs)
	{
		return;
	}
	
	lvec.normalize();
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glDepthFunc(GL_LEQUAL);
	
	glBegin(GL_LINES);
	
	for(i=0;i < mesh->face_cnt;i++)
	{
		glColor4f(0.0, 0.0, 0.0, 1.0);
		
		vp[0]	=p + (R * *mesh->get_face_vertex(i, 0));
		vp[1]	=p + (R * *mesh->get_face_vertex(i, 1));
		vp[2]	=p + (R * *mesh->get_face_vertex(i, 2));
		
		glVertex3f(vp[0].x, vp[0].y, vp[0].z);
		glVertex3f(vp[1].x, vp[1].y, vp[1].z);
		
		glVertex3f(vp[1].x, vp[1].y, vp[1].z);
		glVertex3f(vp[2].x, vp[2].y, vp[2].z);
		
		glVertex3f(vp[2].x, vp[2].y, vp[2].z);
		glVertex3f(vp[0].x, vp[0].y, vp[0].z);
	}
	
	glEnd();
	
	glDepthFunc(GL_LEQUAL);
	
	glBegin(GL_TRIANGLES);
	
	for(i=0;i < mesh->face_cnt;i++)
	{
		SINGLE	dn	=dot_product(R * *mesh->get_face_normal(i), lvec);

		if(dn < 0.0)
		{
			dn	=0.0;
		}
		
		dn	=0.3 + (dn * 0.7);
		
		glColor4f(r * dn, g * dn, b * dn, 1.0);
		
		vp[0] = p + (R * *mesh->get_face_vertex(i, 0));
		vp[1] = p + (R * *mesh->get_face_vertex(i, 1));
		vp[2] = p + (R * *mesh->get_face_vertex(i, 2));
		
		glVertex3f(vp[0].x, vp[0].y, vp[0].z);
		glVertex3f(vp[1].x, vp[1].y, vp[1].z);
		glVertex3f(vp[2].x, vp[2].y, vp[2].z);
	}
	
	glEnd();
}

int	Object::GetNumMeshes(INSTANCE_INDEX idx)
{
	int	cnt	=0;
	
	if(idx != INVALID_INSTANCE_INDEX)
	{
		INSTANCE_INDEX	child	=INVALID_INSTANCE_INDEX;
		child	=ENGINE->get_instance_child_next(idx, EN_DONT_RECURSE, child);

		cnt++;

		while(child != INVALID_INSTANCE_INDEX)
		{
			cnt		+=GetNumMeshes(child);
			child	=ENGINE->get_instance_child_next(index, EN_DONT_RECURSE, child);
		}
	}

	return	cnt;
}

void	cmp_render(BaseCamera *camera, INSTANCE_INDEX root)
{
	mesh_render(camera, root, 0.4, 0.4, 0.5);

	INSTANCE_INDEX	child	=INVALID_INSTANCE_INDEX;
	child	=ENGINE->get_instance_child_next(root, EN_DONT_RECURSE, child);
	
	while(child != INVALID_INSTANCE_INDEX)
	{
		mesh_render(camera, child, 0.4, 0.4, 0.5);
		cmp_render(camera, child);
		child	=ENGINE->get_instance_child_next(root, EN_DONT_RECURSE, child);
	}
}

void	Object::render(BaseCamera *camera, BOOL32 render_ext)
{
//	glHint(GL_VOLUME_CLIPPING_HINT, GL_NICEST);
	
	glEnable(GL_TEXTURE_2D);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	
	physicsEditor.initialize_matrix();

	if(index != INVALID_INSTANCE_INDEX)
	{
		if(physicsEditor.flags & PEC_DRAW_TEXTURES)
		{
			ENGINE->render_instance(camera, index, EN_DONT_RECURSE, 1.0f, NULL, NULL);

			//need to reset this since render_instance goofs with things
			physicsEditor.initialize_matrix();
		}
		else
		{
			if(!physicsEditor.cmp_edit)
			{
				mesh_render(camera, index, 0.4, 0.4, 0.5);
			}
			else
			{
				cmp_render(camera, index);
			}
		}
		Vector	com	=PHYSICS->get_center_of_mass(index);
		
		glDepthFunc(GL_ALWAYS);
		
		glBegin(GL_POINTS);
		
		// center of mass
		glColor4f(1.0, 1.0, 0.0, 1.0);
		glVertex3f(com.x, com.y, com.z);
		
		glEnd();
	}	
}

void	recurse_render(BaseExtent *ptr, Vector & p, Matrix & o)
{
	BOOL32	skip	=FALSE;
	
	if(ptr)
	{
		if(physicsEditor.flags & PEC_COLLIDABLE_ONLY)
		{
			if(ptr->child)
			{
				skip	=TRUE;
			}
		}
		
		if(!skip)
		{
			switch(ptr->type)
			{
				case ET_SPHERE:
					draw_sphere((SphereExtent *) ptr, p, o);
					break;
				
				case ET_BOX:
					draw_box((BoxExtent *) ptr, p, o);
					break;
				
				case ET_CONVEX_MESH:
					draw_mesh((ConvexMeshExtent *) ptr, p, o);
					break;
			}
		}
		
		if(ptr->child)
		{
			recurse_render(ptr->child, p, o);
		}
		
		if(ptr->next)
		{
			recurse_render(ptr->next, p, o);
		}
	}
}

void	Object::render_extents(void)
{
	BaseExtent	*traverse	=tree;
	
	if(!traverse)
	{
		return;
	}
	
	Vector	position	=PHYSICS->get_center_of_mass(index);
	Matrix	orientation	=ENGINE->get_orientation(index);
	
	glEnable(GL_CULL_FACE);
//	glHint(GL_VOLUME_CLIPPING_HINT, GL_NICEST);
	
	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	recurse_render(traverse, position, orientation);
	glDisable(GL_CULL_FACE);
}

void	recurse_select(BaseExtent *ptr, Transform &T)
{
	if(ptr)
	{
		if(ptr->child)
		{
			recurse_select(ptr->child, T);
		}
		
		if(COLLISION->intersect_ray_with_extent(intersect, normal, clickpos, clickdir, *ptr, T))
        {
			intersect.subtract(clickpos);
			SINGLE	d	=intersect.magnitude();
			if(d < closestlen)
			{
				closestlen	=d;
				got			=ptr;
			}
        }
		
		if(ptr->next)
		{
			recurse_select(ptr->next, T);
		}
	}
}

void	Object::select_extents(Vector &raynear)
{
	BaseExtent	*traverse	=tree;

	got	=NULL;
	
	if(!traverse)
	{
		return;
	}
	
	Vector	position	=PHYSICS->get_center_of_mass(index);
	Matrix	orientation	=ENGINE->get_orientation(index);

	Transform	T(orientation);

	T.set_position(position);

	clickpos	=(physicsEditor.camera->get_position());
	clickdir	=(clickpos + raynear);

	recurse_select(traverse, T);

	if(got)
	{
		physicsEditor.selected_extent	=got;
	}
}

BOOL32	Object::load(C8 *fn)
{
	COMPTR	<IFileSystem>	fs;
	BOOL32	success	=FALSE;
	
	if(ENGINE->create_file_system(fn, fs) == GR_OK)
	{
		TEXTURELIB->load_library(fs, NULL);
		
		INSTANCE_INDEX	findex	=ENGINE->create_instance(fn, fs, NULL);
		
		if(findex != INVALID_INSTANCE_INDEX)
		{
			success	=TRUE;
			initialize();
			index	=findex;
			
			center_of_mass	= PHYSICS->get_center_of_mass(index);
			mass			= PHYSICS->get_mass(index);
			Ibody			= *PHYSICS->get_inertia_tensor(index);
		}
	}

	if(!success)
	{
		//something went wrong
		MessageBox(windowHandle, "Unable to load object. (corrupted?)", "Open Error", MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
		return	FALSE;
	}
	
	//compute scale
	SINGLE	box[6];
	
	ARCHETYPE_INDEX	arch	=ENGINE->get_instance_archetype(index);
	
	if(arch != INVALID_ARCHETYPE_INDEX)
	{
		RENDERER->get_archetype_bounding_box(arch, 1, box);
		ENGINE->release_archetype(arch);
	}
	else
	{
		AppFatal("object::initialize: couldn't get 3D bounds");
	}
	
	scale	=fabs(box[1] - box[0]);

	if(fabs(scale) > 0.10f)
	{
		if(fabs(box[3] - box[2]) > scale)
		{
			scale	=fabs(box[3] - box[2]);
		}
		
		if(fabs(box[5] - box[4]) > scale)
		{
			scale	=fabs(box[5] - box[4]);
		}
	}
	else	//take a guess at a good scale
	{
		scale	=6969.0f;
	}
	
	//extent tree
	BOOL32	corrupted	=FALSE;
	
	if(!PHYSICS->get_extent((const BaseExtent **) &tree, index))
	{
		MessageBox(windowHandle, "IPhysics couldn't read this object's extent tree. (corrupted?)", "Warning", MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
		tree	=NULL;
	}
	
	//treebar
	TreeBar::display_master_tree();
	
	//object name	
	char	work[1024];
	strcpy(work, appName);
	strcat(work, " - ");
	strcat(work, fn);
	
	//mesh
	mesh	=RENDERER->get_instance_mesh(index);
	
	//submesh
	subMeshEditor.reset(index);
	
	SetWindowText(windowHandle, work);
	return	TRUE;
}
