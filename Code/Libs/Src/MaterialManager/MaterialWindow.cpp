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

IInternalMaterialManager* internalManager = NULL;

Material * selectedMat = NULL;

extern HINSTANCE g_hInstance;

static HWND mainHwnd;
static HWND matTree;
static HWND headerWin;
static HWND workWin;
static HIMAGELIST matImageList;
static HTREEITEM dragItem;

bool bUpdateingDisplay = false;//set to true when code is updating vlaues in the display so we can throuw away excess changes
bool g_fDragging = false;//true when dragging materials around

#define MAT_TREE_WIDTH 200
#define HEADER_HEIGHT 60

//custom windows message;
#define WM_UPDATE_MAT_DATA (WM_USER+100)

struct DataWinNode
{
	DataWinNode * next;
	DataNode * data;
	HWND dlg;
	U32 offsetY;
};

DataWinNode * dataWinList = NULL;
U32 dataWinHeight = 0;
S32 dataWinOffset = 0;
bool bAdvancedMode = false;
bool bUpdateingMatTree = 0;

COLORREF customColors[16];

bool ChooseTexture(char * textureName,HWND pWin);

//----------------------------------------------------------------------------------------------
//
void materialParamChanged()
{
	DataWinNode * search = dataWinList;
	while(search)
	{
		SendMessage(search->dlg,WM_UPDATE_MAT_DATA,0,0);
		search = search->next;
	}
}

void alignMaterialControls()
{
	RECT rect;
	GetClientRect(mainHwnd,&rect);

	MoveWindow(matTree,0,0,MAT_TREE_WIDTH,rect.bottom-rect.top,false);

	MoveWindow(headerWin,MAT_TREE_WIDTH,0,rect.right-(rect.left+MAT_TREE_WIDTH),HEADER_HEIGHT,false);

	MoveWindow(workWin,MAT_TREE_WIDTH,HEADER_HEIGHT,rect.right-(rect.left+MAT_TREE_WIDTH),rect.bottom-(rect.top+HEADER_HEIGHT),false);

	GetClientRect(workWin,&rect);
	U32 viewHeight = rect.bottom-rect.top;
	if(viewHeight < dataWinHeight)
	{
		EnableScrollBar(workWin,SB_VERT,ESB_ENABLE_BOTH);
		SetScrollRange(workWin,SB_VERT,0,dataWinHeight-viewHeight,true);
		dataWinOffset = GetScrollPos(workWin,SB_VERT);
	}
	else
	{
		EnableScrollBar(workWin,SB_VERT,ESB_DISABLE_BOTH);
		dataWinOffset = 0;
	}

	DataWinNode * search = dataWinList;
	while(search)
	{
		RECT dlgRect;
		GetWindowRect(search->dlg,&dlgRect);
		MoveWindow(search->dlg,0,search->offsetY-dataWinOffset,rect.right-rect.left,dlgRect.bottom-dlgRect.top,false);
		InvalidateRect(search->dlg,NULL,false);
		search = search->next;
	}


	InvalidateRect(mainHwnd,NULL,false);
	InvalidateRect(workWin,NULL,false);
	InvalidateRect(headerWin,NULL,false);//don't know why I need this but ahh well...
}
//----------------------------------------------------------------------------------------------
//
void enumerateMatDir(HTREEITEM parent,char * path, IFileSystem * matDir)
{
	WIN32_FIND_DATA findData;
	HANDLE hFile = matDir->FindFirstFile("*", &findData);
	if( hFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY  && findData.cFileName[0] != '.')
			{
				TVINSERTSTRUCT insertStruct;
				insertStruct.hParent = parent;
				insertStruct.hInsertAfter = TVI_LAST;
				insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				insertStruct.itemex.lParam = NULL;
				insertStruct.itemex.pszText = findData.cFileName;
				insertStruct.itemex.cchTextMax = strlen(findData.cFileName);
				insertStruct.itemex.iImage =0;
				insertStruct.itemex.iSelectedImage =0;
				HTREEITEM treeDir = TreeView_InsertItem(matTree,&insertStruct);

				COMPTR<IFileSystem> subDir;
				DAFILEDESC fdesc = findData.cFileName;
				if(matDir->CreateInstance(&fdesc,subDir) == GR_OK)
				{
					char buffer[512];
					sprintf(buffer,"%s%s\\",path,findData.cFileName);
					enumerateMatDir(treeDir,buffer,subDir);
				}
			}
			else
			{
				if( strstr(findData.cFileName,".txt") )
				{
					char fullName[512];
					sprintf(fullName,"%s%s",path,findData.cFileName);
					Material * mat = internalManager->FindMaterial(fullName);
					char strname[256];
					strcpy(strname,findData.cFileName);
					strstr(strname,".txt")[0] = 0;
					TVINSERTSTRUCT insertStruct;
					insertStruct.hParent = parent;
					insertStruct.hInsertAfter = TVI_LAST;
					insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM| TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					insertStruct.itemex.pszText = strname;
					insertStruct.itemex.lParam = (DWORD) mat;
					insertStruct.itemex.cchTextMax = strlen(strname);
					insertStruct.itemex.iImage =2;
					insertStruct.itemex.iSelectedImage =2;
					TreeView_InsertItem(matTree,&insertStruct);
				}
			}
		}
		while( matDir->FindNextFile(hFile, &findData) );
		matDir->FindClose(hFile);
	}

}
//----------------------------------------------------------------------------------------------
//
void updateMaterialList()
{
	bUpdateingMatTree = true;
	TreeView_DeleteAllItems(matTree);

	HTREEITEM parent = TVI_ROOT;

	COMPTR<IFileSystem> matDir;

	DAFILEDESC fdesc = "Materials";

	if(internalManager->GetDirectory()->CreateInstance(&fdesc,matDir) == GR_OK)
	{
		enumerateMatDir(parent,"",matDir);
	}
	bUpdateingMatTree = false;
}
//----------------------------------------------------------------------------------------------
//
U32 findComboboxIndex(HWND combo, DWORD data)
{
	U32 count = ::SendMessage(combo,CB_GETCOUNT,0,0);
	for(U32 i = 0; i < count; ++i)
	{
		if(::SendMessage(combo,CB_GETITEMDATA,i,0) == data)
			return i;
	}
	return -1;
}
//----------------------------------------------------------------------------------------------
//
HTREEITEM findCurrentMaterialTreeDir()
{
	HTREEITEM current = TreeView_GetSelection(matTree);
	TVITEMEX item;
	item.mask = TVIF_PARAM | TVIF_HANDLE;
	item.hItem = current;
	item.lParam = NULL;
	TreeView_GetItem(matTree,&item);

	if(!item.lParam)
	{
		return current;
	}

	return TreeView_GetParent(matTree,current);
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK FloatEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			SINGLE value = selectedMat->GetFloat(node->data->data.floatData.rType,node->data->data.floatData.rValue);

			bUpdateingDisplay = true;//not the user changing values

			char buffer[128];
			sprintf(buffer,"%f",value);
			SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);


			SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETRANGE,false,MAKELONG(0,100));
			S32 pos = (S32)(((value-node->data->data.floatData.min)*100) / (node->data->data.floatData.max-node->data->data.floatData.min));
			SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,pos);

			bUpdateingDisplay = false;
			
			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(node && selectedMat)
			{
				SINGLE value = selectedMat->GetFloat(node->data->data.floatData.rType,node->data->data.floatData.rValue);
				bUpdateingDisplay = true;//not the user changing values
				char buffer[128];
				sprintf(buffer,"%f",value);
				SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);


				SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETRANGE,false,MAKELONG(0,100));
				S32 pos = (S32)(((value-node->data->data.floatData.min)*100) / (node->data->data.floatData.max-node->data->data.floatData.min));
				SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,pos);

				bUpdateingDisplay = false;
			}
		}
		break;
	case WM_HSCROLL:
		{
			if(!bUpdateingDisplay)
			{
				DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
				if(node)
				{
					HWND scroll = (HWND)lParam;
					U32 command = LOWORD(wParam);
					U32 posiblePos = HIWORD(wParam);
					if(scroll == GetDlgItem(hwndDlg,IDC_VALUE_SLIDER))
					{
						U32 pos = SendMessage(scroll,TBM_GETPOS,0,0);
						switch(command)
						{
						case SB_LINELEFT:
							if(pos > 0)
								pos--;
							break;
						case SB_LINERIGHT:
							if(pos < 100)
								++pos;
							break;
						case SB_PAGELEFT:
							if(pos > 10)
								pos -= 10;
							else
								pos = 0;
							break;
						case SB_PAGERIGHT:
							if(pos < 91)
								pos += 10;
							else
								pos = 100;
							break;
						case SB_THUMBPOSITION:
						case SB_THUMBTRACK:
							pos = posiblePos;
							break;
						}

						SINGLE value = node->data->data.floatData.min+((((SINGLE)pos)*(node->data->data.floatData.max-node->data->data.floatData.min))/100);

						selectedMat->SetFloat(node->data->data.floatData.rType,node->data->data.floatData.rValue,value);

						bUpdateingDisplay = true;//not the user changing values

						SendMessage(scroll,TBM_SETPOS,true,pos);

						char buffer[128];
						sprintf(buffer,"%f",value);
						SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);

						bUpdateingDisplay = false;
						
						materialParamChanged();
					}
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_VALUE))
			{
				if(!bUpdateingDisplay)
				{
					switch(dwCode)
					{
					case EN_CHANGE:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer,255);
								buffer[255] = 0;
								SINGLE value = (SINGLE)atof(buffer);
								if(value > node->data->data.floatData.max)
									value = node->data->data.floatData.max;
								else if(value < node->data->data.floatData.min)
									value = node->data->data.floatData.min;

								selectedMat->SetFloat(node->data->data.floatData.rType,node->data->data.floatData.rValue,value);

								bUpdateingDisplay = true;
								S32 pos = (S32)(((value-node->data->data.floatData.min)*100) / (node->data->data.floatData.max-node->data->data.floatData.min));
								SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,pos);
								bUpdateingDisplay = false;
//								materialParamChanged();
							}
						} 
						break;
					case EN_KILLFOCUS:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer,255);
								buffer[255] = 0;
								SINGLE value = (SINGLE)atof(buffer);
								if(value > node->data->data.floatData.max)
									value = node->data->data.floatData.max;
								else if(value < node->data->data.floatData.min)
									value = node->data->data.floatData.min;

								selectedMat->SetFloat(node->data->data.floatData.rType,node->data->data.floatData.rValue,value);

								bUpdateingDisplay = true;
								sprintf(buffer,"%f",value);
								SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);

								S32 pos = (S32)(((value-node->data->data.floatData.min)*100) / (node->data->data.floatData.max-node->data->data.floatData.min));
								SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,pos);

								bUpdateingDisplay = false;
								materialParamChanged();
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
#define MIN_FRAME_RATE 0.01f
#define MAX_FRAME_RATE 100.0f

