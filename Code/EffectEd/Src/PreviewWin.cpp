//PreviewWin.cpp

#include "stdafx.h"
#include "globals.h"
#include <DACOM.h>
#include "resource.h"
#include "PreviewWin.h"
#include "Camera.h"
#include "IEffectTarget.h"
#include "IEffectFile.h"
#include "InfoArea.h"
#include "IEffectEvent.h"
#include <vector.h>
#include "SuperTrans.h"
#include <ENGINE.h>
#include <RendPipeline.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <IParticleMAnager.h>
#include <IMeshManager.h>

#include <System.h>
#include "MyVertex.h"

LONG CALLBACK previewProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

static bool bRegisterdPreview = false;

S64 lastRenderTime;
S64 perfFreq;
S64 startPlay;
bool bPlay = false;
bool bPause = false;
SINGLE gameTime;
SINGLE renderTime;

bool bShowingPreview = false;

extern IEffectTarget * selectedTarg;

IMeshInstance * renderAxis;
enum AxisPoint
{
	AXIS_NONE,
	AXIS_Y,
	AXIS_X,
	AXIS_Z,
	AXIS_YZ,
	AXIS_XY,
	AXIS_XZ
};
AxisPoint selectedAxis = AXIS_NONE;
Vector lastPos;
bool bRotateMode = false;


struct ParticleNode
{
	ParticleNode * next;
	IParticleInstance * inst;
};

ParticleNode * particleList;

void PreviewWin::Create()
{
	if(!previewWin)
	{
		QueryPerformanceFrequency((_LARGE_INTEGER *)(&perfFreq));
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(wc));
		wc.cbSize		 = sizeof(wc);
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = previewProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hMainInst;
		wc.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL; //(HBRUSH)(COLOR_APPWORKSPACE+1); // GetStockObject(BLACK_BRUSH); 
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_PREVIEWMENU);
		wc.lpszClassName = "PreviewWindow";

		if(!bRegisterdPreview)
		{
			if(RegisterClassEx(&wc) == 0)
				return;
			bRegisterdPreview = true;
		}

		previewWin = CreateWindowEx(
			0,
			wc.lpszClassName,
			"PreviewWindow",
			WS_OVERLAPPEDWINDOW, 
			0,
			0,
			800,
			600,
			NULL,
			NULL,
			hMainInst,
			NULL);

		RECT rect;
		GetClientRect(previewWin,&rect);
		SCREENRESX = rect.right-rect.left;
		SCREENRESY = rect.bottom-rect.top;

		if(PIPE)
		{
			PIPE->startup(NULL);			// switch back to primary device

			VB_MANAGER->initialize(NULL);

			S32 nextMode = 16;
			PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP,nextMode);
			PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP,16);
			PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,0);
			PIPE->set_pipeline_state(RP_BUFFERS_SWAP_STALL,1);

			PIPE->set_pipeline_state(RP_BUFFERS_ANTIALIAS,TRUE);

			PIPE->create_buffers(previewWin,800, 600);

			U32 yes = 0;
			PIPE->query_device_ability( RP_A_TEXTURE_LOD, &yes);
			if (yes)
			{
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
				SINGLE dummy = 1.0f;
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32 *)&dummy));
			}

			U32 bias = 0;
			TEXLIB->set_library_state(ITL_STATE_TEXTURE_LOD_LOAD_BIAS,*((U32 *)&bias));

			PIPE->set_pipeline_state(RP_CLEAR_COLOR,0xff000000);  // a,r,g,b
			CAMERA->SetPosition(&Vector(10000,10000,10000));
			CAMERA->SetLookAtPosition(Vector(0,0,0));
		}
/*		if(RENDERER)
		{
			RendererInit rInit;
			rInit.Hwnd = previewWin;
			rInit.MainWindow = previewWin;
			rInit.effectOptions = &options;
			rInit.texdir = TEXTURESDIR;
			rInit.multiThreaded = false;

			RENDERER->Create(rInit);
			ShowWindow(previewWin,false);//for some odd reason it does a show in there.

			CAMERA->SetPosition(&Vector(1,1,1));
			CAMERA->SetLookAtPosition(Vector(0,0,0));
		}
*/	}
}

void PreviewWin::Open()
{
	if(previewWin)
	{
		if(!bShowingPreview)
		{
			ShowWindow(previewWin,true);
			bShowingPreview = true;
		}
		SetFocus(previewWin);
	}
}

