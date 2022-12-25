// Sequence.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Sequence.h"
#include "gamesys.h"

#include	<assert.h>

#include "MainFrm.h"
#include "ChildFrm.h"
#include "SequenceDoc.h"
#include "SequenceView.h"
#include "character.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// SequenceApp

BEGIN_MESSAGE_MAP(SequenceApp, CWinApp)
	//{{AFX_MSG_MAP(SequenceApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SequenceApp construction

SequenceApp::SequenceApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

SequenceApp::~SequenceApp()
{
	if(game_camera)
	{
		delete	game_camera;
	}
}

/////////////////////////////////////////////////////////////////////////////
// The one and only SequenceApp object

SequenceApp theApp;

/////////////////////////////////////////////////////////////////////////////
// SequenceApp initialization

BOOL SequenceApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
//	CoInitializeEx(NULL, NULL);
	CoInitialize(NULL);

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_SEQUENTYPE,
		RUNTIME_CLASS(SequenceDoc),
		RUNTIME_CLASS(ChildFrame), // custom MDI child frame
		RUNTIME_CLASS(SequenceView));
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	MainFrame* pMainFrame = new MainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Acquire DACOM

	m_DACOM = DACOM_Acquire();

	if (m_DACOM == NULL) {
		return 0;
	}

	// Get the profile parser interface
	m_DACOM->QueryInterface("IProfileParser", (void **) &m_PROF);

	assert(m_PROF != NULL);

	m_DACOM->SetINIConfig("sequence.ini");

	// We startup as far as the PIPE then shut it down and
	// start it up again to make sure we get a clean start
	m_GS = NULL;
	AGGDESC adesc = "ISystemContainer";

	if (m_DACOM->CreateInstance(&adesc, (void **) &m_GS) == GR_OK) 
	{
		m_GS->LoadSystemComponents();
	}

	if (m_GS == NULL)
	{
		return 0;
	}

	GENRESULT ret = m_GS->QueryInterface(IID_IRenderPipeline, (void **) &m_PIPE);
	if (m_PIPE == NULL)
	{
		return 0;
	}

	m_PIPE->startup();

// Engine hasn't been initialized yet; should be no extra references to
// IRenderPipeline.
	m_PIPE->Release();

	m_GS->Shutdown();
	m_GS->Release();

	if (m_DACOM->CreateInstance(&adesc, (void **) &m_GS) == GR_OK) 
	{
		m_GS->LoadSystemComponents();
	}

	if (m_GS == NULL || (m_ENG = CreateGameEngine(m_GS)) == 0) 
	{
		if (m_GS != NULL) 
		{
			m_GS->Release();
			m_GS = NULL;
		}
		return 0;
	}

//	IncreaseHeapSize(0x800000, 0x800000, DAHEAPFLAG_GROWHEAP);

//	MarkAllocatedBlocks(HEAP);

	m_GS->QueryInterface("IRenderPipeline", (void **) &m_PIPE);
	m_GS->QueryInterface("IRenderPrimitive", (void **) &m_BATCH);
	m_GS->QueryInterface("IViewConstructor2", (void **) &m_PARSER);
	
	m_PIPE->startup();

	m_ENG->QueryInterface("IPhysics",(void**) &m_PHY);
	m_ENG->QueryInterface("ICollision",(void**) &m_COLL);
	m_ENG->QueryInterface("IRenderer", (void **) &m_REND);
	m_ENG->QueryInterface("ILightManager", (void **) &m_LIGHTMAN);
	m_ENG->QueryInterface(IID_ITextureLibrary, (void **) &m_TEXLIB);

	m_ENG->QueryInterface("IAnimation", (void **) &m_ANIM);
	m_ENG->QueryInterface("IHardpoint", (void **) &m_HARDPOINT);
	m_ENG->QueryInterface("IModel", (void **) &m_MODEL);

	DeformOpen(m_GS, m_ENG);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();


	m_PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP,	16);
	m_PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP,	16);
//	m_PIPE->set_pipeline_state(RP_BUFFERS_STENCIL_BPP,	8);
	m_PIPE->set_pipeline_state(RP_BUFFERS_COUNT,		2);
	m_PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,		FALSE);
	m_PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_AUTOW,	FALSE);

	//m_PIPE->set_pipeline_state(RP_BUFFERS_OFFSCREEN, TRUE);
	//m_PIPE->set_pipeline_state(RP_DEBUG_PROFILE_LOG, TRUE);

//	m_physics = new PhysicalSystem;

 	// Create the Main Window..which in turn creates all the child windows
//	m_pMainWnd = new CMainWnd();
//	m_pMainWnd->Create(m_AppName);

//	m_hWnd = m_pMainWnd->GetMainWndHandle();

