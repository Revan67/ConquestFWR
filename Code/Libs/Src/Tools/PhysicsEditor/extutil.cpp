//
// <extutil.cpp> - extent utilities
//

#include "phyedit.h"
#include "extutil.h"
#include "fileutil.h"
#include "resource.h"

void recursive_save(IFileSystem * fs, BaseExtent * tree);

void remove_extent(BaseExtent * base, BaseExtent * remove, BOOL32 unlink_only)
{
	// delete 'remove' extent from the 'base' tree, fixing up resultant extents appropriately

    if (remove == base)
    {
        // how to do pop? maybe have remove_extent_tree not delete the item
        remove_extent_tree(base);
        base = NULL;

    }
    else
    {
        if (base->child)
        {
            if (base->child == remove)
            {
				BaseExtent * next_tmp = base->child->next;
                remove_extent_tree(base->child);
                base->child = next_tmp;
            }
            else
            {
                remove_extent(base->child, remove, unlink_only);
            }
        }

        if (base->next)
        {
            if (base->next == remove)
            {
				BaseExtent * next_tmp = base->next->next;
				remove_extent_tree(base->next);
                base->next = next_tmp;
            }
            else
            {
                remove_extent(base->next, remove, unlink_only);
            }
        }
    }
   
}

void recurse_delete(BaseExtent * base)
{
    if (base->child)
    {
        recurse_delete(base->child);
        base->child = NULL;
    }

	if (base->next)
    {
        recurse_delete(base->next);
        base->next = NULL;
    }

    delete base;
    
}

void remove_extent_tree(BaseExtent * base)
{
    if (base->child)
    {
        recurse_delete(base->child);
        base->child = NULL;
    }

    delete base;
    
}

BOOL32 add_extent_as_next(BaseExtent * base, BaseExtent * extent)
{
    while (base->next)
    {
        base = base->next;
    }

    base->next = extent;

    return TRUE;
}

BOOL32 add_extent_as_child(BaseExtent * base, BaseExtent * extent)
{
    if (base->child)
    {
        return FALSE;
    }
    else
    {
        base->child = extent;
    }

    return TRUE;
    
}

void DebugPrint(C8 * message, ...)
{
	static C8 buffer[1024];

	va_list arglist;
	va_start(arglist, message);
	vsprintf(buffer, message, arglist);
	va_end(arglist);

	OutputDebugString(buffer);
}

void write_extent_tree(IFileSystem * fs, const Object *obj)
{
	if (!fs->SetCurrentDirectory("Rigid body"))
    {
        // no 'rigid body' chunk
        __asm int 0x03;
    }

    // whack old dirs

    DeleteDirectory(fs, "Extent data");
    DeleteDirectory(fs, "Extent tree");

	DeleteDirectory(fs, "Mass properties");
	
	// Mass, Center of mass, Inertia tensor
	fs->CreateDirectory("Mass properties");
	fs->SetCurrentDirectory("Mass properties");

	SaveFile("Mass", &physicsEditor.object.mass, sizeof(SINGLE), fs);
	SaveFile("Center of mass", (void *)&obj->center_of_mass, sizeof(Vector), fs);
	SaveFile("Inertia tensor", (void *)&obj->Ibody, sizeof(Matrix), fs);
	
	fs->SetCurrentDirectory("..");

    // create
    fs->CreateDirectory("Extent tree");
    fs->SetCurrentDirectory("Extent tree");

    recursive_save(fs, obj->tree);
	fs->SetCurrentDirectory("..\\..\\..");
}

