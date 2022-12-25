//$Header: /Conquest/App/Src/anim2d.cpp 35    7/28/00 11:42p Rmarr $
//Copyright (c) 1997 Digital Anvil, Inc.

#include "pch.h"
#include <globals.h>

#include "anim2d.h"
#include "camera.h"
#include "Startup.h"
#include "CQTrace.h"
#include "MyVertex.h"
#include "CQBATCH.h"

#include <ITextureLibrary.h>
#include <ICamera.h>
#include <FileSys.h>
#include <Engine.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>

AnimArchetype::AnimArchetype (void) : frames (NULL)
{
}
#define ANIM_INDEX INSTANCE_INDEX

#pragma pack (push, 1)
struct UVRect
{
	int txm_id;
	SINGLE x0;
	SINGLE y0;
	SINGLE x1;
	SINGLE y1;
};
#pragma pack (pop)

void AnimArchetype::load (IFileSystem* fs)
{
//	int any_txms_found = txm_lib->load_library (NULL, fs);

	DAFILEDESC desc ("Frequency");

	HANDLE hndl;
	DWORD read;

	hndl = fs->OpenChild (&desc);
	CQASSERT (hndl != INVALID_HANDLE_VALUE);

	fs->ReadFile (hndl, &capture_rate, sizeof (SINGLE), &read);
	fs->CloseHandle (hndl);

	fs->SetCurrentDirectory ("UV Set");

/*	desc.lpFileName = "Library name";

	hndl = fs->OpenChild (&desc);
	CQASSERT (hndl != INVALID_HANDLE_VALUE);

	int len_names = fs->GetFileSize (hndl);

	char* libName = (char*)malloc (len_names);
	fs->ReadFile (hndl, libName, len_names, &read);
	fs->CloseHandle (hndl);*/

	desc.lpFileName = "Texture names";

	hndl = fs->OpenChild (&desc);
	CQASSERT (hndl != INVALID_HANDLE_VALUE);

	S32 len_names = fs->GetFileSize (hndl);

	char* buf = (char*)malloc (len_names);
	fs->ReadFile (hndl, buf, fs->GetFileSize (hndl), &read);

	#define MAX_TXMS 64

	char* name_ptrs[MAX_TXMS];

	{
		int cur = 1;
		name_ptrs[0] = buf;
		for (int i = 0; i < len_names; i++)
		{
			if (!buf[i])
				name_ptrs[cur++] = buf + i + 1;
		}
		texCnt = cur-1;
		CQASSERT(texCnt);
	}

	fs->CloseHandle (hndl);

	desc.lpFileName = "UV Rects";

	hndl = fs->OpenChild (&desc);
	CQASSERT (hndl != INVALID_HANDLE_VALUE);

	DWORD fsize = fs->GetFileSize (hndl);
	CQASSERT (!(fsize % sizeof (UVRect)));
	frame_cnt = fsize / sizeof (UVRect);

	CQASSERT(frame_cnt && "Anim has no frames");

	UVRect* uvrects = new UVRect[frame_cnt];

	frames = new AnimFrame[frame_cnt];

	fs->ReadFile (hndl, uvrects, fsize, &read);
	fs->CloseHandle (hndl);

/*	desc.lpFileName = "Frame Counts";

	U32 i;

	frame_cnt_per[0] = frame_cnt;
	for (i=1;i<MAX_ANIM;i++)
	{
		frame_cnt_per[i] = 0;
	}

	hndl = fs->OpenChild (&desc);
	if (hndl != INVALID_HANDLE_VALUE)
	{

		fsize = fs->GetFileSize (hndl);
		fs->ReadFile (hndl, frame_cnt_per, fsize, &read);
		fs->CloseHandle (hndl);
	}*/

/*	start_frame[0] = 0;

	for (i=1;i<MAX_ANIM;i++)
	{
		start_frame[i] = start_frame[i-1] + frame_cnt_per[i-1];
	}*/
	
	CQASSERT(texCnt < 256 && "REALLY big tex anm.  expand arrays?");
	tex = new U32[texCnt];
	for (unsigned int i = 0;i<texCnt;i++)
	{
		if (TEXLIB->has_texture_id(name_ptrs[i]) != GR_OK)		// name is not present yet
			TEXLIB->load_library(fs, 0);
		tex[i]=0;
		TEXLIB->get_texture_id(name_ptrs[i], tex+i);
		//	TEXLIB->add_ref_texture_id(tex[i], NULL);

	}

	for (unsigned int j = 0; j < frame_cnt; j++)
	{
		ITL_TEXTUREFRAME_IRP tframe;
		TEXLIB->get_texture_frame(tex[uvrects[j].txm_id], ITL_FRAME_FIRST, &tframe );

		frames[j].texture = tframe.rp_texture_id;
//		frames[j].texture = tex[uvrects[j].txm_id];

		static int AssertionWaiting = 1;	// to facilitate clicking-through this assertion just once
		if (AssertionWaiting &&  frames[j].texture == 0)
		{
			CQASSERT(frames[j].texture != 0);
			AssertionWaiting = 0;
		}
		/*if (frames[j].texture == ITL_INVALID_ID)
		{
			txm_lib->load_library(fs);//texFile);
			frames[j].texture = txm_lib->get_texture_id (name_ptrs[uvrects[j].txm_id]);
		}*/
		
		
		CQASSERT (uvrects[j].txm_id < MAX_TXMS);

		//frames[j].texture = txm_lib->get_texture_id (name_ptrs[uvrects[j].txm_id]);
		frames[j].x0 = uvrects[j].x0;
		frames[j].y0 = uvrects[j].y0;
		frames[j].x1 = uvrects[j].x1;
		frames[j].y1 = uvrects[j].y1;
	}

	delete [] uvrects;
	free (buf);

	fs->SetCurrentDirectory ("..");

	//free (libName);
}

