#include "stdafx.h"
#include <IMaterialManager.h>
#include "InternalMaterialManager.h"
#include "Material.h"

#include "resource.h"
#include <commctrl.h>
#include <Commdlg.h>

#include <tcomponent.h>
#include <da_heap_utility.h>
#include <filesys.h>
#include <TSmartPointer.h>

extern IInternalMaterialManager* internalManager;

extern HINSTANCE g_hInstance;

char * textureBuffer = NULL;

HWND texHwnd = NULL;
HWND textureTree = NULL;

char * fileFilter = "Textures\0*.tga;*.dds\0\0";

static HIMAGELIST textureImageList;

//----------------------------------------------------------------------------------------------
//
void enumerateTextureDir(HTREEITEM parent,char * path, IFileSystem * textureDir)
{
	WIN32_FIND_DATA findData;
	HANDLE hFile = textureDir->FindFirstFile("*", &findData);
	if( hFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY  && findData.cFileName[0] != '.')
			{
				TVINSERTSTRUCT insertStruct;
				insertStruct.hParent = parent;
				insertStruct.hInsertAfter = TVI_LAST;
				insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |TVIF_SELECTEDIMAGE;
				insertStruct.itemex.lParam = 1;
				insertStruct.itemex.pszText = findData.cFileName;
				insertStruct.itemex.cchTextMax = strlen(findData.cFileName);
				insertStruct.itemex.iImage = 0;
				insertStruct.itemex.iSelectedImage = 0;
				HTREEITEM treeDir = TreeView_InsertItem(textureTree,&insertStruct);

				COMPTR<IFileSystem> subDir;
				DAFILEDESC fdesc = findData.cFileName;
				if(textureDir->CreateInstance(&fdesc,subDir) == GR_OK)
				{
					char buffer[512];
					sprintf(buffer,"%s%s\\",path,findData.cFileName);
					enumerateTextureDir(treeDir,buffer,subDir);
				}
			}
			else
			{
				if( strstr(findData.cFileName,".tga") ||  strstr(findData.cFileName,".dds") )
				{
					char fullName[512];
					sprintf(fullName,"%s%s",path,findData.cFileName);
					TVINSERTSTRUCT insertStruct;
					insertStruct.hParent = parent;
					insertStruct.hInsertAfter = TVI_LAST;
					insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE |TVIF_SELECTEDIMAGE;
					insertStruct.itemex.pszText = findData.cFileName;
					insertStruct.itemex.lParam = 0;
					insertStruct.itemex.cchTextMax = strlen(findData.cFileName);
					insertStruct.itemex.iImage = 2;
					insertStruct.itemex.iSelectedImage = 2;
					HTREEITEM entry	= TreeView_InsertItem(textureTree,&insertStruct);
					if(strcmp(fullName,textureBuffer) == 0)
					{
						TreeView_SelectItem(textureTree,entry);
					}
				}
			}
		}
		while( textureDir->FindNextFile(hFile, &findData) );
		textureDir->FindClose(hFile);
	}
}
//----------------------------------------------------------------------------------------------
//
void updateTextureList()
{
	TreeView_DeleteAllItems(textureTree);

	HTREEITEM parent = TVI_ROOT;

	COMPTR<IFileSystem> textureDir;

	DAFILEDESC fdesc = "Textures";

	if(internalManager->GetDirectory()->CreateInstance(&fdesc,textureDir) == GR_OK)
	{
		enumerateTextureDir(parent,"",textureDir);
	}
}

HTREEITEM findCurrentTextureTreeDir()
{
	HTREEITEM current = TreeView_GetSelection(textureTree);
	TVITEMEX item;
	item.mask = TVIF_PARAM | TVIF_HANDLE;
	item.hItem = current;
	item.lParam = NULL;
	TreeView_GetItem(textureTree,&item);

	if(item.lParam)
	{
		return current;
	}

	return TreeView_GetParent(textureTree,current);
}

