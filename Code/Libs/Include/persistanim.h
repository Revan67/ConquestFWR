#ifndef _PERSISTANIM_H_
#define _PERSISTANIM_H_

const unsigned int PersistAnimTARGET_NAME_MAX = 128;
const unsigned int PersistAnimCHANNEL_NAME_MAX = 128;

struct PersistAnimChannelMapping
{
	char parent[PersistAnimTARGET_NAME_MAX];
	char child[PersistAnimTARGET_NAME_MAX];
	char channel[PersistAnimCHANNEL_NAME_MAX];
};

const char PersistAnimJointMapStem[] = "Joint map";
const char PersistAnimObjectMapStem[] = "Object map";
const char PersistAnimEventMapStem[] = "Event map";

const char PersistAnimParentFilename[] = "Parent name";
const char PersistAnimChildFilename[] = "Child name";
const char PersistAnimChannelFilename[] = "Channel name";
const char PersistAnimChannelData[] = "Channel";
const char PersistAnimRootHeightFilename[] = "Root height";

#endif
