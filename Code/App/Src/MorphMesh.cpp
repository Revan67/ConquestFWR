//--------------------------------------------------------------------------//
//                                                                          //
//                               MorphMesh.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MorphMesh.cpp 14    6/14/00 4:06p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <stdio.h>

#include "TSmartpointer.h"
#include "CQTrace.h"
#include "IMorphMesh.h"

#include <FileSys.h>
#include <Renderer.h>
#include <Mesh.h>
#include <HeapObj.h>
//
// Catmull-Rom basis.
//
float Bcr[4][4] =
{
	{-1.0/2.0,  3.0/2.0, -3.0/2.0,  1.0/2.0},
	{ 2.0/2.0, -5.0/2.0,  4.0/2.0, -1.0/2.0},
	{-1.0/2.0,  0.0/2.0,  1.0/2.0,  0.0/2.0},
	{ 0.0/2.0,  2.0/2.0,  0.0/2.0,  0.0/2.0}
};

struct ControlPoint
{
	SINGLE time;
	SINGLE percent;

	// *this += v.
	inline const ControlPoint& add(const ControlPoint & v)
	{
		time += v.time;
		percent += v.percent;
		return *this;
	}

	inline const ControlPoint& operator += (const ControlPoint & v)
	{
		return add(v);
	}
//
// friend functions follow.
//
	inline friend ControlPoint add(const ControlPoint & v1, const ControlPoint & v2)
	{
		ControlPoint result;
		result.time = v1.time + v2.time;
		result.percent = v1.percent + v2.percent;
		return result;
	}

	inline friend ControlPoint scale(const ControlPoint & v, SINGLE s)
	{
		ControlPoint result;
		result.time = v.time * s;
		result.percent = v.percent * s;
		return result;
	}
//
// Friend overloaded operators follow.
//
	inline friend ControlPoint operator + (const ControlPoint & v1, const ControlPoint & v2)
	{
		return ::add(v1, v2);
	}

	inline friend ControlPoint operator * (const ControlPoint & v, SINGLE s)
	{
		return ::scale(v, s);
	}

	inline friend ControlPoint operator * (SINGLE s, const ControlPoint & v)
	{
		return ::scale(v, s);
	}
};

void ComputeCoeff(ControlPoint & a, ControlPoint & b, ControlPoint & c, ControlPoint & d, ControlPoint p[4]);

struct MorphAnim
{
	ARCHETYPE_INDEX mesh_id[MAX_TARGETS];
	Parameters params[MAX_TARGETS];
	ControlPoint cp[MAX_CONTROL_POINTS];
	S32 numControlPoints;
	S32 numMeshes;
	SINGLE totalTime;
	
	MorphAnim()
	{
		numMeshes = 0;
		for (int i=0;i<MAX_TARGETS;i++)
		{
			mesh_id[i] = INVALID_ARCHETYPE_INDEX;
			params[i].scale = 1.0;
		}
	}

