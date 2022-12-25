#ifndef PARTICLEPARAMETERS_H
#define PARTICLEPARAMETERS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ParticleParameters.H                        //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Tmauer $
*/


/*

*/
//--------------------------------------------------------------------------//

//encoding types are used for save load purposes.
#include "stdio.h"

struct EncodedRampEntry
{
	U32 value;//index into floatEntry list
	SINGLE key;
	U32 next;//index into rampEntry list
};

struct EncodedFloatTypeEntry
{
	S32 type;
	union
	{
		SINGLE constant;
		struct Range
		{
			U32 min;//index into floatEntry list
			U32 max;//index into floatEntry list
		}range;
		struct Parameter
		{
			char name[32];
			U32 targetID;//0 for global parameter
		}parameter;
		struct Ramp
		{
			S32 type;
			U32  firstKey;//index into rampEntry list
		}ramp;
		struct Add
		{
			U32 value1;//index into floatEntry list
			U32 value2;//index into floatEntry list
		}add;
		struct Subtract
		{
			U32 value1;//index into floatEntry list
			U32 value2;//index into floatEntry list
		}subtract;
		struct Multiply
		{
			U32 value1;//index into floatEntry list
			U32 value2;//index into floatEntry list
		}multiply;
		struct Divide
		{
			U32 value1;//index into floatEntry list
			U32 value2;//index into floatEntry list
		}divide;
	};
};

struct EncodedFloatTypeHeader
{
	U32 size;
	U32 floatEntryOffset;
	U32 rampOffset;
//	EncodedFloatTypeEntry floatEntry[];
//	EncodedRampEntry rampEntry[];
};

struct EncodedTransformTypeEntry_Version1
{
	S32 type;
	union
	{
		struct TargetTrans
		{
			U32 targetID;
			U32 hpID;
		}targetTrans;
		char filterName[32];
		struct Offset
		{
			U32 offX;//index into floatEntry list
			U32 offY;//index into floatEntry list
			U32 offZ;//index into floatEntry list
			U32 baseTrans;//index into transEntry list
		}offset;
	};
};

struct EncodedTransformTypeEntry
{
	S32 type;
	union
	{
		struct TargetTrans
		{
			U32 targetID;
			U32 hpID;
		}targetTrans;
		struct TargetTrans_Str
		{
			U32 targetID;//upper 16 bits is the attachment id
			char hpName[32];
		}targetTrans_str;
		char filterName[32];
		struct Offset
		{
			U32 offX;//index into floatEntry list
			U32 offY;//index into floatEntry list
			U32 offZ;//index into floatEntry list
			U32 baseTrans;//index into transEntry list
		}offset;
		struct OffsetIJK
		{
			U32 offI;//index into floatEntry list
			U32 offJ;//index into floatEntry list
			U32 offK;//index into floatEntry list
			U32 baseTrans;//index into transEntry list
		}offsetIJK;
		struct RotateIJK
		{
			U32 rotI;//index into floatEntry list
			U32 rotJ;//index into floatEntry list
			U32 rotK;//index into floatEntry list
			U32 baseTrans;//index into transEntry list
		}rotateIJK;
		struct LookAtUp
		{
			U32 look; //index into transEntry list
			U32 at; //index into transEntry list
			U32 up; //index into transEntry list
		}lookAtUp;
	};
};

struct EncodedTransformTypeHeader
{
	U32 size;
	U32 transformEntryOffset;
	U32 floatEntryOffset;
	U32 rampOffset;
//	EncodedTransformTypeEntry floatEntry[];
//	EncodedFloatTypeEntry floatEntry[];
//	EncodedRampEntry rampEntry[];
};


// These types are used for parameters to the particle filters
//you need to take care that they get deleted by the process that made them.
struct FloatType;
struct RampKey
{
	FloatType * value;
	SINGLE key;
	RampKey * next;

	RampKey();

	~RampKey();

	RampKey * CreateCopy() const;
};



