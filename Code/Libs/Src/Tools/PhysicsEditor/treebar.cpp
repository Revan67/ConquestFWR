//
// <treebar.cpp>
//

#include "treebar.h"
#include "phyedit.h"
#include "resource.h"
#include "extutil.h"
#include "mingeom.h"

HWND        TreeBar::handle = NULL;
HFONT       TreeBar::font = NULL;

#include <commctrl.h>

BOOL CALLBACK extent_select_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
			if(!physicsEditor.object.mesh)	//disable convex mesh button on non mesh object
			{
				EnableWindow(GetDlgItem(hwnd, IDC_RADIO_CONVEX), FALSE);
			}
            return TRUE;

        case WM_COMMAND:
            switch LOWORD(wParam)
            {
                case IDOK:

                    if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_SPHERE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        SphereExtent * sp = new SphereExtent;
                        sphereEditor.init(sp, TRUE);
                        physicsEditor.free_extent_list[physicsEditor.free_extent_count] = sp;
                        physicsEditor.free_extent_editors[physicsEditor.free_extent_count++] = &sphereEditor;
                        physicsEditor.current_editor = (EditMode *) &sphereEditor;
                        
                        TreeBar::refresh_extent_list();
                    }
                    else
                    if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_BOX), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        BoxExtent * x = new BoxExtent;
                        boxEditor.init(x, TRUE);
                        physicsEditor.free_extent_list[physicsEditor.free_extent_count] = x;
                        physicsEditor.free_extent_editors[physicsEditor.free_extent_count++] = &boxEditor;
                        physicsEditor.current_editor = (EditMode *) &boxEditor;
                        
                        TreeBar::refresh_extent_list();
                    }
                    else
                    if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_CONVEX), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        ConvexMeshExtent * cm = new ConvexMeshExtent;
                        convexMeshEditor.init(cm, TRUE);
                        physicsEditor.free_extent_list[physicsEditor.free_extent_count] = cm;
                        physicsEditor.free_extent_editors[physicsEditor.free_extent_count++] = &convexMeshEditor;
                        physicsEditor.current_editor = (EditMode *) &convexMeshEditor;
                        
                        TreeBar::refresh_extent_list();
                    }
					else
					if (SendMessage(GetDlgItem(hwnd, IDC_RADIO_TUBE), BM_GETCHECK, 0, 0) == BST_CHECKED)
					{
						TubeExtent * t = new TubeExtent;
						tubeEditor.init(t, TRUE);
						physicsEditor.free_extent_list[physicsEditor.free_extent_count] = t;
                        physicsEditor.free_extent_editors[physicsEditor.free_extent_count++] = &tubeEditor;
                        physicsEditor.current_editor = (EditMode *) &tubeEditor;
                        
                        TreeBar::refresh_extent_list();
					}

                    EndDialog(hwnd, 0);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, -1);
                    break;
            }
            break;
    }

    return FALSE;
}

static char extent_name[1024];

BOOL CALLBACK name_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch LOWORD(wParam)
            {
                case IDC_OK:
					SendMessage(GetDlgItem(hwnd, IDC_NAME_EDIT), WM_GETTEXT, 1024, (LPARAM) extent_name);
					EndDialog(hwnd, TRUE);
					break;
				
				case IDC_CANCEL:
					EndDialog(hwnd, FALSE);
					break;
			}
			break;

	}

	return FALSE;
}