void PreviewWin::Close()
{
	if(previewWin)
	{
		if(bShowingPreview)
		{
			ShowWindow(previewWin,false);
			bShowingPreview = false;
		}
	}	
	if(renderAxis)
	{
		MESHMAN->DestroyMesh(renderAxis);
		renderAxis = NULL;
	}

	while(particleList)
	{
		ParticleNode * tmp = particleList;
		particleList = particleList->next;
		PARTMAN->ReleaseInstance(tmp->inst);
		delete tmp;
	}
}

void renderGrid()
{
	PIPE->set_texture_stage_texture(0,0);
	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

	PIPE->set_texture_stage_texture(1,0);
	PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	CAMERA->SetModelView();
	PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PIPE->set_render_state(D3DRS_LIGHTING,FALSE);

	PB.Begin(PB_LINES);
	PB.Color3ub(80,80,80);
	for(S32 i = -5; i <= 5 ;++ i)
	{
		PB.Vertex3f(i*4096*FEET_TO_WORLD,-(5*4096)*FEET_TO_WORLD,0);
		PB.Vertex3f(i*4096*FEET_TO_WORLD,(5*4096)*FEET_TO_WORLD,0);

		PB.Vertex3f(-(5*4096)*FEET_TO_WORLD,i*4096*FEET_TO_WORLD,0);
		PB.Vertex3f((5*4096)*FEET_TO_WORLD,i*4096*FEET_TO_WORLD,0);
	}
	PB.End();
}

void PreviewWin::Update()
{
	if(PIPE && previewWin && bShowingPreview)
	{
		Sleep(1);
		S64 newTick;
		QueryPerformanceCounter((_LARGE_INTEGER *)(&newTick));
		if(((newTick-lastRenderTime)*30)/perfFreq > 1)
		{
			renderTime = (((SINGLE)(newTick-lastRenderTime))/((SINGLE)perfFreq));
			CAMERA->Update();
			if(bPause)
				startPlay += newTick-lastRenderTime;
			if(bPlay)
			{
				DOUBLE testTime = (newTick-startPlay);
				testTime /= perfFreq;
				gameTime = (SINGLE)testTime;
				ENGINE->update(0);
				PARTMAN->Update(gameTime);
			}
			IEffectTarget * search = EFFECTFILE->GetFirstTarget();
			while(search)
			{
				search->Update();
				search = search->GetNextTarget();
			}

			lastRenderTime = newTick;
			CAMERA->SetPerspective();
			PIPE->set_viewport(0,0,800,600);
			PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
			PIPE->begin_scene();
			CAMERA->SetModelView();

			renderGrid();

			search = EFFECTFILE->GetFirstTarget();
			while(search)
			{
				search->Render();
				search = search->GetNextTarget();
			}

			PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);


			ParticleNode * node = particleList;
			ParticleNode * prev = NULL;
			while(node)
			{
				if(node->inst->IsFinished())
				{
					PARTMAN->ReleaseInstance(node->inst);
					if(prev)
						prev->next = node->next;
					else
						particleList = node->next;
					delete node;
					if(prev)
						node = prev->next;
					else
						node = particleList;
				}
				else
				{
					node->inst->Render();
					prev = node;
					node = node->next;
				}
			}

			if(selectedTarg)
			{
				PreviewWin::RenderAxis();
			}

			PIPE->end_scene();
			PIPE->swap_buffers();
		}
	}
}

void PreviewWin::RenderAxis()
{
	if(!renderAxis)
		renderAxis = MESHMAN->CreateMesh("gizmo.cmp");
	if(renderAxis)
	{
		TRANSFORM axisTrans = selectedTarg->GetTransform();
		SINGLE fov = CAMERA->GetVerticalFOV() * (MUL_DEG_TO_RAD*2);
		SINGLE viewSize = tan(fov)*((CAMERA->GetPosition()-axisTrans.translation).fast_magnitude());

		TRANSFORM trans;
		trans.translation = axisTrans.translation;
		trans.scale(viewSize*0.05);
		axisTrans = trans;

		renderAxis->SetTransform(axisTrans);

///////////////////////////////////
		PIPE->set_texture_stage_texture(0,0);
		PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

		PIPE->set_texture_stage_texture(1,0);
		PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		PIPE->set_render_state(D3DRS_ZENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PIPE->set_render_state(D3DRS_LIGHTING,TRUE);
		PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);

///////////////////////
		renderAxis->Update(0);
		renderAxis->Render();
	}
}

