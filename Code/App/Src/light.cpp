//--------------------------------------------------------------------------//
//                                                                          //
//                               Light.cpp                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/light.cpp 70    10/23/00 11:05p Jasony $
*/			    
//--------------------------------------------------------------------------//
#include "pch.h"
#include <globals.h>

#include "TDocClient.h"
#include "Menu.h"
#include "Resource.h"
#include "DBHotkeys.h"
//#include "SysMap.h"
#include "UserDefaults.h"
#include "Camera.h"
#include "Lightman.h"
#include "Startup.h"
#include "DLight.h"
#include "Sector.h"		// for cqlight
#include "CQTrace.h"
#include "CQLight.h"
#include "Objlist.h"
#include "DLight.h"
#include "TObject.h"
#include "Mission.h"
#include "ArchHolder.h"
#include "MGlobals.h"
#include "GridVector.h"
#include "DQuickSave.h"
#include "TManager.h"
#include "IGameLight.h"
#include "TObjFrame.h"
#include "TObjRender.h"
//setting ambient
#include "Field.h"
#include "DField.h"
#include "TerrainMap.h"
//

#include <Mesh.h>
#include <Renderer.h>
#include <Engine.h>
#include <3DMath.h>
#include <TComponent.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <Viewer.h>
#include <EventSys.h>
#include <Quat.h>
#include <ITextureLibrary.h>
#include <View2D.h>
#include <map>
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)

#include <malloc.h>
#include <stdlib.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
static char szRegKey[] = "Lights";
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
CQLight::CQLight (void)
{
	COMPTR<IDAConnectionPoint> connection;

	bLogicalOn = false;
	systemID = 0;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(static_cast<IEventCallback *>(this), &callbackHandle);
	}
	engineID = ILights::GetNextEngineID();

// this is an odd place to put this, moved it to Initalize()
//	OBJLIST->DEBUG_IncLightCount();
}
//----------------------------------------------------------------------------
//
CQLight::~CQLight (void)
{
	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(callbackHandle);
	}
	ILights::ReleaseEngineID(engineID);
	if( OBJLIST )
		OBJLIST->DEBUG_DecLightCount();
	removeFromRegList();
}
//----------------------------------------------------------------------------
//
GENRESULT CQLight::Notify(U32 message, void *param)
{
	if (message == CQE_SYSTEM_CHANGED)
	{
		if (bAmbient)
		{
			if (systemID == (U32)param)
			{
				//LIGHT->set_ambient_light(color.r,color.g,color.b);  //only good so get_ambient_light still works
				LIGHTS->SetSysAmbientLight(color.r,color.g,color.b);
			}
		}
		else
		{
			UpdateLight();
		}
	}

	if (message == CQE_ENTERING_INGAMEMODE)
	{
		UpdateLight();
	}

	if (message == CQE_CAMERA_MOVED)
	{
		UpdateLight();
	}

	return GR_OK;
}

GENRESULT CQLight::set_On (BOOL32 _on)  
{
	bLogicalOn = (_on != 0);
	UpdateLight();

	return GR_OK;
}

void CQLight::UpdateLight(bool bUseThisLight)
{
	//if (bLogicalOn==false || systemID==0 || systemID == SECTOR->GetCurrentSystem())
	bool bReallyReallyOn = bLogicalOn && (systemID == SECTOR->GetCurrentSystem() || systemID==0);

	//if (!bUseThisLight) bReallyReallyOn = false;

	BaseLight::set_On(bReallyReallyOn);

	if (bReallyReallyOn)
		addToRegList();
	else
		removeFromRegList();
	
	if (bReallyReallyOn && CQFLAGS.b3DEnabled && this->bAmbient == 0)
	{
		//const Transform *trans = CAMERA->GetInverseTransform();
		//Vector v = trans->rotate_translate(transform.translation);
		// changed for DX9 - Ryan
		Vector v = transform.translation;
		D3DLIGHT9 d3dl;
		// Initialize the structure.    
		ZeroMemory(&d3dl, sizeof(D3DLIGHT9)); 
		
		if (!infinite)
		{
			d3dl.Type = D3DLIGHT_POINT;
		}
		else
		{
		//	Vector d = trans->rotate(direction);
		//	if (d.magnitude_squared())
		//		d.normalize();
		// changed by Ryan for DX9
			d3dl.Type = D3DLIGHT_DIRECTIONAL;
			d3dl.Direction.x = direction.x;
			d3dl.Direction.y = direction.y;
			d3dl.Direction.z = direction.z;
		}


		d3dl.Diffuse.r = color.r/255.0;    d3dl.Diffuse.g = color.g/255.0;
		d3dl.Diffuse.b = color.b/255.0;    
		
		d3dl.Ambient.r = 0.0f;
		d3dl.Ambient.g = 0.0f;    d3dl.Ambient.b = 0.0f;
		d3dl.Specular.r = 0.0f;    d3dl.Specular.g = 0.0f;
		d3dl.Specular.b = 0.0f; 
		

		d3dl.Position.x = v.x;
		d3dl.Position.y = v.y;
		d3dl.Position.z = v.z; 
		//attenuation from objview
		if( range < 0.0001 ) {
			d3dl.Attenuation1 = 0.0001f;
			d3dl.Range = sqrt(FLT_MAX);
		}
		else {
			d3dl.Attenuation0 = 1.0f;
			d3dl.Attenuation1 = 0.0f; //1.0f/(4.00f * r);
			d3dl.Attenuation2 = 1.0f/(0.25f * range * range);
			d3dl.Range = 2.0f * range;
		}
		
		PIPE->set_light(engineID,&d3dl);
		//	PIPE->set_light_enable(engineID,TRUE);
	}
}

struct GameLightArchetype : RenderArch
{
	ARCHETYPE_INDEX archeIndex;
	PARCHETYPE pArchetype;
	BT_LIGHT *data;
	U32 lightTex;

	~GameLightArchetype()
	{
	//	if (pArchetype)
	//		ARCHLIST->Release(pArchetype);
		ENGINE->release_archetype(archeIndex);
		TMANAGER->ReleaseTextureRef(lightTex);
	}
};

CQLight * CQLight::pRegList;		// list of registered lights

struct DUMMY
{
};

struct ObjInstanceIndex : IBaseObject
{
	INSTANCE_INDEX instanceIndex;
};

//----------------------------------------------------------------------------
//
struct GameLight : ObjectRender<ObjectFrame<ObjInstanceIndex,DUMMY,GameLightArchetype> >, CQLight, IPhysicalObject, ISaveLoad, IQuickSaveLoad, IGameLight
{

