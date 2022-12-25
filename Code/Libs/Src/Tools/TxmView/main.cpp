
#include "project.h"
#include "sys.h"
#include "window.h"
#include "ini.h"		// INI_Reader
#include "resource.h"

#include <RendPipeline.h>
#include <engine.h>

extern int ScreenWidth;
extern int ScreenHeight;

//---------------------------------------------------------------------------
// DEBUG
//---------------------------------------------------------------------------

void DebugPrint (char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		OutputDebugString(work);
	}
}

//---------------------------------------------------------------------------
// OpenGL
//---------------------------------------------------------------------------

typedef unsigned char uchar;

inline uchar CLAMP (float r)
{
	r *= 255;
	if (r < 0)
		return 0;
	if (r > 255)
		return 255;
	return r;
}

void glViewport (int x, int y, int w, int h)
{
	PIPE->set_viewport(x,y,w,h);
}
void glClearColor (float r, float g, float b, float a)
{
	uchar rr = CLAMP(r);
	uchar gg = CLAMP(g);
	uchar bb = CLAMP(b);
	uchar aa = CLAMP(a);
	U32 clear_color = RGBA_MAKE(rr,gg,bb,aa);

	PIPE->set_pipeline_state(RP_CLEAR_COLOR,clear_color);
}
void glClearDepth (float depth)
{
	#define ZMAX double(0xFFFFFFFF);
	double clear_depth = depth*ZMAX;
	PIPE->set_pipeline_state(RP_CLEAR_DEPTH,clear_depth);
}

#define GL_COLOR_BUFFER_BIT		RP_CLEAR_COLOR_BIT
#define GL_DEPTH_BUFFER_BIT		RP_CLEAR_DEPTH_BIT

void glClear (int bits)
{
	DWORD flags = 0;
	if (bits & GL_COLOR_BUFFER_BIT)
	flags |= RP_CLEAR_COLOR_BIT;
	if (bits & GL_DEPTH_BUFFER_BIT)
	flags |= RP_CLEAR_DEPTH_BIT;

	// it should default to viewport
	RECT *clip = 0; //rc->scissor_enable ? &rc->scissor_box : 0;

	PIPE->clear_buffers(flags, clip);
}

void glOrtho (float left, float right, float bottom, float top, float znear=-1, float zfar=+1)
{
	PIPE->set_ortho(left,right, bottom,top, znear,zfar);
}

void glxModelView (const Transform &t)
{
	PIPE->set_modelview(t);
}

void glxOrtho2D (void)
{
	int w = ScreenWidth;
	int h = ScreenHeight;

	glViewport(0,0,w,h);    // is this necessary?

	Transform identity;

	glxModelView(identity);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	glOrtho(0,float(w),float(h),0, -1,+1);
}

void glxClearColor (U32 color)
{
	static float inv = 1.0 / 255;
	float r = RGBA_GETRED(color) * inv; 
	float g = RGBA_GETGREEN(color) * inv; 
	float b = RGBA_GETBLUE(color) * inv; 
	float a = 1.0;
	glClearColor(r,g,b,a);
}

void glxBindTexture (int id)
{
	PIPE->set_render_state(D3DRS_TEXTUREHANDLE, id);
}

void glFlush (void)
{
	PIPE->flush(RP_OPAQUE|RP_TRANSLUCENT_DEPTH_SORTED);
}

void glxTextureWrap (bool wrap)
{
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,param);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,param);

	if (wrap)
	{
		PIPE->set_render_state(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
		PIPE->set_render_state(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_WRAP);
	}
	else
	{
		PIPE->set_render_state(D3DRS_TEXTUREADDRESSU, D3DTADDRESS_CLAMP);
		PIPE->set_render_state(D3DRS_TEXTUREADDRESSV, D3DTADDRESS_CLAMP);
	}
}

//---------------------------------------------------------------------------
// GLOBAL
//---------------------------------------------------------------------------

HINSTANCE	AppInstance = 0;
HMODULE		AppResource = 0;

int AppState = 0;

char ExePath[MAX_PATH] = "";
char DataPath[MAX_PATH] = "";

char InitialFile[MAX_PATH] = "";

bool IsRoot (const char *name)
{
	return name[0] && (name[0] == '\\' || name[1] == ':');
}

void MakePath (char *dst, const char *path, const char *file)
{
	if (IsRoot(file))
	{
		strcpy(dst,file);
	}
	else
	{
		_makepath(dst, 0,path,file,0);
	}
}

void OptionalPath (char *src, const char *path)
{
	char work[MAX_PATH];
	MakePath(work,path,src);
	strcpy(src,work);
}

