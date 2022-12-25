//PreviewWin.cpp

#include "stdafx.h"
#include "globals.h"
#include <DACOM.h>
#include "resource.h"
#include "MathLayoutWin.h"
#include "IEffectFile.h"
#include "IEffectTarget.h"
#include "ITargetHP.h"
#include <IParticleManager.h>

#include <HeapObj.h>
#include <FileSys.h>
#include <TSmartPointer.h>

#include <stdlib.h>
#include <commdlg.h>

void drawRect(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2, HBRUSH type);
void drawLine(HDC hdc, S32 x1, S32 y1, S32 x2, S32 y2);

struct MathNode
{
	MathNode * next;
	MathNode * orphanNext;

	FloatType * floatType;
	TransformType * transType;
	RampKey * rampKey;

	bool bMarked;//mark testing for graph inclusion

	S32 xPos;
	S32 yPos;
	S32 width;
	S32 height;

	MathNode()
	{
		next = NULL;
		orphanNext = NULL;
		floatType = NULL;
		transType = NULL;
		rampKey = NULL;

		bMarked = false;

		xPos = 0;
		yPos = 0;
		width = 0;
		height = 0;
	}
};

#define MATH_AREA_SIZE 2000
#define MF_ZOOM_SPEED 0.001f
#define MF_ZOOM_MIN 1.0
#define MF_ZOOM_MAX 4.0

struct MathEditInfo
{
	FloatType * inputFloat;
	TransformType * inputTrans;

	bool bFloatInput;
	bool bTransInput;

	MathNode * mathList; //complete list of nodes
	MathNode * orphanList;//list of root nodes of orphaned trees

	MathNode * selectedMath;

	bool bAreaInvalid;
	HDC mathDC;
	HBITMAP mathBitmap;
	S32 sx;
	S32 sy;
	SINGLE zoomLevel;

	bool bMoving;
	bool bPanning;
	bool bConnecting;
	S32 mouseX;
	S32 mouseY;
	S32 orgX;
	S32 orgY;

	RECT neededArea;
	RECT viewArea;

	HBRUSH rootBrush;
	HBRUSH rootHighBoarderBrush;
	HBRUSH rootLowBoarderBrush;

	HBRUSH floatBrush;
	HBRUSH floatHighBoarderBrush;
	HBRUSH floatLowBoarderBrush;

	HBRUSH rampBrush;
	HBRUSH rampHighBoarderBrush;
	HBRUSH rampLowBoarderBrush;

	HBRUSH transBrush;
	HBRUSH transHighBoarderBrush;
	HBRUSH transLowBoarderBrush;

	HFONT font;

	S32 rootX;
	S32 rootY;

	//used for connecting nodes
	MathNode * connectNode;
	FloatType ** connectFloat;
	TransformType ** connectTrans;
	bool connectKey;

	MathEditInfo()
	{
		inputFloat = NULL;
		inputTrans = NULL;
		bFloatInput = false;
		bTransInput= false;


		mathList= NULL;
		selectedMath = NULL;
		orphanList = NULL;

		bAreaInvalid = true;
		mathDC = NULL;
		mathBitmap = NULL;
		sx = 0;
		sy = 0;
		zoomLevel = 1.0;

		rootX = 0;
		rootY = 0;

		bMoving = false;
		bPanning = false;
		bConnecting = false;
		mouseX = 0;
		mouseY = 0;
	}
};

struct FloatEditInfo
{
	FloatType * floatType;
	MathEditInfo * info;
};

struct TransEditInfo
{
	TransformType * transType;
	MathEditInfo * info;
};

FloatType * editFloatType(MathEditInfo * info,FloatType * floatType);
TransformType * editTransType(MathEditInfo * info,TransformType * transType);
void editRampKey(RampKey * key);

void createFloatMathNode(FloatType * floatType,MathEditInfo * info);

#define MF_STANDARD_WIDTH 100
#define MF_STANDARD_SPACING 20
#define FONT_BUFFER 3
#define MF_LINE_HEIGHT 20
#define MF_CIRC_WIDTH 10
#define MF_CIRC_WIDTH_HALF (MF_CIRC_WIDTH >> 1)
#define MF_FONT_POINT_SIZE 9

void computeNodeSize(MathNode * node)
{
	node->width = MF_STANDARD_WIDTH;
	if(node->rampKey)
	{
		node->height = MF_LINE_HEIGHT;
	}
	else if(node->floatType)
	{
		switch(node->floatType->type)
		{
		case FloatType::CONSTANT:
		case FloatType::PARAMETER:
			node->height = MF_LINE_HEIGHT;
			break;
		case FloatType::RANGE:
		case FloatType::CONST_RANGE:
		case FloatType::ADD:
		case FloatType::SUBTRACT:
		case FloatType::MULTIPLY:
		case FloatType::DIVIDE:
			{
				node->height = MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT;
			}
			break;
		case FloatType::RAMP:
		case FloatType::LOOP_RAMP:
		case FloatType::OSCILATE_RAMP:
			{
				node->height = MF_LINE_HEIGHT+MF_LINE_HEIGHT;
				RampKey * search = node->floatType->ramp.firstKey;
				while(search)
				{
					node->height += MF_LINE_HEIGHT;
					search = search->next;
				}
			}
			break;
		}		
	}
	else if(node->transType)
	{
		switch(node->transType->type)
		{
		case TransformType::UP:
		case TransformType::CAMERA:
		case TransformType::CAMERA_LOOK:
		case TransformType::INPUT_POINT:
		case TransformType::FILTER_EFFECT:
			node->height = MF_LINE_HEIGHT;
			break;
		case TransformType::TARGET_TRANSFORM:
		case TransformType::TARGET_TRANSFORM_STR:
			node->height = MF_LINE_HEIGHT+MF_LINE_HEIGHT;
			break;
		case TransformType::OFFSET:
		case TransformType::OFFSET_IJK:
		case TransformType::ROTATE_IJK:
			node->height = MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT;
			break;
		case TransformType::LOOK_AT_UP:
			node->height = MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT;
			break;
		}
	}
};

void createRampKeyMathNode(RampKey * rampKey,MathEditInfo * info)
{
	MathNode * node = new MathNode;
	node->next = info->mathList;
	info->mathList = node;
	node->rampKey = rampKey;

	computeNodeSize(node);

	if(rampKey->value)
		createFloatMathNode(rampKey->value,info);
}

void createFloatMathNode(FloatType * floatType,MathEditInfo * info)
{
	MathNode * node = new MathNode;
	node->next = info->mathList;
	info->mathList = node;
	node->floatType = floatType;

	computeNodeSize(node);

	switch(floatType->type)
	{
	case FloatType::CONSTANT:
	case FloatType::PARAMETER:
		break;
	case FloatType::RANGE:
	case FloatType::CONST_RANGE:
		{
			if(floatType->range.min)
				createFloatMathNode(floatType->range.min,info);
			if(floatType->range.max)
				createFloatMathNode(floatType->range.max,info);
		}
		break;
	case FloatType::RAMP:
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
		{
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				createRampKeyMathNode(search,info);
				search = search->next;
			}
		}
		break;
	case FloatType::ADD:
		{
			if(floatType->add.value1)
				createFloatMathNode(floatType->add.value1,info);
			if(floatType->add.value2)
				createFloatMathNode(floatType->add.value2,info);
		}
		break;
	case FloatType::SUBTRACT:
		{
			if(floatType->subtract.value1)
				createFloatMathNode(floatType->subtract.value1,info);
			if(floatType->subtract.value2)
				createFloatMathNode(floatType->subtract.value2,info);
		}
		break;
	case FloatType::MULTIPLY:
		{
			if(floatType->multiply.value1)
				createFloatMathNode(floatType->multiply.value1,info);
			if(floatType->multiply.value2)
				createFloatMathNode(floatType->multiply.value2,info);
		}
		break;
	case FloatType::DIVIDE:
		{
			if(floatType->divide.value1)
				createFloatMathNode(floatType->divide.value1,info);
			if(floatType->divide.value2)
				createFloatMathNode(floatType->divide.value2,info);
		}
		break;
	}
}

void createTransMathNode(TransformType * transType,MathEditInfo * info)
{
	MathNode * node = new MathNode;
	node->next = info->mathList;
	info->mathList = node;
	node->transType = transType;

	computeNodeSize(node);

	switch(transType->type)
	{
	case TransformType::TARGET_TRANSFORM:
	case TransformType::FILTER_EFFECT:
	case TransformType::UP:
	case TransformType::INPUT_POINT:
	case TransformType::TARGET_TRANSFORM_STR:
	case TransformType::CAMERA:
	case TransformType::CAMERA_LOOK:
		break;
	case TransformType::OFFSET:
		{
			if(transType->offset.offX)
				createFloatMathNode(transType->offset.offX,info);
			if(transType->offset.offY)
				createFloatMathNode(transType->offset.offY,info);
			if(transType->offset.offZ)
				createFloatMathNode(transType->offset.offZ,info);
			if(transType->offset.baseTrans)
				createTransMathNode(transType->offset.baseTrans,info);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			if(transType->offsetIJK.offI)
				createFloatMathNode(transType->offsetIJK.offI,info);
			if(transType->offsetIJK.offJ)
				createFloatMathNode(transType->offsetIJK.offJ,info);
			if(transType->offsetIJK.offK)
				createFloatMathNode(transType->offsetIJK.offK,info);
			if(transType->offsetIJK.baseTrans)
				createTransMathNode(transType->offsetIJK.baseTrans,info);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			if(transType->rotateIJK.rotI)
				createFloatMathNode(transType->rotateIJK.rotI,info);
			if(transType->rotateIJK.rotJ)
				createFloatMathNode(transType->rotateIJK.rotJ,info);
			if(transType->rotateIJK.rotK)
				createFloatMathNode(transType->rotateIJK.rotK,info);
			if(transType->rotateIJK.baseTrans)
				createTransMathNode(transType->rotateIJK.baseTrans,info);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			if(transType->lookAtUp.look)
				createTransMathNode(transType->lookAtUp.look,info);
			if(transType->lookAtUp.at)
				createTransMathNode(transType->lookAtUp.at,info);
			if(transType->lookAtUp.up)
				createTransMathNode(transType->lookAtUp.up,info);
		}
		break;
	}
}

void fillOutMathlist(MathEditInfo * info)
{
	if(info->inputFloat)
	{
		createFloatMathNode(info->inputFloat,info);
	}
	else if(info->inputTrans)
	{
		createTransMathNode(info->inputTrans,info);
	}
}

MathNode * findMathNode(MathNode * root,FloatType * floatType)
{
	MathNode * search = root;
	while(search)
	{
		if(search->floatType == floatType)
			return search;
		search = search->next;
	}
	return NULL;
}

MathNode * findMathNode(MathNode * root,TransformType * transType)
{
	MathNode * search = root;
	while(search)
	{
		if(search->transType == transType)
			return search;
		search = search->next;
	}
	return NULL;
}

MathNode * findMathNode(MathNode * root,RampKey * rampKey)
{
	MathNode * search = root;
	while(search)
	{
		if(search->rampKey == rampKey)
			return search;
		search = search->next;
	}
	return NULL;
}

S32 layoutMathlist(MathNode * root, MathNode * node, S32 levelHeight)
{
	S32 returnHeight = node->height+MF_STANDARD_SPACING;
	node->xPos = MF_STANDARD_WIDTH+MF_STANDARD_SPACING;
	node->yPos = levelHeight;

	S32 currentHeight = 0;
	if(node->floatType)
	{
		switch(node->floatType->type)
		{
		case FloatType::CONSTANT:
		case FloatType::PARAMETER:
			break;
		case FloatType::RANGE:
		case FloatType::CONST_RANGE:
			{
				if(node->floatType->range.min)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->range.min),currentHeight);
				if(node->floatType->range.max)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->range.max),currentHeight);
			}
			break;
		case FloatType::RAMP:
		case FloatType::LOOP_RAMP:
		case FloatType::OSCILATE_RAMP:
			{
				RampKey * search = node->floatType->ramp.firstKey;
				while(search)
				{
					currentHeight = layoutMathlist(root,findMathNode(root,search),currentHeight);
					search = search->next;
				}
			}
			break;
		case FloatType::ADD:
			{
				if(node->floatType->add.value1)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->add.value1),currentHeight);
				if(node->floatType->add.value2)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->add.value2),currentHeight);
			}
			break;
		case FloatType::SUBTRACT:
			{
				if(node->floatType->subtract.value1)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->subtract.value1),currentHeight);
				if(node->floatType->subtract.value2)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->subtract.value2),currentHeight);
			}
			break;
		case FloatType::MULTIPLY:
			{
				if(node->floatType->multiply.value1)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->multiply.value1),currentHeight);
				if(node->floatType->multiply.value2)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->multiply.value2),currentHeight);
			}
			break;
		case FloatType::DIVIDE:
			{
				if(node->floatType->divide.value1)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->divide.value1),currentHeight);
				if(node->floatType->divide.value2)
					currentHeight = layoutMathlist(root,findMathNode(root,node->floatType->divide.value2),currentHeight);
			}
			break;
		}
	}
	else if(node->transType)
	{
		switch(node->transType->type)
		{
		case TransformType::TARGET_TRANSFORM:
		case TransformType::FILTER_EFFECT:
		case TransformType::UP:
		case TransformType::INPUT_POINT:
		case TransformType::TARGET_TRANSFORM_STR:
		case TransformType::CAMERA:
		case TransformType::CAMERA_LOOK:
			break;
		case TransformType::OFFSET:
			{
				if(node->transType->offset.offX)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offset.offX),currentHeight);
				if(node->transType->offset.offY)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offset.offY),currentHeight);
				if(node->transType->offset.offZ)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offset.offZ),currentHeight);
				if(node->transType->offset.baseTrans)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offset.baseTrans),currentHeight);
			}
			break;
		case TransformType::OFFSET_IJK:
			{
				if(node->transType->offsetIJK.offI)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offsetIJK.offI),currentHeight);
				if(node->transType->offsetIJK.offJ)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offsetIJK.offJ),currentHeight);
				if(node->transType->offsetIJK.offK)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offsetIJK.offK),currentHeight);
				if(node->transType->offsetIJK.baseTrans)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->offsetIJK.baseTrans),currentHeight);
			}
			break;
		case TransformType::ROTATE_IJK:
			{
				if(node->transType->rotateIJK.rotI)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->rotateIJK.rotI),currentHeight);
				if(node->transType->rotateIJK.rotJ)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->rotateIJK.rotJ),currentHeight);
				if(node->transType->rotateIJK.rotK)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->rotateIJK.rotK),currentHeight);
				if(node->transType->rotateIJK.baseTrans)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->rotateIJK.baseTrans),currentHeight);
			}
			break;
		case TransformType::LOOK_AT_UP:
			{
				if(node->transType->lookAtUp.look)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->lookAtUp.look),currentHeight);
				if(node->transType->lookAtUp.at)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->lookAtUp.at),currentHeight);
				if(node->transType->lookAtUp.up)
					currentHeight = layoutMathlist(root,findMathNode(root,node->transType->lookAtUp.up),currentHeight);
			}
			break;
		}
	}
	else if(node->rampKey)
	{
		if(node->rampKey->value)
			currentHeight = layoutMathlist(root,findMathNode(root,node->rampKey->value),currentHeight);
	}

	return levelHeight + __max(returnHeight,currentHeight);
}

