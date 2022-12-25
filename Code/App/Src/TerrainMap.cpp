//--------------------------------------------------------------------------//
//                                                                          //
//                               TerrainMap.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TerrainMap.cpp 112   10/17/00 9:48a Jasony $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "TerrainMap.h"
#include "Startup.h"
#include "TObject.h"
#include "GridVector.h"

#include <Physics.h>
#include <HeapObj.h>
#include <TComponent.h>
#include <stdlib.h>


#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)

#define MAX_MAP_SIZE 64
#define MAX_FOOTPRINT 16

#define AXIAL_LENGTH    1.0f
#define DIAGONAL_LENGTH 1.41421f
#define INFTY			10000000.0f
#define INFTY_LITE       4000000.0f

#define OPEN_START   1
#define CLOSED_START 2

extern "C" void __cdecl clearmemFPU (void * buffer, U32 bufferSize);

struct CellRef;
struct TerrainMap;

#define NUM_STEPS 162
static const POINT g_steps[NUM_STEPS] = 
{ 
	{ 0, 0}, { 0, 1}, { 1, 0}, { 0,-1}, {-1, 0}, { 1, 1}, { 1,-1}, {-1,-1}, {-1, 1} ,	// 0 - 8
	{ 0, 2}, { 2, 0}, { 0,-2}, {-2, 0}, { 1, 2}, { 2,-1}, {-1,-2}, {-2, 1}, { 2, 1}	,	// 9 - 17
	{ 1,-2}, {-2,-1}, {-1, 2}, { 2, 2}, { 2,-2}, {-2,-2}, {-2, 2}, { 0, 3}, { 3, 0}	,	// 18 - 26
	{ 0,-3}, {-3, 0}, { 1, 3}, { 3,-1}, {-1,-3}, {-3, 1}, {-1, 3}, { 3, 1}, { 1,-3} ,   // 27 - 35
	{-3,-1}, {-2, 3}, { 2, 3}, { 3, 2}, { 3,-2}, { 2,-3}, {-2,-3}, {-3,-2}, {-3, 2}	,	// 36 - 44
	{-3, 3}, { 3, 3}, { 3,-3}, {-3,-3}, {-4, 0}, { 0, 4}, { 4, 0}, { 0,-4}, {-4, 1}	,	// 45 - 53
	{ 1, 4}, { 4,-1}, {-1,-4}, {-4,-1}, {-1, 4}, { 4, 1}, { 1,-4}, {-4,-2}, {-2, 4} ,	// 54 - 62
	{ 4, 2}, { 2,-4}, {-2,-4}, {-4, 2}, { 2, 4}, { 4,-2}, { 3,-4}, {-3,-4}, {-4,-3} ,	// 63 - 71
	{-4, 3}, {-3, 4}, { 3, 4}, { 4, 3}, { 4,-3}, { 4,-4}, {-4,-4}, {-4, 4}, { 4, 4}	,	// 72 - 80
	{ 5, 5}, { 5, 4}, { 5, 3}, { 5, 2}, { 5, 1}, { 5, 0}, { 5,-1}, { 5,-2}, { 5,-3} ,	// 81 - 89
	{ 5,-4}, { 5,-5}, { 4,-5}, { 3,-5}, { 2,-5}, { 1,-5}, { 0,-5}, {-1,-5}, {-2,-5} ,	// 90 - 98
	{-3,-5}, {-4,-5}, {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5, 0}, {-5, 1} ,	// 99 - 107
	{-5, 2}, {-5, 3}, {-5, 4}, {-5, 5}, {-4, 5}, {-3, 5}, {-2, 5}, {-1, 5}, { 0, 5} ,	// 108 - 116
	{ 1, 5}, { 2, 5}, { 3, 5}, { 4, 5}, { 5, 5}, { 6, 5}, { 6, 4}, { 6, 3}, { 6, 2}	,	// 117 - 125
	{ 6, 1}, { 6, 0}, { 6,-1}, { 6,-2}, { 6,-3}, { 6,-4}, { 6,-5}, { 6,-6}, { 5,-6} ,	// 126 - 134
	{ 4,-6}, { 3,-6}, { 2,-6}, { 1,-6}, { 0,-6}, {-1,-6}, {-2,-6}, {-3,-6}, {-4,-6} ,	// 135 - 143
	{-5,-6}, {-6,-6}, {-6,-5}, {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6, 0}, {-6, 1}	,	// 144 - 152
	{-6, 2}, {-6, 3}, {-6, 4}, {-6, 5}, {-6, 6}, {-5, 6}, {-4, 6}, {-3, 6}, {-2, 6}		// 153 - 161
};

//----------------------------------------------------------------------------------------------
//
static const U32 pos_to_flag (SINGLE xf, SINGLE yf)
{
	U32 flag;

	int x = xf;
	int y = yf;

	SINGLE xdif = xf - x;
	SINGLE ydif = yf - y;
	
	if (xdif < 0.5f && ydif < 0.5f)
	{
		return flag = TERRAIN_BOTTOMLEFT;
	}
	if (xdif < 0.5f && ydif >= 0.5f)
	{
		return flag = TERRAIN_TOPLEFT;
	}
	if (xdif >= 0.5f && ydif >= 0.5f)
	{
		return flag = TERRAIN_TOPRIGHT;
	}

	return flag = TERRAIN_BOTTOMRIGHT;
}

// PATH FINDING STUFF
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//

struct CellRef
{
	int x;
	int y;

	CellRef ()
	{
		x = 0; 
		y = 0;
	}

	CellRef (int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	inline const CellRef & operator += (const CellRef & c)
	{
		x += c.x;
		y += c.y;
		return *this;
	}

	inline friend CellRef operator + (const CellRef& rhs, const CellRef& lhs)
	{
		return CellRef(rhs.x + lhs.x, rhs.y + lhs.y);
	}

	inline friend bool operator == (const CellRef& lhs, const CellRef& rhs)
	{
		return (lhs.x == rhs.x) && (lhs.y == rhs.y);
	}
};

static const CellRef g_cellDirections[8] = 
{
	CellRef(0,1), CellRef(1,0), CellRef(0,-1), CellRef(-1,0), CellRef(1,1), CellRef(1,-1),  CellRef(-1,-1), CellRef(-1,1) 
};

static const SINGLE g_cellCosts[2] = { AXIAL_LENGTH, DIAGONAL_LENGTH };

// needed for path finding
struct PathNode
{
	SINGLE g;
	SINGLE h;
	SINGLE f;

	PathNode* pNext;
	PathNode* pPrev;

	U32 priorityID;

	CellRef currentCell;
	PathNode* parentNode;

	PathNode ()
	{
		memset(this, 0, sizeof(PathNode));
	}
};

class PriorityQueue
{
	PathNode * pFirst;
	PathNode * pLast;

	U32 uniqueID;

public:
	PriorityQueue ()
	{
		pFirst = NULL;
		pLast  = NULL;
	}

	void set_id (U32 id)
	{
		uniqueID = id;
	}

	bool empty ()
	{
		return pFirst == NULL;
	}

	void push (PathNode* pNode)
	{
		pNode->priorityID = uniqueID;

		if (pFirst == NULL)
		{
			// add the first object to the list
			pNode->pNext = NULL;
			pNode->pPrev = NULL;
			pFirst = pNode;
			pLast = pFirst;
			return;
		}

		// go through the linked list and insert the node in order
		PathNode * p = pFirst;

		while (p)
		{
			if (pNode->f <= p->f)
			{
				// insert the node

				// special case if we're to insert at the front of the list
				if (p == pFirst)
				{
					pNode->pPrev = NULL;
					pNode->pNext = p;
					p->pPrev = pNode;
					pFirst = pNode;
					return; 
				}

				pNode->pNext = p;
				pNode->pPrev = p->pPrev;

				pNode->pPrev->pNext = pNode;
				pNode->pNext->pPrev = pNode;
				return;
			}
			p = p->pNext;
		}

		// if we're here, add it to the back
		pNode->pNext = NULL;
		pNode->pPrev = pLast;
		pLast->pNext = pNode;
		pLast = pNode;
	}

	PathNode* pop ()
	{
		// take the first item off the list
		if (pFirst)
		{
			PathNode * pNode = pFirst;
			
			pFirst = pFirst->pNext;
			if (pFirst)
			{
				pFirst->pPrev = NULL;
			}
			
			pNode->pNext = NULL;

			CQASSERT(pNode->pPrev == NULL);
			pNode->priorityID = 0;
			return pNode;
		}
		else
		{
			return NULL;
		}
	}

	void flush (U32 id)
	{
		// empty the list
		pFirst = NULL;
		pLast = NULL;
		set_id(id);
	}

	bool contains (PathNode* pNode)
	{
		return (pNode->priorityID == uniqueID);
	}

	void remove (PathNode* pNode)
	{
		PathNode * p = pFirst;

		while (p)
		{
			if (p->currentCell == pNode->currentCell)
			{
				// remove 'p' from the linked list

				if (p == pFirst)
				{
					// special case:  'p' is first on the list
					pFirst = p->pNext;
					
					if (pFirst)
					{
						pFirst->pPrev = NULL;
					}
				}
				else if (p == pLast)
				{
					// special case:  'p' is last on the list
					pLast = p->pPrev;

					if (pLast)
					{
						pLast->pNext = NULL;
					}
				}
				else
				{
					p->pPrev->pNext = p->pNext;
					p->pNext->pPrev = p->pPrev;
				}

				p->pNext = NULL;
				p->pPrev = NULL;
				p->priorityID = 0;
				return;
			}
			p = p->pNext;
		}
	}
};
//-------------------------------------------------------------------
//
#define SUBBLOCK_NODES 16
template <class Type>
struct BlockAllocator
{
	struct SUBNODE : Type
	{
		SUBNODE * pNext;
	};