	BEGIN_MAP_INBOUND(GameLight)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IGameLight)
	END_MAP()

	struct ILights *factory;
	GameLight *next;
	char name[32];
	//this flag is here to tag objects that will be deleted when the user clicks
	//OK on the dialog box.  If the user clicks cancel, no lights will be deleted
	BOOL32 flag:1;
	OBJBOX box;	// maxx,minx,maxy,miny,maxz,minz  in object coordinates
	SINGLE boxRadius;		// largest of box coordinates
	U32 lightTex;
	
	GameLight (void);

	~GameLight (void);


	/* IBaseObject methods */

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual S32 GetObjectIndex (void) const
	{
		return instanceIndex;
	}
	
	virtual const TRANSFORM & GetTransform (void) const
	{
		return *(static_cast<const TRANSFORM *> (&transform));
	}

	virtual Vector GetVelocity (void)
	{
		return Vector(0,0,0);
	}

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual SINGLE TestHighlight (const RECT &rect);

	void DrawSelected ();

#include "corners.h"

	virtual void View ();

	virtual void Render();

	/* IEngineInstance methods */

	virtual void   COMAPI create_instance (INSTANCE_INDEX index);
	
	virtual void   COMAPI destroy_instance (INSTANCE_INDEX index);

	virtual void COMAPI get_centered_radius (INSTANCE_INDEX object, float *radius, Vector *center) const
	{
		*radius = 1000.0f;
		center->zero();
	}
	
	//IPhysicalObject
	virtual void SetSystemID (U32 newSystemID);

	virtual void SetPosition (const Vector & position, U32 newSystemID);
	
	virtual void SetTransform (const TRANSFORM & _transform, U32 newSystemID);

	virtual void SetVelocity (const Vector & velocity)
	{
	}

	virtual void SetAngVelocity (const Vector & angVelocity)
	{
	}

	// ISaveLoad 

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file); 

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	//IGameLight methods

	virtual void SetColor (U8 red,U8 green,U8 blue);

	virtual void GetColor (U8 & red,U8 & green,U8 & blue);

	virtual void SetName (const char * name);

	virtual void GetName (char * destName);

	virtual void SetRange (S32 range,bool bInfinite);

	virtual void GetRange (S32 &range,bool &bInfinite);

	virtual void SetDirection (Vector dir, bool bAmbient);

	virtual void GetDirection (Vector & dir, bool & bAmbient);

	//GameLight methods
	void GetBaseLight(CQLIGHT_SAVELOAD *data);

	void SetBaseLight(CQLIGHT_SAVELOAD *data);
	
};
//--------------------------------------------------------------------------//

GameLight::GameLight (void)
{
	instanceIndex = INVALID_INSTANCE_INDEX;
	set_On(TRUE);
}

GameLight::~GameLight (void)
{
	if (factory)
	{
		factory->DeleteLight(this);
		factory = 0;
	}
	if (instanceIndex != -1)
		ENGINE->destroy_instance(instanceIndex);
}

//---------------------------------------------------------------------------
//
void GameLight::View (void)
{
	CQLIGHT_SAVELOAD data;
	Vector vec;

	GetBaseLight(&data);

//	memcpy(data.name,name, sizeof(data.name));

	if (DEFAULTS->GetUserData("CQLIGHT_SAVELOAD", " ", &data, sizeof(data)))
	{
		SetBaseLight(&data);
		memcpy(name, data.name, sizeof(data.name));
	}
}

//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void GameLight::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)	
{
	bVisible = (GetSystemID() == currentSystem &&
			     defaults.bEditorMode);
	
	if (bVisible)
	{
		RECT _rect;

		Vector pos;
		const Transform *trans= CAMERA->GetInverseWorldTransform();
		pos = trans->rotate_translate(transform.translation);
		Vector p[4];
		CAMERA->PaneToPoints(p[0],p[1],p[2],p[3]);
		
		_rect.left  = F2LONG(pos.x - range);
		_rect.right	= F2LONG(pos.x + range);
		_rect.top = F2LONG(pos.y - range);
		_rect.bottom = F2LONG(pos.y + range);
		
		RECT screenRect;
		screenRect.left = F2LONG(p[0].x);
		screenRect.right = F2LONG(p[1].x);
		screenRect.top = F2LONG(p[2].y);
		screenRect.bottom = F2LONG(p[0].y);
		
		bVisible = RectIntersects(_rect, screenRect);
	}
}

SINGLE GameLight::TestHighlight (const RECT & rect)
{
	SINGLE closeness = 999999.0f;

	bHighlight = 0;
	if (bVisible)
	{
		float depth, center_x, center_y, radius;

		if (REND->get_instance_projected_bounding_sphere(instanceIndex, MAINCAM, LODPERCENT, center_x, center_y, radius, depth) != 0)
		{
			RECT _rect;

			_rect.left  = F2LONG(center_x - radius);
			_rect.right	= F2LONG(center_x + radius);
			_rect.top = F2LONG(center_y - radius);
			_rect.bottom = F2LONG(center_y + radius);

			RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };

			if (RectIntersects(_rect, screenRect) != 0)
			{
				if (RectIntersects(rect, _rect))
				{
					ViewPoint points[64];
					int numVerts = sizeof(points) / sizeof(VFX_POINT);

					if (REND->get_instance_projected_bounding_polygon(instanceIndex, MAINCAM, LODPERCENT, numVerts, points, numVerts, depth))
					{
						bHighlight = RectIntersects(rect, points, numVerts);

						if (rect.left == rect.right && rect.top == rect.bottom)
							closeness = fabs(rect.left - center_x) * fabs(rect.top - center_y);
						else
							closeness = 0.0f;
					}
				}
			}
		}
	}
	return closeness;
}