void unmarkMathlist(MathNode * node)
{
	MathNode * search = node;
	while(search)
	{
		search->bMarked = false;
		search = search->next;
	}
}

void markMathTree(MathNode * root, FloatType * floatType);

void markMathTree(MathNode * root, RampKey * rampKey)
{
	MathNode * node = findMathNode(root,rampKey);
	if(node)
	{
		node->bMarked = true;
	}

	if(rampKey->value)
		markMathTree(root,rampKey->value);
}

void markMathTree(MathNode * root, FloatType * floatType)
{
	MathNode * node = findMathNode(root,floatType);
	if(node)
	{
		node->bMarked = true;
	}

	switch(floatType->type)
	{
	case FloatType::CONSTANT:
	case FloatType::PARAMETER:
		break;
	case FloatType::RANGE:
	case FloatType::CONST_RANGE:
		{
			if(floatType->range.min)
				markMathTree(root,floatType->range.min);
			if(floatType->range.max)
				markMathTree(root,floatType->range.max);
		}
		break;
	case FloatType::RAMP:
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
		{
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				markMathTree(root,search);
				search = search->next;
			}
		}
		break;
	case FloatType::ADD:
		{
			if(floatType->add.value1)
				markMathTree(root,floatType->add.value1);
			if(floatType->add.value2)
				markMathTree(root,floatType->add.value2);
		}
		break;
	case FloatType::SUBTRACT:
		{
			if(floatType->subtract.value1)
				markMathTree(root,floatType->subtract.value1);
			if(floatType->subtract.value2)
				markMathTree(root,floatType->subtract.value2);
		}
		break;
	case FloatType::MULTIPLY:
		{
			if(floatType->multiply.value1)
				markMathTree(root,floatType->multiply.value1);
			if(floatType->multiply.value2)
				markMathTree(root,floatType->multiply.value2);
		}
		break;
	case FloatType::DIVIDE:
		{
			if(floatType->divide.value1)
				markMathTree(root,floatType->divide.value1);
			if(floatType->divide.value2)
				markMathTree(root,floatType->divide.value2);
		}
		break;
	}
}

void markMathTree(MathNode * root, TransformType * transType)
{
	MathNode * node = findMathNode(root,transType);
	if(node)
	{
		node->bMarked = true;
	}

	switch(transType->type)
	{
	case TransformType::TARGET_TRANSFORM:
	case TransformType::FILTER_EFFECT:
	case TransformType::UP:
	case TransformType::INPUT_POINT:
	case TransformType::TARGET_TRANSFORM_STR:
	case TransformType::CAMERA:
	case TransformType::CAMERA_LOOK:
		break;
	case TransformType::OFFSET:
		{
			if(transType->offset.offX)
				markMathTree(root,transType->offset.offX);
			if(transType->offset.offY)
				markMathTree(root,transType->offset.offY);
			if(transType->offset.offZ)
				markMathTree(root,transType->offset.offZ);
			if(transType->offset.baseTrans)
				markMathTree(root,transType->offset.baseTrans);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			if(transType->offsetIJK.offI)
				markMathTree(root,transType->offsetIJK.offI);
			if(transType->offsetIJK.offJ)
				markMathTree(root,transType->offsetIJK.offJ);
			if(transType->offsetIJK.offK)
				markMathTree(root,transType->offsetIJK.offK);
			if(transType->offsetIJK.baseTrans)
				markMathTree(root,transType->offsetIJK.baseTrans);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			if(transType->rotateIJK.rotI)
				markMathTree(root,transType->rotateIJK.rotI);
			if(transType->rotateIJK.rotJ)
				markMathTree(root,transType->rotateIJK.rotJ);
			if(transType->rotateIJK.rotK)
				markMathTree(root,transType->rotateIJK.rotK);
			if(transType->rotateIJK.baseTrans)
				markMathTree(root,transType->rotateIJK.baseTrans);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			if(transType->lookAtUp.look)
				markMathTree(root,transType->lookAtUp.look);
			if(transType->lookAtUp.at)
				markMathTree(root,transType->lookAtUp.at);
			if(transType->lookAtUp.up)
				markMathTree(root,transType->lookAtUp.up);
		}
		break;
	}
}

void updateMathViewRegion(HWND hWindow,MathEditInfo * info)
{
	info->neededArea.top = 0;
	info->neededArea.left = 0;
	info->neededArea.bottom = (U32)(MATH_AREA_SIZE/info->zoomLevel);
	info->neededArea.right = (U32)(MATH_AREA_SIZE/info->zoomLevel);
	GetClientRect(hWindow,&(info->viewArea));
	if(info->neededArea.right <= info->viewArea.right)
	{
		info->sx = 0;
		SetScrollPos(hWindow,SB_HORZ,0,true);
		EnableScrollBar(hWindow,SB_HORZ,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(hWindow,SB_HORZ,ESB_ENABLE_BOTH);
		S32 width = info->neededArea.right-info->viewArea.right;
		SetScrollRange(hWindow,SB_HORZ,0,width,true);
		if(info->sx > width)
		{
			info->sx = width;
			SetScrollPos(hWindow,SB_HORZ,info->sx,true);
		}
	}
	if(info->neededArea.bottom <= info->viewArea.bottom)
	{
		info->sy = 0;
		SetScrollPos(hWindow,SB_VERT,0,true);
		EnableScrollBar(hWindow,SB_VERT,ESB_DISABLE_BOTH);
	}
	else
	{
		EnableScrollBar(hWindow,SB_VERT,ESB_ENABLE_BOTH);
		S32 height = info->neededArea.bottom-info->viewArea.bottom;
		SetScrollRange(hWindow,SB_VERT,0,height,true);
		if(info->sy > height)
		{
			info->sy = height;
			SetScrollPos(hWindow,SB_VERT,info->sy,true);
		}
	}
}

void allocGraphDC(HDC hdc,MathEditInfo * info)
{
	if(!(info->mathDC))
	{
		info->mathDC = CreateCompatibleDC(hdc);
		info->mathBitmap = CreateCompatibleBitmap(hdc,MATH_AREA_SIZE,MATH_AREA_SIZE);
		SelectObject(info->mathDC,info->mathBitmap);
	}
}

void drawMathTree(MathEditInfo * info, FloatType * floatType, S32 baseX, S32 baseY);

void drawMathTree(MathEditInfo * info, RampKey * rampKey, S32 baseX, S32 baseY)
{
	MathNode * node = findMathNode(info->mathList,rampKey);
	RECT rect;
	rect.left = node->xPos+baseX;
	rect.right = rect.left + node->width;
	rect.top = node->yPos+baseY;
	rect.bottom = rect.top + node->height;

	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.bottom,info->rampBrush);

	drawRect(info->mathDC,rect.left,rect.top,rect.left+1,rect.bottom,info->rampHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.top+1,info->rampHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.bottom-1,rect.right,rect.bottom,info->rampLowBoarderBrush);
	drawRect(info->mathDC,rect.right-1,rect.top,rect.right,rect.bottom,info->rampLowBoarderBrush);

	char buffer[64];
	sprintf(buffer,"%f",rampKey->key);
	TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,buffer,strlen(buffer));


	if(rampKey->value)
	{
		SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
		SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER,rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH);

		MathNode * child = findMathNode(info->mathList,rampKey->value);
		drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_CIRC_WIDTH_HALF,rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

		drawMathTree(info,rampKey->value,rect.left,rect.top);
	}
	else
	{
		SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
		SelectObject(info->mathDC,GetStockObject(WHITE_BRUSH));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER,rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH);
	}
}

void drawMathTree(MathEditInfo * info, FloatType * floatType, S32 baseX, S32 baseY)
{
	MathNode * node = findMathNode(info->mathList,floatType);
	RECT rect;
	rect.left = node->xPos+baseX;
	rect.right = rect.left + node->width;
	rect.top = node->yPos+baseY;
	rect.bottom = rect.top + node->height;

	//draw background
	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.bottom,info->floatBrush);

	drawRect(info->mathDC,rect.left,rect.top,rect.left+1,rect.bottom,info->floatHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.top+1,info->floatHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.bottom-1,rect.right,rect.bottom,info->floatLowBoarderBrush);
	drawRect(info->mathDC,rect.right-1,rect.top,rect.right,rect.bottom,info->floatLowBoarderBrush);

	//draw label and inputs
	SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
	SelectObject(info->mathDC,GetStockObject(WHITE_BRUSH));

	char buffer[64];

	switch(floatType->type)
	{
	case FloatType::CONSTANT:
		sprintf(buffer,"%f",floatType->constant);
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,buffer,strlen(buffer));
		break;
	case FloatType::PARAMETER:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,floatType->parameter.name,strlen(floatType->parameter.name));
		break;
	case FloatType::RANGE:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Range",strlen("Range"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Min",strlen("Min"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Max",strlen("Max"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	case FloatType::CONST_RANGE:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Const Range",strlen("Const Range"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Min",strlen("Min"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Max",strlen("Max"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	case FloatType::RAMP:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Ramp",strlen("Ramp"));
			U32 key = 0;
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				sprintf(buffer,"Key %d",key);
				S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
				TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,buffer,strlen(buffer));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
					rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
				search = search->next;
				++key;
			}

			S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,"New Key",strlen("New Key"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
				rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
		}
		break;
	case FloatType::LOOP_RAMP:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Looping Ramp",strlen("Looping Ramp"));
			U32 key = 0;
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				sprintf(buffer,"Key %d",key);
				S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
				TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,buffer,strlen(buffer));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
					rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
				search = search->next;
				++key;
			}

			S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,"New Key",strlen("New Key"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
				rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
		}
		break;
	case FloatType::OSCILATE_RAMP:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Oscilating Ramp",strlen("Oscilating Ramp"));
			U32 key = 0;
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				sprintf(buffer,"Key %d",key);
				S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
				TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,buffer,strlen(buffer));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
					rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
				search = search->next;
				++key;
			}

			S32 baseHeight = rect.top+FONT_BUFFER+(MF_LINE_HEIGHT*(key+1));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,baseHeight,"New Key",strlen("New Key"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight,
				rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);
		}
		break;
	case FloatType::ADD:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Add",strlen("Add"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Input1",strlen("Input1"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Input2",strlen("Input2"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	case FloatType::SUBTRACT:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Subtract",strlen("Subtract"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Input1",strlen("Input1"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Input2",strlen("Input2"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	case FloatType::MULTIPLY:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Multiply",strlen("Multiply"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Input1",strlen("Input1"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Input2",strlen("Input2"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	case FloatType::DIVIDE:
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Divide",strlen("Divide"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Input1",strlen("Input1"));
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,"Input2",strlen("Input2"));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		break;
	}

	//draw sub math tree
	switch(floatType->type)
	{
	case FloatType::CONSTANT:
	case FloatType::PARAMETER:
		break;
	case FloatType::RANGE:
	case FloatType::CONST_RANGE:
		{
			if(floatType->range.min)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->range.min);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->range.min,rect.left,rect.top);
			}
			if(floatType->range.max)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT
					,rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->range.max);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_CIRC_WIDTH_HALF+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->range.max,rect.left,rect.top);
			}
		}
		break;
	case FloatType::RAMP:
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
		{
			S32 baseHeight = rect.top+FONT_BUFFER+MF_LINE_HEIGHT;
			RampKey * search = floatType->ramp.firstKey;
			while(search)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),baseHeight
					,rect.right-FONT_BUFFER,baseHeight+MF_CIRC_WIDTH);

				MathNode * child = findMathNode(info->mathList,search);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),baseHeight+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));
				
				drawMathTree(info,search,rect.left,rect.top);

				baseHeight += MF_LINE_HEIGHT;
				search = search->next;
			}
		}
		break;
	case FloatType::ADD:
		{
			if(floatType->add.value1)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->add.value1);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->add.value1,rect.left,rect.top);
			}
			if(floatType->add.value2)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->add.value2);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->add.value2,rect.left,rect.top);
			}
		}
		break;
	case FloatType::SUBTRACT:
		{
			if(floatType->subtract.value1)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->subtract.value1);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->subtract.value1,rect.left,rect.top);
			}
			if(floatType->subtract.value2)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->subtract.value2);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->subtract.value2,rect.left,rect.top);
			}
		}
		break;
	case FloatType::MULTIPLY:
		{
			if(floatType->multiply.value1)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->multiply.value1);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->multiply.value1,rect.left,rect.top);
			}
			if(floatType->multiply.value2)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->multiply.value2);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->multiply.value2,rect.left,rect.top);
			}
		}
		break;
	case FloatType::DIVIDE:
		{
			if(floatType->divide.value1)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->divide.value1);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->divide.value1,rect.left,rect.top);
			}
			if(floatType->divide.value2)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,floatType->divide.value2);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,floatType->divide.value2,rect.left,rect.top);
			}
		}
		break;
	}
}

