#ifndef TOBJDAMAGE_H
#define TOBJDAMAGE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjDamage.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjDamage.h 127   10/13/00 12:07p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//


#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef IBLAST_H
#include "IBlast.h"
#endif

#ifndef ANIM2D_H
#include "Anim2d.h"
#endif

//for extents

#ifndef COLLISION_H
#include "Collision.h"
#endif

//#ifndef CMESH_H
//#include "CMesh.h"
//#endif

#ifndef MYVERTEX_H
#include "MyVertex.h"
#endif


#ifndef SIMPLEMESH_H
#include "SimpleMesh.h"
#endif

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef RENDERER_H
#include "Renderer.h"
#endif

#ifndef SFX_H
#include "Sfx.h"
#endif

#ifndef CQBATCH_H
#include "CQBatch.h"
#endif

#ifndef ISHIPDAMAGE_H
#include "IShipDamage.h"
#endif

#ifndef DAMAGE_H
#include "Damage.h"
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif
//#ifndef MATERIAL_H
//#include "Material.h"
//#endif


#define NUM_FIRES 10
//#define BBOX_MAX__CHILDS 12

#define THETA_FACTOR 260

#define SHIELD_SCALE 1.4

struct Fire
{
	INSTANCE_INDEX index;
	//Vector pos;
	TRANSFORM trans;
	U16 timer;
};

/*enum FACE_STATE
{
	FS_HIDDEN,
	FS_BUILDING,
	FS_VISIBLE
};*/

#define POLYS_PER_HOLE 32

/*struct TexCoord
{
	SINGLE u,v;
};*/

struct DamageRecord //: DamageSave
{
	SINGLE *polyDist;//[POLYS_PER_HOLE];
	IEffectChannel *ec;

protected:
	U8 poly_cnt;
	U8 buff_size;

public:

	DamageRecord()
	{
		poly_cnt = 0;
	}

	~DamageRecord()
	{
		delete polyDist;
	}

	/*void SetPolyCnt(U8 new_poly_cnt)
	{
#if 0
		CQASSERT(new_poly_cnt <= 80);
#endif

		if (buff_size >= new_poly_cnt)
		{
			poly_cnt = new_poly_cnt;
			return;
		}
		
		int old_buff_size = buff_size;
		while (buff_size < new_poly_cnt)
			buff_size += 16;
		
		U32 *nPolyList;
		SINGLE *nPolyDist;
		TexCoord *nTexCoord;
		
		poly_cnt = new_poly_cnt;

		nPolyList = new U32[buff_size];
		nPolyDist = new SINGLE[buff_size];
		nTexCoord = new TexCoord[buff_size*3];

		memcpy(nPolyList,polyList,sizeof(U32)*old_buff_size);
		memcpy(nPolyDist,polyDist,sizeof(SINGLE)*old_buff_size);
		memcpy(nTexCoord,texCoord,sizeof(TexCoord)*old_buff_size*3);

		delete polyList;
		delete polyDist;
		delete texCoord;

		polyList = nPolyList;
		polyDist = nPolyDist;
		texCoord = nTexCoord;
	}

	U8 GetPolyCnt()
	{
		return poly_cnt;
	}*/
};

struct TexGenNode
{
	SINGLE dist_u,dist_v;
	S32 vertexRef;
	bool bAnchored:1;
	bool bDown:1;
	Vector genVec;
};

struct TexFaceNode
{
	S32 vertexRef[3];
	S32 nodeRef[3];
	bool bAnchored:1;
	//bool bDown:1;
};



static COLORREF polyColor[POLYS_PER_HOLE] = 
{
	RGB(255,0,0),
	RGB(0,255,0),  
	RGB(0,0,255),	 
	RGB(255,255,0), 
	RGB(255,0,255),	 
	RGB(255,255,255),
	RGB(0,255,255),
	RGB(0,0,0),
//	RGB(140,255,140)
};	   

//

// Sutherland-Cohen line clip algorithm.