BOOL CALLBACK callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            return TRUE;

		case WM_NOTIFY:

			switch LOWORD(wParam)
			{
				case IDC_TREE_LIST:
					
					LPNMHDR hdr = (LPNMHDR) lParam;

					// switch back into main edit mode
                    physicsEditor.current_editor = NULL;

					switch (hdr->code)
					{
						case TVN_SELCHANGED:

							LPNM_TREEVIEW pn = (NM_TREEVIEW FAR *) lParam;
							
							physicsEditor.selected_extent	=(BaseExtent *) pn->itemNew.lParam;
							if(physicsEditor.cmp_edit)
							{
								physicsEditor.CurObject	=physicsEditor.FindExtentOwner(physicsEditor.selected_extent);
							}
							break;
					}
			}
			
			break;

		case WM_COMMAND:

            switch LOWORD(wParam)
            {
                case IDC_ADD_ROOT:
                {
                    S32 index = SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_GETCURSEL, 0, 0);
                    
                    if (index != -1)
                    {
                        if (physicsEditor.CurObject->tree != NULL)
                        {
                            MessageBox(windowHandle, "Root node already exists.", "Error", MB_OK);
                        }
                        else
                        {
                            // add extent to tree
							physicsEditor.CurObject->tree = physicsEditor.free_extent_list[physicsEditor.selected_free_extent];
                        
                            // remove extent from list and lower window
                            for (int i = physicsEditor.selected_free_extent; i < (physicsEditor.free_extent_count - 1); i++)
                            {
                                physicsEditor.free_extent_list[i] = physicsEditor.free_extent_list[i + 1];
                                physicsEditor.free_extent_editors[i] = physicsEditor.free_extent_editors[i + 1];
                            }

                            physicsEditor.free_extent_count--;

                            // refresh
                            TreeBar::refresh_all_windows();
                        }

                    }

                    break;
                }

                case IDC_ADD_CHILD:
                {
                    S32 index = SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_GETCURSEL, 0, 0);
                    
                    if (index != -1)
                    {
                        if (physicsEditor.CurObject->tree == NULL)
                        {
                            MessageBox(windowHandle, "Tree contains no root node.", "Error", MB_OK);
                        }
                        else
                        {
                            BaseExtent * ex = physicsEditor.selected_extent;
                            BOOL32 result;
                            
                            if (ex != NULL)
                            {
                                result = add_extent_as_child(ex, physicsEditor.free_extent_list[physicsEditor.selected_free_extent]);

                                // if we can't add as child directly, add as the next of the child
                                if (result == FALSE)
                                    add_extent_as_next(ex->child, physicsEditor.free_extent_list[physicsEditor.selected_free_extent]);

                                // remove extent from list and lower window
                                for (int i = physicsEditor.selected_free_extent; i < (physicsEditor.free_extent_count - 1); i++)
                                {
                                    physicsEditor.free_extent_list[i] = physicsEditor.free_extent_list[i + 1];
                                    physicsEditor.free_extent_editors[i] = physicsEditor.free_extent_editors[i + 1];
                                }

                                physicsEditor.free_extent_count--;

                                // refresh
                                TreeBar::refresh_all_windows();
                            }
                            else
                            {
                                MessageBox(windowHandle, "Select a node in the upper window to link to.", "Error", MB_OK);
                            }
                        }
                    }
                    else
                    {
                        MessageBox(windowHandle, "Select a node in the lower window to connect.", "Error", MB_OK);
                    }

                    break;
                }

                case IDC_POP_NODE:

                    // fix this

                /*
                    // pop the node back onto the free extent list
                    if (physicsEditor.selected_extent != NULL)
                    {
                        // pop all extents in this subtree
                        
                        
                        // remove subtree from the top
                        remove_extent(physicsEditor.CurObject->tree, physicsEditor.selected_extent, TRUE);

                        // add to bottom
                        physicsEditor.free_extent_list[physicsEditor.free_extent_count++] = physicsEditor.selected_extent;
                        physicsEditor.selected_extent = NULL;

                        // refresh
                        TreeBar::refresh_all_windows();
                    }
*/
                    break;

                case IDC_DELETE_NODE:

					// FIX DELETE

                    if (physicsEditor.selected_extent)
                    {
                        if (MessageBox(windowHandle, "Delete this extent node?", "Delete", MB_YESNO) == IDYES)
                        {
							if (physicsEditor.selected_extent == physicsEditor.CurObject->tree)
                            {
                                remove_extent_tree(physicsEditor.CurObject->tree);
                                physicsEditor.CurObject->tree = NULL;
                            }
                            else
                            {
                                remove_extent(physicsEditor.CurObject->tree, physicsEditor.selected_extent);
                            }

                            physicsEditor.selected_extent = NULL;
                            
							TreeBar::display_master_tree();
                            
                        }
                    }
                    
                    break;

				case IDC_NAME_ENTRY:

					if (physicsEditor.selected_extent)
					{
						if (DialogBox(appInstance, MAKEINTRESOURCE(IDD_NAME_ENTRY), windowHandle, (DLGPROC) name_proc) == TRUE)
						{
							S32 sl = strlen(extent_name);

							if (sl > 0)
							{
								physicsEditor.selected_extent->name = (char *) malloc(sl + 1);
								strcpy(physicsEditor.selected_extent->name, extent_name);
							}
							else
							{
								physicsEditor.selected_extent->name = NULL;
							}
							
							// refresh
                            TreeBar::refresh_all_windows();
                    
						}
					}
					break;

                case IDC_DELETE_ALL:

                    if (MessageBox(windowHandle, "Delete all extents?", "Delete", MB_YESNO) == IDYES)
                    {
                        remove_extent_tree(physicsEditor.CurObject->tree);
                        physicsEditor.CurObject->tree = NULL;
						physicsEditor.selected_extent = NULL;
                        TreeBar::display_master_tree();
                    }
                    
                    break;
            
                case IDC_CREATE_FREE:
                    
					if (physicsEditor.object.index != INVALID_INSTANCE_INDEX)
						DialogBox(appInstance, MAKEINTRESOURCE(IDD_EXTENT_TYPE), windowHandle, (DLGPROC) extent_select_proc);
                    
                    break;

                case IDC_DELETE_FREE:
                {
					if(physicsEditor.selected_free_extent != -1)
					{
						delete physicsEditor.free_extent_list[physicsEditor.selected_free_extent];
                        
						for (int i = physicsEditor.selected_free_extent; i < physicsEditor.free_extent_count - 1; i++)
						{
							physicsEditor.free_extent_list[i] = physicsEditor.free_extent_list[i + 1];
							physicsEditor.free_extent_editors[i] = physicsEditor.free_extent_editors[i + 1];
						}

						physicsEditor.free_extent_count--;
                  
						physicsEditor.selected_free_extent = -1;
						physicsEditor.current_editor = NULL;
                    
						SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_SETCURSEL, 0, 0);

						// refresh
						TreeBar::refresh_all_windows();
					}
                    
                    break;
                }

                case IDC_DELETE_ALL_FREE:
                {                    
                    for (int i = 0; i < physicsEditor.free_extent_count; i++)
                    {
                        delete physicsEditor.free_extent_list[i];
                        physicsEditor.free_extent_list[i] = NULL;
                        physicsEditor.free_extent_editors[i] = NULL;
                    }

                    physicsEditor.free_extent_count = 0;
                  
                    physicsEditor.selected_free_extent = -1;
                    physicsEditor.current_editor = NULL;
                    
                    SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_SETCURSEL, 0, 0);

                    // refresh
                    TreeBar::refresh_all_windows();
                    
                    break;
                }

                case IDC_EXTENT_LIST:

                    S32 index = SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_GETCURSEL, 0, 0);
                    
                    if (index != -1)
                    {
                        physicsEditor.selected_free_extent = index;
                    
                        physicsEditor.current_editor = physicsEditor.free_extent_editors[index];
                        physicsEditor.current_editor->init(physicsEditor.free_extent_list[index], FALSE);
                    }
                    
                    break;

            }

    }
    
    return FALSE;
}


