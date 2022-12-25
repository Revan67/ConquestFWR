//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Toolbar.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Toolbar.cpp 87    9/30/00 1:35a Jasony $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IHotButton.h"
#include "IShapeLoader.h"
#include "IImageReader.h"
#include "DrawAgent.h"
#include "Camera.h"
#include "Startup.h"
#include "StatusBar.h"
#include "IToolbar.h"
#include "IStatic.h"
#include "IProgressStatic.h"
#include "IEdit2.h"
#include "IHotStatic.h"
#include "ITabControl.h"
#include "IShipSilButton.h"
#include "ObjList.h"
#include "MPart.h"
#include "IIcon.h"
#include "IQueueControl.h"
#include "Sysmap.h"
#include "ScrollingText.h"

#include "Hotkeys.h"
#include "DBHotkeys.h"
#include <DStatic.h>
#include <DProgressStatic.h>
#include <DEdit.h>
#include <DHotButton.h>
#include <DHotStatic.h>
#include <DShipSilButton.h>
#include <DTabControl.h>
#include <DIcon.h>
#include <DQueueControl.h>

#include <DataParser.h>
#include <ViewCnst.h>

struct MenuBriefing;
Frame * __stdcall CreateMenuBriefing (void);

//--------------------------------------------------------------------------//
//
struct CONTROL_NODE
{
	CONTROL_NODE * pNext;
	const char * name;
	U32 offset;
	GENBASE_TYPE type;
	HOTBUTTONTYPE::TYPE buttonType;

	COMPTR<IHotButton> pButton;
	COMPTR<IStatic> pStatic;
	COMPTR<IProgressStatic> pProgressStatic;
	COMPTR<IEdit2> pEdit;
	COMPTR<IHotStatic> pTech;
	COMPTR<IShipSilButton> pSil;
	COMPTR<ITabControl> pTab;
	COMPTR<IIcon> pIcon;
	COMPTR<IQueueControl> pQControl;

	GENRESULT getControl (void ** ppControl)
	{
		GENRESULT result = GR_OK;

		if (pButton)
		{
			*ppControl = pButton.ptr;
			pButton->AddRef();
			goto Done;
		}
		if (pStatic)
		{
			*ppControl = pStatic.ptr;
			pStatic->AddRef();
			goto Done;
		}
		if(pProgressStatic)
		{
			*ppControl = pProgressStatic.ptr;
			pProgressStatic->AddRef();
			goto Done;
		}
		if (pEdit)
		{
			*ppControl = pEdit.ptr;
			pEdit->AddRef();
			goto Done;
		}
		if (pTech)
		{
			*ppControl = pTech.ptr;
			pTech->AddRef();
			goto Done;
		}
		if (pSil)
		{
			*ppControl = pSil.ptr;
			pSil->AddRef();
			goto Done;
		}
		if (pTab)
		{
			*ppControl = pTab.ptr;
			pTab->AddRef();
			goto Done;
		}
		if (pIcon)
		{
			*ppControl = pIcon.ptr;
			pIcon->AddRef();
			goto Done;
		}
		if (pQControl)
		{
			*ppControl = pQControl.ptr;
			pQControl->AddRef();
			goto Done;
		}
		*ppControl = 0;
		result = GR_GENERIC;
	Done:
		return result;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	void initControl (void * data, BaseHotRect * parent, IShapeLoader * loader, bool bContextBehavior)
	{
		if (type == GBT_HOTBUTTON)
		{
			HOTBUTTON_DATA * hotdata = (HOTBUTTON_DATA *)  (((char *)data) + offset);
			if (buttonType==HOTBUTTONTYPE::HOTBUTTON)
				pButton->InitHotButton(*hotdata, parent, loader); 
			else if(buttonType==HOTBUTTONTYPE::BUILD)
				pButton->InitBuildButton(*((BUILDBUTTON_DATA*)hotdata), parent, loader); 
			else if(buttonType==HOTBUTTONTYPE::RESEARCH)
				pButton->InitResearchButton(*((RESEARCHBUTTON_DATA*)hotdata), parent, loader); 
			else
				pButton->InitMultiHotButton(*((MULTIHOTBUTTON_DATA*)hotdata), parent, loader); 
			if (bContextBehavior)
				pButton->EnableContextMenuBehavior();
		}
		else
		if (type == GBT_STATIC)
		{
			STATIC_DATA * hotdata = (STATIC_DATA *)  (((char *)data) + offset);
			pStatic->InitStatic(*hotdata, parent); 
		}
		else
		if (type == GBT_PROGRESS_STATIC)
		{
			PROGRESS_STATIC_DATA * hotdata = (PROGRESS_STATIC_DATA *)  (((char *)data) + offset);
			pProgressStatic->InitProgressStatic(*hotdata, parent); 
		}
		else
		if (type == GBT_EDIT)
		{
			EDIT_DATA * hotdata = (EDIT_DATA *)  (((char *)data) + offset);
			pEdit->InitEdit(*hotdata, parent); 
			pEdit->EnableToolbarBehavior();
		}
		else
		if (type == GBT_HOTSTATIC)
		{
			HOTSTATIC_DATA * hotdata = (HOTSTATIC_DATA *)  (((char *)data) + offset);
			pTech->InitHotStatic(*hotdata, parent, loader); 
		}
		else
		if (type == GBT_SHIPSILBUTTON)
		{
			SHIPSILBUTTON_DATA * hotdata = (SHIPSILBUTTON_DATA *) (((char *)data)+ offset);
			pSil->InitShipSilButton(*hotdata,parent,loader);
		}
		else
		if (type == GBT_TABCONTROL)
		{
			TABCONTROL_DATA * hotdata = (TABCONTROL_DATA *) (((char *)data)+ offset);
			pTab->InitTab(*hotdata,parent,loader);
		}
		else
		if (type == GBT_ICON)
		{
			ICON_DATA * icondata = (ICON_DATA *) (((char *)data)+ offset);
			pIcon->InitIcon(*icondata,parent,loader);
		}
		else
		if (type == GBT_QUEUECONTROL)
		{
			QUEUECONTROL_DATA * qdata = (QUEUECONTROL_DATA *) (((char *)data)+ offset);
			pQControl->InitQueueControl(*qdata,parent);
		}
	}
};
//--------------------------------------------------------------------------//
//
struct Menu_context : public BaseHotRect, IToolbar
{
	//
	// interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Menu_context)
	DACOM_INTERFACE_ENTRY(IToolbar)
	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	//
	CONTROL_NODE * controlList;
	Menu_context * pNext;
	Menu_context * pTabMenu;
	const char * name;
	int  arrayIndex;		// 0 based
	U32 toolbarID;
	M_OBJCLASS mObjClass;
	COMPTR<ITabControl> pTabControl;