BOOL32	write_extent_tree(C8 * filename)
{
	IFileSystem	*fs;
    DAFILEDESC	fd;
	int			i;
	
	fd.lpFileName				=filename;
	fd.lpImplementation			="UTF";
	fd.dwDesiredAccess			=GENERIC_READ | GENERIC_WRITE;
	fd.dwCreationDistribution	=OPEN_ALWAYS;
	
	if(DACOM->CreateInstance(&fd, (void **) &fs) != GR_OK)
	{
		// read-only
		MessageBox(windowHandle, "Save error. (File is probably read-only)", "Error", MB_OK);
		return	FALSE;
	}
	
	if(!physicsEditor.cmp_edit)
	{
		write_extent_tree(fs, &physicsEditor.object);
	}
	else
	{
		WIN32_FIND_DATA	find_data;
		HANDLE			srch	=fs->FindFirstFile("*.3DB", &find_data);
		
		while(INVALID_HANDLE_VALUE != srch)
		{
			if(!fs->SetCurrentDirectory(find_data.cFileName))
			{
				__asm int 0x03;
			}

			//find the matching object
			for(i=0;i < physicsEditor.NumChildObjects;i++)
			{
				if(!strcmp(physicsEditor.ChildObjects[i].filename, find_data.cFileName))
				{
					write_extent_tree(fs, &physicsEditor.ChildObjects[i]);
					break;
				}
			}

			if(i > physicsEditor.NumChildObjects)
			{
				_asm int 0x03;
			}

			if(!fs->FindNextFile(srch, &find_data))
			{
				assert(ERROR_NO_MORE_FILES == fs->GetLastError());
				break;
			}
		}
	}
	fs->Release();
	return	TRUE;
}

void save_sphere(IFileSystem * fs, BaseExtent * tree)
{
    SphereExtent * sphere = (SphereExtent *) tree;
    
    SaveFile("Transform", &sphere->xform, sizeof(Transform), fs);
	
	if (sphere->name)
		SaveFile("Name", sphere->name, strlen(sphere->name) + 1, fs);
    
	SaveFile("Radius", &sphere->sphere.radius, sizeof(sphere->sphere.radius), fs);

}

void save_box(IFileSystem * fs, BaseExtent * tree)
{
    BoxExtent * box = (BoxExtent *) tree;
    
    SaveFile("Transform",   &box->xform,        sizeof(Transform), fs);

   	if (box->name)
		SaveFile("Name", box->name, strlen(box->name) + 1, fs);

	SaveFile("half x",      &box->box.half_x,   sizeof(SINGLE), fs);
    SaveFile("half y",      &box->box.half_y,   sizeof(SINGLE), fs);
    SaveFile("half z",      &box->box.half_z,   sizeof(SINGLE), fs);

}

void	save_cylinder(IFileSystem *fs, BaseExtent *tree)
{
	CylinderExtent	*cyl	=(CylinderExtent *)tree;
	
	SaveFile("Transform", &cyl->xform, sizeof(Transform), fs);
	
	if(cyl->name)
	{
		SaveFile("Name", cyl->name, strlen(cyl->name) + 1, fs);
	}
	
	SaveFile("length", &cyl->cylinder.length, sizeof(SINGLE), fs);
	SaveFile("radius", &cyl->cylinder.radius, sizeof(SINGLE), fs);
}

void	save_tube(IFileSystem *fs, BaseExtent *tree)
{
	save_cylinder(fs, tree);	//same for now
}

void save_convex_mesh(IFileSystem * fs, BaseExtent * tree)
{
    ConvexMeshExtent * cmesh = (ConvexMeshExtent *) tree;
    CollisionMesh * mesh = cmesh->mesh;

    SaveFile("Transform", &cmesh->xform, sizeof(Transform), fs);
    
	if (cmesh->name)
		SaveFile("Name", cmesh->name, strlen(cmesh->name) + 1, fs);

    SaveFile("Centroid", &mesh->centroid, sizeof(Vector), fs);

    SaveFile("Vertex count", &mesh->num_vertices, sizeof(S32), fs);
    SaveFile("Vertex list", mesh->vertices, sizeof(Vertex) * mesh->num_vertices, fs);

    SaveFile("Edge count", &mesh->num_edges, sizeof(S32), fs);
    SaveFile("Edge list", mesh->edges, sizeof(Edge) * mesh->num_edges, fs);

    SaveFile("Face count", &mesh->num_triangles, sizeof(S32), fs);
    SaveFile("Face list", mesh->triangles, sizeof(Triangle) * mesh->num_triangles, fs);

    SaveFile("Normal count", &mesh->num_normals, sizeof(S32), fs);
    SaveFile("Normal list", mesh->normals, sizeof(Vector) * mesh->num_normals, fs);

    SaveFile("Triangle D", mesh->triangle_d, sizeof(float) * mesh->num_triangles, fs);
}