	struct SUBBLOCK
	{
		SUBBLOCK * pNext;
		SUBNODE    node[SUBBLOCK_NODES];
	};
	//
	// list items
	// 
	SUBBLOCK * pBlockList;
	SUBNODE  * pFreeNodeList;
	//
	// methods
	//
	BlockAllocator (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~BlockAllocator (void)
	{
		reset();
	}

	// WARNING: make sure you have shut down all FootprintList's that reference us
	void reset (void)
	{
		SUBBLOCK * node = pBlockList;
		while (node)
		{
			pBlockList = node->pNext;
			delete node;
			node = pBlockList;
		}

		pFreeNodeList = 0;
	}

	void addBlock (void)
	{
		SUBBLOCK * node = new SUBBLOCK;
		node->pNext = pBlockList;
		pBlockList = node;

		int i;
		for (i = 0; i < SUBBLOCK_NODES-1; i++)
		{
			node->node[i].pNext = &node->node[i+1];
		}
		node->node[SUBBLOCK_NODES-1].pNext = pFreeNodeList;
		pFreeNodeList = &node->node[0];
	}

	SUBNODE * alloc (void)
	{
		SUBNODE * result=pFreeNodeList;
		if (result==0)
		{
			addBlock();
			result=pFreeNodeList;
		}
		pFreeNodeList = pFreeNodeList->pNext;
		return result;
	}

	void free (SUBNODE * node)
	{
		node->pNext = pFreeNodeList;
		pFreeNodeList = node;
	}
};
//-------------------------------------------------------------------
static BlockAllocator<FootprintInfo> blockAlloc;
void __stdcall ResetTerrainBlockAllocator (void)
{
	blockAlloc.reset();
}
//-------------------------------------------------------------------
//
template <class Type>
struct InfoList
{
	typename BlockAllocator<Type>::SUBNODE * pList, *pEnd;

	InfoList (void)
	{
		pList = pEnd = 0;
	}

	~InfoList (void)
	{
		reset();
	}

	void reset (void)
	{
		BlockAllocator<Type>::SUBNODE * node = pList;
		while (node)
		{
			pList = node->pNext;
			blockAlloc.free(node);
			node = pList;
		}
		pEnd = 0;
	}

	void add (const Type & data)
	{
		BlockAllocator<Type>::SUBNODE * node = blockAlloc.alloc();

		*(static_cast<Type *>(node)) = data;
		
		node->pNext = 0;
		if (pEnd)
		{
			pEnd->pNext = node;
			pEnd = node;
		}
		else
		{
			pEnd = pList = node;
		}
	}
};
//-------------------------------------------------------------------
//
template <class Type>
struct InfoIterator
{
	typename BlockAllocator<Type>::SUBNODE * pNode, *pPrev;
	InfoList<Type> & list;

	InfoIterator (InfoList<Type> & _list) : list(_list)
	{
		pPrev = 0;
		pNode = list.pList;
	}

	void reset (void)
	{
		pPrev = 0;
		pNode = list.pList;
	}

	operator bool (void) const
	{
		return (pNode!=0);
	}

	Type * operator ++ (void)
	{
		pPrev = pNode;
		if (pNode)
			pNode = pNode->pNext;

		return pNode;
	}

	Type * operator -> (void)
	{
		return pNode;
	}

	void remove (void)		// remove current element, advance to next element
	{
		if (pNode)
		{
			if (pPrev)
			{
				if ((pPrev->pNext = pNode->pNext) == 0)
					list.pEnd = pPrev;
			}
			else
			{
				if ((list.pList = pNode->pNext) == 0)
					list.pEnd = 0;
			}

			blockAlloc.free(pNode);
			if (pPrev)
				pNode = pPrev->pNext;
			else
				pNode = list.pList;
		}
	}
};

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct FootprintList
{
	typedef InfoList<FootprintInfo> FOOTPRINTARRAY;

	FOOTPRINTARRAY fpInfoList;

	int count;
	U32 allFlags;

	// needed for path finding
	PathNode node;

	FootprintList ()
	{
		count = 0;
		allFlags = 0;
	}

	void Add (const FootprintInfo& fpInfo)
	{
		fpInfoList.add(fpInfo);
		count++;

		allFlags |= fpInfo.flags;
	}

	void Undo (const FootprintInfo& fpInfo)
	{
		InfoIterator<FootprintInfo> it(fpInfoList);

		while (it)
		{
			if (it->missionID == fpInfo.missionID)
			{
				// delete it from the list man!
				count--;
				it.remove();
			}
			else
				++it;
		}
		ResetFlags();
	}

	int GetFootprintCount (void)
	{
		return count;
	}

	void ResetFlags (void)
	{
		allFlags = 0;
		InfoIterator<FootprintInfo> it(fpInfoList);

		while (it)
		{
			allFlags |= it->flags;
			++it;
		}
	}

	U32 GetMissionFlags (U32 missionID, U32 cornerID = 0)
	{
		U32 retFlag = 0;
		InfoIterator<FootprintInfo> it(fpInfoList);

		while (it)
		{
			// get all flags associated with the mission ID
			if (cornerID)
			{
				if ((it->missionID == missionID) && (it->flags & cornerID))
				{
					retFlag |= it->flags;
				}
			}
			else
			{
				if ((it->missionID == missionID))
				{
					retFlag |= it->flags;
				}
			}

			++it;
		}
		return retFlag;
	}

/*	void Reset (void)
	{
		if (allFlags & TERRAIN_OUTOFSYSTEM)
		{
			count = 1;
			allFlags = TERRAIN_OUTOFSYSTEM | TERRAIN_IMPASSIBLE | TERRAIN_FULLSQUARE;
			fpInfoList[0].flags = allFlags;
		}
		else
		{
			allFlags = 0;
			count = 0;
		}
	}
*/
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct MapMatrix
{
	FootprintList* matrix[MAX_MAP_SIZE][MAX_MAP_SIZE];	// a double array of pointers to FootprintList objects
	const U32 nDimension;

	// used for path finding
	static PriorityQueue QOpen;
	static PriorityQueue QClosed;
	static int nMaps;
	static U32 openID;
	static U32 closedID;

	TerrainMap *terrainMap;

	MapMatrix (const int dimension, TerrainMap *pTerrain) : nDimension(dimension)
	{
		CQASSERT(dimension > 0);

		nMaps++;

		// size the matrix
		U32 i, j;
		SINGLE fCntr = SINGLE(nDimension)/2.0f;
		SINGLE x, y;

		FootprintInfo fpinfo;
		memset(&fpinfo, 0, sizeof(FootprintInfo));
		fpinfo.flags = TERRAIN_FULLSQUARE | TERRAIN_OUTOFSYSTEM | TERRAIN_IMPASSIBLE | TERRAIN_FULLYTAKEN;
		
		for (i = 0; i < nDimension; i++)
		{
			for (j = 0; j < nDimension; j++)
			{
				matrix[i][j] = new FootprintList;
				matrix[i][j]->node.currentCell = CellRef(i, j);

				x = i + 0.5f;
				y = j + 0.5f;

				// set the out of bounds stuff
				SINGLE xf = (fCntr - x);
				SINGLE yf = (fCntr - y);
				SINGLE fdist = sqrt(xf*xf + yf*yf);
				if (fdist > fCntr)
				{
					// set a footprint for the out of system stuff
					matrix[i][j]->Add(fpinfo);
				}
			}
		}
		terrainMap = pTerrain;
	}

	~MapMatrix (void)
	{
		nMaps--;

		CQASSERT(nMaps >= 0);

		if (nMaps == 0)
		{
			openID = OPEN_START;
			closedID = CLOSED_START;
		}

		// delete everything
		U32 i, j;
		for (i = 0; i < nDimension; i++)
		{
			for (j = 0; j < nDimension; j++)
			{
				if (matrix[i][j])
				{
					delete matrix[i][j];
					matrix[i][j] = NULL;
				}
			}
		}
	}

	bool AStarPathClense (GRIDVECTOR * gridpath, int& numgrids);

	const int GetFootprintCount (const U32 x, const U32 y)
	{
		if ((x >= 0 && x < nDimension) && (y >= 0 && y < nDimension))
		{
			return matrix[x][y]->GetFootprintCount();
		}
		else
		{
			return 0;
		}
	}

	const U32 GetFieldID (const U32 x, const U32 y)
	{
		if (x >= nDimension || y >= nDimension)
			return 0;

		if (matrix[x][y]->allFlags & TERRAIN_FIELD)
		{
			// find the field ID
			InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);
			while (it)
			{
				if (it->flags & TERRAIN_FIELD)
				{
					return it->missionID;
				}
				++it;
			}
		}

		// if we've gotten here, than this square is not part of a field
		return 0;
	}

	const bool IsDestinationOpen (const U32 x, const U32 y, const U32 dwMissionID)
	{
		U32 dwFlags;
		InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

		// if the square is out of bounds, then keep moving
		if (matrix[x][y]->allFlags & TERRAIN_OUTOFSYSTEM)
		{
			return false;
		}

		while (it)
		{
			if (dwMissionID == it->missionID)
			{
				// ignore footprints that this missionID already set
				++it;
				continue;
			}

			dwFlags = it->flags;
			if (((it->missionID & PLAYERID_MASK) == (dwMissionID & PLAYERID_MASK)) && (dwFlags & TERRAIN_DESTINATION))
			{
				// there is a player of the same ID requesting this spot
				return false;
			}
			if (dwFlags & (TERRAIN_PARKED | TERRAIN_IMPASSIBLE | TERRAIN_OUTOFSYSTEM))
			{
				// there is someone parked here already
				return false;
			}

			++it;
		}
		return true;
	}

	const bool IsParkedAtSquare (const U32 x, const U32 y, const U32 dwMissionID)
	{
		U32 dwFlags;
		InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

		// if the square is out of bounds, then keep moving
		if (matrix[x][y]->allFlags & TERRAIN_OUTOFSYSTEM)
		{
			return false;
		}

		while (it)
		{
			if (dwMissionID == it->missionID)
			{
				dwFlags = it->flags;
				if (dwFlags & TERRAIN_PARKED)
					return true;
			}

			++it;
		}
		return false;
	}

	const bool IsParkedAtCorner (const U32 x, const U32 y, const U32 dwMissionID, const U32 dwCornerID)
	{
		// if the square is completely taken, then keep moving
		U32 allFlags;
		allFlags = matrix[x][y]->allFlags;
		if (matrix[x][y]->allFlags & TERRAIN_FULLYTAKEN)
		{
			return false;
		}

		InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);
		U32 dwFlags;

		while (it)
		{
			dwFlags = it->flags;

			if (dwFlags & dwCornerID)
			{
				if (dwMissionID == it->missionID)
				{
					if (dwFlags & TERRAIN_PARKED)
						return true;
				}
			}

			++it;
		}
		return false;
	}