	~MorphAnim()
	{
		for (int i=0;i<numMeshes;i++)
		{
			ENGINE->release_archetype(mesh_id[i]);
			mesh_id[i] = INVALID_ARCHETYPE_INDEX;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct MorphMeshArchetype : IProfileCallback
{
	MorphAnim *anims;
	U32 iniStage;
	S32 numAnims;
	S32 currentAnim;
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	MorphMeshArchetype()
	{
	}
	
	~MorphMeshArchetype()
	{
		if (anims)
			delete [] anims;
	}
	
	BOOL32 __stdcall ProfileCallback (struct IProfileParser2 * parser, const C8 * sectionName, const C8 * value, void *context);

	BOOL32 ReadINI(char *filename);
};

BOOL32 MorphMeshArchetype::ProfileCallback(struct IProfileParser2 * parser, const C8 * sectionName, const C8* value, void *context)
{
	BOOL32 result = 0;

	if (iniStage == 0)
	{
		if (strcmp(sectionName,"Header"))
		{
			CQERROR0("[Header] must begin ini file");
			goto Done;
		}
	}

	if (iniStage == 1)
	{
		if (strcmp(sectionName,"MorphTargets"))
		{
			CQERROR0("Each animation must begin with [MorphTargets]");
			goto Done;
		}
	}

	if (iniStage == 2)
	{

		if (strcmp(sectionName,"ControlPoints"))
		{
			if (strcmp(sectionName,"Parameters"))
			{
				CQERROR0("Each [MorphTargets] must be followed by a [ControlPoints] or [Parameters]");
				goto Done;
			}
			else
			{
				//hacky way to get to parameters stage
				iniStage = 3;
			}
		}
	}
	

	HANDLE hSection;
	char buffer[256];


	if ((hSection = parser->CreateSection(sectionName)) != 0)
	{
		switch (iniStage)
		{
		case 0:
			{
				U32 len;
			
				if ((len = parser->ReadKeyValue(hSection, "NumAnims", buffer, sizeof(buffer))) != 0)
				{
					if (sscanf(buffer,"%i",&numAnims) == -1)
					{
						CQERROR0("Error reading NumAnims field");
						goto Done;
					}
					anims = new MorphAnim[numAnims];
				}
				else
				{
					CQERROR0("Need NumAnims field");
					goto Done;
				}
				iniStage++;
			}
			break;
		case 1:
			{
				DAFILEDESC fdesc;
				COMPTR<IFileSystem> file;
				// load mesh archetypes
				if (currentAnim == numAnims)
				{
					CQERROR0("Read more anims than specified");
					goto Done;
				}

				int line = 0;
				int cnt = 0;
				
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					line++;
					//if line is not a comment
					if (buffer[0] != ';' && buffer[0] != 0)
					{
						CQASSERT(cnt < MAX_TARGETS && "Too many morph targets");
						fdesc = buffer;
						if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
						{
							anims[currentAnim].mesh_id[cnt] = ENGINE->create_archetype(fdesc.lpFileName, file);
							if (anims[currentAnim].mesh_id[cnt] != -1)
								cnt++;
						}
						else
							CQFILENOTFOUND(fdesc.lpFileName);
					}
				}
				iniStage++;
				
				anims[currentAnim].numMeshes = cnt;
			}
			break;
		case 2:
			{
				// load control points
				int line = 0;
				int cnt = 1;
				
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					CQASSERT(cnt < MAX_CONTROL_POINTS-1 && "Too many control points");
					line++;
					
					if (buffer[0] != ';' && sscanf( buffer, "%f", &anims[currentAnim].cp[cnt].percent) != -1)
					{
						bool gotLine=false;
						while (gotLine == 0 && parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
						{
							line++;
							if (buffer[0] != ';' && sscanf( buffer, "%f", &anims[currentAnim].cp[cnt].time) != -1)
							{
								anims[currentAnim].cp[cnt].time /= 30.0;
								gotLine = true;
							}
						}
						
						if (gotLine == 0)
							CQERROR0("No time for last control point");
						
						CQASSERT(anims[currentAnim].cp[cnt].percent <= anims[currentAnim].numMeshes);
						
						cnt++;
					}
				}
				
				//double anchors for non-looping case
				anims[currentAnim].cp[0].time = anims[currentAnim].cp[1].time;
				anims[currentAnim].cp[0].percent = anims[currentAnim].cp[1].percent;
				anims[currentAnim].cp[cnt].time = anims[currentAnim].cp[cnt-1].time;
				anims[currentAnim].cp[cnt].percent = anims[currentAnim].cp[cnt-1].percent;
				
				CQASSERT(cnt >= 2 && "Not enough control points");
				anims[currentAnim].numControlPoints = cnt;
				anims[currentAnim].totalTime = anims[currentAnim].cp[cnt-1].time;
				iniStage--;
				currentAnim++;
			}
			break;
		case 3:
				int line = 0;
				int cnt = 0;
				
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					line++;
					//if line is not a comment
					if (buffer[0] != ';' && strstr(buffer, "{scale}"))
					{

						while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0 && buffer[0] != '{')
						{
							line++;
							if (buffer[0] != ';' && sscanf( buffer, "%f", &anims[currentAnim].params[cnt].scale) != -1)
							{
								cnt++;
							}
						}
					}
				}
				iniStage = 2;

			break;
		}
	}
	else goto Done;

	result = 1;

Done:

	return result;
}

