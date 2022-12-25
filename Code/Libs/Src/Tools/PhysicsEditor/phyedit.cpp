//
// <PhyEdit.cpp> - Physics editor main file
//
//
// todo: clicking/selecting extents
//       moving, resizing, and shifting extents
//       treeview sidebar for viewing trees? (and maybe selecting them and etc?)
//

#include "phyedit.h"
#include "fileutil.h"
#include "resource.h"
#include "input.h"
#include "treebar.h"
#include "mesh.h"
#include "extutil.h"
#include "ctest.h"
#include "cvisualize.h"
#include "primitive.h"
#include <commctrl.h>
#include "memfile.h"

// Main editor:
PhysicsEditor   physicsEditor;

// Sub-mesh editor:
SubMeshEditor   subMeshEditor;

// Sub-editors:
SphereEditor        sphereEditor;
BoxEditor           boxEditor;
TubeEditor			tubeEditor;
ConvexMeshEditor    convexMeshEditor;

extern	void	select_extent_treeitem(BaseExtent *xt, HWND tList, HTREEITEM hti);
extern	SINGLE	closestlen;
extern	HINSTANCE   appInstance;

void transformTo4x4(SINGLE m[16],const Transform &t)
{
	m[ 0] = t.d[0][0]; m[ 1] = t.d[1][0]; m[ 2] = t.d[2][0]; m[ 3] = 0;
	m[ 4] = t.d[0][1]; m[ 5] = t.d[1][1]; m[ 6] = t.d[2][1]; m[ 7] = 0;
	m[ 8] = t.d[0][2]; m[ 9] = t.d[1][2]; m[10] = t.d[2][2]; m[11] = 0;
	m[12] = t.translation.x; m[13] = t.translation.y; m[14] = t.translation.z; m[15] = 1;
}

//stolen from petal thankee tony b
static	bool CreateUTFFromResource (HINSTANCE hResource, int resId, const char *resType, IFileSystem ** pFile)
{
	// NOTE: Under Win32, the resource functions don't provide for "freeing" or "releasing" a resource.
	// These will automatically get released when the program exits.
	// Therefore, the only record of the loaded resource is the point to its contents, which will be forgotten
	// when the returned IFileSystem is released.

	HRSRC hRes;

	*pFile = 0;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(resId), resType)) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			LPVOID pData;

			if ((pData = LockResource(hGlobal)) != 0)
			{
				MEMFILEDESC mdesc = "resource file";

				mdesc.lpBuffer = pData;
				mdesc.dwBufferSize = SizeofResource(hResource, hRes);
				mdesc.dwFlags = CMF_DONT_COPY_MEMORY;

				CreateUTFMemoryFile(mdesc, pFile);
			}
		}
	}

	return (*pFile != 0);
}


void PhysicsEditor::init(void)
{
    camera = new BaseCamera(ENGINE, NULL);
    camera->set_position(Vector(0.0, 0.0, 100.0));
    camera->set_far_plane_distance(15000.0);

    font = new Font;

	COMPTR<IFileSystem> fs;

	if(CreateUTFFromResource(appInstance, IDR_TXM1, "TXM", fs))
	{
		font->load(fs);
	}

    TreeBar::create();
}