void AnimArchetype::release ()
{
	for (unsigned int i = 0; i < texCnt; i++)
	{
		TEXLIB->release_texture_id(tex[i]);
		tex[i]=0;
	}
}

AnimArchetype::~AnimArchetype (void)
{
	release();
	delete [] tex;
	delete [] frames;
}


void AnimInstance::Init (const AnimArchetype* _archetype)
										/*archetype (_archetype), 
										t (0.0f), 
										x_meters (-1.0f),
										y_meters (-1.0f),
										delay (0.0f)*/
{
	archetype = _archetype;
	x_meters = archetype->frames[0].x1 - archetype->frames[0].x0;
	y_meters = archetype->frames[0].y1 - archetype->frames[0].y0;
	rate = archetype->capture_rate;
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 255;
	alwaysFront = TRUE;
	delay = archetype->delay;
	totalAnimTime = archetype->frame_cnt/rate+delay;
//	animID = 0;
	loop = 1;
	sinA = 0;
	pivx = x_meters*0.5f;
	pivy = y_meters*0.5f;
//	width = x_meters;
	cosA = 1;
//	transform.set_identity();
	x_normal.set(1,0,0);
	y_normal.set(0,1,0);
}


const AnimFrame* AnimInstance::retrieve_current_frame ()
{
	const AnimFrame* result = NULL;

	if (delay <= 0.0f && t >= 0.0f)
	{
	//	unsigned int frame = /*inst->archetype->start_frame[inst->animID] + */floor (t * rate);
	
		U32 frame = (U32)(t*rate);
		if (frame < archetype->frame_cnt)
		{
			result = archetype->frames + frame;//+ inst->archetype->start_frame[inst->animID];
		}
		else
		{
			if (loop)
			{
				frame = frame % archetype->frame_cnt;
				result = archetype->frames + frame;// + inst->archetype->start_frame[inst->animID];
			}
		}
		
	}

	return result;
}

BOOL32 AnimInstance::update (SINGLE dt)
{
	if (delay > 0.0f)
	{
		CQASSERT(dt > 0.0f);

		delay -= dt;

		if (delay < 0.0f)
			t = -delay;
	}
	else
	{
		CQASSERT(dt > -99.0 && dt < 99.0);
		t += dt;
		if (loop)
		{
			while (t < 0)
			{
				t += totalAnimTime;
			}
			if (t >= totalAnimTime)
			{
				t = fmod(t,totalAnimTime);
				t -= archetype->delay;
				if (t > 0)
				{
					delay = 0;
				}
				else
				{
					delay = -t;
					t=0;
				}
			}
		}
	}

	return (retrieve_current_frame() != 0);
}

