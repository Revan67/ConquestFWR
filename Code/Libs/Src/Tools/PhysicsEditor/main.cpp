//
//  DACOM/GL testbed wrapper
//

#include "main.h"
#include "input.h"
#include "resource.h"
#include "iprofileparser.h"
#include <commctrl.h>

// DACOM components:

IEngine *           ENGINE      = NULL;
IPhysics *          PHYSICS     = NULL;
IRenderer *         RENDERER    = NULL;
ICOManager *        DACOM       = NULL;
ILightManager *     LIGHT       = NULL;
IWindowManager *    WIN         = NULL;
ITextureLibrary *	TEXTURELIB	= NULL;
IHardpoint *        HARDPOINT   = NULL;
IAnimation *        ANIM        = NULL;
IChannel *          CHANNEL     = NULL;
ICollision *        COLLISION   = NULL;
IRenderPipeline *	RP			= NULL;
ISystemContainer *	SYSTEM		= NULL;

const char * appName = "Physics Editor v1.02";
     
// Windows components:

HINSTANCE   appInstance;
HWND        windowHandle;
HDC         globalDC;

// App globals:

BOOL32      fullScreen  = FALSE;
BOOL32      exitProgram = FALSE;
Timer       timer;

KeyboardDriver *keyboard;

extern void appInit();
extern void appMainLoop();
extern void appUninit();
extern void appWindowCallback(UINT msg, WPARAM wParam, LPARAM lParam);

void initComponents();
void destroyComponents();