#define L1 box[5]	//-1000
#define L2 box[4]	// 1000
#define W1 box[1]	//-500
#define W2 box[0]	// 500
#define H1 box[3]
#define H2 box[2]
void GameLight::DrawSelected (void)
{
	if (bVisible==0 || instanceIndex == INVALID_INSTANCE_INDEX)
		return;

	BATCH->set_state(RPR_BATCH,FALSE);
	CAMERA->SetModelView(&transform);

	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	//no textures
	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );	
	//			
	PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	PB.Color4ub(216,201,20, 180);		// RGB_GOLD
	
	const SINGLE TWIDTH  = __min((-0.2*L1) , 200.0);

	PB.Begin(PB_TRIANGLES);

	PB.Vertex3f(W1, 0, L1);
	PB.Vertex3f(W1+TWIDTH, 0, L1);
	PB.Vertex3f(W1, 0, L1+TWIDTH);

	PB.Vertex3f(W2-TWIDTH, 0, L1);
	PB.Vertex3f(W2, 0, L1);
	PB.Vertex3f(W2, 0, L1+TWIDTH);

	PB.Vertex3f(W1, 0, L2-TWIDTH);
	PB.Vertex3f(W1+TWIDTH, 0, L2);
	PB.Vertex3f(W1, 0, L2);

	PB.Vertex3f(W2, 0, L2-TWIDTH);
	PB.Vertex3f(W2, 0, L2);
	PB.Vertex3f(W2-TWIDTH, 0, L2);

	PB.End(); 	// end of GL_QUADS
}

void GameLight::Render ()
{
	if (bVisible)
	{			
		BATCH->set_state(RPR_BATCH,false);
		mc.mi[0]->fgi->emissive.r = color.r;
		mc.mi[0]->fgi->emissive.g = color.g;
		mc.mi[0]->fgi->emissive.b = color.b;
		mc.mi[0]->fgi->diffuse.r = 0;
		mc.mi[0]->fgi->diffuse.g = 0;
		mc.mi[0]->fgi->diffuse.b = 0;
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);

		TreeRender(mc);

		if (!infinite)
		{
			PB.Color4ub(color.r/2,color.g/2,color.b/2,255);

			Vector v[4];
			Vector i(1,0,0);
			Vector j(0,1,0);

			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);

			SetupDiffuseBlend(lightTex,TRUE);
			CAMERA->SetModelView(&transform);
			
			if (cutoff > 0 && cutoff < 180 && direction.magnitude())
			{
				PB.Begin(PB_TRIANGLES);
				i = direction;
				i.normalize();
			//	Vector look = -CAMERA->GetTransform()->get_k();
				j = cross_product(Vector(0,0,1),i);
				j.normalize();
				if (j.x == 0 && j.y == 0 && j.z == 0)
					j.set(1,0,0);

				v[0].set(0,0,0);
				if (cutoff >= 45)
				{
					v[1] = i*range-j*range;
					v[2] = i*range+j*range;

					PB.TexCoord2f(0.5,0.5);
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(1,1);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(0,1);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					
					if (cutoff >= 135)
					{
						v[1] = i*range-j*range;
						v[2] = -i*range-j*range;
						
						PB.TexCoord2f(0.5,0.5);
						PB.Vertex3f(v[0].x,v[0].y,v[0].z);
						PB.TexCoord2f(1,0);
						PB.Vertex3f(v[1].x,v[1].y,v[1].z);
						PB.TexCoord2f(0,0);
						PB.Vertex3f(v[2].x,v[2].y,v[2].z);
						
						v[1] = i*range+j*range;
						v[2] = -i*range+j*range;
						
						PB.TexCoord2f(0.5,0.5);
						PB.Vertex3f(v[0].x,v[0].y,v[0].z);
						PB.TexCoord2f(1,1);
						PB.Vertex3f(v[1].x,v[1].y,v[1].z);
						PB.TexCoord2f(0,1);
						PB.Vertex3f(v[2].x,v[2].y,v[2].z);

						if (cutoff > 135)
						{
							SINGLE sinA = sin(cutoff*PI/180.0);
							v[1] = -i*range+sinA*j*range;
							v[2] = -i*range+j*range;
							
							PB.TexCoord2f(0.5,0.5);
							PB.Vertex3f(v[0].x,v[0].y,v[0].z);
							PB.TexCoord2f(0,sinA*0.5+0.5);
							PB.Vertex3f(v[1].x,v[1].y,v[1].z);
							PB.TexCoord2f(0,1);
							PB.Vertex3f(v[2].x,v[2].y,v[2].z);

							v[1] = -i*range-sinA*j*range;
							v[2] = -i*range-j*range;

							PB.TexCoord2f(0.5,0.5);
							PB.Vertex3f(v[0].x,v[0].y,v[0].z);
							PB.TexCoord2f(0,-sinA*0.5+0.5);
							PB.Vertex3f(v[1].x,v[1].y,v[1].z);
							PB.TexCoord2f(0,0);
							PB.Vertex3f(v[2].x,v[2].y,v[2].z);


						}
					}
					else
					{
						SINGLE cosA = cos(cutoff*PI/180.0);
						v[1] = i*range-j*range;
						v[2] = -j*range+cosA*i*range;

						PB.TexCoord2f(0.5,0.5);
						PB.Vertex3f(v[0].x,v[0].y,v[0].z);
						PB.TexCoord2f(1,0);
						PB.Vertex3f(v[1].x,v[1].y,v[1].z);
						PB.TexCoord2f(cosA*0.5+0.5,0);
						PB.Vertex3f(v[2].x,v[2].y,v[2].z);

						v[1] = i*range+j*range;
						v[2] = cosA*i*range+j*range;

						PB.TexCoord2f(0.5,0.5);
						PB.Vertex3f(v[0].x,v[0].y,v[0].z);
						PB.TexCoord2f(1,1);
						PB.Vertex3f(v[1].x,v[1].y,v[1].z);
						PB.TexCoord2f(cosA*0.5+0.5,1);
						PB.Vertex3f(v[2].x,v[2].y,v[2].z);
					}
				}
				else
				{
					SINGLE sinA = sin(cutoff*PI/180.0);
					v[1] = i*range;
					v[2] = v[1];
					v[1] += sinA*j*range;
					v[2] -= sinA*j*range;
					
					PB.TexCoord2f(0.5,0.5);
					PB.Vertex3f(v[0].x,v[0].y,v[0].z);
					PB.TexCoord2f(1,sinA*0.5+0.5);
					PB.Vertex3f(v[1].x,v[1].y,v[1].z);
					PB.TexCoord2f(1,-sinA*0.5+0.5);
					PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				}
				

				PB.End();

			/*	SINGLE theta = atan2(direction.y,direction.x);
				theta -= cutoff*PI/180.0;
				Vector a = direction;
				a.normalize();
				a.z = 0;
				SINGLE mag = a.magnitude();
				a = direction;
				a.normalize();
				v[0] = range*a;
				a.set (cos(theta),sin(theta),a.z);
				v[1] = range*a;
				v[2].set(0,0,0);
				theta += 2*cutoff*PI/180.0;
				a.set (cos(theta),sin(theta),a.z);
				v[3] = range*a;

				PB.Begin(PB_TRIANGLES);
				PB.TexCoord2f(0.5,0.5);
				PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				PB.TexCoord2f(1,0.5);
				PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				PB.Vertex3f(v[1].x,v[1].y,v[1].z);

				PB.TexCoord2f(0.5,0.5);
				PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				PB.TexCoord2f(1,0.5);
				PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				PB.End();*/

			}
			else
			{
				v[0] = -i*range-j*range;
				v[1] = i*range-j*range;
				v[2] = i*range+j*range;
				v[3] = -i*range+j*range;
				PB.Begin(PB_QUADS);
				
				PB.TexCoord2f(0,0);
				PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				PB.TexCoord2f(1,0);
				PB.Vertex3f(v[1].x,v[1].y,v[1].z);
				PB.TexCoord2f(1,1);
				PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				PB.TexCoord2f(0,1);
				PB.Vertex3f(v[3].x,v[3].y,v[3].z);
				PB.End();
			}
		}
		BATCH->set_state(RPR_BATCH,true);

	}
}