void PhysicsEditor::handle_keyboard(SINGLE dt)
{
    INSTANCE_INDEX index = object.index;

    // Object keys:

    if (index != INVALID_INSTANCE_INDEX)
    {
		keyboard_dt = dt;

        SINGLE spin_rate	= 3.0 * dt;
		SINGLE pan_rate		= object.scale * 1.0 * dt;

        if (keyboard->pressed(VK_SHIFT))
        {
            spin_rate = 1.0 * dt;
			pan_rate = object.scale * 0.3 * dt;
        }
        
        if (keyboard->pressed(VK_LEFT))
        {
            Quaternion q(Vector(0.0, 1.0, 0.0), spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));

			ENGINE->update_instance(index, 0, dt);

			// UGH
			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
        if (keyboard->pressed(VK_RIGHT))
        {
            Quaternion q(Vector(0.0, 1.0, 0.0), -spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));

			ENGINE->update_instance(index, 0, dt);

			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
        if (keyboard->pressed(VK_INSERT))
        {
            Quaternion q(Vector(0.0, 0.0, 1.0), spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));

			ENGINE->update_instance(index, 0, dt);

			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
        if (keyboard->pressed(VK_DELETE))
        {
            Quaternion q(Vector(0.0, 0.0, 1.0), -spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));
			
			ENGINE->update_instance(index, 0, dt);

			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
 
        if (keyboard->pressed(VK_UP))
        {
            Quaternion q(Vector(1.0, 0.0, 0.0), spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));

			ENGINE->update_instance(index, 0, dt);

			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
        if (keyboard->pressed(VK_DOWN))
        {
            Quaternion q(Vector(1.0, 0.0, 0.0), -spin_rate);
            ENGINE->set_orientation(index, Matrix(q) * ENGINE->get_orientation(index));

			ENGINE->update_instance(index, 0, dt);

			subMeshEditor.vcount = 0;
			subMeshEditor.fcount = 0;
			subMeshEditor.fill_vlist(index);
        }
        if (keyboard->pressed(VK_ADD))
        {           
            camera->set_position(camera->get_position() + Vector(0.0, 0.0, pan_rate));
        }
        if (keyboard->pressed(VK_SUBTRACT))
        {           
            camera->set_position(camera->get_position() - Vector(0.0, 0.0, pan_rate));
        }
		if (keyboard->pressed(VK_NUMPAD6))
		{
			camera->set_position(camera->get_position() - Vector(-pan_rate, 0.0, 0.0));
		}
		if (keyboard->pressed(VK_NUMPAD4))
		{
			camera->set_position(camera->get_position() - Vector(pan_rate, 0.0, 0.0));
		}
		if (keyboard->pressed(VK_NUMPAD8))
		{
			camera->set_position(camera->get_position() - Vector(0.0, -pan_rate, 0.0));
		}
		if (keyboard->pressed(VK_NUMPAD2))
		{
			camera->set_position(camera->get_position() - Vector(0.0, pan_rate, 0.0));
		}
       
    }
    
}