BOOL32 MorphMeshArchetype::ReadINI(char *filename)
{
//	bLooping = _bLooping;

	//unload previous info - anims should be 0 if struct is new
	if (anims)
	{
		delete [] anims;
		anims = 0;
	}

	iniStage = 0;

	COMPTR<IProfileParser2> parser;

	if (CreateProfileParser(filename, parser) == GR_OK)
	{
		if (parser->EnumerateSections(this) == 0)
		{
			CQERROR0("Failed to parse INI file");
			if (anims)
			{
				delete [] anims;
				anims = 0;
			}
			return 0;
		}
		currentAnim = 0;
		return 1;

/*			HANDLE hSection;
		
		int cnt=0;
		// load mesh archetypes
		if ((hSection = parser->CreateSection("MorphTargets")) != 0)
		{
			char buffer[256];
			cnt = 0;
			
			while (parser->ReadProfileLine(hSection, cnt, buffer, sizeof(buffer)) != 0)
			{
				CQASSERT(cnt < MAX_TARGETS && "Too many morph targets");
				fdesc = buffer;
				if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
					mesh_id[cnt] = ENGINE->create_archetype(fdesc.lpFileName, file);
				else
					CQFILENOTFOUND(fdesc.lpFileName);

				cnt++;
			}
		}
		else
			CQASSERT("Can't find [MorphTargets]");

		numMeshes = cnt;


		// load control points
		if ((hSection = parser->CreateSection("ControlPoints")) != 0)
		{
			char buffer[256];
			int line = 0;
			cnt = 1;
			
			while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
			{
				CQASSERT(cnt < MAX_CONTROL_POINTS-1 && "Too many control points");
				line++;
				//	sscanf( buffer, "%f %f", &cp[line].time, &cp[line].percent);
				if (sscanf( buffer, "%f", &cp[cnt].percent) != -1)
				{
					if (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
					{
						if (sscanf( buffer, "%f", &cp[cnt].time) != -1)
						{
						  cp[cnt].time /= 30.0;
						  line++;
						}
					}
					else
						CQERROR0("No time for last control point");
					
					CQASSERT(cp[cnt].percent <= numMeshes);
					
					cnt++;
				}
			}
		}
		else
			CQASSERT("Can't find [ControlPoints]");

		if (bLooping)
		{
			cp[0].time = cp[cnt-2].time-cp[cnt-1].time;
			cp[0].percent = cp[cnt-2].percent;
			cp[cnt].time = cp[cnt-1].time+cp[2].time;
			cp[cnt].percent = cp[cnt-1].percent+cp[2].percent;
		//	cp[cnt-1].percent = cp[1].percent;
		}
		else
		{
			//double anchors for non-looping case
			cp[0].time = cp[1].time;
			cp[0].percent = cp[1].percent;
			cp[cnt].time = cp[cnt-1].time;
			cp[cnt].percent = cp[cnt-1].percent;
		}

		CQASSERT(cnt >= 2 && "Not enough control points");
		numControlPoints = cnt;
		totalTime = cp[cnt-1].time;*/
	}
	else
	{
		CQERROR1("Can't open ini file - %s",filename);
		return 0;
	}
}


struct AnimQueue
{
	SINGLE triggerTime,transTime;
	S32 nextAnim;
	bool bLooping;
	AnimQueue *next;