/* IEngineInstance methods */

void COMAPI GameLight::create_instance (INSTANCE_INDEX _index)
{
	instanceIndex = _index;
	//best places for this??
	ComputeCorners(box, instanceIndex);
	boxRadius = __max(box[0], -box[1]);
	boxRadius = __max(boxRadius, box[4]);
	boxRadius = __max(boxRadius, -box[5]);
}

void COMAPI GameLight::destroy_instance (INSTANCE_INDEX _index)
{
	CQASSERT(_index == instanceIndex);
	instanceIndex = -1;
}

// IPhysical Object
void GameLight::SetSystemID (U32 newSystemID)
{
	setSystem(newSystemID);
	UpdateLight();
}

void GameLight::SetPosition (const Vector & position, U32 newSystemID)
{
	setSystem(newSystemID);
	transform.translation = position;
	gridPos = position;
	UpdateLight();
}

void GameLight::SetTransform (const TRANSFORM & _transform, U32 newSystemID)
{
	setSystem(newSystemID);
	transform = _transform;
	gridPos = transform.translation;
	UpdateLight();
}

//ISaveload
BOOL32 GameLight::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file,file2;
	BOOL32 result = 0;

	DWORD dwWritten;

	CQLIGHT_SAVELOAD save;

	fdesc.lpFileName = "CQLIGHT_SAVELOAD";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	GetBaseLight(&save);
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;

}

BOOL32 GameLight::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "CQLIGHT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	CQLIGHT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("CQLIGHT_SAVELOAD", buffer, &load);

	SetBaseLight(&load);
	gridPos = transform.translation;

	result = 1;

Done:	
	return result;
}

