#ifndef VIDEOSYSTEM_H
#define VIDEOSYSTEM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               VideoSystem.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2003 Warthog Texas                        //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/INCLUDE/VideoSystem.h 3     10/07/03 12:49p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#define IID_IVideoSystem MAKE_IID("IVideoSystem",1)

typedef unsigned int VIDEO_HANDLE;

struct DACOM_NO_VTABLE IVideoSystem : public IDAComponent
{
	virtual GENRESULT Initialize (struct IFileSystem * movieDir, struct IRenderPipeline * rend) = 0;

	virtual void Shutdown() = 0;

	virtual VIDEO_HANDLE LoadVideoMovie(const char * movieFile, bool bLooping) = 0;

	virtual void ReleaseVideoMovie(VIDEO_HANDLE handle) = 0;

	virtual void StartVideoMovie(VIDEO_HANDLE handle) = 0;

	virtual void StopVideoMovie(VIDEO_HANDLE handle) = 0;

	virtual bool IsPlaying(VIDEO_HANDLE handle) = 0;

	virtual U32 GetVideoTexture(VIDEO_HANDLE vHandle) = 0;

	virtual void GetVideoTexureCoords(VIDEO_HANDLE vHandle, SINGLE & u, SINGLE & v) = 0;

	virtual void SetVideoFrame(VIDEO_HANDLE vHandle, U32 frameNum) = 0;

	virtual void UpdateVideoFrame(VIDEO_HANDLE vHandle) = 0;

	virtual U32 GetNumFrames(VIDEO_HANDLE vHandle) = 0;

	virtual U32 GetVideoWidth(VIDEO_HANDLE vHandle) = 0;

	virtual U32 GetVideoHeight(VIDEO_HANDLE vHandle) = 0;
};

#endif 