	AnimQueue::AnimQueue()
	{
		next = NULL;
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct MorphMesh : IMorphMesh
{
	BEGIN_DACOM_MAP_INBOUND(MorphMesh)
	DACOM_INTERFACE_ENTRY (IDAComponent)
	DACOM_INTERFACE_ENTRY (IMorphMesh)
	END_DACOM_MAP()
	
	S32 currentAnim;
	S32 last_cp;
	SINGLE animTimer,endTime;
	SINGLE transTime;
	bool bLooping,nextLooping;
	S32 numAnims;
	AnimQueue *animQueue;

	MorphAnim *anims;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	MorphMesh::MorphMesh();
	~MorphMesh();

	virtual SINGLE GetMorphPos(Mesh **mesh0,Mesh **mesh1);
	virtual S32 GetVertices(Vector *vertices,Parameters *params);
//	virtual void GetParams(struct Parameters * params);
	virtual SINGLE GetMorphTime();
	virtual Mesh *GetMesh0();
	virtual void KillAnimQueue();
	virtual void MorphTo(S32 animID,SINGLE transTime,bool _bLooping);
	virtual void QueueAnim(S32 animID,SINGLE _timeFromEnd,bool _bLooping);
	virtual void RunAnim(S32 animID,bool _bLooping);
	virtual void Update(SINGLE dt);
//	virtual void *GetBase();
};

MorphMesh::MorphMesh()
{

	last_cp = 0;
	currentAnim = 0;
	//nextAnim = -1;

}

MorphMesh::~MorphMesh()
{
	KillAnimQueue();
}



SINGLE MorphMesh::GetMorphPos(Mesh **mesh0,Mesh **mesh1)
{
#define num_steps 6

	SINGLE result=-1;

	if (animTimer >= anims[currentAnim].totalTime)
	{
		*mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[anims[currentAnim].numMeshes-1]);
		*mesh1 = 0;

		return 0;
	}

	ControlPoint *cp = anims[currentAnim].cp;

	while (last_cp < anims[currentAnim].numControlPoints-2 && animTimer > cp[last_cp+1].time)
	{
		last_cp++;
	}

	while (last_cp > 1 && animTimer < cp[last_cp].time)
	{
		last_cp--;
	}
	
	
	ControlPoint a,b,c,d;
	ComputeCoeff(a,b,c,d,&cp[last_cp-1]);
	float delta = 1.0 / num_steps;
	float delta2 = delta * delta;
	float delta3 = delta2 * delta;
			
	ControlPoint df = a * delta3 + b * delta2 + c * delta;
	ControlPoint ddf;
	ControlPoint dddf;
	ddf = 6.0 * a * delta3 + 2.0 * b * delta2;
	dddf = 6.0 * a * delta3;
	
	if (df.time < 0 || df.time + ddf.time*num_steps < 0 || df.time + (ddf.time+dddf.time*num_steps*0.5)*num_steps < 0)
	{
		df.time = (cp[last_cp+1].time-cp[last_cp].time)/num_steps;
		df.percent = (cp[last_cp+1].percent-cp[last_cp].percent)/num_steps;
		ddf.time = dddf.time = ddf.percent = dddf.percent = 0;
	}

	ControlPoint p = d;
	ControlPoint last_p = d;

	while (p.time < animTimer)
	{
		last_p = p;
		p += df;
		df += ddf;
		ddf += dddf;
	}

	CQASSERT(p.time-last_p.time > 0);

	SINGLE share = (animTimer-last_p.time)/(p.time-last_p.time);

	if (share < 0)
		share = 0;
	//CQASSERT(share >= 0);

	result = last_p.percent*(1-share)+p.percent*share;

	CQASSERT(result >= 0);

	S32 m_id = floor(result);
	if (bLooping && m_id == anims[currentAnim].numMeshes-1)//bLooping)
	{
		*mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id]);
		*mesh1 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[0]);
	}
	else
	{
		*mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id]);
		*mesh1 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id+1]);
	}

	if (*mesh1 == 0)
	{
		result = 0;
	}
	else
	{
		CQASSERT((*mesh0)->object_vertex_cnt == (*mesh1)->object_vertex_cnt);

		result = result-floor(result);
	}

	return result;
#undef num_steps
}
///////////////////////////////////////////////////////
// returns # of vertices
S32 MorphMesh::GetVertices(Vector *vertices,Parameters *params)
{
#define num_steps 6

	Mesh *mesh0=0,*mesh1=0;
	Parameters *params0,*params1;
	params0=params1=0;
	ControlPoint p;
	ControlPoint last_p;

	SINGLE result=-1;

	if (animTimer >= anims[currentAnim].totalTime)
	{
		mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[anims[currentAnim].numMeshes-1]);
		mesh1 = 0;
		params0 = &anims[currentAnim].params[anims[currentAnim].numMeshes-1];

		result = 0;
		goto Finish;
	}

	ControlPoint *cp;
	cp = anims[currentAnim].cp;
	while (last_cp < anims[currentAnim].numControlPoints-2 && animTimer >= cp[last_cp+1].time)
	{
		last_cp++;
	}

	while (last_cp > 1 && animTimer < cp[last_cp].time)
	{
		last_cp--;
	}
	
	
	ControlPoint a,b,c,d;
	ComputeCoeff(a,b,c,d,&cp[last_cp-1]);
	float delta;
	float delta2;
	float delta3;

	delta = 1.0 / num_steps;
	delta2 = delta * delta;
	delta3 = delta2 * delta;
			
	ControlPoint df;
	ControlPoint ddf;
	ControlPoint dddf;
	df = a * delta3 + b * delta2 + c * delta;
	ddf = 6.0 * a * delta3 + 2.0 * b * delta2;
	dddf = 6.0 * a * delta3;
	
	if (df.time < 0 || df.time + ddf.time*num_steps < 0 || df.time + (ddf.time+dddf.time*num_steps*0.5)*num_steps < 0)
	{
		df.time = (cp[last_cp+1].time-cp[last_cp].time)/num_steps;
		df.percent = (cp[last_cp+1].percent-cp[last_cp].percent)/num_steps;
		ddf.time = dddf.time = ddf.percent = dddf.percent = 0;
	}

	p = d;
	last_p = d;

	while (p.time < animTimer)
	{
		last_p = p;
		p += df;
		df += ddf;
		ddf += dddf;
	}

	SINGLE share;


	if (p.time-last_p.time > 0)
		share = (animTimer-last_p.time)/(p.time-last_p.time);
	else
		share = 0;

	if (share < 0)
		share = 0;
	//CQASSERT(share >= 0);

	result = last_p.percent*(1-share)+p.percent*share;

	if (result < 0)
		result = 0;

	S32 m_id;
	m_id = floor(result);
	if (bLooping && m_id == anims[currentAnim].numMeshes-1)//bLooping)
	{
		mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id]);
		mesh1 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[0]);

		params0 = &anims[currentAnim].params[m_id];
		params1 = &anims[currentAnim].params[0];
	}
	else
	{
		mesh0 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id]);
		params0 = &anims[currentAnim].params[m_id];

		if (anims[currentAnim].mesh_id[m_id+1] != 0xffffffff)
		{
			mesh1 = REND->get_archetype_mesh(anims[currentAnim].mesh_id[m_id+1]);
			params1 = &anims[currentAnim].params[m_id+1];
		}
		else
			mesh1=0;
	}

	if (mesh1 == 0)
	{
		result = 0;
	}
	else
	{
		CQASSERT(mesh0->object_vertex_cnt == mesh1->object_vertex_cnt);

		result = result-floor(result);
	}

