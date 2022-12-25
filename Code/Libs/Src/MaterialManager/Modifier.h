#ifndef _MODIFIER_H_
#define _MODIFIER_H_

#include "dacom.h"
#include <string>

//----------------------------------------------------------------------------------------------

struct Material;
struct MatFloat;
struct MatInt;
struct MatColor;
struct MatState;
struct IInternalMaterialManager;

struct Modifier : public IModifier
{
	//IModifier
	virtual void SetNextModifier(IModifier * other);

	virtual IModifier * GetNextModifier();

	virtual IMaterial * GetMaterial();

	virtual void Release();

	virtual void AddRef();

	//Modifier

	Modifier(struct IInternalMaterialManager * _owner);

	~Modifier();

	//data

	Modifier * next;
	Material * material;
	IInternalMaterialManager * owner;

	U32 refCount;

	enum ModType
	{
		MT_INT,
		MT_FLOAT,
		MT_COLOR,
		MT_RENDERSTATE,
	}modType;

	union
	{
		S32 intValue;
		SINGLE floatValue;
		struct ColorValue
		{
			U8 red;
			U8 green;
			U8 blue;
		}colorValue;
		DWORD stateValue;
	};

	union
	{
		MatFloat * floatNode;
		MatInt * intNode;
		MatColor * colorNode;
		MatState * stateNode;
	};
};

#endif
