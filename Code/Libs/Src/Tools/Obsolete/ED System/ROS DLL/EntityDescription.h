// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef EntityDescription_h
#define EntityDescription_h
// --------------------------------------------------------------------------
typedef const char** EntityDescriptionStrings;
typedef unsigned int EntityDescriptionStringsCount;
// --------------------------------------------------------------------------
enum CompoundStringIndex
{	kFilename
};

enum DeformableStringIndex
{	kMeshFilename,
	kSkeletonFilePath,
	kAnimationFilename
};
// --------------------------------------------------------------------------
#endif