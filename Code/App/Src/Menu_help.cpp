//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_help.cpp                           //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_help.cpp 13    6/26/01 10:35a Tmauer $
*/
//--------------------------------------------------------------------------//
// New player dialog
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include <DMenu1.h>

#include "Frame.h"
#include "IStatic.h"
#include "IButton2.h"
#include "MusicManager.h"

#define MENU1_MUSIC		"Main_menu_screen.wav"

U32 __stdcall DoMenu_Credits (Frame * parent);

//--------------------------------------------------------------------------//
//
struct Menu_help : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::HELPMENU & data;
	const GT_MENU1 & menu1;

	COMPTR<IStatic> background, title;
	COMPTR<IStatic> staticConquest, staticVersion, staticNumber;
	COMPTR<IButton2> buttonOk;

	COMPTR<IStatic> staticProductID, staticProductNumber;
	COMPTR<IStatic> staticLegal;
	COMPTR<IButton2> buttonCredits;

	//
	// instance methods
	//
	// 
	Menu_help (Frame * _parent, const GT_MENU1 & _data) : data(_data.helpMenu), menu1(_data)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_help (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}


	/* IEventCallback methods */
	
	virtual GENRESULT __stdcall	Notify (U32 message, void *param);



	/* Menu_help methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_OK:
			endDialog(1);
			break;

		case IDS_CREDITS:
			DoMenu_Credits(this);
			break;

		default:
			break;
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);
};
//----------------------------------------------------------------------------------//
//
Menu_help::~Menu_help (void)
{
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Menu_help::Notify (U32 message, void *param)
{
	if (message == CQE_SET_FOCUS)
	{
		if (CQFLAGS.b3DEnabled == 0)
		{
			MUSICMANAGER->PlayMusic(MENU1_MUSIC);
		}
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_help::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//
	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);
	staticConquest->InitStatic(data.staticConquest, this);
	staticVersion->InitStatic(data.staticVersion, this);
	staticNumber->InitStatic(data.staticNumber, this);
	buttonOk->InitButton(data.buttonOk, this);
	buttonCredits->InitButton(data.buttonCredits, this);

	staticProductID->InitStatic(data.staticProductID, this);
	staticProductNumber->InitStatic(data.staticProductNumber, this);

	staticProductID->SetVisible(false);
	staticProductNumber->SetVisible(false);

	staticLegal->InitStatic(data.staticLegal, this);

	wchar_t szWide[128];
	char    szAnsi[128];
	GetProductVersion(szAnsi, sizeof(szAnsi));

	_localAnsiToWide(szAnsi, szWide, sizeof(szWide));
	staticNumber->SetText(szWide);

	// get the product ID
	if (DEFAULTS->GetInstallStringFromRegistry("PID", szAnsi, sizeof(szAnsi)))
	{
		_localAnsiToWide(szAnsi, szWide, sizeof(szWide));
		staticProductNumber->SetText(szWide);
	}
	else
	{
		staticProductNumber->SetText(L"No Product Number");
	}


	setFocus(buttonOk);
}
//--------------------------------------------------------------------------//
//
void Menu_help::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	GENDATA->CreateInstance(data.staticConquest.staticType, pComp);
	pComp->QueryInterface("IStatic", staticConquest);

	GENDATA->CreateInstance(data.staticVersion.staticType, pComp);
	pComp->QueryInterface("IStatic", staticVersion);

	GENDATA->CreateInstance(data.staticNumber.staticType, pComp);
	pComp->QueryInterface("IStatic", staticNumber);

	GENDATA->CreateInstance(data.buttonOk.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonOk);

	GENDATA->CreateInstance(data.staticProductID.staticType, pComp);
	pComp->QueryInterface("IStatic", staticProductID);

	GENDATA->CreateInstance(data.staticProductNumber.staticType, pComp);
	pComp->QueryInterface("IStatic", staticProductNumber);

	GENDATA->CreateInstance(data.staticLegal.staticType, pComp);
	pComp->QueryInterface("IStatic", staticLegal);

	GENDATA->CreateInstance(data.buttonCredits.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonCredits);

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_help (Frame * parent, const GT_MENU1 & data)
{
	static U8 recurse;
	CQASSERT(recurse==0);

	recurse++;

	Menu_help * dlg = new Menu_help(parent, data);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	recurse--;
	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_help.cpp----------------------------//
//--------------------------------------------------------------------------//