void AnimInstance::retrieve_frame_ext (const AnimFrame **frame1,const AnimFrame **frame2,SINGLE *share)
{
	if (delay <= 0.0f && t >= 0.0f)
	{
		U32 frame = (U32)(t*rate);
		*share = 1-(t*rate-frame);
		*frame2 = NULL;
		if (frame < archetype->frame_cnt)
		{
			if (frame >= 0)
			{
				*frame1 = archetype->frames + frame;
				if (frame+1 < archetype->frame_cnt)
					*frame2 = *frame1+1;
				else if (loop)
					*frame2 = archetype->frames;
			}
			else
			{
				if (loop)
				{
					/*while (frame < 0)
					{
						frame += archetype->frame_cnt;
						t += (archetype->frame_cnt)/rate;
					}*/
					*frame1 = archetype->frames + frame;
					*frame2 = archetype->frames + (frame+1)%archetype->frame_cnt;
				}
				else
				{
					*frame1 = NULL;
					if (frame == -1)
					{
						*frame2 = archetype->frames;
					}
				}
			}
		}
		else
		{
			if (loop)
			{
			/*	frame = frame % archetype->frame_cnt;
				t -= (archetype->frame_cnt)/rate;*/
				*frame1 = archetype->frames + frame;
				*frame2 = archetype->frames + (frame+1)%archetype->frame_cnt;
			}
			else
				*frame1 = NULL;
		}
		
	}
}

#define TOLERANCE 0.00001f



void AnimInstance::Randomize()
{
	rate = archetype->capture_rate - (SINGLE)(rand()*4)/RAND_MAX;

	delay = archetype->delay;
	totalAnimTime = archetype->frame_cnt/rate+delay;

	t = rand()%100 * (totalAnimTime)*0.01;

	SINGLE angle = 2*PI*rand()/RAND_MAX;

	cosA = cos(angle);
	sinA = sin(angle);
}

/*void Anim2D::SetRate(ANIM_INDEX index,SINGLE rate)
{
	AnimInstance* inst = instances[index];

	inst->rate = rate;
}*/

void AnimInstance::SetColor(U8 r,U8 g,U8 b,U8 a)
{
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
}

void AnimInstance::SetRotation(SINGLE angle,SINGLE _pivx,SINGLE _pivy)
{
	//SINGLE cosA,sinA;

	cosA = (SINGLE)cos(angle);
	sinA = (SINGLE)sin(angle);

	if (_pivx == -1.0 || _pivy == -1.0)
	{
		pivx = x_meters*0.5f;
		pivy = y_meters*0.5f;
	}
	else
	{
		pivx = _pivx;
		pivy = _pivy;
	}

//	Vector epos (inst->pos);

//#define RAD2 1.41421356

//	SINGLE x2 = (inst->x_meters / 2.0f);//*RAD2;
//	SINGLE y2 = (inst->y_meters / 2.0f);//*RAD2;
/*	SINGLE x = inst->x_meters;
	SINGLE y = inst->y_meters;

	inst->v0.set(epos.x+((0-pivx)*cosA-(0-pivy)*sinA)+pivx,epos.y+((0-pivx)*sinA+(0-pivy)*cosA)+pivy,epos.z);
	inst->v1.set(epos.x+((x-pivx)*cosA-(0-pivy)*sinA)+pivx,epos.y+((x-pivx)*sinA+(0-pivy)*cosA)+pivy,epos.z);
	inst->v2.set(epos.x+((x-pivx)*cosA-(y-pivy)*sinA)+pivx,epos.y+((x-pivx)*sinA+(y-pivy)*cosA)+pivy,epos.z);
	inst->v3.set(epos.x+((0-pivx)*cosA-(y-pivy)*sinA)+pivx,epos.y+((0-pivx)*sinA+(y-pivy)*cosA)+pivy,epos.z);*/
}

void AnimInstance::ForceFront(bool ff)
{
	alwaysFront = ff;

/*	if (!ff)
	{
		Vector epos (transform.translation);

		SINGLE x = x_meters;
		SINGLE y = y_meters;

		v0.set(epos.x-x/2,epos.y-y/2,epos.z);
		v1.set(epos.x+x/2,epos.y-y/2,epos.z);
		v2.set(epos.x+x/2,epos.y+y/2,epos.z);
		v3.set(epos.x-x/2,epos.y+y/2,epos.z);
	}*/
}

void AnimInstance::SetWidth(SINGLE _width)
{
	pivx *= (SINGLE)_width/x_meters;
	pivy *= (SINGLE)_width/x_meters;  //???????
//	width = _width;
		
	x_meters = _width;
	y_meters = (archetype->frames[0].y1 - archetype->frames[0].y0) * x_meters / (archetype->frames[0].x1 - archetype->frames[0].x0);

}

void AnimInstance::Restart()
{
	t = 0;
	delay = archetype->delay;
	totalAnimTime = archetype->frame_cnt/rate+delay;
}