void StripWrapper (char *dst, const char *src, char first)
{
	if (src[0] == first)
	{
		char last;
		switch (first)
		{
			case '(': last = ')'; break;
			case '[': last = ']'; break;
			case '{': last = '}'; break;
			case '<': last = '>'; break;
			default:  last = first;
		}
		src += 1;
		int l = strlen(src);
		if (src[l-1] == last)
			l -= 1;
		memcpy(dst,src,l);
		dst[l] = 0;
	}
	else
	{
		strcpy(dst,src);
	}
}

//---------------------------------------------------------------------------
// Configure
//---------------------------------------------------------------------------

void Configure (const char *name)
{
	INI_Reader ini;

	if (ini.open(name))
	{
		ini.reset();

		while (ini.read_header())
		{
			if (ini.is_header("Path"))
			{
			}
		}

		ini.close();
	}
}

//---------------------------------------------------------------------------
// Clock
//---------------------------------------------------------------------------

struct Clock
{
	LARGE_INTEGER	last,now;
	U32				ticks,elapsed;

	float			dt;

	double			tick_time;

	Clock (void)
	{
	//
	// Get timing data for FPS count
	//
		LARGE_INTEGER  freq;

		QueryPerformanceFrequency(&freq);

		ticks = freq.u.LowPart;			// clock ticks per second
		assert(freq.u.HighPart == 0);

		QueryPerformanceCounter(&last);

		dt = 1.0 / 30;

		tick_time = 1.0 / double(ticks);
	}

	float update (void)
	{
		QueryPerformanceCounter(&now);
		elapsed = now.u.LowPart - last.u.LowPart;
		last = now;

		dt = elapsed * tick_time;

		return dt;
	}
};

//---------------------------------------------------------------------------
// TxmView
//---------------------------------------------------------------------------

#include "common.h"		// CDocument
#include "pen.h"		// CBrush
#include "filesys.h"
#include "iTxmLib.h"

struct Color
{
	union // anonymous
	{
		struct // anonymous
		{
			uchar b,g,r,a;
		};
		U32 color;
	};

	operator = (U32 c)
	{
		color = c;
	}

	operator U32 (void)
	{
		return color;
	}
};

struct TxmView : CDocument
{
	HMENU		hMenu;
	CStatusBar	status_bar;
	ToolTip		tool_tip;

	CBrush		outline_color;
	Color		background_color;

	U32			object;		// temporary holder

// TEXTURE

	int			txm_index;	// current texture to display

	float		width;		// size of texture on screen
	float		height;

	float		scale;
	float		tile_x;
	float		tile_y;

	bool		show_mipmaps;

	TxmView (void)
	{
		hMenu = 0;

		outline_color.create(0,0,255);

		txm_index = 0;
		object = -1;

		background_color = RGB_MAKE(0,0,32);

		scale = 1;
		width = 256;
		height = 256;

		tile_x = 1;
		tile_y = 1;

		show_mipmaps = false;
	}

	U32 get_texture (void) const
	{
		return TXMLIB->get_entry_id(txm_index);
	}

	void update_info (void)
	{
		char msg[256];

		U32 t = get_texture();
		if (t == INVALID_TXM_ID)
		{
			sprintf(msg,"TXM=%d  invalid",txm_index);
		}
		else
		{
			width = TXMLIB->get_width(t);
			height = TXMLIB->get_width(t);

			int w = width;
			int h = height;

			char name[128];
			TXMLIB->get_texture_name(t,name);

			sprintf(msg,"TXM=%d  w=%d,h=%d, format=%d,%d,%d,%d  '%s'",
				txm_index,w,h,
				TXMLIB->get_red_bits(t),
				TXMLIB->get_green_bits(t),
				TXMLIB->get_blue_bits(t),
				TXMLIB->get_alpha_bits(t),
				name
				);
		}
		status_bar.SetText(msg);
	}

	void set_texture (int index)
	{
		txm_index = index;
		update_info();
	}

	void load_texture (const char *filename)
	{
		DAFILEDESC desc(filename);
		IFileSystem *fs = 0;
		DACOM->CreateInstance(&desc,(void**)&fs);
		if (fs)
		{
			TXMLIB->load_library(fs);
		}
		set_texture(txm_index);
	}
	void load_object (const char *filename)
	{
		DAFILEDESC desc(filename);
		IFileSystem *fs = 0;
		DACOM->CreateInstance(&desc,(void**)&fs);
		if (fs)
		{
			if (fs->SetCurrentDirectory("Texture library"))
			{
				fs->SetCurrentDirectory("..");
				TXMLIB->load_library(fs); // for (*.CMP) compound objects
			}
			else if (fs->SetCurrentDirectory("openFLAME 3D N-mesh"))
			{
				TXMLIB->load_library(fs);
				fs->SetCurrentDirectory("..");
			}

			//object = ENGINE->create_archetype(filename, fs);
			//destroy?
		}
		set_texture(txm_index);
	}