	const bool IsOkForBuildingAtSquare (const U32 x, const U32 y, bool checkParkedUnits)
	{
		U32 dwFlags;
		const U32 testFlags = (checkParkedUnits) ? (TERRAIN_PARKED|TERRAIN_OUTOFSYSTEM|TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS) : (TERRAIN_OUTOFSYSTEM|TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS);

		// no building out of bounds, or in terrain
		if (matrix[x][y]->allFlags & testFlags)
		{
			return false;
		}

		if (checkParkedUnits)
		{
			InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

			while (it)
			{
				dwFlags = it->flags;
				if (dwFlags & testFlags)
					return false;

				++it;
			}
		}
		return true;
	}

	const bool IsOkForBuildingAtCorner (const U32 x, const U32 y, bool checkParkedUnits, const U32 dwCornerID)
	{
		U32 dwFlags;
		const U32 testFlags = (checkParkedUnits) ? (TERRAIN_PARKED|TERRAIN_OUTOFSYSTEM|TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS) : (TERRAIN_OUTOFSYSTEM|TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS);

		// no building out of bounds, or in terrain
		if (matrix[x][y]->allFlags & testFlags)
		{
			return false;
		}

		if (checkParkedUnits)
		{
			InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

			while (it)
			{
				dwFlags = it->flags;
				if (dwFlags & dwCornerID)
				{
					if (dwFlags & testFlags)
						return false;
				}

				++it;
			}
		}
		return true;
	}

	const bool IsCornerDestinationOpen (const U32 x, const U32 y, const U32 dwMissionID, const U32 dwCornerID);

	const bool IsCellValid (const int& x, const int& y)
	{
		return (matrix[x][y]->allFlags & TERRAIN_IMPASSIBLE) == false;
	}

	const bool IsCellInSystem (const int& x, const int& y)
	{
		return (!(matrix[x][y]->allFlags & TERRAIN_OUTOFSYSTEM));
	}

	const U32& GetFlags (const U32& x, const U32& y)
	{
		CQASSERT(x < nDimension && y < nDimension);

		return matrix[x][y]->allFlags;
	}

	const U32 GetQuarterFlags (const int& x, const int& y, const U32& dwQuarter)
	{
		U32 flags = 0;
		InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

		while (it)
		{
			if (it->flags & dwQuarter)
			{
				flags |= it->flags;
			}

			++it;
		}

		if (matrix[x][y]->allFlags & TERRAIN_FULLYTAKEN)
		{
			flags |= TERRAIN_FULLYTAKEN;
		}
		if (matrix[x][y]->allFlags & TERRAIN_IMPASSIBLE)
		{
			flags |= TERRAIN_IMPASSIBLE;
		}

		return flags;
	}

	void RenderFootprints (void)
	{
		U32 i, j;

		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

		for (i = 0; i < nDimension; i++)
		{
			for (j = 0; j < nDimension; j++)
			{
				U32 flags = matrix[i][j]->allFlags;

				if (matrix[i][j]->count > 0)
				{
					SINGLE x0 = SINGLE(i)*GRIDSIZE;
					SINGLE y0 = SINGLE(j)*GRIDSIZE;
					SINGLE x1 = SINGLE(i+1)*GRIDSIZE;
					SINGLE y1 = SINGLE(j+1)*GRIDSIZE;
					SINGLE xM = x0 + HALFGRID;
					SINGLE yM = y0 + HALFGRID;

					if (flags & TERRAIN_FULLSQUARE)
					{
						if (flags & TERRAIN_IMPASSIBLE)
						{
							PB.Color4ub(180, 50, 80, 100);
						}
						else
						{
							if (flags & TERRAIN_DESTINATION)
							{
								PB.Color4ub(255, 255, 0, 100);
							}
							else if (flags & TERRAIN_PARKED)
							{
								PB.Color4ub(255, 255, 255, 100);
							}
							else
							{
								PB.Color4ub(80, 50, 180, 100);
							}
						}

						PB.Begin(PB_QUADS);
						PB.Vertex3f(x0, y0, 0);
						PB.Vertex3f(x1, y0, 0);
						PB.Vertex3f(x1, y1, 0);
						PB.Vertex3f(x0, y1, 0);
						PB.End();
					}
					else
					{
						// all small terrain features is blue unless it's a destination
						if (flags & TERRAIN_DESTINATION)
						{
							PB.Color4ub(80, 180, 50, 100);
						}
						else if (flags & TERRAIN_PARKED)
						{
							PB.Color4ub(0, 255, 255, 100);
						}
						else
						{
							PB.Color4ub(80, 50, 180, 100);
						}
						
						PB.Begin(PB_QUADS);

						if (flags & TERRAIN_TOPLEFT)
						{
							PB.Vertex3f(x0, yM, 0);
							PB.Vertex3f(xM, yM, 0);
							PB.Vertex3f(xM, y1, 0);
							PB.Vertex3f(x0, y1, 0);
						}
						if (flags & TERRAIN_TOPRIGHT)
						{
							PB.Vertex3f(xM, yM, 0);
							PB.Vertex3f(x1, yM, 0);
							PB.Vertex3f(x1, y1, 0);
							PB.Vertex3f(xM, y1, 0);
						}
						if (flags & TERRAIN_BOTTOMLEFT)
						{
							PB.Vertex3f(x0, y0, 0);
							PB.Vertex3f(xM, y0, 0);
							PB.Vertex3f(xM, yM, 0);
							PB.Vertex3f(x0, yM, 0);
						}
						if (flags & TERRAIN_BOTTOMRIGHT)
						{
							PB.Vertex3f(xM, y0, 0);
							PB.Vertex3f(x1, y0, 0);
							PB.Vertex3f(x1, yM, 0);
							PB.Vertex3f(xM	, yM, 0);
						}
						PB.End();
					}
				}
			}
		}
	}

	void AddFootprint (const U32 i, const U32 j, const FootprintInfo& fpi)
	{
		CQASSERT(i < nDimension && j < nDimension && "AddFootprint in MapMatrix");
		matrix[i][j]->Add(fpi);
	}

	void UndoFootprint (const U32 i, const U32 j, const FootprintInfo& fpi)
	{
		CQASSERT(i < nDimension && j < nDimension && "Undo Footprint in MapMatrix");
		matrix[i][j]->Undo(fpi);
	}

