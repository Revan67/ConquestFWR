// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Environment_h
#define Environment_h

#include "StringType.h"
#include "Links.h"

class LightMap;
class Terrain;
class EventZoneMap;
class Lighting;
//-----------------------------------------------------------------
// Environment
//-----------------------------------------------------------------

 /**# :[Description = "Maintains environmental settings"] */

class Environment
{
  public:
	const ROS::ROSString& GetName() const;
	void Write(std::ostream& ostreamR) const;
	void Read(std::istream& istreamR);
	void SetLightMap(LightMap& lightMapR);
	void SetEventZoneMap(EventZoneMap& eventZoneMapR);
	void SetTerrain(Terrain& terrainR);
private :
	ROS::ROSString mName;
	/**#: [Cardinalities = "0..1/"]*/
	AggPointer<Terrain>	mTerrainV;
	/**#: [Cardinalities = "0..1/"]*/
	AggPointer<EventZoneMap>	mEventZoneMapV;
	/**#: [Cardinalities = "1..1/"]*/
	AggPointer<Lighting>	mLightingP;
};

#endif