struct FloatType
{
	enum Type
	{
		CONSTANT,
		RANGE,
		PARAMETER,
		RAMP,
		ADD,
		SUBTRACT,
		MULTIPLY,
		DIVIDE,
		LOOP_RAMP,
		OSCILATE_RAMP,
		CONST_RANGE,
	}type;
	enum RampType
	{
		LIFETIME,
		SPEED,
		LIFETIME_PERCENT,
		EFFECT_LIFETIME,
	};
	union
	{
		SINGLE constant;
		struct Range
		{
			FloatType * min;
			FloatType * max;
		}range;
		struct Parameter
		{
			char name[32];
			U32 targetID;//0 for global parameter
		}parameter;
		struct Ramp
		{
			RampType type;
			RampKey * firstKey;
		}ramp;
		struct Add
		{
			FloatType * value1;
			FloatType * value2;
		}add;
		struct Subtract
		{
			FloatType * value1;
			FloatType * value2;
		}subtract;
		struct Multiply
		{
			FloatType * value1;
			FloatType * value2;
		}multiply;
		struct Divide
		{
			FloatType * value1;
			FloatType * value2;
		}divide;
	};

	char stringDef[256];

	FloatType()
	{
		memset(this,0,sizeof(FloatType));
	};

	~FloatType()
	{
		switch(type)
		{
		case PARAMETER:
		case CONSTANT:
			break;
		case RANGE:
		case CONST_RANGE:
			{
				delete range.min;
				delete range.max;
			}
			break;
		case LOOP_RAMP:
		case OSCILATE_RAMP:
		case RAMP:
			{
				delete ramp.firstKey;
			}
			break;
		case ADD:
			{
				delete add.value1;
				delete add.value2;
			}
			break;
		case SUBTRACT:
			{
				delete subtract.value1;
				delete subtract.value2;
			}
			break;
		case MULTIPLY:
			{
				delete multiply.value1;
				delete multiply.value2;
			}
			break;
		case DIVIDE:
			{
				delete divide.value1;
				delete divide.value2;
			}
			break;
		}
	}

	FloatType * CreateCopy() const
	{
		FloatType * copy = new FloatType();
		copy->type = type;
		switch(type)
		{
		case CONSTANT:
			{
				copy->constant = constant;
			}
			break;
		case CONST_RANGE:
		case RANGE:
			{
				if(range.min)
					copy->range.min = range.min->CreateCopy();
				else
					copy->range.min = NULL;
				if(range.max)
					copy->range.max = range.max->CreateCopy();
				else
					copy->range.max = NULL;
			}
			break;
		case PARAMETER:
			{
				strcpy(copy->parameter.name,parameter.name);
				copy->parameter.targetID = parameter.targetID;
			}
			break;
		case LOOP_RAMP:
		case OSCILATE_RAMP:
		case RAMP:
			{
				copy->ramp.type = ramp.type;
				if(ramp.firstKey)
					copy->ramp.firstKey = ramp.firstKey->CreateCopy();
				else
					copy->ramp.firstKey = NULL;
			}
			break;
		case ADD:
			{
				if(add.value1)
					copy->add.value1 = add.value1->CreateCopy();
				else
					copy->add.value1 = NULL;
				if(add.value2)
					copy->add.value2 = add.value2->CreateCopy();
				else
					copy->add.value2 = NULL;
			}
			break;
		case SUBTRACT:
			{
				if(subtract.value1)
					copy->subtract.value1 = subtract.value1->CreateCopy();
				else
					copy->subtract.value1 = NULL;
				if(subtract.value2)
					copy->subtract.value2 = subtract.value2->CreateCopy();
				else
					copy->subtract.value2 = NULL;
			}
			break;
		case MULTIPLY:
			{
				if(multiply.value1)
					copy->multiply.value1 = multiply.value1->CreateCopy();
				else
					copy->multiply.value1 = NULL;
				if(multiply.value2)
					copy->multiply.value2 = multiply.value2->CreateCopy();
				else
					copy->multiply.value2 = NULL;
			}
			break;
		case DIVIDE:
			{
				if(divide.value1)
					copy->divide.value1 = divide.value1->CreateCopy();
				else
					copy->divide.value1 = NULL;
				if(divide.value2)
					copy->divide.value2 = divide.value2->CreateCopy();
				else
					copy->divide.value2 = NULL;
			}
			break;
		}
		return copy;
	}

