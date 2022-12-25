//InfoAra.cpp

#include "stdafx.h"
#include "globals.h"
#include "resource.h"
#include "InfoArea.h"
#include "IEffectTarget.h"
#include "IEffectFile.h"
#include "ITargetHp.h"
#include "ITargetAnim.h"
#include "IEffectParam.h"
#include "IEffectEvent.h"
#include "IEffectAction.h"
#include "ITargetCue.h"
#include "ParticleView.h"
#include "EventGraph.h"
#include "Archlist.h"

#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>

char stringValue[256];//buffer used to rtuen valuse out of the modal dialog boxes

//this will most likely work as a global font since this app uniformly uses the same one
HFONT hFont;

HWND visibleInfoWND;
HWND targetList;

IEffectTarget * selectedTarg=0;
HWND targetInfo;

HWND paramView;
HWND eventView;
extern IEffectEvent * selectedEvent;//TimeBar.cpp

HWND actionAnimationView;
HWND actionGameEventView;
HWND actionSoundView;
HWND actionSwitchView;
HWND actionHideTargetView;
HWND actionListenView;
HWND actionJointTrackView;
extern IEffectAction * selectedAction;

ITargetAnim * selectedAnim;
HWND animTargetInfo;

BOOL CALLBACK targetAnimInfoProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionGameEventProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionSoundProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionSwitchProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionHideTargetProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionListenProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionJointTrackProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK actionAnimProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK newActionProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK eventProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK paramListProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK targetInfoProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK targetListProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK targetAddProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
LONG CALLBACK keyCatcherProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);


void InfoArea::Deselect()
{
	if(selectedTarg)
		selectedTarg->Deselect();
	selectedTarg = NULL;
	selectedAnim = NULL;
	if(visibleInfoWND)
	{
		DestroyWindow(visibleInfoWND);
		ParticleView::Null();
		visibleInfoWND = NULL;
		targetList = NULL;
		targetInfo = NULL;
		paramView = NULL;
		eventView = NULL;
		actionAnimationView = NULL;
		actionGameEventView = NULL;
		actionSoundView = NULL;
		actionSwitchView = NULL;
		actionHideTargetView = NULL;
		actionListenView = NULL;
		actionJointTrackView = NULL;
		animTargetInfo = NULL;
	}
}

void InfoArea::SelectTargetList()
{
	InfoArea::Deselect();
	if(!targetList)
	{
		targetList = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_TARGET_VIEW), infoArea, targetListProc);
	}
	ShowWindow(targetList,true);
	visibleInfoWND = targetList;
}

void InfoArea::SelectTarget(struct IEffectTarget * selTarg)
{
	InfoArea::Deselect();
	selectedTarg = selTarg;
	selectedTarg->Select();
	if(!targetInfo)
	{
		targetInfo = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_TARGET_INFO), infoArea, targetInfoProc);
	}
	ShowWindow(targetInfo,true);
	visibleInfoWND = targetInfo;
}

void InfoArea::SelectParamList()
{
	InfoArea::Deselect();
	if(!paramView)
	{
		paramView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_PARAMETER_VIEW), infoArea, paramListProc);
	}
	ShowWindow(paramView,true);
	visibleInfoWND = paramView;
}

void InfoArea::SelectEvent(IEffectEvent * targ)
{
	InfoArea::Deselect();
	if(!eventView)
	{
		eventView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_EVENT_SELECT), infoArea, eventProc);
	}
	ShowWindow(eventView,true);
	visibleInfoWND = eventView;
}

void InfoArea::SelectAction(IEffectAction * targ)
{
	InfoArea::Deselect();

	IActionAnimation * anim = targ->GetActionAnimation();
	IActionParticle * part = targ->GetActionParticle();
	IActionGameEvent * event = targ->GetActionGameEvent();
	IActionSound * sound = targ->GetActionSound();
	IActionSwitch * aSwitch = targ->GetActionSwitch();
	IActionHideTarget * hideTarget = targ->GetActionHideTarget();
	IActionListen * actionListen = targ->GetActionListen();
	IActionJointTrack * actionJointTrack = targ->GetActionJointTrack();
	if(anim)
	{
		if(!actionAnimationView)
		{
			actionAnimationView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ANIMATION_DIALOG), infoArea, actionAnimProc);
		}
		ShowWindow(actionAnimationView,true);
		visibleInfoWND = actionAnimationView;
	}
	else if(part)
	{
		visibleInfoWND = ParticleView::Open();
	}
	else if(event)
	{
		if(!actionGameEventView)
		{
			actionGameEventView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_GAMEEVENT), infoArea, actionGameEventProc);
		}
		ShowWindow(actionGameEventView,true);
		visibleInfoWND = actionGameEventView;
	}
	else if(sound)
	{
		if(!actionSoundView)
		{
			actionSoundView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_SOUND), infoArea, actionSoundProc);
		}
		ShowWindow(actionSoundView,true);
		visibleInfoWND = actionSoundView;
	}
	else if(aSwitch)
	{
		if(!actionSwitchView)
		{
			actionSwitchView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_SWITCH), infoArea, actionSwitchProc);
		}
		ShowWindow(actionSwitchView,true);
		visibleInfoWND = actionSwitchView;
	}
	else if(hideTarget)
	{
		if(!actionHideTargetView)
		{
			actionHideTargetView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_HIDE_TARGET), infoArea, actionHideTargetProc);
		}
		ShowWindow(actionHideTargetView,true);
		visibleInfoWND = actionHideTargetView;
	}
	else if(actionListen)
	{
		if(!actionListenView)
		{
			actionListenView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_LISTEN), infoArea, actionListenProc);
		}
		ShowWindow(actionListenView,true);
		visibleInfoWND = actionListenView;
	}
	else if(actionJointTrack)
	{
		if(!actionJointTrackView)
		{
			actionJointTrackView = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_ACTION_JOINT_TRACK), infoArea, actionJointTrackProc);
		}
		ShowWindow(actionJointTrackView,true);
		visibleInfoWND = actionJointTrackView;
	}
}

void InfoArea::SelectTargetAnim(IEffectTarget * sTarg,ITargetAnim * targ)
{
	InfoArea::Deselect();
	selectedTarg = sTarg;
	selectedTarg->Select();
	selectedAnim = targ;
	if(!animTargetInfo)
	{
		animTargetInfo = CreateDialog(hMainInst,MAKEINTRESOURCE(IDD_TARGET_ANIM_INFO), infoArea, targetAnimInfoProc);
	}
	ShowWindow(animTargetInfo,true);
	visibleInfoWND = animTargetInfo;
}

