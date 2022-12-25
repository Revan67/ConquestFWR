//--------------------------------------------------------------------------//
//                                                                          //
//                              VideoSystem.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 2003 Warthog Texas, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Libs/Src/VideoSystem/VideoSystem.cpp 6     10/29/03 10:05a Tmauer $

   Audio player
*/
//--------------------------------------------------------------------------//

#include <windows.h>

#include "TComponent.h"
#include "TConnContainer.h"
#include "TConnPoint.h"
#include "System.h"
#include "da_heap_utility.h"
#include "EventSys.h"
#include "FDump.h"
#include "VideoSystem.h"
#include "FileSys.h"
#include <RendPipeline.h>

#include <bink\bink.h>

#include <stdio.h>
#include <d3d9.h>


//--------------------------------------------------------------------------//
//---------------------------Local structures-------------------------------//
//--------------------------------------------------------------------------//

static char interface_name[] = "IVideoSystem";

static IRenderPipeline * PIPE = NULL;

struct VideoNode
{
	VideoNode* next;

	HBINK binkMovie;
	U32 textureHandle;
	bool bLooping;

	enum VideoState
	{
		VS_STOPED,
		VS_PLAYING,
	}videoState;

	VideoNode()
	{
		next = NULL;
		textureHandle = NULL;
		binkMovie = 0;
		videoState = VS_STOPED;
	}
};

