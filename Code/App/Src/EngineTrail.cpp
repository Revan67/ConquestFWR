//--------------------------------------------------------------------------//
//                                                                          //
//                                 EngineTrail.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $ $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "IEngineTrail.h"
#include "DEngineTrail.h"
#include "TObject.h"
#include "IHardpoint.h"
#include "ArchHolder.h"
#include "Objlist.h"
#include "Startup.h"
#include "Camera.h"
#include <DEffectOpts.h>
#include "TManager.h"

#include <FileSys.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <IConnection.h>


#define NUM_BATCH_RENDER_STAGES 2
#define BATCH_RENDER_TRAIL 0
#define BATCH_RENDER_GLOW 1
#define MIN_LOD 0.0//0.10
#define BUFFER_LOD 0.0//0.20

struct EngineTrailArchetype
{
	INSTANCE_INDEX textureID;
	INSTANCE_INDEX engineTextureID;
	BT_ENGINETRAIL * pData;

	virtual ~EngineTrailArchetype()
	{
		TMANAGER->ReleaseTextureRef(engineTextureID);
		TMANAGER->ReleaseTextureRef(textureID);
	}
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct _NO_VTABLE EngineTrail : public IBaseObject, IEngineTrail
{

	BEGIN_MAP_INBOUND(EngineTrail)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IEngineTrail)
	END_MAP()

	
	//------------------------------------------

	EngineTrailArchetype * archetype;

	S32 ownerIndex;
	INSTANCE_INDEX engineIndex;
	HardpointInfo hardpointinfo;
	bool bHasHardpoint:1;
	bool bDecayMode:1;
	IBaseObject * owner;
	U32 numSteps;

	//instance based alterations for upgrades
	U32 segments;
//	SINGLE timePerSegment;

	Vector * trailPoints;
	Vector * trailPointsLeft;
	Vector * trailPointsRight;
	//Vector *trailPoints;

	S32 start;
	S32 stop;
	SINGLE pointTime;

	//------------------------------------------