LONG CALLBACK infoAreaProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SETFONT:
		{
			hFont = (HFONT)wParam;
		}
		break;
	case WM_SIZE:
		{
			ParticleView::Resize();
		}
		break;
	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}

U32 listIndex;//index of the record being edited;
bool bNameEdit;

WNDPROC oldListViewProc;

LONG CALLBACK keyCatcherProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_KEYDOWN:
		{
			switch (LOWORD(wParam))
			{
			case VK_BACK:		//fallthrough intentional
			case VK_DELETE:
				{
					SendMessage(GetParent(hWindow),message, wParam, lParam);
				}
				break;
			}
		}
		break;
	}

	return CallWindowProc(oldListViewProc,hWindow, message, wParam, lParam);
}

BOOL CALLBACK targetAnimInfoProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAnim && selectedTarg)
			{
				HWND animName = GetDlgItem(hWindow, IDC_TARGET_ANIM_NAME);
				SetWindowText(animName,selectedAnim->GetName());
				
				HWND parentName = GetDlgItem(hWindow, IDC_PARENT_TARGET);
				SetWindowText(parentName,selectedTarg->GetName());

				HWND fileName = GetDlgItem(hWindow, IDC_ANIM_FILE_NAME);
				if(selectedAnim->GetFileName()[0])
					SetWindowText(fileName,selectedAnim->GetFileName());
				else
					SetWindowText(fileName,"No File Loaded");

				HWND scripted = GetDlgItem(hWindow, IDC_SCRIPTED);
				if(selectedAnim->IsScripted())
				{
					SetWindowText(scripted,"Yes");
					HWND cueList = GetDlgItem(hWindow, IDC_CUE_LIST);
					ITargetCue * cue = selectedAnim->GetFirstCue();
					while(cue)
					{
						SendMessage(cueList,LB_ADDSTRING,0,(DWORD)(cue->GetName()));
						cue = cue->GetNextCue();
					}
				}
				else
					SetWindowText(scripted,"No");
			}
		}
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TO_PARENT:
						{
							if(selectedTarg)
							{
								InfoArea::SelectTarget(selectedTarg);
							}
						}
						break;
					case IDC_ASSIGN_FILENAME:
						{
							if(selectedTarg && selectedAnim)
							{
								char buffer[255];
								buffer[0] = 0;
								OPENFILENAME fileName;
								memset(&fileName,0,sizeof(OPENFILENAME));
								fileName.lStructSize = sizeof(OPENFILENAME);
								fileName.lpstrFilter = "Granny Animation\0*.gr2\0\0";
								fileName.nFilterIndex = 1;
								fileName.lpstrFile = buffer;
								fileName.nMaxFile = 255;
								fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
    									OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
								if(GetOpenFileName(&fileName))
								{
									if(selectedTarg && selectedAnim)//just in case
									{
										if(selectedAnim->GetFileName()[0])
											selectedTarg->UnloadAnimation(selectedAnim->GetName());
										selectedAnim->SetFileName(buffer);
										selectedTarg->LoadAnimFile(buffer,selectedAnim->GetName());
										selectedAnim->SetPlayTime(selectedTarg->GetAnimPlaytime(selectedAnim->GetName()));
										HWND fileName = GetDlgItem(hWindow, IDC_ANIM_FILE_NAME);
										if(selectedAnim->GetFileName()[0])
											SetWindowText(fileName,selectedAnim->GetFileName());
										else
											SetWindowText(fileName,"No File Loaded");
									}
								}
							}
						}
						break;
					case IDC_REMOVE_FILENAME:
						{
							if(selectedTarg && selectedAnim)
							{
								if(selectedAnim->GetFileName()[0])
									selectedTarg->UnloadAnimation(selectedAnim->GetName());

								HWND fileName = GetDlgItem(hWindow, IDC_ANIM_FILE_NAME);
								SetWindowText(fileName,"No File Loaded");
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
}

BOOL CALLBACK actionGameEventProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionGameEvent * event = selectedAction->GetActionGameEvent();
				if(event)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());
					HWND eventString = GetDlgItem(hWindow,IDC_EVENT_STRING);
					SetWindowText(eventString,event->GetGameEventName());
					IEffectTarget * targ = event->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
						SendMessage(combo,CB_SETCURSEL,index,0);
					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					HWND speedWin = GetDlgItem(hWindow, IDC_SPEED_EDIT);
					char buffer[256];
					sprintf(buffer,"%f",event->GetSpeed());
					SetWindowText(speedWin,buffer);
					EnableWindow(speedWin,event->GetDistanceFlag());

					CheckDlgButton(hWindow,IDC_DISTANCE_CHECK,event->GetDistanceFlag() ? BST_CHECKED: BST_UNCHECKED);
					CheckDlgButton(hWindow,IDC_EFFECTEND_CHECK,event->GetEndEffectFlag() ? BST_CHECKED: BST_UNCHECKED);
					CheckDlgButton(hWindow,IDC_RESPONCE_CHECK,event->GetResponceFlag() ? BST_CHECKED: BST_UNCHECKED);	
			
					// SetFocus(name);
					return false;
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_DISTANCE_CHECK:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									if(IsDlgButtonChecked(hWindow,IDC_DISTANCE_CHECK) == BST_CHECKED)
									{
										event->SetDistanceFlag(true);
										HWND speedWin = GetDlgItem(hWindow, IDC_SPEED_EDIT);
										EnableWindow(speedWin,true);
									}
									else
									{
										event->SetDistanceFlag(false);
										HWND speedWin = GetDlgItem(hWindow, IDC_SPEED_EDIT);
										EnableWindow(speedWin,false);
									}
								}
							}
						}
						break;
					case IDC_EFFECTEND_CHECK:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									if(IsDlgButtonChecked(hWindow,IDC_EFFECTEND_CHECK) == BST_CHECKED)
									{
										event->SetEndEffectFlag(true);
									}
									else
									{
										event->SetEndEffectFlag(false);
									}
								}
							}
						}
						break;
					case IDC_RESPONCE_CHECK:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									if(IsDlgButtonChecked(hWindow,IDC_RESPONCE_CHECK) == BST_CHECKED)
									{
										event->SetResponceFlag(true);
									}
									else
									{
										event->SetResponceFlag(false);
									}
								}
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_EVENT_STRING:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									char buffer[256];
									HWND name = GetDlgItem(hWindow,IDC_EVENT_STRING);
									GetWindowText(name,buffer,255);
									buffer[255] = 0;
									event->SetGameEventName(buffer);
								}
							}
						}
						break;
					case IDC_SPEED_EDIT:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									char buffer[256];
									HWND speed = GetDlgItem(hWindow,IDC_SPEED_EDIT);
									GetWindowText(speed,buffer,255);
									buffer[255] = 0;
									event->SetSpeed(atof(buffer));
								}
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionGameEvent * event = selectedAction->GetActionGameEvent();
								if(event)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										event->SetTarget(targ);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK actionListenProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionListen * event = selectedAction->GetActionListen();
				if(event)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());
					HWND listenString = GetDlgItem(hWindow,IDC_LISTEN_STRING);
					SetWindowText(listenString,event->GetListenName());
					IEffectTarget * targ = event->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
						SendMessage(combo,CB_SETCURSEL,index,0);
					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					return false;
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_LISTEN_STRING:
						{
							if(selectedAction)
							{
								IActionListen * event = selectedAction->GetActionListen();
								if(event)
								{
									char buffer[256];
									HWND name = GetDlgItem(hWindow,IDC_LISTEN_STRING);
									GetWindowText(name,buffer,255);
									buffer[255] = 0;
									event->SetListenName(buffer);
								}
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionListen * event = selectedAction->GetActionListen();
								if(event)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										event->SetTarget(targ);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK actionJointTrackProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionJointTrack * event = selectedAction->GetActionJointTrack();
				if(event)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());
					HWND jointString = GetDlgItem(hWindow,IDC_JOINT_STRING);
					SetWindowText(jointString,event->GetJointName());

					IEffectTarget * targ = event->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
						SendMessage(combo,CB_SETCURSEL,index,0);
					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					IEffectTarget * source = event->GetSource();
					HWND sourceCombo = GetDlgItem(hWindow, IDC_SOURCE_COMBO);
					index = SendMessage(sourceCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(sourceCombo,CB_SETITEMDATA,index,0);
					if(!source)
						SendMessage(sourceCombo,CB_SETCURSEL,index,0);
					searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(sourceCombo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(sourceCombo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == source)
						{
							SendMessage(sourceCombo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					char buffer[256];
					sprintf(buffer,"%f",event->GetAngVelocity());
					HWND velString = GetDlgItem(hWindow,IDC_ANG_VELOCITY_STRING);
					SetWindowText(velString,buffer);
					
					return false;
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_JOINT_STRING:
						{
							if(selectedAction)
							{
								IActionJointTrack * event = selectedAction->GetActionJointTrack();
								if(event)
								{
									char buffer[256];
									HWND name = GetDlgItem(hWindow,IDC_JOINT_STRING);
									GetWindowText(name,buffer,255);
									buffer[255] = 0;
									event->SetJointName(buffer);
								}
							}
						}
						break;
					case IDC_ANG_VELOCITY_STRING:
						{
							if(selectedAction)
							{
								IActionJointTrack * event = selectedAction->GetActionJointTrack();
								if(event)
								{
									char buffer[256];
									HWND name = GetDlgItem(hWindow,IDC_ANG_VELOCITY_STRING);
									GetWindowText(name,buffer,255);
									buffer[255] = 0;
									SINGLE angVel = atof(buffer);
									event->SetAngVelocity(angVel);
								}
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionJointTrack * event = selectedAction->GetActionJointTrack();
								if(event)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										event->SetTarget(targ);
									}
								}
							}
						}
						break;
					case IDC_SOURCE_COMBO:
						{
							if(selectedAction)
							{
								IActionJointTrack * event = selectedAction->GetActionJointTrack();
								if(event)
								{
									HWND combo = GetDlgItem(hWindow, IDC_SOURCE_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										event->SetSource(targ);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

struct SoundSrchEnum :public IArchetypeEnum
{
	HWND combo;
	const char * str;
	U32 soundDropLen;

	SoundSrchEnum(HWND hwnd, const char * curSel)
	{
		str = curSel;
		combo = hwnd;
		soundDropLen = 0;

		U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
		SendMessage(combo,CB_SETCURSEL,index,0);
	}

	virtual	BOOL32 ArchetypeEnum (const char * name, void *data, U32 size)
	{
/*		BASIC_DATA * sound = (BASIC_DATA*)data;
		if(sound->objClass == OC_BINARY && sound->bCanon)
		{
			if(strncmp(name,"SOUNDENTRY!!",12) == 0)
			{
				U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(name));

				TEXTMETRIC tm;
				HDC hdc = GetDC(combo);

				SelectObject (hdc,hFont);       
				GetTextMetrics (hdc, &tm);

				U32 len = 0;
				U32 x = 0;
				while(name[x])
				{
					int tmp;
					GetCharWidth32(hdc, name[x], name[x], &tmp);

					len += tmp;
					x++;
				}

				if(len > soundDropLen)
					soundDropLen = len;

				if(str && (strcmp(str,name) == 0))
				{
					SendMessage(combo,CB_SETCURSEL,index,0);
				}

				ReleaseDC (combo, hdc);
			}
		}
*/		return 1;
	}
};

BOOL CALLBACK actionSoundProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionSound * sound = selectedAction->GetActionSound();
				if(sound)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());

					IEffectTarget * targ = sound->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
						SendMessage(combo,CB_SETCURSEL,index,0);
					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					HWND hpCombo = GetDlgItem(hWindow, IDC_HARDPOINT_COMBO);
					index = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(hpCombo,CB_SETCURSEL,index,0);
					if(targ)
					{
						ITargetHp * hp = targ->GetFirstHardpoint();
						while(hp)
						{
							index = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hp->GetName()));
							if(sound->GetHpName() && (strcmp(sound->GetHpName(),hp->GetName()) == 0))
								SendMessage(hpCombo,CB_SETCURSEL,index,0);			
							hp = hp->GetNextHP();
						}
					}

					SoundSrchEnum soundArchEnum(GetDlgItem(hWindow,IDC_SOUND),sound->GetSoundEntry());
					ARCHLIST->EnumerateArchetypeData(&soundArchEnum);
					SendMessage(soundArchEnum.combo, CB_SETDROPPEDWIDTH, (WPARAM)soundArchEnum.soundDropLen, 0);

					char buffer[256];
					HWND minRangeWin = GetDlgItem(hWindow,IDC_MIN_RANGE);
					sprintf(buffer,"%f",sound->GetMinSound());
					SetWindowText(minRangeWin,buffer);

					HWND maxRangeWin = GetDlgItem(hWindow,IDC_MAX_RANGE);
					sprintf(buffer,"%f",sound->GetMaxSound());
					SetWindowText(maxRangeWin,buffer);

					CheckDlgButton(hWindow,IDC_LOOPING,sound->IsLooping() ? BST_CHECKED: BST_UNCHECKED);
			
					SetFocus(name);
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_LOOPING:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									if(IsDlgButtonChecked(hWindow,IDC_LOOPING) == BST_CHECKED)
									{
										sound->SetLooping(true);
									}
									else
									{
										sound->SetLooping(false);
									}
								}
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_MIN_RANGE:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									char buffer[256];
									HWND range = GetDlgItem(hWindow,IDC_MIN_RANGE);
									GetWindowText(range,buffer,255);
									buffer[255] = 0;
									sound->SetMinSound(atof(buffer));
								}
							}
						}
						break;
					case IDC_MAX_RANGE:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									char buffer[256];
									HWND range = GetDlgItem(hWindow,IDC_MAX_RANGE);
									GetWindowText(range,buffer,255);
									buffer[255] = 0;
									sound->SetMaxSound(atof(buffer));
								}
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										sound->SetTarget(targ);

										HWND hpCombo = GetDlgItem(hWindow, IDC_HARDPOINT_COMBO);
										SendMessage(hpCombo,CB_RESETCONTENT,0,0);
										index = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(hpCombo,CB_SETCURSEL,index,0);
										if(targ)
										{
											ITargetHp * hp = targ->GetFirstHardpoint();
											while(hp)
											{
												index = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hp->GetName()));
												if(sound->GetHpName() && (strcmp(sound->GetHpName(),hp->GetName()) == 0))
													SendMessage(hpCombo,CB_SETCURSEL,index,0);			
												hp = hp->GetNextHP();
											}
										}
									}

								}
							}
						}
						break;
					case IDC_HARDPOINT_COMBO:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									HWND combo = GetDlgItem(hWindow, IDC_HARDPOINT_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										char buffer[64];
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETLBTEXT,index,(DWORD)(buffer));
										if(strcmp(buffer,"None") == 0)
											sound->SetHpName("");
										else
											sound->SetHpName(buffer);
									}
								}
							}
						}
						break;
					case IDC_SOUND:
						{
							if(selectedAction)
							{
								IActionSound * sound = selectedAction->GetActionSound();
								if(sound)
								{
									HWND combo = GetDlgItem(hWindow, IDC_SOUND);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										char buffer[64];
										GetWindowText(GetDlgItem(hWindow,IDC_SOUND),buffer,63);
										buffer[63] = 0;
										if(strcmp(buffer,"None") == 0)
											sound->SetSoundEntry(NULL);
										else
											sound->SetSoundEntry(buffer);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK actionSwitchProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionSwitch * aSwitch = selectedAction->GetActionSwitch();
				if(aSwitch)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());

					HWND numChoices = GetDlgItem(hWindow,IDC_NUM_CHOICES);
					char buffer[256];
					itoa(aSwitch->GetSwitchNumber(),buffer,10);
					SetWindowText(numChoices,buffer);

					SetWindowLong(hWindow,GWL_USERDATA,10);
			
					SetFocus(name);
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_NUM_CHOICES:
						{
							if(GetWindowLong(hWindow,GWL_USERDATA) == 10)
							{
								if(selectedAction)
								{
									IActionSwitch * aSwitch = selectedAction->GetActionSwitch();
									if(aSwitch)
									{
										char buffer[256];
										HWND name = GetDlgItem(hWindow,IDC_NUM_CHOICES);
										GetWindowText(name,buffer,255);
										buffer[255] = 0;
										aSwitch->SetSwitchNumber(atoi(buffer));
										InvalidateRect(mainWindow,NULL,false);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK actionHideTargetProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionHideTarget * hideTarget = selectedAction->GetActionHideTarget();
				if(hideTarget)
				{
					HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
					SetWindowText(name,selectedAction->GetName());

					IEffectTarget * targ = hideTarget->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
						SendMessage(combo,CB_SETCURSEL,index,0);
					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					CheckDlgButton(hWindow,IDC_HIDE_CHECK,hideTarget->GetHide() ? BST_CHECKED: BST_UNCHECKED);

					SetFocus(name);
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_HIDE_CHECK:
						{
							if(selectedAction)
							{
								IActionHideTarget * hideTarget = selectedAction->GetActionHideTarget();
								if(hideTarget)
								{
									if(IsDlgButtonChecked(hWindow,IDC_HIDE_CHECK) == BST_CHECKED)
									{
										hideTarget->SetHide(true);
									}
									else
									{
										hideTarget->SetHide(false);
									}
								}
							}
						}
						break;
					}
				}
				break;
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ACTION_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_ACTION_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionHideTarget * hideTarget = selectedAction->GetActionHideTarget();
								if(hideTarget)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										hideTarget->SetTarget(targ);
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;}

BOOL CALLBACK actionAnimProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedAction)
			{
				IActionAnimation * anim = selectedAction->GetActionAnimation();
				if(anim)
				{
					HWND name = GetDlgItem(hWindow,IDC_NAME);
					SetWindowText(name,selectedAction->GetName());
					IEffectTarget * targ = anim->GetTarget();
					HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
					HWND animCombo = GetDlgItem(hWindow, IDC_ANIM_COMBO);
					U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(combo,CB_SETITEMDATA,index,0);
					if(!targ)
					{
						SendMessage(combo,CB_SETCURSEL,index,0);
						EnableWindow(animCombo,false);
					}
					else
					{
						EnableWindow(animCombo,true);
						U32 index = SendMessage(animCombo,CB_ADDSTRING,0,(DWORD)("None"));
						SendMessage(animCombo,CB_SETITEMDATA,index,0);
						if(!(anim->GetAnimation()))
						{
							SendMessage(animCombo,CB_SETCURSEL,index,0);
						}
						ITargetAnim * search = targ->GetFirstAnim();
						while(search)
						{
							index = SendMessage(animCombo,CB_ADDSTRING,0,(DWORD)(search->GetName()));
							SendMessage(animCombo,CB_SETITEMDATA,index,(DWORD)search);
							if(search == anim->GetAnimation())
							{
								SendMessage(animCombo,CB_SETCURSEL,index,0);
							}
							search = search->GetNextAnim();
						}
					}
					if(anim->IsNamedAnim())
					{
						SetWindowText(animCombo,anim->GetAnimName());
					}

					IEffectTarget * searchTarg = EFFECTFILE->GetFirstTarget();
					while(searchTarg)
					{
						index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)(searchTarg->GetName()));
						SendMessage(combo,CB_SETITEMDATA,index,(DWORD)searchTarg);
						if(searchTarg == targ)
						{
							SendMessage(combo,CB_SETCURSEL,index,0);
						}
						searchTarg = searchTarg->GetNextTarget();
					}

					CheckDlgButton(hWindow,IDC_LOOPING_CHECK,anim->IsLooping() ? BST_CHECKED: BST_UNCHECKED);
					CheckDlgButton(hWindow,IDC_FORCE_CALL,anim->IsForced() ? BST_CHECKED: BST_UNCHECKED);
			
					SetFocus(combo);
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_NAME:
						{
							if(selectedAction)
							{
								char buffer[256];
								HWND name = GetDlgItem(hWindow,IDC_NAME);
								GetWindowText(name,buffer,255);
								buffer[255] = 0;
								selectedAction->SetName(buffer);
							}
						}
						break;
					}
				}
				break;
			case CBN_EDITCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ANIM_COMBO:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									char buffer[256];
									HWND name = GetDlgItem(hWindow,IDC_ANIM_COMBO);
									GetWindowText(name,buffer,255);
									buffer[255] = 0;
									anim->SetNamedAnim(buffer);
								}
							}
						}
						break;
					}
				}
				break;
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_DELETE:
						{
							if(selectedAction)
							{
								EFFECTFILE->DeleteAction(selectedAction);
								selectedAction = NULL;
								EventGraph::Deselect();
								InvalidateRect(mainWindow,NULL,false);
							}
						}
						break;
					case IDC_FORCE_CALL:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									if(IsDlgButtonChecked(hWindow,IDC_FORCE_CALL) == BST_CHECKED)
									{
										anim->SetForced(true);
									}
									else
									{
										anim->SetForced(false);
									}
								}
							}
						}
						break;
					case IDC_LOOPING_CHECK:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									if(IsDlgButtonChecked(hWindow,IDC_LOOPING_CHECK) == BST_CHECKED)
									{
										anim->SetLooping(true);
									}
									else
									{
										anim->SetLooping(false);
									}
								}
							}
						}
						break;
					case IDC_SELECT_TARGET:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									if(anim->GetTarget())
									{
										InfoArea::SelectTarget(anim->GetTarget());
									}
								}
							}
						}
						break;
					case IDC_SELECT_ANIMATION:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									if(anim->GetAnimation() && anim->GetTarget())
									{
										InfoArea::SelectTargetAnim(anim->GetTarget(), anim->GetAnimation());
									}
								}
							}
						}
						break;
					}
				}
				break;
			case CBN_SELCHANGE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_COMBO:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									HWND combo = GetDlgItem(hWindow, IDC_TARGET_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										HWND animCombo = GetDlgItem(hWindow, IDC_ANIM_COMBO);
										IEffectTarget * targ = (IEffectTarget *)SendMessage(combo,CB_GETITEMDATA,index,0);
										anim->SetTarget(targ);
										anim->SetAnimation(NULL);
										SendMessage(animCombo,CB_RESETCONTENT,0,0);
										U32 index = SendMessage(animCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(animCombo,CB_SETITEMDATA,index,0);
										if(!targ)
										{
											EnableWindow(animCombo,false);
										}
										else
										{
											EnableWindow(animCombo,true);
											SendMessage(animCombo,CB_SETCURSEL,index,0);
											ITargetAnim * search = targ->GetFirstAnim();
											while(search)
											{
												index = SendMessage(animCombo,CB_ADDSTRING,0,(DWORD)(search->GetName()));
												SendMessage(animCombo,CB_SETITEMDATA,index,(DWORD)search);
												search = search->GetNextAnim();
											}
										}
									}
								}
							}
						}
						break;
					case IDC_ANIM_COMBO:
						{
							if(selectedAction)
							{
								IActionAnimation * anim = selectedAction->GetActionAnimation();
								if(anim)
								{
									HWND combo = GetDlgItem(hWindow, IDC_ANIM_COMBO);
									U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
									if(index != -1)
									{
										ITargetAnim * targ = (ITargetAnim *)SendMessage(combo,CB_GETITEMDATA,index,0);
										anim->SetAnimation(targ);										
									}
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK newActionProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND combo = GetDlgItem(hWindow, IDC_ACTION_TYPE);
			U32 index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Animation"));
			SendMessage(combo,CB_SETITEMDATA,index,0);
			SendMessage(combo,CB_SETCURSEL,index,0);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Particle Effect"));
			SendMessage(combo,CB_SETITEMDATA,index,1);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Game Event"));
			SendMessage(combo,CB_SETITEMDATA,index,2);
//			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Sound Action"));
//			SendMessage(combo,CB_SETITEMDATA,index,3);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Switch Action"));
			SendMessage(combo,CB_SETITEMDATA,index,4);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Hide Target Action"));
			SendMessage(combo,CB_SETITEMDATA,index,5);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Listen Action"));
			SendMessage(combo,CB_SETITEMDATA,index,6);
			index = SendMessage(combo,CB_ADDSTRING,0,(DWORD)("Joint Track Action"));
			SendMessage(combo,CB_SETITEMDATA,index,7);
			SetFocus(combo);
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDOK:
						{
							HWND combo = GetDlgItem(hWindow, IDC_ACTION_TYPE);
							U32 index = SendMessage(combo,CB_GETCURSEL,0,0);
							if(index != -1)
							{
								U32 val = SendMessage(combo,CB_GETITEMDATA,index,0);
								if(val == 0)//Animation
								{
									if(selectedEvent)
									{
										IEffectAction * anim = MakeEffectActionAnim();
										anim->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(anim);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 1)//ParticleEffect
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionParticle();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 2)//Game Event
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionGameEvent();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 3)//Sound Action
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionSound();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 4)//switch Action
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionSwitch();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 5)//hide target Action
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionHideTarget();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 6)//listen Action
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionListen();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
								else if(val == 7)//joint track Action
								{
									if(selectedEvent)
									{
										IEffectAction * part = MakeEffectActionJointTrack();
										part->SetNextAction(selectedEvent->GetFirstAction());
										selectedEvent->SetFirstAction(part);
										EventGraph::InvalidateEventGraph();
									}
								}
							}
							EndDialog(hWindow,1);
						}
						break;
					case IDCANCEL:
						{
							EndDialog(hWindow,0);
						}
						break;
					}
					break;
				}
			}
		}
		break;
	}
	return false;
};