struct DACOM_NO_VTABLE VideoSystem : public ISystemComponent, 
											IVideoSystem
{
	BEGIN_DACOM_MAP_INBOUND(VideoSystem)
	DACOM_INTERFACE_ENTRY(IVideoSystem)
	DACOM_INTERFACE_ENTRY(ISystemComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IVideoSystem,IVideoSystem)
	DACOM_INTERFACE_ENTRY2(IID_ISystemComponent,ISystemComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	GENRESULT init (AGGDESC * info);

	DA_HEAP_DEFINE_NEW_OPERATOR(VideoSystem)

	IFileSystem * movieDir;

	//bink interface elements
	VideoNode* vNodeList;

	VideoSystem (void)
	{
		movieDir = NULL;
		vNodeList = NULL;
	}

	~VideoSystem(void);

	//IVideoSystem

	virtual GENRESULT Initialize (IFileSystem * movieDir, struct IRenderPipeline * rend);

	virtual void Shutdown();

	virtual VIDEO_HANDLE LoadVideoMovie(const char * movieFile, bool bLooping);

	virtual void ReleaseVideoMovie(VIDEO_HANDLE handle);

	virtual void StartVideoMovie(VIDEO_HANDLE handle);

	virtual void StopVideoMovie(VIDEO_HANDLE handle);

	virtual bool IsPlaying(VIDEO_HANDLE handle);

	virtual U32 GetVideoTexture(VIDEO_HANDLE vHandle);

	virtual void GetVideoTexureCoords(VIDEO_HANDLE vHandle, SINGLE & u, SINGLE & v);

	virtual void SetVideoFrame(VIDEO_HANDLE vHandle, U32 frameNum);

	virtual void UpdateVideoFrame(VIDEO_HANDLE vHandle);

	virtual U32 GetNumFrames(VIDEO_HANDLE vHandle);

	virtual U32 GetVideoWidth(VIDEO_HANDLE vHandle);

	virtual U32 GetVideoHeight(VIDEO_HANDLE vHandle);

	/* IAggregateComponent members */

    DEFMETHOD(Initialize) (void);
	
	/* ISystemComponent members */

    DEFMETHOD_(void,Update) (void);

	/* SoundSystem members */

	IDAComponent * GetBase(void)
	{
		return (ISystemComponent *) this;
	}

	//VideoSystem
};

//--------------------------------------------------------------------------//
//
GENRESULT VideoSystem::init (AGGDESC * info)
{
	if (info->description && strcmp(info->description, "VideoSystem"))
		return GR_INTERFACE_UNSUPPORTED;
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
VideoSystem::~VideoSystem (void)
{
	Shutdown();
}
//--------------------------------------------------------------------------//
//
GENRESULT VideoSystem::Initialize (IFileSystem * _movieDir, struct IRenderPipeline * rend)
{
	PIPE = rend;
	movieDir = _movieDir;
	//Initialize Bink
	BinkSoundUseDirectSound(0);

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void VideoSystem::Shutdown()
{
	while(vNodeList)
	{
		VideoNode * node = vNodeList;
		vNodeList = node->next;
		if(node->textureHandle)
		{
			PIPE->destroy_texture(node->textureHandle);
			node->textureHandle = NULL;
		}
		if(node->binkMovie)
		{
			BinkClose( node->binkMovie );
			node->binkMovie = 0;
		}
		delete node;
	}
}
//--------------------------------------------------------------------------//
//
VIDEO_HANDLE VideoSystem::LoadVideoMovie(const char * movieFile, bool bLooping)
{
	VIDEO_HANDLE ret = 0;

	if(strstr(movieFile,".bik"))//if this is a bink movie
	{
		VideoNode* newNode = new VideoNode();
		newNode->bLooping = bLooping;

		char dirFilename[MAX_PATH];
		movieDir->GetFileName(dirFilename,MAX_PATH);
		strcat(dirFilename,"\\");
		strcat(dirFilename,movieFile);

		newNode->binkMovie = BinkOpen(dirFilename,BINKALPHA);

		if(newNode->binkMovie)
		{
			PixelFormat desiredFormat(32, 8, 8, 8, 8);

			PIPE->create_texture(newNode->binkMovie->Width,newNode->binkMovie->Height, desiredFormat,1,0,newNode->textureHandle);

			ret = (VIDEO_HANDLE)(newNode);

			newNode->next = vNodeList;
			vNodeList = newNode;
		}
	}

	return ret;
}
//--------------------------------------------------------------------------//
//
void VideoSystem::ReleaseVideoMovie(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		VideoNode * prev = NULL;
		VideoNode * search = vNodeList;
		while(search)
		{
			if(search == node)
			{
				BinkClose( node->binkMovie );
				node->binkMovie = 0;
				if(node->textureHandle)
				{
					PIPE->destroy_texture(node->textureHandle);
					node->textureHandle = NULL;
				}

				if(prev)
					prev->next = node->next;
				else
					vNodeList = node->next;
				delete node;
				return;
			}
			prev = search;
			search = search->next;
		}
	}
}
//--------------------------------------------------------------------------//
//
void VideoSystem::StartVideoMovie(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		node->videoState = VideoNode::VS_PLAYING;
	}
}
//--------------------------------------------------------------------------//
//
void VideoSystem::StopVideoMovie(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		node->videoState = VideoNode::VS_STOPED;
	}
}
//--------------------------------------------------------------------------//
//
bool VideoSystem::IsPlaying(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		if(node->videoState == VideoNode::VS_PLAYING)
			return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
U32 VideoSystem::GetVideoTexture(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		return node->textureHandle;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
void VideoSystem::GetVideoTexureCoords(VIDEO_HANDLE vHandle, SINGLE & u, SINGLE & v)
{
	u = 1.0;//for now, if the textures come back at powers of 2 we will need to alter this
	v = 1.0;
}
//--------------------------------------------------------------------------//
//
void VideoSystem::SetVideoFrame(VIDEO_HANDLE handle, U32 frameNum)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		BinkGoto(node->binkMovie,frameNum,0);
	}
}
//--------------------------------------------------------------------------//
//
void VideoSystem::UpdateVideoFrame(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node && node->videoState == VideoNode::VS_PLAYING)
	{
		if ( !BinkWait( node->binkMovie ) )
		{
			//
			// Decompress the Bink frame.
			//

			BinkDoFrame( node->binkMovie );

			LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)(node->textureHandle);
			D3DLOCKED_RECT lockedRect;
			IDirect3DSurface9 * d3dSurface = NULL;
			texture->GetSurfaceLevel(0,&d3dSurface);
			if(d3dSurface)
			{
				S32 surface_type = BinkDX9SurfaceType(d3dSurface);
				d3dSurface->LockRect(&lockedRect,NULL,0);
				BinkCopyToBuffer( node->binkMovie, lockedRect.pBits,	lockedRect.Pitch, node->binkMovie->Height,
									0, 0, surface_type);

				d3dSurface->UnlockRect();
				d3dSurface->Release();
				d3dSurface = NULL;


				if(node->bLooping)
				{
					if(node->binkMovie->FrameNum == node->binkMovie->Frames)
					{
						BinkGoto(node->binkMovie,1,0);
					}
				}
				else
				{
					if ( node->binkMovie->FrameNum != node->binkMovie->Frames )
					{
						BinkNextFrame( node->binkMovie );
					}
					else
					{
						node->videoState = VideoNode::VS_STOPED;
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
U32 VideoSystem::GetNumFrames(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		return node->binkMovie->Frames;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
U32 VideoSystem::GetVideoWidth(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		return node->binkMovie->Width;
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 VideoSystem::GetVideoHeight(VIDEO_HANDLE handle)
{
	VideoNode * node = (VideoNode*)handle;
	if(node)
	{
		return node->binkMovie->Height;
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
GENRESULT VideoSystem::Initialize (void)
{
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void VideoSystem::Update (void)
{
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void RegisterTheVideoSystem (ICOManager *DACOM)
{
	IComponentFactory *pServer = new DAComponentFactory2<DAComponentAggregate<VideoSystem>, AGGDESC> (interface_name);

	if (pServer)
	{
		DACOM->RegisterComponent(pServer, interface_name);
		pServer->Release();
	}
}


//--------------------------------------------------------------------------//
//---------------------------------SoundSystem.cpp-----------------------------//
//--------------------------------------------------------------------------//