INT_PTR CALLBACK AnimUVEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			U32 columns = selectedMat->GetAnimUVColumns();
			U32 rows = selectedMat->GetAnimUVRows();
			SINGLE frameRate = selectedMat->GetAnimUVFrameRate();

			bUpdateingDisplay = true;//not the user changing values

			char buffer[128];
			sprintf(buffer,"%d",columns);
			SetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer);

			SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETRANGE,false,MAKELONG(1,256));
			SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETPOS,true,columns);

			sprintf(buffer,"%d",rows);
			SetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer);

			SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETRANGE,false,MAKELONG(1,256));
			SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETPOS,true,rows);

			sprintf(buffer,"%f",frameRate);
			SetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer);

			SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETRANGE,false,MAKELONG(0,100));
			S32 pos = (S32)(((frameRate-MIN_FRAME_RATE)*100) / (MAX_FRAME_RATE-MIN_FRAME_RATE));
			SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETPOS,true,pos);

			bUpdateingDisplay = false;
			
			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(node && selectedMat)
			{
				U32 columns = selectedMat->GetAnimUVColumns();
				U32 rows = selectedMat->GetAnimUVRows();
				SINGLE frameRate = selectedMat->GetAnimUVFrameRate();
				bUpdateingDisplay = true;//not the user changing values
				char buffer[128];
				sprintf(buffer,"%d",columns);
				SetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer);

				SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETRANGE,false,MAKELONG(1,256));
				SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETPOS,true,columns);

				sprintf(buffer,"%d",rows);
				SetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer);

				SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETRANGE,false,MAKELONG(1,256));
				SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETPOS,true,rows);

				sprintf(buffer,"%f",frameRate);
				SetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer);

				SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETRANGE,false,MAKELONG(0,100));
				S32 pos = (S32)(((frameRate-MIN_FRAME_RATE)*100) / (MAX_FRAME_RATE-MIN_FRAME_RATE));
				SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETPOS,true,pos);

				bUpdateingDisplay = false;
			}
		}
		break;
	case WM_HSCROLL:
		{
			if(!bUpdateingDisplay)
			{
				DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
				if(node)
				{
					HWND scroll = (HWND)lParam;
					U32 command = LOWORD(wParam);
					U32 posiblePos = HIWORD(wParam);
					if(scroll == GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER))
					{
						U32 pos = SendMessage(scroll,TBM_GETPOS,0,0);
						switch(command)
						{
						case SB_LINELEFT:
							if(pos > 1)
								pos--;
							break;
						case SB_LINERIGHT:
							if(pos < 256)
								++pos;
							break;
						case SB_PAGELEFT:
							if(pos > 11)
								pos -= 10;
							else
								pos = 1;
							break;
						case SB_PAGERIGHT:
							if(pos < 247)
								pos += 10;
							else
								pos = 256;
							break;
						case SB_THUMBPOSITION:
						case SB_THUMBTRACK:
							pos = posiblePos;
							break;
						}

						selectedMat->SetAnimUVColumns(pos);

						bUpdateingDisplay = true;//not the user changing values

						SendMessage(scroll,TBM_SETPOS,true,pos);

						char buffer[128];
						sprintf(buffer,"%d",pos);
						SetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer);

						bUpdateingDisplay = false;
						
						materialParamChanged();
					}
					else if(scroll == GetDlgItem(hwndDlg,IDC_ROWS_SLIDER))
					{
						U32 pos = SendMessage(scroll,TBM_GETPOS,0,0);
						switch(command)
						{
						case SB_LINELEFT:
							if(pos > 1)
								pos--;
							break;
						case SB_LINERIGHT:
							if(pos < 256)
								++pos;
							break;
						case SB_PAGELEFT:
							if(pos > 11)
								pos -= 10;
							else
								pos = 1;
							break;
						case SB_PAGERIGHT:
							if(pos < 247)
								pos += 10;
							else
								pos = 256;
							break;
						case SB_THUMBPOSITION:
						case SB_THUMBTRACK:
							pos = posiblePos;
							break;
						}

						selectedMat->SetAnimUVRows(pos);

						bUpdateingDisplay = true;//not the user changing values

						SendMessage(scroll,TBM_SETPOS,true,pos);

						char buffer[128];
						sprintf(buffer,"%d",pos);
						SetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer);

						bUpdateingDisplay = false;
						
						materialParamChanged();
					}
					else if(scroll == GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER))
					{
						U32 pos = SendMessage(scroll,TBM_GETPOS,0,0);
						switch(command)
						{
						case SB_LINELEFT:
							if(pos > 0)
								pos--;
							break;
						case SB_LINERIGHT:
							if(pos < 100)
								++pos;
							break;
						case SB_PAGELEFT:
							if(pos > 10)
								pos -= 10;
							else
								pos = 0;
							break;
						case SB_PAGERIGHT:
							if(pos < 91)
								pos += 10;
							else
								pos = 100;
							break;
						case SB_THUMBPOSITION:
						case SB_THUMBTRACK:
							pos = posiblePos;
							break;
						}

						SINGLE value = MIN_FRAME_RATE+((((SINGLE)pos)*(MAX_FRAME_RATE-MIN_FRAME_RATE))/100);
						selectedMat->SetAnimUVFrameRate(value);

						bUpdateingDisplay = true;//not the user changing values

						SendMessage(scroll,TBM_SETPOS,true,pos);

						char buffer[128];
						sprintf(buffer,"%f",value);
						SetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer);

						bUpdateingDisplay = false;
						
						materialParamChanged();
					}
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_COLUMNS))
			{
				if(!bUpdateingDisplay)
				{
					switch(dwCode)
					{
					case EN_CHANGE:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer,255);
								buffer[255] = 0;
								U32 value = (U32)atoi(buffer);
								if(value > 256)
									value = 256;
								else if(value < 1)
									value = 1;

								selectedMat->SetAnimUVColumns(value);

								bUpdateingDisplay = true;
								SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETPOS,true,value);
								bUpdateingDisplay = false;
//								materialParamChanged();
							}
						} 
						break;
					case EN_KILLFOCUS:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer,255);
								buffer[255] = 0;
								U32 value = atoi(buffer);
								if(value > 256)
									value = 256;
								else if(value < 1)
									value = 1;

								selectedMat->SetAnimUVColumns(value);

								bUpdateingDisplay = true;
								sprintf(buffer,"%d",value);
								SetWindowText(GetDlgItem(hwndDlg,IDC_COLUMNS),buffer);

								SendMessage(GetDlgItem(hwndDlg,IDC_COLUMNS_SLIDER),TBM_SETPOS,true,value);

								bUpdateingDisplay = false;
								materialParamChanged();
							}
						}
						break;
					}
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_ROWS))
			{
				if(!bUpdateingDisplay)
				{
					switch(dwCode)
					{
					case EN_CHANGE:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer,255);
								buffer[255] = 0;
								U32 value = (U32)atoi(buffer);
								if(value > 256)
									value = 256;
								else if(value < 1)
									value = 1;

								selectedMat->SetAnimUVRows(value);

								bUpdateingDisplay = true;
								SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETPOS,true,value);
								bUpdateingDisplay = false;
//								materialParamChanged();
							}
						} 
						break;
					case EN_KILLFOCUS:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer,255);
								buffer[255] = 0;
								U32 value = atoi(buffer);
								if(value > 256)
									value = 256;
								else if(value < 1)
									value = 1;

								selectedMat->SetAnimUVRows(value);

								bUpdateingDisplay = true;
								sprintf(buffer,"%d",value);
								SetWindowText(GetDlgItem(hwndDlg,IDC_ROWS),buffer);

								SendMessage(GetDlgItem(hwndDlg,IDC_ROWS_SLIDER),TBM_SETPOS,true,value);

								bUpdateingDisplay = false;
								materialParamChanged();
							}
						}
						break;
					}
				}
			}
			else if(hWnd == GetDlgItem(hwndDlg,IDC_FRAME_RATE))
			{
				if(!bUpdateingDisplay)
				{
					switch(dwCode)
					{
					case EN_CHANGE:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer,255);
								buffer[255] = 0;
								SINGLE value = (SINGLE)atof(buffer);
								if(value > MAX_FRAME_RATE)
									value = MAX_FRAME_RATE;
								else if(value < MIN_FRAME_RATE)
									value = MIN_FRAME_RATE;

								selectedMat->SetAnimUVFrameRate(value);

								S32 pos = (S32)(((value-MIN_FRAME_RATE)*100) / (MAX_FRAME_RATE-MIN_FRAME_RATE));

								bUpdateingDisplay = true;
								SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETPOS,true,pos);
								bUpdateingDisplay = false;
//								materialParamChanged();
							}
						} 
						break;
					case EN_KILLFOCUS:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer,255);
								buffer[255] = 0;
								SINGLE value = (SINGLE)atof(buffer);
								if(value > MAX_FRAME_RATE)
									value = MAX_FRAME_RATE;
								else if(value < MIN_FRAME_RATE)
									value = MIN_FRAME_RATE;

								selectedMat->SetAnimUVFrameRate(value);

								S32 pos = (S32)(((value-MIN_FRAME_RATE)*100) / (MAX_FRAME_RATE-MIN_FRAME_RATE));

								bUpdateingDisplay = true;
								sprintf(buffer,"%f",value);
								SetWindowText(GetDlgItem(hwndDlg,IDC_FRAME_RATE),buffer);

								SendMessage(GetDlgItem(hwndDlg,IDC_FRAME_RATE_SLIDER),TBM_SETPOS,true,pos);

								bUpdateingDisplay = false;
								materialParamChanged();
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK IntEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			S32 value = selectedMat->GetInt(node->data->data.intData.rType,node->data->data.intData.rValue);

			bUpdateingDisplay = true;//not the user changing values

			char buffer[128];
			sprintf(buffer,"%d",value);
			SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);


			SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETRANGE,false,
				MAKELONG(node->data->data.intData.min,node->data->data.intData.max));
			SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,value);

			bUpdateingDisplay = false;
			
			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(node && selectedMat)
			{
				S32 value = selectedMat->GetInt(node->data->data.intData.rType,node->data->data.intData.rValue);
				bUpdateingDisplay = true;//not the user changing values

				char buffer[128];
				sprintf(buffer,"%d",value);
				SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);


				SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETRANGE,false,
					MAKELONG(node->data->data.intData.min,node->data->data.intData.max));
				SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,value);

				bUpdateingDisplay = false;
			}
		}
		break;
	case WM_HSCROLL:
		{
			if(!bUpdateingDisplay)
			{
				HWND scroll = (HWND)lParam;
				U32 command = LOWORD(wParam);
				S32 posiblePos = (S16)(HIWORD(wParam));
				if(scroll == GetDlgItem(hwndDlg,IDC_VALUE_SLIDER))
				{
					DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
					if(node)
					{
						S32 value = SendMessage(scroll,TBM_GETPOS,0,0);
						switch(command)
						{
						case SB_LINELEFT:
							if(value > node->data->data.intData.min)
								value--;
							break;
						case SB_LINERIGHT:
							if(value < node->data->data.intData.max)
								++value;
							break;
						case SB_PAGELEFT:
							if(value > node->data->data.intData.min+10)
								value -= 10;
							else
								value = node->data->data.intData.min;
							break;
						case SB_PAGERIGHT:
							if(value < node->data->data.intData.max-10)
								value += 10;
							else
								value = node->data->data.intData.max;
							break;
						case SB_THUMBPOSITION:
						case SB_THUMBTRACK:
							value = posiblePos;
							break;
						}

						selectedMat->SetInt(node->data->data.intData.rType,node->data->data.intData.rValue,value);

						bUpdateingDisplay = true;//not the user changing values

						SendMessage(scroll,TBM_SETPOS,true,value);

						char buffer[128];
						sprintf(buffer,"%d",value);
						SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);

						bUpdateingDisplay = false;
						materialParamChanged();
					}
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_VALUE))
			{
				if(!bUpdateingDisplay)
				{
					switch(dwCode)
					{
					case EN_CHANGE:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer,255);
								buffer[255] = 0;
								S32 value = atoi(buffer);
								if(value > node->data->data.intData.max)
									value = node->data->data.intData.max;
								else if(value < node->data->data.intData.min)
									value = node->data->data.intData.min;

								selectedMat->SetInt(node->data->data.intData.rType,node->data->data.intData.rValue,value);

								bUpdateingDisplay = true;
								SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,value);
								bUpdateingDisplay = false;	
								materialParamChanged();
							} 
						}
						break;
					case EN_KILLFOCUS:
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(node)
							{
								char buffer[256];
								GetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer,255);
								buffer[255] = 0;
								S32 value = atoi(buffer);
								if(value > node->data->data.intData.max)
									value = node->data->data.intData.max;
								else if(value < node->data->data.intData.min)
									value = node->data->data.intData.min;

								selectedMat->SetInt(node->data->data.intData.rType,node->data->data.intData.rValue,value);

								bUpdateingDisplay = true;
								sprintf(buffer,"%d",value);
								SetWindowText(GetDlgItem(hwndDlg,IDC_VALUE),buffer);

								SendMessage(GetDlgItem(hwndDlg,IDC_VALUE_SLIDER),TBM_SETPOS,true,value);

								bUpdateingDisplay = false;
								materialParamChanged();
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK ColorEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);
	
			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
		}
		break;
	case WM_DRAWITEM:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(node && selectedMat)
			{
				DRAWITEMSTRUCT * lpdis = (DRAWITEMSTRUCT *) lParam;
				LOGBRUSH logBrush;
				logBrush.lbStyle = BS_SOLID;
				U8 red = 0;
				U8 green = 0;
				U8 blue = 0;
				selectedMat->GetColor(node->data->data.colorData.rType,node->data->data.colorData.rValue,red,green,blue);
				logBrush.lbColor = RGB(red,green,blue);
				HBRUSH brush = CreateBrushIndirect(&logBrush);
				FillRect(lpdis->hDC,&(lpdis->rcItem),brush);
				DeleteObject(brush);
			}
			return 1; 
		}
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_COLOR_BUTTON))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					{
						DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
						if(node && selectedMat)
						{
							U8 red = 0;
							U8 green = 0;
							U8 blue = 0;
							selectedMat->GetColor(node->data->data.colorData.rType,node->data->data.colorData.rValue,red,green,blue);
							CHOOSECOLOR cc;

							cc.lStructSize = sizeof(CHOOSECOLOR);
							cc.hwndOwner = mainHwnd;
							cc.hInstance = 0;
							cc.rgbResult = RGB(red,green,blue);
							cc.lpCustColors = customColors;
							cc.Flags = CC_RGBINIT| CC_FULLOPEN;
							cc.lCustData = 0;
							cc.lpfnHook = 0;
							cc.lpTemplateName = NULL;
							if(ChooseColor(&cc))
							{
								red = GetRValue(cc.rgbResult);
								green = GetGValue(cc.rgbResult);
								blue = GetBValue(cc.rgbResult);
								selectedMat->SetColor(node->data->data.colorData.rType,node->data->data.colorData.rValue,red,green,blue);
								InvalidateRect(hWnd,NULL,true);
								materialParamChanged();
							}
						}
					}
					break;
				}
			}
		}
		break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK TextureEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			SetWindowText(GetDlgItem(hwndDlg,IDC_FILE_NAME),selectedMat->GetTextureName(node->data->data.textureData.channel));

			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg,IDC_PREVIEW_BUTTON))
			{
				switch(dwCode)
				{
				case BN_CLICKED:
					{
						DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
						if(node && selectedMat)
						{
							char buffer[256];
							strcpy(buffer,selectedMat->GetTextureName(node->data->data.textureData.channel));
							if(ChooseTexture(buffer,mainHwnd))
							{
								selectedMat->SetTextureName(node->data->data.textureData.channel,buffer);
								SetWindowText(GetDlgItem(hwndDlg,IDC_FILE_NAME),buffer);
								InvalidateRect(hWnd,NULL,true);
								materialParamChanged();
							}
						}
					}
					break;
				}
			}
		}
		break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK StateEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);

			DWORD currentValue = selectedMat->GetRenderStateValue(node->data->data.stateData.state);

			bUpdateingDisplay = true;
			KRS_ValueType type = internalManager->GetMaterialStateValueType(node->data->data.stateData.state);
			if(type == KRS_BOOL)
			{
				U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)("True"));
				SendMessage(comboList,CB_SETITEMDATA,index,true);
				if(currentValue)
					SendMessage(comboList,CB_SETCURSEL,index,0);
			
				index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)("False"));
				SendMessage(comboList,CB_SETITEMDATA,index,false);
				if(!currentValue)
					SendMessage(comboList,CB_SETCURSEL,index,0);
			}
			else if(type == KRS_NAMED)
			{
				U32 numValues = internalManager->GetNumMaterialStateValues(node->data->data.stateData.state);
				for(U32 i = 0; i < numValues; ++i)
				{
					U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)(internalManager->GetMaterialStateValueName(node->data->data.stateData.state,i)));
					DWORD locValue = internalManager->GetMaterialStateValue(node->data->data.stateData.state,i);
					SendMessage(comboList,CB_SETITEMDATA,index,locValue);
					if(locValue == currentValue)
						SendMessage(comboList,CB_SETCURSEL,index,0);
				}
			}
			bUpdateingDisplay = false;

			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(selectedMat && node)
			{
				HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);

				DWORD currentValue = selectedMat->GetRenderStateValue(node->data->data.stateData.state);
				bUpdateingDisplay = true;
				SendMessage(comboList,CB_RESETCONTENT,0,0);
				KRS_ValueType type = internalManager->GetMaterialStateValueType(node->data->data.stateData.state);
				if(type == KRS_BOOL)
				{
					U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)("True"));
					SendMessage(comboList,CB_SETITEMDATA,index,true);
					if(currentValue)
						SendMessage(comboList,CB_SETCURSEL,index,0);
				
					index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)("False"));
					SendMessage(comboList,CB_SETITEMDATA,index,false);
					if(!currentValue)
						SendMessage(comboList,CB_SETCURSEL,index,0);
				}
				else if(type == KRS_NAMED)
				{
					U32 numValues = internalManager->GetNumMaterialStateValues(node->data->data.stateData.state);
					for(U32 i = 0; i < numValues; ++i)
					{
						U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)(internalManager->GetMaterialStateValueName(node->data->data.stateData.state,i)));
						DWORD locValue = internalManager->GetMaterialStateValue(node->data->data.stateData.state,i);
						SendMessage(comboList,CB_SETITEMDATA,index,locValue);
						if(locValue == currentValue)
							SendMessage(comboList,CB_SETCURSEL,index,0);
					}
				}
				bUpdateingDisplay = false;
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;

			if( hWnd == GetDlgItem(hwndDlg,IDC_DROP_COMBO) )
			{
				switch(dwCode)
				{
				case CBN_SELCHANGE:
					{
						if(!bUpdateingDisplay)
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(selectedMat && node)
							{
								HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);
								U32 index = ::SendMessage(comboList,CB_GETCURSEL,0,0);
								if(index != -1)
								{
									DWORD newValue = SendMessage(comboList,CB_GETITEMDATA,index,0);
									selectedMat->SetRenderStateValue(node->data->data.stateData.state,newValue);
									materialParamChanged();
								}
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK EnumEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);

			DataWinNode * node = (DataWinNode *) lParam;
			SetWindowText(GetDlgItem(hwndDlg,IDC_NAME),node->data->name);

			HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);

			bUpdateingDisplay = true;
			bool bSelectedSet = false;
			U32 numValues = selectedMat->shader->GetNumEnumStates(node->data->name);
			for(U32 i = 0; i < numValues; ++i)
			{
				const char * valueName = selectedMat->shader->GetEnumStateName(node->data->name,i);
				U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)valueName);
				if(selectedMat->shader->TestEnumMatch(node->data->name,valueName,selectedMat))
				{
					SendMessage(comboList,CB_SETCURSEL,index,0);
					bSelectedSet = true;
				}
			}
			U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)"Custom");
			if(!bSelectedSet)
			{
				SendMessage(comboList,CB_SETCURSEL,index,0);
			}
			bUpdateingDisplay = false;

			ShowWindow(hwndDlg,true);
			return 1;
		}
	case WM_UPDATE_MAT_DATA:
		{
			DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
			if(selectedMat && node)
			{
				HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);

				bUpdateingDisplay = true;
				SendMessage(comboList,CB_RESETCONTENT,0,0);
				bool bSelectedSet = false;
				U32 numValues = selectedMat->shader->GetNumEnumStates(node->data->name);
				for(U32 i = 0; i < numValues; ++i)
				{
					const char * valueName = selectedMat->shader->GetEnumStateName(node->data->name,i);
					U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)valueName);
					if(selectedMat->shader->TestEnumMatch(node->data->name,valueName,selectedMat))
					{
						SendMessage(comboList,CB_SETCURSEL,index,0);
						bSelectedSet = true;
					}
				}
				U32 index = SendMessage(comboList,CB_ADDSTRING,0,(DWORD)"Custom");
				if(!bSelectedSet)
				{
					SendMessage(comboList,CB_SETCURSEL,index,0);
				}
				bUpdateingDisplay = false;
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;

			if( hWnd == GetDlgItem(hwndDlg,IDC_DROP_COMBO) )
			{
				switch(dwCode)
				{
				case CBN_SELCHANGE:
					{
						if(!bUpdateingDisplay)
						{
							DataWinNode * node = (DataWinNode *) GetWindowLong(hwndDlg,GWL_USERDATA);
							if(selectedMat && node)
							{
								HWND comboList = GetDlgItem(hwndDlg,IDC_DROP_COMBO);
								U32 index = ::SendMessage(comboList,CB_GETCURSEL,0,0);
								if(index != -1)
								{
									char enumValue[64];
									SendMessage(comboList,CB_GETLBTEXT,index,(DWORD)enumValue);
									selectedMat->shader->SetEnumActive(node->data->name,enumValue,selectedMat);
									materialParamChanged();								
								}
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
void destroyWorkWindows()
{
	dataWinHeight = 0;
	while(dataWinList)
	{
		DataWinNode * tmp = dataWinList;
		dataWinList = dataWinList->next;
		SetWindowLong(tmp->dlg,GWL_USERDATA,0);
		EndDialog(tmp->dlg,0);
		delete tmp;
	}
}
//----------------------------------------------------------------------------------------------
//
void addDataWindow(DataNode * node)
{
	if(node->dataType == MDT_FLOAT)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_FLOAT_EDIT), workWin, FloatEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_INT)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_FLOAT_EDIT), workWin, IntEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_COLOR)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_COLOR_EDIT), workWin, ColorEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_TEXTURE)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_TEXTURE_EDIT), workWin, TextureEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_STATE)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_DROP_EDIT), workWin, StateEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_ENUM)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_DROP_EDIT), workWin, EnumEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
	else if(node->dataType == MDT_ANIMUV)
	{
		DataWinNode * dataWinNode = new DataWinNode;
		if(dataWinList)
		{
			RECT rect;
			GetWindowRect(dataWinList->dlg,&rect);
			dataWinNode->offsetY = dataWinList->offsetY+(rect.bottom-rect.top);
		}
		else
		{
			dataWinNode->offsetY = 0;
		}
		dataWinNode->next = dataWinList;
		dataWinList = dataWinNode;

		dataWinNode->data = node;

		dataWinNode->dlg = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_ANIM_UV_EDIT), workWin, AnimUVEditDialogProc, (DWORD)dataWinNode);
		RECT rect;
		GetWindowRect(dataWinNode->dlg,&rect);
		dataWinHeight += rect.bottom-rect.top;
	}
}
//----------------------------------------------------------------------------------------------
//
void updateSelectionView()
{
	//update header
	HWND name = GetDlgItem(headerWin,IDC_NAME);
	HWND shaderList = GetDlgItem(headerWin,IDC_SHADER);

	if(selectedMat)
	{
		::EnableWindow(name,true);
		SetWindowText(name,selectedMat->szMaterialName.c_str());
		::EnableWindow(shaderList,true);
		::SendMessage(shaderList,CB_SETCURSEL,findComboboxIndex(shaderList,(DWORD)(selectedMat->shader)),0);
	}
	else
	{
		::EnableWindow(name,false);
		::EnableWindow(shaderList,false);
	}

	//update work area
	destroyWorkWindows();
	if(selectedMat)
	{
		IShader * shader = selectedMat->shader;
		if(shader)
		{
			DataNode* node = shader->GetDataList();
			while(node)
			{
				if(bAdvancedMode)
				{
					addDataWindow(node);
				}
				else if(!node->advanced)
				{
					addDataWindow(node);
				}
				node = node->next;
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK HeaderDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
        {
			HWND shaderList = GetDlgItem(hwndDlg,IDC_SHADER);

			IShader * search = internalManager->GetFirstShader();
			while(search)
			{
				U32 index = ::SendMessage(shaderList,CB_ADDSTRING ,0,(DWORD)(search->GetName()));
				::SendMessage(shaderList,CB_SETITEMDATA,index,(DWORD)search);
				search = internalManager->GetNextShader(search);
			}
			ShowWindow( hwndDlg, SW_NORMAL );
			return 1;
		}
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;

			if( hWnd == GetDlgItem(hwndDlg,IDC_SHADER) )
			{
				switch(dwCode)
				{
				case CBN_SELCHANGE:
					{
						HWND shaderList = GetDlgItem(hwndDlg,IDC_SHADER);
						U32 index = ::SendMessage(shaderList,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							IShader * shader = (IShader*)(::SendMessage(shaderList,CB_GETITEMDATA,index,0));
							if(selectedMat)
							{
								destroyWorkWindows();
								selectedMat->SetShader(shader);
								updateSelectionView();
								//fix up alignment
								alignMaterialControls();
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}
//----------------------------------------------------------------------------------------------
//
LRESULT CALLBACK WorkAreaProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_VSCROLL:
		{
			HWND scroll = (HWND)lParam;
			U32 command = LOWORD(wParam);
			U32 posiblePos = HIWORD(wParam);
			U32 pos = GetScrollPos(hwndDlg,SB_VERT);
			RECT rect;
			GetClientRect(hwndDlg,&rect);
			U32 viewHeight = rect.bottom-rect.top;

			switch(command)
			{
			case SB_LINELEFT:
				if(pos > 0)
					pos--;
				break;
			case SB_LINERIGHT:
				if(pos < dataWinHeight-viewHeight)
					++pos;
				break;
			case SB_PAGELEFT:
				if(pos > 10)
					pos -= 10;
				else
					pos = 0;
				break;
			case SB_PAGERIGHT:
				if((dataWinHeight-viewHeight > 10) && (pos < (dataWinHeight-viewHeight-10)))
					pos += 10;
				else
					pos = dataWinHeight-viewHeight;
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				pos = posiblePos;
				break;
			}

			SetScrollPos(hwndDlg,SB_VERT,pos,true);

			dataWinOffset = pos;
			GetClientRect(workWin,&rect);

			DataWinNode * search = dataWinList;
			while(search)
			{
				RECT dlgRect;
				GetWindowRect(search->dlg,&dlgRect);
				MoveWindow(search->dlg,0,search->offsetY-dataWinOffset,rect.right-rect.left,dlgRect.bottom-dlgRect.top,false);
				InvalidateRect(search->dlg,NULL,false);
				search = search->next;
			}
			InvalidateRect(workWin,NULL,false);

		}
		break;
	}
	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}
//----------------------------------------------------------------------------------------------
//
void creatNewMatDir()
{
	//build the internal path
	char internalPath[256];
	internalPath[0] = 0;

	HTREEITEM currentDir = findCurrentMaterialTreeDir();
	HTREEITEM addParentDir = currentDir;
	char itemName[256];
	while(currentDir)
	{
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE;
		item.hItem = currentDir;
		item.pszText = itemName;
		item.cchTextMax = 255;
		TreeView_GetItem(matTree,&item);
		char buffer[256];
		sprintf(buffer,"%s\\%s",item.pszText,internalPath);
		strcpy(internalPath,buffer);
		currentDir = TreeView_GetParent(matTree,currentDir);
	}

	char externalPath[512];
	internalManager->GetDirectory()->GetFileName(externalPath,511);

	//now put it together
	// matDirPath + "Materials" + internalPath + dirName
	char targetFile[512];

	sprintf(targetFile,"%s\\Materials\\%s%s",externalPath,
		internalPath,"NewName");

	if(CreateDirectory(targetFile,NULL))
	{
		TVINSERTSTRUCT insertStruct;
		insertStruct.hParent = addParentDir;
		insertStruct.hInsertAfter = TVI_LAST;
		insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		insertStruct.itemex.lParam = NULL;
		insertStruct.itemex.pszText = "NewName";
		insertStruct.itemex.cchTextMax = strlen("NewName");
		insertStruct.itemex.iImage =0;
		insertStruct.itemex.iSelectedImage =0;
		HTREEITEM treeDir = TreeView_InsertItem(matTree,&insertStruct);
		TreeView_Expand(matTree,addParentDir,TVE_EXPAND);
		TreeView_EditLabel(matTree,treeDir);
	}
}
//----------------------------------------------------------------------------------------------
//
void copyMaterial()
{
	HTREEITEM current = TreeView_GetSelection(matTree);
	if(current && selectedMat)
	{
		char internalPath[512];
		char itemName[256];
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE |TVIF_PARAM;
		item.hItem = current;
		item.pszText = itemName;
		item.cchTextMax = 255;
		item.lParam = 0;
		TreeView_GetItem(matTree,&item);
		strcpy(internalPath,item.pszText);

		if(item.lParam)//not a directory
		{
			current = TreeView_GetParent(matTree,current);
			HTREEITEM destDir = current;
			while(current)
			{
				TVITEMEX item;
				item.mask = TVIF_TEXT | TVIF_HANDLE;
				item.hItem = current;
				item.pszText = itemName;
				item.cchTextMax = 255;
				TreeView_GetItem(matTree,&item);
				char buffer[256];
				sprintf(buffer,"%s\\%s",item.pszText,internalPath);
				strcpy(internalPath,buffer);
				current = TreeView_GetParent(matTree,current);
			}

			char externalPath[512];
			internalManager->GetDirectory()->GetFileName(externalPath,511);

			//now put it together
			// matDirPath + "Textures" + internalPath + " Copy.txt"
			char testFile[512];
			char targetFile[512];

			U32 count = 0;
			do
			{
				sprintf(testFile,"%s Copy%d",internalPath,count);
				strcat(testFile,".txt");
				++count;
			}while(internalManager->FindMaterial(testFile));

            sprintf(targetFile,"%s\\Materials\\%s",externalPath,testFile);

			char importFile[512];
			sprintf(importFile,"%s\\Materials\\%s",externalPath,selectedMat->szFileName.c_str());
			
			CopyFile(importFile,targetFile,false);

			Material * mat = internalManager->LoadNewMaterial(testFile);

			char strname[256];
			strcpy(strname,mat->szMaterialName.c_str());
			char * strPtr = strrchr(strname,'\\');
			if(strPtr)
				++strPtr;
			else
				strPtr = strname;

			TVINSERTSTRUCT insertStruct;
			insertStruct.hParent = destDir;
			insertStruct.hInsertAfter = TVI_LAST;
			insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM| TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			insertStruct.itemex.pszText = testFile;
			insertStruct.itemex.lParam = (DWORD) mat;
			insertStruct.itemex.cchTextMax = strlen(testFile);
			insertStruct.itemex.iImage =2;
			insertStruct.itemex.iSelectedImage =2;
			TreeView_InsertItem(matTree,&insertStruct);
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void deleteMaterial()
{
	if(selectedMat)
	{
		char externalPath[512];
		internalManager->GetDirectory()->GetFileName(externalPath,511);
		char importFile[512];
		sprintf(importFile,"%s\\Materials\\%s",externalPath,selectedMat->szFileName.c_str());
		DeleteFile(importFile);

		internalManager->DeleteMaterial(selectedMat);
		selectedMat = NULL;

		HTREEITEM current = TreeView_GetSelection(matTree);
		TreeView_DeleteItem(matTree,current);

		updateSelectionView();

		//fix up alignment
		alignMaterialControls();
	}
	else
	{
		HTREEITEM current = TreeView_GetSelection(matTree);
		HTREEITEM target = current;
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
			TreeView_GetItem(matTree,&item);
			strcpy(internalPath,item.pszText);

			if(!(item.lParam))
			{
				current = TreeView_GetParent(matTree,current);
				while(current)
				{
					TVITEMEX item;
					item.mask = TVIF_TEXT | TVIF_HANDLE;
					item.hItem = current;
					item.pszText = itemName;
					item.cchTextMax = 255;
					TreeView_GetItem(matTree,&item);
					char buffer[256];
					sprintf(buffer,"%s\\%s",item.pszText,internalPath);
					strcpy(internalPath,buffer);
					current = TreeView_GetParent(matTree,current);
				}

				char externalPath[512];
				internalManager->GetDirectory()->GetFileName(externalPath,511);

				//now put it together
				// matDirPath + "Textures" + internalPath
				char targetFile[512];

				sprintf(targetFile,"%s\\Materials\\%s",externalPath,internalPath);

				if(RemoveDirectory(targetFile))
				{
					TreeView_DeleteItem(matTree,target);
				}
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
//
bool renameDirectory(HTREEITEM itemHandle, const char * newName)
{
	char internalPathSource[MAX_PATH];
	internalPathSource[0] = 0;

	HTREEITEM search = itemHandle;

	char itemName[MAX_PATH];
	while(search)
	{
		TVITEMEX item;
		item.mask = TVIF_TEXT | TVIF_HANDLE;
		item.hItem = search;
		item.pszText = itemName;
		item.cchTextMax = MAX_PATH-1;
		TreeView_GetItem(matTree,&item);
		char buffer[MAX_PATH];
		sprintf(buffer,"%s\\%s",item.pszText,internalPathSource);
		strcpy(internalPathSource,buffer);
		search = TreeView_GetParent(matTree,search);
	}

	char externalPath[512];
	internalManager->GetDirectory()->GetFileName(externalPath,511);

	char sourcePath[MAX_PATH];
	sprintf(sourcePath,"%s\\Materials\\%s",externalPath,internalPathSource);

	char internalPathTarget[MAX_PATH];
	strcpy(internalPathTarget,internalPathSource);
	char * str = strrchr(internalPathTarget,'\\');
	if(str)
	{
		str[0] = 0;
		str = strrchr(internalPathTarget,'\\');
		if(str)
			str[1] = 0;
		else
			internalPathTarget[0] = 0;
	}
	else
		internalPathTarget[0] = 0;

	char targetPath[MAX_PATH];
	sprintf(targetPath,"%s\\Materials\\%s%s",externalPath,internalPathTarget,newName);
	if(MoveFile(sourcePath,targetPath))
	{
		U32 len = strlen(internalPathSource);
		Material * mat = internalManager->GetFirstMaterial();
		while(mat)
		{
			if(len == 0 || strncmp(internalPathSource,mat->szFileName.c_str(),len) == 0)
			{
				char filename[MAX_PATH];
				sprintf(filename,"%s%s\\%s",internalPathTarget,newName,&(mat->szFileName.c_str()[len]));
				mat->Rename(filename);
			}
			mat = internalManager->GetNextMaterial(mat);
		}
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool renameMaterial(Material * mat, const char * newName)
{
	char externalPath[512];
	internalManager->GetDirectory()->GetFileName(externalPath,511);
	char importFile[512];
	sprintf(importFile,"%s\\Materials\\%s",externalPath,selectedMat->szFileName.c_str());
	if(DeleteFile(importFile))
	{
		char filename[512];
		strcpy(filename,selectedMat->szFileName.c_str());
		char * str = strrchr(filename,'\\');
		if(str)
		{
			++str;
			str[0] = 0;
		}
		else
			filename[0] = 0;
		strcat(filename,newName);
		strcat(filename,".txt");
		mat->Rename(filename);
		mat->Save(internalManager->GetMatDir());
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
void beginTreeDrag(NMTREEVIEW * treeHdr)
{
	HIMAGELIST himl;    // handle to image list 

	// Tell the tree-view control to create an image to use 
	// for dragging. 
	dragItem = treeHdr->itemNew.hItem;
	himl = TreeView_CreateDragImage(matTree, dragItem); 

 	ImageList_BeginDrag(himl, 0, 0, -30); 


	// Hide the mouse pointer, and direct mouse input to the 
	// parent window. 
	ShowCursor(FALSE); 
	SetCapture(GetParent(matTree)); 
	g_fDragging = TRUE; 

    ImageList_DragEnter(GetParent(matTree),0, 0); 
 	return; 
}
//----------------------------------------------------------------------------------------------
//
void endTreeDrag()
{
	HTREEITEM dest = TreeView_GetDropHilight(matTree);
	TreeView_SelectDropTarget(matTree,NULL);
	if(dest && dragItem)
	{
		//first find the target directory of the move.
		TVITEMEX item;
		item.mask = TVIF_PARAM | TVIF_HANDLE;
		item.hItem = dest;
		item.lParam = NULL;
		TreeView_GetItem(matTree,&item);

		if(item.lParam)
		{
			dest = TreeView_GetParent(matTree,dest);
		}
		HTREEITEM destDir = dest;

		char internalPath[MAX_PATH];
		internalPath[0] = 0;

		char itemName[MAX_PATH];
		while(dest)
		{
			TVITEMEX item;
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.hItem = dest;
			item.pszText = itemName;
			item.cchTextMax = MAX_PATH-1;
			TreeView_GetItem(matTree,&item);
			char buffer[256];
			sprintf(buffer,"%s\\%s",item.pszText,internalPath);
			strcpy(internalPath,buffer);
			dest = TreeView_GetParent(matTree,dest);
		}

		char externalPath[512];
		internalManager->GetDirectory()->GetFileName(externalPath,511);
        	
		//find out what kind of move to perform
		char nameStr[MAX_PATH];
		item.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_TEXT;
		item.hItem = dragItem;
		item.lParam = NULL;
		item.pszText = nameStr;
		item.cchTextMax = MAX_PATH-1;
		TreeView_GetItem(matTree,&item);

		if(item.lParam) // file move
		{
			Material * mat = (Material *)item.lParam;
			//now put it together
			// matDirPath + "Materials" + internalPath
			char sourcePath[MAX_PATH];

			sprintf(sourcePath,"%s\\Materials\\%s",externalPath,mat->szFileName.c_str());

			char targetPath[MAX_PATH];
			sprintf(targetPath,"%s\\Materials\\%s%s%s",externalPath,internalPath,nameStr,".txt");
			
			if(MoveFile(sourcePath,targetPath))
			{
				char filename[MAX_PATH];
				sprintf(filename,"%s%s%s",internalPath,nameStr,".txt");
				mat->Rename(filename);

				TreeView_DeleteItem(matTree,dragItem);

				TVINSERTSTRUCT insertStruct;
				insertStruct.hParent = destDir;
				insertStruct.hInsertAfter = TVI_LAST;
				insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM| TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				insertStruct.itemex.pszText = nameStr;
				insertStruct.itemex.lParam = (DWORD) mat;
				insertStruct.itemex.cchTextMax = strlen(nameStr);
				insertStruct.itemex.iImage =2;
				insertStruct.itemex.iSelectedImage =2;
				HTREEITEM treeDir =TreeView_InsertItem(matTree,&insertStruct);

				TreeView_Expand(matTree,destDir,TVE_EXPAND);
				TreeView_Select(matTree,treeDir,TVGN_CARET);

			}
		}
		else//dir move
		{
			char internalPathSource[MAX_PATH];
			internalPathSource[0] = 0;

			while(dragItem)
			{
				TVITEMEX item;
				item.mask = TVIF_TEXT | TVIF_HANDLE;
				item.hItem = dragItem;
				item.pszText = itemName;
				item.cchTextMax = MAX_PATH-1;
				TreeView_GetItem(matTree,&item);
				char buffer[MAX_PATH];
				sprintf(buffer,"%s\\%s",item.pszText,internalPathSource);
				strcpy(internalPathSource,buffer);
				dragItem = TreeView_GetParent(matTree,dragItem);
			}

			char sourcePath[MAX_PATH];
			sprintf(sourcePath,"%s\\Materials\\%s",externalPath,internalPathSource);

			char targetPath[MAX_PATH];
			sprintf(targetPath,"%s\\Materials\\%s%s",externalPath,internalPath,nameStr);
			if(MoveFile(sourcePath,targetPath))
			{
				U32 len = strlen(internalPathSource);
				Material * mat = internalManager->GetFirstMaterial();
				while(mat)
				{
					if(len == 0 || strncmp(internalPathSource,mat->szFileName.c_str(),len) == 0)
					{
						char filename[MAX_PATH];
						sprintf(filename,"%s%s%s",internalPath,nameStr,&(mat->szFileName.c_str()[len]));
						mat->Rename(filename);
						break;
					}
					mat = internalManager->GetNextMaterial(mat);
				}

				updateMaterialList();
			}
		}
	}

    ImageList_DragLeave(GetParent(matTree)); 
	ImageList_EndDrag(); 
	ReleaseCapture(); 
	ShowCursor(TRUE); 
	g_fDragging = FALSE; 
};
//----------------------------------------------------------------------------------------------
//
INT_PTR CALLBACK MaterialDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_CLOSE:
		{
			internalManager->SetDialog(NULL);
			EndDialog(hwndDlg,0);
		}
		break;
	case WM_INITDIALOG:
        {
			if(!matImageList)
			{
				matImageList= ImageList_Create(16,16,ILC_COLOR32|ILC_MASK ,0,1);

				HICON bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_CLOSEDFOLDER),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(matImageList,bitmap);

				bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_OPENFOLDER),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(matImageList,bitmap);

				bitmap =  (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MATERIAL),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				ImageList_AddIcon(matImageList,bitmap);
			}

			internalManager = (IInternalMaterialManager*)lParam;
			selectedMat = NULL;

			mainHwnd = hwndDlg;

			CheckMenuItem(GetSubMenu(GetMenu(hwndDlg),3),IDM_ADVANCED,(bAdvancedMode?MF_CHECKED:MF_UNCHECKED)|MF_BYCOMMAND);

			//create TreeView

			matTree = CreateWindowEx(0,WC_TREEVIEW,"Material List",
				WS_CHILD|WS_VISIBLE|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_EDITLABELS ,
				0,0,10,10,hwndDlg,NULL,g_hInstance,NULL);

			TreeView_SetImageList(matTree,matImageList,TVSIL_NORMAL);

			//create header view
			headerWin = ::CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_HEADER), mainHwnd, HeaderDialogProc, NULL);

			//create workarea

			WNDCLASSEX wcx; 
			wcx.cbSize = sizeof(wcx);          // size of structure 
			wcx.style = CS_HREDRAW | CS_VREDRAW;                    // redraw if size changes 
			wcx.lpfnWndProc = WorkAreaProc;     // points to window procedure 
			wcx.cbClsExtra = 0;                // no extra class memory 
			wcx.cbWndExtra = 0;                // no extra window memory 
			wcx.hInstance = g_hInstance;         // handle to instance 
			wcx.hIcon = NULL;              // predefined app. icon 
			wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow 
			wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);                  // white background brush 
			wcx.lpszMenuName =  NULL;    // name of menu resource 
			wcx.lpszClassName = "MatWorkArea";  // name of window class 
			wcx.hIconSm = NULL; 
 		    RegisterClassEx(&wcx); 

			workWin = CreateWindow("MatWorkArea",        // name of window class 
				"MatWorkArea",            // title-bar string 
				WS_CHILD|WS_VISIBLE|WS_VSCROLL, // top-level window 
				0,       // default horizontal position 
				0,       // default vertical position 
				10,       // default width 
				10,       // default height 
				hwndDlg,         // no owner window 
				(HMENU) NULL,        // use class menu 
				g_hInstance,           // handle to application instance 
				(LPVOID) NULL);      // no window-creation data 

			//fill in data
			updateMaterialList();
			updateSelectionView();

			//fix up alignment
			alignMaterialControls();

			ShowWindow( hwndDlg, SW_NORMAL );
			return 1;
		}
		break;
	case WM_MOUSEMOVE:
		{
			HTREEITEM htiTarget;  // handle to target item 
			TVHITTESTINFO tvht;  // hit test information 

			U32 xCur = LOWORD(lParam); 
			U32 yCur = HIWORD(lParam); 

			if (g_fDragging) 
			{ 
				// Drag the item to the current position of the mouse pointer. 
				ImageList_DragMove(xCur, yCur); 

				// Find out if the pointer is on the item. If it is, 
				// highlight the item as a drop target. 
				tvht.pt.x = xCur; 
				tvht.pt.y = yCur; 
				if ((htiTarget = TreeView_HitTest(matTree, &tvht)) != NULL) 
				{ 
					TreeView_SelectDropTarget(matTree, htiTarget); 
				} 
			} 
		}
		break;
	case WM_LBUTTONUP:
		{
			if (g_fDragging) 
			{ 
				endTreeDrag();
			} 
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR * hdr = (NMHDR*) lParam;
			if(hdr->hwndFrom == matTree)
			{
				NMTREEVIEW * treeHdr = (NMTREEVIEW *)hdr;
				switch(hdr->code)
				{
				case TVN_SELCHANGED:
					{
						if(!bUpdateingMatTree)
						{
							destroyWorkWindows();
							selectedMat = (Material*)(treeHdr->itemNew.lParam);
							updateSelectionView();
							//fix up alignment
							alignMaterialControls();
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

						TreeView_SetItem(matTree,&item);
					}
					break;
				case TVN_BEGINLABELEDIT:
					{
						return false;
					};
				case TVN_ENDLABELEDIT:
					{
						NMTVDISPINFO * infoHDR = (NMTVDISPINFO *)hdr;
						if(infoHDR->item.pszText == NULL)
							return false;
						if(infoHDR->item.lParam == NULL)//a directory
						{
							if(renameDirectory(infoHDR->item.hItem,infoHDR->item.pszText))
							{
								TreeView_SetItem(matTree,&(infoHDR->item));				
								return true;
							}
						}
						//otherwise we renamed it
						if(renameMaterial((Material * )(infoHDR->item.lParam),infoHDR->item.pszText))
						{
							TreeView_SetItem(matTree,&(infoHDR->item));				
							return true;
						}
					}
					break;
				case TVN_BEGINDRAG:
					{
						beginTreeDrag(treeHdr);
					}
					break;
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			DWORD dwCode = HIWORD(wParam);
			DWORD dwId   = LOWORD(wParam);
			HWND  hWnd   = (HWND)lParam;

			if( dwId == ID_FILE_NEWMAT )
			{
				char internalPath[256];
				internalPath[0] = 0;

				HTREEITEM currentDir = findCurrentMaterialTreeDir();
				HTREEITEM addParentDir = currentDir;
				char itemName[256];
				while(currentDir)
				{
					TVITEMEX item;
					item.mask = TVIF_TEXT | TVIF_HANDLE;
					item.hItem = currentDir;
					item.pszText = itemName;
					item.cchTextMax = 255;
					TreeView_GetItem(matTree,&item);
					char buffer[256];
					sprintf(buffer,"%s\\%s",item.pszText,internalPath);
					strcpy(internalPath,buffer);
					currentDir = TreeView_GetParent(matTree,currentDir);
				}

				Material * mat = internalManager->MakeNewMaterial(internalPath);

				char strname[256];
				strcpy(strname,mat->szMaterialName.c_str());
				char * strPtr = strrchr(strname,'\\');
				if(strPtr)
					++strPtr;
				else
					strPtr = strname;
				TVINSERTSTRUCT insertStruct;
				insertStruct.hParent = addParentDir;
				insertStruct.hInsertAfter = TVI_LAST;
				insertStruct.itemex.mask = TVIF_TEXT | TVIF_PARAM| TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				insertStruct.itemex.pszText = strPtr;
				insertStruct.itemex.lParam = (DWORD) mat;
				insertStruct.itemex.cchTextMax = strlen(strPtr);
				insertStruct.itemex.iImage =2;
				insertStruct.itemex.iSelectedImage =2;
				HTREEITEM treeDir =TreeView_InsertItem(matTree,&insertStruct);

				TreeView_Expand(matTree,addParentDir,TVE_EXPAND);
				TreeView_Select(matTree,treeDir,TVGN_CARET);
				TreeView_EditLabel(matTree,treeDir);
			}
			else if( dwId == IDM_FILE_SAVE )
			{
				internalManager->SaveAll();
			}
			else if( dwId == ID_FILE_RELOAD )
			{
				internalManager->ReloadAll();
				updateMaterialList();
				updateSelectionView();
			}
			else if( dwId == ID_FILE_EXIT )
			{
				EndDialog( hwndDlg, IDCANCEL );

				if( internalManager )
					internalManager->SetDialog(NULL);

				return 1;
			}
			else if( dwId == ID_FILE_NEWDIRECTORY)
			{
				creatNewMatDir();
			}
			else if( dwId == ID_FILE_COPY)
			{
				copyMaterial();
			}
			else if( dwId == ID_FILE_DELETE)
			{
				deleteMaterial();
			}
			else if( dwId == ID_SOURCECONTROL_CHECKOUT )
			{
				::MessageBox( hwndDlg, "Not implemented", "ID_SOURCECONTROL_CHECKOUT", MB_OK );
			}
			else if( dwId == ID_SOURCECONTROL_CHECKIN )
			{
				::MessageBox( hwndDlg, "Not implemented", "ID_SOURCECONTROL_CHECKIN", MB_OK );
			}
			else if( dwId == ID_SOURCECONTROL_IMPORT )
			{
				::MessageBox( hwndDlg, "Not implemented", "ID_SOURCECONTROL_IMPORT", MB_OK );
			}
			else if( dwId == ID_PREVIEW )
			{
				::MessageBox( hwndDlg, "Not implemented", "ID_PREVIEW", MB_OK );
			}
			else if(dwId == IDM_ADVANCED)
			{
				bAdvancedMode = !bAdvancedMode;
				CheckMenuItem(GetSubMenu(GetMenu(hwndDlg),3),IDM_ADVANCED,(bAdvancedMode?MF_CHECKED:MF_UNCHECKED)|MF_BYCOMMAND);
				updateSelectionView();
				alignMaterialControls();
			}
		}
		break;
	case WM_SIZE:
		{
			alignMaterialControls();
		}
		break;
	}

	return 0;
}