BOOL CALLBACK eventProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND edit = GetDlgItem(hWindow, IDC_EVENT_NAME);
			SetWindowText(edit,selectedEvent->GetName());
			// SetFocus(edit);
			return false;
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ADD_ACTION:
						{
							if(DialogBox(hMainInst,MAKEINTRESOURCE(IDD_NEW_ACTION), mainWindow, newActionProc))
							{
							}
						}
						break;
					}
					break;
				}
			}
		}
		break;
	}
	return false;
}

BOOL CALLBACK paramListProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND listView = GetDlgItem(hWindow,IDC_PARAM_LIST);
			ListView_SetExtendedListViewStyle(listView,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
			oldListViewProc = (WNDPROC)(GetWindowLong(listView,GWL_WNDPROC));
			SetWindowLong(listView,GWL_WNDPROC,(DWORD)(keyCatcherProc));

			LVCOLUMN column;
			column.mask = LVCF_TEXT;
			column.pszText = "Parameter Name";
			ListView_InsertColumn(listView,0,&column);
			column.pszText = "Test Value";
			ListView_InsertColumn(listView,1,&column);

			RECT rect;
			GetClientRect(listView,&rect);

			ListView_SetColumnWidth(listView,0,(rect.right-rect.left)>>1);
			ListView_SetColumnWidth(listView,1,(rect.right-rect.left)>>1);

			IEffectParam * param = EFFECTFILE->GetFirstParam();
			while(param)
			{
				char buffer[256];
				LVITEM item;
				item.mask = LVIF_TEXT |LVIF_PARAM;
				item.lParam = (DWORD)param;
				strcpy(buffer,param->GetName());
				item.pszText = buffer;
				item.iItem = 0;
				item.iSubItem = 0;
				U32 ipos = ListView_InsertItem(listView,&item);
				item.iItem = ipos;
				item.iSubItem = 1;
				item.mask = LVIF_TEXT;
				sprintf(buffer,"%f",param->GetValue());
				item.pszText = buffer;
				ListView_SetItem(listView,&item);

				param = param->GetNextParam();
			}
		}
		break;
	case WM_NOTIFY:
		{
			HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
			NMHDR * header = (NMHDR *)lParam;
			if(header->hwndFrom == listView)
			{
				NMLISTVIEW * nmListView= (NMLISTVIEW *) lParam;
				if(nmListView->hdr.code == NM_DBLCLK)
				{
					if(nmListView->iItem != -1)
					{
						if(nmListView->iSubItem == 0)
						{
							bNameEdit = true;
							//editing the name
							listIndex = nmListView->iItem;
							LVITEM item;
							item.iItem = listIndex;
							item.iSubItem = 0;
							item.mask = LVIF_PARAM;
							ListView_GetItem(listView,&item);
							IEffectParam * param = (IEffectParam *)(item.lParam);
							if(param)
							{
								RECT itemRect;
								ListView_GetItemRect(listView,listIndex,&itemRect,LVIR_BOUNDS);
								U32 width = ListView_GetColumnWidth(listView,0);
								itemRect.right = itemRect.left+width;

								RECT listRect;
								POINT listPoint;
								GetWindowRect(listView,&listRect);
								listPoint.x = listRect.left;
								listPoint.y = listRect.top;
								ScreenToClient(hWindow,&listPoint);

								HWND editbox = GetDlgItem(hWindow, IDC_VALUE_EDIT);
								SetWindowPos(editbox,HWND_TOP,itemRect.left+listPoint.x+2,itemRect.top+listPoint.y,width,(itemRect.bottom-itemRect.top)+10,0);
								char buffer[256];
								strcpy(buffer,param->GetName());
								SetWindowText(editbox,buffer);
								SendMessage(editbox,EM_SETSEL ,0,-1);
								ShowWindow(editbox,true);
								SetFocus(editbox);
							}
						}
						else if (nmListView->iSubItem == 1)
						{
							bNameEdit = false;
							//editing the value
							listIndex = nmListView->iItem;
							LVITEM item;
							item.iItem = listIndex;
							item.iSubItem = 0;
							item.mask = LVIF_PARAM;
							ListView_GetItem(listView,&item);
							IEffectParam * param = (IEffectParam *)(item.lParam);
							if(param)
							{
								RECT itemRect;
								ListView_GetItemRect(listView,listIndex,&itemRect,LVIR_BOUNDS);
								U32 width = ListView_GetColumnWidth(listView,0);
								itemRect.left = itemRect.left+width;
								itemRect.right = itemRect.left+width;

								RECT listRect;
								POINT listPoint;
								GetWindowRect(listView,&listRect);
								listPoint.x = listRect.left;
								listPoint.y = listRect.top;
								ScreenToClient(hWindow,&listPoint);

								HWND editbox = GetDlgItem(hWindow, IDC_VALUE_EDIT);
								SetWindowPos(editbox,HWND_TOP,itemRect.left+listPoint.x+2,itemRect.top+listPoint.y,width,(itemRect.bottom-itemRect.top)+10,0);
								char buffer[256];
								sprintf(buffer,"%f",param->GetValue());
								SetWindowText(editbox,buffer);
								SendMessage(editbox,EM_SETSEL ,0,-1);
								ShowWindow(editbox,true);
								SetFocus(editbox);
							}
						}
					}
					else //must be a new one
					{
						listIndex = -1;
						bNameEdit = true;
						//editing the value

						RECT itemRect;
						U32 width = ListView_GetColumnWidth(listView,0);
						itemRect.left = 0;
						itemRect.right = width;
						itemRect.top = 0;
						itemRect.bottom = 20;

						RECT listRect;
						POINT listPoint;
						GetWindowRect(listView,&listRect);
						listPoint.x = listRect.left;
						listPoint.y = listRect.top;
						ScreenToClient(hWindow,&listPoint);

						HWND editbox = GetDlgItem(hWindow, IDC_VALUE_EDIT);
						SetWindowPos(editbox,HWND_TOP,itemRect.left+listPoint.x+2,itemRect.top+listPoint.y,width,(itemRect.bottom-itemRect.top)+10,0);
						SetWindowText(editbox,"");
						SendMessage(editbox,EM_SETSEL ,0,-1);
						ShowWindow(editbox,true);
						SetFocus(editbox);
					}
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			if( ((HWND)lParam) == GetDlgItem(hWindow, IDC_VALUE_EDIT) )
			{
				U32 wNotifyCode = HIWORD(wParam); 
				if(wNotifyCode == EN_KILLFOCUS)
				{
					HWND editbox = GetDlgItem(hWindow, IDC_VALUE_EDIT);
					ShowWindow(editbox,false);

					char buf[256];
					GetWindowText(editbox,buf,256);
					char * tmp = strchr(buf,0x0d);
					if(tmp)
					{
						char buffer[256];
						strcpy(buffer,tmp+2);
						strcpy(tmp,buffer);
					}
					buf[255] = 0;

					if(listIndex != -1)
					{
						if(bNameEdit)
						{
							HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
							LVITEM item;
							item.iItem = listIndex;
							item.iSubItem = 0;
							item.mask = LVIF_PARAM;
							ListView_GetItem(listView,&item);
							IEffectParam * param = (IEffectParam *)(item.lParam);
							param->SetName(buf);

							item.mask = LVIF_TEXT;
							item.pszText = buf;
							ListView_SetItem(listView,&item);
						}
						else
						{
							HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
							LVITEM item;
							item.iItem = listIndex;
							item.iSubItem = 0;
							item.mask = LVIF_PARAM;
							ListView_GetItem(listView,&item);
							IEffectParam * param = (IEffectParam *)(item.lParam);

							param->SetValue((SINGLE)(atof(buf)));

							sprintf(buf,"%f",param->GetValue());
							item.mask = LVIF_TEXT;
							item.iSubItem = 1;
							item.pszText = buf;
							ListView_SetItem(listView,&item);
						}
					}
					else//new one
					{
						HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
						IEffectParam * param = EFFECTFILE->AddParam(buf);
						char buffer[256];
						LVITEM item;
						item.mask = LVIF_TEXT |LVIF_PARAM;
						item.lParam = (DWORD)param;
						strcpy(buffer,param->GetName());
						item.pszText = buffer;
						item.iItem = 0;
						item.iSubItem = 0;
						U32 ipos = ListView_InsertItem(listView,&item);
						item.iItem = ipos;
						item.iSubItem = 1;
						item.mask = LVIF_TEXT;
						sprintf(buffer,"%f",param->GetValue());
						item.pszText = buffer;
						ListView_SetItem(listView,&item);
					}
				}
				else if(wNotifyCode == EN_CHANGE)
				{
					HWND editbox = GetDlgItem(hWindow, IDC_VALUE_EDIT);
					if(SendMessage(editbox,EM_GETLINECOUNT,0,0) > 1)
					{
						SetFocus(GetDlgItem(hWindow, IDC_PARAM_LIST));
					}
				}
			}
		}
		break;
	case WM_KEYDOWN:
		{
			switch (LOWORD(wParam))
			{
			case VK_BACK:		//fallthrough intentional
			case VK_DELETE:
				{
					HWND listView = GetDlgItem(hWindow, IDC_PARAM_LIST);
					U32 index = ListView_GetSelectionMark(listView);
					if(index != -1)
					{
						LVITEM item;
						item.iItem = index;
						item.iSubItem = 0;
						item.mask = LVIF_PARAM;
						ListView_GetItem(listView,&item);
						IEffectParam * param = (IEffectParam *)(item.lParam);

						EFFECTFILE->RemoveParam(param);
						ListView_DeleteItem(listView,index);
					}
				}
				break;
			}
		}
		break;
	}
	return false;
}

