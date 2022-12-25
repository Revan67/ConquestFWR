// BigImageTest.cpp
//
// Tests various sizes and rects for bigimage
//


#include	<windows.h>
#include	<conio.h>
#include	"DarthTest.h"
#include	"DACOM.h"
#include	"System.h"
#include	"bigimage.h"


static	HANDLE		s_Module		=0;
static	U32			s_Windowed		=0;
static	HANDLE		hTestBmp		=0;

static	char		ImageFileNames[9][64]=
{
	"data\\biggrid300x200.bmp",
	"data\\biggrid200x300.bmp",
	"data\\biggrid300x300.bmp",
	"data\\biggrid200x200.bmp",
	"data\\biggrid1100x200.bmp",
	"data\\biggrid200x1100.bmp",
	"data\\biggrid.bmp",
	"data\\biggrid200x200.bmp",
	"data\\pia162.bmp"
};

static	RECT	SourceRects[9]	=
{
	{0, 0, 300, 200},
	{0, 0, 200, 300},
	{0, 0, 300, 300},
	{0, 0, 200, 200},
	{0, 0, 1100, 200},
	{0, 0, 200, 1100},
	{0, 0, 2048, 2048},
	{0, 0, 200, 200},
	{0, 0, 639, 479}
};

static	RECT	DestRects[9]	=
{
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 639, 479},
	{0, 0, 319, 479}
};

static	long FAR PASCAL	PipeViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return	DefWindowProc(hWnd, message, wParam, lParam);
}

DARTH_DEFINE_NAME(			"BigImageTest" )
DARTH_DEFINE_CONTACT(		"kbaird" )
DARTH_DEFINE_CATEGORY(		"BigImage" )
DARTH_DEFINE_DESCRIPTION(	"Will run a series of rect tests on" 
							" various sized images.  Ensure the grid"
							" stays proportional with no distorted areas"
							" for the test to pass." );

DARTH_DEFINE_DLLMAIN(		s_Module )