void AnimInstance::SetTime (SINGLE time)
{
	if (loop)
		t = fmod(time,totalAnimTime);

	if (t > archetype->delay)
		t = t-archetype->delay;
	else
	{
		delay = archetype->delay-t;
		t = 0;
	}
}

struct DACOM_NO_VTABLE Anim2D : public IAnim2D
{
	protected:

	//	PrimitiveBuilder PB;


	public:

	//
	// Incoming interface map
	//

	BEGIN_DACOM_MAP_INBOUND(Anim2D)
	//DACOM_INTERFACE_ENTRY (IDAComponent)
	//DACOM_INTERFACE_ENTRY (IAnim2D)
	END_DACOM_MAP()

	Anim2D (void);

	~Anim2D (void);

	virtual GENRESULT COMAPI Initialize (void);

	virtual AnimArchetype * create_archetype (char *fileName);

	virtual AnimArchetype * create_archetype (struct IFileSystem *parent);

	virtual void start_batch(AnimArchetype *arch);

	virtual void start_batch (U32 texID);

	virtual void end_batch();

	virtual void COMAPI render_instance (AnimInstance *inst,const Transform * const _trans);

	virtual void COMAPI render_instance (ICamera * camera,AnimInstance *inst,const Transform * const _trans);

	virtual void COMAPI render_smooth (AnimInstance *inst,const Transform * const _trans);

	virtual void render(AnimInstance *inst,const Transform * const _trans);

//	virtual void SetAnim(INSTANCE_INDEX index,U8 val);
	
};

Anim2D::Anim2D(void) //: txm_lib_owned (false), txm_lib (NULL)
{
}

/*GENRESULT Anim2D::init (SYSCONSUMERDESC* info)
{
	GENRESULT result = GR_INVALID_PARMS;

	if (system = info->system)
	{
		if (info->system != NULL)
		{
			if (info->system->QueryInterface ("IDisplay", GL) == GR_OK)
				result = GR_OK;
		}
	}

	return result;
}*/

Anim2D::~Anim2D()
{
	/*
	{
		MetaNode<Camera2D>* node = NULL;
		while (cameras.traverse (node))
		{
			delete node->object;
			node->object = NULL;
		}
	}
	*/

/*	for (unsigned int i = 0; i < instances.num_entries (); i++)
	{
		delete instances[i];
		instances[i] = NULL;
	}

	for (unsigned int j = 0; j < archetypes.num_entries (); j++)
	{
		delete archetypes[j];
		archetypes[j] = NULL;
	}*/

/*	if (txm_lib_owned && txm_lib)
	{
		txm_lib->Release ();
		txm_lib = NULL;
	}*/
}

GENRESULT Anim2D::Initialize (void)
{
//	IEngineComponent* me = (IEngineComponent*)this;

//	if (me->QueryInterface("IEngine", (void **)&ENGINE) != GR_OK)
//		return GR_GENERIC;
//	ENGINE->Release();

//	if (ENGINE->QueryInterface ("ITXMLib", (void**)&txm_lib) != GR_OK)
//	{
		//emaurer:  this texture database instance should ideally is shared by everyone.

/*		ENGCOMPDESC desc ("ITXMLib");
		desc.system = system;

		COMPTR<IDAComponent> component;

		if (GR_OK != DACOM_Acquire ()->CreateInstance (&desc, component))
			return GR_GENERIC;
		else
		{
			if (GR_OK != component->QueryInterface ("ITXMLib", (void**)&txm_lib))
				return GR_GENERIC;
			else
				txm_lib_owned = true;
		}*/
//	}
//	else
//		txm_lib->Release ();

//	PB.SetPipeline(PIPE);

	return GR_OK;
}

AnimArchetype *Anim2D::create_archetype (char *fileName)
{
	AnimArchetype *result = 0;

	DAFILEDESC fdesc=fileName;
	COMPTR<IFileSystem> objFile;
	if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
	{
		if (objFile->SetCurrentDirectory ("Animation"))
		{
			result = new AnimArchetype;
			result->load (objFile);
			
			objFile->SetCurrentDirectory ("..");
		}
	}
	else 
	{
		CQFILENOTFOUND(fdesc.lpFileName);
	}

	return result;
}

AnimArchetype *Anim2D::create_archetype (IFileSystem* parent)
{
	AnimArchetype *result = 0;

	if (parent->SetCurrentDirectory ("Animation"))
	{
		result = new AnimArchetype;
		result->load (parent);

		parent->SetCurrentDirectory ("..");
		//result = TRUE;
	}

	return result;
}