BOOL CALLBACK targetInfoProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			if(selectedTarg)
			{
				HWND meshName = GetDlgItem(hWindow, IDC_MESH_NAME);
				char * name = selectedTarg->GetMeshName();
				if(name[0])
					SetWindowText(meshName,name);
				else
					SetWindowText(meshName,"No Mesh");							

				HWND editName = GetDlgItem(hWindow, IDC_TARGET_NAME_2);
				SetWindowText(editName,selectedTarg->GetName());

				HWND hpList = GetDlgItem(hWindow, IDC_HARDPOINTLIST);
				ITargetHp * hp = selectedTarg->GetFirstHardpoint();
				while(hp)
				{
					U32 index = SendMessage(hpList,LB_ADDSTRING,0,(DWORD)(hp->GetName()));
					SendMessage(hpList,LB_SETITEMDATA,index,(DWORD)hp);

					hp = hp->GetNextHP();
				}

				HWND animList = GetDlgItem(hWindow, IDC_ANIMATIONLIST);
				ITargetAnim * anim = selectedTarg->GetFirstAnim();
				while(anim)
				{
					U32 index = SendMessage(animList,LB_ADDSTRING,0,(DWORD)(anim->GetName()));
					SendMessage(animList,LB_SETITEMDATA,index,(DWORD)anim);

					anim = anim->GetNextAnim();
				}		

				return false;
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case LBN_DBLCLK:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ANIMATIONLIST:
						{
							if(selectedTarg)
							{
								HWND list = GetDlgItem(hWindow, IDC_ANIMATIONLIST);
								U32 index = SendMessage(list,LB_GETCURSEL,0,0);
								if(index != LB_ERR)
								{
									ITargetAnim * targ = (ITargetAnim *) (SendMessage(list,LB_GETITEMDATA,index,0));
									if(targ)
									{
										InfoArea::SelectTargetAnim(selectedTarg,targ);
									}
								}
							}
						}
						break;
					}
				}
				break;
			case EN_UPDATE:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_NAME_2:
						{
							char buffer[256];
							HWND editName = GetDlgItem(hWindow, IDC_TARGET_NAME_2);
							GetWindowText(editName,buffer,256);
							buffer[255] = 0;
							if(selectedTarg)
								selectedTarg->SetName(buffer);
						}
						break;
					}
				}
				break;
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TO_PARENT:
						{
							InfoArea::SelectTargetList();
						}
						break;
					case IDC_LOAD_MESH:
						{
							if(selectedTarg)
							{
								char buffer[255];
								buffer[0] = 0;
								OPENFILENAME fileName;
								memset(&fileName,0,sizeof(OPENFILENAME));
								fileName.lStructSize = sizeof(OPENFILENAME);
								fileName.lpstrFilter = "3DB Mesh\0*.3db;*.cmp\0\0";
								fileName.nFilterIndex = 1;
								fileName.lpstrFile = buffer;
								fileName.nMaxFile = 255;
								fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
    									OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
								if(GetOpenFileName(&fileName))
								{
									if(selectedTarg)
										selectedTarg->LoadMesh(buffer);
									HWND meshName = GetDlgItem(hWindow, IDC_MESH_NAME);
									char * name = selectedTarg->GetMeshName();
									if(name[0])
										SetWindowText(meshName,name);
									else
										SetWindowText(meshName,"No Mesh");		
									
									HWND hpList = GetDlgItem(hWindow, IDC_HARDPOINTLIST);
									SendMessage(hpList,LB_RESETCONTENT,0,0);
									ITargetHp * hp = selectedTarg->GetFirstHardpoint();
									while(hp)
									{
										U32 index = SendMessage(hpList,LB_ADDSTRING,0,(DWORD)(hp->GetName()));
										SendMessage(hpList,LB_SETITEMDATA,index,(DWORD)hp);

										hp = hp->GetNextHP();
									}

									HWND animList = GetDlgItem(hWindow, IDC_ANIMATIONLIST);
									SendMessage(animList,LB_RESETCONTENT,0,0);
									ITargetAnim * anim = selectedTarg->GetFirstAnim();
									while(anim)
									{
										U32 index = SendMessage(animList,LB_ADDSTRING,0,(DWORD)(anim->GetName()));
										SendMessage(animList,LB_SETITEMDATA,index,(DWORD)anim);

										anim = anim->GetNextAnim();
									}		
								}
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}
	return false;
}