void GameLight::ResolveAssociations()
{}
//--------------------------------------------------------------------------------------
//
void GameLight::QuickSave (struct IFileSystem * file)
{
	static U8 unique=0;
	unique++;
	DAFILEDESC fdesc;
	HANDLE hFile;

	char buffer[256];

	//this crappy code makes part names unique by adding numbers like 001 to the beginning
	strcpy(buffer,"000");
	if (unique > 99)
	{
		_ltoa(unique,buffer,10);
	}
	else if (unique > 9)
	{
		_ltoa(unique,&buffer[1],10);
	}
	else
	{
		_ltoa(unique,&buffer[2],10);
	}
	strcpy(&buffer[3],name);
	fdesc.lpFileName = buffer;

	file->CreateDirectory("MT_LIGHT_QLOAD");
	if (file->SetCurrentDirectory("MT_LIGHT_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_LIGHT_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_LIGHT_QLOAD qload;
		DWORD dwWritten;
		
		qload.position.init(GetGridPosition(),systemID);
		qload.bAmbient = bAmbient;
		qload.bInfinite = (infinite != 0);
		qload.blue = color.b;
		qload.cutoff = cutoff;
		qload.direction = direction;
		qload.green = color.g;
		qload.range = F2LONG(range);
		qload.red = color.r;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void GameLight::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_LIGHT_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetPosition(qload.position, qload.position.systemID);
	strcpy(name,&szInstanceName[3]);

	bAmbient = qload.bAmbient;
	infinite = (qload.bInfinite);
	color.b = qload.blue;
	color.g = qload.green;
	color.r = qload.red;
	cutoff = qload.cutoff;
	direction = qload.direction;
	range = qload.range;
}
//--------------------------------------------------------------------------------------
//
void GameLight::QuickResolveAssociations (void)
{
}
//--------------------------------------------------------------------------------------
//
void GameLight::SetColor (U8 red,U8 green,U8 blue)
{
	color.r = red;
	color.g = green;
	color.b = blue;
	if (on)
		UpdateLight();
}
//--------------------------------------------------------------------------------------
//
void GameLight::GetColor (U8 & red,U8 & green,U8 & blue)
{
	red = color.r;
	green = color.g;
	blue = color.b;
}
//--------------------------------------------------------------------------------------
//
void GameLight::SetName (const char * _name)
{
	strcpy(name,_name);
}
//--------------------------------------------------------------------------------------
//
void GameLight::GetName (char * destName)
{
	strcpy(destName,name);
}
//--------------------------------------------------------------------------------------
//
void GameLight::SetRange (S32 _range,bool _bInfinite)
{
	range = _range;
	infinite = _bInfinite;
	if (on)
		UpdateLight();
}
//--------------------------------------------------------------------------------------
//
void GameLight::GetRange (S32 & _range,bool & _bInfinite)
{
	_range = F2LONG(range);
	_bInfinite = (infinite != 0);
}
//--------------------------------------------------------------------------------------
//
void GameLight::SetDirection (Vector dir, bool _bAmbient)
{
	direction = dir;
	makeAmbient(_bAmbient);
	if (on)
		UpdateLight();
}
//--------------------------------------------------------------------------------------
//
void GameLight::GetDirection (Vector & dir, bool & _bAmbient)
{
	dir = direction;
	_bAmbient = bAmbient;
}
//--------------------------------------------------------------------------------------
//
void GameLight::GetBaseLight (CQLIGHT_SAVELOAD *data)
{
	data->bInfinite = (infinite != 0);
	data->red = color.r;
	data->green = color.g;
	data->blue = color.b;
	data->cutoff = cutoff;
	data->range = F2LONG(range);
	data->direction = direction;
	data->systemID = systemID;
	data->position = transform.translation;
	data->bAmbient = bAmbient;
	memcpy(data->name,name, sizeof(data->name));
}

void GameLight::SetBaseLight (CQLIGHT_SAVELOAD *data)
{
	infinite = (data->bInfinite != 0);
	color.r = data->red;
	color.g = data->green;
	color.b = data->blue;
	cutoff = data->cutoff;
	range = data->range;
	direction = data->direction;
	setSystem(data->systemID);
	Notify(CQE_SYSTEM_CHANGED,(void *)SECTOR->GetCurrentSystem());
	transform.translation = data->position;
	makeAmbient(data->bAmbient);
	memcpy(name,data->name, sizeof(name));
	if (on)
		UpdateLight();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_ENGINE_LIGHTS 128
#define MAX_BEST_LIGHTS 8
static bool engineLightArray[MAX_ENGINE_LIGHTS];
static int nextEngineID = MAX_ENGINE_LIGHTS;

struct DACOM_NO_VTABLE Lights : public IEventCallback, DocumentClient, IObjectFactory, ILights
{
	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;

	LIGHT_DATA lightData;
	CQLight lights[2];
	Vector light2vec;

	U32 menuID;
	U32 eventHandle,factoryHandle;
	//specular highlight texture
	U32 textureID;

	struct GameLight *lightList;

	BOOL32 bCameraMoved;
	int bestCount;

	CQLight *lightArray[MAX_BEST_LIGHTS];
		
	U8_RGB sys_amb;

	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(Lights)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(ILights)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	Lights (void)
	{
	}

	~Lights (void);
	
//	BOOL32 LoadData (IFileSystem* parent);

	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	BOOL32 CreateViewer (void);
	
	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

//	BOOL32 RegBmp(char *fileName);

	static BOOL CALLBACK LightListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	virtual void loadTextures (bool bEnable);

	//ILights
	virtual void DeleteLight (GameLight *light);

	virtual void EnableSpecular (BOOL32 bEnable);

	virtual	BOOL32 __stdcall New (void);

	virtual	BOOL32 __stdcall Load (struct IFileSystem * inFile);

	int get_best_lights (const Vector & spot, float radius);

	virtual void ActivateBestLights (const Vector &spot, int maxLights, float maxRadius);

	virtual void ActivateAmbientLight(const Vector &_spot);

	virtual void DeactivateAllLights ();

	virtual void RestoreAllLights ();

	virtual void SetSysAmbientLight (U8 r,U8 g,U8 b);

	virtual void GetSysAmbientLight (U8 &r,U8 &g,U8 &b);

	virtual void SetAmbientLight (U8 r,U8 g,U8 b);


	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	static int __stdcall insertSort (CQLight *lightArray[MAX_BEST_LIGHTS], float scoreArray[MAX_BEST_LIGHTS], int numEntries, CQLight * light, float score);
};
//------------------------------------------------------------------------
//------------------------------------------------------------------------
Lights::~Lights (void)
{
	COMPTR<IDAConnectionPoint> connection;
	if (GS)
	{
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

	DEFAULTS->SetDataInRegistry(szRegKey, &lightData, sizeof(lightData));
	MAINLIGHT = CAMERALIGHT = 0;
}

//--------------------------------------------------------------------------//
//
GENRESULT Lights::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	if (MGlobals::IsGlobalLighting())
	{
		DWORD dwRead;
		
		if (doc)
		{
			doc->SetFilePointer(0,0);
			doc->ReadFile(0, &lightData, sizeof(lightData), &dwRead, 0);
		}
		
		//
		// light creation
		//
		
		Vector dir;
		
		lights[0].setColor(lightData.light1.red,lightData.light1.green,lightData.light1.blue);
		if (lights[0].color.r || lights[0].color.g || lights[0].color.b)
		{
			dir.x = lightData.light1.direction.x;
			dir.y = lightData.light1.direction.y;
			dir.z = lightData.light1.direction.z;
			
			lights[0].infinite = true;
			lights[0].direction = dir;
			lights[0].set_On(true);
		}
		else
			lights[0].set_On(false);
		
		lights[1].setColor(lightData.light2.red,lightData.light2.green,lightData.light2.blue);
		if (lights[1].color.r || lights[1].color.g || lights[1].color.b)
		{
			dir.x = lightData.light2.direction.x;
			dir.y = lightData.light2.direction.y;
			dir.z = lightData.light2.direction.z;
			light2vec = dir;
			
			lights[1].infinite = true;
			lights[1].direction = dir;
			lights[1].set_On(true);
			
			// tie light to camera
			SINGLE angle = CAMERA->GetWorldRotation()*PI/180;
			Vector a(0,0,-1);
			Quaternion bob(a,angle);
			Vector ted = bob.transform(light2vec);
			lights[1].direction = ted;
		}
		else
			lights[1].set_On(false);
		
		//float ambient_RGBA[4] = {lightData.ambient.red, lightData.ambient.green, lightData.ambient.blue, 1.0};
		//	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_RGBA);
		LIGHTS->SetSysAmbientLight(lightData.ambient.red, lightData.ambient.green, lightData.ambient.blue);
		LIGHTS->SetAmbientLight(lightData.ambient.red, lightData.ambient.green, lightData.ambient.blue);
		
	}
	else
	{
		lights[0].set_On(false);
		lights[1].set_On(false);
		LIGHTS->SetSysAmbientLight(0,0,0);
		LIGHTS->SetAmbientLight(0,0,0);
		EVENTSYS->Send(CQE_SYSTEM_CHANGED,(void *)SECTOR->GetCurrentSystem());
	}
	
	//	U32 bud = LIGHT->get_active_lights(0);

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 Lights::CreateViewer (void)
{
	DACOMDESC desc = "IViewConstructor";
	COMPTR<IDAConnectionPoint> connection;
	HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
	MENUITEMINFO minfo;

	memset(&minfo, 0, sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE;
	minfo.fType = MFT_STRING;
	minfo.wID = IDS_VIEWLIGHTS;
	minfo.dwTypeData = "Lights";
	minfo.cch = strlen(minfo.dwTypeData);
		
	if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
		menuID = IDS_VIEWLIGHTS;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &eventHandle);
	}

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}

	if (DEFAULTS->GetDataFromRegistry(szRegKey, &lightData, sizeof(lightData)) != sizeof(lightData))
	{
		lightData.light1.red = 180;
		lightData.light1.blue = 180;
		lightData.light1.green = 180;
		lightData.light1.direction.x = 0;
		lightData.light1.direction.y = 0.3;
		lightData.light1.direction.z = -1;

		lightData.light2.red = 0;
		lightData.light2.blue = 0;
		lightData.light2.green = 0;
		lightData.light2.direction.x = 0;
		lightData.light2.direction.y = 1;
		lightData.light2.direction.z = -1.1;

		lightData.ambient.red = 0;
		lightData.ambient.blue = 0;
		lightData.ambient.green = 0;
	}

	OnUpdate(0,0,0);
	if (CQFLAGS.bNoGDI)
		return 1;

	DOCDESC ddesc;

	ddesc.memory = &lightData;
	ddesc.memoryLength = sizeof(lightData);

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "LIGHT_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);
		//	MoveWindow(hwnd, 140, 140, 100, 100, 1);

			viewer->set_instance_name("Let There Be");

			MakeConnection(doc);

			OnUpdate(doc,NULL,NULL);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Lights::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_SYSTEM_CHANGED:
//		LIGHT->set_ambient_light(0,0,0);  why??
		break;
	case CQE_CAMERA_MOVED:
		{
		//bCameraMoved = 1;
		SINGLE angle = CAMERA->GetWorldRotation()*PI/180;
		Vector a(0,0,-1);
		Quaternion bob(a,angle);
		Vector ted = bob.transform(light2vec);
		lights[1].direction = ted;
		}

		break;

	case WM_CLOSE:
		if (viewer)
			viewer->set_display_state(0);
		loadTextures(false);
		break;
	
	case CQE_DEBUG_HOTKEY:
		switch ((U32)param)
		{
		case IDH_LIGHTS:
			{
				HWND dialog = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_FIELDLIST), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) LightListDlgProc, LPARAM(this));
				SetParent(dialog,hMainWindow);
				ShowWindow(dialog,SW_SHOWNORMAL);
				SetWindowPos(dialog,HWND_NOTOPMOST, 20, 20, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
			}
			break;
			
		case IDH_EXPLODE:
			IBaseObject *obj = OBJLIST->GetSelectedList();
			IBaseObject *nextObj;
			while (obj)
			{
				nextObj = obj->nextSelected;
				GameLight *pos,*last,*next;
				last = NULL;
				pos = lightList;
				while (pos)
				{
					next = pos->next;
					IBaseObject * light_bo;
					light_bo = static_cast<IBaseObject *>(pos);
					if (light_bo == obj)
					{
						if (last)
							last->next=pos->next;
						else
							lightList = pos->next;
						
						OBJLIST->RemoveObject(pos);
						
						delete pos;
					}
					else
						last = pos;
					pos = next;
				}
				obj = nextObj;
			}
			
			break;
			
		}
	/*	if (CQFLAGS.bGameActive && (U32)param == IDH_VIEWSTARS)
		{
			viewer->set_display_state(1);
		}*/
		break;

	case CQE_ENTERING_INGAMEMODE:
		loadTextures(true);
	//	EnableSpecular(param!=0);
		break;
	case CQE_LEAVING_INGAMEMODE:
		loadTextures(false);
	//	EnableSpecular(param!=0);
		break;
		
	

	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			if (viewer)
				viewer->set_display_state(1);
		}
		break;
	
	}

	return GR_OK;
}