void drawMathTree(MathEditInfo * info, TransformType * transType, S32 baseX, S32 baseY)
{
	MathNode * node = findMathNode(info->mathList,transType);
	RECT rect;
	rect.left = node->xPos+baseX;
	rect.right = rect.left + node->width;
	rect.top = node->yPos+baseY;
	rect.bottom = rect.top + node->height;

	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.bottom,info->transBrush);

	drawRect(info->mathDC,rect.left,rect.top,rect.left+1,rect.bottom,info->transHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.top+1,info->transHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.bottom-1,rect.right,rect.bottom,info->transLowBoarderBrush);
	drawRect(info->mathDC,rect.right-1,rect.top,rect.right,rect.bottom,info->transLowBoarderBrush);

	char buffer[128];

	SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
	SelectObject(info->mathDC,GetStockObject(WHITE_BRUSH));

	switch(transType->type)
	{
	case TransformType::UP:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Up",strlen("Up"));
		}
		break;
	case TransformType::FILTER_EFFECT:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,transType->filterName,strlen(transType->filterName));
		}
		break;
	case TransformType::INPUT_POINT:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Input Point",strlen("Input Point"));
		}
		break;
	case TransformType::CAMERA:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Camera",strlen("Camera"));
		}
		break;
	case TransformType::CAMERA_LOOK:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Camera Look",strlen("Camera Look"));
		}
		break;

	case TransformType::TARGET_TRANSFORM:
		{
			sprintf(buffer,"Target: %d",transType->targetTrans.targetID);
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,buffer,strlen(buffer));
			sprintf(buffer,"HP: %d",transType->targetTrans.hpID);
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,buffer,strlen(buffer));
		}
		break;
	case TransformType::TARGET_TRANSFORM_STR:
			sprintf(buffer,"Target: %d",transType->targetTrans_str.targetID);
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,buffer,strlen(buffer));
			sprintf(buffer,"HP: %s",transType->targetTrans_str.hpName);
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,buffer,strlen(buffer));
		break;
	case TransformType::OFFSET:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Offset XYZ",strlen("Offset XYZ"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Offset X",strlen("Offset X"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Offset Y",strlen("Offset Y"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Offset Z",strlen("Offset Z"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Base Trans",strlen("Base Trans"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Offset IJK",strlen("Offset IJK"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Offset I",strlen("Offset I"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Offset J",strlen("Offset J"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Offset K",strlen("Offset K"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Base Trans",strlen("Base Trans"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Rotate IJK",strlen("Rotate IJK"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Rotate I",strlen("Rotate I"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Rotate J",strlen("Rotate J"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Rotate K",strlen("Rotate K"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Base Trans",strlen("Base Trans"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Look At Up",strlen("Look At Up"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT,"Look",strlen("Look"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"At",strlen("At"));
			TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
				"Up",strlen("Up"));
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
			Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER,
				rect.right-FONT_BUFFER,rect.top+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+FONT_BUFFER+MF_CIRC_WIDTH);
		}
		break;
	}

	switch(transType->type)
	{
	case TransformType::TARGET_TRANSFORM:
	case TransformType::FILTER_EFFECT:
	case TransformType::UP:
	case TransformType::INPUT_POINT:
	case TransformType::TARGET_TRANSFORM_STR:
	case TransformType::CAMERA:
	case TransformType::CAMERA_LOOK:
		break;
	case TransformType::OFFSET:
		{
			if(transType->offset.offX)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offset.offX);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offset.offX,rect.left,rect.top);
			}
			if(transType->offset.offY)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offset.offY);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offset.offY,rect.left,rect.top);
			}
			if(transType->offset.offZ)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offset.offZ);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offset.offZ,rect.left,rect.top);
			}
			if(transType->offset.baseTrans)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offset.baseTrans);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offset.baseTrans,rect.left,rect.top);
			}
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			if(transType->offsetIJK.offI)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offsetIJK.offI);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offsetIJK.offI,rect.left,rect.top);
			}
			if(transType->offsetIJK.offJ)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offsetIJK.offJ);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offsetIJK.offJ,rect.left,rect.top);
			}
			if(transType->offsetIJK.offK)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offsetIJK.offK);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offsetIJK.offK,rect.left,rect.top);
			}
			if(transType->offsetIJK.baseTrans)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->offsetIJK.baseTrans);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->offsetIJK.baseTrans,rect.left,rect.top);
			}
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			if(transType->rotateIJK.rotI)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->rotateIJK.rotI);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->rotateIJK.rotI,rect.left,rect.top);
			}
			if(transType->rotateIJK.rotJ)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->rotateIJK.rotJ);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->rotateIJK.rotJ,rect.left,rect.top);
			}
			if(transType->rotateIJK.rotK)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->rotateIJK.rotK);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->rotateIJK.rotK,rect.left,rect.top);
			}
			if(transType->rotateIJK.baseTrans)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->rotateIJK.baseTrans);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->rotateIJK.baseTrans,rect.left,rect.top);
			}
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			if(transType->lookAtUp.look)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->lookAtUp.look);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->lookAtUp.look,rect.left,rect.top);
			}
			if(transType->lookAtUp.at)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->lookAtUp.at);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->lookAtUp.at,rect.left,rect.top);
			}
			if(transType->lookAtUp.up)
			{
				SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
				SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
				Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
					rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT);

				MathNode * child = findMathNode(info->mathList,transType->lookAtUp.up);
				drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH_HALF,
					rect.left+child->xPos,rect.top+child->yPos+(child->height >> 1));

				drawMathTree(info,transType->lookAtUp.up,rect.left,rect.top);
			}
		}
		break;
	}
}

void drawMath(MathEditInfo * info)
{
	//make brushes
	LOGBRUSH logBrush;
	memset(&logBrush,0,sizeof(LOGBRUSH));
	logBrush.lbStyle = BS_SOLID;

	logBrush.lbColor = RGB(200,100,100);
	info->rootBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(250,150,150);
	info->rootHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(100,50,50);
	info->rootLowBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(100,200,100);
	info->floatBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(150,250,150);
	info->floatHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(50,100,50);
	info->floatLowBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(100,100,200);
	info->rampBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(150,150,250);
	info->rampHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(50,50,100);
	info->rampLowBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(100,200,200);
	info->transBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(150,250,250);
	info->transHighBoarderBrush = CreateBrushIndirect(&logBrush);

	logBrush.lbColor = RGB(50,100,100);
	info->transLowBoarderBrush = CreateBrushIndirect(&logBrush);

	info->font = CreateFont(-MulDiv(MF_FONT_POINT_SIZE, GetDeviceCaps(info->mathDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Arial");
	SelectObject(info->mathDC,info->font);

	SetBkMode(info->mathDC,TRANSPARENT);

	//draw base rect

	RECT rect;
	rect.left = info->rootX;
	rect.right = info->rootX + MF_STANDARD_WIDTH;
	rect.top = info->rootY;
	rect.bottom = info->rootY + MF_LINE_HEIGHT;

	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.bottom,info->rootBrush);

	drawRect(info->mathDC,rect.left,rect.top,rect.left+1,rect.bottom,info->rootHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.top,rect.right,rect.top+1,info->rootHighBoarderBrush);
	drawRect(info->mathDC,rect.left,rect.bottom-1,rect.right,rect.bottom,info->rootLowBoarderBrush);
	drawRect(info->mathDC,rect.right-1,rect.top,rect.right,rect.bottom,info->rootLowBoarderBrush);

	MathNode * firstChild = NULL;
	if(info->bFloatInput)
	{
		if(info->inputFloat)
			firstChild = findMathNode(info->mathList,info->inputFloat);
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Float",strlen("Float"));
	}
	else if(info->bTransInput)
	{
		if(info->inputTrans)
			firstChild = findMathNode(info->mathList,info->inputTrans);
		TextOut(info->mathDC,rect.left+FONT_BUFFER,rect.top+FONT_BUFFER,"Transform",strlen("Transform"));
	}

	if(firstChild)
	{
		SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
		SelectObject(info->mathDC,GetStockObject(BLACK_BRUSH));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH);

		drawLine(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH_HALF),rect.top+FONT_BUFFER+MF_CIRC_WIDTH_HALF,
			rect.left+firstChild->xPos,rect.top+firstChild->yPos+(firstChild->height >> 1));
	}
	else
	{
		SelectObject(info->mathDC,GetStockObject(BLACK_PEN));
		SelectObject(info->mathDC,GetStockObject(WHITE_BRUSH));
		Ellipse(info->mathDC,rect.right-(FONT_BUFFER+MF_CIRC_WIDTH),rect.top+FONT_BUFFER,
			rect.right-FONT_BUFFER,rect.top+FONT_BUFFER+MF_CIRC_WIDTH);
	}

	//draw main tree
	
	if(info->inputFloat)
		drawMathTree(info,info->inputFloat,info->rootX,info->rootY);
	else if(info->inputTrans)
		drawMathTree(info,info->inputTrans,info->rootX,info->rootY);

	//draw orphan trees

	MathNode * orphan = info->orphanList;
	while(orphan)
	{
		if(orphan->floatType)
			drawMathTree(info,orphan->floatType,0,0);
		else if(orphan->transType)
			drawMathTree(info,orphan->transType,0,0);
		else if(orphan->rampKey)
			drawMathTree(info,orphan->rampKey,0,0);

		orphan = orphan->orphanNext;
	}

	//draw connecting line
	if(info->bConnecting)
	{
		drawLine(info->mathDC,info->mouseX,info->mouseY, info->orgX, info->orgY);
	}

	//release brushes
	DeleteObject(info->font);

	DeleteObject(info->transBrush);
	DeleteObject(info->transHighBoarderBrush);
	DeleteObject(info->transLowBoarderBrush);
	DeleteObject(info->rampBrush);
	DeleteObject(info->rampHighBoarderBrush);
	DeleteObject(info->rampLowBoarderBrush);
	DeleteObject(info->floatBrush);
	DeleteObject(info->floatHighBoarderBrush);
	DeleteObject(info->floatLowBoarderBrush);
	DeleteObject(info->rootBrush);
	DeleteObject(info->rootHighBoarderBrush);
	DeleteObject(info->rootLowBoarderBrush);

}

void invalidateMathView(HWND hWindow, MathEditInfo * info)
{
	info->bAreaInvalid = true;
	InvalidateRect(hWindow,NULL,false);
}

MathNode * findMathNodeParent(MathEditInfo * info, RampKey * node)
{
	MathNode * search = info->mathList;
	while(search)
	{
		if(search->floatType)
		{
			switch(search->floatType->type)
			{
			case FloatType::CONSTANT:
			case FloatType::PARAMETER:
			case FloatType::RANGE:
			case FloatType::CONST_RANGE:
			case FloatType::ADD:
			case FloatType::SUBTRACT:
			case FloatType::MULTIPLY:
			case FloatType::DIVIDE:
				break;
			case FloatType::RAMP:
			case FloatType::LOOP_RAMP:
			case FloatType::OSCILATE_RAMP:
				{
					RampKey * rampSearch = search->floatType->ramp.firstKey;
					while(rampSearch)
					{
						if(rampSearch == node)
							return search;
						rampSearch = rampSearch->next;
					}
				}
				break;
			}
		}
		search = search->next;
	}
	return NULL;
}

MathNode * findMathNodeParent(MathEditInfo * info, FloatType * node)
{
	DWORD searchVal = 0;
	MathNode * search = info->mathList;
	while(search)
	{
		if(search->rampKey)
		{
			if(search->rampKey->value == node)
				return search;
		}
		else if(search->floatType)
		{
			switch(search->floatType->type)
			{
			case FloatType::CONSTANT:
			case FloatType::PARAMETER:
			case FloatType::RAMP:
			case FloatType::LOOP_RAMP:
			case FloatType::OSCILATE_RAMP:
				break;
			case FloatType::RANGE:
			case FloatType::CONST_RANGE:
				{
					if(search->floatType->range.min == node)
						return search;
					if(search->floatType->range.max == node)
						return search;				
				}
				break;
			case FloatType::ADD:
				{
					if(search->floatType->add.value1 == node)
						return search;
					if(search->floatType->add.value2 == node)
						return search;				
				}
				break;
			case FloatType::SUBTRACT:
				{
					if(search->floatType->subtract.value1 == node)
						return search;
					if(search->floatType->subtract.value2 == node)
						return search;				
				}
				break;
			case FloatType::MULTIPLY:
				{
					if(search->floatType->multiply.value1 == node)
						return search;
					if(search->floatType->multiply.value2 == node)
						return search;				
				}
				break;
			case FloatType::DIVIDE:
				{
					if(search->floatType->divide.value1 == node)
						return search;
					if(search->floatType->divide.value2 == node)
						return search;				
				}
				break;
			}
		}
		else if(search->transType)
		{
			switch(search->transType->type)
			{
			case TransformType::TARGET_TRANSFORM:
			case TransformType::FILTER_EFFECT:
			case TransformType::UP:
			case TransformType::INPUT_POINT:
			case TransformType::TARGET_TRANSFORM_STR:
			case TransformType::CAMERA:
			case TransformType::CAMERA_LOOK:
			case TransformType::LOOK_AT_UP:
				break;
			case TransformType::OFFSET:
				{
					if(search->transType->offset.offX == node)
						return search;
					if(search->transType->offset.offY == node)
						return search;
					if(search->transType->offset.offZ == node)
						return search;
				}
				break;
			case TransformType::OFFSET_IJK:
				{
					if(search->transType->offsetIJK.offI == node)
						return search;
					if(search->transType->offsetIJK.offJ == node)
						return search;
					if(search->transType->offsetIJK.offK == node)
						return search;
				}
				break;
			case TransformType::ROTATE_IJK:
				{
					if(search->transType->rotateIJK.rotI == node)
						return search;
					if(search->transType->rotateIJK.rotJ == node)
						return search;
					if(search->transType->rotateIJK.rotK == node)
						return search;
				}
				break;
			}
		}
		search = search->next;
	}
	return NULL;
}

MathNode * findMathNodeParent(MathEditInfo * info, TransformType * node)
{
	DWORD searchVal = 0;
	MathNode * search = info->mathList;
	while(search)
	{
		if(search->transType)
		{
			switch(search->transType->type)
			{
			case TransformType::TARGET_TRANSFORM:
			case TransformType::FILTER_EFFECT:
			case TransformType::UP:
			case TransformType::INPUT_POINT:
			case TransformType::TARGET_TRANSFORM_STR:
			case TransformType::CAMERA:
			case TransformType::CAMERA_LOOK:
				break;
			case TransformType::OFFSET:
				{
					if(search->transType->offset.baseTrans == node)
						return search;
				}
				break;
			case TransformType::OFFSET_IJK:
				{
					if(search->transType->offsetIJK.baseTrans == node)
						return search;
				}
				break;
			case TransformType::ROTATE_IJK:
				{
					if(search->transType->rotateIJK.baseTrans == node)
						return search;
				}
				break;
			case TransformType::LOOK_AT_UP:
				{
					if(search->transType->lookAtUp.look == node)
						return search;
					if(search->transType->lookAtUp.at == node)
						return search;
					if(search->transType->lookAtUp.up == node)
						return search;
				}
				break;
			}
		}
		search = search->next;
	}
	return NULL;
}

