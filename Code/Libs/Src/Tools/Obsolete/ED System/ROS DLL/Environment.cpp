// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Environment.h"

/**# implementation Environment:: id(C_0886779039) */
enum FieldID
{
};

void Environment::SetTerrain(Terrain& terrainR) 
{

}

void Environment::SetLightMap(LightMap& lightMapR) 
{

}

void Environment::SetEventZoneMap(EventZoneMap& eventZoneMapR) 
{

}

const ROS::ROSString& Environment::GetName() const 
{
	return mName;
}

void Environment::Write(std::ostream& ostreamR) const 
{
	OStreamWiz<FieldID>	oWiz(oStream);
}

void Environment::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(oStream);
}