void importImage()
{
	char importFile[512];
	importFile[0] = 0;
	OPENFILENAME open;
	memset(&open,0,sizeof(OPENFILENAME));
	open.lStructSize = sizeof(OPENFILENAME);
	open.hwndOwner = texHwnd;
	open.lpstrFilter = fileFilter;
	open.lpstrFile = importFile;
	open.nMaxFile = 511;
	open.lpstrTitle = "Import";
	open.Flags = OFN_FILEMUSTEXIST |OFN_PATHMUSTEXIST;
	if(GetOpenFileName(&open))
	{
		//build the internal path
		char internalPath[256];
		internalPath[0] = 0;

		HTREEITEM currentDir = findCurrentTextureTreeDir();
		char itemName[256];
		while(currentDir)
		{
			TVITEMEX item;
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.hItem = currentDir;
			item.pszText = itemName;
			item.cchTextMax = 255;
			TreeView_GetItem(textureTree,&item);
			char buffer[256];
			sprintf(buffer,"%s\\%s",item.pszText,internalPath);
			strcpy(internalPath,buffer);
			currentDir = TreeView_GetParent(textureTree,currentDir);
		}

		char externalPath[512];
		internalManager->GetDirectory()->GetFileName(externalPath,511);

		//now put it together
		// matDirPath + "Textures" + internalPath + filename
		char targetFile[512];

		sprintf(targetFile,"%s\\Textures\\%s%s",externalPath,
			internalPath,importFile+open.nFileOffset);
		
		CopyFile(importFile,targetFile,false);

		updateTextureList();
	}
}

void createTextureDir()
{
	//build the internal path
	char internalPath[256];
	internalPath[0] = 0;

	HTREEITEM currentDir = findCurrentTextureTreeDir();
	char itemName[256];
	while(currentDir)
	{
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE;
		item.hItem = currentDir;
		item.pszText = itemName;
		item.cchTextMax = 255;
		TreeView_GetItem(textureTree,&item);
		char buffer[256];
		sprintf(buffer,"%s\\%s",item.pszText,internalPath);
		strcpy(internalPath,buffer);
		currentDir = TreeView_GetParent(textureTree,currentDir);
	}

	char externalPath[512];
	internalManager->GetDirectory()->GetFileName(externalPath,511);

	//now put it together
	// matDirPath + "Textures" + internalPath + dirName
	char targetFile[512];

	sprintf(targetFile,"%s\\Textures\\%s%s",externalPath,
		internalPath,"NewName");

	CreateDirectory(targetFile,NULL);

	updateTextureList();
}

void deleteSelected()
{
	HTREEITEM current = TreeView_GetSelection(textureTree);
	if(current)
	{
		char internalPath[512];
		char itemName[256];
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE |TVIF_PARAM;
		item.hItem = current;
		item.pszText = itemName;
		item.cchTextMax = 255;
		item.lParam = 0;
		TreeView_GetItem(textureTree,&item);
		strcpy(internalPath,item.pszText);

		bool bDirectory = (item.lParam!=0);

		current = TreeView_GetParent(textureTree,current);
		while(current)
		{
			TVITEMEX item;
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.hItem = current;
			item.pszText = itemName;
			item.cchTextMax = 255;
			TreeView_GetItem(textureTree,&item);
			char buffer[256];
			sprintf(buffer,"%s\\%s",item.pszText,internalPath);
			strcpy(internalPath,buffer);
			current = TreeView_GetParent(textureTree,current);
		}

		char externalPath[512];
		internalManager->GetDirectory()->GetFileName(externalPath,511);

		//now put it together
		// matDirPath + "Textures" + internalPath
		char targetFile[512];

		sprintf(targetFile,"%s\\Textures\\%s",externalPath,internalPath);

		if(bDirectory)
		{
			RemoveDirectory(targetFile);
		}
		else
		{
			DeleteFile(targetFile);
		}
		updateTextureList();
	}
}

void replaceTexture()
{
	HTREEITEM current = TreeView_GetSelection(textureTree);
	if(current)
	{
		char internalPath[512];
		char itemName[256];
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE |TVIF_PARAM;
		item.hItem = current;
		item.pszText = itemName;
		item.cchTextMax = 255;
		item.lParam = 0;
		TreeView_GetItem(textureTree,&item);
		strcpy(internalPath,item.pszText);

		if(item.lParam == 0)//not a directory
		{
			current = TreeView_GetParent(textureTree,current);
			while(current)
			{
				TVITEMEX item;
				item.mask = TVIF_TEXT | TVIF_HANDLE;
				item.hItem = current;
				item.pszText = itemName;
				item.cchTextMax = 255;
				TreeView_GetItem(textureTree,&item);
				char buffer[256];
				sprintf(buffer,"%s\\%s",item.pszText,internalPath);
				strcpy(internalPath,buffer);
				current = TreeView_GetParent(textureTree,current);
			}

			char externalPath[512];
			internalManager->GetDirectory()->GetFileName(externalPath,511);

			//now put it together
			// matDirPath + "Textures" + internalPath
			char targetFile[512];

			sprintf(targetFile,"%s\\Textures\\%s",externalPath,internalPath);


			char importFile[512];
			importFile[0] = 0;
			OPENFILENAME open;
			memset(&open,0,sizeof(OPENFILENAME));
			open.lStructSize = sizeof(OPENFILENAME);
			open.hwndOwner = texHwnd;
			open.lpstrFilter = fileFilter;
			open.lpstrFile = importFile;
			open.nMaxFile = 511;
			open.lpstrTitle = "Replace With";
			open.Flags = OFN_FILEMUSTEXIST |OFN_PATHMUSTEXIST;
			if(GetOpenFileName(&open))
			{
				CopyFile(importFile,targetFile,false);

				updateTextureList();
			}
		}
	}
}