	virtual void OnCreate (CREATESTRUCT *cs)
	{
		SetMenu(IDR_VIEWER);
		SetAccelerator(IDR_ACCEL_VIEWER);

		status_bar.Create(CCS_BOTTOM,hWnd,IDC_STATUS_BAR);
		status_bar.SetText("Version: May 5 1999 (pci) Digital Anvil");

		tool_tip.Create(hWnd,TTS_ALWAYSTIP);
		tool_tip.AddTool(hWnd,"test",0,0);
		//tool_tip.AddTool(hWnd,LPSTR_TEXTCALLBACK,0,0);
		tool_tip.SendMessage(TTM_SETMAXTIPWIDTH, 0, 1024);
		tool_tip.SendMessage(TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767);
		tool_tip.SendMessage(TTM_SETDELAYTIME, TTDT_INITIAL, 25);//200);
		tool_tip.SendMessage(TTM_SETDELAYTIME, TTDT_RESHOW, 200);

if (!TheSystem.open_view(this))
{
	OnClose();
	//AppState = -1;
}

SetWindowLong(hWnd, GWL_STYLE,
	GetWindowLong(hWnd, GWL_STYLE)
	| WS_POPUP
	& ~(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX||WS_THICKFRAME));


		// FUTURE: tool_bar?

		RECT rect;
		::GetClientRect(hWnd,&rect);
		OnSize(SIZE_RESTORED, rect.right,rect.bottom);
	}

	virtual void OnDestroy (void)
	{
		PostQuitMessage(0);
	}

	virtual void OnSize (int size_type, int w, int h)
	{
		if (size_type==SIZE_RESTORED || size_type==SIZE_MAXIMIZED)
		{
			status_bar.SendMessage(WM_SIZE, 0); // auto re-size
			Invalidate();
		}
	}

inline void SetXY (RPVertex &v, float _x, float _y)
{
	v.pos.x = _x;
	v.pos.y = _y;
	v.pos.z = 0;
}
inline void SetUV (RPVertex &v, float _u, float _v)
{
	v.u = _u;
	v.v = _v;
}

	void DrawRect (int x, int y, int w, int h, float u1=1, float v1=1)
	{
		RPVertex vlist[4];

		float x0 = x;
		float x1 = x+w;
		float y0 = y;
		float y1 = y+h;

		float u0 = 0;
		float v0 = 0;

		RPVertex *v = vlist;
		SetXY(*v, x0,y0); SetUV(*v, u0,v0); v->color = RGBA_MAKE(255,255,255, 255);
		v++;
		SetXY(*v, x1,y0); SetUV(*v, u1,v0); v->color = RGBA_MAKE(255,255,255, 255);
		v++;
		SetXY(*v, x1,y1); SetUV(*v, u1,v1); v->color = RGBA_MAKE(255,255,255, 255);
		v++;
		SetXY(*v, x0,y1); SetUV(*v, u0,v1); v->color = RGBA_MAKE(255,255,255, 255);
		v++;

		int vcount = v - vlist;

		U16 ilist[] = { 0,1,2, 0,2,3 };
		int icount = 6;

		PIPE->draw_indexed_primitive(D3DPT_TRIANGLELIST, D3DVT_RPVERTEX, vlist,vcount, ilist,icount, true);
	}

	virtual void OnPaint (HDC dc)
	{
		RECT rect;
		GetClientRect(&rect);
		PIPE->set_window(hWnd,rect.left,rect.top,rect.right,rect.bottom);

		glViewport(0,0,rect.right,rect.bottom);

			int cx = rect.right / 2;
			int cy = rect.bottom / 2;

			//PIPE->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);//D3DCULL_CCW);

		PIPE->begin_scene();

		glxClearColor(background_color);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glxOrtho2D();

			U32 t = get_texture();
			glxBindTexture(t);

			float w,h;
			if (t == INVALID_TXM_ID)
				w = h = 16;
			else
			{
				w = TXMLIB->get_width(t)*scale;
				h = TXMLIB->get_height(t)*scale;
			}

			glxTextureWrap(true);

			DrawRect(cx-w/2,cy-h/2, w,h,tile_x,tile_y);

			if (show_mipmaps)
			{
				int left = cx-w/2;
				int bottom = cy+h/2 + 2;

				while (w > 1)
				{
					w = w/2;
					h = h/2;

					DrawRect(left,bottom, w,h,tile_x,tile_y);
					left += w + 2;
				}
			}

        PIPE->end_scene();

        PIPE->swap_buffers();
	}
	virtual void OnErase (HDC dc)
	{
	}