Finish:
	params->scale = params0->scale*(1-result);
	int i;
	for (i=0;i<mesh0->object_vertex_cnt;i++)
	{
		vertices[i] = (mesh0->object_vertex_list[i])*(1-result);

		if (mesh1)
		{
			vertices[i] += (mesh1->object_vertex_list[i])*(result);
		}
	}

	if (mesh1)
	{
		CQASSERT(params1);
		params->scale += params1->scale*result;
	}
	
	if (animQueue && animTimer > animQueue->triggerTime)
	{
		Mesh *mesh2 = REND->get_archetype_mesh(anims[animQueue->nextAnim].mesh_id[0]);
		Parameters *params2 = &anims[animQueue->nextAnim].params[0];
		SINGLE blend = (animTimer-animQueue->triggerTime)/animQueue->transTime;

		for (i=0;i<mesh0->object_vertex_cnt;i++)
		{
			vertices[i] = vertices[i]*(1-blend)+mesh2->object_vertex_list[i]*blend;
		}

		CQASSERT(params2);
		params->scale = params->scale*(1-blend)+params2->scale*blend;
	}

#undef num_steps

	return mesh0->object_vertex_cnt;
}

SINGLE MorphMesh::GetMorphTime()
{
	return anims[currentAnim].totalTime;
}

Mesh *MorphMesh::GetMesh0()
{
	return REND->get_archetype_mesh(anims[0].mesh_id[0]);
}

void MorphMesh::KillAnimQueue()
{
	AnimQueue *pos = animQueue;

	while (pos)
	{
		animQueue = pos->next;
		delete pos;
		pos = animQueue;
	}

	animQueue = 0;
}

void MorphMesh::MorphTo(S32 animID, SINGLE _transTime,bool _bLooping)
{
	CQASSERT(animID < numAnims && "Called an anim that doesn't exist");
	KillAnimQueue();

	animQueue = new AnimQueue;
	animQueue->bLooping = _bLooping;
	animQueue->nextAnim = animID;
	animQueue->triggerTime = animTimer;
	animQueue->transTime = _transTime;
}

void MorphMesh::QueueAnim(S32 animID, SINGLE _timeFromEnd,bool _bLooping)
{
	CQASSERT(animID < numAnims && "Called an anim that doesn't exist");

	AnimQueue *newNode = new AnimQueue;
	AnimQueue *pos = animQueue;
	U32 thisAnim = currentAnim;

	if (animQueue == NULL)
		animQueue = newNode;
	else
	{
		while (pos->next)
		{
			pos = pos->next;
		}
		pos->next = newNode;
	}

	if (pos)
		thisAnim = pos->nextAnim;

	newNode->nextAnim = animID;
	newNode->triggerTime = anims[thisAnim].totalTime - _timeFromEnd;
	newNode->transTime = _timeFromEnd;
	newNode->bLooping = _bLooping;
}

