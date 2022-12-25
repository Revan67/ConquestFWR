//
// RenderWin.cpp - Widget for performing DACOM rendering.
//

//
// Design Notes:
//     This module is designed to be used both from C++ and from the scripting language.
// Both default to a single scene, multi-view rendering in a single TopLevel widget.  All rendering
// operations must go through the functional interface, but the Widget is available for
// standard widget manipulation.
//     The functional interface and the widget are exported to the scripting language.
//

//
// Include files
//

#include <windows.h>

#include <rendpipeline.h>
#include <engine.h>
#include <basecam.h>
#include <filesys.h>
#include <ianim.h>
#include <lightman.h>
#include <timer.h>
#include <fdump.h>
#include <tempstr.h>
#include <IProperties.h>
#include <davariant.h>
#include <fontimage.h>
#include <bigimage.h>
#include <iimagesource.h>
#include <irenderprimitive.h>
#include <itexturelibrary.h>

#include <stdio.h>

#include "stdwidget.h"
#include "script.h"
#include "unilist.h"
#include "unitool.h"
#include "psys.h"

//
// Imported Variables
//

extern ICOManager *      DACOM;
extern IRenderPipeline * PIPE;
extern IEngine *         ENGINE;
extern ITextureLibrary * TLIB;
extern IAnimation *		 ANIM;
extern ILightManager *   LIGHT;
extern IRenderPrimitive *PRIM;
extern IProperties*      PROPERTIES;

extern SINGLE            frameTime;
extern SINGLE            frameRate; // in unitool.cpp

extern int               fontTag;  // in dastuff.cpp

//
// Class and structure definitions
//

// Images overlayed on the screen
struct OverlayImage
{
protected:
	COMPTR<IBigImage> image;

public:
	float             alpha;
	Vector            color;
	RECT              srcRect;
	RECT              destRect;

public:
	OverlayImage (IImageSource *imageSrc);
	OverlayImage (U8 *bits, PixelFormat *srcPf, int width, int height, int stride);

	void render ();
};

// The render widget is a normal top level with special drawing code.
#define RenderWidgetAncestor TopLevelWidget
struct RenderWidget : public RenderWidgetAncestor
{
protected:
	UniList<INSTANCE_INDEX> stuff;  // the list of things to render.
	UniList<ICamera *>      camera; // the list of cameras to render.
	UniList<OverlayImage *> overlay;// the list of overlays to render.
	UniList<IParticleSystem *> psys;// the list of particle systems to render.
	Vector ambientLight;
	Vector clearColor;

protected:
	void render_cam (ICamera *cam);  // render the entire world using the given camera

public:
	RenderWidget ();
	~RenderWidget ();

	// Rendering functions
	void render ();  // renders the entire world using each of the cameras.

	// Updating functions
	void update (SINGLE dt);

	// Database manipulation functions.
	void add_camera (ICamera *cam);
	void del_camera (ICamera *cam);
	void add_stuff (INSTANCE_INDEX index);
	void del_stuff (INSTANCE_INDEX index);
	void add_overlay (OverlayImage *image);
	void del_overlay (OverlayImage *image);
	void add_psys (IParticleSystem *_psys);
	void del_psys (IParticleSystem *_psys);

	// Light manipulation methods
	void set_ambient (Vector &v);
	void set_clear_color (Vector &v);

	// BaseWidget overloads
	virtual LRESULT on_create (LPCREATESTRUCT lpcs); // WM_CREATE
	virtual bool on_destroy (); // WM_DESTROY
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_paint (HDC hdc); // WM_PAINT
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND
};

//
// Local variables
//

static RenderWidget render_widget;
static bool render_created = false;

static int RENDEROBJECT_TAG;
static int RENDERCAMERA_TAG;
static int DACAMERA_TAG;
static int DAVECTOR_TAG;
static int DAXFORM_TAG;
static int VECTOR_TAG;
static int XFORM_TAG;
static int RENDERSCRIPT_TAG;
static int OVERLAY_TAG;

static char *RO_INST_INDEX = "instance_index";
static char *RO_ANIM_INDEX = "anim";
static char *RS_INST_INDEX = "instance";
static char *RS_NAME_INDEX = "name";
static char *RS_DURATION_INDEX = "duration";
static char *RC_INDEX = "dacamera";

//
// Methods
//

RenderWidget::RenderWidget ()
{
}

RenderWidget::~RenderWidget ()
{
}

void RenderWidget::render_cam (ICamera *cam)
{
	// Render each instance in the world via this camera.

	if (ENGINE && PIPE && LIGHT)
	{
		const ViewRect *pane = cam->get_pane();

		LIGHT->update_lighting (cam);

		PIPE->set_pipeline_state(RP_CLEAR_COLOR, D3DRGB(clearColor.x, clearColor.y, clearColor.z));
		PIPE->set_viewport (pane->x0, pane->y0, pane->x1 - pane->x0 + 1, pane->y1 - pane->y0 + 1);

		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);

		Transform mv(0);
		mv = cam->get_transform();

		PIPE->set_modelview( mv.get_inverse() );
		PIPE->set_perspective( cam->get_fovy(), cam->get_aspect(), cam->get_znear(), cam->get_zfar() );

		{
			UniNode<INSTANCE_INDEX>* node = stuff.get_head();
			
			while (node)
			{
				ENGINE->render_instance (cam, node->object, 0, 1.0, RF_RELATIVE_LOD, NULL);
				if (!stuff.traverse (node))
				{
					// Not sure if node is NULL after a failed traverse, so break here.
					break;
				}
			}
		}

		// Render the particle list.
		{
			UniNode<IParticleSystem *>* node = psys.get_head();
			
			while (node)
			{
				node->object->render (PRIM, cam);
				if (!psys.traverse (node))
				{
					// Not sure if node is NULL after a failed traverse, so break here.
					break;
				}
			}
		}
	}
}

void RenderWidget::render ()
{
	// Set the light value to its default: full on ambient
	LIGHT->set_ambient_light ((int) ambientLight.x, (int) ambientLight.y, (int) ambientLight.z);

	// Render the world from each camera's viewpoint.
	bool didRender = false;
	{
		UniNode<ICamera *>* node = camera.get_head();

		while (node)
		{
			didRender = true;
			render_cam(node->object);

			if (!camera.traverse (node))
			{
				// Not sure if node is NULL after a failed traverse, so break here.
				break;
			}
		}
	}

	{
		// Set ortho mode before proceeding.
		RECT r;
		GetClientRect (hBaseWnd, &r);

		PIPE->set_viewport (0, 0, r.right, r.bottom);
		Transform mv;
		PIPE->set_modelview (mv);
		PIPE->set_ortho (0, r.right, r.bottom, 0);

		PIPE->set_render_state (D3DRS_ALPHABLENDENABLE, FALSE);
//		PIPE->set_render_state (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
//		PIPE->set_render_state (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		UniNode<OverlayImage *>* node = overlay.get_head();

		while (node)
		{
			didRender = true;
			(node->object)->render();

			if (!overlay.traverse (node))
			{
				// Not sure if node is NULL after a failed traverse, so break here.
				break;
			}
		}
	}

	if (!didRender)
	{
		// To prevent garbage from being rendered, set the viewport to the client rect and
		// clear the buffers.

		RECT rc;
		GetClientRect (hBaseWnd, &rc);
		PIPE->set_pipeline_state(RP_CLEAR_COLOR, D3DRGB(clearColor.x, clearColor.y, clearColor.z));
		PIPE->set_viewport (0, 0, rc.right, rc.bottom);
		PIPE->begin_scene();
		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);
		PIPE->end_scene();
	}
}

void RenderWidget::update (SINGLE dt)
{
	// Update each of the objects
	{
		UniNode<INSTANCE_INDEX>* node = stuff.get_head();
		
		while (node)
		{
			ENGINE->update_instance (node->object, dt, 0);
			if (!stuff.traverse (node))
			{
				// Not sure if node is NULL after a failed traverse, so break here.
				break;
			}
		}
	}

	// Update each of the particle systems in the list.

	{
		UniNode<IParticleSystem *>* node = psys.get_head();
	
		while (node)
		{
			node->object->update (dt);
			if (!psys.traverse (node))
			{
				// Not sure if node is NULL after a failed traverse, so break here.
				break;
			}
		}
	}
}