/*void Anim2D::destroy_archetype (ARCHETYPE_INDEX index)
{
	if (index != INVALID_ARCHETYPE_INDEX)
	{
		delete archetypes[index];
		archetypes[index] = NULL;
	}
}*/
                               
/*BOOL32 Anim2D::create_instance (INSTANCE_INDEX where, ARCHETYPE_INDEX index)
{
	if (index != INVALID_ARCHETYPE_INDEX  && archetypes[index])
	{
		instances[where] = new AnimInstance (archetypes[index]);
	}

	return (instances[where] != NULL);
}
    
BOOL32 Anim2D::create_instance (ANIM_INDEX where, const C8* name)
{
	BOOL32 result = FALSE;

static PANE bogus_pane;
bogus_pane.x0 = 0;
bogus_pane.y0 = 0;
bogus_pane.x1 = SCREEN_WIDTH-1;
bogus_pane.y1 = SCREEN_HEIGHT-1;

	if (strcmp(name,"Camera") == 0)
	{
		cameras.append (new Camera2D (where, GL, &bogus_pane));

		result = TRUE;
	}

	return result;
}*/

/*
Camera2D* Anim2D::find_camera (ANIM_INDEX idx) const
{
	Camera2D* result = NULL;

	MetaNode<Camera2D>* node = NULL;

	while (cameras.traverse (node))
	{
		if (node->object->id == idx)
		{
			result = node->object;
			break;
		}
	}

	return result;
}
*/
	
/*void Anim2D::destroy_instance (ANIM_INDEX index)
{
	if (index != INVALID_INSTANCE_INDEX)
	{
		if (instances[index])
		{
			delete instances[index];
			instances[index] = NULL;
		}
		else
		{
			MetaNode<Camera2D>* node = NULL;

			while (cameras.traverse (node))
			{
				if (node->object->id == index)
				{
					delete node->object;
					cameras.remove (node);
					break;
				}
			}
		}
	}
}*/

/*void Anim2D::set_instance_property (ANIM_INDEX index, const C8* name, DACOM_VARIANT value)
{
	Camera2D* cam = find_camera (index);

	if (cam)
	{
		cam->set_property (name, value);
	}
	else
	{
		AnimInstance* exp = instances[index];

		if (exp)
		{
			if (!strcmp (name, "Width"))
			{
				exp->x_meters = value;
				exp->y_meters = (exp->archetype->frames[0].y1 - exp->archetype->frames[0].y0) * exp->x_meters / (exp->archetype->frames[0].x1 - exp->archetype->frames[0].x0);
				exp->pivx = (SINGLE)value/2;
				exp->pivy = (SINGLE)value/2;
			}
			else if (!strcmp (name, "Delay"))
			{
				exp->delay = value;
			}
		}
	}
}
*/

//IMPORTANT COMMENT
// the way this stands, the start_batch - end_batch concept is only good for single texture anims
void Anim2D::start_batch(AnimArchetype *arch)
{
//	if (inst->alwaysFront)
		CAMERA->SetPerspective();

	CAMERA->SetModelView();
	
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
//	BATCH->set_texture_stage_texture( 0,arch->frames[0].texture);
	SetupDiffuseBlend(arch->frames[0].texture,TRUE);
	
	PB.Begin (PB_QUADS);
}

void Anim2D::start_batch(U32 texID)
{
	CAMERA->SetPerspective();

	CAMERA->SetModelView();
	
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	//BATCH->set_texture_stage_texture( 0,texID);
	SetupDiffuseBlend(texID,TRUE);
	
	PB.Begin (PB_QUADS);
}

void Anim2D::end_batch()
{
	PB.End ();
}