void MorphMesh::RunAnim(S32 animID,bool _bLooping)
{
	CQASSERT(animID < numAnims);

	if (animID == currentAnim && animTimer == 0)
		return;

	currentAnim = animID;
	animTimer = 0;
	bLooping = _bLooping;
	last_cp = 0;
	
	ControlPoint *cp = anims[currentAnim].cp;
	int cnt = anims[currentAnim].numControlPoints;
	if (bLooping)
	{
		cp[0].time = cp[cnt-2].time-cp[cnt-1].time;
		cp[0].percent = cp[1].percent+(cp[cnt-2].percent-cp[cnt-1].percent);
		cp[cnt].time = cp[cnt-1].time+cp[2].time;
		cp[cnt].percent = cp[cnt-1].percent+(cp[2].percent-cp[1].percent);
	//	cp[cnt-1].percent = cp[1].percent;
	}
	else
	{
		//double anchors for non-looping case
		cp[0].time = cp[1].time;
		cp[0].percent = cp[1].percent;
		cp[cnt].time = cp[cnt-1].time;
		cp[cnt].percent = cp[cnt-1].percent;
	}
}

void MorphMesh::Update(SINGLE dt)
{
	animTimer += dt;
	if (animQueue)
	{
		if (animTimer-animQueue->triggerTime >= animQueue->transTime)
		{
			currentAnim = animQueue->nextAnim;
			animTimer = 0;
			bLooping = animQueue->bLooping;
			AnimQueue *temp = animQueue;
			animQueue = animQueue->next;
			delete temp;
			last_cp = 0;
			
			ControlPoint *cp = anims[currentAnim].cp;
			int cnt = anims[currentAnim].numControlPoints;
			if (bLooping)
			{
				cp[0].time = cp[cnt-2].time-cp[cnt-1].time;
				cp[0].percent = cp[cnt-2].percent;
				cp[cnt].time = cp[cnt-1].time+cp[2].time;
				cp[cnt].percent = cp[cnt-1].percent+cp[2].percent;
			//	cp[cnt-1].percent = cp[1].percent;
			}
			else
			{
				//double anchors for non-looping case
				cp[0].time = cp[1].time;
				cp[0].percent = cp[1].percent;
				cp[cnt].time = cp[cnt-1].time;
				cp[cnt].percent = cp[cnt-1].percent;
			}
		}
	}
	else while (bLooping && animTimer > anims[currentAnim].totalTime)
	{
		animTimer -= anims[currentAnim].totalTime;
	}
}

/*void MorphMesh::MorphTo(IMorphMesh *_future)
{
	future = (MorphMesh *)_future->GetBase();
}

void *MorphMesh::GetBase()
{
	return this;
}*/

///////////////////////////////////////////////////////////////////////////////////////
GENRESULT CreateMorphMesh(COMPTR<IMorphMesh> &imm,struct MorphMeshArchetype * arch)
{
	MorphMesh *mm = new DAComponent<MorphMesh>;

	GENRESULT result = mm->QueryInterface("IMorphMesh",imm);
	mm->anims = arch->anims;
	mm->numAnims = arch->numAnims;
	mm->Release();

	return result;
}

MorphMeshArchetype *CreateMorphMeshArchetype(char *filename)
{
	MorphMeshArchetype *mm_arch = new MorphMeshArchetype;

	if (mm_arch->ReadINI(filename))
		return mm_arch;
	else
	{
		delete mm_arch;
		return 0;
	}
}

void DestroyMorphMeshArchetype(MorphMeshArchetype *mm_arch)
{
	delete mm_arch;
}
///////////////////////////////////////////////////////////////////////////////////////
void ComputeCoeff(ControlPoint & a, ControlPoint & b, ControlPoint & c, ControlPoint & d, ControlPoint p[4])
{
//
// Catmull-Rom basis.
//
#define B Bcr
	
	a = B[0][0] * p[0] + B[0][1] * p[1] + B[0][2] * p[2] + B[0][3] * p[3];
	b = B[1][0] * p[0] + B[1][1] * p[1] + B[1][2] * p[2] + B[1][3] * p[3];
	c = B[2][0] * p[0] + B[2][1] * p[1] + B[2][2] * p[2] + B[2][3] * p[3];
	d = B[3][0] * p[0] + B[3][1] * p[1] + B[3][2] * p[2] + B[3][3] * p[3];
}

//---------------------------------------------------------------------------
//------------------------End MorphMesh.cpp----------------------------------
//---------------------------------------------------------------------------