	U32 GetMissionFlags (const int & x, const int & y, const U32 & missionID, U32 cornerID = 0)
	{
		return matrix[x][y]->GetMissionFlags(missionID, cornerID);
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	PathNode * GetNode (CellRef cell)
	{
		if ((cell.x < (int)nDimension && cell.x >= 0) && (cell.y < (int)nDimension && cell.y >= 0))
		{
			return &matrix[cell.x][cell.y]->node;
		}
		else
		{
			return NULL;
		}
	}

	SINGLE GetHeuristic (CellRef src, CellRef dst)
	{
		int deltaX = abs(src.x - dst.x);
		int deltaY = abs(src.y - dst.y);

		return DIAGONAL_LENGTH*(min(deltaX, deltaY)) + AXIAL_LENGTH * abs(deltaX - deltaY);
	}

	SINGLE GetDistance (CellRef src, CellRef dst)
	{
		SINGLE xdiff = src.x - dst.x;
		SINGLE ydiff = src.y - dst.y;
		return sqrt(xdiff*xdiff + ydiff*ydiff);
	}

	SINGLE GetCellCost (CellRef cell, int dir)
	{
		// if the cell is impassible, it's cost is INFTY
		// otherwise it's the axial or diagonal length
		U32 dwFlags = matrix[cell.x][cell.y]->allFlags;

		if (dwFlags & TERRAIN_IMPASSIBLE)
		{
			return INFTY;
		}
		else
		{
			if (dir < 4)
			{
				return AXIAL_LENGTH;
			}
			else
			{
				return DIAGONAL_LENGTH;
			}
		}
	}

	SINGLE GetCellCostIgnoreImpassible (int dir)
	{
		if (dir < 4)
		{
			return AXIAL_LENGTH;
		}
		return DIAGONAL_LENGTH;
	}

	bool AStarPath (const GRIDVECTOR &src, const GRIDVECTOR &dst, GRIDVECTOR * gridpath, int& numgrids);
};

//-------------------------------------------------------------------
//
const bool MapMatrix::IsCornerDestinationOpen (const U32 x, const U32 y, const U32 dwMissionID, const U32 dwCornerID)
{
	// if the square is completely taken, then keep moving
	U32 allFlags;
	allFlags = matrix[x][y]->allFlags;

	if (matrix[x][y]->allFlags & TERRAIN_FULLYTAKEN)
	{
		return false;
	}

	U32 dwFlags;
	bool bSamePlayer;
	U32 thisMissionID;
	InfoIterator<FootprintInfo> it(matrix[x][y]->fpInfoList);

	while (it)
	{
		thisMissionID = it->missionID;

		if (dwMissionID == thisMissionID)
		{
			// ignore footprints that this missionID already set
			++it;
			continue;
		}

		dwFlags = it->flags;
		bSamePlayer = (thisMissionID & PLAYERID_MASK) == (dwMissionID & PLAYERID_MASK);

		if (dwFlags & dwCornerID)
		{
			if (bSamePlayer && (dwFlags & TERRAIN_DESTINATION))
			{
				// there is a player of the same ID requesting this spot
				return false;
			}
			if (dwFlags & (TERRAIN_PARKED | TERRAIN_IMPASSIBLE | TERRAIN_OUTOFSYSTEM))
			{
				// there is someone else parked here buddy, keep moving
				return false;
			}
		}
		else
		{
			// is there a big freindly ship that has requested this spot
			if ((dwFlags & TERRAIN_FULLSQUARE) && (dwFlags & TERRAIN_DESTINATION) && bSamePlayer)
			{
				return false;
			}
		}

		++it;
	}
	return true;
}





// the static shit for the map matrix
PriorityQueue	MapMatrix::QOpen;
PriorityQueue	MapMatrix::QClosed;
int				MapMatrix::nMaps		= 0;
U32				MapMatrix::openID		= OPEN_START;
U32				MapMatrix::closedID		= CLOSED_START;


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TerrainMap : ITerrainMap
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(TerrainMap)
	DACOM_INTERFACE_ENTRY(ITerrainMap)
	END_DACOM_MAP()

	//
	// member data
	//
	RECT worldRect;
	
	MapMatrix * map;
	U32 dwPreferredCorner;
	SINGLE xPreferredOffset;
	SINGLE yPreferredOffset;

	//
	// methods
	//

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	TerrainMap (void);

	~TerrainMap (void)
	{
		if (map != NULL)
		{
			delete map;
			map = NULL;
		}
	}

	/* ITerrainMap methods */

	virtual void SetWorldRect (const RECT & worldRect);

	virtual bool  TestSegment (const GRIDVECTOR & from, const GRIDVECTOR & to, ITerrainSegCallback * callback); 

	virtual void  UndoFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & _info);