LRESULT CALLBACK windowCallback(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

inline const char * format_error_msg( const char * fmt, ... ) 
{
	static char errorstr[1024] = {"\0"};

	va_list args;
	va_start (args, fmt);
	_vsnprintf (errorstr, 1024, fmt, args); 
	va_end (args);

	return errorstr;
}

int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

static void windowExit(int)
{
	exit(0);
}

void AppExit(void)
{
    appUninit();
	destroyComponents();
	exit(-1);
	
}

void AppFatal(C8 *message)
{
	MessageBox(windowHandle, message, "AppError", MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
    AppExit();
	
}

#define GET_COMPONENT( com, str, ptr ) \
	if (com->QueryInterface(str, (void **) &ptr) != GR_OK) {\
		AppFatal("Missing " #com " component (" str "). (INI file problem?)"); \
	} \

#define RELEASE_COMPONENT(x)	if (x) { x->Release(); x = NULL; }

void initComponents()
{
	// Initialize component object manager.
	DACOM = DACOM_Acquire();

	if (!DACOM)
	{
		AppFatal("Failed to acquire DACOM.");
	}

	if (DACOM->SetINIConfig("phyedit.ini", 0) != GR_OK)
	{
		AppFatal("Couldn't find INI file.");
	}
	
	// initialize app window


	WNDCLASSEX wc;
	memset( &wc, 0, sizeof( wc ));

	wc.cbSize		= sizeof (wc) ;
	wc.style		= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	= windowCallback;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= appInstance;
	wc.hIcon		= 0;
	wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "Physics Editor";
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_PE_MENU);
		
	AGGDESC adesc = "ISystemContainer";

	if (DACOM->CreateInstance(&adesc, (void **) &SYSTEM) != GR_OK)
	{
		AppFatal("Missing system container. (INI file problem?)");
	}
	
	SYSTEM->LoadSystemComponents();

	GET_COMPONENT( SYSTEM, "IWindowManager", WIN );
	WIN->Startup(appInstance, appName, windowExit, 0, &wc);
	
	if( FAILED( SYSTEM->QueryInterface ( IID_IRenderPipeline, (void**)&RP ) ) ) {
		AppFatal("Failed to find IID_IRenderPipeline\n");
	}
	
	DACOMDESC desc = "IEngine";

	if (DACOM->CreateInstance(&desc, (void **) &ENGINE) != GR_OK)
	{
		AppFatal("Missing engine system. (INI file problem?)");
	}

	GET_COMPONENT( SYSTEM, "ILightManager",	LIGHT );
	GET_COMPONENT( SYSTEM, "ITextureLibrary", TEXTURELIB);
	
	ENGINE->load_engine_components(SYSTEM);
	{
		GET_COMPONENT( ENGINE, "IRenderer",		RENDERER );
		GET_COMPONENT( ENGINE, "IHardpoint",	HARDPOINT );
		GET_COMPONENT( ENGINE, "ICollision",	COLLISION );
	}

	if(ENGINE->QueryInterface(IID_IPhysics, (void **)&PHYSICS) != GR_OK)
	{
		AppFatal("Missing physics dammit! (INI file problem?)");
	}

	// Initialize window handle:
	windowHandle = WIN->GetWindowHandle();

	// Initialize IRenderPipeline:
	if (RP->startup())
	{
		AppFatal("Unable to startup IRenderPipeline");
	}

	RP->set_pipeline_state(RP_BUFFERS_FULLSCREEN, FALSE);
	RP->set_pipeline_state(RP_BUFFERS_COLOR_BPP, 16);
	RP->set_pipeline_state(RP_BUFFERS_DEPTH_BPP, 16);

	if (RP->create_buffers(windowHandle, 640, 480) != GR_OK)
	{
		AppFatal("Unable to create IRenderPipeline buffers.");
	}

	//WIN->ShowWindow(640, 480, 0);
	WIN->SetWindowPos(640, 480, WMF_SHOW);

	// Reroute SAL window events to the system handler.
	WIN->SetCallback(windowCallback);

	// Keyboard driver:
    keyboard = new KeyboardDriver();

    // Lights:
    LIGHT->set_ambient_light(255, 255, 255);

	// error handler
	FDUMP = STANDARD_DUMP;
	
}

void destroyComponents(void)
{
    delete keyboard;
    
	if (RP)
	{
		RP->destroy_buffers();
		RP->shutdown();
		RP = NULL;
	}

	RELEASE_COMPONENT(PHYSICS);
	RELEASE_COMPONENT(RENDERER);
	RELEASE_COMPONENT(LIGHT);
	RELEASE_COMPONENT(TEXTURELIB);
	RELEASE_COMPONENT(ANIM);
	RELEASE_COMPONENT(CHANNEL);
	RELEASE_COMPONENT(WIN);

	RELEASE_COMPONENT(ENGINE);
	RELEASE_COMPONENT(SYSTEM);

	if (DACOM != NULL)
	{
		DACOM->ShutDown();
		DACOM = NULL;
	}

}
int PASCAL WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	// Initialize component object manager.
	InitCommonControls();

	appInstance = hInstance;
	
    initComponents();

 	appInit();

    do	
	{
		appMainLoop();
        WIN->ServeMessageQueue();
    } 
	while (!exitProgram);

	appUninit();

	destroyComponents();

	return 0;
}

void setWindowedMode()
{
	fullScreen = false;

	RP->set_pipeline_state(RP_BUFFERS_FULLSCREEN, fullScreen);
	RP->create_buffers(windowHandle, 640, 480);

	SetWindowPos(windowHandle, NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE );
	WIN->SetWindowPos(640, 480, WMF_SHOW);
}

void setFullscreenMode()
{
	fullScreen = true;

	RP->set_pipeline_state(RP_BUFFERS_FULLSCREEN, fullScreen);
	RP->create_buffers(windowHandle, 640, 480);

	WIN->SetWindowPos(640, 480, WMF_FULL_SCREEN);
}

LRESULT CALLBACK windowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
	{
		case WM_KEYDOWN:
			
            if (wParam == VK_TAB)
			{
				if (fullScreen)
					setWindowedMode();
				else
					setFullscreenMode();
			}
            else
            if (wParam == VK_ESCAPE)
            {
                exitProgram = TRUE;
            }

            keyboard->keyboard_array[wParam] = TRUE;

            break;

        case WM_KEYUP:
            keyboard->keyboard_array[wParam] = FALSE;
            
            break;
		        
	}

   appWindowCallback(message, wParam, lParam);

   return DefWindowProc(hwnd, message, wParam, lParam);

}

// fdump interface
int __cdecl STANDARD_DUMP (ErrorCode code, const C8 * fmt, ...)
{
	if (code.severity <= DA_ERROR_LEVEL)
	{
		char buffer[4096];
		{
			va_list args;
			va_start (args, fmt);
			_vsnprintf (buffer, 4096, fmt, args);
			va_end (args);
		}

		OutputDebugString(buffer);
		
		if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || code.severity == SEV_WARNING)
		{
			// skip the polymesh no-texture warning
			if (strstr(buffer, "cur->texture_id")) return 0;

			DWORD dwFlags = (code.severity == SEV_ERROR) ? (MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_TOPMOST) : 
														   (MB_ICONEXCLAMATION | MB_OK | MB_TOPMOST);

			switch (MessageBox(windowHandle, buffer, "Physics Editor", dwFlags))
			{
				case IDOK:
					
					if (code.severity == SEV_FATAL)
					{
						__asm int 3;
					}
					
					break;

				case IDABORT:
					
					exit(-1);
					
					break;
			
				case IDRETRY:

					__asm int 3;

					break;
			
				case IDIGNORE:

					break;
			}
		}
	}

	return 0;
}

