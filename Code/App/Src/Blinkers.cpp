//--------------------------------------------------------------------------//
//                                                                          //
//                               Blinkers.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Blinkers.cpp 10    4/24/00 5:12p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <stdio.h>

#include "TSmartpointer.h"
#include "CQTrace.h"
#include "IBlinkers.h"
#include "ArchHolder.h"
#include "IExplosion.h"

#include <FileSys.h>
#include <Renderer.h>
#include <Mesh.h>
#include <HeapObj.h>
#include <IHardpoint.h>


struct Blinker
{
	SINGLE blinkTime,lifeTime;
	Vector pos;
	ARCHETYPE_INDEX owner_arch_id;
};

S32 iniStage;

void GetAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize)
{
	if (last < arraySize)
	{
		array[last] = instanceIndex;
		last++;
		INSTANCE_INDEX lastChild = INVALID_INSTANCE_INDEX,child;
		while ((child = ENGINE->get_instance_child_next(instanceIndex,EN_DONT_RECURSE,lastChild)) != INVALID_INSTANCE_INDEX)
		{
			Vector childPos = ENGINE->get_position(child);
			GetAllChildren(child,array,last,arraySize);
			lastChild = child;
		}
	}
}

#define MAX_HARDPOINTS 64
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct BlinkersArchetype : IProfileCallback
{
	struct Blinker *tempList,*blinker;
	S32 numHardpoints;
	SINGLE totalTime;
	SINGLE currentBlinkTime,currentBlinkLife;
	ARCHETYPE_INDEX parentIndex;
	//TEMP INDEX - fix model.h and take this OUT
	INSTANCE_INDEX instanceIndex;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	BlinkersArchetype()
	{
	}
	
	~BlinkersArchetype()
	{
		delete [] blinker;
	}
	
	BOOL32 __stdcall ProfileCallback(struct IProfileParser2 * parser, const C8 * sectionName, const C8 * value,void *context);
	
	void ReadINI(const char *filename);
};

/*static BOOL32 __stdcall Global_INICallback(struct IProfileParser * parser, const C8 * sectionName,void *context)
{
	BlinkersArchetype *blink_arch = (BlinkersArchetype *)context;
	return blink_arch->INICallback(parser,sectionName,NULL);
}*/

BOOL32 BlinkersArchetype::ProfileCallback(struct IProfileParser2 * parser, const C8 * sectionName,const C8 * value,void *context)
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
		if (strcmp(sectionName,"Hardpoints"))
		{
			CQERROR0("Expected [Hardpoints]");
			goto Done;
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
			
				if ((len = parser->ReadKeyValue(hSection, "TotalTime", buffer, sizeof(buffer))) != 0)
				{
					if (sscanf(buffer,"%f",&totalTime) == -1)
					{
						CQERROR0("Error reading TotalTime field");
						goto Done;
					}
				}
				else
				{
					CQERROR0("Need TotalTime field");
					goto Done;
				}
				iniStage++;
			}
			break;
		case 1:
			{
				DAFILEDESC fdesc;
				COMPTR<IFileSystem> file;

				U32 len;
				int line = 0;

				if ((len = parser->ReadKeyValue(hSection, "BlinkTime", buffer, sizeof(buffer))) != 0)
				{
					if (sscanf(buffer,"%f",&currentBlinkTime) == -1)
					{
						CQERROR0("Error reading BlinkTime field");
						goto Done;
					}
				}
				else
				{
					CQERROR0("Need BlinkTime field");
					goto Done;
				}

				if ((len = parser->ReadKeyValue(hSection, "BlinkLife", buffer, sizeof(buffer))) != 0)
				{
					if (sscanf(buffer,"%f",&currentBlinkLife) == -1)
					{
						CQERROR0("Error reading BlinkLife field");
						goto Done;
					}
				}
				else
				{
					CQERROR0("Need BlinkLife field");
					goto Done;
				}

				
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					line++;
					//if line is a hardpoint
					if (buffer[0] == '\\')
					{
						INSTANCE_INDEX childIndex;
						HardpointInfo hardpointinfo;

						CQASSERT(numHardpoints < MAX_HARDPOINTS && "Too many blinkies");
						FindHardpoint(buffer,childIndex,hardpointinfo,instanceIndex);
						if (childIndex != INVALID_INSTANCE_INDEX)
						{
							tempList[numHardpoints].owner_arch_id = ENGINE->get_instance_archetype(childIndex);
							//release reference early
							ENGINE->release_archetype(tempList[numHardpoints].owner_arch_id);
							tempList[numHardpoints].pos = hardpointinfo.point;
							tempList[numHardpoints].blinkTime = currentBlinkTime;
							tempList[numHardpoints].lifeTime = currentBlinkLife;
							numHardpoints++;
						}
					}
				}
			}
			break;
		}
	}
	else goto Done;

	result = 1;

Done:

	return result;
}