	EngineTrail (void)
	{
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~EngineTrail (void);

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void PhysUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void Reset (void)
	{
		start = 0;
		stop = -1;
		pointTime = 0;
	}
	//IEngineTrail

	virtual BOOL32 InitEngineTrail (IBaseObject * _owner, S32 _ownerIndex);

	virtual void SetLengthModifier(SINGLE percent);

	virtual void BatchRender (U32 state);

	virtual void SetupBatchRender (U32 stage);

	virtual void FinishBatchRender (U32 stage);

//	virtual void PhysicalTimeUpdate (SINGLE dt);

	virtual U32 GetBatchRenderStateNumber (void);


	virtual INSTANCE_INDEX GetTextureID (void)
	{
		return archetype->textureID;
	}

	virtual void SetDecayMode(void)
	{
		bDecayMode = true;
	}

	//EngineTrail
	void init(EngineTrailArchetype * _archetype);

//	BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);

//	S32 findChild (const char * pathname, INSTANCE_INDEX parent);
};

//----------------------------------------------------------------------------------
//
EngineTrail::~EngineTrail (void)
{
	if(trailPoints) 
	{
		delete [] trailPoints;
		delete [] trailPointsLeft;
		delete [] trailPointsRight;
		trailPoints = 0;
		trailPointsRight = 0;
		trailPointsLeft = 0;
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 EngineTrail::Update (void)
{
	if (!CQEFFECTS.bWeaponTrails)
		return 0;

	if (!bVisible)
		PhysUpdate(ELAPSED_TIME);
	
	//assuming parent is visible;
	if (stop != -1)
	{
		RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
		RECT rect;
		CAMERA->PointToScreen(trailPoints[start],&(rect.left),&(rect.top));
		CAMERA->PointToScreen(trailPoints[stop],&(rect.right),&(rect.bottom));
		S32 tmp;
		if(rect.top > rect.bottom)
		{
			tmp = rect.top; rect.top = rect.bottom; rect.bottom = tmp;
		}
		if(rect.left > rect.right)
		{
			tmp = rect.left; rect.left = rect.right; rect.right = tmp;
		}
		bVisible = RectIntersects(rect, screenRect);

	}
	
	return (start != stop);
}
//----------------------------------------------------------------------------------
//
/*void EngineTrail::PhysicalTimeUpdate (SINGLE dt)
{
	pointTime += dt;
}*/
//----------------------------------------------------------------------------------
//
void EngineTrail::PhysicalUpdate (SINGLE dt)
{
	if (!CQEFFECTS.bWeaponTrails)
		return;

	if (bVisible)
		PhysUpdate(dt);
}

void EngineTrail::PhysUpdate (SINGLE dt)
{
	Vector pos;
	if(bHasHardpoint)
	{
		TRANSFORM hpTrans;
		hpTrans.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
		hpTrans = ENGINE->get_transform(engineIndex).multiply(hpTrans);

		pos = hpTrans.translation;
	}
	else
	{
		pos = owner->GetPosition();
	}

		if(stop == -1)//we are just beginning
	{
		numSteps = 1;
		start = 0;
		stop = 0;
		trailPointsLeft[stop] = trailPointsRight[stop] = trailPoints[stop] = pos;
	}
	else if(BUFFER_LOD > LODPERCENT || bDecayMode)
	{
		pointTime += dt;
		U32 numPoints = (U32)(pointTime/archetype->pData->timePerSegment);
		while(numPoints && (stop!= start))
		{
			start = (start+1)%(segments+1);
			--numPoints;
			pointTime -= archetype->pData->timePerSegment;
		}
	}
	else
	{
		if ((pos.x != trailPoints[stop].x) ||
			(pos.y != trailPoints[stop].y) ||
			(pos.z != trailPoints[stop].z))
		{
			pointTime +=dt;
			U32 numPoints = (U32)(pointTime/archetype->pData->timePerSegment);
			if(!numPoints)
			{
				//	stop = (stop+1)%(segments+1);
				if(start == stop)
				{
					stop = (stop+1)%(segments+1);
					}/*else{
					 ++numSteps;
			}*/
				trailPoints[stop] = pos;
			}
			U32 pointsLaid = 1;
			Vector oldPos = trailPoints[stop];
			while(numPoints >= pointsLaid)
			{
				Vector midPoint = ((oldPos-pos)*(1.0-(((SINGLE)pointsLaid)/((SINGLE)numPoints))))+pos;
				if((midPoint.x != trailPoints[stop].x) ||
					(midPoint.y != trailPoints[stop].y) ||
					(midPoint.z != trailPoints[stop].z))
				{
					stop = (stop+1)%(segments+1);
					if(start == stop)
					{
						start = (start+1)%(segments+1);
					}else{
						++numSteps;
					}
					trailPoints[stop] = midPoint;
				}
				++pointsLaid;
				pointTime -= archetype->pData->timePerSegment;
			}
			if(start != stop)
			{
				//compute side points
				SINGLE width = 0;
				SINGLE widthStep = 0;
				//changes by rmarr to fix dumb looking trails
				int activeSegments = stop-start;
				if (activeSegments <= 0)
					activeSegments += segments+1;
				//
				if(archetype->pData->tapperType == EngineTrailStraight)
				{
					width = archetype->pData->width * 0.5;
					widthStep = 0;
				}
				else if(archetype->pData->tapperType == EngineTrailTapperZero)
				{
					widthStep = archetype->pData->width/(activeSegments+2);
					width = widthStep;
				}
				else if(archetype->pData->tapperType == EngineTrailBleed)
				{
					widthStep = -archetype->pData->tapperMod;
					width = (-widthStep)*(activeSegments)+archetype->pData->width;
				}
				else
				{
					CQASSERT(false);
				}
				Vector *prevPos = &(trailPoints[start]);
				trailPointsLeft[start] = trailPointsRight[start] = (*prevPos);
				int index = (start+1)%(segments+1);

				//smooth factor
				SINGLE factor = -widthStep*(pointTime/archetype->pData->timePerSegment);
				while(true)
				{
					Vector * currentPos = &(trailPoints[index]);
					Vector diffVect = (*currentPos) - (*prevPos);
					SINGLE mag = diffVect.fast_magnitude();
					if(mag)
					{
						diffVect = diffVect/mag;
					}
					else
						diffVect = Vector(0,1,0);
					Vector sideVect;
					if((diffVect.x != 0) || (diffVect.y != 0))
					{
						sideVect = cross_product(diffVect,Vector(0,0,1));
					}
					else
					{
						sideVect = cross_product(diffVect,Vector(1,0,0));
					}
					sideVect.normalize();
					sideVect = sideVect*(width+factor);
					trailPointsLeft[index] = (*currentPos)+sideVect;
					trailPointsLeft[index].z -= (width+factor)*0.3;//archetype->pData->width;
					trailPointsRight[index] = (*currentPos)-sideVect;
					trailPointsRight[index].z -= (width+factor)*0.3;//archetype->pData->width;
					
					if(index == stop)
						break;
					index = (index+1)%(segments+1);
					width = width+widthStep;
					prevPos = currentPos;
				}
			}
			else
			{
				trailPointsLeft[start] = trailPointsRight[start] =trailPoints[start];
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
void EngineTrail::Render (void)
{
	if(LODPERCENT < MIN_LOD || CQEFFECTS.bWeaponTrails == 0)
	{
		Reset();
		return;
	}

	if(bVisible && (start!= stop) && (stop != -1))
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_state(RPR_STATE_ID,archetype->textureID);
		CAMERA->SetModelView();
//		BATCH->set_texture_stage_texture(0, archetype->textureID);
		SetupDiffuseBlend(archetype->textureID,FALSE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		
		S32 prevIndex = start;
		S32 index = (start+1)%(segments+1);
		//		SINGLE lodIndex = index;
		//trying to defeat the disappearing alpha poly on the end
		S32 alphaStep = archetype->pData->colorMod.alpha/(segments);  //was segments+1
		S32 alpha = alphaStep*((segments+1)-numSteps);  
		//correct alpha for remaining time - rmarr
		alpha -= alphaStep*((pointTime/archetype->pData->timePerSegment));
		
		
		
		
		PB.Begin(PB_QUADS);
		while(true)//breaks out later
		{
			S32 thisAlph = alpha;
			S32 alphaPlus = alpha+alphaStep;
			if (thisAlph < 0)
				thisAlph = 0;
			CQASSERT(alphaPlus >= 0);
			if(archetype->textureID)
			{
				if(CQRENDERFLAGS.bNoPerVertexAlpha)
				{
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
					PB.TexCoord2f(0.5,0);   PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
					PB.TexCoord2f(0,0);   PB.Vertex3f(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
					PB.TexCoord2f(0,1);   PB.Vertex3f(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
					PB.TexCoord2f(0.5,1);   PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
					
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
					PB.TexCoord2f(0.5,0);   PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
					PB.TexCoord2f(1,0);   PB.Vertex3f(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
					PB.TexCoord2f(1,1);   PB.Vertex3f(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
					PB.TexCoord2f(0.5,1);   PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
				}
				else
				{
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alphaPlus);
					PB.TexCoord2f(0.5,0);   PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
					PB.TexCoord2f(0,0);   PB.Vertex3f(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
					PB.TexCoord2f(0,1);   PB.Vertex3f(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
					PB.TexCoord2f(0.5,1);   PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
					
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alphaPlus);
					PB.TexCoord2f(0.5,0);   PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
					PB.TexCoord2f(1,0);   PB.Vertex3f(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
					PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
					PB.TexCoord2f(1,1);   PB.Vertex3f(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
					PB.TexCoord2f(0.5,1);   PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
				}
			}
			else if(CQRENDERFLAGS.bNoPerVertexAlpha)
			{
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
				PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
				PB.Vertex3f(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
				PB.Vertex3f(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
				PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
				
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
				PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
				PB.Vertex3f(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
				PB.Vertex3f(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
				PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
			}
			else
			{
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alphaPlus);
				PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
				PB.Vertex3f(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
				PB.Vertex3f(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph);
				PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
				
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alphaPlus);
				PB.Vertex3f(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
				PB.Vertex3f(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
				PB.Vertex3f(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
				PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,thisAlph );
				PB.Vertex3f(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
			}
			alpha += alphaStep;
			if(alpha > 255)
				alpha = 255;
			
			if(index == stop)
				break;
			prevIndex = index;
			index = (index+1)%(segments+1);
		}
		PB.End();
		if(LODPERCENT < BUFFER_LOD)
		{
			Reset();
			goto Done;
		}

		if(archetype->engineTextureID && bDecayMode == 0)
		{
//			BATCH->set_texture_stage_texture(0, archetype->engineTextureID);
			BATCH->set_state(RPR_STATE_ID,archetype->engineTextureID);
			SetupDiffuseBlend(archetype->engineTextureID,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			PB.Begin(PB_QUADS);
			Vector epos = trailPoints[stop];		
			
			Vector cpos (CAMERA->GetPosition());
			
			Vector look (epos - cpos);
			
			Vector k = look.normalize();

			Vector tmpUp(epos.x,epos.y,epos.z+50000);

			Vector j (cross_product(k,tmpUp));
			j.normalize();

			Vector i (cross_product(j,k));

			i.normalize();

			TRANSFORM trans;
			trans.set_i(i);
			trans.set_j(j);
			trans.set_k(k);

			i = trans.get_i();
			j = trans.get_j();

			Vector v[4];
			SINGLE size = archetype->pData->engineGlowWidth;
			v[0] = epos - i*size- j*size;
			v[1] = epos + i*size - j*size;
			v[2] = epos + i*size + j*size;
			v[3] = epos - i*size + j*size;
			PB.Color4ub(archetype->pData->engineGlowColorMod.red,
				archetype->pData->engineGlowColorMod.blue,
				archetype->pData->engineGlowColorMod.green,
				archetype->pData->engineGlowColorMod.alpha);
			PB.TexCoord2f(0,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
			PB.TexCoord2f(1,0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
			PB.TexCoord2f(1,1);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
			PB.TexCoord2f(0,1);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);

			PB.End();
		}
	}

Done:
	BATCH->set_state(RPR_STATE_ID,0);
}
//----------------------------------------------------------------------------------
//
void EngineTrail::BatchRender (U32 stage)
{
	if(LODPERCENT < MIN_LOD || CQEFFECTS.bWeaponTrails == 0)
	{
		Reset();
		return;
	}
	switch(stage)
	{
	case BATCH_RENDER_TRAIL:
		{
			if(bVisible && (start!= stop) && (stop != -1))
			{
				S32 prevIndex = start;
				S32 index = (start+1)%(segments+1);
				U32 alphaStep = archetype->pData->colorMod.alpha/(segments+1);
				U32 alpha = alphaStep*((segments+1)-numSteps);
			//	memcpy(trailPoints,trailPoints,sizeof(Vector)*(segments+1));
			//	memcpy(trailPointsLeft,trailPointsLeft,sizeof(Vector)*(segments+1));
			//	memcpy(trailPointsRight,trailPointsRight,sizeof(Vector)*(segments+1));
				Vector *trailPt=&trailPoints[index];
				Vector *trailPtPrev=&trailPoints[prevIndex];
				int color,color_step;
				while(true)//breaks out later
				{
					color = PB.MakeColor4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
					color_step = PB.MakeColor4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha+alphaStep);

					if(archetype->textureID)
					{
					/*	if(CQRENDERFLAGS.bNoPerVertexAlpha)
						{
							PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
							PB.TexCoord2f(0,0);   PB.Vertex3f_NC(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
							PB.TexCoord2f(0,1);   PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				

							PB.TexCoord2f(0.5,1);   PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
							PB.TexCoord2f(0,1);   PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				



							PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
							PB.TexCoord2f(1,0);   PB.Vertex3f_NC(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
							PB.TexCoord2f(1,1);   PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);

							PB.TexCoord2f(0.5,1);   PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
							PB.TexCoord2f(1,1);   PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);


						}
						else*/
						{
							PB.Begin(PB_TRIANGLES,12);
							PB.Color(color_step);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(*trailPt);
							PB.TexCoord2f(0,0);   PB.Vertex3f_NC(trailPointsLeft[index]);
							PB.Color(color);
							PB.TexCoord2f(0,1);   PB.Vertex3f_NC(trailPointsLeft[prevIndex]);				
							
							PB.TexCoord2f(0.5,1);   PB.Vertex3f_NC(*trailPtPrev);
							PB.Color(color_step);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(*trailPt);
							PB.Color(color);
							PB.TexCoord2f(0,1);   PB.Vertex3f_NC(trailPointsLeft[prevIndex]);				


							PB.Color(color_step);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(*trailPt);
							PB.TexCoord2f(1,0);   PB.Vertex3f_NC(trailPointsRight[index]);
							PB.Color(color);
							PB.TexCoord2f(1,1);   PB.Vertex3f_NC(trailPointsRight[prevIndex]);
							
							PB.TexCoord2f(0.5,1);   PB.Vertex3f_NC(*trailPtPrev);				
							PB.Color(color_step);
							PB.TexCoord2f(0.5,0);   PB.Vertex3f_NC(*trailPt);
							PB.Color(color);
							PB.TexCoord2f(1,1);   PB.Vertex3f_NC(trailPointsRight[prevIndex]);
							PB.End();
						}
					}
					/*else if(CQRENDERFLAGS.bNoPerVertexAlpha)
					{
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Vertex3f_NC(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
						PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
					
						PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				


						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Vertex3f_NC(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
						PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
						
						PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);

					}
					else
					{
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha+alphaStep);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
						PB.Vertex3f_NC(trailPointsLeft[index].x,trailPointsLeft[index].y,trailPointsLeft[index].z);
						PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				
						
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha);
						PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha+alphaStep);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
						PB.Vertex3f_NC(trailPointsLeft[prevIndex].x,trailPointsLeft[prevIndex].y,trailPointsLeft[prevIndex].z);				


						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha+ alphaStep);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
						PB.Vertex3f_NC(trailPointsRight[index].x,trailPointsRight[index].y,trailPointsRight[index].z);
						PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);
		
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha );
						PB.Vertex3f_NC(trailPoints[prevIndex].x,trailPoints[prevIndex].y,trailPoints[prevIndex].z);				
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,alpha+ alphaStep);
						PB.Vertex3f_NC(trailPoints[index].x,trailPoints[index].y,trailPoints[index].z);
						PB.Color4ub(archetype->pData->colorMod.red,archetype->pData->colorMod.green,archetype->pData->colorMod.blue,0);
						PB.Vertex3f_NC(trailPointsRight[prevIndex].x,trailPointsRight[prevIndex].y,trailPointsRight[prevIndex].z);


					}*/
					alpha += alphaStep;
					if(alpha > 255)
						alpha = 255;

					if(index == stop)
						break;
					prevIndex = index;
					index = (index+1)%(segments+1);
					trailPt = &trailPoints[index];
					trailPtPrev = &trailPoints[prevIndex];
				}
			}
			break;
		}
	case BATCH_RENDER_GLOW:
		{
			if(LODPERCENT < BUFFER_LOD)
			{
				Reset();
				return;
			}

			if(bVisible && archetype->engineTextureID)//should have been set in render state 1
			{
				Vector epos = trailPoints[stop];		
				
				Vector cpos (CAMERA->GetPosition());
				
				Vector look (epos - cpos);
				
				Vector k = look.normalize();

				Vector tmpUp(epos.x,epos.y,epos.z+50000);

				Vector j (cross_product(k,tmpUp));
				j.normalize();

				Vector i (cross_product(j,k));

				i.normalize();

				TRANSFORM trans;
				trans.set_i(i);
				trans.set_j(j);
				trans.set_k(k);

				i = trans.get_i();
				j = trans.get_j();

				Vector v[4];
				SINGLE size = archetype->pData->engineGlowWidth;
				v[0] = epos - i*size- j*size;
				v[1] = epos + i*size - j*size;
				v[2] = epos + i*size + j*size;
				v[3] = epos - i*size + j*size;
				PB.Color4ub(archetype->pData->engineGlowColorMod.red,
					archetype->pData->engineGlowColorMod.blue,
					archetype->pData->engineGlowColorMod.green,
					archetype->pData->engineGlowColorMod.alpha);
				PB.TexCoord2f(0,0);   PB.Vertex3f(v[0].x,v[0].y,v[0].z);
				PB.TexCoord2f(1,0);   PB.Vertex3f(v[1].x,v[1].y,v[1].z);
				PB.TexCoord2f(1,1);   PB.Vertex3f(v[2].x,v[2].y,v[2].z);
				PB.TexCoord2f(0,1);   PB.Vertex3f(v[3].x,v[3].y,v[3].z);

			}
			break;
		}
	}
}
//----------------------------------------------------------------------------------
//
void EngineTrail::SetupBatchRender (U32 stage)
{
	if(LODPERCENT < MIN_LOD || CQEFFECTS.bWeaponTrails == 0)
		return;
	switch(stage)
	{
	case BATCH_RENDER_TRAIL:
		{
			BATCH->set_state(RPR_BATCH,true);
			BATCH->set_state(RPR_STATE_ID,archetype->textureID);
		//	BATCH->set_texture_stage_texture(0, archetype->textureID);
			SetupDiffuseBlend(archetype->textureID,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	//		PB.Begin(PB_TRIANGLES,(segments+1)*12);
			break;
		}
	case BATCH_RENDER_GLOW:
		{
			if(LODPERCENT < BUFFER_LOD)
			{
				Reset();
				return;
			}

			if(archetype->engineTextureID)
			{
				BATCH->set_state(RPR_BATCH,true);
				BATCH->set_state(RPR_STATE_ID,archetype->engineTextureID);
			//	BATCH->set_texture_stage_texture(0, archetype->engineTextureID);
				SetupDiffuseBlend(archetype->engineTextureID,TRUE);
				BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
				BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
				BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
				PB.Begin(PB_QUADS);
			}
			break;
		}
	}
}
//----------------------------------------------------------------------------------
//
void EngineTrail::FinishBatchRender (U32 stage)
{
	if(LODPERCENT < MIN_LOD || CQEFFECTS.bWeaponTrails == 0)
		return;
	switch(stage)
	{
	case BATCH_RENDER_TRAIL:
		{
	//		PB.End();
		//	BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_state(RPR_STATE_ID,0);
			break;
		}
	case BATCH_RENDER_GLOW:
		{
			if(LODPERCENT < BUFFER_LOD)
			{
				Reset();
				BATCH->set_state(RPR_STATE_ID,0);
				return;
			}
			if(archetype->engineTextureID)
			{
				PB.End();
				//BATCH->set_state(RPR_BATCH,TRUE);
				BATCH->set_state(RPR_STATE_ID,0);
			}
			break;
		}
	}
}
//----------------------------------------------------------------------------------
//
U32 EngineTrail::GetBatchRenderStateNumber (void)
{
	return NUM_BATCH_RENDER_STAGES;
}
//----------------------------------------------------------------------------------
//
BOOL32 EngineTrail::InitEngineTrail (IBaseObject * _owner, S32 _ownerIndex)
{
	owner = _owner;
	ownerIndex = _ownerIndex;
	start = 0;
	stop = -1;
	pointTime = 0.0;

	segments = archetype->pData->segments;
	trailPoints = new Vector[segments+1];
	trailPointsLeft = new Vector[segments+1];
	trailPointsRight = new Vector[segments+1];

	if (archetype->pData->hardpoint[0])
		bHasHardpoint = (FindHardpoint(archetype->pData->hardpoint, engineIndex, hardpointinfo, ownerIndex) != 0);
	else
		bHasHardpoint = false;

	return true;
}
//----------------------------------------------------------------------------------
//
void EngineTrail::SetLengthModifier(SINGLE percent)
{
	segments = archetype->pData->segments*percent;
	delete [] trailPoints;
	delete [] trailPointsLeft;
	delete [] trailPointsRight;
	trailPoints = new Vector[segments+1];
	trailPointsLeft = new Vector[segments+1];
	trailPointsRight = new Vector[segments+1];
}
//----------------------------------------------------------------------------------
//
void EngineTrail::init(EngineTrailArchetype * _archetype)
{
	archetype = _archetype;
};

//---------------------------------------------------------------------------
//
/*S32 EngineTrail::findChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = MODEL->get_child(parent, child)) != -1)
	{
		if (MODEL->is_named(child, buffer))
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}
//----------------------------------------------------------------------------------------
//
BOOL32 EngineTrail::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	*ptr++ = 0;
	if (buffer[0])
		index = findChild(buffer, parent);
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}
*/
//----------------------------------------------------------------------------------
//---------------------------EngineTRail Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------


struct DACOM_NO_VTABLE EngineTrailManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(EngineTrailManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//EngineTrailManager methods

	EngineTrailManager (void) 
	{
	}

	~EngineTrailManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// EngineTrailManager methods

EngineTrailManager::~EngineTrailManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

}
//--------------------------------------------------------------------------
//
void EngineTrailManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE EngineTrailManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_EFFECT)
	{
		BASE_EFFECT * base = (BASE_EFFECT *)data;
		if(base->fxClass == FX_ENGINETRAIL)
		{
			EngineTrailArchetype * result= new EngineTrailArchetype;
			BT_ENGINETRAIL * etData = (BT_ENGINETRAIL *)data;
			if (etData->texture[0])
				result->textureID = TMANAGER->CreateTextureFromFile(etData->texture, TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			else
				result->textureID = 0;
			if (etData->engineGlowTexture[0])
				result->engineTextureID = TMANAGER->CreateTextureFromFile(etData->engineGlowTexture, TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			else
				result->engineTextureID = 0;
			result->pData = etData;
			return result;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 EngineTrailManager::DestroyArchetype(HANDLE hArchetype)
{
	EngineTrailArchetype *deadguy = (EngineTrailArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * EngineTrailManager::CreateInstance(HANDLE hArchetype)
{
	EngineTrailArchetype *pEngineTrailArch = (EngineTrailArchetype *)hArchetype;
	EngineTrail * engineTrail = new ObjectImpl<EngineTrail>;

	engineTrail->init(pEngineTrailArch);	

	return engineTrail;
}
//--------------------------------------------------------------------------
//
void EngineTrailManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

//----------------------------------------------------------------------------------------------
//
struct _engineTrailGC : GlobalComponent
{
	struct EngineTrailManager *engineTrailMgr;

	virtual void Startup (void)
	{
		engineTrailMgr = new DAComponent<EngineTrailManager>;
		AddToGlobalCleanupList((IDAComponent **) &engineTrailMgr);
	}

	virtual void Initialize (void)
	{
		engineTrailMgr->init();
	}
};

static _engineTrailGC etgc;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End EngineTrail.cpp------------------------------
//---------------------------------------------------------------------------