//---------------------------------------------------------------------
//
void Lights::EnableSpecular (BOOL32 bEnable)
{
	if (bEnable)
	{
		CQASSERT(0 && "Disabled");
	//	MAINLIGHT->map = textureID;
	}
	else
	{
		MAINLIGHT->map = 0;
	}
}

BOOL32 __stdcall Lights::New (void)
{
	OnUpdate(doc,NULL,NULL);

	return 1;
}

BOOL32 __stdcall Lights::Load (struct IFileSystem * inFile)
{
	OnUpdate(doc,NULL,NULL);

	return 1;
}

void Lights::loadTextures (bool bEnable)
{
/*	if (bEnable)
	{
		textureID = TMANAGER->CreateTextureFromFile("SpecularHighlight.tga", TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
	}
	else
	{
		TMANAGER->ReleaseTextureRef(textureID);
		textureID = 0;
	}*/
}

BOOL Lights::LightListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	//FieldNode * fieldList = (FieldNode *)GetWindowLong(hwnd, DWL_USER);
	Lights *lightmgr = (Lights *)GetWindowLong(hwnd, DWL_USER);
	char newName[32];

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd,IDC_LIST1);

			SetWindowLong(hwnd, DWL_USER, lParam);

			lightmgr = (Lights *)lParam;

			GameLight *pos = lightmgr->lightList;
			U32 cnt = 0;
			while (pos)
			{	
				SendMessage(hList, LB_INSERTSTRING, cnt, (LPARAM)pos->name);
				SendMessage(hList, LB_SETITEMDATA, cnt, cnt);
				cnt++;
				pos->flag = FALSE;
				pos = pos->next;
			}

			SetFocus(GetDlgItem(hwnd, IDC_LIST1));
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			{
				switch (HIWORD(wParam))
				{
				case LBN_DBLCLK:
					U32 sel;
					HWND hList = GetDlgItem(hwnd,IDC_LIST1);
					GameLight *pos = lightmgr->lightList;

					sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
					U32 i;
					for (i=0;i<sel;i++)
					{
						pos = pos->next;
					}

					pos->View();
					U32 index= SendMessage(hList,LB_GETITEMDATA,sel,0);
					SendMessage(hList,LB_DELETESTRING,sel,0);
					SendMessage(hList,LB_INSERTSTRING,sel,(LPARAM)pos->name);
					SendMessage(hList, LB_SETITEMDATA, sel, index);
			
					break;
				}
			}
			break;
		case IDDELETE:
			{
				U32 sel;
				HWND hList = GetDlgItem(hwnd,IDC_LIST1);
				
				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				SendMessage(hList,LB_DELETESTRING,sel,0);
			}
			break;
		case IDOK:
			{
			//	if (lightmgr->managing)
			//	{
					GameLight *pos = lightmgr->lightList,*last,*next;
					HWND hList = GetDlgItem(hwnd,IDC_LIST1);
					
					U32 cnt=0,sel,listCnt = 0;
					U32 total = SendMessage(hList,LB_GETCOUNT,0,0);
					
					while (pos && cnt < total)
					{
						sel = SendMessage(hList,LB_GETITEMDATA,cnt,0);
						result = SendMessage(hList,LB_GETTEXT,cnt,(LPARAM)newName);
						while (listCnt < sel)
						{
							pos = pos->next;
							listCnt++;
						}
						strcpy(pos->name,newName);
						pos->flag = TRUE;
						cnt++;
					}
					
					last = NULL;
					pos = lightmgr->lightList;
					while (pos)
					{
						next = pos->next;
						if (!pos->flag)
						{
							if (last)
								last->next=pos->next;
							else
								lightmgr->lightList = pos->next;
							
							OBJLIST->RemoveObject(pos);

							delete pos;
						}
						else
							last = pos;
						pos = next;
					}
					//lightmgr->managing = FALSE;
					EndDialog(hwnd,1);
			/*	}
				else 
				{
					MessageBox(hMainWindow,"Your changes have been pre-empted by another user's changes","Bite",MB_OK);
					EndDialog(hwnd,0);
				}*/
			}
			break;
		case IDCANCEL:
		//	lightmgr->managing = FALSE;
			EndDialog(hwnd, 0);
			break;
		}
	}

	return result;
}