static	int	InitObjectList(INSTANCE_INDEX idx, Object *olist)
{
	int	cnt	=0;

	if(idx != INVALID_INSTANCE_INDEX)
	{
		INSTANCE_INDEX	child	=INVALID_INSTANCE_INDEX;
		child	=ENGINE->get_instance_child_next(idx, EN_DONT_RECURSE, child);
		
		olist->initialize();
		olist->index	=idx;
		
		olist->center_of_mass	=PHYSICS->get_center_of_mass(idx);
		olist->mass				=PHYSICS->get_mass(idx);
		olist->Ibody			=*PHYSICS->get_inertia_tensor(idx);
		
		// compute scale
		SINGLE	box[6];

		ARCHETYPE_INDEX arch = ENGINE->get_instance_archetype(idx);

		if (arch != INVALID_ARCHETYPE_INDEX)
		{
			RENDERER->get_archetype_bounding_box(arch, 1, box);
			ENGINE->release_archetype(arch);
		}
		else
		{
			AppFatal("object::initialize: couldn't get 3D bounds");
		}
		olist->scale = fabs(box[1] - box[0]);

		if (fabs(box[3] - box[2]) > olist->scale)
		{
			olist->scale = fabs(box[3] - box[2]);
		}

		if (fabs(box[5] - box[4]) > olist->scale)
		{
			olist->scale = fabs(box[5] - box[4]);
		}

		// extent tree
		BOOL32 corrupted = FALSE;

		if (!PHYSICS->get_extent((const BaseExtent **) &olist->tree, idx))
		{
			MessageBox(windowHandle, "IPhysics couldn't read this object's extent tree. (corrupted?)", "Warning", MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
			olist->tree	=NULL;
		}

		// mesh
		olist->mesh	=RENDERER->get_instance_mesh(idx);

		//these are the names in the compound section
		strcpy(olist->name, ENGINE->get_instance_part_name(idx));

		cnt++;
		olist++;

		while(child != INVALID_INSTANCE_INDEX)
		{
			int	incr	=InitObjectList(child, olist);

			cnt		+=incr;
			olist	+=incr;
			child	=ENGINE->get_instance_child_next(idx, EN_DONT_RECURSE, child);
		}
	}

	return	cnt;
}


//slow
Object	*PhysicsEditor::FindExtentOwner(BaseExtent *bx)
{
	BaseExtent	*ch, *nx;
	int			i;

	assert(bx);

	for(i=0;i < NumChildObjects;i++)
	{
		for(ch=ChildObjects[i].tree;ch;ch=ch->child)
		{
			if(ch == bx)
			{
				return	&ChildObjects[i];
			}
			for(nx=ch->next;nx;nx=nx->next)
			{
				if(nx == bx)
				{
					return	&ChildObjects[i];
				}
			}
		}
	}
	return	NULL;
}

void PhysicsEditor::update(void)
{
	SINGLE dt = timer.compute_elapsed_time();
    
	// input
    handle_keyboard(dt);
    
    // render
    
    // bah
    ENGINE->update(0.0);

	if (current_editor)
    {
		object.render(camera, FALSE);
		
        current_editor->post_render();

		if (selected_extent)
		{
			if(CurObject)
			{
				render_extent(selected_extent, PHYSICS->get_center_of_mass(CurObject->index), ENGINE->get_orientation(CurObject->index));
			}
		}

    }
    else
    {
		if(free_extent_count == 0)
		{
			object.render(camera, TRUE);

			if(cmp_edit)
			{
				for(int i=0;i < NumChildObjects;i++)
				{
					ChildObjects[i].render_extents();
				}
			}
			else
			{
				object.render_extents();
			}
		}
		else
		{
			object.render(camera, FALSE);
		}

		if(selected_extent)
		{
			if(CurObject)
			{
				render_extent(selected_extent, PHYSICS->get_center_of_mass(CurObject->index), ENGINE->get_orientation(CurObject->index));
			}
		}
    }
    
//    glFlush();

    // submesh
    subMeshEditor.render();

    // cvis
    if (CVisualization::view_function)
        CVisualization::view_function();

	char fbuf[256];

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);

    font->begin();

	if (cmp_edit)
	{
		glColor4f(0.6, 0.6, 1.0, 1.0);
		font->draw_text(400, 380, "COMPOUND");
	}

    if (current_editor)
    {
        glColor4f(0.9, 0.9, 1.0, 1.0); 
        sprintf(fbuf, current_editor->mode_string); 
        font->draw_text(10, 350, fbuf); 

        current_editor->font_render(font);
    }

    font->print_var(10, 380, "FPS:", "%3.1f", 1.0 / dt);
    
    Vector cpos = camera->get_position();
    font->print_var(10, 435, "CDist:", "%2.2f", cpos.z);
    
    glColor4f(0.9, 0.9, 0.9, 1.0); sprintf(fbuf, "Object: "); font->draw_text(10, 400, fbuf);
    glColor4f(1.0, 0.8, 0.8, 1.0); 
    
    if(object.index == INVALID_INSTANCE_INDEX)
	{
		sprintf(fbuf, "(none)");
	}
	else
	{
		if(CurObject)
		{
			sprintf(fbuf, "%s", CurObject->name);
		}
		else
		{
			sprintf(fbuf, "%s", object.name);
		}
	}
    
    font->draw_text(10 + (8 * 8), 400, fbuf);

	if(CurObject)
	{
		font->print_var(10, 415, "Mass:", "%5.3f", CurObject->mass);
		font->print_var(150, 415, "Idiag:", "(%3.2f, %3.2f, %3.2f)", CurObject->Ibody.d[0][0], CurObject->Ibody.d[1][1], CurObject->Ibody.d[2][2]);
	}

    if (object.mesh)
    {
        font->print_var(510, 415, "Verts:", "%d", object.mesh->object_vertex_cnt);
        font->print_var(510, 430, "Polys:", "%d", object.mesh->face_cnt);
    }

	if(bHelp && current_editor)
	{
		current_editor->help_print(font);
	}
    
    font->end();
    
}