MathNode * findMathNodeParent(MathEditInfo * info, MathNode * node)
{
	if(node->rampKey)
		return findMathNodeParent(info,node->rampKey);
	if(node->floatType)
		return findMathNodeParent(info,node->floatType);
	if(node->transType)
		return findMathNodeParent(info,node->transType);
	return NULL;
}

void findMathNodeAbsPos(MathEditInfo * info, MathNode * node,S32 & nodeX,S32 & nodeY)
{
	nodeX = node->xPos;
	nodeY = node->yPos;

	MathNode * parent = findMathNodeParent(info,node);
	if(parent)
	{
		S32 parentX = 0;
		S32 parentY = 0;
		findMathNodeAbsPos(info,parent,parentX,parentY);
		nodeX += parentX;
		nodeY += parentY;
	}
}

void disconnectMathNodes(MathEditInfo * info, MathNode * parent,MathNode * child)
{
	if(child->rampKey)
	{
		if(parent->floatType)
		{
			switch(parent->floatType->type)
			{
			case FloatType::CONSTANT:
			case FloatType::PARAMETER:
			case FloatType::RANGE:
			case FloatType::CONST_RANGE:
			case FloatType::ADD:
			case FloatType::SUBTRACT:
			case FloatType::MULTIPLY:
			case FloatType::DIVIDE:
				break;
			case FloatType::RAMP:
			case FloatType::LOOP_RAMP:
			case FloatType::OSCILATE_RAMP:
				{
					RampKey * search = parent->floatType->ramp.firstKey;
					RampKey * prev = NULL;
					while(search)
					{
						if(search == child->rampKey)
						{
							if(prev)
								prev->next = search->next;
							else
								parent->floatType->ramp.firstKey = search->next;
							search->next = NULL;
							break;
						}
						prev = search;
						search = search->next;
					}
				}
				break;
			}
		}
	}
	else if(child->floatType)
	{
		if(parent->rampKey)
		{
			if(parent->rampKey->value == child->floatType)
				parent->rampKey->value = NULL;
		}
		else if(parent->floatType)
		{
			switch(parent->floatType->type)
			{
			case FloatType::CONSTANT:
			case FloatType::PARAMETER:
			case FloatType::RAMP:
			case FloatType::LOOP_RAMP:
			case FloatType::OSCILATE_RAMP:
				break;
			case FloatType::RANGE:
			case FloatType::CONST_RANGE:
				{
					if(parent->floatType->range.min == child->floatType)
						parent->floatType->range.min = NULL;
					if(parent->floatType->range.max == child->floatType)
						parent->floatType->range.max = NULL;
				}
				break;
			case FloatType::ADD:
				{
					if(parent->floatType->add.value1 == child->floatType)
						parent->floatType->add.value1 = NULL;
					if(parent->floatType->add.value2 == child->floatType)
						parent->floatType->add.value2 = NULL;
				}
				break;
			case FloatType::SUBTRACT:
				{
					if(parent->floatType->subtract.value1 == child->floatType)
						parent->floatType->subtract.value1 = NULL;
					if(parent->floatType->subtract.value2 == child->floatType)
						parent->floatType->subtract.value2 = NULL;
				}
				break;
			case FloatType::MULTIPLY:
				{
					if(parent->floatType->multiply.value1 == child->floatType)
						parent->floatType->multiply.value1 = NULL;
					if(parent->floatType->multiply.value2 == child->floatType)
						parent->floatType->multiply.value2 = NULL;
				}
				break;
			case FloatType::DIVIDE:
				{
					if(parent->floatType->divide.value1 == child->floatType)
						parent->floatType->divide.value1 = NULL;
					if(parent->floatType->divide.value2 == child->floatType)
						parent->floatType->divide.value2 = NULL;
				}
				break;
			}
		}
		else if(parent->transType)
		{
			switch(parent->transType->type)
			{
			case TransformType::TARGET_TRANSFORM:
			case TransformType::FILTER_EFFECT:
			case TransformType::UP:
			case TransformType::INPUT_POINT:
			case TransformType::TARGET_TRANSFORM_STR:
			case TransformType::CAMERA:
			case TransformType::CAMERA_LOOK:
			case TransformType::LOOK_AT_UP:
				break;
			case TransformType::OFFSET:
				{
					if(parent->transType->offset.offX == child->floatType)
						parent->transType->offset.offX = NULL;
					if(parent->transType->offset.offY == child->floatType)
						parent->transType->offset.offY = NULL;
					if(parent->transType->offset.offZ == child->floatType)
						parent->transType->offset.offZ = NULL;
				}
				break;
			case TransformType::OFFSET_IJK:
				{
					if(parent->transType->offsetIJK.offI == child->floatType)
						parent->transType->offsetIJK.offI = NULL;
					if(parent->transType->offsetIJK.offJ == child->floatType)
						parent->transType->offsetIJK.offJ = NULL;
					if(parent->transType->offsetIJK.offK == child->floatType)
						parent->transType->offsetIJK.offK = NULL;
				}
				break;
			case TransformType::ROTATE_IJK:
				{
					if(parent->transType->rotateIJK.rotI == child->floatType)
						parent->transType->rotateIJK.rotI = NULL;
					if(parent->transType->rotateIJK.rotJ == child->floatType)
						parent->transType->rotateIJK.rotJ = NULL;
					if(parent->transType->rotateIJK.rotK == child->floatType)
						parent->transType->rotateIJK.rotK = NULL;
				}
				break;
			}
		}
	}
	else if(child->transType)
	{

		if(parent->transType)
		{
			switch(parent->transType->type)
			{
			case TransformType::TARGET_TRANSFORM:
			case TransformType::FILTER_EFFECT:
			case TransformType::UP:
			case TransformType::INPUT_POINT:
			case TransformType::TARGET_TRANSFORM_STR:
			case TransformType::CAMERA:
			case TransformType::CAMERA_LOOK:
				break;
			case TransformType::OFFSET:
				{
					if(parent->transType->offset.baseTrans)
						parent->transType->offset.baseTrans = NULL;
				}
				break;
			case TransformType::OFFSET_IJK:
				{
					if(parent->transType->offsetIJK.baseTrans)
						parent->transType->offsetIJK.baseTrans = NULL;
				}
				break;
			case TransformType::ROTATE_IJK:
				{
					if(parent->transType->rotateIJK.baseTrans)
						parent->transType->rotateIJK.baseTrans = NULL;
				}
				break;
			case TransformType::LOOK_AT_UP:
				{
					if(parent->transType->lookAtUp.look)
						parent->transType->lookAtUp.look = NULL;
					if(parent->transType->lookAtUp.at)
						parent->transType->lookAtUp.at = NULL;
					if(parent->transType->lookAtUp.up)
						parent->transType->lookAtUp.up = NULL;
				}
				break;
			}
		}
	}
	S32 parentX = 0;
	S32 parentY = 0;
	findMathNodeAbsPos(info,parent,parentX,parentY);
	child->xPos = child->xPos+parentX;
	child->yPos = child->yPos+parentY;

	child->orphanNext = info->orphanList;
	info->orphanList = child;
}

bool inRect(S32 xPos,S32 yPos, S32 minX,S32 minY,S32 maxX, S32 maxY)
{
	return (xPos >= minX && xPos <= maxX && yPos >= minY && yPos <= maxY);
}

bool mathHitTest(MathEditInfo * info,S32 xPos,S32 yPos)
{
	xPos = (S32)(xPos*info->zoomLevel)+info->sx;
	yPos = (S32)(yPos*info->zoomLevel)+info->sy;
	//first check the root node
	if(xPos >= info->rootX && xPos <= info->rootX+MF_STANDARD_WIDTH && yPos >= info->rootY && yPos <= info->rootY+MF_LINE_HEIGHT)
	{
		info->connectNode = NULL;
		info->bConnecting = true;
		info->orgX = xPos;
		info->orgY = yPos;
		info->mouseX = xPos;
		info->mouseY = yPos;

		MathNode * child = NULL;
		if(info->bFloatInput && info->inputFloat)
			child = findMathNode(info->mathList,info->inputFloat);
		else if(info->bTransInput&& info->inputTrans)
			child = findMathNode(info->mathList,info->inputTrans);

		info->inputFloat = NULL;
		info->inputTrans = NULL;
		if(child)
		{
			child->xPos = child->xPos+info->rootX;
			child->yPos = child->yPos+info->rootY;

			child->orphanNext = info->orphanList;
			info->orphanList = child;
		}
		return true;
	}

	MathNode * node = info->mathList;
	while(node)
	{
		S32 nodeX = 0;
		S32 nodeY = 0;
		findMathNodeAbsPos(info,node,nodeX,nodeY);
		if(xPos >= nodeX && xPos <= nodeX+node->width && yPos >= nodeY && yPos <= nodeY+node->height)
		{
			//ok I hit, but did I hit an output or not
			info->connectFloat = NULL;
			info->connectTrans = NULL;
			info->connectKey = false;
			info->connectNode = NULL;
			if(node->rampKey)
			{
				if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER,
					nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_CIRC_WIDTH))
				{
					//value hit
					if(node->rampKey->value)
						disconnectMathNodes(info,node,findMathNode(info->mathList,node->rampKey->value));
					info->connectFloat = &(node->rampKey->value);
				}
			}
			else if(node->floatType)
			{
				switch(node->floatType->type)
				{
				case FloatType::CONSTANT:
				case FloatType::PARAMETER:
					break;
				case FloatType::RANGE:
				case FloatType::CONST_RANGE:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->range.min)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->range.min));
							info->connectFloat = &(node->floatType->range.min);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->range.max)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->range.max));
							info->connectFloat = &(node->floatType->range.max);
						}
					}
					break;
				case FloatType::RAMP:
				case FloatType::LOOP_RAMP:
				case FloatType::OSCILATE_RAMP:
					{
						U32 key = 1;
						RampKey * search = node->floatType->ramp.firstKey;
						while(search)
						{
							if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+(MF_LINE_HEIGHT*key),
								nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+(MF_LINE_HEIGHT*key)+MF_CIRC_WIDTH))
							{
								disconnectMathNodes(info,node,findMathNode(info->mathList,search));
								info->connectKey = true;
								computeNodeSize(node);
								break;
							}
							++key;
							search = search->next;
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+(MF_LINE_HEIGHT*key),
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+(MF_LINE_HEIGHT*key)+MF_CIRC_WIDTH))
						{
							info->connectKey = true;
						}
					}
					break;
				case FloatType::ADD:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->add.value1)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->add.value1));
							info->connectFloat = &(node->floatType->add.value1);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->add.value2)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->add.value2));
							info->connectFloat = &(node->floatType->add.value2);
						}
					}
					break;
				case FloatType::SUBTRACT:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->subtract.value1)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->subtract.value1));
							info->connectFloat = &(node->floatType->subtract.value1);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->subtract.value2)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->subtract.value2));
							info->connectFloat = &(node->floatType->subtract.value2);
						}
					}
					break;
				case FloatType::MULTIPLY:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->multiply.value1)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->multiply.value1));
							info->connectFloat = &(node->floatType->multiply.value1);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->multiply.value2)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->multiply.value2));
							info->connectFloat = &(node->floatType->multiply.value2);
						}
					}
					break;
				case FloatType::DIVIDE:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->divide.value1)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->divide.value1));
							info->connectFloat = &(node->floatType->divide.value1);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->floatType->divide.value2)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->divide.value2));
							info->connectFloat = &(node->floatType->divide.value2);
						}
					}
					break;
				}
			}
			else if(node->transType)
			{
				switch(node->transType->type)
				{
				case TransformType::TARGET_TRANSFORM:
				case TransformType::FILTER_EFFECT:
				case TransformType::UP:
				case TransformType::INPUT_POINT:
				case TransformType::TARGET_TRANSFORM_STR:
				case TransformType::CAMERA:
				case TransformType::CAMERA_LOOK:
					break;
				case TransformType::OFFSET:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offset.offX)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offX));
							info->connectFloat = &(node->transType->offset.offX);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offset.offY)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offY));
							info->connectFloat = &(node->transType->offset.offY);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offset.offZ)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offZ));
							info->connectFloat = &(node->transType->offset.offZ);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offset.baseTrans)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.baseTrans));
							info->connectTrans = &(node->transType->offset.baseTrans);
						}
					}
					break;
				case TransformType::OFFSET_IJK:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offsetIJK.offI)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offI));
							info->connectFloat = &(node->transType->offsetIJK.offI);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offsetIJK.offJ)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offJ));
							info->connectFloat = &(node->transType->offsetIJK.offJ);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offsetIJK.offK)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offK));
							info->connectFloat = &(node->transType->offsetIJK.offK);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->offsetIJK.baseTrans)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.baseTrans));
							info->connectTrans = &(node->transType->offsetIJK.baseTrans);
						}
					}
					break;
				case TransformType::ROTATE_IJK:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->rotateIJK.rotI)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotI));
							info->connectFloat = &(node->transType->rotateIJK.rotI);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->rotateIJK.rotJ)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotJ));
							info->connectFloat = &(node->transType->rotateIJK.rotJ);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->rotateIJK.rotK)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotK));
							info->connectFloat = &(node->transType->rotateIJK.rotK);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->rotateIJK.baseTrans)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.baseTrans));
							info->connectTrans = &(node->transType->rotateIJK.baseTrans);
						}
					}
					break;
				case TransformType::LOOK_AT_UP:
					{
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->lookAtUp.look)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.look));
							info->connectTrans = &(node->transType->lookAtUp.look);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->lookAtUp.at)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.at));
							info->connectTrans = &(node->transType->lookAtUp.at);
						}
						if(inRect(xPos,yPos,nodeX+node->width-(FONT_BUFFER+MF_CIRC_WIDTH),nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT,
							nodeX+node->width-FONT_BUFFER,nodeY+FONT_BUFFER+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_LINE_HEIGHT+MF_CIRC_WIDTH))
						{
							//value hit
							if(node->transType->lookAtUp.up)
								disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.up));
							info->connectTrans = &(node->transType->lookAtUp.up);
						}
					}
					break;
				}
			}

			info->mouseX = xPos;
			info->mouseY = yPos;
			if(info->connectFloat || info->connectTrans || info->connectKey)
			{
				info->connectNode = node;
				info->bConnecting = true;
				info->orgX = xPos;
				info->orgY = yPos;
			}
			else
			{
				info->selectedMath = node;
				info->bMoving = true;
			}

			return true;
		}
		node = node->next;
	}
	return false;
}