//	m_input = new Input;
//	m_input->open();

	// We need to create the buffers before we try to load any textures
	if (m_PIPE->create_buffers(NULL, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) != GR_OK)
	{
		RECT rect;
		this->m_pMainWnd->GetClientRect(&rect);
		int w = rect.right - rect.left + 1;
		int h = rect.bottom - rect.top + 1;
		if (m_PIPE->create_buffers(NULL, w, h) != GR_OK)
		{
			if (m_PIPE->create_buffers(NULL, 800, 600) != GR_OK)
			{
				GENERAL_FATAL("Unable to create render buffers in CBruteApp::Initialize().\n");
			}
		}
	}

//
// SET UP BATCHING POOLS.
//
	m_BATCH->set_state(RPR_BATCH_POOLS,				RPR_OPAQUE|RPR_TRANSLUCENT_DEPTH_SORTED|RPR_TRANSLUCENT_UNSORTED);


// enable/disable batching, affects subsequent draw_primitive() calls.
	m_BATCH->set_state(RPR_BATCH,					FALSE);
// change translucent mode, affects subsequent draw_primitive() calls...
	m_BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE,	RPR_TRANSLUCENT_UNSORTED);

	m_BATCH->set_render_state(D3DRS_DITHERENABLE, TRUE);

//	m_pMainWnd->CreateChildren();
//	m_pPaletteWnd = m_pMainWnd->GetPaletteWnd();

	// Load a default texture for the terrain, the sky and a light
	IFileSystem* file = NULL;
	DAFILEDESC desc("data\\textures\\textures.txm");
	if (m_DACOM->CreateInstance(&desc, (void **) &file) == GR_OK) 
	{
		m_TEXLIB->load_library(file, NULL);

/*		if (m_TEXLIB->has_texture_id("Diffuse_alpha") == GR_OK) {
			ITL_TEXTURE_ID light_tex_id;
			m_TEXLIB->get_texture_id("Diffuse_alpha", &light_tex_id);
			m_TEXLIB->add_ref_texture_id(light_tex_id, &light_texture_id);
			m_TEXLIB->release_texture_id(light_tex_id);
		}*/

		file->Release();

		// Get rid of any texture we didn't want
		//m_TXMLIB->collect_garbage();
	}

	float fov = 90.0f; // 90 Degree Field of View

	// Make the cameras
	Transform transform;
	transform.set_identity();

	transform.set_position(Vector(0, 0, 3));

	game_camera	=new GameCamera(theApp.m_ENG, NULL);
	game_camera->set_Horizontal_FOV(fov);
	game_camera->set_Horizontal_to_vertical_aspect(4.0f/3.0f);
	game_camera->set_near_plane_distance(1.0f);
	game_camera->set_far_plane_distance(25000.0f);
	game_camera->build_view_planes();

	m_ENG->set_transform(game_camera->index, transform);

	return TRUE;
}

void SequenceApp::EngineMainDraw(Character	*c) 
{
	assert(c);
//	U32 flags = m_pMainWnd->GetPerspectiveView()->GetFlags();
	U32	flags	=0;


	m_BATCH->set_state(RPR_BATCH, FALSE);

	m_BATCH->begin_scene();

// THIS SEEMS TO BE NECESSARY TO GET CORRECT CULLING. ???
	m_BATCH->set_render_state(D3DRS_CULLMODE, D3DCULL_CCW);
	m_BATCH->set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);

//	if (flags & RVF_WIREFRAME) 
//	{
//		// Set wireframe mode
//		m_BATCH->set_render_state(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
//		SetNoTexture();
//	} 
//	else 
	{
		m_BATCH->set_render_state(D3DRS_FILLMODE, D3DFILL_SOLID);
	}

// NEW SKY SCHEME REQUIRES CLEARING ALWAYS.
	m_PIPE->set_pipeline_state(RP_CLEAR_STENCIL, 0);
	m_PIPE->clear_buffers(RP_CLEAR_COLOR_BIT | RP_CLEAR_DEPTH_BIT | RP_CLEAR_STENCIL_BIT, NULL);

	// Pick a camera
//	GameCamera* the_camera = (slew_mode) ? slew_camera : game_camera;
	GameCamera* the_camera = game_camera;

	the_camera->obj_attached	=CurChar;
//	the_camera->obj_attached	=NULL;

	Transform transform;
	transform.set_identity();
	static	float rx=0;
	static	float ry=0.6f;
	static	float rz=3;
	transform.rotate_x(rx);
	transform.rotate_y(ry);
	transform.rotate_z(rz);
//	rx	+=0.0069f;
//	ry	+=0.069f;
	rz	-=0.00069;
//	rz	+=0.69;

	Vector cpos = the_camera->get_position();// + camera_move_vector;
//	transform.set_position(Vector(rx, ry, rz));
//	transform.set_position(cpos);

//	m_ENG->set_transform(game_camera->index, transform);

//	the_camera->Update(69);

// Move the camera
//	the_camera->set_position(Vector(rx, ry, rz));
//	m_ENG->set_position(the_camera->index, cpos);
//	m_ENG->set_position(the_camera->index, Vector(rx, ry, rz));
//	camera_move_vector.zero();

