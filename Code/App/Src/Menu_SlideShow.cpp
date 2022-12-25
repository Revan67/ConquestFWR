//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_SlideShow.cpp                           //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_SlideShow.cpp 2     4/09/01 4:19p Tmauer $
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
#include "IShapeLoader.h"
#include "DrawAgent.h"
#include "MusicManager.h"
#include "MScript.h" // for SPLASHINFO

U32 __stdcall DoMenu_SlideShow (const char * vfxName,SINGLE speed, bool bAllowExit);

//--------------------------------------------------------------------------//
//
struct Menu_SlideShow : public DAComponent<Frame>
{
	//
	// data items
	//

	SINGLE frameSpeed;
	SINGLE timer;
	bool bAllowExit;
	bool bUseSpacebar;

	U32 currentSlide;
	COMPTR<IDrawAgent> slide;
	COMPTR<IShapeLoader> slideLoader;

	//
	// instance methods
	//
	// 
	Menu_SlideShow (IShapeLoader * loader, SINGLE speed, bool _bAllowExit)
	{
		bUseSpacebar = false;
		bAllowExit = _bAllowExit;
		slideLoader = loader; 
		frameSpeed = speed;
		initializeFrame(NULL);
		init();
	}

	~Menu_SlideShow (void);

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


	/* Menu_SlideShow methods */

	virtual void setStateInfo (void);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		if(bAllowExit)
			loadShape(currentSlide+1);
		return true;
	}

	void init (void);

	void loadShape(U32 newShape);

	void drawShape();
};
//----------------------------------------------------------------------------------//
//
Menu_SlideShow::~Menu_SlideShow (void)
{
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Menu_SlideShow::Notify (U32 message, void *param)
{
	if(message == CQE_UPDATE)
	{
		timer += (U32(param) >> 10)/1000.0f;
		if(timer > frameSpeed)
			loadShape(currentSlide+1);
	}
	else if(message == CQE_ENDFRAME)
	{
		drawShape();
	}
	else if(message == WM_CHAR)
	{
		if (bHasFocus && !bInvisible && bUseSpacebar)
		{
			MSG* msg = (MSG*)param;
			if( TCHAR(msg->wParam) == ' ' ) // spacebar
			{
				endDialog(0);
				return GR_OK;
			}
		}
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_SlideShow::setStateInfo (void)
{
	loadShape(0);
}
//--------------------------------------------------------------------------//
//
void Menu_SlideShow::init (void)
{
	setStateInfo();
}
//--------------------------------------------------------------------------//
//

void Menu_SlideShow::loadShape(U32 newShape)
{
	slide.free();
	if(slideLoader->CreateDrawAgent(newShape,slide) == GR_OK)
	{
		timer = 0;
		currentSlide = newShape;
	}
	else
	{
		endDialog(1);
	}
}

//--------------------------------------------------------------------------//
//

void Menu_SlideShow::drawShape()
{
	if(slide)
	{
		slide->Draw(0, 0, 0);
	}
}

//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_SlideShow (const char * vfxName,SINGLE speed, bool bAllowExit)
{
	static U8 recurse;
	CQASSERT(recurse==0);

	recurse++;

	COMPTR<IShapeLoader> loader;

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(vfxName, pComp);
	pComp->QueryInterface("IShapeLoader", loader);

	Menu_SlideShow * dlg = new Menu_SlideShow(loader,speed, bAllowExit);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;
	loader.free();

	recurse--;
	return result;
}

//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_Splash(struct SPLASHINFO& _splashInfo)
{
	static U8 recurse;
	CQASSERT(recurse==0);
	recurse++;

	COMPTR<IShapeLoader> loader;
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(_splashInfo.vfxName, pComp);
	pComp->QueryInterface("IShapeLoader", loader);

	Menu_SlideShow * dlg = new Menu_SlideShow(loader,_splashInfo.speed,_splashInfo.bAllowExit);
	dlg->bUseSpacebar = true;
	dlg->bAllowExit = false;
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;
	loader.free();

	// make sure NOT to confirm exit, as we really do want to
	CQFLAGS.bNoExitConfirm = true;

	// fools the app to end instantly, instead of show the Menu1 screen (could be a better way to do this...)
	CQFLAGS.bDPLobby = true;

	// clean up the game resources and quit
	::PostMessage(hMainWindow,WM_CLOSE,0,0);

	recurse--;
	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_help.cpp----------------------------//
//--------------------------------------------------------------------------//