	const char * GetStringDef()
	{
		switch(type)
		{
		case PARAMETER:
			{
				sprintf(stringDef,"Parameter:%s",parameter.name);
			}
			break;
		case CONSTANT:
			{
				sprintf(stringDef,"%f",constant);
			}
			break;
		case CONST_RANGE:
		case RANGE:
			{
				const char * strMin = ((range.min)?range.min->GetStringDef():"NULL");
				const char * strMax = ((range.max)?range.max->GetStringDef():"NULL");
				if(type == RANGE)
					strcpy(stringDef,"Range(");
				else
					strcpy(stringDef,"Const Range(");

				if(strlen(stringDef)+strlen(strMin) > 254)
					break;
				strcat(stringDef,strMin);
				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");

				if(strlen(stringDef)+strlen(strMax) > 254)
					break;
				strcat(stringDef,strMax);
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");

			}
			break;
		case LOOP_RAMP:
		case OSCILATE_RAMP:
		case RAMP:
			{
				if(type == LOOP_RAMP)
					strcpy(stringDef,"Looping ");
				else if(type == OSCILATE_RAMP)
					strcpy(stringDef,"Oscilating ");
				else
					strcpy(stringDef,"Infinite ");
				
				switch(ramp.type)
				{
				case LIFETIME:
					{
						strcat(stringDef,"Lifetime Ramp(");
					}
					break;
				case SPEED:
					{
						strcat(stringDef,"Speed Ramp(");
					}
					break;
				case LIFETIME_PERCENT:
					{
						strcat(stringDef,"LifePercent Ramp(");
					}
					break;
				case EFFECT_LIFETIME:
					{
						strcat(stringDef,"Effect Lifetime Ramp(");
					}
					break;
				}
				RampKey * key = ramp.firstKey;
				while(key)
				{
					char buffer[256];
					const char * strKey = ((key->value)?key->value->GetStringDef():"NULL");
					sprintf(buffer,"(%f,%s)",key->key,strKey);
					if(strlen(stringDef)+strlen(buffer) > 254)
						break;
					strcat(stringDef,buffer);
					key = key->next;
				}
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case ADD:
			{
				const char * strV1 = ((add.value1)?add.value1->GetStringDef():"NULL");
				const char * strV2 = ((add.value2)?add.value2->GetStringDef():"NULL");
				strcpy(stringDef,"Add(");

				if(strlen(stringDef)+strlen(strV1) > 254)
					break;
				strcat(stringDef,strV1);
				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");

				if(strlen(stringDef)+strlen(strV2) > 254)
					break;
				strcat(stringDef,strV2);
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case SUBTRACT:
			{
				const char * strV1 = ((subtract.value1)?subtract.value1->GetStringDef():"NULL");
				const char * strV2 = ((subtract.value2)?subtract.value2->GetStringDef():"NULL");
				strcpy(stringDef,"Subtract(");

				if(strlen(stringDef)+strlen(strV1) > 254)
					break;
				strcat(stringDef,strV1);
				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");

				if(strlen(stringDef)+strlen(strV2) > 254)
					break;
				strcat(stringDef,strV2);
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case MULTIPLY:
			{
				const char * strV1 = ((multiply.value1)?multiply.value1->GetStringDef():"NULL");
				const char * strV2 = ((multiply.value2)?multiply.value2->GetStringDef():"NULL");
				strcpy(stringDef,"Multiply(");

				if(strlen(stringDef)+strlen(strV1) > 254)
					break;
				strcat(stringDef,strV1);
				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");

				if(strlen(stringDef)+strlen(strV2) > 254)
					break;
				strcat(stringDef,strV2);
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case DIVIDE:
			{
				const char * strV1 = ((divide.value1)?divide.value1->GetStringDef():"NULL");
				const char * strV2 = ((divide.value2)?divide.value2->GetStringDef():"NULL");
				strcpy(stringDef,"Divide(");

				if(strlen(stringDef)+strlen(strV1) > 254)
					break;
				strcat(stringDef,strV1);
				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");

				if(strlen(stringDef)+strlen(strV2) > 254)
					break;
				strcat(stringDef,strV2);
				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		}
		return stringDef;
	}
};

struct TransformType
{
	enum Type
	{
		TARGET_TRANSFORM,
		FILTER_EFFECT,
		UP,
		CAMERA,
		CAMERA_LOOK,
		OFFSET,
		INPUT_POINT,
		TARGET_TRANSFORM_STR,
		OFFSET_IJK,
		ROTATE_IJK,
		LOOK_AT_UP,
	}type;
	union
	{
		struct TargetTrans
		{
			U32 targetID;
			U32 hpID;
		}targetTrans;
		struct TargetTrans_Str
		{
			U32 targetID;//only realy 16 bit can be stored
			char hpName[32];
		}targetTrans_str;
		char filterName[32];
		struct Offset
		{
			FloatType * offX;
			FloatType * offY;
			FloatType * offZ;
			TransformType * baseTrans;
		}offset;
		struct OffsetIJK
		{
			FloatType * offI;
			FloatType * offJ;
			FloatType * offK;
			TransformType * baseTrans;
		}offsetIJK;
		struct RotateIJK
		{
			FloatType * rotI;
			FloatType * rotJ;
			FloatType * rotK;
			TransformType * baseTrans;
		}rotateIJK;
		struct LookAtUp
		{
			TransformType * look;
			TransformType * at;
			TransformType * up;
		}lookAtUp;
	};

	char stringDef[256];

	TransformType()
	{
		memset(this,0,sizeof(TransformType));
	}

	~TransformType()
	{
		if(type == OFFSET)
		{
			delete offset.offX;
			delete offset.offY;
			delete offset.offZ;
			delete offset.baseTrans;
		}
		else if(type == OFFSET_IJK)
		{
			delete offsetIJK.offI;
			delete offsetIJK.offJ;
			delete offsetIJK.offK;
			delete offsetIJK.baseTrans;
		}
		else if(type == ROTATE_IJK)
		{
			delete rotateIJK.rotI;
			delete rotateIJK.rotJ;
			delete rotateIJK.rotK;
			delete rotateIJK.baseTrans;
		}
		else if(type == LOOK_AT_UP)
		{
			delete lookAtUp.look;
			delete lookAtUp.at;
			delete lookAtUp.up;
		}
	}

	TransformType * CreateCopy() const
	{
		TransformType * copy = new TransformType();
		copy->type = type;
		switch(type)
		{
		case TARGET_TRANSFORM:
			{
				copy->targetTrans.targetID = targetTrans.targetID;
				copy->targetTrans.hpID = targetTrans.hpID;
			}
			break;
		case TARGET_TRANSFORM_STR:
			{
				copy->targetTrans_str.targetID = targetTrans_str.targetID;
				strcpy(copy->targetTrans_str.hpName,targetTrans_str.hpName);
			}
			break;
		case FILTER_EFFECT:
			{
				strcpy(copy->filterName,filterName);
			}
			break;
		case UP:
		case CAMERA:
		case CAMERA_LOOK:
		case INPUT_POINT:
			break;
		case OFFSET:
			{
				if(offset.offX)
					copy->offset.offX = offset.offX->CreateCopy();
				else
					copy->offset.offX = NULL;
				if(offset.offY)
					copy->offset.offY = offset.offY->CreateCopy();
				else
					copy->offset.offY = NULL;
				if(offset.offZ)
					copy->offset.offZ = offset.offZ->CreateCopy();
				else
					copy->offset.offZ = NULL;
				if(offset.baseTrans)
					copy->offset.baseTrans = offset.baseTrans->CreateCopy();
				else
					copy->offset.baseTrans = NULL;
			}
			break;
		case OFFSET_IJK:
			{
				if(offsetIJK.offI)
					copy->offsetIJK.offI = offsetIJK.offI->CreateCopy();
				else
					copy->offsetIJK.offI = NULL;
				if(offsetIJK.offJ)
					copy->offsetIJK.offJ = offsetIJK.offJ->CreateCopy();
				else
					copy->offsetIJK.offJ = NULL;
				if(offsetIJK.offK)
					copy->offsetIJK.offK = offsetIJK.offK->CreateCopy();
				else
					copy->offsetIJK.offK = NULL;
				if(offsetIJK.baseTrans)
					copy->offsetIJK.baseTrans = offsetIJK.baseTrans->CreateCopy();
				else
					copy->offsetIJK.baseTrans = NULL;
			}
			break;
		case ROTATE_IJK:
			{
				if(rotateIJK.rotI)
					copy->rotateIJK.rotI = rotateIJK.rotI->CreateCopy();
				else
					copy->rotateIJK.rotI = NULL;
				if(rotateIJK.rotJ)
					copy->rotateIJK.rotJ = rotateIJK.rotJ->CreateCopy();
				else
					copy->rotateIJK.rotJ = NULL;
				if(rotateIJK.rotK)
					copy->rotateIJK.rotK = rotateIJK.rotK->CreateCopy();
				else
					copy->rotateIJK.rotK = NULL;
				if(rotateIJK.baseTrans)
					copy->rotateIJK.baseTrans = rotateIJK.baseTrans->CreateCopy();
				else
					copy->rotateIJK.baseTrans = NULL;
			}
			break;
		case LOOK_AT_UP:
			{
				if(lookAtUp.look)
					copy->lookAtUp.look = lookAtUp.look->CreateCopy();
				else
					copy->lookAtUp.look = NULL;
				if(lookAtUp.at)
					copy->lookAtUp.at = lookAtUp.at->CreateCopy();
				else
					copy->lookAtUp.at = NULL;
				if(lookAtUp.up)
					copy->lookAtUp.up = lookAtUp.up->CreateCopy();
				else
					copy->lookAtUp.up = NULL;
			}
			break;
		}
		return copy;
	}
	
	const char * GetStringDef()
	{
		switch(type)
		{
		case TARGET_TRANSFORM:
			{
				sprintf(stringDef,"OLD FIX THIS Target %d",targetTrans.targetID);
			}
			break;
		case TARGET_TRANSFORM_STR:
			{
				sprintf(stringDef,"Target %d, HP:%s",targetTrans_str.targetID,targetTrans_str.hpName);
			}
			break;
		case FILTER_EFFECT:
			{
				sprintf(stringDef,"From Filter:%s",filterName);
			}
			break;
		case UP:
			{
				strcpy(stringDef,"Up");
			}
			break;
		case CAMERA:
			{
				strcpy(stringDef,"Camera");
			}
			break;
		case CAMERA_LOOK:
			{
				strcpy(stringDef,"Camera Look");
			}
			break;
		case INPUT_POINT:
			{
				strcpy(stringDef,"Input Point");
			}
			break;
		case OFFSET:
			{
				const char * strBaseDef = ((offset.baseTrans)?offset.baseTrans->GetStringDef():"NULL");
				const char * strOffX = ((offset.offX)?offset.offX->GetStringDef():"NULL");
				const char * strOffY = ((offset.offY)?offset.offY->GetStringDef():"NULL");
				const char * strOffZ = ((offset.offZ)?offset.offZ->GetStringDef():"NULL");
				strcpy(stringDef,"Offset (");
				if(strlen(stringDef)+strlen(strBaseDef) > 254)
					break;
				strcat(stringDef,strBaseDef);
				if(strlen(stringDef)+strlen(") + (") > 254)
					break;
				strcat(stringDef,") + (");
				if(strlen(stringDef)+strlen(strOffX) > 254)
					break;
				strcat(stringDef,strOffX);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffY) > 254)
					break;
				strcat(stringDef,strOffY);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffZ) > 254)
					break;
				strcat(stringDef,strOffZ);

				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case OFFSET_IJK:
			{
				const char * strBaseDef = ((offsetIJK.baseTrans)?offsetIJK.baseTrans->GetStringDef():"NULL");
				const char * strOffI = ((offsetIJK.offI)?offsetIJK.offI->GetStringDef():"NULL");
				const char * strOffJ = ((offsetIJK.offJ)?offsetIJK.offJ->GetStringDef():"NULL");
				const char * strOffK = ((offsetIJK.offK)?offsetIJK.offK->GetStringDef():"NULL");
				strcpy(stringDef,"OffsetIJK (");
				if(strlen(stringDef)+strlen(strBaseDef) > 254)
					break;
				strcat(stringDef,strBaseDef);
				if(strlen(stringDef)+strlen(") + (") > 254)
					break;
				strcat(stringDef,") + (");
				if(strlen(stringDef)+strlen(strOffI) > 254)
					break;
				strcat(stringDef,strOffI);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffJ) > 254)
					break;
				strcat(stringDef,strOffJ);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffK) > 254)
					break;
				strcat(stringDef,strOffK);

				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case ROTATE_IJK:
			{
				const char * strBaseDef = ((rotateIJK.baseTrans)?rotateIJK.baseTrans->GetStringDef():"NULL");
				const char * strOffI = ((rotateIJK.rotI)?rotateIJK.rotI->GetStringDef():"NULL");
				const char * strOffJ = ((rotateIJK.rotJ)?rotateIJK.rotJ->GetStringDef():"NULL");
				const char * strOffK = ((rotateIJK.rotK)?rotateIJK.rotK->GetStringDef():"NULL");
				strcpy(stringDef,"RotateIJK (");
				if(strlen(stringDef)+strlen(strBaseDef) > 254)
					break;
				strcat(stringDef,strBaseDef);
				if(strlen(stringDef)+strlen(") + (") > 254)
					break;
				strcat(stringDef,") + (");
				if(strlen(stringDef)+strlen(strOffI) > 254)
					break;
				strcat(stringDef,strOffI);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffJ) > 254)
					break;
				strcat(stringDef,strOffJ);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strOffK) > 254)
					break;
				strcat(stringDef,strOffK);

				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		case LOOK_AT_UP:
			{
				const char * strLook = ((lookAtUp.look)?lookAtUp.look->GetStringDef():"NULL");
				const char * strAt = ((lookAtUp.at)?lookAtUp.at->GetStringDef():"NULL");
				const char * strUp = ((lookAtUp.up)?lookAtUp.up->GetStringDef():"NULL");
				strcpy(stringDef,"LookAtUp (");
				if(strlen(stringDef)+strlen(strLook) > 254)
					break;
				strcat(stringDef,strLook);
				if(strlen(stringDef)+strlen(") + (") > 254)
					break;
				strcat(stringDef,") + (");
				if(strlen(stringDef)+strlen(strAt) > 254)
					break;
				strcat(stringDef,strAt);

				if(strlen(stringDef)+strlen(",") > 254)
					break;
				strcat(stringDef,",");
				if(strlen(stringDef)+strlen(strUp) > 254)
					break;
				strcat(stringDef,strUp);

				if(strlen(stringDef)+strlen(")") > 254)
					break;
				strcat(stringDef,")");
			}
			break;
		}
		return stringDef;
	}
};

//RampKey Methods
inline RampKey::RampKey()
{
	next = NULL;
	value = NULL;
	key = 0;
};

inline RampKey::~RampKey()
{
	delete value;
	delete next;
}

inline RampKey * RampKey::CreateCopy() const
{
	RampKey * ramp = new RampKey();
	ramp->key = key;
	if(value)
		ramp->value = value->CreateCopy();
	else
		ramp->value = NULL;
	if(next)
		ramp->next = next->CreateCopy();
	else
		ramp->next = NULL;
	return ramp;
}

#endif