MathNode * hitTestFindNode(MathEditInfo * info, S32 xPos, S32 yPos)
{
	MathNode * node = info->mathList;
	while(node)
	{
		S32 nodeX = 0;
		S32 nodeY = 0;
		findMathNodeAbsPos(info,node,nodeX,nodeY);
		if(xPos >= nodeX && xPos <= nodeX+node->width && yPos >= nodeY && yPos <= nodeY+node->height)
		{
			return node;
		}
		node = node->next;
	}
	return NULL;
}

void orphanChildren(MathEditInfo * info,MathNode * node)
{
	if(node->rampKey)
	{
		if(node->rampKey->value)
			disconnectMathNodes(info,node,findMathNode(info->mathList,node->rampKey->value));
	}
	else if(node->floatType)
	{
		switch(node->floatType->type)
		{
		case FloatType::CONSTANT:
		case FloatType::PARAMETER:
			break;
		case FloatType::RANGE:
		case FloatType::CONST_RANGE:
			{
				if(node->floatType->range.min)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->range.min));
				if(node->floatType->range.max)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->range.max));
			}
			break;
		case FloatType::RAMP:
		case FloatType::LOOP_RAMP:
		case FloatType::OSCILATE_RAMP:
			{
				while(node->floatType->ramp.firstKey)
				{
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->ramp.firstKey));
				}
			}
			break;
		case FloatType::ADD:
			{
				if(node->floatType->add.value1)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->add.value1));
				if(node->floatType->add.value2)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->add.value2));
			}
			break;
		case FloatType::SUBTRACT:
			{
				if(node->floatType->subtract.value1)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->subtract.value1));
				if(node->floatType->subtract.value2)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->subtract.value2));
			}
			break;
		case FloatType::MULTIPLY:
			{
				if(node->floatType->multiply.value1)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->multiply.value1));
				if(node->floatType->multiply.value2)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->multiply.value2));
			}
			break;
		case FloatType::DIVIDE:
			{
				if(node->floatType->divide.value1)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->divide.value1));
				if(node->floatType->divide.value2)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->floatType->divide.value2));
			}
			break;
		}
	}
	else if(node->transType)
	{
		switch(node->transType->type)
		{
		case TransformType::TARGET_TRANSFORM:
		case TransformType::FILTER_EFFECT:
		case TransformType::UP:
		case TransformType::INPUT_POINT:
		case TransformType::TARGET_TRANSFORM_STR:
		case TransformType::CAMERA:
		case TransformType::CAMERA_LOOK:
			break;
		case TransformType::OFFSET:
			{
				if(node->transType->offset.offX)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offX));
				if(node->transType->offset.offY)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offY));
				if(node->transType->offset.offZ)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.offZ));
				if(node->transType->offset.baseTrans)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offset.baseTrans));
			}
			break;
		case TransformType::OFFSET_IJK:
			{
				if(node->transType->offsetIJK.offI)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offI));
				if(node->transType->offsetIJK.offJ)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offJ));
				if(node->transType->offsetIJK.offK)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.offK));
				if(node->transType->offsetIJK.baseTrans)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->offsetIJK.baseTrans));
			}
			break;
		case TransformType::ROTATE_IJK:
			{
				if(node->transType->rotateIJK.rotI)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotI));
				if(node->transType->rotateIJK.rotJ)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotJ));
				if(node->transType->rotateIJK.rotK)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.rotK));
				if(node->transType->rotateIJK.baseTrans)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->rotateIJK.baseTrans));
			}
			break;
		case TransformType::LOOK_AT_UP:
			{
				if(node->transType->lookAtUp.look)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.look));
				if(node->transType->lookAtUp.at)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.at));
				if(node->transType->lookAtUp.up)
					disconnectMathNodes(info,node,findMathNode(info->mathList,node->transType->lookAtUp.up));
			}
			break;
		}
	}
}

void editNode(MathEditInfo * info,MathNode * node)
{
	if(node->floatType)
	{
		editFloatType(info,node->floatType);
		computeNodeSize(node);
	}
	else if(node->transType)
	{
		editTransType(info,node->transType);
		computeNodeSize(node);
	}
	else if(node->rampKey)
	{
		editRampKey(node->rampKey);
		//need to resort parent
		MathNode * parent = findMathNodeParent(info,node);
		if(parent)
		{
			//remove key
			RampKey * search = parent->floatType->ramp.firstKey;
			RampKey * prev = NULL;
			while(search)
			{
				if(search == node->rampKey)
					break;
				prev = search;
				search = search->next;
			}
			if(prev)
				prev->next = node->rampKey->next;
			else
				parent->floatType->ramp.firstKey = node->rampKey->next;
			node->rampKey->next = NULL;

			//reinsert key
			search = parent->floatType->ramp.firstKey;
			prev = NULL;
			while(search)
			{
				if(search->key > node->rampKey->key)
					break;
				prev = search;
				search = search->next;
			}
			node->rampKey->next = search;
			if(prev)
				prev->next = node->rampKey;
			else
				parent->floatType->ramp.firstKey = node->rampKey;

			S32 oldX = parent->xPos;
			layoutMathlist(info->mathList, parent, parent->yPos);
			parent->xPos = oldX;
		}
	}
}

#define MATH_SAVE_FLOAT 1
#define MATH_SAVE_TRANS 2

bool testContextMenu(MathEditInfo * info,HWND hWindow, S32 xPos,S32 yPos)
{
	xPos = (S32)(xPos*info->zoomLevel)+info->sx;
	yPos = (S32)(yPos*info->zoomLevel)+info->sy;

	MathNode * node = info->mathList;
	while(node)
	{
		S32 nodeX = 0;
		S32 nodeY = 0;
		findMathNodeAbsPos(info,node,nodeX,nodeY);
		if(xPos >= nodeX && xPos <= nodeX+node->width && yPos >= nodeY && yPos <= nodeY+node->height)
		{
			POINT point;
			GetCursorPos(&point);
			HMENU menu = LoadMenu(hMainInst,MAKEINTRESOURCE(IDR_MATH_CONTEXT_RIGHT));
			HMENU hmenuTrackPopup = GetSubMenu(menu, 0); 
			int command = TrackPopupMenuEx(hmenuTrackPopup,TPM_CENTERALIGN|TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,hWindow,NULL);
			DestroyMenu(menu);
			switch(command)
			{
			case ID_DELETE:
				{
					//disconnect from parent or root
					MathNode * parent = findMathNodeParent(info,node);
					if(parent)
						disconnectMathNodes(info,parent,node);

					//make children orphans
					orphanChildren(info,node);

					//remove from orphan List
					MathNode * target = info->orphanList;
					MathNode * prev = NULL;
					while(target)
					{
						if(target == node)
						{
							if(prev)
								prev->orphanNext = node->orphanNext;
							else
								info->orphanList = node->orphanNext;
							node->orphanNext = NULL;
							break;
						}
						prev = target;
						target = target->orphanNext;
					}
					//remove from main list
					target = info->mathList;
					prev = NULL;
					while(target)
					{
						if(target == node)
						{
							if(prev)
								prev->next = node->next;
							else
								info->mathList = node->next;
							node->orphanNext = NULL;
							break;
						}
						prev = target;
						target = target->next;
					}

					//delete
					if(node->rampKey)
					{
						delete node->rampKey;
					}
					else if(node->floatType)
					{
						delete node->floatType;
					}
					else if(node->transType)
					{
						delete node->transType;
					}
					delete node;

					invalidateMathView(hWindow,info);
				}
				break;
			case ID_EDIT:
				{
					editNode(info,node);
					SetFocus(hWindow);
					invalidateMathView(hWindow,info);
				}
				break;
			case ID_RESORT:
				{
					S32 oldX = node->xPos;
					layoutMathlist(info->mathList,node,node->yPos);
					node->xPos = oldX;
					invalidateMathView(hWindow,info);
				}
				break;
			}

			return true;
		}
		node = node->next;
	}

	POINT point;
	GetCursorPos(&point);
	HMENU menu = LoadMenu(hMainInst,MAKEINTRESOURCE(IDR_MATH_STANDARD_RIGHT));
	HMENU hmenuTrackPopup = GetSubMenu(menu, 0); 
	int command = TrackPopupMenuEx(hmenuTrackPopup,TPM_CENTERALIGN|TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,hWindow,NULL);
	DestroyMenu(menu);
	switch(command)
	{
	case ID_NEWFLOAT:
		{
			MathNode * node = new MathNode;
			node->xPos = xPos;
			node->yPos = yPos;

			node->floatType = new FloatType;
			node->floatType->type = FloatType::CONSTANT;
			node->floatType->constant = 0.0f;

			node->next = info->mathList;
			info->mathList = node;

			node->orphanNext = info->orphanList;
			info->orphanList = node;

			computeNodeSize(node);
			invalidateMathView(hWindow,info);
		}
		break;
	case ID_NEWKEY:
		{
			MathNode * node = new MathNode;
			node->xPos = xPos;
			node->yPos = yPos;

			node->rampKey = new RampKey;

			node->next = info->mathList;
			info->mathList = node;

			node->orphanNext = info->orphanList;
			info->orphanList = node;

			computeNodeSize(node);
			invalidateMathView(hWindow,info);
		}
		break;
	case ID_NEWTRANS:
		{
			MathNode * node = new MathNode;
			node->xPos = xPos;
			node->yPos = yPos;

			node->transType = new TransformType;
			node->transType->type = TransformType::UP;

			node->next = info->mathList;
			info->mathList = node;

			node->orphanNext = info->orphanList;
			info->orphanList = node;

			computeNodeSize(node);
			invalidateMathView(hWindow,info);
		}
		break;
	case ID_SAVE:
		{
			char buffer[255];
			buffer[0] = 0;
			OPENFILENAME fileName;
			memset(&fileName,0,sizeof(OPENFILENAME));
			fileName.lStructSize = sizeof(OPENFILENAME);
			fileName.lpstrFilter = "Math Export\0*.math\0\0";
			fileName.nFilterIndex = 1;
			fileName.lpstrFile = buffer;
			fileName.nMaxFile = 255;
			fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			if(GetSaveFileName(&fileName))
			{
				char testBuff[256];
				strcpy(testBuff,buffer);
				U32 filelen = strlen(testBuff);
				_strupr(testBuff);
				bool bAppend = false;
				if(filelen < 4)
					bAppend = true;
				else if(strcmp(&(testBuff[filelen-5]),".MATH") != 0)
					bAppend = true;
				if(bAppend)
				{
					if(filelen+5 > 255)
					{
						strcpy(&(buffer[251]),".MATH");
					}
					else
					{
						strcat(buffer,".MATH");
					}
				}
				//save code here

				COMPTR<IFileSystem> outFile;
				DAFILEDESC fdesc = buffer;
				fdesc.lpImplementation = "UTF";
				fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
				fdesc.dwCreationDistribution = CREATE_ALWAYS;
				U32 dwWritten = 0;
				if (DACOM->CreateInstance(&fdesc, outFile) == GR_OK)
				{
					if(info->bFloatInput && info->inputFloat)
					{
						U8 saveType = MATH_SAVE_FLOAT;
						outFile->WriteFile(0,&saveType ,sizeof(U8),&dwWritten);
						PARTMAN->SaveFloatType(outFile,info->inputFloat);
					}
					else if(info->bTransInput&& info->inputTrans)
					{
						U8 saveType = MATH_SAVE_TRANS;
						outFile->WriteFile(0,&saveType ,sizeof(U8),&dwWritten);
						PARTMAN->SaveTransType(outFile,info->inputTrans);
					}
				}
			}
		}
		break;
	case ID_LOAD:
		{
			char buffer[255];
			buffer[0] = 0;
			OPENFILENAME fileName;
			memset(&fileName,0,sizeof(OPENFILENAME));
			fileName.lStructSize = sizeof(OPENFILENAME);
			fileName.lpstrFilter = "Math Export\0*.math\0\0";
			fileName.nFilterIndex = 1;
			fileName.lpstrFile = buffer;
			fileName.nMaxFile = 255;
			fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;
			if(GetOpenFileName(&fileName))
			{
				DAFILEDESC fdesc = buffer;
				COMPTR<IFileSystem> inFile;

				fdesc.lpImplementation = "UTF";
				if (DACOM->CreateInstance(&fdesc, inFile) == GR_OK)
				{	
					U8 saveType = 0;
					U32 dwWritten = 0;
					inFile->ReadFile(0,&(saveType) ,sizeof(U8),&dwWritten);
					if(dwWritten)
					{
						MathNode * node = NULL;
						if(saveType == MATH_SAVE_FLOAT)
						{
							FloatType * floatType = PARTMAN->LoadFloatType(inFile);
							createFloatMathNode(floatType,info);
							node = findMathNode(info->mathList,floatType);
						}
						else if(saveType == MATH_SAVE_TRANS)
						{
							TransformType * transType = PARTMAN->LoadTransType(inFile);
							createTransMathNode(transType,info);
							node = findMathNode(info->mathList,transType);
						}
						if(node)
						{
							node->orphanNext = info->orphanList;
							info->orphanList = node;

							layoutMathlist(info->mathList,node,0);

							node->xPos = xPos;
							node->yPos = yPos;
							invalidateMathView(hWindow,info);
						}
					}
				}
			}
		}
		break;
	}

	return false;
}