void recursive_save(IFileSystem * fs, BaseExtent * tree)
{
	IFileSystem	*exfs;
	
	S32	idx_sphere	=1;
    S32	idx_box		=1; 
    S32	idx_cmesh	=1;
    S32	idx_gmesh	=1;
    S32	idx_tube	=1;
    S32	idx_cylinder=1;

    C8  ename[256];
    
    while (tree)
    {    
        switch (tree->type)
        {
            case ET_SPHERE:
                sprintf(ename, "%s %d", "Sphere", idx_sphere++);
                fs->CreateDirectory(ename);
                exfs = OpenDirectory(ename, fs);
                save_sphere(exfs, tree);
                break;

            case ET_BOX:
                sprintf(ename, "%s %d", "Box", idx_box++);
                fs->CreateDirectory(ename);
                exfs = OpenDirectory(ename, fs);
                save_box(exfs, tree);
                break;

            case ET_CONVEX_MESH:
                sprintf(ename, "%s %d", "Convex mesh", idx_cmesh++);
                fs->CreateDirectory(ename);
                exfs = OpenDirectory(ename, fs);
                save_convex_mesh(exfs, tree);
                break;

            case ET_GENERAL_MESH:
                break;

            case ET_TUBE:
                sprintf(ename, "%s %d", "Tube", idx_tube++);
                fs->CreateDirectory(ename);
                exfs	=OpenDirectory(ename, fs);
                save_tube(exfs, tree);
                break;

            case ET_CYLINDER:
                sprintf(ename, "%s %d", "Cylinder", idx_cylinder++);
                fs->CreateDirectory(ename);
                exfs	=OpenDirectory(ename, fs);
                save_cylinder(exfs, tree);
                break;
        };

        // child save

        if (tree->child)
        {
            exfs->CreateDirectory("Children");
            IFileSystem * fs2 = OpenDirectory("Children", exfs);
            recursive_save(fs2, tree->child);
            fs2->Release();
        }

        exfs->Release();
          
        // next save
        tree = tree->next;
    }
    
}

// autogenerate

#include "mingeom.h"
#include "mesh.h"

void autogenerate_sphere()
{
	if (physicsEditor.object.index == INVALID_INSTANCE_INDEX) return;

	//compute_optimal_sphere(physicsEditor.object.tree, physicsEditor.object.mesh->object_vertex_cnt, physicsEditor.object.mesh->object_vertex_list);
    //physicsEditor.object.tree->xform.translation -= physicsEditor.object.center_of_mass;

};

void autogenerate_sphere_box()
{
	if (physicsEditor.object.index == INVALID_INSTANCE_INDEX) return;

	//compute_optimal_sphere(physicsEditor.object.tree, physicsEditor.object.mesh->object_vertex_cnt, physicsEditor.object.mesh->object_vertex_list);
    //physicsEditor.object.tree->xform.translation -= physicsEditor.object.center_of_mass;
}

// editing

void recurse_mirror(BaseExtent * extent, MirrorType type, const Vector & mirror_bias)
{
	Matrix tx;

	switch (type)
	{
		case MIRROR_X:
		
			extent->xform.translation += mirror_bias;
			
			extent->xform.translation.x *= -1.0;
			
			tx = extent->xform.get_orientation();
			tx.set_i(tx.get_i() * -1.0);
			extent->xform.set_orientation(tx);

			extent->xform.translation -= mirror_bias;
			
			break;

		case MIRROR_Y:

			extent->xform.translation += mirror_bias;
			
			extent->xform.translation.y *= -1.0;

			tx = extent->xform.get_orientation();
			tx.set_j(tx.get_j() * -1.0);
			extent->xform.set_orientation(tx);

			extent->xform.translation -= mirror_bias;
			
			break;

		default:
			return;
	}

	if (extent->next)
		recurse_mirror(extent->next, type, mirror_bias);

	if (extent->child)
		recurse_mirror(extent->child, type, mirror_bias);

}

void mirror_tree(BaseExtent * extent, MirrorType type, const Vector & mirror_bias)
{
	if (physicsEditor.object.tree)
	{
		recurse_mirror(physicsEditor.object.tree, type, mirror_bias);
	}
}