void Anim2D::render_instance (AnimInstance *inst,const Transform * const _trans)
{
	if (inst != NULL)
	{
		if (inst->x_meters > 0.0 && inst->y_meters > 0.0f)
		{

			const AnimFrame* anim = inst->retrieve_current_frame ();

			if (anim)
			{
			

			
				Vector v0,v1,v2,v3;
				Vector epos (inst->GetPosition());//ENGINE->get_position (instance));
				if (_trans)
					epos = _trans->rotate_translate(epos);
			//	SINGLE xbytwo = inst->x_meters / 2.0f;
			//	SINGLE ybytwo = inst->y_meters / 2.0f;

				SINGLE pivx = inst->pivx;
				SINGLE pivy = inst->pivy;
				SINGLE cosA = inst->cosA;
				SINGLE sinA = inst->sinA;
				SINGLE x = inst->x_meters;
				SINGLE y = inst->y_meters;

			//	Transform trans;
				Vector i,j;
				if (inst->alwaysFront)
				{

					
					Vector cpos (MAINCAM->get_position());
					
					
					

					Vector look (epos - cpos);
					
					i.set(look.y, -look.x, 0);
					
					if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
					{
						i.x = 1.0f;
					}
					
					i.normalize ();
					
					Vector k (-look);
					k.normalize ();
					j = (cross_product (k, i));

			/*		Transform trans;
					trans.set_i(i);
					trans.set_j(j);
					trans.set_k(k);
					trans.translation = epos;
					
					v0.set(((0-pivx)*cosA-(0-pivy)*sinA),((0-pivx)*sinA+(0-pivy)*cosA),0);
					v1.set(((x-pivx)*cosA-(0-pivy)*sinA),((x-pivx)*sinA+(0-pivy)*cosA),0);
					v2.set(((x-pivx)*cosA-(y-pivy)*sinA),((x-pivx)*sinA+(y-pivy)*cosA),0);
					v3.set(((0-pivx)*cosA-(y-pivy)*sinA),((0-pivx)*sinA+(y-pivy)*cosA),0);
					
					v0 = trans*v0;
					v1 = trans*v1;
					v2 = trans*v2;
					v3 = trans*v3;*/
					
				}
				else
				{
				/*	if (_trans)
						trans = *_trans*inst->transform;
					else
						trans = inst->transform;*/

					//trans.set_i(inst->x_normal);
					//trans.set_j(inst->y_normal);

					if (_trans)
					{
						MATH_ENGINE()->rotate(i, *_trans, inst->x_normal);
						MATH_ENGINE()->rotate(j, *_trans, inst->y_normal);
					}
					else
					{
						i = inst->x_normal;
						j = inst->y_normal;
					}

				}

				v0=(epos+i*((0-pivx)*cosA-(0-pivy)*sinA)-j*((0-pivx)*sinA+(0-pivy)*cosA));
				v1=(epos+i*((x-pivx)*cosA-(0-pivy)*sinA)-j*((x-pivx)*sinA+(0-pivy)*cosA));
				v2=(epos+i*((x-pivx)*cosA-(y-pivy)*sinA)-j*((x-pivx)*sinA+(y-pivy)*cosA));
				v3=(epos+i*((0-pivx)*cosA-(y-pivy)*sinA)-j*((0-pivx)*sinA+(y-pivy)*cosA));
		
				
				PB.Color4ub (inst->color.r, inst->color.g, inst->color.b, inst->color.a);
				
				PB.TexCoord2f (anim->x0, anim->y0);
				PB.Vertex3f (v0.x, v0.y, v0.z);
				PB.TexCoord2f (anim->x1, anim->y0);
				PB.Vertex3f (v1.x, v1.y, v1.z);
				PB.TexCoord2f (anim->x1, anim->y1);
				PB.Vertex3f (v2.x, v2.y, v2.z);
				PB.TexCoord2f (anim->x0, anim->y1);
				PB.Vertex3f (v3.x, v3.y, v3.z);
				
			}
		}
	}
}

