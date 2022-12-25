// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include <vector>

#include "Hardpoints.h"
#include "Engine.h"
#include "TSmartPointer.h"
#include "IHardPoint.h"
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "DAMatrixUtil.h"
#include "CodeMsg.h"
//---------------------------------------------------------------------------
typedef std::vector<std::string>	STLNameList;
//---------------------------------------------------------------------------
void SingleHardPoint::Draw() const
{
	ASSERT(PIPE);

	Transform	oldModelView, currModelView;

	PIPE->get_modelview(oldModelView);

	currModelView = oldModelView;

	Vector	position = mHardpointInfo.point;
	Matrix	orientation = mHardpointInfo.orientation;

	Transform	tr(orientation, position);

	currModelView = currModelView * tr;

	PIPE->set_modelview(currModelView);

	PrimitiveBuilder	pb(PIPE);

	pb.Begin(PB_LINES);

		pb.Color3f(1, 0, 0);
		position = mHardpointInfo.orientation.get_i();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	
		pb.Color3f(0, 1, 0);
		position = mHardpointInfo.orientation.get_j();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	
		pb.Color3f(0, 0, 0);
		position = mHardpointInfo.orientation.get_k();
		pb.Vertex3f(0, 0, 0);
		pb.Vertex3f(position.x, position.y, position.z);
	
	pb.End();

	PIPE->set_modelview(oldModelView);
}
//---------------------------------------------------------------------------
void __cdecl HardPointCallback(const char* name, void* misc)
{
	STLNameList* nameListP = static_cast<STLNameList*>(misc);

	nameListP->push_back(std::string(name));
}
//---------------------------------------------------------------------------
HardPoints::HardPoints(INSTANCE_INDEX instanceIndex, IEngine* engine)
:mInstanceIndex(instanceIndex), mEngine(engine)
{
	COMPTR<IHardpoint>		iHardpoint;
	int						hardPointCount;
	STLNameList				hardPointNames;
	ARCHETYPE_INDEX			archetypeIndex = mEngine->get_archetype(mInstanceIndex);
	
	if(mEngine->QueryInterface ("IHardpoint", iHardpoint) == GR_OK)
	{	iHardpoint->enumerate_hardpoints (HardPointCallback, archetypeIndex, (void*)&hardPointNames);
		hardPointCount = hardPointNames.size();

		mHardPoints = new SingleHardPoint[hardPointCount];
		
		int	hardPointsInserted = 0;
		for(int hardPointIdx = 0; hardPointIdx < hardPointCount; ++hardPointIdx)
		{	HardpointInfo	hardpointInfo;

			std::string	hardPointName = hardPointNames[hardPointIdx];
			if(true == iHardpoint->retrieve_hardpoint_info (archetypeIndex, hardPointName.c_str(), hardpointInfo))
			{	mHardPoints[hardPointsInserted] = SingleHardPoint(hardPointName, hardpointInfo);
				++hardPointsInserted;
			}
		}
		mHardPointCount = hardPointsInserted;
	}
	
	// Make sure to release the archetype, because it is no longer needed.
	mEngine->release_archetype(archetypeIndex);
}
//---------------------------------------------------------------------------
HardPoints::~HardPoints()
{
	delete[] mHardPoints;
}
//---------------------------------------------------------------------------
void HardPoints::Draw(const Transform& modelView) const
{
	ASSERT(PIPE);

	Transform	oldModelView, currModelView;

	PIPE->get_modelview(oldModelView);
    PIPE->set_pipeline_state(RP_TEXTURE, FALSE);
//    PIPE->set_pipeline_state(GL_LIGHTING);

	Matrix orientation = mEngine->get_orientation(mInstanceIndex);
	Vector position = mEngine->get_position(mInstanceIndex);

	Transform	tr(orientation, position);

	currModelView = modelView * tr;

	PIPE->set_modelview(currModelView);

	const SingleHardPoint*	hardPointCurr = Begin();
	const SingleHardPoint* const	hardPointEnd = End();

	while(hardPointCurr != hardPointEnd)
	{	hardPointCurr->Draw();

		++hardPointCurr;
	}

//    PIPE->set_pipeline_state(GL_LIGHTING);
    PIPE->set_pipeline_state(RP_TEXTURE, TRUE);
	PIPE->set_modelview(oldModelView);
}
//---------------------------------------------------------------------------