void autogenerate_inertia_tensor()
{
	if (physicsEditor.object.tree)
	{
		// recompute inertia tensor based on bounding box or sphere dimensions and mass

		BaseExtent * ex = physicsEditor.object.tree;
		SINGLE mass = physicsEditor.object.mass;
		
		if (ex->child)
		{
			if (ex->child->type == ET_BOX)
			{
				Box * box = &((BoxExtent *) ex->child)->box;
				
				physicsEditor.object.Ibody.zero();

				// box is M * (a^2 + b^2) / 12
				
				physicsEditor.object.Ibody.d[0][0] = mass * (pow(box->half_y * 2.0, 2.0) + pow(box->half_z * 2.0, 2.0)) / 12.0;
				physicsEditor.object.Ibody.d[1][1] = mass * (pow(box->half_x * 2.0, 2.0) + pow(box->half_z * 2.0, 2.0)) / 12.0;
				physicsEditor.object.Ibody.d[2][2] = mass * (pow(box->half_x * 2.0, 2.0) + pow(box->half_y * 2.0, 2.0)) / 12.0;
			}
		}
		else
		{
			if (ex->type == ET_SPHERE)
			{
				Sphere * sphere = &((SphereExtent *) ex)->sphere;

				physicsEditor.object.Ibody.zero();
				
				// sphere is M * 2 * r^2 / 5

				physicsEditor.object.Ibody.d[0][0] = 2.0 * mass * pow(sphere->radius, 2.0) / 5.0;
				physicsEditor.object.Ibody.d[1][1] = 2.0 * mass * pow(sphere->radius, 2.0) / 5.0;
				physicsEditor.object.Ibody.d[2][2] = 2.0 * mass * pow(sphere->radius, 2.0) / 5.0;

			}
		}
	}
}

BOOL CALLBACK rigid_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char mass[256], ix[256], iy[256], iz[256];

	switch (message)
    {
        case WM_INITDIALOG:

			sprintf(mass, "%f", physicsEditor.object.mass);
			SendMessage(GetDlgItem(hwnd, IDC_MASS_EDIT), WM_SETTEXT, 0, (LPARAM) mass);
            sprintf(ix, "%f", physicsEditor.object.Ibody.d[0][0]);
			SendMessage(GetDlgItem(hwnd, IDC_INERTIA_X), WM_SETTEXT, 0, (LPARAM) ix);
            sprintf(iy, "%f", physicsEditor.object.Ibody.d[1][1]);
			SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Y), WM_SETTEXT, 0, (LPARAM) iy);
            sprintf(iz, "%f", physicsEditor.object.Ibody.d[2][2]);
			SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Z), WM_SETTEXT, 0, (LPARAM) iz);
            return TRUE;

        case WM_COMMAND:
            switch LOWORD(wParam)
            {
				case IDC_COMPUTE_IBODY:

					if (HIWORD(wParam) == BN_CLICKED)
					{
						autogenerate_inertia_tensor();
						
						sprintf(ix, "%f", physicsEditor.object.Ibody.d[0][0]);
						SendMessage(GetDlgItem(hwnd, IDC_INERTIA_X), WM_SETTEXT, 0, (LPARAM) ix);
						sprintf(iy, "%f", physicsEditor.object.Ibody.d[1][1]);
						SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Y), WM_SETTEXT, 0, (LPARAM) iy);
						sprintf(iz, "%f", physicsEditor.object.Ibody.d[2][2]);
						SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Z), WM_SETTEXT, 0, (LPARAM) iz);
           			}
					break;

                case IDOK:
					SendMessage(GetDlgItem(hwnd, IDC_MASS_EDIT), WM_GETTEXT, 256, (LPARAM) mass);
					physicsEditor.object.mass = (SINGLE) atof(mass);
					SendMessage(GetDlgItem(hwnd, IDC_INERTIA_X), WM_GETTEXT, 256, (LPARAM) ix);
					physicsEditor.object.Ibody.d[0][0] = (SINGLE) atof(ix);
					SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Y), WM_GETTEXT, 256, (LPARAM) iy);
					physicsEditor.object.Ibody.d[1][1] = (SINGLE) atof(iy);
					SendMessage(GetDlgItem(hwnd, IDC_INERTIA_Z), WM_GETTEXT, 256, (LPARAM) iz);
					physicsEditor.object.Ibody.d[2][2] = (SINGLE) atof(iz);
										
					EndDialog(hwnd, TRUE);
					break;
				
				case IDCANCEL:
					EndDialog(hwnd, FALSE);
					break;
			}
			break;

	}

	return FALSE;
}

void edit_rigid_data()
{
	if (physicsEditor.object.index != INVALID_INSTANCE_INDEX)
	{
		DialogBox(appInstance, MAKEINTRESOURCE(IDD_SET_RIGID_DATA), windowHandle, (DLGPROC) rigid_proc);
	}
}