void Anim2D::render_instance (ICamera * camera,AnimInstance *inst,const Transform * const _trans)
{
	if (inst != NULL)
	{
		if (inst->x_meters > 0.0 && inst->y_meters > 0.0f)
		{

			const AnimFrame* anim = inst->retrieve_current_frame ();

			if (anim)
			{
			

			
				Vector v0,v1,v2,v3;
				Vector epos (inst->GetPosition());//ENGINE->get_position (instance));
				if (_trans)
					epos = _trans->rotate_translate(epos);
			//	SINGLE xbytwo = inst->x_meters / 2.0f;
			//	SINGLE ybytwo = inst->y_meters / 2.0f;

				SINGLE pivx = inst->pivx;
				SINGLE pivy = inst->pivy;
				SINGLE cosA = inst->cosA;
				SINGLE sinA = inst->sinA;
				SINGLE x = inst->x_meters;
				SINGLE y = inst->y_meters;

			//	Transform trans;
				Vector i,j;
				if (inst->alwaysFront)
				{

					
					Vector cpos (MAINCAM->get_position());
					
					
					

					Vector look (epos - cpos);
					
					i.set(look.y, -look.x, 0);
					
					if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
					{
						i.x = 1.0f;
					}
					
					i.normalize ();
					
					Vector k (-look);
					k.normalize ();
					j = (cross_product (k, i));

/*					trans.set_i(i);
					trans.set_j(j);
					trans.set_k(k);
					trans.translation = epos;

					v0.set(((0-pivx)*cosA-(0-pivy)*sinA),((0-pivx)*sinA+(0-pivy)*cosA),0);
					v1.set(((x-pivx)*cosA-(0-pivy)*sinA),((x-pivx)*sinA+(0-pivy)*cosA),0);
					v2.set(((x-pivx)*cosA-(y-pivy)*sinA),((x-pivx)*sinA+(y-pivy)*cosA),0);
					v3.set(((0-pivx)*cosA-(y-pivy)*sinA),((0-pivx)*sinA+(y-pivy)*cosA),0);

					v0 = trans*v0;
					v1 = trans*v1;
					v2 = trans*v2;
					v3 = trans*v3;*/

				}
				else
				{
					if (_trans)
					{
						MATH_ENGINE()->rotate(i, *_trans, inst->x_normal);
						MATH_ENGINE()->rotate(j, *_trans, inst->y_normal);
					}
					else
					{
						i = inst->x_normal;
						j = inst->y_normal;
					}
					
				}
				
				v0=(epos+i*((0-pivx)*cosA-(0-pivy)*sinA)-j*((0-pivx)*sinA+(0-pivy)*cosA));
				v1=(epos+i*((x-pivx)*cosA-(0-pivy)*sinA)-j*((x-pivx)*sinA+(0-pivy)*cosA));
				v2=(epos+i*((x-pivx)*cosA-(y-pivy)*sinA)-j*((x-pivx)*sinA+(y-pivy)*cosA));
				v3=(epos+i*((0-pivx)*cosA-(y-pivy)*sinA)-j*((0-pivx)*sinA+(y-pivy)*cosA));
				
				
				PB.Color4ub (inst->color.r, inst->color.g, inst->color.b, inst->color.a);
				
				PB.TexCoord2f (anim->x0, anim->y0);
				PB.Vertex3f (v0.x, v0.y, v0.z);
				PB.TexCoord2f (anim->x1, anim->y0);
				PB.Vertex3f (v1.x, v1.y, v1.z);
				PB.TexCoord2f (anim->x1, anim->y1);
				PB.Vertex3f (v2.x, v2.y, v2.z);
				PB.TexCoord2f (anim->x0, anim->y1);
				PB.Vertex3f (v3.x, v3.y, v3.z);
				
			}
		}
	}
}