void PhysicsEditor::uninit(void)
{
    delete camera;
    camera = NULL;

    delete font;
    font = NULL;

    TreeBar::destroy();
}

void PhysicsEditor::initialize_matrix(void)
{
	Transform xf = camera->transform.get_inverse();

	RP->set_modelview(xf);
	RP->set_perspective(camera->fovy, camera->aspect, camera->znear, camera->zfar);

}

// Main app functions:
void appInit(void)
{
    physicsEditor.init();
}

void flag_function(WPARAM opt, U32 flag)
{
    HMENU           hMenu = GetMenu(windowHandle);
    MENUITEMINFO    mInfo;

    GetMenuItemInfo(hMenu, opt, FALSE, &mInfo);

    if (GetMenuState(hMenu, opt, MF_BYCOMMAND) == MF_CHECKED)
    {
        CheckMenuItem(hMenu, opt, MF_UNCHECKED);
        physicsEditor.clr_flag(flag);
    }
    else
    {
        CheckMenuItem(hMenu, opt, MF_CHECKED);
        physicsEditor.set_flag(flag);
    }
    
}

void reset_editor(void)
{
    // reset all editors
    for (int i = 0; i < physicsEditor.free_extent_count; i++)
    {
        delete physicsEditor.free_extent_list[i];
        physicsEditor.free_extent_list[i] = NULL;
    }

    physicsEditor.free_extent_count = 0;

    physicsEditor.current_editor = NULL;
   
    // refresh
    TreeBar::refresh_all_windows();

}

void	GetPartNames(const char *fn)
{
    COMPTR	<IFileSystem>	fs;
	int		i;

	assert(fn);

    if(ENGINE->create_file_system(fn, fs) == GR_OK)
    {
		if(!fs->SetCurrentDirectory("Cmpnd"))
		{
			__asm int 0x03;
		}
		else
		{
			WIN32_FIND_DATA	find_data;
			HANDLE			srch	=fs->FindFirstFile("*.*", &find_data);
			
			while(INVALID_HANDLE_VALUE != srch)
			{
				char	*pfn;

				if(! strcmp(find_data.cFileName, "Cons"))
				{
					break;
				}
				if(!fs->SetCurrentDirectory(find_data.cFileName))
				{
					__asm int 0x03;
				}

				//if the directory has "Part_" at the beginning, skip past
				if(*((U32 *)find_data.cFileName)	== 0x74726150)	//Part
				{
					pfn	=find_data.cFileName + 5;
				}
				else
				{
					pfn	=find_data.cFileName;
				}

				//locate which childobject this is
				for(i=0;i < physicsEditor.NumChildObjects;i++)
				{
					if(!strcmp(physicsEditor.ChildObjects[i].name, pfn))
					{
						LoadFile("File name", &physicsEditor.ChildObjects[i].filename, 256, fs);
						break;
					}
				}
				if(i > physicsEditor.NumChildObjects)
				{
					_asm int 0x03;
				}
				if(!fs->SetCurrentDirectory(".."))
				{
					__asm int 0x03;
				}

				if(!fs->FindNextFile(srch, &find_data))
				{
					assert(ERROR_NO_MORE_FILES == fs->GetLastError());
					break;
				}
			}
		}
	}
}