BOOL CALLBACK editMathProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hWindow,GWL_USERDATA,lParam);
			MathEditInfo * info = (MathEditInfo*)lParam;
			info->mathList = NULL;
			fillOutMathlist(info);
			if(info->inputFloat)
				layoutMathlist(info->mathList,findMathNode(info->mathList,info->inputFloat),0);
			else if(info->inputTrans)
				layoutMathlist(info->mathList,findMathNode(info->mathList,info->inputTrans),0);
		}
		break;
	case WM_CLOSE:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));

			//erase mathlist
			unmarkMathlist(info->mathList);
			if(info->inputFloat)
				markMathTree(info->mathList,info->inputFloat);
			else if(info->inputTrans)
				markMathTree(info->mathList,info->inputTrans);

			while(info->mathList)
			{
				if(!(info->mathList->bMarked))//dangling math, delete
				{
					if(info->mathList->floatType)
						delete info->mathList->floatType;
					if(info->mathList->transType)
						delete info->mathList->transType;
				}
				MathNode * tmp = info->mathList;
				info->mathList = info->mathList->next;
				delete tmp;
			}

			if(info->mathBitmap)
				DeleteObject(info->mathBitmap);
			if(info->mathDC)
				DeleteDC(info->mathDC);

			EndDialog(hWindow,0);
			return true;
		}
		break;
	case WM_HSCROLL:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			switch(LOWORD(wParam))
			{
			case SB_LEFT:
				{
				}
				break;
			case SB_RIGHT:
				{
				}
				break;
			case SB_LINELEFT:
				{
					if(info->sx> 0)
					{
						info->sx = info->sx-1;
						SetScrollPos(hWindow,SB_HORZ,info->sx,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(info->sx < (info->neededArea.right-info->viewArea.right))
					{
						info->sx = info->sx+1;
						SetScrollPos(hWindow,SB_HORZ,info->sx,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					info->sx = info->sx-info->viewArea.right;
					if(info->sx < 0)
						info->sx = 0;
					SetScrollPos(hWindow,SB_HORZ,info->sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					info->sx = info->sx+info->viewArea.right;
					if(info->sx > (info->neededArea.right-info->viewArea.right))
						info->sx = (info->neededArea.right-info->viewArea.right);
					SetScrollPos(hWindow,SB_HORZ,info->sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					info->sx = HIWORD(wParam);
					SetScrollPos(hWindow,SB_HORZ,info->sx,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_VSCROLL:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			switch(LOWORD(wParam))
			{
			case SB_LEFT:
				{
				}
				break;
			case SB_RIGHT:
				{
				}
				break;
			case SB_LINELEFT:
				{
					if(info->sy> 0)
					{
						info->sy = info->sy-1;
						SetScrollPos(hWindow,SB_VERT,info->sy,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_LINERIGHT:
				{
					if(info->sy < (info->neededArea.bottom-info->viewArea.bottom))
					{
						info->sy = info->sy+1;
						SetScrollPos(hWindow,SB_VERT,info->sy,true);
						InvalidateRect(hWindow,NULL,false);
					}
				}
				break;
			case SB_PAGELEFT:
				{
					info->sy = info->sy-info->viewArea.bottom;
					if(info->sy < 0)
						info->sy = 0;
					SetScrollPos(hWindow,SB_VERT,info->sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_PAGERIGHT:
				{
					info->sy = info->sy+info->viewArea.bottom;
					if(info->sy > (info->neededArea.bottom-info->viewArea.bottom))
						info->sy = (info->neededArea.bottom-info->viewArea.bottom);
					SetScrollPos(hWindow,SB_VERT,info->sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				{
					info->sy = HIWORD(wParam);
					SetScrollPos(hWindow,SB_VERT,info->sy,true);
					InvalidateRect(hWindow,NULL,false);
				}
				break;
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			S16 value = -(S16)(HIWORD(wParam));
			SINGLE change = (value*MF_ZOOM_SPEED);
			SINGLE newZoom = info->zoomLevel + change;
			if(newZoom > MF_ZOOM_MAX)
				newZoom  = MF_ZOOM_MAX;
			else if (newZoom < MF_ZOOM_MIN)
				newZoom = MF_ZOOM_MIN;
			change = newZoom-info->zoomLevel;
			info->zoomLevel = newZoom;

			POINT point;
			point.x = (S32)(LOWORD(lParam));
			point.y = (S32)(HIWORD(lParam));
			RECT rect;
			GetClientRect(hWindow,&rect);
			ScreenToClient(hWindow,&point);
			SINGLE horzPercent = (1.0f-(((SINGLE)(rect.right-point.x))/((SINGLE)rect.right)));
			SINGLE vertPercent = (1.0f-(((SINGLE)(rect.bottom-point.y))/((SINGLE)rect.bottom)) );

			updateMathViewRegion(hWindow,info);

			info->sx = (S32)(info->sx + (rect.right*(-change)*horzPercent));
			if(info->sx < 0 || info->neededArea.right-info->viewArea.right < 0)
				info->sx = 0;
			else if(info->sx > (info->neededArea.right-info->viewArea.right))
				info->sx = (info->neededArea.right-info->viewArea.right);
			SetScrollPos(hWindow,SB_HORZ,info->sx,true);

			info->sy = (S32)(info->sy + (rect.bottom*(-change)*vertPercent));
			if(info->sy < 0|| info->neededArea.bottom-info->viewArea.bottom < 0)
				info->sy = 0;
			else if(info->sy > (info->neededArea.bottom-info->viewArea.bottom))
				info->sy = (info->neededArea.bottom-info->viewArea.bottom);
			SetScrollPos(hWindow,SB_VERT,info->sy,true);

			InvalidateRect(hWindow,NULL,false);

			return 1;
		}
		break;
	case WM_RBUTTONDOWN:
		{
			SetFocus(hWindow);
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			testContextMenu(info,hWindow,LOWORD(lParam),HIWORD(lParam));
		}
		break;
	case WM_MBUTTONDOWN:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			SetFocus(hWindow);
			if(!info->bMoving)
			{
				info->bPanning = true;
				info->mouseX = (S32)(LOWORD(lParam));
				info->mouseY = (S32)(HIWORD(lParam));
				SetCapture(hWindow);
			}
		}
		break;
	case WM_MBUTTONUP:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			if(info->bPanning)
			{
				info->bPanning = false;
				ReleaseCapture();
			}
		}
		break;
	case WM_LBUTTONDBLCLK:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			SetFocus(hWindow);
			S32 newX = (S32)(LOWORD(lParam)*info->zoomLevel)+info->sx;
			S32 newY = (S32)(HIWORD(lParam)*info->zoomLevel)+info->sy;
			MathNode * node = hitTestFindNode(info,newX,newY);
			if(node)
			{
				editNode(info,node);
				SetFocus(hWindow);
				invalidateMathView(hWindow,info);
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			SetFocus(hWindow);
			if(mathHitTest(info,LOWORD(lParam),HIWORD(lParam)))
			{
				invalidateMathView(hWindow,info);
				SetCapture(hWindow);
			}
		}
		break;
	case WM_LBUTTONUP:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			if(info->bMoving)
			{
				S32 newX = (S32)(LOWORD(lParam)*info->zoomLevel)+info->sx;
				S32 newY = (S32)(HIWORD(lParam)*info->zoomLevel)+info->sy;
				if(newX != info->mouseX && newY != info->mouseY)
				{
					if(info->selectedMath)
					{
						info->selectedMath->xPos += (newX-info->mouseX);
						info->selectedMath->yPos += (newY-info->mouseY);
					}
					info->mouseX = newX;
					info->mouseY = newY;
					invalidateMathView(hWindow,info);
				}
				info->bMoving = false;
				ReleaseCapture();
			}
			else if(info->bConnecting)
			{
				S32 newX = (S32)(LOWORD(lParam)*info->zoomLevel)+info->sx;
				S32 newY = (S32)(HIWORD(lParam)*info->zoomLevel)+info->sy;
				MathNode * target = hitTestFindNode(info,newX,newY);
				if(target)
				{
					if(!(info->connectNode))
					{
						if((target->floatType && info->bFloatInput) ||(target->transType && info->bTransInput))
						{
							//disconnect from parent
							MathNode * oldParent = findMathNodeParent(info,target);
							if(oldParent)
								disconnectMathNodes(info,oldParent,target);
							//remove from orphan list 

							MathNode * search = info->orphanList;
							MathNode * prev = NULL;
							while(search)
							{
								if(search == target)
								{
									if(prev)
										prev->orphanNext = search->orphanNext;
									else
										info->orphanList = search->orphanNext;
									search->orphanNext = NULL;
									break;
								}
								prev = search;
								search = search->orphanNext;
							}

							if(info->bFloatInput)
								info->inputFloat = target->floatType;
							else if(info->bTransInput)
								info->inputTrans = target->transType;

							target->xPos = target->xPos-info->rootX;
							target->yPos = target->yPos-info->rootY;
						}
					}
					else if((target != info->connectNode) && ((target->floatType && info->connectFloat) || 
						(target->transType && info->connectTrans) || 
						(target->rampKey && info->connectKey)))
					{
						//disconnect from parent
						MathNode * oldParent = findMathNodeParent(info,target);
						if(oldParent)
							disconnectMathNodes(info,oldParent,target);

						//remove from orphan list 

						MathNode * search = info->orphanList;
						MathNode * prev = NULL;
						while(search)
						{
							if(search == target)
							{
								if(prev)
									prev->orphanNext = search->orphanNext;
								else
									info->orphanList = search->orphanNext;
								search->orphanNext = NULL;
								break;
							}
							prev = search;
							search = search->orphanNext;
						}

						//link to parent
						if(info->connectFloat)
							*(info->connectFloat) = target->floatType;
						else if(info->connectTrans)
							*(info->connectTrans) = target->transType;
						else if(info->connectKey)
						{
							RampKey * search = info->connectNode->floatType->ramp.firstKey;
							RampKey * prev = NULL;
							while(search)
							{
								if(search->key > target->rampKey->key)
									break;
								prev = search;
								search = search->next;
							}
							target->rampKey->next = search;
							if(prev)
								prev->next = target->rampKey;
							else
								info->connectNode->floatType->ramp.firstKey = target->rampKey;
						}



						//set window offset
						if(info->connectKey)
						{
							//we need to resort a rampkey
							computeNodeSize(info->connectNode);
							S32 oldX = info->connectNode->xPos;
							layoutMathlist(info->mathList, info->connectNode, info->connectNode->yPos);
							info->connectNode->xPos = oldX;
						}
						else
						{
							S32 parentX = 0;
							S32 parentY = 0;
							findMathNodeAbsPos(info,info->connectNode,parentX,parentY);
							target->xPos = target->xPos-parentX;
							target->yPos = target->yPos-parentY;
						}
					}
				}
				invalidateMathView(hWindow,info);
				info->bConnecting = false;
				ReleaseCapture();
			}
		}
		break;
	case WM_MOUSEMOVE:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			if(info->bMoving)
			{
				S32 newX = (S32)(LOWORD(lParam)*info->zoomLevel)+info->sx;
				S32 newY = (S32)(HIWORD(lParam)*info->zoomLevel)+info->sy;
				if(newX != info->mouseX || newY != info->mouseY)
				{
					if(info->selectedMath)
					{
						info->selectedMath->xPos += (newX-info->mouseX);
						info->selectedMath->yPos += (newY-info->mouseY);
					}
					info->mouseX = newX;
					info->mouseY = newY;
					invalidateMathView(hWindow,info);
				}
			}
			else if(info->bPanning)
			{
				S32 newX = (S32)(LOWORD(lParam));
				S32 newY = (S32)(HIWORD(lParam));
				if(newX != info->mouseX || newY != info->mouseY)
				{
					info->sx -= newX-info->mouseX;
					if(info->sx < 0)
						info->sx = 0;
					if(info->sx > (info->neededArea.right-info->viewArea.right))
						info->sx = (info->neededArea.right-info->viewArea.right);
					SetScrollPos(hWindow,SB_HORZ,info->sx,true);

					info->sy -= newY-info->mouseY;
					if(info->sy < 0)
						info->sy = 0;
					if(info->sy > (info->neededArea.bottom-info->viewArea.bottom))
						info->sy = (info->neededArea.bottom-info->viewArea.bottom);
					SetScrollPos(hWindow,SB_VERT,info->sy,true);
					info->mouseX = newX;
					info->mouseY = newY;
					InvalidateRect(hWindow,NULL,false);
				}
			}
			else if(info->bConnecting)
			{
				S32 newX = (S32)(LOWORD(lParam)*info->zoomLevel)+info->sx;
				S32 newY = (S32)(HIWORD(lParam)*info->zoomLevel)+info->sy;
				if(newX != info->mouseX || newY != info->mouseY)
				{
					info->mouseX = newX;
					info->mouseY = newY;
					invalidateMathView(hWindow,info);
				}
			}
		}
		break;
	case WM_PAINT:
		{
			MathEditInfo * info = (MathEditInfo*)(GetWindowLong(hWindow,GWL_USERDATA));
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWindow,&ps);
			
			updateMathViewRegion(hWindow,info);

			if(info->bAreaInvalid)
			{
				allocGraphDC(hdc,info);
				HBRUSH brush = (HBRUSH)(GetStockObject(DKGRAY_BRUSH));
				RECT rect;
				rect.top = 0;
				rect.bottom = MATH_AREA_SIZE;
				rect.left = 0;
				rect.right = MATH_AREA_SIZE;
				FillRect(info->mathDC,&rect,brush);

				drawMath(info);
				info->bAreaInvalid = false;
			}

			RECT clientRect;
			GetClientRect(hWindow,&clientRect);
			StretchBlt(hdc,0,0,clientRect.right,clientRect.bottom,info->mathDC,info->sx,info->sy,(int)(clientRect.right*info->zoomLevel),(int)(clientRect.bottom*info->zoomLevel),SRCCOPY);
			EndPaint(hWindow,&ps);

			return true;
		}
		break;
	}
	return false;
}

FloatType * fixNullRef(FloatType * input);
FloatType * MakeDefaultFloat(SINGLE value);
TransformType * MakeDefaultTrans();

RampKey * fixNullRef(RampKey * input)
{
	if(input)
	{
		input->value = fixNullRef(input->value);
		return input;
	}
	return NULL;
};

FloatType * fixNullRef(FloatType * input)
{
	if(input)
	{
		switch(input->type)
		{
		case FloatType::CONSTANT:
		case FloatType::PARAMETER:
			break;
		case FloatType::RANGE:
		case FloatType::CONST_RANGE:
			{
				input->range.min = fixNullRef(input->range.min);
				input->range.max = fixNullRef(input->range.max);
			}
			break;
		case FloatType::RAMP:
		case FloatType::LOOP_RAMP:
		case FloatType::OSCILATE_RAMP:
			{
				RampKey * search = input->ramp.firstKey;
				while(search)
				{
					fixNullRef(search);
					search = search->next;
				}
			}
			break;
		case FloatType::ADD:
			{
				input->add.value1 = fixNullRef(input->add.value1);
				input->add.value2 = fixNullRef(input->add.value2);
			}
			break;
		case FloatType::SUBTRACT:
			{
				input->subtract.value1 = fixNullRef(input->subtract.value1);
				input->subtract.value2 = fixNullRef(input->subtract.value2);
			}
			break;
		case FloatType::MULTIPLY:
			{
				input->multiply.value1 = fixNullRef(input->multiply.value1);
				input->multiply.value2 = fixNullRef(input->multiply.value2);
			}
			break;
		case FloatType::DIVIDE:
			{
				input->divide.value1 = fixNullRef(input->divide.value1);
				input->divide.value2 = fixNullRef(input->divide.value2);
			}
			break;
		}
		return input;
	}
	input = MakeDefaultFloat(0);
	return input;
};

TransformType * fixNullRef(TransformType * input)
{
	if(input)
	{
		switch(input->type)
		{
		case TransformType::TARGET_TRANSFORM:
		case TransformType::FILTER_EFFECT:
		case TransformType::UP:
		case TransformType::INPUT_POINT:
		case TransformType::TARGET_TRANSFORM_STR:
		case TransformType::CAMERA:
		case TransformType::CAMERA_LOOK:
			break;
		case TransformType::OFFSET:
			{
				input->offset.offX = fixNullRef(input->offset.offX);
				input->offset.offY = fixNullRef(input->offset.offY);
				input->offset.offZ = fixNullRef(input->offset.offZ);
				input->offset.baseTrans = fixNullRef(input->offset.baseTrans);
			}
			break;
		case TransformType::OFFSET_IJK:
			{
				input->offsetIJK.offI = fixNullRef(input->offsetIJK.offI);
				input->offsetIJK.offJ = fixNullRef(input->offsetIJK.offJ);
				input->offsetIJK.offK = fixNullRef(input->offsetIJK.offK);
				input->offsetIJK.baseTrans = fixNullRef(input->offsetIJK.baseTrans);
			}
			break;
		case TransformType::ROTATE_IJK:
			{
				input->rotateIJK.rotI = fixNullRef(input->rotateIJK.rotI);
				input->rotateIJK.rotJ = fixNullRef(input->rotateIJK.rotJ);
				input->rotateIJK.rotK = fixNullRef(input->rotateIJK.rotK);
				input->rotateIJK.baseTrans = fixNullRef(input->rotateIJK.baseTrans);
			}
			break;
		case TransformType::LOOK_AT_UP:
			{
				input->lookAtUp.look = fixNullRef(input->lookAtUp.look);
				input->lookAtUp.at = fixNullRef(input->lookAtUp.at);
				input->lookAtUp.up = fixNullRef(input->lookAtUp.up);
			}
			break;
		}
		return input;
	}
	input = MakeDefaultTrans();
	return input;
};

FloatType * MathLayoutWin::EditFloat(FloatType * input)
{
	MathEditInfo info;
	info.inputFloat = input;
	info.inputTrans = NULL;
	info.bFloatInput = true;
	DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_MATH_LAYOUT),mainWindow,editMathProc,(DWORD)(&info));

	info.inputFloat = fixNullRef(info.inputFloat);
	return info.inputFloat;
}

TransformType * MathLayoutWin::EditTrans(TransformType * input)
{
	MathEditInfo info;
	info.inputFloat = NULL;
	info.inputTrans = input;
	info.bTransInput = true;
	DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_MATH_LAYOUT),mainWindow,editMathProc,(DWORD)(&info));

	info.inputTrans = fixNullRef(info.inputTrans);

	return info.inputTrans;
}

///////////////////////////////////////////////////////////////////////////
//Edit Float Dialog
BOOL CALLBACK editFloatProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			FloatEditInfo * fInfo = (FloatEditInfo *)(lParam);
			FloatType * floatType = fInfo->floatType;
			SetWindowLong(hWindow,GWL_USERDATA,lParam);

			HWND typeCombo = GetDlgItem(hWindow,IDC_TYPE_COMBO);
			U32 index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Constant");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::CONSTANT);
			if(floatType->type == FloatType::CONSTANT)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Range");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::RANGE);
			if(floatType->type == FloatType::RANGE)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Parameter");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::PARAMETER);
			if(floatType->type == FloatType::PARAMETER)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Ramp");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::RAMP);
			if(floatType->type == FloatType::RAMP)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Add");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::ADD);
			if(floatType->type == FloatType::ADD)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Subtract");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::SUBTRACT);
			if(floatType->type == FloatType::SUBTRACT)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Multiply");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::MULTIPLY);
			if(floatType->type == FloatType::MULTIPLY)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Divide");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::DIVIDE);
			if(floatType->type == FloatType::DIVIDE)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Const Range");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::CONST_RANGE);
			if(floatType->type == FloatType::CONST_RANGE)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Looping Ramp");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::LOOP_RAMP);
			if(floatType->type == FloatType::LOOP_RAMP)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Oscilating Ramp");
			SendMessage(typeCombo,CB_SETITEMDATA,index,FloatType::OSCILATE_RAMP);
			if(floatType->type == FloatType::OSCILATE_RAMP)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			HWND editValue = GetDlgItem(hWindow,IDC_EDIT_VALUE1);
			HWND editValueStatic = GetDlgItem(hWindow,IDC_VALUE1);
			HWND targetCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);

			switch(floatType->type)
			{
			case FloatType::CONSTANT:
				{
					EnableWindow(editValue,true);
					EnableWindow(editValueStatic,true);
					EnableWindow(targetCombo,false);
					SetWindowText(editValueStatic,"Constant");
					char buffer[256];
					sprintf(buffer,"%f",floatType->constant);
					SetWindowText(editValue,buffer);
				}
				break;
			case FloatType::CONST_RANGE:
			case FloatType::RANGE:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,false);
				}
				break;
			case FloatType::PARAMETER:
				{
					EnableWindow(editValue,true);
					EnableWindow(editValueStatic,true);
					EnableWindow(targetCombo,true);

					U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Global"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,-1);
					if(floatType->parameter.targetID == -1)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
					while(targ)
					{
						U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
						SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
						if(floatType->parameter.targetID == targ->GetTargetID())
							SendMessage(targetCombo,CB_SETCURSEL,index,0);
						targ = targ->GetNextTarget();
					}

					SetWindowText(editValueStatic,"Parameter Name");
					SetWindowText(editValue,floatType->parameter.name);
				}
				break;
			case FloatType::LOOP_RAMP:
			case FloatType::OSCILATE_RAMP:
			case FloatType::RAMP:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,true);

					U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Lifetime"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::LIFETIME);
					if(floatType->ramp.type == FloatType::LIFETIME)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Speed"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::SPEED);
					if(floatType->ramp.type == FloatType::SPEED)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Life Percent"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::LIFETIME_PERCENT);
					if(floatType->ramp.type == FloatType::LIFETIME_PERCENT)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Effect Lifetime"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::EFFECT_LIFETIME);
					if(floatType->ramp.type == FloatType::EFFECT_LIFETIME)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);

				}
				break;
			case FloatType::ADD:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,false);
				}
				break;
			case FloatType::SUBTRACT:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,false);
				}
				break;
			case FloatType::MULTIPLY:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,false);
				}
				break;
			case FloatType::DIVIDE:
				{
					EnableWindow(editValue,false);
					EnableWindow(editValueStatic,false);
					EnableWindow(targetCombo,false);
				}
				break;

			}
		}
		break;
	case WM_COMMAND:
		{
			FloatEditInfo * fInfo = (FloatEditInfo *)(GetWindowLong(hWindow,GWL_USERDATA));
			FloatType * floatType = fInfo->floatType;

			if(LOWORD(wParam) == IDOK)
			{
				EndDialog(hWindow,GetWindowLong(hWindow,GWL_USERDATA));
			}
			else if(LOWORD(wParam) == IDC_EDIT_VALUE1)
			{
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					{
						if(floatType->type == FloatType::CONSTANT)
						{
							HWND editValue = GetDlgItem(hWindow,IDC_EDIT_VALUE1);
							char buffer[256];
							GetWindowText(editValue,buffer,255);
							buffer[255] = 0;
							floatType->constant = (SINGLE)(atof(buffer));
						}
						else if(floatType->type == FloatType::PARAMETER)
						{
							HWND editValue = GetDlgItem(hWindow,IDC_EDIT_VALUE1);
							GetWindowText(editValue,floatType->parameter.name,31);
							floatType->parameter.name[31] = 0;
						}
					}
					break;
				}
			}
			else if(LOWORD(wParam) == IDC_TARGET_COMBO)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						HWND typeCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);
						U32 index = SendMessage(typeCombo,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							if(floatType->type == FloatType::PARAMETER)
							{
								floatType->parameter.targetID = SendMessage(typeCombo,CB_GETITEMDATA,index,0);
							}
							else if(floatType->type == FloatType::RAMP || floatType->type == FloatType::LOOP_RAMP || floatType->type == FloatType::OSCILATE_RAMP)
							{
								floatType->ramp.type = (FloatType::RampType)(SendMessage(typeCombo,CB_GETITEMDATA,index,0));
							}
						}
					}
					break;
				}
			}
			else if(LOWORD(wParam) == IDC_TYPE_COMBO)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						HWND typeCombo = GetDlgItem(hWindow,IDC_TYPE_COMBO);
						U32 index = SendMessage(typeCombo,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							FloatType::Type newType = (FloatType::Type)(SendMessage(typeCombo,CB_GETITEMDATA,index,0));
							if(newType != floatType->type)
							{
								HWND editValue = GetDlgItem(hWindow,IDC_EDIT_VALUE1);
								HWND editValueStatic = GetDlgItem(hWindow,IDC_VALUE1);
								HWND targetCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);
								bool bKeepParams = ((floatType->type == FloatType::CONST_RANGE) ||
									(floatType->type == FloatType::RANGE) ||
									(floatType->type == FloatType::ADD) ||
									(floatType->type == FloatType::SUBTRACT) ||
									(floatType->type == FloatType::MULTIPLY) ||
									(floatType->type == FloatType::DIVIDE)) &&
									((newType == FloatType::CONST_RANGE) ||
									(newType == FloatType::RANGE) ||
									(newType == FloatType::ADD) ||
									(newType == FloatType::SUBTRACT) ||
									(newType == FloatType::MULTIPLY) ||
									(newType == FloatType::DIVIDE));
								bool bKeepKeys = ((floatType->type == FloatType::LOOP_RAMP) ||
									(floatType->type == FloatType::OSCILATE_RAMP) ||
									(floatType->type == FloatType::RAMP)) &&
									((newType == FloatType::LOOP_RAMP) ||
									(newType == FloatType::OSCILATE_RAMP) ||
									(newType == FloatType::RAMP));
							
								//need to orphan children here
								if(!(bKeepParams || bKeepKeys))
									orphanChildren(fInfo->info,findMathNode(fInfo->info->mathList,floatType));

								floatType->type = newType;

								switch(floatType->type)
								{
								case FloatType::CONSTANT:
									{
										floatType->constant = 0;
										EnableWindow(editValue,true);
										EnableWindow(editValueStatic,true);
										EnableWindow(targetCombo,false);
										SetWindowText(editValueStatic,"Constant");
										char buffer[256];
										sprintf(buffer,"%f",floatType->constant);
										SetWindowText(editValue,buffer);
									}
									break;
								case FloatType::CONST_RANGE:
								case FloatType::RANGE:
									{
										if(!bKeepParams)
										{
											floatType->range.min = NULL;
											floatType->range.max = NULL;
										}

										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,false);
									}
									break;
								case FloatType::PARAMETER:
									{
										floatType->parameter.targetID = 0;
										floatType->parameter.name[0] = 0;

										EnableWindow(editValue,true);
										EnableWindow(editValueStatic,true);
										EnableWindow(targetCombo,true);

										SendMessage(targetCombo,CB_RESETCONTENT,0,0);

										U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Global"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,-1);
										if(floatType->parameter.targetID == -1)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
										while(targ)
										{
											U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
											SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
											if(floatType->parameter.targetID == targ->GetTargetID())
												SendMessage(targetCombo,CB_SETCURSEL,index,0);
											targ = targ->GetNextTarget();
										}

										SetWindowText(editValueStatic,"Parameter Name");
										SetWindowText(editValue,floatType->parameter.name);
									}
									break;
								case FloatType::LOOP_RAMP:
								case FloatType::OSCILATE_RAMP:
								case FloatType::RAMP:
									{
										if(!bKeepKeys)
										{
											floatType->ramp.type = FloatType::LIFETIME_PERCENT;
											floatType->ramp.firstKey = NULL;
										}

										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,true);

										SendMessage(targetCombo,CB_RESETCONTENT,0,0);

										U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Lifetime"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::LIFETIME);
										if(floatType->ramp.type == FloatType::LIFETIME)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Speed"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::SPEED);
										if(floatType->ramp.type == FloatType::SPEED)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Life Percent"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::LIFETIME_PERCENT);
										if(floatType->ramp.type == FloatType::LIFETIME_PERCENT)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("Effect Lifetime"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,FloatType::EFFECT_LIFETIME);
										if(floatType->ramp.type == FloatType::EFFECT_LIFETIME)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										
									}
									break;
								case FloatType::ADD:
									{
										if(!bKeepParams)
										{
											floatType->add.value1 = NULL;
											floatType->add.value2 = NULL;
										}
										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,false);
									}
									break;
								case FloatType::SUBTRACT:
									{
										if(!bKeepParams)
										{
											floatType->subtract.value1 = NULL;
											floatType->subtract.value2 = NULL;
										}
										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,false);
									}
									break;
								case FloatType::MULTIPLY:
									{
										if(!bKeepParams)
										{
											floatType->multiply.value1 = NULL;
											floatType->multiply.value2 = NULL;
										}
										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,false);
									}
									break;
								case FloatType::DIVIDE:
									{
										if(!bKeepParams)
										{
											floatType->divide.value1 = NULL;
											floatType->divide.value2 = NULL;
										}
										EnableWindow(editValue,false);
										EnableWindow(editValueStatic,false);
										EnableWindow(targetCombo,false);
									}
									break;
								}
							}
						}
					}
					break;
				}
			}
		}
		break;
	}
	return false;
}