#define TOP		0x0001
#define BOTTOM	0x0002
#define RIGHT	0x0004
#define LEFT	0x0008

//

typedef unsigned int OutCode;

//
#define NUM_SHIELD_HITS 8

void GetAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize);

//static TXM_ID shieldTexID = INVALID_TXM_ID;
//static AnimArchetype *shieldArch = 0;

#define ObjectDamage _Cod
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectDamage : public Base, IShipDamage, DAMAGE_SAVELOAD
{
	struct PostRenderNode	postRenderNode;
	struct UpdateNode		updateNode;
	struct PhysUpdateNode	physUpdateNode;
	struct InitNode			initNode;
	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct ResolveNode		resolveNode;

	typename typedef Base::INITINFO DAMAGEINITINFO;
	typename typedef Base::SAVEINFO DMGSAVEINFO;
	
	Fire fire[NUM_FIRES];
	int numFires;
//	ChildBlast *childBlastList;

	SINGLE threshold;
	ARCHETYPE_INDEX smoke_archID;

	DamageRecord damageRecord[DAMAGE_RECORDS];
	TexGenNode texGen[POLYS_PER_HOLE*3];
	TexFaceNode texFace[POLYS_PER_HOLE];

	SINGLE sizeFactor;
	SINGLE dmgTimer;
	struct AnimArchetype * damageAnimArch;
	S32 numDamages;
	S32 val;
//	SINGLE lastDamage;
	PARCHETYPE pDamageBlast;
	PARCHETYPE pSparkBlast;

	//new shield Hit stuff
	ShieldMap *shieldHitPolys[NUM_SHIELD_HITS];
	Vector *vertex_normals;
//	BaseExtent *m_extent;
	SMesh *smesh;
	Vector shieldScale;
	struct AnimArchetype *shieldAnimArch,*shieldFizzAnimArch;
	S32 numShieldHits;
	HSOUND shieldSound[NUM_SHIELD_HITS];
	HSOUND fizzSound;
	enum SFX::ID shieldSoundID, fizzSoundID;
	SINGLE shieldDownTimer;
	U32 damageTexID;

	U16 timer;
	SINGLE coarseTimer;

	bool bShieldsUp:1;
	bool bUseSMeshAsShield:1;

	ObjectDamage (void);
	~ObjectDamage (void);

	/* IShipDamage */
	virtual U32 GetNumDamageSpots ();
	
	virtual void GetNextDamageSpot (Vector & vect, Vector & dir);
	
	virtual void FixDamageSpot ();
	
	virtual struct SMesh *GetShieldMesh ()
	{
		return smesh;
	}

	virtual void FancyShieldRender()
	{
		if(shieldDownTimer <= 0)
			shieldDownTimer = 1.0;
	}
	
	/* ObjectDamage methods */
//	void freeArrays();
	void saveDamage (DMGSAVEINFO & saveStruct);
	void loadDamage (DMGSAVEINFO & saveStruct);
	void initDamage (const DAMAGEINITINFO & data);
	void damagePreRender (void);
	void damagePostRender (void);
	BOOL32 updateDamage (void);
	void physUpdateDamage (SINGLE dt);
	void resolveDamage ();
	void CreateShieldHit (const Vector & pos, const Vector &dir,Vector &collide_point,U32 damage);
	void GenerateShieldHit(const Vector & pos,const Vector &dir,Vector &collide_point,U32 damage);
//	void GenerateNormals(CollisionMesh *cmesh);
	void renderShieldHits();
	void renderShield();
	void renderShieldDown();
	void renderShieldUp(int texID);
	void getScale();
	
	
	//void renderMeshChunk (U32 texID,S32 attrib);
	void renderDamageSpots ();
//	void DamageFace (S32 childID,S32 faceID);
	void DamageSpot (U32 spot);
//	void GenerateTexMapping (U8 spot,S32 numFaces,S32 minDistPlace);
	void RegisterDamage(Vector pos,U32 amount);
	
#define XBOUND 1.03f
#define YBOUND 1.03f
	
	static inline OutCode ComputeCode(float x, float y)//, int vx0, int vy0, int vx1, int vy1 )
	{
		OutCode code = 0;
		if (y > YBOUND) {
			code |= BOTTOM;
		}
		else if (y < 0) {
			code |= TOP;
		}
		if (x > XBOUND) {
			code |= RIGHT;
		}
		else if (x < 0) {
			code |= LEFT;
		}
		return code;
	}
	
	static bool pt_is_in_rect (TexCoord p0)//, U32 vx, U32 vy, U32 vw, U32 vh)
	{
		if (p0.u > 0 && p0.u < XBOUND && p0.v > 0 && p0.v < YBOUND)
			return TRUE;
		
		return FALSE;
	}
	
	static bool clipped( TexCoord p0, TexCoord p1)//, U32 vx, U32 vy, U32 vw, U32 vh )
	{
		bool accept = false;
		bool done = false;
		
		SINGLE x1 = p0.u;
		SINGLE x2 = p1.u;
		SINGLE y1 = p0.v;
		SINGLE y2 = p1.v;
		
		S32 loops=0;
		
		//	U32 vx = 0;
		//	U32 vy = 0;
		//	U32 vw = 1;
		//	U32 vh = 1;
		
		OutCode code1 = ComputeCode( p0.u, p0.v);//, vx, vy, vx+vw, vy+vh );
		OutCode code2 = ComputeCode( p1.u, p1.v);//, vx, vy, vx+vw, vy+vh );
		
		do
		{
			if (!(code1 | code2))
			{
				accept = true;
				done = true;
			}
			else if (code1 & code2)
			{
				done = true;
			}
			else
			{
				float x=0, y=0;
				OutCode out = code1 ? code1 : code2;
				
				CQASSERT(out);
				
				if (out & TOP)
				{
					x = x1 + (x2 - x1) * (-y1) / (y2 - y1);
					y = 0;
				}
				else if (out & BOTTOM)
				{
					x = x1 + (x2 - x1) * (YBOUND - y1) / (y2 - y1);
					y = YBOUND;
				}
				else if (out & RIGHT)
				{
					x = XBOUND;
					y = y1 + (y2 - y1) * (XBOUND - x1) / (x2 - x1);
				}
				else if (out & LEFT)
				{
					x = 0;
					y = y1 + (y2 - y1) * (-x1) / (x2 - x1);
				}
				
				if (out == code1)
				{
					x1 = x;
					y1 = y;
					code1 = ComputeCode(x1, y1);//, vx, vy, vx+vw, vy+vh );
				}
				else
				{
					x2 = x;
					y2 = y;
					code2 = ComputeCode(x2, y2);//, vx, vy, vx+vw, vy+vh );
				}
			}
			
			CQASSERT(loops++ < 1000);
			
		} while (!done);
		
		return accept;
	}
	
	// vx,vy is the origin of the rect, vw,vh is the width and height
	
	static bool intersects( TexCoord tri[3])//, U32 vx, U32 vy, U32 vw, U32 vh  )
	{
		bool pt0 = pt_is_in_rect( tri[0]);//, vx, vy, vw, vh );
		bool pt1 = pt_is_in_rect( tri[1]);//, vx, vy, vw, vh );
		bool pt2 = pt_is_in_rect( tri[2]);//, vx, vy, vw, vh );
		if( !pt0 && !pt1 && !pt2 ) {
			bool e0 = clipped( tri[0], tri[1]);//, vx, vy, vw, vh );
			bool e1 = clipped( tri[1], tri[1]);//, vx, vy, vw, vh );
			bool e2 = clipped( tri[2], tri[1]);//, vx, vy, vw, vh );
			return (e0 || e1 || e2);
		}
		return true;
	}
};


//---------------------------------------------------------------------------
//---------------------------End TObjDamage.h---------------------------------
//---------------------------------------------------------------------------
#endif