//-------------------------------------------------------------------
//
#define PLANE_XY 0
#define PLANE_XZ 1
#define PLANE_YZ 2

Vector findPlanePos(Vector origin,U32 plane,Vector camPos,Vector rayDir)
{
	if(plane == PLANE_XY)
	{
		if(rayDir.z != 0)
		{
			return camPos + (rayDir*((origin.z-camPos.z)/rayDir.z));
		}
		return origin;
	}
	else if(plane == PLANE_XZ)
	{
		if(rayDir.y != 0)
		{
			return camPos + (rayDir*((origin.y-camPos.y)/rayDir.y));
		}
		return origin;
	}
	else if(plane == PLANE_YZ)
	{
		if(rayDir.x != 0)
		{
			return camPos + (rayDir*((origin.x-camPos.x)/rayDir.x));
		}
		return origin;
	}
	return origin;
}

Vector findLastPos(Vector rayDir)
{
	U32 viewPlane1 = 0;
	U32 viewPlane2 = 0;
	Vector orbitPos, cPos;

	cPos = CAMERA->GetPosition();
	orbitPos = CAMERA->GetLookAtPosition();

	Vector camDir = (orbitPos - cPos);
	camDir.normalize();
	if(camDir.x > camDir.y)
	{
		if(camDir.x > camDir.z)
		{
			if(camDir.y > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_XZ;
			}
			else
			{
				viewPlane1 = PLANE_XZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_XZ;
			viewPlane2 = PLANE_YZ;
		}
	}
	else
	{
		if(camDir.y > camDir.z)
		{
			if(camDir.x > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_YZ;
			}
			else
			{
				viewPlane1 = PLANE_YZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_YZ;
			viewPlane2 = PLANE_XZ;
		}
	}

	switch(selectedAxis)
	{
		case AXIS_X:
			{
				if(viewPlane1 != PLANE_YZ)
					return findPlanePos(selectedTarg->GetPosition(),viewPlane1,cPos,rayDir);
				else
					return findPlanePos(selectedTarg->GetPosition(),viewPlane2,cPos,rayDir);
			}
			break;
		case AXIS_Y:
			{
				if(viewPlane1 != PLANE_XZ)
					return findPlanePos(selectedTarg->GetPosition(),viewPlane1,cPos,rayDir);
				else
					return findPlanePos(selectedTarg->GetPosition(),viewPlane2,cPos,rayDir);
			}
			break;
		case AXIS_Z:
			{
				if(viewPlane1 != PLANE_XY)
					return findPlanePos(selectedTarg->GetPosition(),viewPlane1,cPos,rayDir);
				else
					return findPlanePos(selectedTarg->GetPosition(),viewPlane2,cPos,rayDir);
			}
			break;
		case AXIS_XY:
			{
				return findPlanePos(selectedTarg->GetPosition(),PLANE_XY,cPos,rayDir);
			}
			break;
		case AXIS_XZ:
			{
				return findPlanePos(selectedTarg->GetPosition(),PLANE_XZ,cPos,rayDir);
			}
			break;
		case AXIS_YZ:
			{
				return findPlanePos(selectedTarg->GetPosition(),PLANE_YZ,cPos,rayDir);
			}
			break;
	}
	return Vector(0,0,0);
}

void PreviewWin::OnMouseDown(U32 xPos, U32 yPos)
{
	if(GetKeyState(VK_LSHIFT) >> 8)
	{
		SINGLE sx = (SINGLE)xPos;
		SINGLE sy = (SINGLE)yPos;
		Vector dir = CAMERA->ScreenToPoint(sx,sy);
		dir = dir.fast_normalize();

		if(renderAxis && selectedTarg)
		{
			HitDef hitPoint;
			Vector cPos = CAMERA->GetPosition();
			if(renderAxis->ComputeHitTest(&cPos, &dir, hitPoint))
			{
				selectedAxis = (enum AxisPoint)hitPoint.faceGroup;
				lastPos = findLastPos(dir);
				bRotateMode = true;
				SetCapture(previewWin);
			}
		}
	}
	else if(!(GetKeyState(VK_LCONTROL) >> 8))//make sure there is no control key
	{
		SINGLE sx = (SINGLE)xPos;
		SINGLE sy = (SINGLE)yPos;
		Vector dir = CAMERA->ScreenToPoint(sx,sy);
		dir = dir.fast_normalize();

		bool bAxisHit = false;
		if(renderAxis && selectedTarg)
		{
			HitDef hitPoint;
			Vector cPos = CAMERA->GetPosition();
			if(renderAxis->ComputeHitTest(&cPos, &dir, hitPoint))
			{
				selectedAxis = (enum AxisPoint)hitPoint.faceGroup;
				lastPos = findLastPos(dir);
				bAxisHit = true;
				bRotateMode = false;
				SetCapture(previewWin);
			}
		}

		if(!bAxisHit)
		{
			IEffectTarget * best = NULL;
			SINGLE bestDist = 0;
			IEffectTarget * search = EFFECTFILE->GetFirstTarget();
			while(search)
			{
				SINGLE testDist = 0;
				if(search->HitTest(CAMERA->GetPosition(),dir,testDist))
				{
					if(best)
					{
						if(bestDist > testDist)
						{
							best = search;
							bestDist = testDist;
						}
					}
					else
					{
						best = search;
						bestDist = testDist;
					}				
				}
				search = search->GetNextTarget();
			}
			if(best)
			{
				InfoArea::SelectTarget(best);
			}
			else
			{
				InfoArea::Deselect();
			}
		}
	}

}

void PreviewWin::OnMouseUp(U32 xPos, U32 yPos)
{
	if(selectedAxis != AXIS_NONE)
	{
		selectedAxis = AXIS_NONE;
		ReleaseCapture();
	}
}

//-------------------------------------------------------------------
//
void slideObject(Vector rayDir)
{
	U32 viewPlane1 = 0;
	U32 viewPlane2 = 0;
	Vector pos, objPos, objOffset = Vector(0,0,0);
	Vector orbitPos, cPos;

	objPos = selectedTarg->GetPosition();
	cPos = CAMERA->GetPosition();
	orbitPos = CAMERA->GetLookAtPosition();

	Vector camDir = (orbitPos - cPos);
	camDir.normalize();
	if(camDir.x > camDir.y)
	{
		if(camDir.x > camDir.z)
		{
			if(camDir.y > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_XZ;
			}
			else
			{
				viewPlane1 = PLANE_XZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_XZ;
			viewPlane2 = PLANE_YZ;
		}
	}
	else
	{
		if(camDir.y > camDir.z)
		{
			if(camDir.x > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_YZ;
			}
			else
			{
				viewPlane1 = PLANE_YZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_YZ;
			viewPlane2 = PLANE_XZ;
		}
	}
	switch(selectedAxis)
	{
		case AXIS_X:
			{
				if(viewPlane1 != PLANE_YZ)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.x = (pos - lastPos).x;
			}
			break;
		case AXIS_Y:
			{
				if(viewPlane1 != PLANE_XZ)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.y += (pos - lastPos).y;
			}
			break;
		case AXIS_XY:
			{
				pos = findPlanePos(objPos,PLANE_XY,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
		case AXIS_Z:
			{
				if(viewPlane1 != PLANE_XY)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.z += (pos - lastPos).z;
			}
			break;
		case AXIS_XZ:
			{
				pos = findPlanePos(objPos,PLANE_XZ,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
		case AXIS_YZ:
			{
				pos = findPlanePos(objPos,PLANE_YZ,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
	}

	lastPos = pos;

	selectedTarg->SetPosition(selectedTarg->GetTransform().translation+objOffset);
}

void rotateObject(Vector rayDir)
{
	U32 viewPlane1 = 0;
	U32 viewPlane2 = 0;
	Vector pos, objPos, objOffset = Vector(0,0,0);
	Vector orbitPos, cPos;

	objPos = selectedTarg->GetPosition();
	cPos = CAMERA->GetPosition();
	orbitPos = CAMERA->GetLookAtPosition();

	Vector camDir = (orbitPos - cPos);
	camDir.normalize();
	if(camDir.x > camDir.y)
	{
		if(camDir.x > camDir.z)
		{
			if(camDir.y > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_XZ;
			}
			else
			{
				viewPlane1 = PLANE_XZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_XZ;
			viewPlane2 = PLANE_YZ;
		}
	}
	else
	{
		if(camDir.y > camDir.z)
		{
			if(camDir.x > camDir.z)
			{
				viewPlane1 = PLANE_XY;
				viewPlane2 = PLANE_YZ;
			}
			else
			{
				viewPlane1 = PLANE_YZ;
				viewPlane2 = PLANE_XY;
			}
		}
		else
		{
			viewPlane1 = PLANE_YZ;
			viewPlane2 = PLANE_XZ;
		}
	}
	switch(selectedAxis)
	{
		case AXIS_X:
			{
				if(viewPlane1 != PLANE_YZ)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.x = (pos - lastPos).x;
			}
			break;
		case AXIS_Y:
			{
				if(viewPlane1 != PLANE_XZ)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.y += (pos - lastPos).y;
			}
			break;
		case AXIS_XY:
			{
				pos = findPlanePos(objPos,PLANE_XY,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
		case AXIS_Z:
			{
				if(viewPlane1 != PLANE_XY)
					pos = findPlanePos(objPos,viewPlane1,cPos,rayDir);
				else
					pos = findPlanePos(objPos,viewPlane2,cPos,rayDir);
				objOffset.z += (pos - lastPos).z;
			}
			break;
		case AXIS_XZ:
			{
				pos = findPlanePos(objPos,PLANE_XZ,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
		case AXIS_YZ:
			{
				pos = findPlanePos(objPos,PLANE_YZ,cPos,rayDir);
				objOffset += (pos - lastPos);
			}
			break;
	}

	lastPos = pos;

	TRANSFORM trans = selectedTarg->GetTransform();

	TRANSFORM rot;

	rot.rotate_about_i(objOffset.x*10);
	rot.rotate_about_j(objOffset.y*10);
	rot.rotate_about_k(objOffset.z*10);

	trans = rot*trans;
	
	selectedTarg->SetTransform(trans);
}

void PreviewWin::OnMouseMove(U32 xPos, U32 yPos)
{
	if(selectedAxis != AXIS_NONE)
	{
		if(selectedTarg)
		{
			SINGLE sx = xPos;
			SINGLE sy = yPos;
			Vector dir = CAMERA->ScreenToPoint(sx,sy);
			dir = dir.fast_normalize();
			if(bRotateMode)
				rotateObject(dir);
			else
				slideObject(dir);
		}
	}
}

void PreviewWin::Play()
{
	Stop();
	QueryPerformanceCounter((_LARGE_INTEGER *)(&startPlay));
	bPlay = true;
	bPause = false;
	gameTime = 0;
	IEffectEvent * event = EFFECTFILE->GetStartEvent();
	if(event)
		event->TriggerEvent();
}

void PreviewWin::PauseToggle()
{
	bPause = !bPause;
}

void PreviewWin::Stop()
{
	gameTime = 0;
	CAMERA->EndHardpointMode();
	CAMERA->ResetClipPlane();
	bPlay = false;
	IEffectTarget * search = EFFECTFILE->GetFirstTarget();
	while(search)
	{
		search->Hide(false);
		search->StopAnimation();
		search->ClearUpdateList();
		search = search->GetNextTarget();
	}
	while(particleList)
	{
		ParticleNode * tmp = particleList;
		particleList = particleList->next;
		PARTMAN->ReleaseInstance(tmp->inst);
		delete tmp;
	}

//	SOUNDSYS->StopAll();
}

void PreviewWin::AddParticleEffect(IParticleInstance * inst)
{
	ParticleNode * node = new ParticleNode;
	node->next = particleList;
	node->inst = inst;
	particleList = node;
}


SINGLE PreviewWin::GetGameTime()
{
	return gameTime;
}

SINGLE PreviewWin::GetRenderTime()
{
	return renderTime;
}

LONG CALLBACK previewProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	msg.hwnd = hWindow;
	msg.message = message;
	msg.lParam = lParam;
	msg.wParam = wParam;
	CAMERA->Notify(message,&msg);
	switch(message)
	{
	case WM_CLOSE:
		{
			PreviewWin::Close();
			return 0;
		}
		break;
	case WM_LBUTTONDOWN:
		{
			PreviewWin::OnMouseDown(LOWORD(lParam),HIWORD(lParam));
		}
		break;
	case WM_LBUTTONUP:
		{
			PreviewWin::OnMouseUp(LOWORD(lParam),HIWORD(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		{
			PreviewWin::OnMouseMove(LOWORD(lParam),HIWORD(lParam));
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case ID_PLAYBACK_PLAY:
				{
					PreviewWin::Play();
				}
				break;
			case ID_PLAYBACK_STOP:
				{
					PreviewWin::Stop();
				}
				break;
			}
		}
		break;
/*	case SOM_STREAMER:
		{
			if(SOUNDSYS)
			{
				SOUNDSYS->StreamerMessage(wParam,lParam);
			}
		}
		break;
*/	case WM_KEYDOWN:
		{
			if(wParam == VK_SPACE)
				PreviewWin::PauseToggle();
		}
	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}