FloatType * editFloatType(MathEditInfo * info, FloatType * floatType)
{
	FloatEditInfo fInfo;
	fInfo.floatType = floatType;
	fInfo.info = info;
	return (FloatType *)(DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_FLOAT_EDITOR),mainWindow,editFloatProc,(DWORD)(&fInfo)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//Edit ramp key
BOOL CALLBACK editRampKeyProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hWindow,GWL_USERDATA,lParam);
			RampKey * key = (RampKey *)lParam;

			HWND editWin = GetDlgItem(hWindow,IDC_KEY_EDIT);
			char buffer[256];
			sprintf(buffer,"%f",key->key);
			SetWindowText(editWin,buffer);
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDOK)
			{
				char buffer[256];
				HWND editWin = GetDlgItem(hWindow,IDC_KEY_EDIT);
				GetWindowText(editWin,buffer,255);
				buffer[255] = 0;
				RampKey * key = (RampKey*)GetWindowLong(hWindow,GWL_USERDATA);
				key->key = (SINGLE)(atof(buffer));
				EndDialog(hWindow,0);
			}

		}
		break;
	}
	return false;
}

void editRampKey(RampKey * key)
{
	DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_EDIT_RAMP_KEY),mainWindow,editRampKeyProc,(DWORD)key);
}