	//
	// instance methods
	//

	Menu_context (void) : BaseHotRect(TOOLBAR)
	{
		bInvisible = true;
		resPriority = parent->resPriority;
	}

	~Menu_context (void)
	{
		if (pTabMenu!=0)
		{
			Menu_context * mNode = pTabMenu;

			while (mNode)
			{
				mNode = mNode->pNext;
				U32 count = pTabMenu->GetBase()->Release();
				CQASSERT(count==0);
				pTabMenu = mNode;
			}
		}
		
		flushControlList();
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IToolbar methods */

	virtual GENRESULT __stdcall GetControl (const char *buttonName, void ** ppControl)
	{
		GENRESULT result = GR_GENERIC;
		CONTROL_NODE * node = controlList;

		*ppControl = 0;

		while (node)
		{
			if (strcmp(buttonName, node->name) == 0)
			{
				result = node->getControl(ppControl);
				break;
			}
			node = node->pNext;
		}

		// now look through tab controls

		if (result!=GR_OK)
		{
			Menu_context * mNode = pTabMenu;

			while (mNode)
			{
				if ((result = mNode->GetControl(buttonName, ppControl)) == GR_OK)
					break;
				mNode = mNode->pNext;
			}
		}

		return result;
	}

	virtual GENRESULT __stdcall GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race)
	{
		*ppMenu = 0;
		return GR_GENERIC;
	}

	virtual void SetVisible (bool bVisible)
	{
		if (bVisible==false)
		{
			bool bOrigFocus = bHasFocus;
			BaseHotRect::Notify(CQE_KILL_FOCUS, 0);
			bHasFocus = bOrigFocus;
			bInvisible = true;
		}
		else
		{
			bInvisible = false;
			if (bHasFocus)
				BaseHotRect::Notify(CQE_SET_FOCUS, 0);
		}

		// send a mouse update
		{
			MSG msg;
			S32 x, y;

			WM->GetCursorPos(x, y);

			msg.hwnd = hMainWindow;
			msg.message = WM_MOUSEMOVE;
			msg.wParam = 0;
			msg.lParam = MAKELPARAM(x,y);
			Notify(WM_MOUSEMOVE, &msg);
		}
	}

	virtual bool GetVisible (void)
	{
		return !bInvisible;
	}

	virtual void SetToolbarID (U32 id)
	{
		toolbarID = id;
	}

	virtual U32 GetToolbarID (void)
	{
		return toolbarID;
	}

	virtual void GetSystemMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
	}

	virtual void GetSectorMapRect(S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
	}
	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_LHOTBUTTON:
			onLeftButtonEvent((U32)param);
			return GR_OK;
		case CQE_RHOTBUTTON:
			onRightButtonEvent((U32)param);
			return GR_OK;
		case CQE_LDBLHOTBUTTON:
			onLeftDblButtonEvent((U32)param);
			return GR_OK;
		case CQE_SET_FOCUS:
			if (bInvisible)
			{
				onSetFocus(true);
				return GR_OK;
			}
			break;