	void set_scale (float s)
	{
		scale = s;
		char msg[128];
		sprintf(msg,"scale = %f",scale);
		status_bar.SetText(msg);
		Invalidate();
	}

	virtual void OnKeyDown (int key, int shifts)
	{
		switch (key)
		{
			case 'B':
				background_color.b += 4;
				Invalidate();
				break;

			case 'M':
				show_mipmaps = !show_mipmaps;
				Invalidate();
				break;

			case VK_NEXT:
				set_texture(txm_index+1);
				Invalidate();
				break;

			case VK_PRIOR:
				set_texture(txm_index-1);
				Invalidate();
				break;

			case VK_HOME:		set_scale(1.0); break;

			case VK_ADD:		set_scale(scale*1.25); break;
			case VK_SUBTRACT:	set_scale(scale/1.25); break;

			case VK_LEFT:		tile_x -= 1; if(tile_x<1)tile_x=1; Invalidate(); break;
			case VK_RIGHT:		tile_x += 1; Invalidate(); break;
			case VK_UP:			tile_y -= 1; if(tile_y<1)tile_y=1; Invalidate(); break;
			case VK_DOWN:		tile_y += 1; Invalidate(); break;

			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
		}
	}
};

//---------------------------------------------------------------------------
// EditorApp
//---------------------------------------------------------------------------

struct EditorApp
{
	Clock		clock;

	TxmView		view;

	EditorApp (void)
	{
		AppState = 0;
	}

	int open (HINSTANCE hInstance, char *cmd_line)
	{
		int ok = 0;

		char file[_MAX_PATH];
		_makepath(file, 0,ExePath,"universe.ini",0);
		Configure(file);

		AppState = 1;

if (cmd_line && cmd_line[0])
{
	StripWrapper(InitialFile, cmd_line, '"');
	if (_access(InitialFile,0) == -1)
	{
		MessageBox(0,InitialFile,"_access = DENIED!",MB_OK);
		InitialFile[0] = 0;
	}
}

		view.create(0,"TxmView",0,320,320);
		ok = (view.hWnd != 0);

		ShowCursor(1);

		//ok = TheSystem.is_ready();

	if (InitialFile[0])
	{
		view.load_object(InitialFile);
	}

		return ok;
	}

	void close (void)
	{
		ShowCursor(1);
	}

	bool update_message (int &result)
	{
		MSG msg;
		result = 0;

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
			{
				result = msg.wParam;
				return false;
			}

//DebugPrint("Wnd = %X\n",msg.hwnd);

			BOOL used = false;

			CWnd *w = CWnd::FromHandle(msg.hwnd);
			if (w)
			{
				used = w->PreTranslateMessage(&msg);
			}
			if (!used)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return true;
	}

	int main_loop (void)
	{
		int FrameIndex = 0;

		int result = 0;

		while (AppState==1)		// open and ready for action
		{
			if (!update_message(result))
				break;

			if (view.hWnd == 0)
				break;

			float dt = clock.update();

			view.Invalidate();

			++FrameIndex;
		}

		return result;
	}
};

EditorApp TheApp;

//---------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------

#include <commctrl.h>

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmd_line, int nCmdShow)
{
	int ok = 1;

	AppInstance = hInstance;
	AppResource = GetModuleHandle("universe.exe");

	InitCommonControls(); // Requires: comctl32.lib 
	InitAppControls();

	SystemParametersInfo(SPI_SETDRAGWIDTH,3,0, 0);//SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETDRAGHEIGHT,3,0, 0);//SPIF_SENDCHANGE);

		int cx = GetSystemMetrics(SM_CXDRAG);
		int cy = GetSystemMetrics(SM_CYDRAG);

	char exe[_MAX_PATH];
	GetModuleFileName(0,exe,sizeof(exe));
	char drive[_MAX_DRIVE];
	char path[_MAX_PATH];
	_splitpath(exe, drive,path,0,0);
	_makepath(ExePath, drive,path,0,0);

	if (!TheSystem.startup(hInstance))
	{
		DebugPrint("ERROR: System failed!\n");
		return false;
	}

	if (TheApp.open(hInstance,cmd_line))
	{
//		Font::startup("fonts.txm");

		int err = TheApp.main_loop();
		TheApp.close();
		if (err)
			exit(1);
	}
	else // failed to startup?
	{
		exit(1);
		ok = 0;
	}

	return ok;
}

HWND GetMainWindow (void)
{
	return 0; //TheApp.universe.hWnd;	// AppWindow
}

//---------------------------------------------------------------------------