void appWindowCallback(UINT msg, WPARAM wParam, LPARAM lParam)
{
    KeyboardMessage kb;
    MouseMessage    m;

    switch (msg)
    {
        case WM_MOVE:
            TreeBar::refresh();
            break;

        case WM_COMMAND:

			switch(wParam)
			{
				case IDD_FILE_EXIT:
					{
						extern HWND windowHandle; // in main.cpp
						DestroyWindow (windowHandle);
					}
					break;

                case IDD_FILE_OPEN:
                    
                    char fn[256];

                    if (open_file_dialog(fn))
                    {
                        physicsEditor.object.reset();

						if(physicsEditor.ChildObjects)
						{
							free(physicsEditor.ChildObjects);
							physicsEditor.ChildObjects	=NULL;
						}
						physicsEditor.NumChildObjects	=0;
                                        
                        if (physicsEditor.object.load(fn))
                        {
                            // set object path/name
                            strcpy(physicsEditor.object.name, fn);

                            // set camera scale
                            Vector cpos(0.0, 0.0, physicsEditor.object.scale * 2.0);
                            physicsEditor.camera->set_position(cpos);
                            physicsEditor.camera->set_far_plane_distance(physicsEditor.object.scale * 4.0);

							strupr(physicsEditor.object.name);

							if (strstr(physicsEditor.object.name, ".CMP"))
							{
								physicsEditor.cmp_edit			=TRUE;
								physicsEditor.NumChildObjects	=physicsEditor.object.GetNumMeshes(physicsEditor.object.index);
								physicsEditor.ChildObjects		=new Object[physicsEditor.NumChildObjects];
								physicsEditor.CurObject			=&physicsEditor.ChildObjects[0];

								InitObjectList(physicsEditor.object.index, physicsEditor.ChildObjects);
								physicsEditor.camera->set_far_plane_distance(physicsEditor.NumChildObjects * physicsEditor.object.scale * 4.0);

								//fill up the file names for matching up parts to extents
								GetPartNames(fn);
							}
							else
							{
								physicsEditor.CurObject	=&physicsEditor.object;
								physicsEditor.cmp_edit = FALSE;
							}
							TreeBar::display_master_tree();

                            // reset all editors
							reset_editor();                            
                        }

                    }
                    break;
					
                case IDD_FILE_SAVE:
					if(physicsEditor.object.index != INVALID_INSTANCE_INDEX)
					{
                        if (physicsEditor.object.tree)
						{
							BOOL32 ok = TRUE;
                        
							if (physicsEditor.free_extent_count != 0)
							{
								if (MessageBox(windowHandle, "There are unattached extents. Discard them?", "Warning", MB_YESNO) != IDYES)
									ok = FALSE;
                            
							}

							if (ok)
							{
								char tmp[256];

								sprintf(tmp, "Save %s?", physicsEditor.object.name);

								if (MessageBox(windowHandle, tmp, "Save", MB_OKCANCEL) == IDOK)
								{
									write_extent_tree(physicsEditor.object.name);
								}
							}
						}
						else
						{
							MessageBox(windowHandle, "Object doesn't have any physics extents to save.", "Error", MB_OK);
						}
					}
                    break;

                case IDD_FILE_SAVEAS:

                    if (physicsEditor.object.index != INVALID_INSTANCE_INDEX)
                    {
                        if (physicsEditor.object.tree)
                        {
                            BOOL32 ok = TRUE;
                            
                            if (physicsEditor.free_extent_count != 0)
                            {
                                if (MessageBox(windowHandle, "There are unattached extents. Discard them?", "Warning", MB_YESNO) != IDYES)
                                    ok = FALSE;
                                
                            }

                            if (ok)
                            {
                                char tmp[256];
                            
                                if (save_as_file_dialog(tmp))
                                {
                                    // copy object to new file, make readable
                                    CopyFile(physicsEditor.object.name, tmp, TRUE);
                                    write_extent_tree(tmp);
                                    strcpy(physicsEditor.object.name, tmp);

                                    // object name
                                    char work[1024];
                                    strcpy(work, appName);
                                    strcat(work, " - ");
                                    strcat(work, physicsEditor.object.name);

                                    SetWindowText(windowHandle, work);
                                }
                            }
                        }
                        else
                        {
                            MessageBox(windowHandle, "Object doesn't have any physics extents to save.", "Error", MB_OK);
                        }
                    }

                    break;

                    case IDD_FILE_COPY:

                        if (physicsEditor.object.index != INVALID_INSTANCE_INDEX)
                        {
                            if (physicsEditor.object.tree)
                            {
                                if (copy_file_dialog())
                                {
                                    char tstr[1024];
                                    sprintf(tstr, "Copy this object's extents into these %d objects?\n", copy_file_count);

                                    if (MessageBox(windowHandle, tstr, "Export", MB_OKCANCEL | MB_TOPMOST) == IDOK)
                                    {
                                        char fn[16384];

                                        for (int i = 0; i < copy_file_count; i++)
                                        {
                                            strcpy(fn, copy_path);
                                            strcat(fn, "\\");
                                            strcat(fn, copy_file_list[i]);

                                            write_extent_tree(fn);
                                        }

                                        sprintf(fn, "Exported into %d objects.", copy_file_count);
                                        MessageBox(windowHandle, fn, "Success", MB_OKCANCEL | MB_TOPMOST);

                                    }
                                }
                            }
                            else
                            {
                                MessageBox(windowHandle, "Object doesn't have any physics extents to save.", "Error", MB_OK);
                            }
                        }

                        break;

				// editing

				case ID_MIRROR_X:
					
					if (physicsEditor.object.tree)
						mirror_tree(physicsEditor.object.tree, MIRROR_X, physicsEditor.object.center_of_mass);
					
					break;

				case ID_MIRROR_Y:
					
					if (physicsEditor.object.tree)
						mirror_tree(physicsEditor.object.tree, MIRROR_Y, physicsEditor.object.center_of_mass);
					
					break;

				case ID_EDIT_RIGID_DATA:

					edit_rigid_data();
					break;

				// autogenerate:

				case ID_GENERATE_SPHERE:

					autogenerate_sphere();
					reset_editor();
                    
					break;

				case ID_GENERATE_SPHERE_BOX:

					autogenerate_sphere_box();
					reset_editor();
					
					break;

				case ID_GENERATE_IBODY:

					autogenerate_inertia_tensor();
					break;
					                    
                // flags:

                case ID_VIEW_SOLIDEXTENTS:
                    flag_function(ID_VIEW_SOLIDEXTENTS, PEC_SOLID_EXTENTS);
                    break;

                case ID_VIEW_TEXTURES:
                    flag_function(ID_VIEW_TEXTURES, PEC_DRAW_TEXTURES);
                    break;

                case ID_VIEW_COLLIDABLE_ONLY:
                    flag_function(ID_VIEW_COLLIDABLE_ONLY, PEC_COLLIDABLE_ONLY);
                    break;

                // tests

                case ID_CENTER_RAYS:

                    char out[256];

                    if (CTest::perform_center_directed_ray_test())
                    {
                        sprintf(out, "Average ray intersect time: %3.4f µsec\n", CTest::ray_test_time * 1000000.0);
                        MessageBox(windowHandle, out, "CTest", MB_OK);
                    }

                    break;

                // visualization

                case ID_VISUALIZE_RAYS:
                {
                    HMENU           hMenu = GetMenu(windowHandle);
                    MENUITEMINFO    mInfo;

                    GetMenuItemInfo(hMenu, ID_VISUALIZE_RAYS, FALSE, &mInfo);

                    if (GetMenuState(hMenu, ID_VISUALIZE_RAYS, MF_BYCOMMAND) == MF_CHECKED)
                    {
                        CheckMenuItem(hMenu, ID_VISUALIZE_RAYS, MF_UNCHECKED);
                        CVisualization::view_function = NULL;
                    }
                    else
                    {
                        CheckMenuItem(hMenu, ID_VISUALIZE_RAYS, MF_CHECKED);
                        CVisualization::view_function = CVisualization::ray_collisions;
                    }

                    break;
                }

            }
            break;

        case WM_KEYDOWN:

			//look for next object key
			
			if(wParam == VK_NEXT)
			{
				for(int i=0;i < physicsEditor.NumChildObjects;i++)
				{
					if(physicsEditor.CurObject == &physicsEditor.ChildObjects[i])
					{
						if(i < (physicsEditor.NumChildObjects - 1))
						{
							physicsEditor.CurObject	=&physicsEditor.ChildObjects[i+1];
						}
						else
						{
							physicsEditor.CurObject	=&physicsEditor.ChildObjects[0];
						}
						break;
					}
				}
				if(physicsEditor.current_editor)
				{
					physicsEditor.current_editor->reset_selection();
				}
				physicsEditor.selected_extent	=NULL;
			}
			else if(wParam == VK_PRIOR)
			{
				for(int i=0;i < physicsEditor.NumChildObjects;i++)
				{
					if(physicsEditor.CurObject == &physicsEditor.ChildObjects[i])
					{
						if(i)
						{
							physicsEditor.CurObject	=&physicsEditor.ChildObjects[i-1];
						}
						else
						{
							physicsEditor.CurObject	=&physicsEditor.ChildObjects[physicsEditor.NumChildObjects - 1];
						}
						break;
					}
				}
				if(physicsEditor.current_editor)
				{
					physicsEditor.current_editor->reset_selection();
				}
				physicsEditor.selected_extent	=NULL;
			}
            kb.key      = wParam;
            kb.pressed  = TRUE;
            
            if (physicsEditor.current_editor)
            {
                physicsEditor.current_editor->keyboard_handler(kb);
            }

            subMeshEditor.process_keyboard(&kb);
            
            break;

        case WM_KEYUP:

            kb.key      = wParam;
            kb.pressed  = FALSE;
            
            if (physicsEditor.current_editor)
            {
                physicsEditor.current_editor->keyboard_handler(kb);
            }

            subMeshEditor.process_keyboard(&kb);
                        
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        
            m.message = msg;
            m.x = LOWORD(lParam);
            m.y = HIWORD(lParam);

            m.left_pressed = (wParam & MK_LBUTTON);
            m.right_pressed = (wParam & MK_RBUTTON);
			if (physicsEditor.current_editor)
            {
                physicsEditor.current_editor->mouse_handler(m);
                if(physicsEditor.current_editor->type != EM_CONVEX_MESH)
				{
                    subMeshEditor.process_mouse(&m);
				}
            }
			else if(m.left_pressed)
			{
				//try to select an extent
				SINGLE	nearp	=physicsEditor.camera->znear;
				Vector	raynear;

				physicsEditor.camera->set_near_plane_distance(6969.0f);

				physicsEditor.camera->screen_to_point(raynear, m.x, m.y);
				raynear.scale(6969.0f);
				raynear.add(physicsEditor.camera->get_position());

				physicsEditor.camera->set_near_plane_distance(nearp);

				BaseExtent	*old	=physicsEditor.selected_extent;

				closestlen	=6969.0f;
				if(physicsEditor.cmp_edit)
				{
					for(int i=0;i < physicsEditor.NumChildObjects;i++)
					{
						physicsEditor.ChildObjects[i].select_extents(raynear);
					}
				}
				else
				{
					physicsEditor.object.select_extents(raynear);
				}

				if(physicsEditor.selected_extent != old)
				{
					if(physicsEditor.cmp_edit)
					{
						physicsEditor.CurObject	=physicsEditor.FindExtentOwner(physicsEditor.selected_extent);
					}
					select_extent_treeitem(physicsEditor.selected_extent,
						GetDlgItem(TreeBar::handle, IDC_TREE_LIST),
						TreeView_GetRoot(GetDlgItem(TreeBar::handle, IDC_TREE_LIST)));

					if(physicsEditor.current_editor)
					{
						physicsEditor.current_editor->reset_selection();
					}
				}
			}
            break;
		    
    }
}
                

void appMainLoop(void)
{
	RP->set_pipeline_state(RP_CLEAR_COLOR, 0);
	RP->set_pipeline_state(RP_CLEAR_DEPTH, 0xFFFFFFFF);
	RP->clear_buffers(RP_CLEAR_COLOR_BIT | RP_CLEAR_DEPTH_BIT, NULL);

	RP->begin_scene();

    physicsEditor.update();

	RP->end_scene();
	RP->swap_buffers();

}

void appUninit(void)
{
    physicsEditor.uninit();
}