void Lights::DeleteLight(GameLight *light)
{
//	if (managing)
//		managing = FALSE;

	GameLight *pos = lightList,*last,*next;
	last = NULL;
	pos = lightList;
	while (pos)
	{
		next = pos->next;
		if (pos == light)
		{
			if (last)
				last->next=pos->next;
			else
				lightList = pos->next;
			
			OBJLIST->RemoveObject(pos);
			return;
		}
		else
 			last = pos;
		pos = next;
	}
}
//---------------------------------------------------------------------------------------------------
//
int Lights::insertSort (CQLight *lightArray[MAX_BEST_LIGHTS], float scoreArray[MAX_BEST_LIGHTS], int numEntries, CQLight * light, float score)
{
	int i;

	for (i = 0; i < MAX_BEST_LIGHTS; i++)
	{
		if (i >= numEntries)
		{
			lightArray[i] = light;
			scoreArray[i] = score;
			numEntries++;
			break;
		}

		if (score > scoreArray[i])
		{
			// push everyone else down
			int j = __min(numEntries, MAX_BEST_LIGHTS-1);		// don't overrun the buffer
			for ( ; j > i ; j--)
			{
				lightArray[j] = lightArray[j-1];
				scoreArray[j] = scoreArray[j-1];
			}

			lightArray[i] = light;
			scoreArray[i] = score;
			numEntries++;
			break;
		}
	}

	return numEntries;
}
//---------------------------------------------------------------------------------------------------
//
int Lights::get_best_lights (const Vector & spot, float radius)
{
//	float r2 = radius * radius;

	CQLight *currentLight = CQLight::pRegList;
	int numEntries = 0;
	float scoreArray[MAX_BEST_LIGHTS];
	GRIDVECTOR gridPos;
	gridPos = spot;

	while (currentLight)
	{
		if (currentLight->IsInfinite())
		{
			// Automatically include infinite lights with a score of 1.0
			numEntries = insertSort(lightArray, scoreArray, numEntries, currentLight, 1.0);
		}
		else
		{
			SINGLE lRange = currentLight->GetRange();
			//lRangeSquared *= lRangeSquared;

			if (lRange == 0)
			{
				// 0 range light. Skip it.
				currentLight = currentLight->pNextReg;
				continue;
			}

			// Check for spot in light range and within check range.
			lRange /= GRIDSIZE;
			// hack until DX9 materials have better light support
			if (true || gridPos - currentLight->getGridPosition() <= lRange)
			{
				Vector lPos;
				currentLight->GetPosition (lPos);
				Vector	L = spot - lPos;
				float dist2 = L.magnitude_squared();
				SINGLE lRangeSquared = lRange*lRange*GRIDSIZE*GRIDSIZE;
				// Light in both ranges, so compute score.
				float baseScore = 1.0f;
				float totalScore = baseScore;

				SINGLE cutoff = currentLight->GetCutoff();
				if(cutoff > 0 && cutoff < 180.0F)
				{
					// The light is not a point light

					// If the spot is within the hemisphere of the spotlight, we include it at a full score
					// Otherwise, we add the light with a lower score
					Transform	tempTrans;
					currentLight->GetTransform(tempTrans);
					Vector direction;
					currentLight->GetDirection(direction);
					Vector lDirInWorld = tempTrans.rotate(direction);

					SINGLE cos_angle = dot_product (L, lDirInWorld);
					if (dist2 < 1e-5)
					{
						// Spot is very close to the light, so the normalization in the code below will
						// probably have numeric error. Just leave the score as-is
					}
					else
					{
						// Adjust the score according to where the spot is relative to the light.
						// Spot lights adjust their contribution based on the size of the penumbra
						float dist = sqrt(dist2);
						float cos_penumbra	= cos(1.3f * cutoff * MUL_DEG_TO_RAD);
						cos_angle /= dist;
						if (cos_angle <= cos_penumbra)
						{
							// Spot is outside of the penumbera. We still add the light to the list, but its
							// score is set low. This is because the test spot is probably the center of the object,
							// and large objects could still be lit by this light even if the object center is not.
							totalScore *= cos_angle;
						}
						else
						{
							// Leave the score as-is. The spot is within the bright part of the spotlight.
						}
					}
				}

				// Adjust the score for distance from the light
				totalScore *= 1.0f - (dist2 / lRangeSquared);

				// Add to the list
				numEntries = insertSort(lightArray, scoreArray, numEntries, currentLight, totalScore);
			}
		}

		currentLight = currentLight->pNextReg;
	}

	return numEntries;
}

void Lights::ActivateAmbientLight(const Vector &_spot)
{
	//setup nebula ambient lights
	Vector spot = _spot;

	CQASSERT(spot.x > -10*GRIDSIZE && spot.x < (64+10)*GRIDSIZE);
	CQASSERT(spot.y > -10*GRIDSIZE && spot.y < (64+10)*GRIDSIZE);

	if (spot.x < 0)
		spot.x = 0;
	if (spot.x > 63*GRIDSIZE)
		spot.x = 63*GRIDSIZE;
	if (spot.y < 0)
		spot.y = 0;
	if (spot.y > 63*GRIDSIZE)
		spot.y = 63*GRIDSIZE;

	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(), map);
	GRIDVECTOR gv;
	//conversion here
	gv.bigGridSquare(spot);
	int fieldID = map->GetFieldID(gv);
	if (fieldID)
	{
		OBJPTR<IField> field;
		IBaseObject *obj;
		
		obj = OBJLIST->FindObject(fieldID);
		obj->QueryInterface(IFieldID,field);
		if (field)
			field->SetAmbientLight(spot,sys_amb);
	}
	else
		SetAmbientLight(sys_amb.r,sys_amb.g,sys_amb.b);
}