	virtual void SetFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & info);

	virtual int FindPath (const GRIDVECTOR & from, const GRIDVECTOR & to, U32 dwMissionID, U32 flags, IFindPathCallback * callback);

	virtual void RenderEdit (void);

	virtual bool IsGridEmpty (const GRIDVECTOR & grid, U32 ignoreID,  bool bFullSquare)
	{
		int x = grid.getX();
		int y = grid.getY();
		int nMax = map->nDimension - 1;

		if (x < 0 || x > nMax || y < 0 || y > nMax)
		{
			return false;
		}

		if (bFullSquare)
		{
			return map->IsDestinationOpen(grid.getIntX(), grid.getIntY(), ignoreID);
		}
		else
		{
			return map->IsCornerDestinationOpen(grid.getIntX(), grid.getIntY(), ignoreID, pos_to_flag(grid.getX(), grid.getY()));
		}
	}

	virtual bool IsParkedAtGrid (const GRIDVECTOR & grid, U32 dwMissionID,  bool bFullSquare)
	{
		int x = grid.getX();
		int y = grid.getY();
		int nMax = map->nDimension - 1;

		if (x < 0 || x > nMax || y < 0 || y > nMax)
		{
			return false;
		}

		if (bFullSquare)
		{
			return map->IsParkedAtSquare(grid.getIntX(), grid.getIntY(), dwMissionID);
		}
		else
		{
			return map->IsParkedAtCorner(grid.getIntX(), grid.getIntY(), dwMissionID, pos_to_flag(grid.getX(), grid.getY()));
		}
	}

	virtual bool IsOkForBuilding (const GRIDVECTOR & grid, bool checkParkedUnits, bool bFullSquare)
	{
		int x = grid.getX();
		int y = grid.getY();
		int nMax = map->nDimension - 1;

		if (x < 0 || x > nMax || y < 0 || y > nMax)
		{
			return false;
		}

		if (bFullSquare)
		{
			return map->IsOkForBuildingAtSquare(grid.getIntX(), grid.getIntY(), checkParkedUnits);
		}
		else
		{
			return map->IsOkForBuildingAtCorner(grid.getIntX(), grid.getIntY(), checkParkedUnits, pos_to_flag(grid.getX(), grid.getY()));
		}
	}

	virtual bool IsGridValid (const GRIDVECTOR &grid)
	{
		int x = grid.getIntX();
		int y = grid.getIntY();
		int nMax = map->nDimension - 1;

		if (x < 0 || x > nMax || y < 0 || y > nMax)
		{
			return false;
		}

		return map->IsCellValid(x, y);
	}

	virtual bool IsGridInSystem (const GRIDVECTOR & grid)
	{
		int x = grid.getIntX();
		int y = grid.getIntY();
		int nMax = map->nDimension - 1;

		if (x < 0 || x > nMax || y < 0 || y > nMax)
		{
			return false;
		}

		return map->IsCellInSystem(x, y);
	};

	virtual U32 GetFieldID (const GRIDVECTOR & grid)
	{
		// are we in any kind of field here?
		return map->GetFieldID(grid.getIntX(), grid.getIntY());
	}

	/* TerrainMap methods */

	const bool testSegmentPassible (const GRIDVECTOR & from, const GRIDVECTOR & to);
	
	const bool testSegmentPassiblePrecise (const SINGLE& xo, const SINGLE& y0, const SINGLE& x1, const SINGLE& y2, GRIDVECTOR * backup = NULL);
	
	int findPath (const GRIDVECTOR & from, const GRIDVECTOR &to, U32 dwMissionID, U32 flags, IFindPathCallback * callback);

	const bool findClosestGrid (int x, int y, U32 dwMissionID, GRIDVECTOR& rGrid);

	const bool findClosestQuarter (const SINGLE& xf, const SINGLE& yf, U32 dwMissionID, GRIDVECTOR& rGrid);

	const GRIDVECTOR findClosestGridNoBackup (GRIDVECTOR startGrid, U32 dwMissionID);

	const GRIDVECTOR findClosestQuarterNoBackup (GRIDVECTOR startGrid, U32 dwMissionID);

	const bool findOpenGridInPath (const SINGLE& _xo, const SINGLE& _yo, const SINGLE& _x1, const SINGLE& _y1, U32 dwMissionID,
								   bool bFullSquare, GRIDVECTOR * backup);

	const bool testSquare (const int x, const int y, ITerrainSegCallback * callback)
	{
		GRIDVECTOR grid;
		grid.init(x, y);
		InfoIterator<FootprintInfo> it(map->matrix[x][y]->fpInfoList);

		if (it)
		{
			FootprintInfo info;

			while (it)
			{
				info = *it.pNode;
				GRIDVECTOR tmp = grid;

				if (info.flags & TERRAIN_HALFSQUARE)
				{
					if (info.flags & TERRAIN_BOTTOMLEFT)
					{
						tmp.x |= 1;
						tmp.y |= 1;
					}
					else
					if (info.flags & TERRAIN_TOPLEFT)
					{
						tmp.x |= 1;
						tmp.y |= 3;
					}
					else
					if (info.flags & TERRAIN_TOPRIGHT)
					{
						tmp.x |= 3;
						tmp.y |= 3;
					}
					else
					if (info.flags & TERRAIN_BOTTOMRIGHT)
					{
						tmp.x |= 3;
						tmp.y |= 1;
					}
				}
				else
				{
					tmp.x |= 2;
					tmp.y |= 2;
				}

				if (callback->TerrainCallback(info, tmp) == 0)
				{
					return false;
				}

				++it;
			}
		}
		else
		{
			FootprintInfo info;
			if (U32(x) >= U32(map->nDimension) || U32(y) >= U32(map->nDimension))
				info.flags |= TERRAIN_OUTOFSYSTEM;

			if (callback->TerrainCallback(info, grid) == 0)
			{
				return false;
			}
		}
		return true;
	}

	const bool testSquarePassible (const int x, const int y)
	{
		if (map->GetFlags(x, y) & TERRAIN_IMPASSIBLE)
		{
			return false;
		}

		return true;
	}

	void updatePrefferedCorner (const GRIDVECTOR & from, const GRIDVECTOR & to)
	{
		SINGLE xDir = to.getX() - from.getX();
		SINGLE yDir = to.getY() - from.getY();
		
		if (xDir < 0.0f && yDir < 0.0f)
		{
			dwPreferredCorner = TERRAIN_BOTTOMLEFT;
			xPreferredOffset = 0.0f;
			yPreferredOffset = 0.0f;
		}
		else if (xDir < 0.0f && yDir >= 0.0f)
		{
			dwPreferredCorner = TERRAIN_TOPLEFT;
			xPreferredOffset = 0.0f;
			yPreferredOffset = 0.75f;
		}
		else if (xDir >= 0.0f && yDir >= 0.0f)
		{
			dwPreferredCorner = TERRAIN_TOPRIGHT;
			xPreferredOffset = 0.75f;
			yPreferredOffset = 0.75f;
		}
		else 
		{
			dwPreferredCorner = TERRAIN_BOTTOMRIGHT;
			xPreferredOffset = 0.75f;
			yPreferredOffset = 0.0f;
		}
	}

	const bool testSquareValidDestination (const int x, const int y, const U32 dwMissionIgnore, const bool bFullSquare, GRIDVECTOR * gridvec)
	{
		U32 dwAllFlags = map->GetFlags(x, y);

		if (dwAllFlags & (TERRAIN_IMPASSIBLE | TERRAIN_OUTOFSYSTEM))
		{
			return false;
		}

		InfoIterator<FootprintInfo> it(map->matrix[x][y]->fpInfoList);
		U32 dwFlags;
		bool bSamePlayer;
		U32 thisMissionID;
		U32 cornerFlags = 0;	// set each corner flag if it is not a valid destination
		U32 thisCorner;

		while (it)
		{
			const FootprintInfo & footprint = *it.pNode;
			
			thisMissionID = footprint.missionID;

			if (dwMissionIgnore == thisMissionID)
			{
				// ignore footprints that this missionID may have already set
				++it;
				continue;
			}

			dwFlags = footprint.flags;
			bSamePlayer = (thisMissionID & PLAYERID_MASK) == (dwMissionIgnore & PLAYERID_MASK);

			if (bFullSquare)
			{
				// we can't use this spot is any big or small ship is parked there or if any small or big
				// ship that is friendly want's it as a destination
				if (bSamePlayer && (dwFlags & TERRAIN_DESTINATION))
				{
					// an object of the same player ID is requesting this spot
					return false;
				}
				if (dwFlags & TERRAIN_PARKED)
				{
					// something else is parked here man
					return false;
				}
			}
			else
			{
				// we are a small ship, we only need a grid corner to move to

				if (dwFlags & TERRAIN_FULLSQUARE)
				{
					// a big ship has set the current footprint, is it a freindly marked destination or something parked there?
					if (dwFlags & TERRAIN_PARKED)
					{
						return false;
					}
					if (bSamePlayer && (dwFlags & TERRAIN_DESTINATION))
					{
						// a friendly big ship is requesting a destination here
						return false;
					}
				}
				else
				{
					thisCorner = (dwFlags & (TERRAIN_TOPLEFT | TERRAIN_TOPRIGHT | TERRAIN_BOTTOMLEFT | TERRAIN_BOTTOMRIGHT));
					
					if (dwFlags & TERRAIN_PARKED)
					{
						// this corner is no good
						cornerFlags |= thisCorner;
					}
					if (bSamePlayer && (dwFlags & TERRAIN_DESTINATION))
					{
						// this corner is no good
						cornerFlags |= thisCorner;
					}
				}
			}

			++it;
		}

		// if we've gotten here, we may have a good grid or grid corner to move to
		if (bFullSquare)
		{
			// set the gridvec and let's go
			gridvec->init(x, y);
			gridvec->centerpos();
		}
		else
		{
			// do we have an open corner?
			if ((cornerFlags & dwPreferredCorner) == 0)
			{
				gridvec->init(x + xPreferredOffset, y + yPreferredOffset);
				gridvec->quarterpos();
				return true;
			}

			if ((cornerFlags & TERRAIN_BOTTOMLEFT) == 0)
			{
				gridvec->init(x, y);
				gridvec->quarterpos();
			}
			else if ((cornerFlags & TERRAIN_TOPLEFT) == 0)
			{
				gridvec->init(x, y+0.75f);
				gridvec->quarterpos();
			}
			else if ((cornerFlags & TERRAIN_TOPRIGHT) == 0)
			{
				gridvec->init(x+0.75f, y+0.7f);
				gridvec->quarterpos();
			}
			else if ((cornerFlags & TERRAIN_BOTTOMRIGHT) == 0)
			{
				gridvec->init(x+0.75f, y);
				gridvec->quarterpos();
			}
			else
			{
				return false;
			}
		}

		return true;
	}
};
//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------
//
TerrainMap::TerrainMap (void)
{
}
//-------------------------------------------------------------------------------------------
//
/*void TerrainMap::ResetMap (void)
{
	// clear the map matrix
	if (map)
	{
		map->ZeroMatrix();
	}
}*/
//-------------------------------------------------------------------------------------------
//
void TerrainMap::SetWorldRect (const RECT & _worldRect)
{
	worldRect = _worldRect;

	if (map != NULL)
	{
		delete map;
	}

	U32 size = (worldRect.right - worldRect.left)/GRIDSIZE;
	map = new MapMatrix(size, this);
}
//----------------------------------------------------------------------------------------------
//
void TerrainMap::UndoFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & info)
{
	// clear the map of all footprints with the given missionID
	int i;
	for (i = 0; i < numSquares; i++)
	{
		int x = squares[i].getIntX();
		int y = squares[i].getIntY();

		S32 nMax = map->nDimension;
		if (x < 0 || x >= nMax || y < 0 || y >= nMax)
		{
			// ignore moving outside the system
			if ((info.flags & (TERRAIN_MOVING|TERRAIN_UNITROTATING)) == 0)
				CQERROR1("Unit 0x%X attempted to UNDO a footprint outside of map boundary.", info.missionID);
			continue;
		}

		map->UndoFootprint(x, y, info);
	}
}
//----------------------------------------------------------------------------------------------
//
void TerrainMap::SetFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & _info)
{
	int i;
	FootprintInfo info = _info;
	for (i = 0; i < numSquares; i++)
	{
		S32 x = squares[i].getIntX();
		S32 y = squares[i].getIntY();

		S32 nMax = map->nDimension;
		if (x < 0 || x >= nMax || y < 0 || y >= nMax)
		{
			// ignore moving outside the system
			if ((info.flags & (TERRAIN_MOVING|TERRAIN_UNITROTATING)) == 0)
				CQERROR1("Unit 0x%X attempted to Set a footprint outside of map boundary.", info.missionID);
			continue;
		}

		if (info.flags & TERRAIN_HALFSQUARE)
		{
			U32 xbit = squares[i].x & 3;
			U32 ybit = squares[i].y & 3;

			if (xbit <= 1 && ybit <= 1)
			{
				info.flags |= TERRAIN_BOTTOMLEFT;
			}
			else if (xbit <= 1 && ybit > 1)
			{
				info.flags |= TERRAIN_TOPLEFT;
			}
			else if (xbit > 1 && ybit <= 1)
			{
				info.flags |= TERRAIN_BOTTOMRIGHT;
			}
			else
			{
				info.flags |= TERRAIN_TOPRIGHT;
			}
		}
		else if (info.flags & TERRAIN_PARKED)
		{
			info.flags |= TERRAIN_FULLYTAKEN;
		}

		map->AddFootprint(x, y, info);
	}
}
//----------------------------------------------------------------------------------------------
//
const GRIDVECTOR TerrainMap::findClosestGridNoBackup (GRIDVECTOR startGrid, U32 dwMissionID)
{
	U32 i;
	int dimension = map->nDimension;

	GRIDVECTOR endGrid;

	int xnew;
	int ynew;

	for (i = 0; i < NUM_STEPS; i++)
	{
		xnew = startGrid.getIntX() + g_steps[i].x;
		ynew = startGrid.getIntY() + g_steps[i].y;

		if ((xnew < dimension && xnew >= 0) && (ynew < dimension && ynew >= 0))
		{
			U32 dwFlags = map->GetFlags(xnew, ynew);
			
			// could this coordinate meet our fancy?
			if ((dwFlags & TERRAIN_IMPASSIBLE) == 0)
			{
				// does a friendly have his destination set here?

				// do not move onto a grid that another ship of the same player designated as a destination
				if (map->IsDestinationOpen(xnew, ynew, dwMissionID))
				{
					// if there is no impassible terrain between this new position and the start-search position, then we are done
					endGrid.init(xnew, ynew);
					if (endGrid == startGrid || testSegmentPassible(startGrid, endGrid))
					{
						return endGrid;
					}
				}
			}
		}
	}

	endGrid = startGrid;
	return endGrid;
}
//----------------------------------------------------------------------------------------------
//
const bool TerrainMap::findClosestGrid (int x, int y, U32 dwMissionID, GRIDVECTOR& rGrid)
{
	const U32 NOT_HERE = TERRAIN_IMPASSIBLE;

	U32 i;
	int dimension = map->nDimension;
	int xnew, ynew;

	bool bBackup = false;
	int xbest = 0;
	int ybest = 0;

	GRIDVECTOR start;
	start.init(x, y);

	for (i = 0; i < NUM_STEPS; i++)
	{
		xnew = x + g_steps[i].x;
		ynew = y + g_steps[i].y;

		if ((xnew < dimension && xnew >= 0) && (ynew < dimension && ynew >= 0))
		{
			U32 dwFlags = map->GetFlags(xnew, ynew);
			
			// could this coordinate meet our fancy?
			if ((dwFlags & NOT_HERE) == 0)
			{
				// does a friendly have his destination set here?

				// do not move onto a grid that another ship of the same player designated as a destination
				if (map->IsDestinationOpen(xnew, ynew, dwMissionID))
				{
					// we have found a place to park
					rGrid.init(xnew, ynew);
					
					// this will be the best position if there is no terrain between it and the asked for destinatoin
					if (testSegmentPassible(start, rGrid))
					{
						rGrid.centerpos();
						return true;
					}
					else
					{
						// back up plan
						if (bBackup == false)
						{
							xbest = xnew;
							ybest = ynew;
							bBackup = true;
						}
					}
				}
			}
		}
	}

	// do we have a back-up plan?
	if (bBackup)
	{
		rGrid.init(xbest, ybest);
		rGrid.centerpos();
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
const GRIDVECTOR TerrainMap::findClosestQuarterNoBackup (GRIDVECTOR startGrid, U32 dwMissionID)
{
	U32 i;
	int dimension = map->nDimension;

	GRIDVECTOR endGrid;

	SINGLE xnew;
	SINGLE ynew;

	// is the quarter spot we've been asked to move to okay?
	U32 dwCorner = pos_to_flag(startGrid.getX(), startGrid.getY());

	for (i = 0; i < NUM_STEPS; i++)
	{
		xnew = startGrid.getX() + 0.5f * g_steps[i].x;
		ynew = startGrid.getY() + 0.5f * g_steps[i].y;

		if ((xnew < dimension && xnew >= 0) && (ynew < dimension && ynew >= 0))
		{
			// could this coordinate meet our fancy?
			U32 flags = map->GetFlags(xnew, ynew);

			// an imapassible flag counts for the whole damn grid
			if (flags & (TERRAIN_IMPASSIBLE | TERRAIN_FULLYTAKEN))
			{
				continue;
			}
				
			dwCorner = pos_to_flag(xnew, ynew);
			if (map->IsCornerDestinationOpen(xnew, ynew, dwMissionID, dwCorner))
			{
				// if there is no impassible terrain between this new position and the start-search position, then we are done
				endGrid.init(xnew, ynew);
				if (endGrid == startGrid || testSegmentPassible(startGrid, endGrid))
				{
					return endGrid;
				}
			}
		}
	}

	endGrid = startGrid;
	return endGrid;
}
//----------------------------------------------------------------------------------------------
//
const bool TerrainMap::findClosestQuarter (const SINGLE& xf, const SINGLE& yf, U32 dwMissionID, GRIDVECTOR& rGrid)
{
	U32 i;
	int dimension = map->nDimension;
	SINGLE xnew;
	SINGLE ynew;

	bool   bBackup = false;
	SINGLE xbest = 0;
	SINGLE ybest = 0;

	GRIDVECTOR start;
	start.init(xf, yf);
	start.quarterpos();


	// is the quarter spot we've been asked to move to okay?
	U32 dwCorner = pos_to_flag(xf, yf);

	for (i = 0; i < NUM_STEPS; i++)
	{
		xnew = xf + 0.5f*g_steps[i].x;
		ynew = yf + 0.5f*g_steps[i].y;

		if ((xnew < dimension && xnew >= 0) && (ynew < dimension && ynew >= 0))
		{
			// could this coordinate meet our fancy?
			U32 flags = map->GetFlags(xnew, ynew);

			// an imapassible flag counts for the whole damn grid
			if (flags & (TERRAIN_IMPASSIBLE | TERRAIN_FULLYTAKEN))
			{
				continue;
			}
				
			dwCorner = pos_to_flag(xnew, ynew);
			if (map->IsCornerDestinationOpen(xnew, ynew, dwMissionID, dwCorner))
			{
				// if there is no impassible terrain between this new position and the start-search position, then we are done
				rGrid.init(xnew, ynew);
				if (rGrid == start || testSegmentPassible(start, rGrid))
				{
					rGrid.quarterpos();
					return true;
				}
				else
				{
					// there is a terrain feature between the start and dest, but at least we have a back-up plan
					if (bBackup == false)
					{
						xbest = xnew;
						ybest = ynew;
						bBackup = true;
					}
				}
			}
		}
	}

	// if we've gotten here, we may still have a back-up plan
	if (bBackup)
	{
		rGrid.init(xbest, ybest);
		rGrid.quarterpos();
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
int TerrainMap::findPath (const GRIDVECTOR & from, const GRIDVECTOR &to, U32 dwMissionID, U32 flags, IFindPathCallback * callback)
{
	static GRIDVECTOR grid_stack[256];

	// can we go directly start to end?
	// ie. is there an impassible object between from and to?
	GRIDVECTOR backup;
	SINGLE xo = from.getX();
	SINGLE yo = from.getY();
	SINGLE x1 = to.getX();
	SINGLE y1 = to.getY();

	if (testSegmentPassiblePrecise(xo, yo, x1, y1, &backup))
	{
		callback->SetPath(this, &to, 1);
		return 1;
	}

	// we have to find a viable path now
	int numpath  = 0;
	if(map->AStarPath(from, to, grid_stack, numpath))
	{
		for (int i = 1; i < numpath-1; i++)
		{
			grid_stack[i].centerpos();
		}

		grid_stack[0] = to;
		callback->SetPath(this, grid_stack, numpath-1);
		return numpath;
	}

	// there is no path to where we are going but we should at least move towards our intended target
	if (flags & TERRAIN_FP_FULLSQUARE)
	{
		// if the sqaure is already occupied than you have to find a sqaure that is open
		backup.centerpos();

		GRIDVECTOR gridClose = findClosestGridNoBackup(backup, dwMissionID);
		gridClose.centerpos();
		
		if (gridClose == from)
		{
			// if we are not parked at this position (ie. on the move), then we need to set a path length of 1
			if (map->GetMissionFlags(from.getIntX(), from.getIntY(), dwMissionID) & TERRAIN_PARKED)
			{
				// do not move
				return 0;
			}
			else
			{
				gridClose.centerpos();
				callback->SetPath(this, &gridClose, 1);
				return 1;
			}
		}

		gridClose.centerpos();
		callback->SetPath(this, &gridClose, 1);
		return 1;
	}
	else
	{
		// if the quarter position is already occuplied than you have to find a quarter square that is open
		backup.quarterpos();

		GRIDVECTOR gridClose = findClosestQuarterNoBackup(backup, dwMissionID);
		gridClose.quarterpos();
		
		if (gridClose == from)
		{
			// if we are not parked at this position (ie. on the move), then we need to set a path length of 1
			if (map->GetMissionFlags(from.getIntX(), from.getIntY(), dwMissionID, pos_to_flag(from.getX(), from.getY())) & TERRAIN_PARKED)
			{
				// we are already parked here, don't move
				return 0;
			}
			else
			{
				// we are on the move, so even though we are over the spot we are going to park at we still need to need to set the path
				gridClose.quarterpos();
				callback->SetPath(this, &gridClose, 1);
				return 1;
			}
		}

		gridClose.quarterpos();
		callback->SetPath(this, &gridClose, 1);
		return 1;
	}

	// if we are here, we've found a second-rate path with only one grid
	callback->SetPath(this, &backup, 1);
	return 1;
}
//----------------------------------------------------------------------------------------------
//
int TerrainMap::FindPath (const GRIDVECTOR & from, const GRIDVECTOR & _to, U32 dwMissionID, U32 flags, IFindPathCallback * callback) 
{
	GRIDVECTOR to=_to;
	S32 nMax = map->nDimension;
	SINGLE fMax = nMax-0.5;
	SINGLE testX, testY;

	testX = to.getX();
	testY = to.getY();

	if (F2LONG(testX) >= nMax)
		testX = fMax;
	if (F2LONG(testY) >= nMax)
		testY = fMax;
	to.init(testX, testY);

	S32 xto =	to.getIntX();
	S32 yto =	to.getIntY();
	SINGLE xf = to.getX();
	SINGLE yf = to.getY();
	BOOL32 fullsquare = flags & TERRAIN_FP_FULLSQUARE;
	
	// find a general path, ignore any terrain sqaures marked as parked or impassible
	GRIDVECTOR newto = to;

	if (fullsquare)
	{
		if (findClosestGrid(xto, yto, dwMissionID, newto) == false)
		{
			// this should hardly happen, but we couldn't find an open grid close to where we wanted to go
			// we now opt to move to any square that is atleast in the direction of where we are going 
			if (findOpenGridInPath(from.getIntX(), from.getIntY(), to.getIntX(), to.getIntY(), dwMissionID, true, &newto) == false)
			{
				// couldn't find a path no matter what
				return 0;
			}
		}

		// is this position the same as our current position, and are we already parked there?
		if (newto.isMostlyEqual(from))
		{
			// check if this mission ID is already parked there
			if (map->GetMissionFlags(newto.getIntX(), newto.getIntY(), dwMissionID) & TERRAIN_PARKED)
			{
				// stay put, return a path of zero
				return 0;
			}
		}

		return findPath(from, newto, dwMissionID, TERRAIN_FP_FULLSQUARE, callback);
	}
	else
	{
		// the object looking for a path is small enough to take up a quarter square

		// do not end path on a friendly destination
		// do not set path on any parked or immpassible object
		if (findClosestQuarter(xf, yf, dwMissionID, newto) == false)
		{
			// couldn't find a corner to move into
			// this should hardly happen, but we couldn't find an open grid-corner close to where we wanted to go
			// we now opt to move to any grid-corner that is atleast in the direction of where we are going 
			updatePrefferedCorner(from, to);
			if (findOpenGridInPath(from.getIntX(), from.getIntY(), to.getIntX(), to.getIntY(), dwMissionID, false, &newto) == false)
			{
				// couldn't find a path no matter what
				return 0;
			}
		}

		// is this position the same as our current position, and are we already parked there?
		if (newto == from)
		{
			// check if this mission ID is already parked there
			if (map->GetMissionFlags(newto.getIntX(), newto.getIntY(), dwMissionID, pos_to_flag(from.getX(), from.getY())) & TERRAIN_PARKED)
			{
				// stay put, return a path of zero
				return 0;
			}
		}

		// do not path find if grid location is the same
		return findPath(from, newto, dwMissionID, TERRAIN_FP_HALFSQUARE, callback);
	}

	return 0;
}
//----------------------------------------------------------------------------------------------
//
bool TerrainMap::TestSegment (const GRIDVECTOR & from, const GRIDVECTOR & to, ITerrainSegCallback * callback)
{
	int xo = (int)from.getIntX();
	int yo = (int)from.getIntY();
	int x1 = (int)to.getIntX();
	int y1 = (int)to.getIntY();
	S32 nMax = map->nDimension;

	if (xo >= nMax)
		xo = nMax-1;
	if (yo >= nMax)
		yo = nMax-1;
	if (x1 >= nMax)
		x1 = nMax-1;
	if (y1 >= nMax)
		y1 = nMax-1;

	int dx, dy;
	int xInc = 0;
	int	yInc = 0;
	int error = 0;        // the discriminant i.e. error i.e. decision variable
    int	i;          

	int x = xo, y = yo;

	// compute horizontal and vertical deltas
	dx = x1 - xo;
	dy = y1 - yo;

	// test which direction the line is going in i.e. slope angle
	if (dx >= 0)
	{
		xInc = 1;
	}
	else
	{
		xInc = -1;
		dx    = -dx;  // need absolute value
	}

	// test y component of slope
	if (dy >= 0)
	{
		yInc = 1;
	}
	else
	{
		yInc = -1;
		dy    = -dy;  // need absolute value
	}

	// now based on which delta is greater we can draw the line
	if (dx > dy)
	{
		// test along the line
		for (i = 0; i <= dx; i++)
		{
		     // adjust the error term
			error += dy;

			if (testSquare(x, y, callback) == false)
			{
				return false; 
			}

			// test if error has overflowed
			if (error > dx)
			{
				error -= dx;
				y += yInc;
			}
			x += xInc;
       }
	}
	else
	{
		// draw the line
		for (i = 0; i <= dy; i++)
		{
			// adjust the error term
			error += dx;

			if (testSquare(x, y, callback) == false)
			{
				return false;
			}

			// test if error overflowed
			if (error>0)
			{
				error-=dy;
				x += xInc;
			}
			y += yInc;
		}
	}
	return true;
}
//----------------------------------------------------------------------------------------------
//
const bool TerrainMap::testSegmentPassible (const GRIDVECTOR & from, const GRIDVECTOR&  to)
{
	int xo = (int)from.getX();
	int yo = (int)from.getY();
	int x1 = (int)to.getX();
	int y1 = (int)to.getY();

	int dx, dy;
	int xInc = 0;
	int yInc = 0;
	int error = 0;        // the discriminant i.e. error i.e. decision variable
    int	i;          

	int x = xo, y = yo;

	// compute horizontal and vertical deltas
	dx = x1 - xo;
	dy = y1 - yo;

	// test which direction the line is going in i.e. slope angle
	if (dx >= 0)
	{
		xInc = 1;
	}
	else
	{
		xInc = -1;
		dx   = -dx;  // need absolute value
	}

	// test y component of slope
	if (dy >= 0)
	{
		yInc = 1;
	}
	else
	{
		yInc = -1;
		dy   = -dy;  // need absolute value
	}

	// now based on which delta is greater we can draw the line
	if (dx > dy)
	{
		// test along the line
		for (i = 0; i <= dx; i++)
		{
		     // adjust the error term
			error += dy;

			if (testSquarePassible(x, y) == false)
			{
				// cannot pass through
				return false;
			}

			// test if error has overflowed
			if (error > dx)
			{
				error -= dx;
				y += yInc;
			}
			x += xInc;
       }
	}
	else
	{
		// draw the line
		for (i = 0; i <= dy; i++)
		{
			// adjust the error term
			error += dx;

			if (testSquarePassible(x, y) == false)
			{
				return false;
			}

			// test if error overflowed
			if (error>0)
			{
				error-=dy;
				x += xInc;
			}
			y += yInc;
		}
	}
	return true;
}
//----------------------------------------------------------------------------------------------
//
const bool TerrainMap::testSegmentPassiblePrecise (const SINGLE& _xo, const SINGLE& _yo, const SINGLE& _x1, const SINGLE& _y1, GRIDVECTOR * backup)
{
	// the resolution of the line segment
	const SINGLE res = 16;

	SINGLE xo = _xo * res;
	SINGLE yo = _yo * res;
	SINGLE x1 = _x1 * res;
	SINGLE y1 = _y1 * res;

	int oldx = -1, oldy = -1;
	int newx, newy;

	int dx, dy;
	int xInc = 0;
	int yInc = 0;
	int error = 0;        // the discriminant i.e. error i.e. decision variable
    int	i;          

	SINGLE x = xo, y = yo;

	// compute horizontal and vertical deltas
	dx = x1 - xo;
	dy = y1 - yo;

	// initialize the backup
	if (backup)
	{
		backup->init(xo/res, yo/res);
	}

	// test which direction the line is going in i.e. slope angle
	if (dx >= 0)
	{
		xInc = 1;
	}
	else
	{
		xInc = -1;
		dx   = -dx;  // need absolute value
	}

	// test y component of slope
	if (dy >= 0)
	{
		yInc = 1;
	}
	else
	{
		yInc = -1;
		dy   = -dy;  // need absolute value
	}

	// now based on which delta is greater we can draw the line
	if (dx > dy)
	{
		// test along the line
		for (i = 0; i <= dx; i++)
		{
		     // adjust the error term
			error += dy;

			newx = x/res;
			newy = y/res;
			if (newx != oldx || newy != oldy)
			{
				if (testSquarePassible(newx, newy) == false)
				{
					// cannot pass through
					return false;
				}
			}
			if (backup)
			{
				backup->init(x/res, y/res);
			}
			oldx = newx;
			oldy = newy;

			// test if error has overflowed
			if (error > dx)
			{
				error -= dx;
				y += yInc;
			}
			x += xInc;
       }
	}
	else
	{
		// draw the line
		for (i = 0; i <= dy; i++)
		{
			// adjust the error term
			error += dx;

			newx = x/res;
			newy = y/res;
			if (newx != oldx || newy != oldy)
			{
				if (testSquarePassible(newx, newy) == false)
				{
					return false;
				}
			}
			if (backup)
			{
				backup->init(x/res, y/res);
			}
			oldx = newx;
			oldy = newy;

			// test if error overflowed
			if (error>0)
			{
				error-=dy;
				x += xInc;
			}
			y += yInc;
		}
	}
	return true;
}
//----------------------------------------------------------------------------------------------
//
const bool TerrainMap::findOpenGridInPath (const SINGLE& _xo, const SINGLE& _yo, const SINGLE& _x1, const SINGLE& _y1, U32 dwMissionID,
										   bool bFullSquare, GRIDVECTOR * backup)
{
	// the resolution of the line segment
	const SINGLE res = 16;

	SINGLE xo = _xo * res;
	SINGLE yo = _yo * res;
	SINGLE x1 = _x1 * res;
	SINGLE y1 = _y1 * res;

	int oldx = -1, oldy = -1;
	int newx, newy;

	int dx, dy;
	int xInc = 0;
	int yInc = 0;
	int error = 0;        // the discriminant i.e. error i.e. decision variable
    int	i;          

	SINGLE x = xo, y = yo;

	// compute horizontal and vertical deltas
	dx = x1 - xo;
	dy = y1 - yo;

	bool bFoundBackup = false;

	// test which direction the line is going in i.e. slope angle
	if (dx >= 0)
	{
		xInc = 1;
	}
	else
	{
		xInc = -1;
		dx   = -dx;  // need absolute value
	}

	// test y component of slope
	if (dy >= 0)
	{
		yInc = 1;
	}
	else
	{
		yInc = -1;
		dy   = -dy;  // need absolute value
	}

	// now based on which delta is greater we can draw the line
	if (dx > dy)
	{
		// test along the line
		for (i = 0; i <= dx; i++)
		{
		     // adjust the error term
			error += dy;

			newx = x/res;
			newy = y/res;
			if (newx != oldx || newy != oldy)
			{
				if (testSquareValidDestination(newx, newy, dwMissionID, bFullSquare, backup) == false)
				{
					// cannot pass through
					goto Done;
				}
				else
				{
					bFoundBackup = true;
				}
			}
			oldx = newx;
			oldy = newy;

			// test if error has overflowed
			if (error > dx)
			{
				error -= dx;
				y += yInc;
			}
			x += xInc;
       }
	}
	else
	{
		// draw the line
		for (i = 0; i <= dy; i++)
		{
			// adjust the error term
			error += dx;

			newx = x/res;
			newy = y/res;
			if (newx != oldx || newy != oldy)
			{
				if (testSquareValidDestination(newx, newy, dwMissionID, bFullSquare, backup) == false)
				{
					// cannot pass through
					goto Done;
				}
				else
				{
					bFoundBackup = true;
				}
			}
			oldx = newx;
			oldy = newy;

			// test if error overflowed
			if (error>0)
			{
				error-=dy;
				x += xInc;
			}
			y += yInc;
		}
	}

Done:
	if (bFoundBackup)
	{
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
void TerrainMap::RenderEdit (void)
{
	// draw a translucent square for every footprint that shows up on the map
	map->RenderFootprints();
}
//----------------------------------------------------------------------------------------------
//
bool __stdcall CreateTerrainMap (ITerrainMap ** map)
{
	*map = new DAComponent<TerrainMap>;
	return (*map != 0);
}
//----------------------------------------------------------------------------------------------
//
struct DummyTerrainMap : ITerrainMap
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(TerrainMap)
	DACOM_INTERFACE_ENTRY(ITerrainMap)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	DummyTerrainMap (void) { }

	/* ITerrainMap methods */

	virtual void SetWorldRect (const RECT & worldRect)
	{
	}

	virtual void SetFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & info) {} 

	virtual void  UndoFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & _info) {}

	
	virtual bool TestSegment (const GRIDVECTOR & from, const GRIDVECTOR & to, ITerrainSegCallback * callback) 
	{
		return true;
	}

	virtual int FindPath (const GRIDVECTOR & from, const GRIDVECTOR & to, U32 dwMissionID, U32 flags, IFindPathCallback * callback) {return 0;}

	virtual void RenderEdit (void) {};

	virtual bool IsGridEmpty (const GRIDVECTOR & grid, U32 dwIgnoreID, bool bFullSquare)
	{
		return true;
	}

	virtual bool IsGridValid (const GRIDVECTOR & grid)
	{
		return true;
	}

	virtual bool IsParkedAtGrid (const GRIDVECTOR & grid, U32 dwMissionID,  bool bFullSquare)
	{
		return true;
	}

	virtual bool IsGridInSystem(const GRIDVECTOR & grid)
	{
		return true;
	}

	virtual U32 GetFieldID (const GRIDVECTOR & grid)
	{
		return 0;
	}
	virtual bool IsOkForBuilding (const GRIDVECTOR & grid, bool bFullSquare, bool checkParkedUnits)
	{
		return true;
	}

};
//----------------------------------------------------------------------------------------------
//
bool __stdcall CreateDummyTerrainMap (ITerrainMap ** map)
{
	*map = new DAComponent<DummyTerrainMap>;
	return (*map != 0);
}
//----------------------------------------------------------------------------------------------
//
bool MapMatrix::AStarPath (const GRIDVECTOR &src, const GRIDVECTOR &dst, GRIDVECTOR * gridpath, int& numgrids)
{
	QOpen.flush(openID);
	QClosed.flush(closedID);

	openID += nMaps*2;
	closedID += nMaps*2;

	// get a pointer to the node given by src
	CellRef srcCell(src.getX(), src.getY());
	CellRef dstCell(dst.getX(), dst.getY());

	PathNode * newnode = NULL;
	
	PathNode * node = GetNode(srcCell);
	node->g = 0;
	node->h = GetHeuristic(srcCell, dstCell);
	node->f = node->g + node->h;
	node->parentNode = NULL;

	QOpen.push(node);

	// this is the body of the search
	while (QOpen.empty() == false)
	{
		// take the best node off the list
		node = QOpen.pop();

		if (node->currentCell == dstCell)
		{
			// hurray!  we've found our path, constuct the path by following the parent pointers
			numgrids = 0;
			PathNode* pnode = node;

			while (pnode)
			{
				gridpath[numgrids].init(pnode->currentCell.x, pnode->currentCell.y); 
				gridpath[numgrids].centerpos(); 
				numgrids++;
				pnode = pnode->parentNode;
			}
			gridpath[0] = dst;
			gridpath[numgrids-1] = src;

			// clean up the path with another run at a special astar function
			if (numgrids > 4)
			{
				AStarPathClense(gridpath, numgrids);
			}

			return true;
		}

		// find out what other nodes can be put onto the Open and Closed priority queues
		// each node can be attached to at most 8 other nodes
		U8 infty_bits = 0;
		for (int i = 0; i < 8; i++)
		{
			newnode = GetNode(node->currentCell + g_cellDirections[i]);
			
			// is this newnode valid?
			if (newnode == NULL)
			{
				infty_bits |= 1 << i;
				continue;
			}

			SINGLE newcost;
			if (node->currentCell == srcCell)
			{
				newcost = node->g + GetCellCostIgnoreImpassible(i)/2.0f + GetCellCost(newnode->currentCell, i)/2.0f;
			}
			else
			{
				newcost = node->g + GetCellCost(node->currentCell, i)/2.0f + GetCellCost(newnode->currentCell, i)/2.0f;
			}

			// if we have infinity, then we don't want to include it now do we?
			if (newcost >= INFTY_LITE)
			{
				infty_bits |= 1 << i;
				continue;
			}

			// if we are looking at a spuare on the directional, then do a quick check to see if the kitty-corner squares are impassible
			if (i >= 4)
			{
				U8 test_bit1 = 1 << (i-4); 
				U8 test_bit2 = 1 << (i-3)%4; 

				// if both bits were set then the diagonal is impassible
				if ((infty_bits & test_bit1) && (infty_bits & test_bit2))
				{
					continue;
				}
			}

			if ((QOpen.contains(newnode) || QClosed.contains(newnode)) && newnode->g <= newcost)
				continue;

			newnode->parentNode = node;
			newnode->g = newcost;
			newnode->h = GetHeuristic(newnode->currentCell, dstCell);
			newnode->f = newnode->g + newnode->h;

			// if newnode is on PClosed then remove it
			if (QClosed.contains(newnode))
			{
				QClosed.remove(newnode);
			}

			// if newnode is not yet in POpen, then push it on
			if (QOpen.contains(newnode) == false)
			{
				QOpen.push(newnode);
			}
		}
		
		// push node onto closed
		QClosed.push(node);
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MapMatrix::AStarPathClense (GRIDVECTOR * gridpath, int& numgrids)
{
	QOpen.flush(openID);
	QClosed.flush(closedID);

	openID += nMaps*2;
	closedID += nMaps*2;

	// get a pointer to the node given by src
	CellRef srcCell(gridpath[numgrids-1].getX(), gridpath[numgrids-1].getY());
	CellRef dstCell(gridpath[0].getX(), gridpath[0].getY());

	PathNode* newnode = NULL;
	
	PathNode* node = GetNode(srcCell);
	node->g = 0;
	node->h = GetDistance(srcCell, dstCell);
	node->f = node->g + node->h;
	node->parentNode = NULL;

	QOpen.push(node);

	// this is the body of the search
	while (QOpen.empty() == false)
	{
		// take the best node off the list 
		node = QOpen.pop();

		if (node->currentCell == dstCell)
		{
			// hurray!  we've found a cleaner path, we hope so anyway
			numgrids = 0;
			PathNode* pnode = node;

			while (pnode)
			{
				gridpath[numgrids].init(pnode->currentCell.x, pnode->currentCell.y);
				numgrids++;
				pnode = pnode->parentNode;
			}
			return true;
		}

		// find the index of the current gridvector
		int index = 0;
		CellRef cell;
		for (int j = 0; j < numgrids; j++)
		{
			CellRef cr(gridpath[j].getX(), gridpath[j].getY());
			if (cr == node->currentCell)
			{
				index = j;
				cell = cr;
				break;
			}
		}

		int next = index+1;
		int prev = index-1;

		// find out what other nodes can be put onto the Open and Closed priority queues
		// each node may be connected to at most numgrid-1 other nodes
		for (int i = 0; i < numgrids; i++)
		{
			// is the new cell the same as the current?
			CellRef newcell(gridpath[i].getX(), gridpath[i].getY());

			if (node->currentCell == newcell)
			{
				continue;
			}

			newnode = GetNode(newcell);

			// can the node and newnode be connected?
			if ((i != next && i != prev) && terrainMap->testSegmentPassiblePrecise(gridpath[index].getX(), gridpath[index].getY(), gridpath[i].getX(), gridpath[i].getY()) == false)
			{
				continue;
			}

			// the newnode is connected to the node, carry on...
			SINGLE newcost = node->g + GetDistance(cell, newcell);

			if ((QOpen.contains(newnode) || QClosed.contains(newnode)) && newnode->g <= newcost)
				continue;

			newnode->parentNode = node;
			newnode->g = newcost;
			newnode->h = GetDistance(newnode->currentCell, dstCell);
			newnode->f = newnode->g + newnode->h;

			// if newnode is on PClosed then remove it
			if (QClosed.contains(newnode))
			{
				QClosed.remove(newnode);
			}

			// if newnode is not yet in POpen, then push it on
			if (QOpen.contains(newnode) == false)
			{
				QOpen.push(newnode);
			}
		}
		
		// push node onto closed
		QClosed.push(node);
	}

	// a path could not be found
	return false;
}

//-------------------------------------------------------------------------------------------------
//----------------------END TerrainMap.cpp---------------------------------------------------------
//-------------------------------------------------------------------------------------------------