		case CQE_KILL_FOCUS:
			if (bInvisible)
			{
				onSetFocus(false);
				return GR_OK;
			}
			break;
		}
		if (bInvisible && message == CQE_ENDFRAME)
			return GR_OK;
		return BaseHotRect::Notify(message, param);
	}

	/* Menu_context methods */

	void flushControlList (void)
	{
		CONTROL_NODE * node;
		
		while ((node = controlList) != 0)
		{
			controlList = controlList->pNext;
			delete node;
		}
	}

	void onLeftButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		while (node)
		{
			next = node->pNext;
			node->client->OnLeftButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

	void onRightButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		while (node)
		{
			next = node->pNext;
			node->client->OnRightButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

	void onLeftDblButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		while (node)
		{
			next = node->pNext;
			node->client->OnLeftDblButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

};


//--------------------------------------------------------------------------//
//
struct Menu_tb : public Frame, IToolbar
{
	typedef const char SHAPENAMESTYPE[GT_PATH];

	struct createCallback : IParserCallback
	{
		CONTROL_NODE * pList;
		Menu_context * menuList;
		COMPTR<IDAComponent> pComp;
		COMPTR<ITabControl> pLastTab;
		PGENTYPE pButtonType;
		PGENTYPE pBuildButtonType;
		PGENTYPE pResearchButtonType;
		PGENTYPE pMultiButtonType;
		PGENTYPE pHotStaticType;
		PGENTYPE pShipSilButtonType;
		PGENTYPE pTabType;
		PGENTYPE pIconType;
		PGENTYPE pQType;
		bool bRecursion;
		const char * arrayVarName;
		int arrayIndex;		// zero-based index that we want
		int arraySize;		// number of elements in the array
		U32 baseOffset;
		char * pBaseData;
		const char * vfxShapeType;
		const SHAPENAMESTYPE * vfxBarShapeType;
		const RECT * pContextRect;
		RECT sysmapRect;
		RECT sectorMapRect;
		U32 topBarX,topBarY;
		int commonMenu;
		M_OBJCLASS mObjClass;


		createCallback (PGENTYPE _pButtonType, PGENTYPE _pBuildButtonType, PGENTYPE _pResearchButtonType,PGENTYPE _pMultiButtonType,
			PGENTYPE _pHotStaticType, PGENTYPE _pShipSilButtonType, PGENTYPE _pTabType, PGENTYPE _pIconType, 
			PGENTYPE _pQType, char * _pBaseData, int _whichMenu)
		{
			baseOffset = 0;
			bRecursion = false;
			arrayVarName = 0;
			pList = 0;
			menuList = 0;
			pButtonType = _pButtonType;
			pBuildButtonType = _pBuildButtonType;
			pResearchButtonType = _pResearchButtonType;
			pMultiButtonType = _pMultiButtonType;
			pHotStaticType = _pHotStaticType;
			pShipSilButtonType = _pShipSilButtonType;
			pTabType = _pTabType;
			pIconType = _pIconType;
			pQType = _pQType;
			pBaseData = _pBaseData;
			vfxShapeType = 0;
			vfxBarShapeType = 0;
			pContextRect = 0;
			arrayIndex = 0;
			arraySize = 0;
			mObjClass = static_cast<M_OBJCLASS>(0);
			if ((commonMenu = _whichMenu) != 0)
				commonMenu--;
		}
	
		/* IParserCallback methods */

		BOOL32 __stdcall VarInstance (IDataParser *newParser, const VARIABLEDESC & varDesc)
		{
			if (baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "vfxShapeType") == 0)
			{
				vfxShapeType = (const char *) (pBaseData + varDesc.offset);
			}
			else if(baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "vfxToolBar") == 0)
			{
				vfxBarShapeType = (SHAPENAMESTYPE *) (pBaseData + varDesc.offset);
			}
			else
			if (baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "contextRect") == 0)
			{
				pContextRect = (RECT *) (pBaseData + varDesc.offset);
			}
			else
			if(baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "sysmapRect") == 0)
			{
				sysmapRect = ((RECT *) (pBaseData + varDesc.offset))[commonMenu];
			}
			else
			if(baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "sectorMapRect") == 0)
			{
				sectorMapRect = ((RECT *) (pBaseData + varDesc.offset))[commonMenu];
			}
			else
			if (baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "topBarX") == 0)
			{
				topBarX = *((U32 *) (pBaseData + varDesc.offset));
			}
			else
			if (baseOffset==0 && varDesc.varName && strcmp(varDesc.varName, "topBarY") == 0)
			{
				topBarY = *((U32 *) (pBaseData + varDesc.offset));
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "M_OBJCLASS") == 0)
			{
				mObjClass = *((M_OBJCLASS *) (pBaseData + baseOffset + varDesc.offset));
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "HOTBUTTON_DATA") == 0)
			{
				HOTBUTTON_DATA * hotdata = (HOTBUTTON_DATA *)  (((char *)pBaseData) + varDesc.offset + baseOffset);
				if(hotdata->baseImage)
				{
					CONTROL_NODE * pNode = new CONTROL_NODE;

					pNode->pNext = pList;
					pList = pNode;
					pNode->name = varDesc.varName;
					pNode->offset = varDesc.offset + baseOffset;
					pNode->type = GBT_HOTBUTTON;
					pNode->buttonType = HOTBUTTONTYPE::HOTBUTTON;

					GENDATA->CreateInstance(pButtonType, pComp);				
					pComp->QueryInterface("IHotButton", pNode->pButton);	
				}
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "BUILDBUTTON_DATA") == 0)
			{
				BUILDBUTTON_DATA * hotdata = (BUILDBUTTON_DATA *)  (((char *)pBaseData) + varDesc.offset + baseOffset);
				if(hotdata->baseImage)
				{
					CONTROL_NODE * pNode = new CONTROL_NODE;

					pNode->pNext = pList;
					pList = pNode;
					pNode->name = varDesc.varName;
					pNode->offset = varDesc.offset + baseOffset;
					pNode->type = GBT_HOTBUTTON;
					pNode->buttonType = HOTBUTTONTYPE::BUILD;

					GENDATA->CreateInstance(pBuildButtonType, pComp);				
					pComp->QueryInterface("IHotButton", pNode->pButton);		
				}
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "RESEARCHBUTTON_DATA") == 0)
			{
				RESEARCHBUTTON_DATA * hotdata = (RESEARCHBUTTON_DATA *)  (((char *)pBaseData) + varDesc.offset + baseOffset);
				if(hotdata->baseImage)
				{
					CONTROL_NODE * pNode = new CONTROL_NODE;

					pNode->pNext = pList;
					pList = pNode;
					pNode->name = varDesc.varName;
					pNode->offset = varDesc.offset + baseOffset;
					pNode->type = GBT_HOTBUTTON;
					pNode->buttonType = HOTBUTTONTYPE::RESEARCH;

					GENDATA->CreateInstance(pResearchButtonType, pComp);				
					pComp->QueryInterface("IHotButton", pNode->pButton);
				}
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "MULTIHOTBUTTON_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_HOTBUTTON;
				pNode->buttonType = HOTBUTTONTYPE::MULTI;

				GENDATA->CreateInstance(pMultiButtonType, pComp);				
				pComp->QueryInterface("IHotButton", pNode->pButton);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "HOTSTATIC_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_HOTSTATIC;

				GENDATA->CreateInstance(pHotStaticType, pComp);				
				pComp->QueryInterface("IHotStatic", pNode->pTech);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "STATIC_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_STATIC;

				if (((STATIC_DATA *)(pBaseData+pNode->offset))->staticType[0] == 0)
				{
					CQBOMB3("Missing datatype for %s[%d].%s", (arrayVarName)?arrayVarName:"", arrayIndex, varDesc.varName);
				}

				GENDATA->CreateInstance(((STATIC_DATA *)(pBaseData+pNode->offset))->staticType, pComp);				
				pComp->QueryInterface("IStatic", pNode->pStatic);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "PROGRESS_STATIC_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_PROGRESS_STATIC;

				if (((PROGRESS_STATIC_DATA *)(pBaseData+pNode->offset))->staticType[0] == 0)
				{
					CQBOMB3("Missing datatype for %s[%d].%s", (arrayVarName)?arrayVarName:"", arrayIndex, varDesc.varName);
				}

				GENDATA->CreateInstance(((PROGRESS_STATIC_DATA *)(pBaseData+pNode->offset))->staticType, pComp);				
				pComp->QueryInterface("IProgressStatic", pNode->pProgressStatic);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "EDIT_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_EDIT;

				if (((EDIT_DATA *)(pBaseData+pNode->offset))->editType[0] == 0)
				{
					CQBOMB3("Missing datatype for %s[%d].%s", (arrayVarName)?arrayVarName:"", arrayIndex, varDesc.varName);
				}

				GENDATA->CreateInstance(((EDIT_DATA *)(pBaseData+pNode->offset))->editType, pComp);				
				pComp->QueryInterface("IEdit2", pNode->pEdit);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "SHIPSILBUTTON_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_SHIPSILBUTTON;

				GENDATA->CreateInstance(pShipSilButtonType, pComp);				
				pComp->QueryInterface("IShipSilButton", pNode->pSil);						
			}
			else
			if (varDesc.typeName && strcmp(varDesc.typeName, "TABCONTROL_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;

				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_TABCONTROL;

				GENDATA->CreateInstance(pTabType, pComp);				
				pComp->QueryInterface("ITabControl", pNode->pTab);		
				pLastTab = pNode->pTab;
			}
			else
			if(varDesc.typeName && strcmp(varDesc.typeName, "ICON_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;
				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_ICON;

				GENDATA->CreateInstance(pIconType, pComp);				
				pComp->QueryInterface("IIcon", pNode->pIcon);						
			}
			else
			if(varDesc.typeName && strcmp(varDesc.typeName, "QUEUECONTROL_DATA") == 0)
			{
				CONTROL_NODE * pNode = new CONTROL_NODE;
				pNode->pNext = pList;
				pList = pNode;
				pNode->name = varDesc.varName;
				pNode->offset = varDesc.offset + baseOffset;
				pNode->type = GBT_QUEUECONTROL;

				GENDATA->CreateInstance(pQType, pComp);				
				pComp->QueryInterface("IQueueControl", pNode->pQControl);						
			}
			else
			if (bRecursion==false && varDesc.kind == VARIABLEDESC::ARRAY && varDesc.varName)
			{
				arrayVarName = varDesc.varName;
				baseOffset = varDesc.offset;
				arraySize = varDesc.arraySize;
				newParser->Enumerate(this);
				arrayVarName = 0;
				baseOffset = 0;
			}
			else // see if we need to create a submenu
			if (bRecursion==false && varDesc.kind==VARIABLEDESC::RECORD && (varDesc.varName || arrayVarName))
			{
				createCallback callback(pButtonType, pBuildButtonType, pResearchButtonType,pMultiButtonType, pHotStaticType, pShipSilButtonType, pTabType, pIconType, pQType, pBaseData, commonMenu+1);
				Menu_context * pNode = new DAComponent<Menu_context>;

				if (arrayVarName)
				{
					pNode->name = arrayVarName;
					if (menuList && menuList->name==pNode->name)		// members of same array
						pNode->arrayIndex = callback.arrayIndex = menuList->arrayIndex+1;
					callback.arrayVarName = arrayVarName;
				}
				else
					pNode->name = varDesc.varName;

				// add to the front of the list
				pNode->pNext = menuList;
				menuList = pNode;
				callback.bRecursion = true;
				callback.baseOffset = varDesc.offset + baseOffset;
			
				// load only one copy of array data
				if (arrayVarName)
				{
					callback.arrayIndex = __min(arraySize-1, commonMenu);
					callback.baseOffset += (varDesc.size * callback.arrayIndex);
					
					newParser->Enumerate(&callback);
					pNode->controlList = callback.pList;
					pNode->mObjClass = callback.mObjClass;
					pNode->pTabMenu = callback.menuList;
					return 0;		// do not parse the remaining members of array
				}
				else
				{
					newParser->Enumerate(&callback);
					pNode->controlList = callback.pList;
					pNode->mObjClass = callback.mObjClass;
					pNode->pTabMenu = callback.menuList;
				}
			}
			else // see if we need to create a submenu especially for the tab control
			if (pLastTab!=0 && varDesc.kind==VARIABLEDESC::RECORD && (varDesc.varName || arrayVarName))
			{
				createCallback callback(pButtonType, pBuildButtonType, pResearchButtonType,pMultiButtonType, pHotStaticType, pShipSilButtonType, pTabType, pIconType, pQType, pBaseData, commonMenu+1);
				Menu_context * pNode = new DAComponent<Menu_context>;

				if (varDesc.varName == 0)
				{
					pNode->name = arrayVarName;
					if (menuList && menuList->name==pNode->name)		// members of same array
						pNode->arrayIndex = callback.arrayIndex = menuList->arrayIndex+1;
					callback.arrayVarName = arrayVarName;
				}
				else
					pNode->name = varDesc.varName;

				// add to the end of the list
				pNode->pNext = 0;
				{
					Menu_context * tmp = menuList;
					if (tmp)
					{
						while (tmp->pNext)
							tmp = tmp->pNext;
						tmp->pNext = pNode;
					}
					else
						menuList = pNode;
				}
				callback.bRecursion = true;
				callback.baseOffset = varDesc.offset + baseOffset;
			
				newParser->Enumerate(&callback);
				pNode->controlList = callback.pList;
				pNode->mObjClass = callback.mObjClass;
				pNode->pTabControl = pLastTab;
				pNode->pTabMenu = callback.menuList;
			}

			return 1;
		}
	};

	//
	// interface map
	//

	BEGIN_DACOM_MAP_INBOUND(Menu_tb)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IToolbar)
	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	//
	void * pData;			// GT_TOOLBAR * type
	U32    dwDataSize;		// sizeof GT_TOOLBAR
	COMPTR<IHotButton> minimize, restore, exitSysMap;
	COMPTR<IDrawAgent> shape,topShape;

	U8 *map;
	U32 realWidth;
	U32 realHeight;
	RECT contextRect;
	RECT sysmapRect;
	RECT sectorMapRect;
	U32 toolbarID;
	M_RACE race;		// valid after loading the interface

	CONTROL_NODE * controlList;
	Menu_context * menuList;

	const char * vfxShapeType;
	const SHAPENAMESTYPE * vfxBarShapeType;
	const RECT * pContextRect;
	U16 topBarX,topBarY,topWidth,topHeight;

	COMPTR<IHotButton> DEBUG_selectedButton;
	
	//
	// instance methods
	//

	Menu_tb (void)
	{
		map = NULL;
		eventPriority = EVENT_PRIORITY_TOOLBAR;
	}

	~Menu_tb (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* IToolbar methods */

	virtual GENRESULT __stdcall GetControl (const char *buttonName, void ** ppControl);

	virtual GENRESULT __stdcall GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);

	virtual void SetToolbarID (U32 id)
	{
		toolbarID = id;
	}

	virtual U32 GetToolbarID (void)
	{
		return toolbarID;
	}

	virtual void GetSystemMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
		left = sysmapRect.left;
		top = sysmapRect.top;
		right = sysmapRect.right;
		bottom = sysmapRect.bottom;
	};

	virtual void GetSectorMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
		left = sectorMapRect.left;
		top = sectorMapRect.top;
		right = sectorMapRect.right;
		bottom = sectorMapRect.bottom;
	};

	/* BaseHotRect methods */

	virtual void DrawRect (void)
	{
		BaseHotRect::DrawRect();

		U32 color = RGB(255, 0, 0);
		//
		// draw the box
		//
		DA::LineDraw(0, screenRect.left+contextRect.left, screenRect.top+contextRect.top, screenRect.left+contextRect.right, screenRect.top+contextRect.top, color);
		DA::LineDraw(0, screenRect.left+contextRect.right, screenRect.top+contextRect.top, screenRect.left+contextRect.right, screenRect.top+contextRect.bottom, color);
		DA::LineDraw(0, screenRect.left+contextRect.right, screenRect.top+contextRect.bottom, screenRect.left+contextRect.left, screenRect.top+contextRect.bottom, color);
		DA::LineDraw(0, screenRect.left+contextRect.left, screenRect.top+contextRect.bottom, screenRect.left+contextRect.left, screenRect.top+contextRect.top, color);
	}

	/* Menu_tb methods */

	virtual void setStateInfo (void);

	void init (void);

	// we are within the shape bounds, are we within the artwork?
	void checkRect (S32 x, S32 y)
	{
		x -= screenRect.left;
		y -= screenRect.top;

		x = REAL2IDEALX(x);
		y = REAL2IDEALY(y);

		if(realWidth > 640) //are we in high res
		{
			x = IDEAL2HRIDEALX(x);
			y = IDEAL2HRIDEALY(y);
		}

		if (U32(x) < realWidth && U32(y) < realHeight)
		{
			bAlert = (map[(y*realWidth)+x] != 0xFF);		// assumes that 0xFF is transparent color
		}
	}

	U32 getBarHeight()
	{
		if(realWidth > 640) //are we in high res
		{
			return HRIDEAL2REALX(realHeight);
		}
		return IDEAL2REALX(realHeight);
	}

	void sendMouseUpdate (BaseHotRect * hotrect)
	{
		MSG msg;
		S32 x, y;

		WM->GetCursorPos(x, y);

		msg.hwnd = hMainWindow;
		msg.message = WM_MOUSEMOVE;
		msg.wParam = 0;
		msg.lParam = MAKELPARAM(x,y);
		hotrect->Notify(WM_MOUSEMOVE, &msg);
	}
	
	void toggleVisibleButtons (void)
	{
		if (shape!=0)	// is interface loaded?
		{
			if (bInvisible)
			{
				minimize->SetVisible(false);
				restore->SetVisible(CQFLAGS.bGameActive!=0 && CQFLAGS.bFullScreenMap == 0 && CQFLAGS.bMovieMode == 0);
				exitSysMap->SetVisible(CQFLAGS.bFullScreenMap!=0);
			}
			else
			{
				minimize->SetVisible(true);
				restore->SetVisible(false);
				exitSysMap->SetVisible(false);
			}
		}
		
		sendMouseUpdate(FULLSCREEN);
	}

	void flushControlList (void)
	{
		CONTROL_NODE * node;
		
		while ((node = controlList) != 0)
		{
			controlList = controlList->pNext;
			delete node;
		}
	}

	void flushMenuList (void)
	{
		Menu_context * node;
		
		BaseHotRect::Notify(CQE_LOAD_INTERFACE, 0);

		while ((node = menuList) != 0)
		{
			menuList = menuList->pNext;

			U32 count = node->GetBase()->Release();
			CQASSERT(count==0);
		}
	}

	void onLeftButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		switch (controlID)
		{
		case IDS_TT_HOMEWORLDRECT:
		case IDS_TT_FLEETRECT:
		case IDS_TT_LIGHTRECT:
		case IDS_TT_REPAIRRECT:
		case IDS_TT_NUCLEARRECT:
		case IDS_TT_HANDSRECT:
		case IDS_TT_TECHRECT:
		case IDS_TT_HEAVYRECT:
			DEBUG_toggleSelect(controlID);
			break;

		default:
			DEBUG_toggleSelect(0);		// deselect all
			break;
		}

		while (node)
		{
			next = node->pNext;
			node->client->OnLeftButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

	void onRightButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		while (node)
		{
			next = node->pNext;
			node->client->OnRightButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

	void onLeftDblButtonEvent (U32 controlID)
	{
		CONNECTION_NODE<IHotControlEvent> * node = point3.pClientList, *next;

		while (node)
		{
			next = node->pNext;
			node->client->OnLeftDblButtonEvent(toolbarID, controlID);
			node = next;
		}
	}

	GENRESULT __stdcall Notify (U32 message, void *param);

	void loadInterface (const enum M_RACE * pRace);
	void unloadInterface (void);

	void DEBUG_cycleContextMenus (void);
	void DEBUG_toggleSelect (U32 id);
	void DEBUG_unownPanel (void);

};
//----------------------------------------------------------------------------------//
//
Menu_tb::~Menu_tb (void)
{
	TOOLBAR = 0;
	flushControlList();
	flushMenuList();
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_tb::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, pData, dwDataSize, &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_tb::GetControl (const char *buttonName, void ** ppControl)
{
	GENRESULT result = GR_GENERIC;
	CONTROL_NODE * node = controlList;

	*ppControl = 0;

	while (node)
	{
		if (strcmp(buttonName, node->name) == 0)
		{
			result = node->getControl(ppControl);
			break;
		}
		node = node->pNext;
	}
			
	if (result != GR_OK)
	{
		//
		// else look in "common" area
		//
		COMPTR<IToolbar> common;
		if (GetToolbar("common", common, M_NO_RACE) == GR_OK)
		{
			result = common->GetControl(buttonName, ppControl);
		}
	}

	return result;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_tb::GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race)
{
	GENRESULT result = GR_GENERIC;
	Menu_context * node = menuList;
	int index;

	if ((index = race) != 0)
		index--;		// allow race==0 to mean "first menu"

	*ppMenu = 0;

	while (node)
	{
		// allow other menus to sub-out in case data is not present
		if (index>=node->arrayIndex && strcmp(menuName, node->name) == 0)
		{
			*ppMenu = node;
			node->GetBase()->AddRef();
			result = GR_OK;
			break;
		}
		node = node->pNext;
	}

	return result;
}
//----------------------------------------------------------------------------------//
//
void Menu_tb::SetVisible (bool bVisible)
{
	bool bOrigFocus = bHasFocus;

	if ((CQFLAGS.bNoToolbar = (!bVisible)) != 0)
	{
		Frame::Notify(CQE_KILL_FOCUS, 0);
		bHasFocus = bOrigFocus;
		bInvisible = true;
		CAMERA->SetInterfaceBarHeight(0);
		STATUS->SetToolbarHeight(0);
		HINTBOX->SetToolbarHeight(0);
		toggleVisibleButtons();

		EVENTSYS->Send(CQE_ADMIRALBAR_ACTIVE, (void*)0);
	}
	else
	{
		bInvisible = false;
		if (bHasFocus)
			Frame::Notify(CQE_SET_FOCUS, 0);

//		CAMERA->SetInterfaceBarHeight(screenRect.bottom-screenRect.top+1-getBarHeight());
		//for now render to the whole screen
		CAMERA->SetInterfaceBarHeight(0);
		STATUS->SetToolbarHeight(screenRect.bottom-screenRect.top+1);
		HINTBOX->SetToolbarHeight(screenRect.bottom-screenRect.top+1);
		toggleVisibleButtons();

		EVENTSYS->Send(CQE_ADMIRALBAR_ACTIVE, (void*)IDEAL2REALY(20));
	}
}
//----------------------------------------------------------------------------------//
//
bool Menu_tb::GetVisible (void)
{
	return !bInvisible;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_tb::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	GENRESULT result=GR_OK;

	if (message == CQE_LOAD_TOOLBAR)
	{
		if (param)
			loadInterface((const enum M_RACE *)param);
		else
			unloadInterface();
		return BaseHotRect::Notify(message, param);
	}
	else
	if (message == WM_CLOSE || message == CQE_FLUSHMESSAGES || message == CQE_DELETE_HOTRECT)
		return Frame::Notify(message, param);
	else
	if (CQFLAGS.bGameActive)
	{
		if (CQFLAGS.bFullScreenMap)			// SYSMAP wouldn't get notified otherwise
		{
			if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
				SYSMAP->Notify(message, param);
		}
		
		switch (message)
		{
		case CQE_PANEL_OWNED:
			if (param != 0)
				DEBUG_unownPanel();
			break;
		case CQE_GAME_ACTIVE:
			toggleVisibleButtons();
			break;
		//debug!!!
		case CQE_DEBUG_HOTKEY:
			if (bInvisible==false && CQFLAGS.bGameActive && bHasFocus && U32(param) == IDH_NEXT_CONTEXTMENU)
				DEBUG_cycleContextMenus();
			break;
		
		case WM_MOUSEMOVE:
			if ((result = Frame::Notify(message, param)) == GR_OK)
			{
				if (bAlert)
					checkRect(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
				if (bAlert)
				{
					desiredOwnedFlags = RF_CURSOR;
					grabAllResources();
				}
				else
				{
					desiredOwnedFlags = 0;
					releaseResources();
				}
			}
			return result;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			if ((result = Frame::Notify(message, param)) == GR_OK)
			{
				if (bAlert)
					result = GR_GENERIC;	// eat mouse press events
			}
			return result;

		case CQE_ENDFRAME:
			if (bInvisible==false)
			{
				BATCH->set_state(RPR_BATCH,FALSE);
				BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
				OrthoView(0);
				SetupDiffuseBlend(0,TRUE);
				EnableFastDrawAgent(true);
				if (shape!=0)
					shape->Draw(0, screenRect.left, screenRect.top);
				if (topShape!=0)
					topShape->Draw(0,topBarX,topBarY);
				result = Frame::Notify(message, param);
				EnableFastDrawAgent(false);
				return result;
			}
			return GR_OK;

		case CQE_MISSION_CLOSE:
			CQFLAGS.bMovieMode = 0;
			break;

		case CQE_MOVIE_MODE:
			if(param)
			{
				SetVisible(false);
				minimize->SetVisible(false);
				restore->SetVisible(false);
				exitSysMap->SetVisible(false);
			}
			else
				SetVisible(true);
			break;

		case CQE_HOTKEY:
			if (CQFLAGS.bGameActive && bHasFocus)
			{
				switch (U32(param))
				{
				case IDH_TOGGLE_TOOLBAR:
					if (CQFLAGS.bFullScreenMap==0)
						SetVisible(CQFLAGS.bNoToolbar != 0);	// toggle the current value
					return GR_OK;

/*				case IDH_FULLSCREEN_MAP:
					{
						static bool bOldVisible;

						if (CQFLAGS.bFullScreenMap)
						{
							// switching back to normal mode
							CQFLAGS.bFullScreenMap = 0;
							SetVisible(bOldVisible);
						}
						else
						{
							// switching to full-screen map
							CQFLAGS.bFullScreenMap = 1;
							bOldVisible = (CQFLAGS.bNoToolbar == 0);
							SetVisible(false);
							minimize->SetVisible(false);
							restore->SetVisible(false);
							exitSysMap->SetVisible(true);
							SYSMAP->BeginFullScreen();
						}
					}
					break;
*/				}
			}
			break;

		case CQE_LHOTBUTTON:
			onLeftButtonEvent((U32)param);
			return GR_GENERIC;
		case CQE_RHOTBUTTON:
			onRightButtonEvent((U32)param);
			return GR_GENERIC;
		case CQE_LDBLHOTBUTTON:
			onLeftDblButtonEvent((U32)param);
			return GR_GENERIC;
		}

		return Frame::Notify(message, param);
	}
	else // not bGameActive
		return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_tb::setStateInfo (void)
{
	COMPTR<IDAComponent> pComp;
	COMPTR<IShapeLoader> pLoader;
	CONTROL_NODE * node = 0;

	contextRect.left   = IDEAL2REALX(pContextRect->left);	
	contextRect.right  = IDEAL2REALX(pContextRect->right+1)-1;	
	contextRect.top    = IDEAL2REALY(pContextRect->top);	
	contextRect.bottom = IDEAL2REALY(pContextRect->bottom+1)-1;	

	if (vfxShapeType[0])
	{
		GENDATA->CreateInstance(vfxShapeType, pComp);
		pComp->QueryInterface("IShapeLoader", pLoader);
	}

	if (menuList)
	{
		Menu_context * menuNode = menuList;
		RECT rect = { screenRect.left + contextRect.left,
					  screenRect.top  + contextRect.top,
					  screenRect.left + contextRect.right,
					  screenRect.top  + contextRect.bottom };

		while (menuNode)
		{
			// check for common data
			if (strcmp(menuNode->name, "common")==0)
			{
				node = menuNode->controlList;
				
				resPriority++;		// temporarily increase res priority so common controls will have priority boost (over sysmap/sector map)

				while (node)
				{
					if (node->name && ((strcmp(node->name, "restore") == 0) || (strcmp(node->name, "exitSysMap") == 0)))
					{
						parent->resPriority += 12;//this 12 is so that the extitSysMap has a higher priority than the systemMap.
						node->initControl(pData, parent, pLoader, false);
						COMPTR<IEventCallback> event;
						node->pButton->QueryInterface("IEventCallback", event);
						parent->SetCallbackPriority(event, eventPriority+1);	// make sure restore and exitSysMap button is higher than us
						parent->resPriority -= 12;
					}
					else
						node->initControl(pData, this, pLoader, false);
					node = node->pNext;
				}

				resPriority--;

			}
			else  // it's a context menu
			{
				menuNode->screenRect = rect;
				node = menuNode->controlList;

				while (node)
				{
					node->initControl(pData, menuNode, pLoader, true);
					node = node->pNext;
				}

				// if there is tabMenu stuff, handle it here!!
				if (menuNode->pTabMenu != 0)
				{
					Menu_context * mNode = menuNode->pTabMenu;
					U32 count=0;

					while (mNode)
					{
						node = mNode->controlList;
						COMPTR<BaseHotRect> baseRect;

						mNode->pTabControl->GetTabMenu(count, baseRect);
						CQASSERT(baseRect!=0);

						while (node)
						{
							node->initControl(pData, baseRect, pLoader, true);
							node = node->pNext;
						}

						// if there is tabMenu stuff, handle it here!!
						if (mNode->pTabMenu != 0)
						{
							Menu_context * nNode = mNode->pTabMenu;
							U32 count=0;

							while (nNode)
							{
								node = nNode->controlList;
								COMPTR<BaseHotRect> baseRect;

								nNode->pTabControl->GetTabMenu(count, baseRect);
								CQASSERT(baseRect!=0);

								while (node)
								{
									node->initControl(pData, baseRect, pLoader, true);
									node = node->pNext;
								}

								nNode->pTabControl->SetCurrentTab(0);		// force control to initialize visibility of submenus

								nNode = nNode->pNext;
								count++;
							}
						}

						//
						// end new code
						//

						mNode->pTabControl->SetCurrentTab(0);		// force control to initialize visibility of submenus

						mNode = mNode->pNext;
						count++;
					}
				}

				if (menuNode->bInvisible)
					menuNode->SetVisible(false);	// make sure menu's controls know about invisibility
			}

			menuNode = menuNode->pNext;
		}
	}

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_tb::loadInterface (const enum M_RACE *pRace)
{
	char buffer[256];
	COMPTR<IViewConstructor2> parser;
	COMPTR<IDataParser> dataParser;
	COMPTR<IShapeLoader> loader;
	COMPTR<IShapeLoader> barLoader;
	COMPTR<IImageReader> reader;
	COMPTR<IDAComponent> pComp;
	DPARSERDESC ddesc;
	race = *pRace;
	
	if (race == M_NO_RACE)
		race = M_TERRAN;

	pData = GENDATA->GetArchetypeData("Toolbar!!Default", dwDataSize);		// also returns data size

	buffer[0] = '\\';
	strcpy(buffer+1, "GT_TOOLBAR");
	strcat(buffer, "\\");
	strcat(buffer, "Toolbar!!Default");

	PARSER->QueryInterface("IViewConstructor2", parser);
	CQASSERT(parser != 0);

	ddesc.symbol = parser->GetSymbol(0, "GT_TOOLBAR");
	CQASSERT(ddesc.symbol!=0);
	CQASSERT(parser->GetTypeSize(ddesc.symbol) == dwDataSize && "GenData out of sync");

	parser->CreateInstance(&ddesc, dataParser);
	CQASSERT(dataParser!=0);

	createCallback callback (GENDATA->LoadArchetype(HOTBUTTON_TYPE), 
							 GENDATA->LoadArchetype(BUILDBUTTON_TYPE), 
							 GENDATA->LoadArchetype(RESEARCHBUTTON_TYPE),
							 GENDATA->LoadArchetype(MULTIHOTBUTTON_TYPE),
							 GENDATA->LoadArchetype(HOTSTATIC_TYPE), 
							 GENDATA->LoadArchetype(SHIPSILBUTTON_TYPE),
							 GENDATA->LoadArchetype(TABCONTROLTYPE),
							 GENDATA->LoadArchetype(ICON_TYPE),
							 GENDATA->LoadArchetype(QUEUECONTROL_TYPE),
							 (char *) pData,
							 race);
	dataParser->Enumerate(&callback);
	CQASSERT(controlList == 0 && menuList == 0);
	controlList = callback.pList;
	menuList = callback.menuList;
	vfxShapeType = callback.vfxShapeType;
	vfxBarShapeType = callback.vfxBarShapeType;
	pContextRect = callback.pContextRect;
	sysmapRect = callback.sysmapRect;
	sectorMapRect = callback.sectorMapRect;
	topBarX = IDEAL2REALX(callback.topBarX);
	topBarY = IDEAL2REALY(callback.topBarY);

	CQASSERT(vfxShapeType);
	CQASSERT(pContextRect);

	GENDATA->CreateInstance(vfxShapeType, pComp);
	pComp->QueryInterface("IShapeLoader", loader);

	GENDATA->CreateInstance(vfxBarShapeType[race-1], pComp);
	pComp->QueryInterface("IShapeLoader", barLoader);

	if (barLoader==0 || barLoader->CreateImageReader(0, reader) != GR_OK)
		CQBOMB1("Failed to load interface art for race #%d (zero based)", callback.commonMenu);

	U16 width, height;
	GT_VFXSHAPE * data = (GT_VFXSHAPE *)(GENDATA->GetArchetypeData(vfxBarShapeType[race-1]));
	CreateDrawAgent(reader, shape,data->bHiRes);
	shape->GetDimensions(width, height);

	realWidth = reader->GetWidth();
	realHeight = reader->GetHeight();
	map = new U8[realHeight*realWidth];

	screenRect.left		= 0;
	screenRect.top		= SCREENRESY - height;
	screenRect.right	= screenRect.left + width - 1;
	screenRect.bottom	= screenRect.top + height - 1;

	RECT rect = { 0, 0, realWidth-1, realHeight-1};
	
	reader->GetImage(PF_COLOR_INDEX, map, &rect);
	
	if (barLoader==0 || barLoader->CreateImageReader(1, reader) != GR_OK)
		CQBOMB1("Failed to load interface art for race #%d (zero based)", callback.commonMenu);
	CreateDrawAgent(reader, topShape);
	
	topShape->GetDimensions(topWidth,topHeight);
	
	createViewer(buffer, "GT_TOOLBAR", IDS_VIEWTOOLBAR);

	COMPTR<IToolbar> common;
	GetToolbar("common", common, race);
	CQASSERT(common!=0);

	common->GetControl("restore", restore);
	CQASSERT(restore!=0);
	common->GetControl("exitSysMap", exitSysMap);
	CQASSERT(exitSysMap!=0);
	
	common->GetControl("minimize", minimize);
	CQASSERT(minimize!=0);

	SetVisible(CQFLAGS.bNoToolbar == 0);

	BaseHotRect::Notify(CQE_LOAD_INTERFACE, loader.ptr);
	loader.free();
	barLoader.free();
	GENDATA->FlushUnusedArchetypes();

	if(CQFLAGS.bMovieMode)
	{
		SetVisible(false);
		minimize->SetVisible(false);
		restore->SetVisible(false);
		exitSysMap->SetVisible(false);
	}
	else
		SetVisible(CQFLAGS.bNoToolbar==0);
}
//--------------------------------------------------------------------------//
//
void Menu_tb::unloadInterface (void)
{
	delete map;
	map = NULL;
	shape.free();
	topShape.free();
	restore.free();
	exitSysMap.free();
	minimize.free();
	flushControlList();
	flushMenuList();
	removeViewer();
}
//--------------------------------------------------------------------------//
//
void Menu_tb::DEBUG_cycleContextMenus (void)
{
	//
	// find the menu that is visible now
	//
	Menu_context * node = menuList;
	IBaseObject * obj = OBJLIST->GetSelectedList();

	while (node)
	{
		if (node->bInvisible==false)
			break;
		node = node->pNext;
	}

	if (obj && obj->nextSelected==0)		// only one item selected
	{
		if (node)
			node->SetVisible(false);

		MPart part = obj;
		if (part.isValid())
		{
			node = menuList;
			while (node)
			{
				if (node->mObjClass == part->mObjClass)
					break;
				node = node->pNext;
			}

			if (node)
				node->SetVisible(true);
		}
	}
	else
	{
		if (node)
		{
			node->SetVisible(false);
			node = node->pNext;
			
			if (node)
				node->SetVisible(true);
		}
		else
		{
			if ((node = menuList) != 0)
				node->SetVisible(true);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_tb::DEBUG_unownPanel (void)
{
	//
	// find the menu that is visible now
	//
	Menu_context * node = menuList;

	while (node)
	{
		if (node->bInvisible==false)
			break;
		node = node->pNext;
	}

	if (node)
		node->SetVisible(false);
}
//--------------------------------------------------------------------------//
//
void Menu_tb::DEBUG_toggleSelect (U32 id)
{
	if (DEBUG_selectedButton)
	{
		DEBUG_selectedButton->SetPushState(false);
		DEBUG_selectedButton.free();
	}
	//
	// find the button that was pressed
	//
	CONTROL_NODE * node = controlList;

	while (node)
	{
		if (node->pButton!=0 && node->pButton->GetControlID() == id)
		{
			DEBUG_selectedButton = node->pButton;
			DEBUG_selectedButton->SetPushState(true);
			break;
		}

		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_tb::init (void)
{
	parent = FULLSCREEN;
	lateInitialize(FULLSCREEN->GetBase());
	parent->SetCallbackPriority(this, eventPriority);
	initializeFrame(NULL);
	resPriority = RES_PRIORITY_TOOLBAR;
	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct FullScreen : public BaseHotRect
{
	//------------------------

	FullScreen (IDAComponent * _parent) : BaseHotRect(_parent)
	{
	}

	~FullScreen (void) { }

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	virtual void onSetFocus (bool bFocus)
	{
		if (bFocus == false)
		{
			bAlert = 0;
			bHasFocus = 0;
			desiredOwnedFlags = 0;
			releaseResources();
		}
		else
		{
			S32 x, y;
			MSG msg;

			bHasFocus = 1;
			WM->GetCursorPos(x, y);

			msg.hwnd = hMainWindow;
			msg.message = WM_MOUSEMOVE;
			msg.wParam = 0;
			msg.lParam = MAKELPARAM(x,y);
			Notify(WM_MOUSEMOVE, &msg);
		}
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT FullScreen::Notify (U32 message, void *param)
{
	if (message == CQE_INTERFACE_RES_CHANGE)
	{
		screenRect.right = SCREENRESX;
		screenRect.bottom = SCREENRESY;
	}

	BaseHotRect::Notify(message, param);
	return GR_OK;
}

//--------------------------------------------------------------------------//
//
struct _panels : GlobalComponent
{
	FullScreen * fullScreen;
	Menu_tb * menu;

	virtual void Startup (void)
	{
		TOOLBAR  = menu = new DAComponent<Menu_tb>; 
		AddToGlobalCleanupList(&TOOLBAR);

//		CreateAdmiralToolbar();

		FULLSCREEN = fullScreen = new FullScreen(0);
		AddToGlobalCleanupList(&FULLSCREEN);
	}

	virtual void Initialize (void)
	{
		fullScreen->lateInitialize(GS);
		fullScreen->screenRect.left = 0;
		fullScreen->screenRect.top = 0;
		fullScreen->screenRect.right = SCREENRESX;
		fullScreen->screenRect.bottom = SCREENRESY;
		menu->init();
//		InitAdmiralToolbar();
	}
};
//--------------------------------------------------------------------------//
//
static _panels panels;

//--------------------------------------------------------------------------//
//-----------------------------End Menu_Pause.cpp---------------------------//
//--------------------------------------------------------------------------//