void Anim2D::render_smooth (AnimInstance *inst,const Transform * const _trans)
{
	if (inst != NULL)
	{
		if (inst->x_meters > 0.0 && inst->y_meters > 0.0f)
		{

			const AnimFrame* anim=NULL,*anim2=NULL;
			SINGLE share;
			
			inst->retrieve_frame_ext (&anim,&anim2,&share);

			if (anim)
			{
				if (inst->alwaysFront)
					CAMERA->SetPerspective();

				CAMERA->SetModelView();
			
				BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			
				BATCH->set_state(RPR_BATCH,false);
				BATCH->set_texture_stage_texture( 0, anim->texture );
				BATCH->set_texture_stage_texture( 1, anim2->texture );
				
				BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
				
				BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);

				BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_BLENDDIFFUSEALPHA );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
				
				
				Vector v0,v1,v2,v3;
				Vector epos (inst->GetPosition());//ENGINE->get_position (instance));
				if (_trans)
					epos = _trans->rotate_translate(epos);
				//	SINGLE xbytwo = inst->x_meters / 2.0f;
				//	SINGLE ybytwo = inst->y_meters / 2.0f;
				
				SINGLE pivx = inst->pivx;
				SINGLE pivy = inst->pivy;
				SINGLE cosA = inst->cosA;
				SINGLE sinA = inst->sinA;
				SINGLE x = inst->x_meters;
				SINGLE y = inst->y_meters;
				
			//	Transform trans;
				Vector i,j;
				if (inst->alwaysFront)
				{
					
					
					Vector cpos (MAINCAM->get_position());
					
					
					
					
					Vector look (epos - cpos);
					
					i.set(look.y, -look.x, 0);
					
					if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
					{
						i.x = 1.0f;
					}
					
					i.normalize ();
					
					Vector k (-look);
					k.normalize ();
					j = (cross_product (k, i));

			//		Transform trans;
				/*	trans.set_i(i);
					trans.set_j(j);
					trans.set_k(k);
					trans.translation = epos;

					v0=(epos+i*((0-pivx)*cosA-(0-pivy)*sinA)-j*((0-pivx)*sinA+(0-pivy)*cosA));
					v1=(epos+i*((x-pivx)*cosA-(0-pivy)*sinA)-j*((x-pivx)*sinA+(0-pivy)*cosA));
					v2=(epos+i*((x-pivx)*cosA-(y-pivy)*sinA)-j*((x-pivx)*sinA+(y-pivy)*cosA));
					v3=(epos+i*((0-pivx)*cosA-(y-pivy)*sinA)-j*((0-pivx)*sinA+(y-pivy)*cosA));*/

				}
				else
				{
					if (_trans)
					{
						MATH_ENGINE()->rotate(i, *_trans, inst->x_normal);
						MATH_ENGINE()->rotate(j, *_trans, inst->y_normal);
					}
					else
					{
						i = inst->x_normal;
						j = inst->y_normal;
					}
					
				}
				
				v0=(epos+i*((0-pivx)*cosA-(0-pivy)*sinA)-j*((0-pivx)*sinA+(0-pivy)*cosA));
				v1=(epos+i*((x-pivx)*cosA-(0-pivy)*sinA)-j*((x-pivx)*sinA+(0-pivy)*cosA));
				v2=(epos+i*((x-pivx)*cosA-(y-pivy)*sinA)-j*((x-pivx)*sinA+(y-pivy)*cosA));
				v3=(epos+i*((0-pivx)*cosA-(y-pivy)*sinA)-j*((0-pivx)*sinA+(y-pivy)*cosA));
				
				BATCHDESC desc;
				desc.type = D3DPT_TRIANGLELIST;
				desc.vertex_format = D3DFVF_RPVERTEX2;
				desc.num_verts = 4;
				desc.num_indices = 6;
				CQBATCH->GetPrimBuffer(&desc);
				Vertex2 *dest_verts = (Vertex2 *)desc.verts;
				U16 *id_list = desc.indices;
				
				U32 color = RGB(inst->color.r, inst->color.g, inst->color.b) | ((255-U32(share*255))<<24);
				dest_verts[0].pos = v0;
				dest_verts[0].color = color;
				dest_verts[0].u = anim->x0;
				dest_verts[0].v = anim->y0;
				dest_verts[0].u2 = anim2->x0;
				dest_verts[0].v2 = anim2->y0;

				dest_verts[1].pos = v1;
				dest_verts[1].color = color;
				dest_verts[1].u = anim->x1;
				dest_verts[1].v = anim->y0;
				dest_verts[1].u2 = anim2->x1;
				dest_verts[1].v2 = anim2->y0;

				dest_verts[2].pos = v2;
				dest_verts[2].color = color;
				dest_verts[2].u = anim->x1;
				dest_verts[2].v = anim->y1;
				dest_verts[2].u2 = anim2->x1;
				dest_verts[2].v2 = anim2->y1;

				dest_verts[3].pos = v3;
				dest_verts[3].color = color;
				dest_verts[3].u = anim->x0;
				dest_verts[3].v = anim->y1;
				dest_verts[3].u2 = anim2->x0;
				dest_verts[3].v2 = anim2->y1;

				id_list[0] = 0;
				id_list[1] = 1;
				id_list[2] = 2;
				id_list[3] = 0;
				id_list[4] = 2;
				id_list[5] = 3;

				CQBATCH->ReleasePrimBuffer(&desc);
			}
		}
	}
}

void Anim2D::render(AnimInstance *inst, const Transform * const _trans)
{
	if (inst)
	{
	
		const AnimFrame* anim = inst->retrieve_current_frame ();
		if (anim)
		{
			if (inst->alwaysFront)
				CAMERA->SetPerspective();

			CAMERA->SetModelView();
			
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		//	BATCH->set_texture_stage_texture( 0,anim->texture);

			SetupDiffuseBlend(anim->texture,TRUE);
			BATCH->set_state(RPR_STATE_ID,anim->texture);
			
			PB.Begin (PB_QUADS);
			
			render_instance(inst,_trans);
			
			PB.End ();
			BATCH->set_state(RPR_STATE_ID,0);
		}
	}
}


struct _anim : GlobalComponent
{
	Anim2D *anim2d;

	virtual void Startup (void)
	{
		ANIM2D = anim2d = new DAComponent<Anim2D>;
		AddToGlobalCleanupList((IDAComponent **) &ANIM2D);
	}

	virtual void Initialize (void)
	{
		anim2d->Initialize();
	}
};

static _anim anim;