void Lights::ActivateBestLights (const Vector &_spot, int maxLights, float radius)
{
	static bool bSkipLights = 0;
	if (bSkipLights) return;
	U64 pretick,posttick;
	QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
	Vector spot = _spot;

	CQASSERT(spot.x > -10*GRIDSIZE && spot.x < (64+10)*GRIDSIZE);
	CQASSERT(spot.y > -10*GRIDSIZE && spot.y < (64+10)*GRIDSIZE);

	if (spot.x < 0)
		spot.x = 0;
	if (spot.x > 63*GRIDSIZE)
		spot.x = 63*GRIDSIZE;
	if (spot.y < 0)
		spot.y = 0;
	if (spot.y > 63*GRIDSIZE)
		spot.y = 63*GRIDSIZE;

	int l;
	for (l=0;l<bestCount;l++)
	{
		if (lightArray[l])
		{
			PIPE->set_light_enable(lightArray[l]->GetEngineID(),FALSE);
		}
	}
	
	bestCount = get_best_lights(spot, radius);
	bestCount = __min(bestCount, maxLights);
	LIGHT->deactivate_all_lights ();
	LIGHT->activate_lights ((ILight **)lightArray, bestCount);
	for (l=0;l<bestCount;l++)
	{
		PIPE->set_light_enable(lightArray[l]->GetEngineID(),TRUE);
	}


	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	OBJLIST->AddTimerTicksToLighting(posttick-pretick);
}

void Lights::DeactivateAllLights()
{
	for (int l=0;l<bestCount;l++)
	{
		if (lightArray[l])
		{
			PIPE->set_light_enable(lightArray[l]->GetEngineID(),FALSE);
			lightArray[l] = 0;
		}
	}
	bestCount = 0;
}

void Lights::RestoreAllLights ()
{
	EVENTSYS->Send(CQE_CAMERA_MOVED,0);
	SetAmbientLight(sys_amb.r,sys_amb.g,sys_amb.b);
}

void Lights::SetSysAmbientLight (U8 r,U8 g,U8 b)
{
	sys_amb.r = r;
	sys_amb.g = g;
	sys_amb.b = b;
}

void Lights::GetSysAmbientLight (U8 &r,U8 &g,U8 &b)
{
	r = sys_amb.r;
	g = sys_amb.g;
	b = sys_amb.b;
}

void Lights::SetAmbientLight (U8 r,U8 g,U8 b)
{
	//light manager lighting
	LIGHT->set_ambient_light(r,g,b);

	if (CQFLAGS.b3DEnabled)
	{
		PIPE->set_render_state(D3DRS_AMBIENT,D3DCOLOR_RGBA(__min(255,r+sys_amb.r),__min(255,g+sys_amb.g),__min(255,b+sys_amb.b),0));
	}
}
//-----------------------------------------------------------------------------
//
HANDLE Lights::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	GameLightArchetype * result = 0;

	if (objClass == OC_LIGHT)
	{
		result = new GameLightArchetype;
		result->data = (BT_LIGHT *) _data;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		DAFILEDESC fdesc = "light.3db";
		COMPTR<IFileSystem> objFile;
		
		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			TEXLIB->load_library(objFile, 0);
		else
			goto Error;
		
		if ((result->archeIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
			goto Error;
		
		char fname[32];
		strcpy(fname,"light.tga");
		if (fname[0])
		{
			result->lightTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
		}
	}
	goto Done;

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 Lights::DestroyArchetype (HANDLE hArchetype)
{
	GameLightArchetype * arch = (GameLightArchetype *)hArchetype;

	EditorStopObjectInsertion(arch->pArchetype);
	delete arch;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * Lights::CreateInstance (HANDLE hArchetype)
{
	GameLightArchetype * arch = (GameLightArchetype *)hArchetype;
	
	GameLight * obj = new ObjectImpl<GameLight>;

	obj->pArchetype = arch->pArchetype;
	obj->objClass = OC_LIGHT;
//	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);

	CQASSERT (arch->archeIndex != -1);
	obj->instanceIndex = ENGINE->create_instance2(arch->archeIndex, obj);
	obj->FRAME_init(*arch);

	if (obj)
	{
		obj->color.r = arch->data->red;
		obj->color.g = arch->data->green;
		obj->color.b = arch->data->blue;
		obj->range = arch->data->range*GRIDSIZE;
		obj->infinite = arch->data->bInfinite;
		obj->cutoff = arch->data->cutoff;
		obj->direction = arch->data->direction;
		obj->makeAmbient(arch->data->bAmbient);
		obj->lightTex = arch->lightTex;

		if (!lightList)
		{
			lightList = obj;
			lightList->next = NULL;
		}
		else
		{
			obj->next = lightList;
			lightList = obj;
		}
		obj->factory = this;
//		obj->setSystem(SECTOR->GetCurrentSystem());  (jy)
		strcpy(obj->name, ARCHLIST->GetArchName(arch->pArchetype));
		
		MGlobals::EnableGlobalLighting(false);
		lights[0].set_On(false);
		lights[1].set_On(false);
		LIGHTS->SetSysAmbientLight(0,0,0);
		LIGHTS->SetAmbientLight(0,0,0);
		EVENTSYS->Send(CQE_SYSTEM_CHANGED,(void *)SECTOR->GetCurrentSystem());
		
	}

	return obj;
}
//-------------------------------------------------------------------
//
void Lights::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	GameLightArchetype * arch = (GameLightArchetype *)hArchetype;
	EditorStartObjectInsertion(arch->pArchetype, info);
}


int ILights::GetNextEngineID()
{
	for (int i=1;i<MAX_ENGINE_LIGHTS;i++)
	{
		if (engineLightArray[i]==false)
		{
			engineLightArray[i]=true;
			return i;
		}
	}
	return nextEngineID++;
}

void ILights::ReleaseEngineID(int engineID)
{
	if (engineID > 0 && engineID < MAX_ENGINE_LIGHTS)
		engineLightArray[engineID] = false;
}

struct _lights : GlobalComponent
{
	DAComponent<Lights>	* lights;

	virtual void Startup (void)
	{
		LIGHTS = lights = new DAComponent<Lights>;
		AddToGlobalCleanupList((IDAComponent **) &lights);

		MAINLIGHT = &lights->lights[0];
		CAMERALIGHT = &lights->lights[1];
	}

	virtual void Initialize (void)
	{
		if (lights->CreateViewer() == 0)
			CQBOMB0("Viewer could not be created.");

		if( OBJLIST )
			OBJLIST->DEBUG_IncLightCount();
	}
};

static _lights lights;

//--------------------------------------------------------------------------//
//----------------------------END Light.cpp---------------------------------//
//--------------------------------------------------------------------------//
