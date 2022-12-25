
#include "project.h"
#include "window.h"

#include "resource.h"

//---------------------------------------------------------------------------

Window *Window::Head = 0;

void Window::Register (Window *w)
{
	if (w)
	{
		w->next = Window::Head;
		Window::Head = w;
	}
}

void Window::UnRegister (Window *old)
{
	Window *prev = 0;
	
	for (Window *w = Window::Head; w; w=w->next)
	{
		if (w == old)
		{
			if (prev)
				prev->next = old->next;
			else
				Window::Head = old->next;
			break;
		}
	}
}

Window *Window::FromHandle (HWND h)
{
	if (h != 0)
	{
		for (Window *w = Window::Head; w; w=w->next)
		{
			if (w->hWnd == h)
				return w;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
// WndProc
//---------------------------------------------------------------------------

int LastMX = 0;
int LastMY = 0;
int LastBTN = 0;
int LastShift = 0;

static long FAR PASCAL MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if 0
	DebugPrint("WND h=%X, m=%X, w=%X,l=%X\n",hWnd,message,wParam,lParam);
#endif

	Window *win = Window::FromHandle(hWnd);

    switch (message)
    {
	case WM_COMMAND:
		if (win)
			win->OnCommand(HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
		break;

	case WM_PARENTNOTIFY:
		{
			//DebugPrint("WM_PARENTNOTIFY (w)\n");
			/*
		int id = (int) wParam; 
		LPNMHDR hdr = (LPNMHDR) lParam;
		if (win)
			win->OnParentNotify(id,hdr);
			*/
		}
		break;

	case WM_NOTIFY:
		{
		int id = (int) wParam; 
		LPNMHDR hdr = (LPNMHDR) lParam;
		if (win)
			win->OnNotify(id,hdr);
		}
		break;

    case WM_ACTIVATEAPP:
		//OnActivateApp(wParam != 0);
        break;

    case WM_ACTIVATE:
		if (win)
			win->OnActivate(wParam != 0);
        break;

    case WM_CREATE:
	{
		CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
		win = (CWnd *)cs->lpCreateParams ;
		if (win)
		{
			assert(win->hWnd == 0);
			win->hWnd = hWnd;
			//Note: avoid WM_PAINT?
			win->OnCreate(cs);
		}
        break;
	}

	case WM_ERASEBKGND:
		if (win)
			win->OnErase((HDC)lParam);
		break;

    case WM_DESTROY:
		if (win)
		{
			win->OnDestroy();
			win->dead = true;
		}
        break;

	case WM_QUIT:
		DestroyWindow(hWnd);
		break;

    case WM_CLOSE:
		if (win)
		{
			win->OnClose();
			return 1;
		}
		break;

	case WM_PAINT:
	if (win)
	{
		PAINTSTRUCT	ps;
		HDC			hdc;

        hdc = BeginPaint(hWnd, &ps);
		win->OnPaint(hdc); // If the display is in window mode, blit the buffer
        EndPaint(hWnd, &ps);
        return 1;
	}
	break;

	case WM_SIZE:
		if (win)
			win->OnSize(wParam, LOWORD(lParam),HIWORD(lParam));
		break;

// KEYBOARD CONTROL

	case WM_KEYDOWN:
		{
		int shift = 0;
		if (GetKeyState(VK_SHIFT) < 0) shift |= MK_SHIFT;
		if (GetKeyState(VK_CONTROL) < 0) shift |= MK_CONTROL;
		LastShift = shift;
		if (win)
			win->OnKeyDown(wParam,LastShift);
		}
		break;
	case WM_KEYUP:
		{
		int shift = 0;
		if (GetKeyState(VK_SHIFT) < 0) shift |= MK_SHIFT;
		if (GetKeyState(VK_CONTROL) < 0) shift |= MK_CONTROL;
		LastShift = shift;
		if (win)
			win->OnKeyUp(wParam,LastShift);
		}
		break;

	case WM_SYSKEYDOWN:
		if (win)
			win->OnSysKeyDown(wParam);
		break;
	case WM_SYSKEYUP:
		if (win)
			win->OnSysKeyUp(wParam);
		break;

// MOUSE CONTROL

	case WM_MOUSEMOVE:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win)
			win->OnMouseMove(LastMX,LastMY,LastShift);
		break;

	case WM_NCMOUSEMOVE:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = 0;
		if (win)
			win->OnNcMouseMove(LastMX,LastMY,wParam);
		break;

	case WM_LBUTTONDOWN:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnLButtonDown(LastMX,LastMY,LastShift);
		break;
	case WM_LBUTTONUP:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnLButtonUp(LastMX,LastMY,LastShift);
		break;
	case WM_LBUTTONDBLCLK:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnLButtonDblClk(LastMX,LastMY,LastShift);
		break;

	case WM_RBUTTONDOWN:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnRButtonDown(LastMX,LastMY,LastShift);
		break;
	case WM_RBUTTONUP:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnRButtonUp(LastMX,LastMY,LastShift);
		break;
	case WM_RBUTTONDBLCLK:
		LastMX = LOWORD(lParam); LastMY = HIWORD(lParam); LastShift = wParam;
		if (win) win->OnRButtonDblClk(LastMX,LastMY,LastShift);
		break;

    default:
		if (win)
		{
			int result = win->OnMessage(message,wParam,lParam);
			if (result != 0)
				return result;
		}
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------
// CREATE WINDOW
//---------------------------------------------------------------------------

void InitAppControls (void)
{
	char *class_name = "AppClass";

	HINSTANCE hInstance = AppInstance;

	WNDCLASS    wc;

	if (!GetClassInfo(hInstance,class_name,&wc))
	{
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = MainWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VIEWER));
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
		wc.lpszMenuName =  NULL;
		wc.lpszClassName = class_name;
		RegisterClass(&wc);
	}
}

//---------------------------------------------------------------------------

bool CWnd::Create (LPCTSTR class_name, LPCTSTR title, bool menu, DWORD style,
	const RECT &_rect, HWND parent, UINT nID)
{
	WNDCLASS    wc;

	if (!GetClassInfo(AppInstance,class_name,&wc))
		return FALSE;

	RECT rect = _rect;
	AdjustWindowRectEx(&rect, style, menu, 0);

	POINT pos;
	pos.x = _rect.left;
	pos.y = _rect.top;

	if (parent)
	{
		ScreenToClient(parent,&pos);
	}

	hWnd = CreateWindowEx(0,
		class_name,
		title,
		style, 
		pos.x,
		pos.y,
		rect.right - rect.left + 1,
		rect.bottom - rect.top + 1,
		parent,
		(HMENU)nID,
		AppInstance,
		this);

	if (hWnd)
	{
		Invalidate();
		//UpdateWindow(hWnd);
	}

	return (hWnd != 0);
}

//---------------------------------------------------------------------------

void CenterRect (HWND parent, RECT &rect, int w, int h)
{
	if (parent == 0)
		::GetWindowRect(GetDesktopWindow(),&rect);
	else
		::GetWindowRect(parent,&rect);

	int cx = (rect.left+rect.right)/2;
	int cy = (rect.top+rect.bottom)/2;
	rect.left = max(0,cx-w/2);
	rect.top = max(0,cy-h/2);
	rect.right = rect.left+w-1;
	rect.bottom = rect.top+h-1;
}

//---------------------------------------------------------------------------

bool CWnd::create (HWND parent, const char *title, POINT *pos, int w, int h, const char *class_name)
{
	destroy();

	RECT rect;

	if (pos == 0)
	{
		CenterRect(parent,rect,w,h);
	}
	else
	{
		rect.left = pos->x;
		rect.top = pos->y;
		rect.right = rect.left+w-1;
		rect.bottom = rect.top+h-1;
	}

	int style = WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW;
	return Create(class_name,title,true,style,rect,parent,0);
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