__declspec(dllexport)	HRESULT	RunTest(IDarthRuntime *RunTime)
{
	ICOManager			*s_DACOM		=NULL;
	ISystemContainer	*s_SYSTEM		=NULL;
	IRenderPipeline		*s_PIPE			=NULL;
	IBigImage			*s_BIGIMAGE		=NULL;
	TCHAR				ini_file[MAX_PATH];
	WNDCLASS			wc;
	HWND				hRender;
	int					i;

	//grab the ini file to use	
	RunTime->GetIniFilename(ini_file, MAX_PATH);

	//init com stuff
	DARTH_REQUIRE_DACOM(RunTime, s_DACOM, ini_file, return DARTH_TEST_FAIL);
	DARTH_REQUIRE_SYSTEM(RunTime, s_DACOM, s_SYSTEM, return DARTH_TEST_FAIL);
	DARTH_REQUIRE_INTERFACE(RunTime, s_SYSTEM, IID_IRenderPipeline, s_PIPE, return DARTH_TEST_FAIL);

	//init the pipe
	if(FAILED(s_PIPE->startup()))
	{
		RunTime->Log(IDR_LC_TEST, 0, "s_PIPE->startup() FAILED");
		return	E_FAIL;
	}

	//check if windowed is ok	
	s_PIPE->query_device_ability(RP_A_DEVICE_WINDOWED, &s_Windowed, NULL);
	
	if(!s_Windowed)
	{
		s_PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, TRUE);
	}

	//reg a window class
	wc.style			=CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		=PipeViewWndProc;
	wc.cbClsExtra		=0;
	wc.cbWndExtra		=0;
	wc.hInstance		=(HINSTANCE)s_Module;
	wc.hIcon			=NULL;
	wc.hCursor			=LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	=NULL;
	wc.lpszMenuName		=NULL;
	wc.lpszClassName	="PipeView";

	RegisterClass(&wc);

	//create a simple 640 window for render output
	hRender	=CreateWindowEx(NULL, "PipeView", "Render Test Window", WS_POPUP, 0, 0, 640, 480, NULL, NULL, (HINSTANCE)s_Module, NULL);

	ShowWindow(hRender, SW_SHOWNORMAL);

	if(FAILED(s_PIPE->create_buffers(hRender, 640, 480)))
	{
		RunTime->Log(IDR_LC_TEST, 0, _T("create_buffers(hRender, 640, 480 ) failed"));
		return	E_FAIL;
	}
	
	s_PIPE->set_viewport(0, 0, 639, 479);
	s_PIPE->set_ortho(0, 639, 479, 0);
	s_PIPE->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);

	for(i=0;i < 8;i++)
	{
		DIBSECTION	dib;
		int			w, h, bpp, stride;

		if((hTestBmp	=LoadImage(NULL, ImageFileNames[i], IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)) == NULL)
		{
			RunTime->Log(IDR_LC_TEST, 0, _T("Unable to find image file: %s"), ImageFileNames[i]);
		}

		int	gotit	=GetObject((HGDIOBJ) hTestBmp, sizeof(dib), &dib);

		if(!gotit)
		{
			return	E_FAIL;
		}

		w		=dib.dsBm.bmWidth;
		h		=dib.dsBm.bmHeight;
		bpp		=dib.dsBm.bmBitsPixel;
		stride	=dib.dsBm.bmWidthBytes;

		U8	*pBits	=(U8 *)malloc(h * stride);
		
		gotit		=GetBitmapBits((HBITMAP)hTestBmp, h * stride, pBits);

		PixelFormat	dpf(PF_4CC_DAOT);
		PixelFormat	spf(bpp, 5, 6, 5, 0);

		BIGIMAGEDESC	bidesc(s_PIPE, &dpf, pBits, &spf, w, h, stride);
		if(FAILED(s_DACOM->CreateInstance(&bidesc, (void **)&s_BIGIMAGE)))
		{
			RunTime->Log(IDR_LC_TEST, 0, "s_DACOM->CreateInstance(BigImage) FAILED");
			return	DARTH_TEST_FAIL;
		}

		free(pBits);

		RECT	srect	=SourceRects[i];
		RECT	drect	=DestRects[i];

		int	dx,dy,sx,sy;

		//test all dest rect sizes
		for(dx=1;dx < drect.right;dx+=128)
		{
			for(dy=1;dy < drect.bottom;dy+=128)
			{
				//test all source sizes
				for(sx=1;sx < srect.right;sx+=(srect.right < 301)? 128 : 512)
				{
					for(sy=1;sy < srect.bottom;sy+=(srect.bottom < 301)? 128 : 512)
					{
						s_PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);
						s_PIPE->begin_scene();

						RECT	sr	=srect;
						sr.right	-=sx;
						sr.bottom	-=sy;

						RECT	dr;

						if(dy & 128)	//alternate reversed rects
						{
							dr.right	=drect.left;
							dr.left		=drect.right - dx;
							dr.bottom	=drect.top;
							dr.top		=drect.bottom - dy;
						}
						else
						{
							dr			=drect;
							dr.right	-=dx;
							dr.bottom	-=dy;
						}

						s_BIGIMAGE->RenderRects(&sr, &dr);

						s_PIPE->end_scene();
						s_PIPE->swap_buffers();

						SleepEx(350, FALSE);	//pause a bit for user to visually check
					}
				}
			}
		}
	}

	//test image correctness
	{
		DIBSECTION	dib;
		int			w, h, bpp, stride;

		if((hTestBmp	=LoadImage(NULL, ImageFileNames[8], IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE )) == NULL)
		{
			RunTime->Log(IDR_LC_TEST, 0, _T("Unable to find image file: %s"), ImageFileNames[8]);
		}
		int	gotit	=GetObject((HGDIOBJ) hTestBmp, sizeof(dib), &dib);

		if(!gotit)
		{
			return	E_FAIL;
		}

		w		=dib.dsBm.bmWidth;
		h		=dib.dsBm.bmHeight;
		bpp		=dib.dsBm.bmBitsPixel;
		stride	=dib.dsBm.bmWidthBytes;

		U8	*pBits	=(U8 *)malloc(h * stride);
		
		gotit	=GetBitmapBits((HBITMAP)hTestBmp, h * stride, pBits);

		PixelFormat	dpf(PF_4CC_DAOT);
		PixelFormat	spf(bpp, 5, 6, 5, 0);

		BIGIMAGEDESC	bidesc(s_PIPE, &dpf, pBits, &spf, w, h, stride);
		if(FAILED(s_DACOM->CreateInstance(&bidesc, (void **)&s_BIGIMAGE)))
		{
			RunTime->Log(IDR_LC_TEST, 0, "s_DACOM->CreateInstance(BigImage) FAILED");
			return	DARTH_TEST_FAIL;
		}

		free(pBits);

		//draw a half screen rect to visually compare output
		RECT	srect	=SourceRects[8];
		RECT	drect	=DestRects[8];

		s_PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);
		s_PIPE->begin_scene();

		s_BIGIMAGE->RenderRects(&srect, &drect);

		s_PIPE->end_scene();
		s_PIPE->swap_buffers();

		//blit the original image to compare with
		{
			HDC		hRenderDC	=GetDC(hRender);
			HDC		hBmpDC		=CreateCompatibleDC(hRenderDC);
			HANDLE	old			=SelectObject(hBmpDC, hTestBmp);

			StretchBlt(hRenderDC, 320, 0, 319, 479, hBmpDC, 0, 0, 639, 479, SRCCOPY);
			SelectObject(hBmpDC, old);
			ReleaseDC(hRender, hRenderDC);
			DeleteDC(hBmpDC);
		}
	}

	HRESULT	RetVal;

	//prompt user for pass or fail
	if(MessageBox(hRender,
		"Did the grid appear uniform in all phases, and are the right and left images identical?",
		"Success?", MB_YESNO | MB_ICONQUESTION)	==IDYES)
	{
		RetVal	=DARTH_TEST_PASS;
	}else
	{
		RetVal	=DARTH_TEST_FAIL;
	}

	//shut everything down and return the result
	if(FAILED(s_PIPE->shutdown()))
	{
		RunTime->Log(IDR_LC_TEST, 0, "s_PIPE->shutdown() failed");
		return	E_FAIL;
	}

	DARTH_RELEASE(s_PIPE);
	DARTH_RELEASE(s_SYSTEM);
	
	s_DACOM->ShutDown();

	MATH_ENGINE_Uninitialize();

	DestroyWindow(hRender);

	return	RetVal;
}