BOOL CALLBACK targetListProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
			SendMessage(list,LB_RESETCONTENT,0,0);
			IEffectTarget * target = EFFECTFILE->GetFirstTarget();
			while(target)
			{
				U32 index = SendMessage(list,LB_ADDSTRING,0,(DWORD)(target->GetName()));
				SendMessage(list,LB_SETITEMDATA,index,(DWORD)target);
				target = target->GetNextTarget();
			}
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case LBN_DBLCLK:
				{
					switch(LOWORD(wParam))
					{
					case IDC_TARGET_LIST:
						{
							HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
							U32 index = SendMessage(list,LB_GETCURSEL,0,0);
							if(index != LB_ERR)
							{
								IEffectTarget * targ = (IEffectTarget *) (SendMessage(list,LB_GETITEMDATA,index,0));
								if(targ)
								{
									InfoArea::SelectTarget(targ);
								}
							}
						}
						break;
					}
				}
				break;
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_ADD_TARGET:
						{
							if(DialogBox(hMainInst,MAKEINTRESOURCE(IDD_ADD_TARGET), mainWindow, targetAddProc))
							{
								IEffectTarget * targ = EFFECTFILE->AddTarget(stringValue);
								HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
								U32 index = SendMessage(list,LB_ADDSTRING,0,(DWORD)(targ->GetName()));
								SendMessage(list,LB_SETITEMDATA,index,(DWORD)targ);
							}
							break;
						}
					case IDC_DELETE_TARGET:
						{
							HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
							U32 index = SendMessage(list,LB_GETCURSEL,0,0);
							if(index != LB_ERR)
							{
								IEffectTarget * targ = (IEffectTarget *) (SendMessage(list,LB_GETITEMDATA,index,0));
								if(targ)
								{
									EFFECTFILE->RemoveEffectTarget(targ);
								}
								SendMessage(list,LB_DELETESTRING,index,0);
							}
							break;
						}
					case IDC_SELECT_TARGET:
						{
							HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
							U32 index = SendMessage(list,LB_GETCURSEL,0,0);
							if(index != LB_ERR)
							{
								IEffectTarget * targ = (IEffectTarget *) (SendMessage(list,LB_GETITEMDATA,index,0));
								if(targ)
								{
									InfoArea::SelectTarget(targ);
								}
							}
							break;
						}
					case IDC_MOVEUP_TARGET:
						{
							HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
							U32 index = SendMessage(list,LB_GETCURSEL,0,0);
							if(index != LB_ERR)
							{
								IEffectTarget * targ = (IEffectTarget *) (SendMessage(list,LB_GETITEMDATA,index,0));
								if(targ)
								{
									EFFECTFILE->MoverTargetUp(targ);
								}
								SendMessage(list,LB_RESETCONTENT,0,0);
								IEffectTarget * target = EFFECTFILE->GetFirstTarget();
								while(target)
								{
									U32 index = SendMessage(list,LB_ADDSTRING,0,(DWORD)(target->GetName()));
									SendMessage(list,LB_SETITEMDATA,index,(DWORD)target);
									if(target == targ)
									{
										SendMessage(list,LB_SETCURSEL,index,0);
									}
									target = target->GetNextTarget();
								}
							}
							break;
						}
					case IDC_MOVEDOWN_TARGET:
						{
							HWND list = GetDlgItem(hWindow, IDC_TARGET_LIST);
							U32 index = SendMessage(list,LB_GETCURSEL,0,0);
							if(index != LB_ERR)
							{
								IEffectTarget * targ = (IEffectTarget *) (SendMessage(list,LB_GETITEMDATA,index,0));
								if(targ)
								{
									EFFECTFILE->MoverTargetDown(targ);
								}
								SendMessage(list,LB_RESETCONTENT,0,0);
								IEffectTarget * target = EFFECTFILE->GetFirstTarget();
								while(target)
								{
									U32 index = SendMessage(list,LB_ADDSTRING,0,(DWORD)(target->GetName()));
									SendMessage(list,LB_SETITEMDATA,index,(DWORD)target);
									if(target == targ)
									{
										SendMessage(list,LB_SETCURSEL,index,0);
									}
									target = target->GetNextTarget();
								}
							}
							break;
						}
					}
					break;
				}
			}
		}
	}

	return false;
}

BOOL CALLBACK targetAddProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			HWND edit = GetDlgItem(hWindow, IDC_TARGET_NAME);
			SetWindowText(edit,"NewTarget");
			SetFocus(edit);
		}
		break;
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
					case IDC_OK_BUTTON:
						{
							GetWindowText(GetDlgItem(hWindow, IDC_TARGET_NAME),stringValue,255);
							EndDialog(hWindow,1);
						}
						break;
					case IDC_CANCEL_BUTTON:
						{
							EndDialog(hWindow,0);
						}
						break;
					}
					break;
				}
			}
		}
		break;
	}
	return false;
}