BOOL32 TreeBar::create()
{
    RECT mainRect;
    RECT treeRect;

    handle = CreateDialog(appInstance, MAKEINTRESOURCE(IDD_DIALOG1), windowHandle, (DLGPROC) callback);

    if (!handle)
    {
        AppFatal("Unable to create dialog resource.");
    }

    GetWindowRect(handle, &treeRect);
    
    GetWindowRect(windowHandle, &mainRect);

    MoveWindow(handle, mainRect.right, mainRect.top, treeRect.right - treeRect.left, treeRect.bottom - treeRect.top, TRUE);
    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);
     
    return TRUE;
}

BOOL32 TreeBar::destroy()
{
    DeleteObject(font);

    return DestroyWindow(handle);
}

void TreeBar::refresh()
{
    RECT mainRect;
    RECT treeRect;
    
    GetWindowRect(handle, &treeRect);
    GetWindowRect(windowHandle, &mainRect);

    MoveWindow(handle, mainRect.right, mainRect.top, treeRect.right - treeRect.left, treeRect.bottom - treeRect.top, TRUE);
    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);

    display_master_tree();

}

void	select_extent_treeitem(BaseExtent *xt, HWND tList, HTREEITEM hti)
{
	HTREEITEM	ht	=TreeView_GetChild(tList, hti);
	TVITEM		tvi;

	if(ht)
	{
		select_extent_treeitem(xt, tList, ht);
	}

	tvi.hItem	=hti;
	tvi.mask	=TVIF_HANDLE;
	TreeView_GetItem(tList, &tvi);

	if((struct BaseExtent *)tvi.lParam == xt)
	{
		TreeView_SelectItem(tList, hti);
		return;	//early out
	}

	ht	=TreeView_GetNextSibling(tList, hti);
	if(ht)
	{
		select_extent_treeitem(xt, tList, ht);
	}
}