INT_PTR CALLBACK TextureLibDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			if(!textureImageList)
			{
				textureImageList= ImageList_Create(16,16,ILC_COLOR32|ILC_MASK ,0,1);

				HICON bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_CLOSEDFOLDER),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(textureImageList,bitmap);

				bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_OPENFOLDER),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(textureImageList,bitmap);

				bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MATERIAL),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(textureImageList,bitmap);
			}

			texHwnd = hwndDlg;
			textureTree = GetDlgItem(hwndDlg,IDC_TEXTURE_TREE);
			TreeView_SetImageList(textureTree,textureImageList,TVSIL_NORMAL);

			updateTextureList();
			ShowWindow(hwndDlg,true);
			return 1;
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_CANCEL))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					EndDialog(hwndDlg,0);//return cancel
					break;
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_SELECT))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					EndDialog(hwndDlg,1);//return ok
					break;
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_IMPORT))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					importImage();
					break;
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_DELETE))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					deleteSelected();
					break;
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_REPLACE))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					replaceTexture();
					break;
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_NEW_DIR))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					createTextureDir();
					break;
				}
			}
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR * hdr = (NMHDR*) lParam;
			if(hdr->hwndFrom == textureTree)
			{
				NMTREEVIEW * treeHdr = (NMTREEVIEW *)hdr;
				switch(hdr->code)
				{
				case TVN_SELCHANGED:
					{
						if(treeHdr->itemNew.lParam)//directory
						{
							EnableWindow(GetDlgItem(hwndDlg,IDC_SELECT),false);
							EnableWindow(GetDlgItem(hwndDlg,IDC_REPLACE),false);
							textureBuffer[0] = 0;
						}
						else
						{
							EnableWindow(GetDlgItem(hwndDlg,IDC_SELECT),true);
							EnableWindow(GetDlgItem(hwndDlg,IDC_REPLACE),true);
							char itemName[256];
							TVITEMEX item;
							item.mask = TVIF_TEXT | TVIF_HANDLE;
							item.hItem = treeHdr->itemNew.hItem;
							item.pszText = itemName;
							item.cchTextMax = 255;
							TreeView_GetItem(textureTree,&item);
							strcpy(textureBuffer,item.pszText);

							HTREEITEM current = TreeView_GetParent(textureTree,treeHdr->itemNew.hItem);
							while(current)
							{
								TVITEMEX item;
								item.mask = TVIF_TEXT | TVIF_HANDLE;
								item.hItem = current;
								item.pszText = itemName;
								item.cchTextMax = 255;
								TreeView_GetItem(textureTree,&item);
								char buffer[256];
								sprintf(buffer,"%s\\%s",item.pszText,textureBuffer);
								strcpy(textureBuffer,buffer);
								current = TreeView_GetParent(textureTree,current);
							}
						}
					}
					break;
				case TVN_ITEMEXPANDED:
					{
						TVITEMEX item;
						item.mask = TVIF_HANDLE | TVIF_IMAGE |TVIF_SELECTEDIMAGE ;
						item.hItem = treeHdr->itemNew.hItem;
						if(treeHdr->action == TVE_COLLAPSE)
						{
							item.iImage =0;
							item.iSelectedImage =0;
						}
						else
						{
							item.iImage =1;
							item.iSelectedImage =1;
						}

						TreeView_SetItem(textureTree,&item);
					}
					break;

				}
			}
		}
		break;
	case WM_CLOSE:
		{
			EndDialog(hwndDlg,0);//return cancel
		}
		break;

	}
	return 0;
}

bool ChooseTexture(char * textureName,HWND pWin)
{
	textureBuffer = textureName;
	if(::DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_TEXTURE_LIB), pWin, TextureLibDialogProc))
		return true;
	return false;
}