void RenderWidget::add_camera (ICamera *cam)
{
	camera.insert_after (camera.get_head(), cam);
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::del_camera (ICamera *cam)
{
	// *** TODO: Find the camera in the list and remove it.
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}
																			  
void RenderWidget::add_stuff (INSTANCE_INDEX index)
{
	stuff.insert_after (stuff.get_head(), index);
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::del_stuff (INSTANCE_INDEX index)
{
	// *** TODO: Find the instance in the list and remove it.
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::add_overlay (OverlayImage *image)
{
	overlay.insert_after (overlay.get_head(), image);
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::del_overlay (OverlayImage *image)
{
	// *** TODO: Find the overlay in the list and remove it.
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::add_psys (IParticleSystem *_psys)
{
	psys.insert_after (psys.get_head(), _psys);
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::del_psys (IParticleSystem *_psys)
{
	// *** TODO: Find the overlay in the list and remove it.
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::set_ambient (Vector &v)
{
	ambientLight.x = max (0.0, min(v.x, 255.0));
	ambientLight.y = max (0.0, min(v.y, 255.0));
	ambientLight.z = max (0.0, min(v.z, 255.0));
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

void RenderWidget::set_clear_color (Vector &v)
{
	clearColor.x = max (0.0, min(v.x, 1.0));
	clearColor.y = max (0.0, min(v.y, 1.0));
	clearColor.z = max (0.0, min(v.z, 1.0));
	if (hBaseWnd)
	{
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}
}

// BaseWidget overloads

LRESULT RenderWidget::on_create (LPCREATESTRUCT lpcs)
{
	// Do nothing for now
	return 0;
}

bool RenderWidget::on_destroy ()
{
	// Destroy the pipeline buffers.
	if (PIPE)
	{
		PIPE->destroy_buffers();
	}

	// Let the default processing continue.
	return false;
}

bool RenderWidget::on_erasebkgnd (HDC hdc)
{
	// Pretend that we erased the background, since we will be drawing
	// the entire screen anyway.
	return true;
}

bool RenderWidget::on_paint (HDC hdc)
{
	RECT r;
	if (PIPE && GetUpdateRect(hBaseWnd, &r, FALSE))
	{
		// Render the image and flip it.
		// NOTE: The begin and end scenes are here so that other rendering
		// other than the render() call can occur here.

		PAINTSTRUCT ps;
		BeginPaint (hBaseWnd, &ps);

		PIPE->begin_scene();
		render ();
		PIPE->end_scene();
		PIPE->swap_buffers();

		// If the frame rate needs to be displayed, display it.
	
		TEXTMETRIC tm;
		if (GetTextMetrics (ps.hdc, &tm))
		{
			static char buffer[1024];

			int len = sprintf (buffer, "%f fps", frameRate);
			SIZE s = get_size ();
			SetTextColor (ps.hdc, RGB(255,255,255));
			SetBkMode (ps.hdc, TRANSPARENT);
			TextOut (ps.hdc, 0, s.cy - tm.tmHeight, buffer, len);
		}
		EndPaint (hBaseWnd, &ps);
	}
	else
	{
		// No paint this time.
		printf ("Skipping paint.\n");
	}

	return true;
}

bool RenderWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	if (PIPE)
	{
		RECT r;
		GetClientRect (hBaseWnd, &r);

		// Destroy the pipeline buffers and create them anew.
		GENRESULT genResult;

		genResult = PIPE->destroy_buffers();
		genResult = PIPE->create_buffers(hBaseWnd, r.right, r.bottom);
		InvalidateRect (hBaseWnd, NULL, FALSE);
	}

	// Do the inherited behavior
	return RenderWidgetAncestor::on_size (fwSizeType, nWidth, nHeight);
}

// OverlayImage methods
OverlayImage::OverlayImage (IImageSource *imageSrc)
{
	// Create a new IBigImage from this image source.
	
	PixelFormat pf (16,5,5,5,1);
	BIGIMAGEDESC idesc (PIPE, imageSrc, &pf);
	if (DACOM->CreateInstance (&idesc, image) != GR_OK)
	{
		GENERAL_ERROR ("Failed to create a BigImage from an image source!\n");
	}

	color = Vector (1.0f, 1.0f, 1.0f);
	alpha = 1.0f;

	U32 w, h;
	if (imageSrc->GetDimensions (w, h) == GR_OK)
	{
		srcRect.left = 0;
		srcRect.right = w;
		srcRect.top = 0;
		srcRect.bottom = h;
	}
	else
	{
		GENERAL_ERROR ("Failed to get image source dimensions!\n");
		srcRect.top = srcRect.left = srcRect.right = srcRect.bottom = 0;
		destRect.top = destRect.left = destRect.right = destRect.bottom = 0;
	}

	destRect = srcRect;

	destRect.top += 200;
	destRect.bottom += 200;
	destRect.left += 50;
	destRect.right += 50;
}

OverlayImage::OverlayImage (U8 *bits, PixelFormat *srcPf, int width, int height, int stride)
{
	// Create a new IBigImage from this image source.
	
	PixelFormat pf (16,5,5,5,1);
	BIGIMAGEDESC idesc (PIPE, &pf, bits, srcPf, width, height, stride);
	if (DACOM->CreateInstance (&idesc, image) != GR_OK)
	{
		GENERAL_ERROR ("Failed to create a BigImage from an image source!\n");
	}

	color = Vector (1.0f, 1.0f, 1.0f);
	alpha = 1.0f;

	image->SetColor (&color);
	image->SetAlpha (alpha);

	srcRect.left = 0;
	srcRect.right = width;
	srcRect.top = 0;
	srcRect.bottom = height;

	destRect = srcRect;

	destRect.top += 200;
	destRect.bottom += 200;
	destRect.left += 50;
	destRect.right += 50;

#if 0
	int midSrcX = (srcRect.left + srcRect.right)/2;
	int midSrcY = (srcRect.top + srcRect.bottom)/2;

	srcRect.top = 100;
	srcRect.bottom = 350;
	srcRect.right = 300;
	srcRect.left = 100;
#endif

}

void OverlayImage::render ()
{
	// Render the image.
	// NOTE: This assumes that we are in ortho mode.
	if (image)
	{
//		image->RenderRects (&srcRect, &destRect);
		image->RenderRects (NULL, &destRect);
#if 0
		Vector pnts[4];
		pnts[0].set( destRect.left , destRect.top, 0);
		pnts[1].set( destRect.right, destRect.top, 0 );
		pnts[3].set( destRect.right, destRect.bottom, 0 );
		pnts[2].set( destRect.left , destRect.bottom, 0 );
		pnts[0].set(  0,  240, 0 );
		pnts[1].set( 320,   0, 0 );
		pnts[2].set( 320, 480, 0 );
		pnts[3].set( 640, 240, 0 );
		image->RenderSquare( &srcRect, pnts);
#endif
	}
}

//
// Local functions
//

//
// Global functions
//

// renderwindow API
bool create_renderwindow(int x, int y, int w, int h, const char *title)
{
	// If not already created, creates the window and returns true.
	// Otherwise, returns false.

	if (render_created)
	{
		return false;
	}

	render_created = render_widget.create (x, y, w, h, title);
	return render_created;
}

IWidget *get_renderwindow ()
{
	return &render_widget;
}

void destroy_renderwindow ()
{
	// NOTE: Just because the widget is destroyed, it doesn't mean that the instances go away.
	HWND hWnd = render_widget.get_hwnd();
	if (hWnd)
	{
		DestroyWindow (hWnd);
	}
}

bool create_object (const char *name, INSTANCE_INDEX &idx, SCRIPT_SET_ARCH &scriptArch)
{
	idx = INVALID_INSTANCE_INDEX;
	scriptArch = INVALID_SCRIPT_SET_ARCH;
	if (ENGINE && TLIB && ANIM)
	{
		COMPTR<IFileSystem> IFS = NULL;

		if( name == NULL ) {
			return false;
		}

		Timer tm1, tm2, tm3, tm4;
		tm1.begin();
		tm2.begin();

		if( ENGINE->create_file_system( name, IFS ) != GR_OK ) {
			return false;
		}
		tm2.end ();
			
		tm3.begin();		
		TLIB->load_library( IFS, NULL );
		tm3.end();		
			
		tm4.begin();
		idx = ENGINE->create_instance( name, IFS, NULL );
		tm4.end();
		tm1.end();

		printf ("ENGINE->create_file_system(): %f secs\n", tm2.deltaSecs());
		printf ("TLIB->load_library(): %f secs\n", tm3.deltaSecs());
		printf ("ENGINE->create_instance(): %f secs\n", tm4.deltaSecs());
		printf ("Total time: %f secs\n", tm1.deltaSecs());

		scriptArch = ANIM->create_script_set_arch (IFS);

		if (idx != INVALID_INSTANCE_INDEX)
		{
			Transform I;
			ENGINE->set_transform (idx, I);

			// Add it to the render widget.
			render_widget.add_stuff (idx);

			// HACK: Set the texture handle to 0 to force all textures in the object to work right.
//			PIPE->set_render_state (D3DRS_TEXTUREHANDLE, 0);

			// All is well. Return success.
			return true;
		}
	}

	return false;
}

void destroy_object (INSTANCE_INDEX idx)
{
	if (ENGINE)
	{
		if (idx != INVALID_INSTANCE_INDEX)
		{
			render_widget.del_stuff (idx);
			ENGINE->destroy_instance (idx);
		}
	}
}

ICamera *create_camera (int x, int y, int w, int h)
{
	if (ENGINE)
	{
		ViewRect p;
		p.x0 = x;
		p.y0 = y;
		p.x1 = x + w - 1;
		p.y1 = y + h - 1;
		BaseCamera *cam = new BaseCamera (ENGINE, &p);

		if (cam)
		{
			// For now, force the camera to a default position.
			cam->set_position (Vector (0,0,10));

			// Add this camera to the list that gets rendered.
			render_widget.add_camera (cam);
		}
		return cam;
	}
	return NULL;
}

void destroy_camera (ICamera *cam)
{
	render_widget.del_camera (cam);
	delete (BaseCamera *) cam;
}

OverlayImage *create_overlay (IImageSource *imageSrc)
{
	OverlayImage *ovi = new OverlayImage (imageSrc);
	render_widget.add_overlay (ovi);
	return ovi;
}

OverlayImage *create_overlay (U8 *bits, PixelFormat *srcPf, int width, int height, int stride)
{
	OverlayImage *ovi = new OverlayImage (bits, srcPf, width, height, stride);
	render_widget.add_overlay (ovi);
	return ovi;
}

void destroy_overlay (OverlayImage *ovi)
{
	render_widget.del_overlay (ovi);
	delete ovi;
}

void add_psys (IParticleSystem *_psys)
{
	render_widget.add_psys (_psys);
}

void del_psys (IParticleSystem *_psys)
{
	render_widget.del_psys (_psys);
}

// =========== Scripting API =============

// *** Math ***

static char *DAV_INDEX = "davec";
static char *DAX_INDEX = "daxform";

void push_vector (Vector *v)
{
	lua_Object vt = lua_createtable();
	lua_pushobject (vt);
	lua_pushstring (DAV_INDEX);
	lua_pushusertag ((void *) v, DAVECTOR_TAG);
	lua_rawsettable ();

	lua_pushobject (vt);
	lua_settag (VECTOR_TAG);

	lua_pushobject (vt);
}

void push_vector (const Vector &v)
{
	Vector *nv = new Vector(v);
	push_vector (nv);
}

Vector *get_vector(lua_Object obj)
{
	if (lua_tag(obj) == VECTOR_TAG)
	{
		lua_pushobject (obj);
		lua_pushstring (DAV_INDEX);
		lua_Object davec = lua_rawgettable();
		if (lua_tag(davec) == DAVECTOR_TAG)
		{
			return (Vector *) lua_getuserdata(davec);
		}
	}

	return NULL;
}

void push_xform (Transform *t)
{
	lua_Object xt = lua_createtable();
	lua_pushobject (xt);
	lua_pushstring (DAX_INDEX);
	lua_pushusertag ((void *) t, DAXFORM_TAG);
	lua_rawsettable ();

	lua_pushobject (xt);
	lua_settag (XFORM_TAG);

	lua_pushobject (xt);
}

void push_xform (Transform &t)
{
	Transform *nt = new Transform(t);
	push_xform (nt);
}

Transform *get_xform (lua_Object obj)
{
	if (lua_tag(obj) == XFORM_TAG)
	{
		lua_pushobject (obj);
		lua_pushstring (DAX_INDEX);
		lua_Object daxform = lua_rawgettable();
		if (lua_tag(daxform) == DAXFORM_TAG)
		{
			return (Transform *) lua_getuserdata(daxform);
		}
	}

	return NULL;
}

void add_vector (void)
{
	Vector *a = get_vector (lua_getparam(1));
	Vector *b = get_vector (lua_getparam(2));
	if (a && b)
	{
		push_vector(*a + *b);
	}
}

void sub_vector (void)
{
	Vector *a = get_vector (lua_getparam(1));
	Vector *b = get_vector (lua_getparam(2));
	if (a && b)
	{
		push_vector(*a - *b);
	}
}

void mul_vector (void)
{
	lua_Object a = lua_getparam(1);
	lua_Object b = lua_getparam(2);
	if (lua_tag(a) == VECTOR_TAG)
	{
		if (lua_tag(b) == VECTOR_TAG)
		{
			// * is dot product
			lua_pushnumber
			(
				dot_product(*get_vector(a), *get_vector(b))
			);
		}
		else if (lua_tag(b) == XFORM_TAG)
		{
			push_vector
			(
				*get_vector(a) * (*get_xform(b))
			);
		}
		else if (lua_isnumber(b))
		{
			push_vector
			(
				*get_vector(a) * lua_getnumber(b)
			);
		}
	}
	else if (lua_tag(b) == VECTOR_TAG)
	{
		if (lua_isnumber (a))	
		{
			push_vector
			(
				lua_getnumber(a) * (*get_vector(b))
			);
		}
	}
}

void div_vector (void)
{
	lua_Object a = lua_getparam(1);
	lua_Object b = lua_getparam(2);
	if (lua_tag(a) == VECTOR_TAG)
	{
		if (lua_tag(b) == VECTOR_TAG)
		{
			// / is cross product
			push_vector
			(
				cross_product(*get_vector(a), *get_vector(b))
			);
		}
		else if (lua_isnumber(b))
		{
			push_vector
			(
				*get_vector(a) / lua_getnumber(b)
			);
		}
	}
}

void unm_vector (void)
{
	Vector *a = get_vector(lua_getparam(1));
	if (a)
	{
		push_vector (-(*a));
	}
}

void gettable_vector (void)
{
	lua_Object vec = lua_getparam(1);
	lua_Object index = lua_getparam(2);

	if (lua_isstring (index))
	{
		Vector *v = get_vector (vec);
		if (v)
		{
			const char *indexStr = lua_getstring(index);
			switch (*indexStr)
			{
			case 'x':
			case 'X':
			case 'i':
			case 'I':
			case 'u':
			case 'U':
				lua_pushnumber (v->x);
				return;
				break;

			case 'y':
			case 'Y':
			case 'j':
			case 'J':
			case 'v':
			case 'V':
				lua_pushnumber (v->y);
				return;
				break;

			case 'z':
			case 'Z':
			case 'k':
			case 'K':
			case 'w':
			case 'W':
				lua_pushnumber (v->z);
				return;
				break;
			}
		}
	}
}

void settable_vector (void)
{
	lua_Object vec = lua_getparam(1);
	lua_Object index = lua_getparam(2);
	lua_Object value = lua_getparam(3);

	if (lua_isstring (index) && lua_isnumber(value))
	{
		Vector *v = get_vector (vec);
		if (v)
		{
			const char *indexStr = lua_getstring(index);
			switch (*indexStr)
			{
			case 'x':
			case 'X':
			case 'i':
			case 'I':
			case 'u':
			case 'U':
				v->x = lua_getnumber (value);
				break;

			case 'y':
			case 'Y':
			case 'j':
			case 'J':
			case 'v':
			case 'V':
				v->y = lua_getnumber (value);
				break;

			case 'z':
			case 'Z':
			case 'k':
			case 'K':
			case 'w':
			case 'W':
				v->z = lua_getnumber (value);
				break;
			}
		}
	}
}

void gc_davector (void)
{
	lua_Object a = lua_getparam(1);
	if (lua_tag(a) == DAVECTOR_TAG)
	{
		delete (Vector *) lua_getuserdata(a);
	}
}

void newVector (void)
{
	lua_Object x = lua_getparam(1);
	lua_Object y = lua_getparam(2);
	lua_Object z = lua_getparam(3);

	if (y == LUA_NOOBJECT && z == LUA_NOOBJECT)
	{
		Vector *v = get_vector(x);
		if (v)
		{
			push_vector (*v);
		}
	}
	else
	{
		push_vector (new Vector(lua_getnumber(x), lua_getnumber(y), lua_getnumber(z)));
	}
}

void add_xform (void)
{
	Transform *a = get_xform (lua_getparam(1));
	Transform *b = get_xform (lua_getparam(1));
	if (a && b)
	{
		push_xform(*a + *b);
	}
}

void sub_xform (void)
{
	Transform *a = get_xform (lua_getparam(1));
	Transform *b = get_xform (lua_getparam(1));
	if (a && b)
	{
		push_xform(*a - *b);
	}
}

void mul_xform (void)
{
	lua_Object a = lua_getparam(1);
	lua_Object b = lua_getparam(2);
	if (lua_tag(a) == XFORM_TAG)
	{
		if (lua_tag(b) == XFORM_TAG)
		{
			push_xform
			(
				*get_xform(a) * (*get_xform(b))
			);
		}
		else if (lua_tag(b) == VECTOR_TAG)
		{
			push_vector
			(
				*get_xform(a) * (*get_vector(b))
			);
		}
	}
	else if (lua_tag(b) == XFORM_TAG)
	{
		if (lua_tag(a) == VECTOR_TAG)
		{
			push_vector
			(
				*get_vector(a) * (*get_xform(b))
			);
		}
	}
}

void gc_daxform (void)
{
	lua_Object a = lua_getparam(1);
	if (lua_tag(a) == DAXFORM_TAG)
	{
		delete (Transform *) lua_getuserdata(a);
	}
}

void newXform (void)
{
	lua_Object a = lua_getparam(1);
	lua_Object b = lua_getparam(2);
	lua_Object c = lua_getparam(3);

	if (lua_tag(a) == VECTOR_TAG)
	{
		if (b == LUA_NOOBJECT && c == LUA_NOOBJECT)
		{
			// Pure translation
			Vector *va = get_vector (a);
			push_xform (new Transform (*va));
		}
		else if (lua_tag(b) == VECTOR_TAG && lua_tag(c) == VECTOR_TAG)
		{
			// Three columns form
			Vector *va = get_vector (a);
			Vector *vb = get_vector (b);
			Vector *vc = get_vector (c);
			push_xform (new Transform (*va, *vb, *vc));
		}
	}
	else if (lua_isnumber(a) && lua_isnumber(b) && lua_isnumber(c))
	{
		// Pitch, Roll, Yaw form.
		Transform *t = new Transform();
		t->set_orientation (lua_getnumber(a), lua_getnumber(b), lua_getnumber(c));
		t->set_position (0, 0, 0);
		push_xform (t);
	}
	else if (lua_tag(a) == XFORM_TAG && b == LUA_NOOBJECT && c == LUA_NOOBJECT)
	{
		// Create a new transform initialized by the passed in transform.
		Transform *t = get_xform(a);
		if (t)
		{
			push_xform (*t);
		}
	}
}

// *** RenderWindow ***
void testModeSwitch (void)
{
	// Test the mode switch by going fullscreen and back to windowed mode.
	PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, true);
	PIPE->create_buffers(hWndMain, 640, 480);
	PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, false);
	render_widget.on_size (0, 0, 0);
}

void setAmbient (void)
{
	lua_Object widget = lua_getparam(1);
	lua_Object level = lua_getparam(2);

	if (lua_isnumber(level))
	{
		float val = lua_getnumber(level);
		Vector v(val, val, val);
		render_widget.set_ambient (v);
	}
	else if (lua_tag(level) == VECTOR_TAG)
	{
		Vector *v = get_vector(level);
		if (v)
		{
			render_widget.set_ambient (*v);
		}
	}
}

void setClearColor (void)
{
	lua_Object widget = lua_getparam(1);
	lua_Object level = lua_getparam(2);
	if (lua_isnumber(level))
	{
		float val = lua_getnumber(level);
		Vector v(val, val, val);
		render_widget.set_clear_color (v);
	}
	else if (lua_tag(level) == VECTOR_TAG)
	{
		Vector *v = get_vector(level);
		if (v)
		{
			render_widget.set_clear_color (*v);
		}
	}
}

void refreshRenderWindow (void)
{
	extern bool dastuff_update ();
	dastuff_update ();
#if 0
	ENGINE->update (0.);

	// Force an update of the render window.
	InvalidateRect (render_widget.get_hwnd(), NULL, FALSE);
	UpdateWindow (render_widget.get_hwnd());
#endif
}

void createRender (void)
{
	// Syntax: 
	//     CreateRender
	//     (
	//         string <title>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>
	//     )
	// Creates the render window, if not already created, and exports it as "RenderWindow"
	// <title> is the name of the window.
	// <xpos>,<ypos> are the location of the button in parent coordinates.
	// <width>,<height> are the width and height of the window

	// Get and validate the parameters
	lua_Object title = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(title))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	// The parameters are valid, so go about creating a new button.

	int x = lua_getnumber(xpos);
	int y = lua_getnumber(ypos);
	int w = lua_getnumber(width);
	int h = lua_getnumber(height);

	if (render_created)
	{
		// Already created, so adjust its parameters
		render_widget.set_size (w, h);
		render_widget.set_position (x, y);
		render_widget.set_text (lua_getstring(title));

		// DO NOT PUSH THE WIDGET HERE!
		// Doing so will create a new object, which is not what we want.
		// The right method is below: to get the global instance value 
	}
	else
	{
		// Not created yet, so create it.
		if (!create_renderwindow (x, y, w, h, lua_getstring(title)))
		{
			return;
		}
		export_widget (&render_widget);

		// Add some functions to the render window by default.
		lua_Object rwo = lua_pop ();
		lua_pushobject (rwo);
		lua_pushstring ("set_ambient");
		lua_pushcfunction (setAmbient);
		lua_rawsettable ();

		lua_pushobject (rwo);
		lua_pushstring ("set_clear_color");
		lua_pushcfunction (setClearColor);
		lua_rawsettable ();

		lua_pushobject (rwo);
		lua_pushstring ("refresh");
		lua_pushcfunction (refreshRenderWindow);
		lua_rawsettable ();

		lua_pushobject (rwo);
		lua_setglobal ("RenderWindow");
	}

	// Also return the RenderWindow object.
	lua_pushobject (lua_getglobal("RenderWindow"));
	return;
}

void destroyRender (void)
{
	destroy_renderwindow();
}

// *** Render Objects ***

INSTANCE_INDEX get_object_instance (lua_Object obj)
{
	if (!lua_istable(obj) || lua_tag(obj) != RENDEROBJECT_TAG)
	{
		return INVALID_INSTANCE_INDEX;
	}
	lua_pushobject (obj);
	lua_pushstring (RO_INST_INDEX);
	lua_Object instance = lua_rawgettable ();
	if (!lua_isnumber(instance))
	{
		return INVALID_INSTANCE_INDEX;
	}
	else
	{
		return (int) lua_getnumber(instance);
	}
}

SCRIPT_INST get_script_instance (lua_Object obj)
{
	// Scripts have two fields: instance, and name
	if (!lua_istable(obj) || lua_tag(obj) != RENDERSCRIPT_TAG)
	{
		return INVALID_SCRIPT_INST;
	}
	lua_pushobject (obj);
	lua_pushstring (RS_INST_INDEX);
	lua_Object scripts = lua_rawgettable ();
	if (!lua_isnumber(scripts))
	{
		return INVALID_SCRIPT_INST;
	}
	else
	{
		return (int) lua_getnumber(scripts);
	}
}

static void startScript (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_script_instance().
	lua_Object scriptInst = lua_getparam(1);  // the script instance
	lua_Object scriptTable = lua_getparam(2); // the 'this' pointer, ignored for now.
	if (lua_isnumber(scriptInst))
	{
		ANIM->script_start ((SCRIPT_INST) lua_getnumber(scriptInst));
	}
}

static void stopScript (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_script_instance().
	lua_Object scriptInst = lua_getparam(1);  // the script instance
	lua_Object scriptTable = lua_getparam(2); // the 'this' pointer, ignored for now.
	if (lua_isnumber(scriptInst))
	{
		ANIM->script_stop ((SCRIPT_INST) lua_getnumber(scriptInst));
	}
}

static void getScriptTime (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_script_instance().
	lua_Object scriptInst = lua_getparam(1);  // the script instance
	lua_Object scriptTable = lua_getparam(2); // the 'this' pointer, ignored for now.
	if (lua_isnumber(scriptInst))
	{
		lua_pushnumber(ANIM->get_current_time ((SCRIPT_INST) lua_getnumber(scriptInst)));
	}
}

static void setScriptTime (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_script_instance().
	lua_Object scriptInst = lua_getparam(1);  // the script instance
	lua_Object scriptTable = lua_getparam(2); // the 'this' pointer, ignored for now.
	lua_Object time = lua_getparam(3);        // the new time to set.
	if (lua_isnumber(scriptInst))
	{
		ANIM->set_current_time ((SCRIPT_INST) lua_getnumber(scriptInst), lua_getnumber(time));
	}
}

static void getScriptDuration (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_script_instance().
	lua_Object scriptInst = lua_getparam(1);  // the script instance
	lua_Object scriptTable = lua_getparam(2); // the 'this' pointer, ignored for now.
	if (lua_isnumber(scriptInst))
	{
		lua_pushnumber(ANIM->get_duration ((SCRIPT_INST) lua_getnumber(scriptInst)));
	}
}

struct pushScriptsEnumState
{
	INSTANCE_INDEX idx;
	SCRIPT_SET_ARCH scripts;
	lua_Object scriptTable;
};

static void pushScriptsEnum (const char *name, void *misc)
{
	pushScriptsEnumState *state = (pushScriptsEnumState *) misc;

	SCRIPT_INST inst = ANIM->create_script_inst (state->scripts, state->idx, name);
	if (inst != INVALID_SCRIPT_INST)
	{
		// We have a valid instance, so create a table for it with its name, duration, and instance.

		lua_Object st = lua_createtable();

		lua_pushobject (st);
		lua_pushstring (RS_INST_INDEX);
		lua_pushnumber (inst);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring (RS_NAME_INDEX);
		lua_pushstring ((char *)name);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring (RS_DURATION_INDEX);
		lua_pushnumber (ANIM->get_duration(inst));
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring ("start");
		lua_pushnumber (inst);
		lua_pushcclosure (startScript, 1);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring ("stop");
		lua_pushnumber (inst);
		lua_pushcclosure (stopScript, 1);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring ("get_time");
		lua_pushnumber (inst);
		lua_pushcclosure (getScriptTime, 1);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring ("set_time");
		lua_pushnumber (inst);
		lua_pushcclosure (setScriptTime, 1);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_pushstring ("get_duration");
		lua_pushnumber (inst);
		lua_pushcclosure (getScriptDuration, 1);
		lua_rawsettable ();

		lua_pushobject (st);
		lua_settag (RENDERSCRIPT_TAG);

		// *** TODO: Push some C functions here for starting and stopping these scripts, perhaps with
		// *** some up values.
	
		// Store this into the script table, indexed with its name.

		lua_pushobject (state->scriptTable);
		lua_pushstring ((char *) name);
		lua_pushobject (st);
		lua_rawsettable ();
	}
}

void pushScripts (INSTANCE_INDEX idx, SCRIPT_SET_ARCH scripts)
{
	// Create a table of all the script instances in the given set, indexed by name.

	pushScriptsEnumState state;
	
	state.idx = idx;
	state.scripts = scripts;
	state.scriptTable = lua_createtable();

	ANIM->enumerate_scripts ((SCRIPT_ENUM_CALLBACK) pushScriptsEnum, scripts, &state);

	// The table is now filled. Put it on the stack for other use.

	lua_pushobject (state.scriptTable);
}

static void setObjectPos (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.
	lua_Object objPos = lua_getparam(3);      // the new position, either vector or xPos
	lua_Object yPos = lua_getparam(4);        // optional parameter
	lua_Object zPos = lua_getparam(5);        // optional parameter 

	// Confirm that the new pos is valid and a vector. If not, do nothing.
	if (objPos == LUA_NOOBJECT)
	{
		return;
	}

	Vector newPos;
	Vector *v = get_vector (objPos);
	if (v)
	{
		newPos = *v;
	}
	else
	{
		// An alternative syntax is to give three numbers. Check that here.
		if (yPos == LUA_NOOBJECT || zPos == LUA_NOOBJECT)
		{
			return;
		}
		if (!lua_isnumber(yPos) || !lua_isnumber(zPos) || !lua_isnumber(objPos))
		{
			return;
		}

		newPos.x = lua_getnumber(objPos);
		newPos.y = lua_getnumber(yPos);
		newPos.z = lua_getnumber(zPos);
	}

	// Set the object's position.
	ENGINE->set_position ((INSTANCE_INDEX) lua_getnumber(objInst), newPos);
}

static void getObjectPos (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.

	// Get the object's position.
	Vector pos;
	pos = ENGINE->get_position ((INSTANCE_INDEX) lua_getnumber(objInst));

	// Return it to lua.
	push_vector (pos);
}

static void setObjectAngularVel (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.
	lua_Object objAngVel = lua_getparam(3);   // the new velocity, either vector or xAngVel
	lua_Object yAngVel = lua_getparam(4);     // optional parameter
	lua_Object zAngVel = lua_getparam(5);     // optional parameter 

	// Confirm that the new pos is valid and a vector. If not, do nothing.
	if (objAngVel == LUA_NOOBJECT)
	{
		return;
	}

	Vector newAngVel;
	Vector *v = get_vector (objAngVel);
	if (v)
	{
		newAngVel = *v;
	}
	else
	{
		// An alternative syntax is to give three numbers. Check that here.
		if (yAngVel == LUA_NOOBJECT || zAngVel == LUA_NOOBJECT)
		{
			return;
		}
		if (!lua_isnumber(yAngVel) || !lua_isnumber(zAngVel) || !lua_isnumber(objAngVel))
		{
			return;
		}

		newAngVel.x = lua_getnumber(objAngVel);
		newAngVel.y = lua_getnumber(yAngVel);
		newAngVel.z = lua_getnumber(zAngVel);
	}

	// Set the object's position.
	ENGINE->set_angular_velocity ((INSTANCE_INDEX) lua_getnumber(objInst), newAngVel);
}

static void getObjectAngularVel (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.

	// Get the object's position.
	Vector angVel;
	angVel = ENGINE->get_angular_velocity ((INSTANCE_INDEX) lua_getnumber(objInst));

	// Return it to lua.
	push_vector (angVel);
}

static void setObjectXform (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.
	lua_Object objXform = lua_getparam(3);    // the new transform

	// Confirm that the new pos is valid and a vector. If not, do nothing.
	if (objXform == LUA_NOOBJECT)
	{
		return;
	}

	Transform *newXform = get_xform (objXform);
	if (!newXform)
	{
		return;
	}

	// Set the object's position.
	ENGINE->set_transform ((INSTANCE_INDEX) lua_getnumber(objInst), *newXform);
}

static void getObjectXform (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.

	// Get the object's position.
	Transform xform;
	xform = ENGINE->get_transform ((INSTANCE_INDEX) lua_getnumber(objInst));

	// Return it to lua.
	push_xform (xform);
}

static void setObjectProperty (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.
	lua_Object objPropName = lua_getparam(3); // the property name
	lua_Object objPropValue = lua_getparam(4); // the property value

	// Confirm that the name and value are valid.
	if (objPropName == LUA_NOOBJECT || objPropValue == LUA_NOOBJECT)
	{
		return;
	}

	// Set the property
	DACOMDESC desc = "IProperty";
	COMPTR<IProperty> prop;
	if (DACOM->CreateInstance (&desc, (void **) &prop) == GR_OK)
	{
		COMPTR<ISetProperty> setProp;
		if (prop->QueryInterface (IID_ISetProperty, (void **) &setProp) == GR_OK)
		{
			if (lua_isnumber (objPropValue))
			{
				setProp->set_double (lua_getnumber (objPropValue));
			}
			else if (lua_isstring (objPropValue))
			{
				setProp->set_string (lua_getstring (objPropValue));
			}
			else
			{
				// No other values are currently supported.
				return;
			}

			// NOTE: The new property has its reference count incremented by the following call.
			PROPERTIES->set_by_name ((INSTANCE_INDEX) lua_getnumber(objInst), lua_getstring(objPropName), prop);
		}
	}
}

static void getObjectProperty (void)
{
	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.
	lua_Object objPropName = lua_getparam(3); // the property name

	// Confirm that the name and value are valid.
	if (objPropName == LUA_NOOBJECT)
	{
		return;
	}

	// Get the property

	COMPTR<IProperty> value;
	if(PROPERTIES->get_by_name ((INSTANCE_INDEX) lua_getnumber(objInst), lua_getstring(objPropName), value) == GR_OK)
	{
		PROP_TYPE type = value->get_type();
		switch (type)
		{
		case PT_STRING:
			{
				const char *str;
				value->get_string (str);
				lua_pushstring ((char *)str);
			}
			break;

		case PT_LONG:
			{
				long n;
				value->get_long(n);
				lua_pushnumber (n);
			}
			break;

		case PT_ULONG:
			{
				unsigned long n;
				value->get_ulong(n);
				lua_pushnumber (n);
			}
			break;

		case PT_SINGLE:
			{
				SINGLE n;
				value->get_single(n);
				lua_pushnumber (n);
			}
			break;

		case PT_DOUBLE:
			{
				DOUBLE n;
				value->get_double(n);
				lua_pushnumber (n);
			}
			break;
		}
	}
}

static void getObjectProperties (void)
{
	// Returns a table consisting of all the object properties that can be manipulated using LUA, namely the
	// string and number properties.

	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.

	// Get the properties

	INSTANCE_INDEX idx = (INSTANCE_INDEX) lua_getnumber(objInst);
	int count = PROPERTIES->get_count(idx);
	if (count > 0)
	{
		// Create a table to hold the property values
		lua_Object pt = lua_createtable();

		for (int i = 0; i < count; ++i)
		{
			COMPTR<IProperty> value;
			if (PROPERTIES->get_by_index (idx, i, value) == GR_OK)
			{
				// Store the value into the table with its property name
				lua_pushobject (pt);
				lua_pushstring ((char *) PROPERTIES->get_name (idx, i));

				PROP_TYPE type = value->get_type();
				switch (type)
				{
				case PT_STRING:
					{
						const char *str;
						value->get_string (str);
						lua_pushstring ((char *)str);
					}
					break;

				case PT_LONG:
					{
						long n;
						value->get_long(n);
						lua_pushnumber (n);
					}
					break;

				case PT_ULONG:
					{
						unsigned long n;
						value->get_ulong(n);
						lua_pushnumber (n);
					}
					break;

				case PT_SINGLE:
					{
						SINGLE n;
						value->get_single(n);
						lua_pushnumber (n);
					}
					break;

				case PT_DOUBLE:
					{
						DOUBLE n;
						value->get_double(n);
						lua_pushnumber (n);
					}
					break;

				default:
					// Push a string indicating that the property is not one of the types we care about.
					lua_pushstring ("[None LUA type]");
					break;
				}
				lua_rawsettable ();
			}
		}

		// Return the property table
		lua_pushobject (pt);
	}
}

static void getArchetypeProperties (void)
{
	// Returns a table consisting of all the object properties that can be manipulated using LUA, namely the
	// string and number properties.

	// NOTE: This is pushed as a C closure, therefore the first parameter is the instance
	// index. This keeps us from having to use the more cumbersome get_object_instance().
	lua_Object objInst = lua_getparam(1);     // the object instance
	lua_Object objTable = lua_getparam(2);    // the 'this' pointer, ignored for now.

	// Get the properties

	INSTANCE_INDEX iidx = (INSTANCE_INDEX) lua_getnumber(objInst);
	ARCHETYPE_INDEX idx = ENGINE->get_instance_archetype (iidx);

	int count = PROPERTIES->arch_get_count(idx);
	if (count > 0)
	{
		// Create a table to hold the property values
		lua_Object pt = lua_createtable();

		for (int i = 0; i < count; ++i)
		{
			COMPTR<IProperty> value;
			if (PROPERTIES->arch_get_by_index (idx, i, value) == GR_OK)
			{
				// Store the value into the table with its property name
				lua_pushobject (pt);
				lua_pushstring ((char *) PROPERTIES->arch_get_name (idx, i));

				PROP_TYPE type = value->get_type();
				switch (type)
				{
				case PT_STRING:
					{
						const char *str;
						value->get_string (str);
						lua_pushstring ((char *)str);
					}
					break;

				case PT_LONG:
					{
						long n;
						value->get_long(n);
						lua_pushnumber (n);
					}
					break;

				case PT_ULONG:
					{
						unsigned long n;
						value->get_ulong(n);
						lua_pushnumber (n);
					}
					break;

				case PT_SINGLE:
					{
						SINGLE n;
						value->get_single(n);
						lua_pushnumber (n);
					}
					break;

				case PT_DOUBLE:
					{
						DOUBLE n;
						value->get_double(n);
						lua_pushnumber (n);
					}
					break;

				default:
					// Push a string indicating that the property is not one of the types we care about.
					lua_pushstring ("[None LUA type]");
					break;
				}
				lua_rawsettable ();
			}
		}

		// Return the property table
		lua_pushobject (pt);
	}
}

void createObject (void)
{
	// Syntax: 
	//     NewObject(string <filename>)
	// Creates a new object from the given filename.
	// Returns a table with an "instance_index" field containing the engine index
	// of the object. Its tag is RENDEROBJECT_TAG.
	// Returns nil on failure.

	lua_Object filename = lua_getparam(1);

	if (!lua_isstring (filename))
	{
		return;
	}

	// Attempt to create the given object.

	INSTANCE_INDEX idx;
	SCRIPT_SET_ARCH scripts;
	if (!create_object (lua_getstring(filename), idx, scripts))
	{
		return;
	}

	// The index is valid, so create and export the table with the RENDEROBJECT_TAG tag.
	lua_Object ot = lua_createtable();
	
	lua_pushobject (ot);
	lua_pushstring (RO_ANIM_INDEX);
	pushScripts (idx, scripts);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring (RO_INST_INDEX);
	lua_pushnumber (idx);
	lua_rawsettable ();
	
	lua_pushobject (ot);
	lua_pushstring ("set_pos");
	lua_pushnumber (idx);
	lua_pushcclosure (setObjectPos, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_pos");
	lua_pushnumber (idx);
	lua_pushcclosure (getObjectPos, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("set_transform");
	lua_pushnumber (idx);
	lua_pushcclosure (setObjectXform, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_transform");
	lua_pushnumber (idx);
	lua_pushcclosure (getObjectXform, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("set_angular_velocity");
	lua_pushnumber (idx);
	lua_pushcclosure (setObjectAngularVel, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_angular_velocity");
	lua_pushnumber (idx);
	lua_pushcclosure (getObjectAngularVel, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("set_property");
	lua_pushnumber (idx);
	lua_pushcclosure (setObjectProperty, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_property");
	lua_pushnumber (idx);
	lua_pushcclosure (getObjectProperty, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_properties");
	lua_pushnumber (idx);
	lua_pushcclosure (getObjectProperties, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_pushstring ("get_arch_properties");
	lua_pushnumber (idx);
	lua_pushcclosure (getArchetypeProperties, 1);
	lua_rawsettable ();

	lua_pushobject (ot);
	lua_settag (RENDEROBJECT_TAG);
	
	lua_pushobject (ot);

	return;
}

void destroyObject (void)
{
	// Syntax: 
	//     DestroyObject(RenderObject <object>)
	// Destroys the given object.

	lua_Object obj = lua_getparam(1);

	INSTANCE_INDEX idx = get_object_instance (obj);
	if (idx != INVALID_INSTANCE_INDEX)
	{
		destroy_object (idx);
	}
}

// *** Render Cameras ***

ICamera *get_camera (lua_Object obj)
{
	if (!lua_istable(obj) || lua_tag(obj) != RENDERCAMERA_TAG)
	{
		return NULL;
	}
	lua_pushobject (obj);
	lua_pushstring (RC_INDEX);
	lua_Object cam = lua_rawgettable ();
	if (!lua_isuserdata(cam) || lua_tag (cam) != DACAMERA_TAG)
	{
		return NULL;
	}
	else
	{
		return (ICamera *) lua_getuserdata(cam);
	}
}

void setCameraPos (void)
{
	// NOTE: This is not intended to be a tag function. Instead, it will be pushed as a c closure with
	// the DACamera object.
	// Arguments: DACamera <cam>, RenderCamera <camTable>, Vector <newPos>
	lua_Object cam = lua_getparam(1);
	lua_Object camTable = lua_getparam(2);
	lua_Object vector = lua_getparam(3);

	if (lua_tag(cam) != DACAMERA_TAG)
	{
		return;
	}

	if (lua_tag(camTable) != RENDERCAMERA_TAG)
	{
		return;
	}

	if (lua_tag(vector) != VECTOR_TAG)
	{
		return;
	}

	// All is well, so set the new position.

	BaseCamera *camPtr = (BaseCamera *) lua_getuserdata(cam);
	Vector *v = get_vector(vector);
	camPtr->set_position (*v);

	// Ensure that the render window draws again.
	InvalidateRect (render_widget.get_hwnd(), NULL, FALSE);
	UpdateWindow (render_widget.get_hwnd());
}

void getCameraPos (void)
{
	// NOTE: This is not intended to be a tag function. Instead, it will be pushed as a c closure with
	// the DACamera object.
	// Arguments: DACamera <cam>, RenderCamera <camTable>
	lua_Object cam = lua_getparam(1);
	lua_Object camTable = lua_getparam(2);

	if (lua_tag(cam) != DACAMERA_TAG)
	{
		return;
	}

	if (lua_tag(camTable) != RENDERCAMERA_TAG)
	{
		return;
	}

	// All is well, so set the new position.

	BaseCamera *camPtr = (BaseCamera *) lua_getuserdata(cam);
	push_vector (camPtr->get_position());
}

void setCameraViewport (void)
{
	// NOTE: This is not intended to be a tag function. Instead, it will be pushed as a c closure with
	// the DACamera object.
	// Arguments: DACamera <cam>, RenderCamera <camTable>, number <x>, number <y>, number <width>, number <height>
	lua_Object cam = lua_getparam(1);
	lua_Object camTable = lua_getparam(2);
	lua_Object xpos = lua_getparam(3);
	lua_Object ypos = lua_getparam(4);
	lua_Object width = lua_getparam(5);
	lua_Object height = lua_getparam(6);

	if (lua_tag(cam) != DACAMERA_TAG)
	{
		return;
	}

	if (lua_tag(camTable) != RENDERCAMERA_TAG)
	{
		return;
	}

	if (!lua_isnumber (xpos) || !lua_isnumber (ypos) || !lua_isnumber (width)|| !lua_isnumber (height))
	{
		return;
	}

	// Set the camera's pane.
	int x = (int) lua_getnumber (xpos);
	int y = (int) lua_getnumber (ypos);
	int w = (int) lua_getnumber (width);
	int h = (int) lua_getnumber (height);
	ViewRect p;
	p.x0 = x;
	p.y0 = y;
	p.x1 = x+w-1;
	p.y1 = y+h-1;

	BaseCamera *camPtr = (BaseCamera *) lua_getuserdata(cam);
	camPtr->set_pane(&p);

	// Ensure that the render window draws again.
	InvalidateRect (render_widget.get_hwnd(), NULL, FALSE);
	UpdateWindow (render_widget.get_hwnd());
}

void getCameraViewport (void)
{
	// Returns the dimensions as seperate objects
	// NOTE: This is not intended to be a tag function. Instead, it will be pushed as a c closure with
	// the DACamera object.
	// Arguments: DACamera <cam>, RenderCamera <camTable>
	lua_Object cam = lua_getparam(1);
	lua_Object camTable = lua_getparam(2);

	if (lua_tag(cam) != DACAMERA_TAG)
	{
		return;
	}

	if (lua_tag(camTable) != RENDERCAMERA_TAG)
	{
		return;
	}

	BaseCamera *camPtr = (BaseCamera *) lua_getuserdata(cam);
	const ViewRect *p = camPtr->get_pane();

	lua_pushnumber (p->x0);
	lua_pushnumber (p->y0);
	lua_pushnumber (p->x1 - p->x0 + 1);
	lua_pushnumber (p->y1 - p->y0 + 1);
}

void createCamera (void)
{
	// Syntax: 
	//     NewCamera
	//     (
	//        number <x>, number <y>,
	//        number <width>, number <height>
	//     )
	// Creates a new camera that renders into the given viewport.
	// Returns a table with a "dacamera" field containing the camera's pointer
	// Its tag is RENDERCAMERA_TAG.
	// Returns nil on failure.

	lua_Object xpos = lua_getparam(1);
	lua_Object ypos = lua_getparam(2);
	lua_Object width = lua_getparam(3);
	lua_Object height = lua_getparam(4);

	if (!lua_isnumber (xpos) || !lua_isnumber (ypos) || !lua_isnumber (width)|| !lua_isnumber (height))
	{
		return;
	}

	// Attempt to create the camera

	int x = (int) lua_getnumber (xpos);
	int y = (int) lua_getnumber (ypos);
	int w = (int) lua_getnumber (width);
	int h = (int) lua_getnumber (height);

	ICamera *cam = create_camera (x, y, w, h);
	if (!cam)
	{
		return;
	}

	// The camera is valid, so create and export the table with the RENDERCAMERA_TAG tag.
	lua_pushusertag ((void *) cam, DACAMERA_TAG);
	lua_Object daCam = lua_pop ();

	lua_Object ct = lua_createtable();
	
	lua_pushobject (ct);
	lua_pushstring (RC_INDEX);
	lua_pushobject (daCam);
	lua_rawsettable ();

	lua_pushobject (ct);
	lua_pushstring ("set_pos");
	lua_pushobject (daCam);
	lua_pushcclosure (setCameraPos, 1);
	lua_rawsettable ();
	
	lua_pushobject (ct);
	lua_pushstring ("get_pos");
	lua_pushobject (daCam);
	lua_pushcclosure (getCameraPos, 1);
	lua_rawsettable ();
	
	lua_pushobject (ct);
	lua_pushstring ("set_viewport");
	lua_pushobject (daCam);
	lua_pushcclosure (setCameraViewport, 1);
	lua_rawsettable ();
	
	lua_pushobject (ct);
	lua_pushstring ("get_viewport");
	lua_pushobject (daCam);
	lua_pushcclosure (getCameraViewport, 1);
	lua_rawsettable ();
	
	lua_pushobject (ct);
	lua_settag (RENDERCAMERA_TAG);
	
	lua_pushobject (ct);

	return;
}

void destroyCamera (void)
{
	// Syntax: 
	//     DestroyCamera(RenderCamera <cam>)
	// Destroys the given object.

	lua_Object obj = lua_getparam(1);

	ICamera *cam = get_camera (obj);
	if (cam)
	{
		destroy_camera (cam);
	}
}

void createTextOverlay (void)
{
	// Syntax: 
	//     NewTextOverlay ( Font <font>, string <text>, number <w>, number <h> )
	// Creates a new OverlayImage from the given text in the given font
	// Returns a userdata with the OVERLAY_TAG tag.
	// Returns nil on failure.

	lua_Object fontObj = lua_getparam(1);
	lua_Object stringObj = lua_getparam(2);
	lua_Object wObj = lua_getparam(3);
	lua_Object hObj = lua_getparam(4);

	// Verify the parameters

	if (!lua_isuserdata (fontObj) || lua_tag(fontObj) != fontTag)
	{
		return;
	}

	if (!lua_isstring (stringObj))
	{
		return;
	}

	if (!lua_isnumber (wObj) || !lua_isnumber (hObj))
	{
		return;
	}

	// Retrieve the IFontFactory interface and create a new IImageSource from the given string.

	COMPTR<IFontFactory> font;
	font = (IFontFactory *) lua_getuserdata (fontObj);
	if (font)
	{
		RECT r;
		r.top = r.left = 0;
		r.right = lua_getnumber (wObj);
		r.bottom = lua_getnumber (hObj);

		FONTIMAGEDESC fdesc;
//		fdesc.dwFlags = FDDFL_MULTILINE;
//		fdesc.pBoundingRect = &r;
		wchar_t *wstr = NULL;
		{
			const char *str = lua_getstring (stringObj);
			wstr = new wchar_t[strlen(str)+1];
			const char *here = str;
			wchar_t *there = wstr;
			while (*here != '\0')
			{
				*there = *here;
				++there;
				++here;
			}
			*there = '\0';
			fdesc.szString = wstr;
		}

		COMPTR<IFontImage> stringImage;
		if (font->CreateInstance (&fdesc, stringImage) == GR_OK)
		{
			stringImage->SetFontColor (0xFFFFFFFF, 0x00000000);

			COMPTR<IImageSource> imageSrc;
			if (stringImage->QueryInterface (IID_IImageSource, imageSrc) == GR_OK)
			{
				OverlayImage *ovi = create_overlay (imageSrc);
				lua_pushusertag (ovi, OVERLAY_TAG);
			}
		}

		if (wstr != NULL)
		{
			delete wstr;
		}
	}
}

void createImageOverlay (void)
{
	// Syntax: 
	//     NewImageOverlay ( string <imagefilename> )
	// Creates a new OverlayImage from the image in the given file
	// Returns a userdata with the OVERLAY_TAG tag.
	// Returns nil on failure.

	lua_Object imageNameObj = lua_getparam(1);

	// Verify the parameters

	if (!lua_isstring (imageNameObj))
	{
		return;
	}

	// Attempt to load the image.
	HANDLE hBmp;

	hBmp = 
		LoadImage 
		(
			NULL,
			lua_getstring(imageNameObj),
			IMAGE_BITMAP,
			0, 0,  
			LR_CREATEDIBSECTION | LR_LOADFROMFILE
		);

	ASSERT (hBmp != NULL);

	// Get its information and confirm that it is a format we want.
	// Only 16 bit, uncompressed formats are supported, for now.

	DIBSECTION dib;
	
	int gotit = GetObject ((HGDIOBJ) hBmp, sizeof(dib), &dib);
	ASSERT (gotit != 0);

	int w, h, bpp;
	int rbits, gbits, bbits;
	int stride;

	w = dib.dsBm.bmWidth;
	h = dib.dsBm.bmHeight;
	bpp = dib.dsBm.bmBitsPixel;
	stride = dib.dsBm.bmWidthBytes;
	rbits = dib.dsBitfields[0];
	gbits = dib.dsBitfields[1];
	bbits = dib.dsBitfields[2];
	U8 *bits = (U8 *) dib.dsBm.bmBits;
	int mipLevels = 0;

	if (bpp > 16 && rbits == 0)
	{
		rbits = gbits = bbits = 8;
	}

	PixelFormat srcPf (bpp, rbits, gbits, bbits, 0);

	if (dib.dsBmih.biCompression == BI_RGB && dib.dsBmih.biBitCount > 8)
	{
		// Create an overlay image

		OverlayImage *ovi = create_overlay (bits + (h-1)*stride, &srcPf, w, h, -stride);
		lua_pushusertag (ovi, OVERLAY_TAG);
	}

	if (hBmp != NULL)
	{
		DeleteObject ((HGDIOBJ) hBmp);
		hBmp = NULL;
	}
}

void destroyOverlay (void)
{
	// Syntax: 
	//     DestroyOverlay(OverlayImage <image>)
	// Destroys the given overlay object.

	lua_Object obj = lua_getparam(1);

	if (!lua_isuserdata(obj) || lua_tag(obj) != OVERLAY_TAG)
	{
		return;
	}

	OverlayImage *ovi = (OverlayImage *) lua_getuserdata (obj);
	if (ovi)
	{
		destroy_overlay (ovi);
	}
}

// Initialization routines

static void init_renderwindow_scripting ()
{
	// Create the tags.

	RENDEROBJECT_TAG = lua_newtag();
	RENDERCAMERA_TAG = lua_newtag();
	RENDERSCRIPT_TAG = lua_newtag();
	DACAMERA_TAG = lua_newtag();
	DAVECTOR_TAG = lua_newtag();
	DAXFORM_TAG = lua_newtag();
	VECTOR_TAG = lua_newtag();
	XFORM_TAG = lua_newtag();
	OVERLAY_TAG = lua_newtag();

	// Expose some tag values for use in scripts. 
	lua_pushnumber (RENDERSCRIPT_TAG);
	lua_setglobal ("RENDERSCRIPT_TAG");
	lua_pushnumber (VECTOR_TAG);
	lua_setglobal ("VECTOR_TAG");
	lua_pushnumber (XFORM_TAG);
	lua_setglobal ("XFORM_TAG");
	lua_pushnumber (OVERLAY_TAG);
	lua_setglobal ("OVERLAY_TAG");

	// Register the camera and object tag functions

	// Register the vector and transform tag functions
	lua_pushcfunction (add_vector);
	lua_settagmethod (VECTOR_TAG, "add");
	lua_pushcfunction (sub_vector);
	lua_settagmethod (VECTOR_TAG, "sub");
	lua_pushcfunction (mul_vector);
	lua_settagmethod (VECTOR_TAG, "mul");
	lua_pushcfunction (div_vector);
	lua_settagmethod (VECTOR_TAG, "div");
	lua_pushcfunction (unm_vector);
	lua_settagmethod (VECTOR_TAG, "unm");
	lua_pushcfunction (gettable_vector);
	lua_settagmethod (VECTOR_TAG, "gettable");
	lua_pushcfunction (settable_vector);
	lua_settagmethod (VECTOR_TAG, "settable");
	lua_pushcfunction (gc_davector);
	lua_settagmethod (DAVECTOR_TAG, "gc");

	lua_pushcfunction (add_xform);
	lua_settagmethod (XFORM_TAG, "add");
	lua_pushcfunction (sub_xform);
	lua_settagmethod (XFORM_TAG, "sub");
	lua_pushcfunction (mul_xform);
	lua_settagmethod (XFORM_TAG, "mul");
	lua_pushcfunction (gc_daxform);
	lua_settagmethod (DAXFORM_TAG, "gc");

	// Register the functions for:
	// * Creating the render window
	// * Destroying the render window
	// * Creating an object
	// * Destroying an object
	// * Creating a camera
	// * Destroying a camera
	// * Creating Vectors
	// * Setting rendering parameters

	lua_register ("CreateRender", createRender);
	lua_register ("DestroyRender", destroyRender);
	lua_register ("NewObject", createObject);
	lua_register ("DestroyObject", destroyObject);
	lua_register ("NewCamera", createCamera);
	lua_register ("DestroyCamera", destroyCamera);
	lua_register ("NewTextOverlay", createTextOverlay);
	lua_register ("NewImageOverlay", createImageOverlay);
	lua_register ("DestroyOverlay", destroyOverlay);
	lua_register ("Vector", newVector);
	lua_register ("Xform", newXform);
	lua_register ("TestModeSwitch", testModeSwitch);
}

bool init_renderwindow ()
{
	// Initialize the global variables

	// Initialize the scripting
	init_renderwindow_scripting ();
	
	return true;
}

void shutdown_renderwindow ()
{
	destroy_renderwindow();
}

void update_renderwindow (SINGLE dt)
{
	// Update the render widget before proceeding.
	render_widget.update (dt);

	// Start a lua block here, so that temporary objects get cleaned up.
	lua_beginblock();

	// If the render window has an OnUpdate member, call it.
	lua_Object rwo = lua_getglobal ("RenderWindow");
	if (rwo != LUA_NOOBJECT)
	{
		if (lua_istable(rwo))
		{
			lua_pushobject (rwo);
			lua_pushstring ("OnUpdate");
			lua_Object update = lua_rawgettable();
			if (update != LUA_NOOBJECT)
			{
				if (lua_isfunction(update))
				{
					lua_pushnumber (dt);
					lua_pushnumber (frameTime);
					lua_callfunction (update);
					// Don't care about return values for now.
				}
			}
		}
	}

	// Invalidate the render window, causing it to render each frame.
	// This is in case there is animation running.
	HWND hwnd = render_widget.get_hwnd();
	if (hwnd)
	{
		InvalidateRect (hwnd, NULL, FALSE);
	}

	// Free all lua stuff allocated in this block
	lua_endblock();
}