void recurse_tree_draw(BaseExtent * extent, HWND tList, HTREEITEM parent, U32 offset)
{
    char string[512];
	memset(string, 0, sizeof(string));

    switch (extent->type)
    {
        case ET_SPHERE:
            strcat(string, "Sphere");
            break;

        case ET_BOX:
            strcat(string, "Box");
            break;

        case ET_CONVEX_MESH:
            strcat(string, "Convex mesh");
            break;

        case ET_GENERAL_MESH:
            strcat(string, "General mesh");
            break;
    }

	if (extent->name != NULL)
	{
		char work_string[256];
		sprintf(work_string, " (name: %s)", extent->name);
		strcat(string, work_string);
	}

    U32 list_index = SendMessage(tList, LB_ADDSTRING, 0, (LPARAM) string);

	TV_ITEM			item;
	item.mask		= TVIF_CHILDREN | TVIF_STATE | TVIF_TEXT | TVIF_PARAM;
	item.cChildren	= (extent->child) ? 1 : 0;
	item.stateMask	= 0;
	item.pszText	= string;
	item.cchTextMax = 512;
	item.lParam		= (LPARAM) extent;

	TV_INSERTSTRUCT tv;
	tv.hParent = parent;
	tv.hInsertAfter = TVI_LAST;
	tv.item = item;

	HTREEITEM ht = TreeView_InsertItem(tList, &tv);

    if (extent->child)
    {
        recurse_tree_draw(extent->child, tList, ht, offset + 1);
        
    }
    
    if (extent->next)
    {
        recurse_tree_draw(extent->next, tList, parent, offset);
    }

}


void TreeBar::display_master_tree()
{
	int		i;
    HWND	tList	= GetDlgItem(handle, IDC_TREE_LIST);

	TreeView_DeleteAllItems(tList);

	if(physicsEditor.cmp_edit)
	{
		for(i=0;i < physicsEditor.NumChildObjects;i++)
		{
			if(physicsEditor.ChildObjects[i].tree)
			{
				recurse_tree_draw(physicsEditor.ChildObjects[i].tree, tList, NULL, 0);
			}
		}
	}
	else if(physicsEditor.object.tree)
	{
		recurse_tree_draw(physicsEditor.object.tree, tList, NULL, 0);
	}
}

void TreeBar::refresh_extent_list()
{
    SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_RESETCONTENT, 0, 0);

    for (int i = 0; i < physicsEditor.free_extent_count; i++)
    {
        BaseExtent * ex = physicsEditor.free_extent_list[i];

        switch (ex->type)
        {
            case ET_SPHERE:
                SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_ADDSTRING, 0, (LPARAM) "Sphere");
                break;

            case ET_BOX:
                SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_ADDSTRING, 0, (LPARAM) "Box");
                break;

            case ET_CONVEX_MESH:
                SendMessage(GetDlgItem(TreeBar::handle, IDC_EXTENT_LIST), LB_ADDSTRING, 0, (LPARAM) "Convex mesh");
                break;

        }
    }
    
}

void TreeBar::refresh_all_windows()
{
    // recreate tree
    display_master_tree();

    // refresh extent list
    refresh_extent_list();
    
}