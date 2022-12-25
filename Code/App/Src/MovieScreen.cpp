//--------------------------------------------------------------------------//
//                                                                          //
//                             MovieScreen.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/MovieScreen.cpp 25    9/17/00 6:47p Jasony $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "Mission.h"
#include "SoundManager.h"

#include <DMovieScreen.h>

#include <VideoSystem.h>

//--------------------------------------------------------------------------//
//
struct MenuMovie : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_MOVIESCREEN data;

	VIDEO_HANDLE video;
	const char * filename;
	bool bPlaying;

	U32 defaultNoFrameLimiting;

	//
	// instance methods
	//

	MenuMovie (Frame * _parent, char * szFileName) : filename(szFileName)
	{
		initializeFrame(_parent);
		init();
		defaultNoFrameLimiting = DEFAULTS->GetDefaults()->bNoFrameLimit;
		video = VIDEOSYS->LoadVideoMovie(filename,false);
		if(video)
		{
			VIDEOSYS->StartVideoMovie(video);
		}
	}

	~MenuMovie (void);

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

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);


	/* MenuMovie methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}


	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endMovie();
		return true;
	}

	void endMovie (void)
	{
		if (video)
		{
			VIDEOSYS->ReleaseVideoMovie(video);
			video = 0;
			endDialog(0);
		}
	}

	void render (void);

	void init (void);

	void initLobby (void);
};
//----------------------------------------------------------------------------------//
//
MenuMovie::~MenuMovie (void)
{
	if (video)
	{
		VIDEOSYS->ReleaseVideoMovie(video);
		video = 0;
	}
	DEFAULTS->GetDefaults()->bNoFrameLimit = defaultNoFrameLimiting;
	CQFLAGS.bFullScreenMovie = false;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuMovie::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuMovie::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_ENDFRAME:
		render();
		break;

	case CQE_UPDATE:
		if (video)
		{
			if (VIDEOSYS->IsPlaying(video) == false)
			{
				endMovie();
			}
			else
			{
				VIDEOSYS->UpdateVideoFrame(video);
			}
		}
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void MenuMovie::render (void)
{
	//clear the screen; //might only want to do this once?
	DA::RectangleFill(NULL, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom, 0);

	if(video)
	{
		U32 textureHandle = VIDEOSYS->GetVideoTexture(video);
		RECT drawRect;
		SINGLE u = 0;
		SINGLE v = 0;
		VIDEOSYS->GetVideoTexureCoords(video,u,v);

		SINGLE width = VIDEOSYS->GetVideoWidth(video);
		SINGLE height = VIDEOSYS->GetVideoHeight(video);

		SINGLE aspect = width/height;

		//coding for width is always largest

		SINGLE vidHeight = ((SCREENRESX-1)/aspect);
		if(vidHeight < SCREENRESY)
		{
			drawRect.left = 0;
			drawRect.right = SCREENRESX-1;
			drawRect.top = ((SCREENRESY-1)-vidHeight)/2;
			drawRect.bottom = drawRect.top+vidHeight;
		}
		else
		{
			SINGLE vidWidth = ((SCREENRESY-1)*aspect);
			drawRect.top = 0;
			drawRect.bottom = SCREENRESX-1;
			drawRect.left = ((SCREENRESX-1)-vidWidth)/2;
			drawRect.right = drawRect.left+vidWidth;
		}

		DA::DrawTexture(NULL,drawRect.left,drawRect.top,drawRect.right,drawRect.bottom,textureHandle,0,0,u,v);
	}

	DEFAULTS->GetDefaults()->bNoFrameLimit = true;
	CQFLAGS.bFullScreenMovie = true;
}
//----------------------------------------------------------------------------------//
//
void MenuMovie::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void MenuMovie::init (void)
{
	data = 	*((GT_MOVIESCREEN *) GENDATA->GetArchetypeData("MovieScreen"));
}
//--------------------------------------------------------------------------//
//
U32 __stdcall MovieScreen (Frame * parent, const char * filename)
{
	if (CQFLAGS.bSkipMovies)
		return 0;
	else
	{
		GENDATA->FlushUnusedArchetypes();
		MenuMovie * menu = new MenuMovie(parent, const_cast<char*>(filename));

		menu->createViewer("\\GT_MOVIESCREEN\\MovieScreen", "GT_MOVIESCREEN", IDS_VIEWMOVIE);
		menu->beginModalFocus();

		S32 level;
		bool bMuted;
		bool mute = true;

		SOUNDMANAGER->GetMusicVolumeLevel(level, bMuted);
		SOUNDMANAGER->SetMusicVolumeLevel(level, mute);
		CURSOR->EnableCursor(false);

		U32 result = CQDoModal(menu);
		delete menu;

		CURSOR->EnableCursor(true);
		SOUNDMANAGER->SetMusicVolumeLevel(level, bMuted);

		return result;
	}
}



