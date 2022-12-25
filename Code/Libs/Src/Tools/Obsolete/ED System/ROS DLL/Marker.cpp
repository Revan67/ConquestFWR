// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Marker.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "Scene.h"
#include "GLUtils.h"
#include "AStaticsState.h"
#include "RPUL.h"
#include "DARenderPipeline.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState,
	kStaticsState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Marker::Marker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticsState& staticsState)
: mSceneEntityState(*this, name, makeNameUnique), mStaticsState(&staticsState), mPreviousMarker(NULL), mNextMarker(NULL)
{
	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
Marker::Marker(std::istream& iStream, Scene& scene, AStaticsState& staticsState)
:mSceneEntityState(*this, "Marker", false), mStaticsState(&staticsState), mPreviousMarker(NULL), mNextMarker(NULL)
{
	mSceneEntityState.SetScene(&scene);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void Marker::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
Marker::~Marker()
{
	if(mNextMarker)
	{
		mNextMarker->mPreviousMarker = mPreviousMarker;
	}

	if(mPreviousMarker)
	{
		mPreviousMarker->mNextMarker = mNextMarker;
	}

	mNextMarker = NULL;
	mPreviousMarker = NULL;

	if(mStaticsState)
	{
		delete mStaticsState;
	}
}
// --------------------------------------------------------------------------
ROSString Marker::GetArchetypeName() const
{
	return GetMarkerArchetypeName();
}
// --------------------------------------------------------------------------
ROSString Marker::GetMarkerArchetypeName()
{
	return "Marker";
}
// --------------------------------------------------------------------------
Time Marker::GetLocationTime(unsigned int keyPointIndex) const
{
    return Time(0);
}
// --------------------------------------------------------------------------
Time Marker::GetOrientationTime(unsigned int keyPointIndex) const
{
    return Time(0);
}
// --------------------------------------------------------------------------
Location Marker::GetLocation(Time time) const
{
	ASSERT(mStaticsState);

    return mStaticsState->GetPosition().GetLocation();
}
// --------------------------------------------------------------------------
Orientation Marker::GetOrientation(Time time) const
{
	ASSERT(mStaticsState);

    return mStaticsState->GetPosition().GetOrientation();
}
// --------------------------------------------------------------------------
void Marker::SetLocation(const Location& location, Time time)
{
	ASSERT(mStaticsState);

    Position	position = mStaticsState->GetPosition();
	
	position.SetLocation(location);

	mStaticsState->SetPosition(position);
}
// --------------------------------------------------------------------------
void Marker::SetOrientation(const Orientation& orientation, Time time)
{
	ASSERT(mStaticsState);

    Position	position = mStaticsState->GetPosition();
	
	position.SetOrientation(orientation);

	mStaticsState->SetPosition(position);
}
// --------------------------------------------------------------------------
void Marker::RemoveLocation(Time time)
{
}
// --------------------------------------------------------------------------
void Marker::RemoveOrientation(Time time)
{
}
// --------------------------------------------------------------------------
bool Marker::IsAtKeyLocation() const
{
	return true;
}
// --------------------------------------------------------------------------
bool Marker::IsAtKeyOrientation() const
{
	return true;
}
// --------------------------------------------------------------------------
void Marker::GotoForLocationRole(Time time)
{
}
// --------------------------------------------------------------------------
void Marker::GotoForOrientationRole(Time time)
{
}
// --------------------------------------------------------------------------
void Marker::LocationStateUpdated(Time time)
{
}
// --------------------------------------------------------------------------
void Marker::OrientationStateUpdated(Time time)
{
}
// --------------------------------------------------------------------------
int Marker::GetLocationRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
int Marker::GetOrientationRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
void Marker::Render(const ROS::DABaseCamera* camera) const
{
	RenderMarker(camera);

	RenderPathToNextMarker(camera);
}
// --------------------------------------------------------------------------
void Marker::RenderMarker(const ROS::DABaseCamera* camera) const
{
	float	color[] = {1.0, 1.0, 1.0};

	GL::WireCube(1, color);
}
// --------------------------------------------------------------------------
void Marker::RenderPathToNextMarker(const ROS::DABaseCamera* camera) const
{
	ASSERT(mStaticsState);

	if(mNextMarker)
	{
		// Draw the connecting line.
		ASSERT(PIPE);

		PrimitiveBuilder pb(PIPE);

		pb.Color3f(1, 1, 1);

		pb.Begin(PB_LINES);
			const Location	currentLocation = mStaticsState->GetPosition().GetLocation();
			const Location	nextLocation = mNextMarker->mStaticsState->GetPosition().GetLocation();

			pb.Vertex3f	(0, 0, 0);
			pb.Vertex3f	(	nextLocation.GetX() - currentLocation.GetX(), 
							nextLocation.GetY() - currentLocation.GetY(),
							nextLocation.GetZ() - currentLocation.GetZ()
						);
		pb.End();
	}
}
// --------------------------------------------------------------------------
CPP_DEFN OrientationMemento Marker::GetOrientationMemento(Time time) const
{
	const Orientation	orientation = GetConstStaticsStateAccessor()->GetOrientation(time);

	return OrientationMemento(orientation, false, time);
}
// --------------------------------------------------------------------------
CPP_DEFN void Marker::SetOrientationMemento(const OrientationMemento& memento)
{
	const Time	mementoTime = memento.GetTime();

	SetOrientation(memento.GetOrientation(), mementoTime);

	ASSERT(!memento.IsKeyPoint());
}
// --------------------------------------------------------------------------
CPP_DEFN LocationMemento Marker::GetLocationMemento(Time time) const
{
	const Location	location = GetLocation(time);

	return LocationMemento(location, false, time);
}
// --------------------------------------------------------------------------
CPP_DEFN void Marker::SetLocationMemento(const LocationMemento& memento)
{
	const Time	mementoTime = memento.GetTime();

	SetLocation(memento.GetLocation(), mementoTime);

	ASSERT(!memento.IsKeyPoint());
}
// --------------------------------------------------------------------------
AStaticSceneEntity::InterpolationType Marker::GetLocationInterpolationType(Time time) const
{
	return AStaticSceneEntity::kLinearFixed;
}
// --------------------------------------------------------------------------
CPP_DEFN void Marker::SetNextMarker(Marker* nextMarker)
{
	mNextMarker = nextMarker;

	if(nextMarker)
	{
		nextMarker->mPreviousMarker = this;
	}
}
// --------------------------------------------------------------------------
const Marker* Marker::GetPreviousMarker() const
{
	return mPreviousMarker;
}
// --------------------------------------------------------------------------
Marker* Marker::GetPreviousMarker()
{
	return mPreviousMarker;
}
// --------------------------------------------------------------------------
const Marker* Marker::GetNextMarker() const
{
	return mNextMarker;
}
// --------------------------------------------------------------------------
Marker* Marker::GetNextMarker()
{
	return mNextMarker;
}
// --------------------------------------------------------------------------
SceneEntityState& Marker::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& Marker::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& Marker::GetPhysicalState()
{
	ASSERT(mStaticsState);

	return *mStaticsState;
}
// --------------------------------------------------------------------------
const APhysicalState& Marker::GetPhysicalState() const
{
	ASSERT(mStaticsState);

	return *mStaticsState;
}
// --------------------------------------------------------------------------
bool Marker::IsPersistent() const
{
	return false;
}
// --------------------------------------------------------------------------
void Marker::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void Marker::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void Marker::ReadSubObject(std::istream& iStream)
{
	ASSERT(mStaticsState);

	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kSceneEntityState, mSceneEntityState);
	iWiz.Get(kStaticsState, *mStaticsState);
}
// --------------------------------------------------------------------------
void Marker::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	ROS::Time timeZero(0);
	char buffer[1024*2];
	int j;
	// write the lua enity table for this entity
	j = sprintf(buffer,			"\n");
	j+= sprintf(buffer + j,		"%s = \n{\n", mSceneEntityState.GetName().c_str());
	j+= sprintf(buffer + j,		"\ttype = MARKER,\n\tflags = 0,\n");
	j+= sprintf(buffer + j,		"\tspatialprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tpos = {%f, %f, %f},\n", GetLocation(timeZero).GetX(), GetLocation(timeZero).GetY(), GetLocation(timeZero).GetZ());
	j+= sprintf(buffer + j,		"\t\torient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n",
			GetOrientation(timeZero).GetI().x, GetOrientation(timeZero).GetI().y, GetOrientation(timeZero).GetI().z, 
			GetOrientation(timeZero).GetJ().x, GetOrientation(timeZero).GetJ().y, GetOrientation(timeZero).GetJ().z, 
			GetOrientation(timeZero).GetK().x, GetOrientation(timeZero).GetK().y, GetOrientation(timeZero).GetK().z
		);
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"\tuserprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"},\n");

	// write the orientation animation data
	if (GetOrientationRoleIndex() >= 0)
	{
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetOrientationRoleIndex()));
		OrientationRole*	role = dynamic_cast<OrientationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}
	// write the location animation data
	if (GetLocationRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetLocationRoleIndex()));
		LocationRole*	role = dynamic_cast<LocationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}

	oWiz.Put(kSceneEntityState, mSceneEntityState);
	oWiz.Put(kStaticsState, *mStaticsState, buffer);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