void BlinkersArchetype::ReadINI(const char *filename)
{
	instanceIndex = ENGINE->create_instance2(parentIndex,NULL);
	//unload previous info - anims should be 0 if struct is new
	if (blinker)
	{
		delete [] blinker;
		blinker = 0;
	}

	tempList = new Blinker[MAX_HARDPOINTS];
	
	iniStage = 0;
	
	struct PROFPARSEDESC pdesc;
	
	COMPTR<IProfileParser2> parser;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc;
	
	if (CreateProfileParser(filename, parser) == GR_OK)
	{
		if (parser->EnumerateSections (this) == 0)
		{
			CQERROR0("Failed to parse INI file");
			if (blinker)
			{
				delete [] blinker;
				blinker = 0;
			}
			goto Done;
		}
	}
	else
	{
		CQERROR1("Can't open ini file - %s",filename);
		goto Done;
	}
	
	blinker = new Blinker[numHardpoints];
	memcpy(blinker,tempList,numHardpoints*sizeof(Blinker));
	
Done:
	delete [] tempList;
	ENGINE->destroy_instance(instanceIndex);
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct Blinkers : IBlinkers
{
	BEGIN_DACOM_MAP_INBOUND(Blinkers)
	DACOM_INTERFACE_ENTRY (IDAComponent)
	DACOM_INTERFACE_ENTRY (IBlinkers)
	END_DACOM_MAP()
	
	S32 numStages,numBlinkers;
	SINGLE timer,totalTime;
	U32 currentStage;
//	SINGLE *endTimes;
	Blinker *blinker;
	bool bNoOverlap;
	INSTANCE_INDEX *owner_idx;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	Blinkers::Blinkers();
	~Blinkers();

	//returns number of points
	virtual S32 GetBlinkers(Vector *points,SINGLE *intensity,S32 arraySize);
	virtual void Update(SINGLE dt);
};

Blinkers::Blinkers()
{
	currentStage = 0;
}

Blinkers::~Blinkers()
{
	delete [] owner_idx;
}

//returns number of points
S32 Blinkers::GetBlinkers(Vector *points,SINGLE *intensity,S32 arraySize)
{
	int i=0;
	int total=0;
	while (i < numBlinkers && total < arraySize)
	{
		SINGLE dist = fabs(timer-blinker[i].blinkTime);
		if (dist < blinker[i].lifeTime*0.5)
		{
			TRANSFORM trans = ENGINE->get_transform(owner_idx[i]);
			points[total] = trans*blinker[i].pos;
			intensity[total] = 1-(dist*2 / blinker[i].lifeTime);
			total++;
		}
		i++;
	}

	return total;
}

void Blinkers::Update(SINGLE dt)
{
	CQASSERT(totalTime);
	timer += dt;
	if (timer > totalTime)
	{
		while (timer > totalTime)
			timer -= totalTime;
		currentStage = 0;
	}

/*	if (bNoOverlap)
	{
		while (timer > endTimes[currentStage])
		{
			currentStage++;
			CQASSERT(currentStage <= numStages);
		}
	}*/
}

///////////////////////////////////////////////////////////////////////////////////////
GENRESULT CreateBlinkers(COMPTR<IBlinkers> &ibs,struct BlinkersArchetype * arch,INSTANCE_INDEX instanceIndex)
{
	if (arch->totalTime <= 0.0)
		return GR_GENERIC;

	Blinkers *bs = new DAComponent<Blinkers>;

	GENRESULT result = bs->QueryInterface("IBlinkers",ibs);
	bs->blinker = arch->blinker;
	bs->numBlinkers = arch->numHardpoints;
	bs->totalTime = arch->totalTime;

	bs->owner_idx = new INSTANCE_INDEX[bs->numBlinkers];

	//ugly ugly ugly
	S32 children[MAX_CHILDS];
	S32 num_children=0;
	HARCH child_arch_ids[MAX_CHILDS];
	GetAllChildren(instanceIndex,children,num_children,MAX_CHILDS);
	int j;
	for (j=0;j<num_children;j++)
	{	
		child_arch_ids[j] = children[j];
	}

	for (int i=0;i<bs->numBlinkers;i++)
	{
		for (j=0;j<num_children;j++)
		{
			if (child_arch_ids[j] == arch->blinker[i].owner_arch_id)
				bs->owner_idx[i] = children[j];
		}
	}
	bs->Release();

	return result;
}

BlinkersArchetype *CreateBlinkersArchetype(const char *filename,ARCHETYPE_INDEX _parentIndex)
{
	BlinkersArchetype *bs_arch = new BlinkersArchetype;

	bs_arch->parentIndex = _parentIndex;

	bs_arch->ReadINI(filename);

	return bs_arch;
}

void DestroyBlinkersArchetype(BlinkersArchetype *bs_arch)
{
	delete bs_arch;
}
//---------------------------------------------------------------------------
//------------------------End Blinkers.cpp----------------------------------
//---------------------------------------------------------------------------