// Set up the projection matrix into the pipeline
	m_BATCH->set_perspective(the_camera->get_fovy(), the_camera->get_aspect(), 0.25f, the_camera->get_zfar());
	Transform camx_inv = the_camera->get_inverse_transform();

	m_BATCH->set_modelview(camx_inv);
//	SetDefaultTextureBlend();

	m_LIGHTMAN->update_lighting(the_camera);
	m_LIGHTMAN->deactivate_all_lights();
	//
	// Render objects. Batch opaque & translucent with sorting.
	//
//	m_BATCH->set_state(RPR_BATCH, TRUE);
//	m_BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_DEPTH_SORTED);

//	m_BATCH->set_modelview(camx_inv);
	m_LIGHTMAN->set_ambient_light(6, 6, 6);
	c->Render(the_camera, RF_FILL);

	Vector	pos	=cpos;

	RPVertex box_line[2];
	memset(box_line, 0, sizeof(RPVertex) * 2);
	box_line[0].r = box_line[1].r =
	box_line[0].g = box_line[1].g =
	box_line[0].b = box_line[1].b = 69;

	box_line[0].pos.y = pos.y - 0.5f;
	box_line[0].pos.x = pos.x - 0.5f;
	box_line[0].pos.z = pos.z;

	box_line[1].pos.y = pos.y - 0.5f;
	box_line[1].pos.x = pos.x + 0.5f;
	box_line[1].pos.z = pos.z;

	m_BATCH->draw_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, box_line, 2, 0);

	box_line[0].pos.x = pos.x + 0.5f;
	box_line[0].pos.y = pos.y + 0.5f;
	m_BATCH->draw_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, box_line, 2, 0);

	box_line[1].pos.x = pos.x - 0.5f;
	box_line[1].pos.y = pos.y + 0.5f;
	m_BATCH->draw_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, box_line, 2, 0);

	box_line[0].pos.x = pos.x - 0.5f;
	box_line[0].pos.y = pos.y - 0.5f;
	m_BATCH->draw_primitive(D3DPT_LINELIST, D3DFVF_RPVERTEX, box_line, 2, 0);
/*
	while (node)
	{
		obj = node->obj;

	// don't really want to reset modelview for every object.
		m_BATCH->set_modelview(camx_inv);
		obj->Render(the_camera, RF_FILL);
		node = node->next;

		//m_BATCH->flush(RPR_OPAQUE);
	}
  */
// subsequent calls go straight throught to render pipeline.
//	m_BATCH->set_state(RPR_BATCH, FALSE);

//	m_BATCH->set_modelview(camx_inv);

	m_BATCH->flush(RPR_OPAQUE);


// Done drawing
	m_BATCH->end_scene();
	m_PIPE->swap_buffers();
}

void	SequenceApp::SetStatusText(char *txt)
{
	assert(txt);

	((MainFrame *)m_pMainWnd)->SetStatusText(txt);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void SequenceApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void SequenceApp::SetNoTexture(void)
{
	m_BATCH->set_texture_stage_state(0, D3DTSS_COLOROP,			D3DTOP_DISABLE);

	//ValidateTextureBlend();
}

BOOL	SequenceApp::GetPerspectiveViewport(int & w, int & h) const
{
	BOOL	result	=0;

	if(m_pMainWnd)
	{
/*		CView	*p	=m_pMainWnd->GetPerspectiveView();

		if(p)
		{
			RECT	rect;

			p->GetClientRect(&rect);

			w = rect.right - rect.left + 1;
			h = rect.bottom - rect.top + 1;

			result	=TRUE;
		}*/
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////
// SequenceApp message handlers

/*
BOOL SequenceApp::DrawPreview(LONG lCount) 
{
	static	BOOL	walkin		=FALSE;
	static	BOOL	crouchin	=FALSE;
	static	BOOL	stoppin		=FALSE;

	if(CurChar)
	{
		static	float	timey	=0;

		timey	+=0.69f;
		CurChar->Update(0.269f);
		game_camera->Update(0.269f);
		m_ENG->update(0.0269f);

		if(timey > 120.0f)
		{
			if(!walkin)
			{
				CurChar->command(CT_WALK);
				walkin	=TRUE;
			}
		}
		if(timey > 696.0f)
		{
			if(!stoppin)
			{
				CurChar->command(CT_STOP);
				stoppin	=TRUE;
			}
		}
		if(timey > 1196.0f)
		{
			if(!crouchin)
			{
				CurChar->command(CT_TOGGLE_SPECIAL);
				crouchin	=TRUE;
			}
		}
		if(timey > 1300)
		{
			timey	=0;
			crouchin	=stoppin	=walkin	=FALSE;
		}
//		CurChar->Update(lCount);
//		CurChar->Update(timey);
//		CurChar->Update(69);
//		m_ENG->update_instance(CurChar->deform->get_root(), 69);
//		game_camera->Update(lCount);
//		game_camera->Update(timey);
//		game_camera->Update(69);
		
		EngineMainDraw(CurChar);
	}
	
	return CWinApp::OnIdle(lCount);
}
*/