////////////////////////////////////////////////////////////////////
//Edit transform type
BOOL CALLBACK editTransProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		{
			TransEditInfo * tInfo = (TransEditInfo *)lParam;
			TransformType * transType = tInfo->transType;
			SetWindowLong(hWindow,GWL_USERDATA,lParam);

			HWND typeCombo = GetDlgItem(hWindow,IDC_TYPE_COMBO);
			U32 index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Old Style Target");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::TARGET_TRANSFORM);
			if(transType->type == TransformType::TARGET_TRANSFORM)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);
			
			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Filter");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::FILTER_EFFECT);
			if(transType->type == TransformType::FILTER_EFFECT)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Up");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::UP);
			if(transType->type == TransformType::UP)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Camera");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::CAMERA);
			if(transType->type == TransformType::CAMERA)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Camera Look");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::CAMERA_LOOK);
			if(transType->type == TransformType::CAMERA_LOOK)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Offset");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::OFFSET);
			if(transType->type == TransformType::OFFSET)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Input Point");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::INPUT_POINT);
			if(transType->type == TransformType::INPUT_POINT)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Target");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::TARGET_TRANSFORM_STR);
			if(transType->type == TransformType::TARGET_TRANSFORM_STR)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Offset IJK");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::OFFSET_IJK);
			if(transType->type == TransformType::OFFSET_IJK)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Rotate IJK");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::ROTATE_IJK);
			if(transType->type == TransformType::ROTATE_IJK)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			index = SendMessage(typeCombo,CB_ADDSTRING,0,(DWORD)"Look At Up");
			SendMessage(typeCombo,CB_SETITEMDATA,index,TransformType::LOOK_AT_UP);
			if(transType->type == TransformType::LOOK_AT_UP)
				SendMessage(typeCombo,CB_SETCURSEL,index,0);

			HWND targetCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);
			HWND hpCombo = GetDlgItem(hWindow,IDC_HARDPOINT_COMBO);
			HWND filterEdit = GetDlgItem(hWindow,IDC_FILTER_EDIT);

			switch(transType->type)
			{
			case TransformType::TARGET_TRANSFORM:
				{
					EnableWindow(targetCombo,true);
					EnableWindow(hpCombo,true);
					EnableWindow(filterEdit,false);

					U32 hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,-1);
					SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);

					U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,0);
					if(transType->targetTrans.targetID == 0)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
					while(targ)
					{
						index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
						SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
						if(transType->targetTrans.targetID == targ->GetTargetID())
						{
							SendMessage(targetCombo,CB_SETCURSEL,index,0);

							ITargetHp * hpTarg = targ->GetFirstHardpoint();
							while(hpTarg)
							{
								hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hpTarg->GetName()));
								SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,targ->GetHardPointIndex(hpTarg));
								if(transType->targetTrans.hpID == targ->GetHardPointIndex(hpTarg))
									SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);
		
								hpTarg = hpTarg->GetNextHP();
							}
						}
						targ = targ->GetNextTarget();
					}
				}
				break;
			case TransformType::TARGET_TRANSFORM_STR:
				{
					EnableWindow(targetCombo,true);
					EnableWindow(hpCombo,true);
					EnableWindow(filterEdit,false);

					U32 hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,0);
					SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);

					U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("None"));
					SendMessage(targetCombo,CB_SETITEMDATA,index,0);
					if(transType->targetTrans.targetID == 0)
						SendMessage(targetCombo,CB_SETCURSEL,index,0);
					IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
					while(targ)
					{
						index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
						SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
						if(transType->targetTrans.targetID == targ->GetTargetID())
						{
							SendMessage(targetCombo,CB_SETCURSEL,index,0);

							ITargetHp * hpTarg = targ->GetFirstHardpoint();
							while(hpTarg)
							{
								hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hpTarg->GetName()));
								SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,(DWORD)hpTarg);
								if(strcmp(transType->targetTrans_str.hpName,hpTarg->GetName()) == 0)
									SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);
		
								hpTarg = hpTarg->GetNextHP();
							}
						}
						targ = targ->GetNextTarget();
					}
				}
				break;
			case TransformType::FILTER_EFFECT:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,true);

					SetWindowText(GetDlgItem(hWindow,IDC_STATIC_FILTER),"Filter Name");
					SetWindowText(filterEdit,transType->filterName);
				}
				break;
			case TransformType::UP:
			case TransformType::CAMERA:
			case TransformType::INPUT_POINT:
			case TransformType::CAMERA_LOOK:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,false);
				}
				break;
			case TransformType::OFFSET:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,false);
				}
				break;
			case TransformType::OFFSET_IJK:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,false);
				}
				break;
			case TransformType::ROTATE_IJK:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,false);
				}
				break;
			case TransformType::LOOK_AT_UP:
				{
					EnableWindow(targetCombo,false);
					EnableWindow(hpCombo,false);
					EnableWindow(filterEdit,false);
				}
				break;
			}
		}
		break;
	case WM_COMMAND:
		{
			TransEditInfo * tInfo = (TransEditInfo *)(GetWindowLong(hWindow,GWL_USERDATA));
			TransformType * transType = tInfo->transType;
			if(LOWORD(wParam) == IDOK)
			{
				EndDialog(hWindow,(DWORD)(tInfo->transType));
			}
			else if(LOWORD(wParam) == IDC_FILTER_EDIT)
			{
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					{
						HWND filterEdit = GetDlgItem(hWindow,IDC_FILTER_EDIT);
						if(transType->type == TransformType::TARGET_TRANSFORM_STR)
						{
						}
						else//FILTER_EFFECT
						{
							GetWindowText(filterEdit,transType->filterName,31);
							transType->filterName[31] = 0;
						}
					}
					break;
				}
			}
			else if(LOWORD(wParam) == IDC_TARGET_COMBO)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						HWND targetCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);
						U32 index = SendMessage(targetCombo,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							transType->targetTrans.targetID = SendMessage(targetCombo,CB_GETITEMDATA,index,0);
							transType->targetTrans.hpID = -1;
							HWND hpCombo = GetDlgItem(hWindow,IDC_HARDPOINT_COMBO);
							SendMessage(hpCombo,CB_RESETCONTENT,0,0);
							U32 hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
							if(transType->type == TransformType::TARGET_TRANSFORM)
								SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,-1);
							else//TARGET_TRANSFORM_STR
								SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,0);
							SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);

							IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
							while(targ)
							{
								if(transType->targetTrans.targetID == targ->GetTargetID())
								{
									ITargetHp * hpTarg = targ->GetFirstHardpoint();
									while(hpTarg)
									{
										hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hpTarg->GetName()));
										if(transType->type == TransformType::TARGET_TRANSFORM)
											SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,targ->GetHardPointIndex(hpTarg));			
										else//TARGET_TRANSFORM_STR
											SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,(DWORD)hpTarg);			

										hpTarg = hpTarg->GetNextHP();
									}
								}
								targ = targ->GetNextTarget();
							}
						}
					}
					break;
				}
			}
			else if(LOWORD(wParam) == IDC_HARDPOINT_COMBO)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						HWND targetCombo = GetDlgItem(hWindow,IDC_HARDPOINT_COMBO);
						U32 index = SendMessage(targetCombo,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							if(transType->type == TransformType::TARGET_TRANSFORM)
								transType->targetTrans.hpID = SendMessage(targetCombo,CB_GETITEMDATA,index,0);
							else
							{
								ITargetHp * hpTarg = (ITargetHp *)(SendMessage(targetCombo,CB_GETITEMDATA,index,0));
								if(hpTarg)
									strcpy(transType->targetTrans_str.hpName,hpTarg->GetName());
								else
									transType->targetTrans_str.hpName[0] = 0;
							}
						}
					}
					break;
				}
			}
			else if(LOWORD(wParam) == IDC_TYPE_COMBO)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					{
						HWND typeCombo = GetDlgItem(hWindow,IDC_TYPE_COMBO);
						U32 index = SendMessage(typeCombo,CB_GETCURSEL,0,0);
						if(index != -1)
						{
							TransformType::Type newType = (TransformType::Type)(SendMessage(typeCombo,CB_GETITEMDATA,index,0));
							if(newType != transType->type)
							{
								HWND targetCombo = GetDlgItem(hWindow,IDC_TARGET_COMBO);
								HWND hpCombo = GetDlgItem(hWindow,IDC_HARDPOINT_COMBO);
								HWND filterEdit = GetDlgItem(hWindow,IDC_FILTER_EDIT);

								orphanChildren(tInfo->info,findMathNode(tInfo->info->mathList,transType));

								transType->type = newType;
								switch(transType->type)
								{
								case TransformType::TARGET_TRANSFORM:
									{
										EnableWindow(targetCombo,true);
										EnableWindow(hpCombo,true);
										EnableWindow(filterEdit,false);

										SendMessage(targetCombo,CB_RESETCONTENT,0,0);
										SendMessage(hpCombo,CB_RESETCONTENT,0,0);

										U32 hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,-1);
										SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);

										U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,0);
										if(transType->targetTrans.targetID == 0)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
										while(targ)
										{
											index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
											SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
											if(transType->targetTrans.targetID == targ->GetTargetID())
											{
												SendMessage(targetCombo,CB_SETCURSEL,index,0);

												ITargetHp * hpTarg = targ->GetFirstHardpoint();
												while(hpTarg)
												{
													hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hpTarg->GetName()));
													SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,targ->GetHardPointIndex(hpTarg));
													if(transType->targetTrans.hpID == targ->GetHardPointIndex(hpTarg))
														SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);
							
													hpTarg = hpTarg->GetNextHP();
												}
											}
											targ = targ->GetNextTarget();
										}
									}
									break;
								case TransformType::TARGET_TRANSFORM_STR:
									{
										EnableWindow(targetCombo,true);
										EnableWindow(hpCombo,true);
										EnableWindow(filterEdit,false);

										SendMessage(targetCombo,CB_RESETCONTENT,0,0);
										SendMessage(hpCombo,CB_RESETCONTENT,0,0);

										U32 hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,0);
										SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);

										transType->targetTrans_str.targetID = 0;
										transType->targetTrans_str.hpName[0] = 0;

										U32 index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)("None"));
										SendMessage(targetCombo,CB_SETITEMDATA,index,0);
										if(transType->targetTrans_str.targetID == 0)
											SendMessage(targetCombo,CB_SETCURSEL,index,0);
										IEffectTarget * targ = EFFECTFILE->GetFirstTarget();
										while(targ)
										{
											index = SendMessage(targetCombo,CB_ADDSTRING,0,(DWORD)(targ->GetName()));
											SendMessage(targetCombo,CB_SETITEMDATA,index,targ->GetTargetID());
											if(transType->targetTrans_str.targetID == targ->GetTargetID())
											{
												SendMessage(targetCombo,CB_SETCURSEL,index,0);

												ITargetHp * hpTarg = targ->GetFirstHardpoint();
												while(hpTarg)
												{
													hpIndex = SendMessage(hpCombo,CB_ADDSTRING,0,(DWORD)(hpTarg->GetName()));
													SendMessage(hpCombo,CB_SETITEMDATA,hpIndex,(DWORD)(hpTarg));
													if(strcmp(transType->targetTrans_str.hpName,hpTarg->GetName()) == 0)
														SendMessage(hpCombo,CB_SETCURSEL,hpIndex,0);
							
													hpTarg = hpTarg->GetNextHP();
												}
											}
											targ = targ->GetNextTarget();
										}
									}
									break;
								case TransformType::FILTER_EFFECT:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,true);

										SetWindowText(GetDlgItem(hWindow,IDC_STATIC_FILTER),"Filter Name");

										transType->filterName[0] = 0;
										SetWindowText(filterEdit,transType->filterName);
									}
									break;
								case TransformType::UP:
								case TransformType::CAMERA:
								case TransformType::CAMERA_LOOK:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,false);
									}
									break;
								case TransformType::OFFSET:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,false);

										transType->offset.offX = NULL;
										transType->offset.offY = NULL;
										transType->offset.offZ = NULL;
										transType->offset.baseTrans = NULL;
									}
									break;
								case TransformType::OFFSET_IJK:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,false);

										transType->offsetIJK.offI = NULL;
										transType->offsetIJK.offJ = NULL;
										transType->offsetIJK.offK = NULL;
										transType->offsetIJK.baseTrans = NULL;
									}
									break;
								case TransformType::ROTATE_IJK:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,false);

										transType->rotateIJK.rotI = NULL;
										transType->rotateIJK.rotJ = NULL;
										transType->rotateIJK.rotK = NULL;
										transType->rotateIJK.baseTrans = NULL;
									}
									break;
								case TransformType::LOOK_AT_UP:
									{
										EnableWindow(targetCombo,false);
										EnableWindow(hpCombo,false);
										EnableWindow(filterEdit,false);

										transType->lookAtUp.look = NULL;
										transType->lookAtUp.at = NULL;
										transType->lookAtUp.up = NULL;
									}
									break;
								}
							}
						}
					}
					break;
				}
			}
		}
		break;
	}
	return false;
}

TransformType * editTransType(MathEditInfo * info,TransformType * transType)
{
	TransEditInfo tInfo;
	tInfo.transType = transType;
	tInfo.info = info;
	return (TransformType *)(DialogBoxParam(hMainInst,MAKEINTRESOURCE(IDD_EDIT_TRANS),mainWindow,editTransProc,(DWORD)(&tInfo)));
}
