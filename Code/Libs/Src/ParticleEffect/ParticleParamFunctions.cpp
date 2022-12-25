#include "stdafx.h"
#include <windows.h>
#include <FDUMP.h>

#include "stdio.h"
#include "windows.h"
#include "ICamera.h"
#include <IParticleManager.h>
#include <stdlib.h>

#include "ParticleParamFunctions.h"
#include "ParticleSystem.h"

#define FEET_TO_WORLD 1.0f//0.0078125    // scale conversion for granny objects in the world
#define WORLD_TO_FEET 1.0f//(1.0/0.0078125)    // scale conversion for granny objects in the world

U32 encodedTransformStride = sizeof(EncodedTransformTypeEntry);

#define TRANS_LIST(TLIST,INDEX) (*( (EncodedTransformTypeEntry*)(((U8*)(TLIST))+(encodedTransformStride*(INDEX))) ))

U32 EncodeParam::WriteRampKeyType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,RampKey * value, U32 & floatIndex,U32 & rampIndex)
{
	U32 index = rampIndex;
	rampIndex++;
	rampList[index].key = value->key;
	rampList[index].value = WriteFloatType(floatList,rampList,value->value, floatIndex, rampIndex);
	if(value->next)
		rampList[index].next = WriteRampKeyType(floatList,rampList,value->next, floatIndex, rampIndex);
	else
		rampList[index].next = -1;
	return index;
}

U32 EncodeParam::WriteFloatType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,FloatType * value, U32 & floatIndex,U32 & rampIndex)
{
	U32 index = floatIndex;
	++floatIndex;
	if(value)
	{
		floatList[index].type = value->type;
		switch(value->type)
		{
		case FloatType::CONSTANT:
			{
				floatList[index].constant = value->constant;
			}
			break;
		case FloatType::CONST_RANGE:
		case FloatType::RANGE:
			{
				floatList[index].range.min = WriteFloatType(floatList,rampList,value->range.min, floatIndex, rampIndex);
				floatList[index].range.max = WriteFloatType(floatList,rampList,value->range.max, floatIndex, rampIndex);
			}
			break;
		case FloatType::PARAMETER:
			{
				strcpy(floatList[index].parameter.name,value->parameter.name);
				floatList[index].parameter.targetID = value->parameter.targetID;
			}
			break;
		case FloatType::LOOP_RAMP:
		case FloatType::OSCILATE_RAMP:
		case FloatType::RAMP:
			{
				floatList[index].ramp.type = value->ramp.type;
				floatList[index].ramp.firstKey = WriteRampKeyType(floatList,rampList,value->ramp.firstKey, floatIndex, rampIndex);
			}
			break;
		case FloatType::ADD:
			{
				floatList[index].add.value1 = WriteFloatType(floatList,rampList,value->add.value1, floatIndex, rampIndex);
				floatList[index].add.value2 = WriteFloatType(floatList,rampList,value->add.value2, floatIndex, rampIndex);
			}
			break;
		case FloatType::SUBTRACT:
			{
				floatList[index].subtract.value1 = WriteFloatType(floatList,rampList,value->subtract.value1, floatIndex, rampIndex);
				floatList[index].subtract.value2 = WriteFloatType(floatList,rampList,value->subtract.value2, floatIndex, rampIndex);
			}
			break;
		case FloatType::MULTIPLY:
			{
				floatList[index].multiply.value1 = WriteFloatType(floatList,rampList,value->multiply.value1, floatIndex, rampIndex);
				floatList[index].multiply.value2 = WriteFloatType(floatList,rampList,value->multiply.value2, floatIndex, rampIndex);
			}
			break;
		case FloatType::DIVIDE:
			{
				floatList[index].divide.value1 = WriteFloatType(floatList,rampList,value->divide.value1, floatIndex, rampIndex);
				floatList[index].divide.value2 = WriteFloatType(floatList,rampList,value->divide.value2, floatIndex, rampIndex);
			}
			break;
		}
	}
	else //default to zero
	{
		floatList[index].type = FloatType::CONSTANT;
		floatList[index].constant = 0;
	}
	return index;
};

U32 EncodeParam::WriteTransformType(EncodedTransformTypeEntry * transformList,EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,TransformType *value,
					   U32 & transformIndex,U32 & floatIndex,U32 & rampIndex)
{
	U32 index = transformIndex;
	++transformIndex;
	transformList[index].type = value->type;
	switch(value->type)
	{
	case TransformType::UP:
	case TransformType::CAMERA:
	case TransformType::CAMERA_LOOK:
		break;
	case TransformType::TARGET_TRANSFORM:
		{
			transformList[index].targetTrans.targetID = value->targetTrans.targetID;
			transformList[index].targetTrans.hpID = value->targetTrans.hpID;
		}
		break;
	case TransformType::TARGET_TRANSFORM_STR:
		{
			transformList[index].targetTrans_str.targetID = value->targetTrans_str.targetID;
			strcpy(transformList[index].targetTrans_str.hpName,value->targetTrans_str.hpName);
		}
		break;
	case TransformType::FILTER_EFFECT:
		{
			strcpy(transformList[index].filterName,value->filterName);
		}
		break;
	case TransformType::OFFSET:
		{
			transformList[index].offset.offX = WriteFloatType(floatList,rampList,value->offset.offX,floatIndex,rampIndex);
			transformList[index].offset.offY = WriteFloatType(floatList,rampList,value->offset.offY,floatIndex,rampIndex);
			transformList[index].offset.offZ = WriteFloatType(floatList,rampList,value->offset.offZ,floatIndex,rampIndex);
			transformList[index].offset.baseTrans = WriteTransformType(transformList,floatList,rampList,value->offset.baseTrans,transformIndex,floatIndex,rampIndex);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			transformList[index].offsetIJK.offI = WriteFloatType(floatList,rampList,value->offsetIJK.offI,floatIndex,rampIndex);
			transformList[index].offsetIJK.offJ = WriteFloatType(floatList,rampList,value->offsetIJK.offJ,floatIndex,rampIndex);
			transformList[index].offsetIJK.offK = WriteFloatType(floatList,rampList,value->offsetIJK.offK,floatIndex,rampIndex);
			transformList[index].offsetIJK.baseTrans = WriteTransformType(transformList,floatList,rampList,value->offsetIJK.baseTrans,transformIndex,floatIndex,rampIndex);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			transformList[index].rotateIJK.rotI = WriteFloatType(floatList,rampList,value->rotateIJK.rotI,floatIndex,rampIndex);
			transformList[index].rotateIJK.rotJ = WriteFloatType(floatList,rampList,value->rotateIJK.rotJ,floatIndex,rampIndex);
			transformList[index].rotateIJK.rotK = WriteFloatType(floatList,rampList,value->rotateIJK.rotK,floatIndex,rampIndex);
			transformList[index].rotateIJK.baseTrans = WriteTransformType(transformList,floatList,rampList,value->rotateIJK.baseTrans,transformIndex,floatIndex,rampIndex);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			transformList[index].lookAtUp.look = WriteTransformType(transformList,floatList,rampList,value->lookAtUp.look,transformIndex,floatIndex,rampIndex);
			transformList[index].lookAtUp.at = WriteTransformType(transformList,floatList,rampList,value->lookAtUp.at,transformIndex,floatIndex,rampIndex);
			transformList[index].lookAtUp.up = WriteTransformType(transformList,floatList,rampList,value->lookAtUp.up,transformIndex,floatIndex,rampIndex);
		}
		break;
	case TransformType::INPUT_POINT:
		break;
	}
	return index;
}

U32 EncodeParam::FindNumFloatTypes(RampKey *value)
{
	if(!value)
		return 0;
	return FindNumFloatTypes(value->next) + FindNumFloatTypes(value->value);
}

U32 EncodeParam::FindNumFloatTypes(FloatType *value)
{
	if(!value)
		return 1;// default of constant zero
	switch(value->type)
	{
	case FloatType::PARAMETER:
	case FloatType::CONSTANT:
		{
			return 1;
		}
		break;
	case FloatType::CONST_RANGE:
	case FloatType::RANGE:
		{
			return 1+ FindNumFloatTypes(value->range.min) + FindNumFloatTypes(value->range.max);
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			return 1+FindNumFloatTypes(value->ramp.firstKey);
		}
		break;
	case FloatType::ADD:
		{
			return 1+ FindNumFloatTypes(value->add.value1) + FindNumFloatTypes(value->add.value2);
		}
		break;
	case FloatType::SUBTRACT:
		{
			return 1+ FindNumFloatTypes(value->subtract.value1) + FindNumFloatTypes(value->subtract.value2);
		}
		break;
	case FloatType::MULTIPLY:
		{
			return 1+ FindNumFloatTypes(value->multiply.value1) + FindNumFloatTypes(value->multiply.value2);
		}
		break;
	case FloatType::DIVIDE:
		{
			return 1+ FindNumFloatTypes(value->divide.value1) + FindNumFloatTypes(value->divide.value2);
		}
		break;
	}
	return 1;
};

U32 EncodeParam::FindNumFloatTypes(TransformType *value)
{
	switch(value->type)
	{
	case TransformType::OFFSET:
		{
			return FindNumFloatTypes(value->offset.offX) + FindNumFloatTypes(value->offset.offY) + FindNumFloatTypes(value->offset.offZ) + FindNumFloatTypes(value->offset.baseTrans);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			return FindNumFloatTypes(value->offsetIJK.offI) + FindNumFloatTypes(value->offsetIJK.offJ) + FindNumFloatTypes(value->offsetIJK.offK) + FindNumFloatTypes(value->offsetIJK.baseTrans);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			return FindNumFloatTypes(value->rotateIJK.rotI) + FindNumFloatTypes(value->rotateIJK.rotJ) + FindNumFloatTypes(value->rotateIJK.rotK) + FindNumFloatTypes(value->rotateIJK.baseTrans);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			return FindNumFloatTypes(value->lookAtUp.look) + FindNumFloatTypes(value->lookAtUp.at) + FindNumFloatTypes(value->lookAtUp.up);
		}
		break;
	}
	return 0;
}

U32 EncodeParam::FindNumRampTypes(RampKey *value)
{
	if(!value)
		return 0;
	return 1+ FindNumRampTypes(value->next)+FindNumRampTypes(value->value);
}

U32 EncodeParam::FindNumRampTypes(FloatType *value)
{
	if(!value)
		return 0;
	switch(value->type)
	{
	case FloatType::PARAMETER:
	case FloatType::CONSTANT:
		{
			return 0;
		}
		break;
	case FloatType::CONST_RANGE:
	case FloatType::RANGE:
		{
			return FindNumRampTypes(value->range.min) + FindNumRampTypes(value->range.max);
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			return FindNumRampTypes(value->ramp.firstKey);
		}
		break;
	case FloatType::ADD:
		{
			return FindNumRampTypes(value->add.value1) + FindNumRampTypes(value->add.value2);
		}
		break;
	case FloatType::SUBTRACT:
		{
			return FindNumRampTypes(value->subtract.value1) + FindNumRampTypes(value->subtract.value2);
		}
		break;
	case FloatType::MULTIPLY:
		{
			return FindNumRampTypes(value->multiply.value1) + FindNumRampTypes(value->multiply.value2);
		}
		break;
	case FloatType::DIVIDE:
		{
			return FindNumRampTypes(value->divide.value1) + FindNumRampTypes(value->divide.value2);
		}
		break;
	}
	return 0;
};

U32 EncodeParam::FindNumRampTypes(TransformType *value)
{
	switch(value->type)
	{
	case TransformType::OFFSET:
		{
			return FindNumRampTypes(value->offset.offX) + FindNumRampTypes(value->offset.offY) + FindNumRampTypes(value->offset.offZ) + FindNumRampTypes(value->offset.baseTrans);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			return FindNumRampTypes(value->offsetIJK.offI) + FindNumRampTypes(value->offsetIJK.offJ) + FindNumRampTypes(value->offsetIJK.offK) + FindNumRampTypes(value->offsetIJK.baseTrans);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			return FindNumRampTypes(value->rotateIJK.rotI) + FindNumRampTypes(value->rotateIJK.rotJ) + FindNumRampTypes(value->rotateIJK.rotK) + FindNumRampTypes(value->rotateIJK.baseTrans);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			return FindNumRampTypes(value->lookAtUp.look) + FindNumRampTypes(value->lookAtUp.at) + FindNumRampTypes(value->lookAtUp.up);
		}
		break;
	}
	return 0;
}

U32 EncodeParam::FindNumTransforms(TransformType * value)
{
	switch(value->type)
	{
	case TransformType::OFFSET:
		{
			return FindNumTransforms(value->offset.baseTrans)+1;
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			return FindNumTransforms(value->offsetIJK.baseTrans)+1;
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			return FindNumTransforms(value->rotateIJK.baseTrans)+1;
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			return FindNumTransforms(value->lookAtUp.look)+FindNumTransforms(value->lookAtUp.at)+FindNumTransforms(value->lookAtUp.up)+1;
		}
		break;
	}
	return 1;//for now this is the only way
}

void EncodeParam::EncodeFloat(EncodedFloatTypeHeader * header,FloatType * value)
{
	//first build up the list of float entries
	header->floatEntryOffset = sizeof(EncodedFloatTypeHeader);
	//now build up the list of ramps
	header->rampOffset = header->floatEntryOffset + FindNumFloatTypes(value)*sizeof(EncodedFloatTypeEntry);

	header->size = header->rampOffset + FindNumRampTypes(value)*sizeof(EncodedRampEntry);

	//now fill out the lists
	U32 rampIndex = 0;
	U32 floatIndex = 0;
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	WriteFloatType(floatList,rampList,value,floatIndex,rampIndex);
};

void EncodeParam::EncodeTransform(EncodedTransformTypeHeader * header,TransformType * value)
{
	header->transformEntryOffset = sizeof(EncodedTransformTypeHeader);
	//build up the list of float entries
	header->floatEntryOffset = header->transformEntryOffset + FindNumTransforms(value)*sizeof(EncodedTransformTypeEntry);
	//now build up the list of ramps
	header->rampOffset = header->floatEntryOffset + FindNumFloatTypes(value)*sizeof(EncodedFloatTypeEntry);

	header->size = header->rampOffset + FindNumRampTypes(value)*sizeof(EncodedRampEntry);

	//now fill out the lists
	U32 rampIndex = 0;
	U32 floatIndex = 0;
	U32 transformIndex = 0;
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	EncodedTransformTypeEntry * transformList = (EncodedTransformTypeEntry *) (((U8*)header) + header->transformEntryOffset);
	WriteTransformType(transformList,floatList,rampList,value,transformIndex,floatIndex,rampIndex);
};

RampKey * EncodeParam::CreateRampType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index)
{
	if(index == -1)
		return NULL;
	RampKey * retVal = new RampKey();
	retVal->key = rampList[index].key;
	retVal->value = CreateFloatType(floatList,rampList,rampList[index].value);
	retVal->next = CreateRampType(floatList,rampList,rampList[index].next);
	return retVal;
}

FloatType * EncodeParam::CreateFloatType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index)
{
	FloatType * retVal = new FloatType();
	retVal->type = (FloatType::Type)(floatList[index].type);
	switch(retVal->type)
	{
	case FloatType::PARAMETER:
		{
			strcpy(retVal->parameter.name,floatList[index].parameter.name);
			retVal->parameter.targetID = floatList[index].parameter.targetID;
		}
		break;
	case FloatType::CONSTANT:
		{
			retVal->constant = floatList[index].constant;
		}
		break;
	case FloatType::CONST_RANGE:
	case FloatType::RANGE:
		{
			retVal->range.min = CreateFloatType(floatList,rampList,floatList[index].range.min);
			retVal->range.max = CreateFloatType(floatList,rampList,floatList[index].range.max);
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			retVal->ramp.type = (FloatType::RampType)(floatList[index].ramp.type);
			retVal->ramp.firstKey = CreateRampType(floatList,rampList,floatList[index].ramp.firstKey);
		}
		break;
	case FloatType::ADD:
		{
			retVal->add.value1 = CreateFloatType(floatList,rampList,floatList[index].add.value1);
			retVal->add.value2 = CreateFloatType(floatList,rampList,floatList[index].add.value2);
		}
		break;
	case FloatType::SUBTRACT:
		{
			retVal->subtract.value1 = CreateFloatType(floatList,rampList,floatList[index].subtract.value1);
			retVal->subtract.value2 = CreateFloatType(floatList,rampList,floatList[index].subtract.value2);
		}
		break;
	case FloatType::MULTIPLY:
		{
			retVal->multiply.value1 = CreateFloatType(floatList,rampList,floatList[index].multiply.value1);
			retVal->multiply.value2 = CreateFloatType(floatList,rampList,floatList[index].multiply.value2);
		}
		break;
	case FloatType::DIVIDE:
		{
			retVal->divide.value1 = CreateFloatType(floatList,rampList,floatList[index].divide.value1);
			retVal->divide.value2 = CreateFloatType(floatList,rampList,floatList[index].divide.value2);
		}
		break;
	}

	return retVal;
}

FloatType * EncodeParam::DecodeFloat(EncodedFloatTypeHeader * header)
{
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	return CreateFloatType(floatList,rampList,0);
}

TransformType * EncodeParam::CreateTransformType(EncodedTransformTypeEntry *transList,EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index)
{
	TransformType * retVal = new TransformType;
	retVal->type = (TransformType::Type)(TRANS_LIST(transList,index).type);
	switch(retVal->type)
	{
	case TransformType::UP:
	case TransformType::CAMERA:
	case TransformType::CAMERA_LOOK:
		break;
	case TransformType::TARGET_TRANSFORM:
		{
			retVal->targetTrans.targetID = TRANS_LIST(transList,index).targetTrans.targetID;
			retVal->targetTrans.hpID = TRANS_LIST(transList,index).targetTrans.hpID ;
		}
		break;
	case TransformType::TARGET_TRANSFORM_STR:
		{
			retVal->targetTrans_str.targetID = TRANS_LIST(transList,index).targetTrans_str.targetID;
			strcpy(retVal->targetTrans_str.hpName,TRANS_LIST(transList,index).targetTrans_str.hpName) ;
		}
		break;
	case TransformType::FILTER_EFFECT:
		{
			strcpy(retVal->filterName,TRANS_LIST(transList,index).filterName);
		}
		break;
	case TransformType::OFFSET:
		{
			retVal->offset.offX = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offset.offX);
			retVal->offset.offY = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offset.offY);
			retVal->offset.offZ = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offset.offZ);
			retVal->offset.baseTrans = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).offset.baseTrans);
		}
		break;
	case TransformType::OFFSET_IJK:
		{
			retVal->offsetIJK.offI = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offsetIJK.offI);
			retVal->offsetIJK.offJ = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offsetIJK.offJ);
			retVal->offsetIJK.offK = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).offsetIJK.offK);
			retVal->offsetIJK.baseTrans = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).offsetIJK.baseTrans);
		}
		break;
	case TransformType::ROTATE_IJK:
		{
			retVal->rotateIJK.rotI = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).rotateIJK.rotI);
			retVal->rotateIJK.rotJ = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).rotateIJK.rotJ);
			retVal->rotateIJK.rotK = CreateFloatType(floatList,rampList,TRANS_LIST(transList,index).rotateIJK.rotK);
			retVal->rotateIJK.baseTrans = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).rotateIJK.baseTrans);
		}
		break;
	case TransformType::LOOK_AT_UP:
		{
			retVal->lookAtUp.look = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.look);
			retVal->lookAtUp.at = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.at);
			retVal->lookAtUp.up = CreateTransformType(transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.up);
		}
		break;
	}
	return retVal;
}

TransformType * EncodeParam::DecodeTransform(EncodedTransformTypeHeader * header)
{
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	EncodedTransformTypeEntry * transformList = (EncodedTransformTypeEntry *) (((U8*)header) + header->transformEntryOffset);
	return CreateTransformType(transformList,floatList,rampList,0);
}

U32 EncodeParam::EncodedFloatSize(FloatType * value)
{
	return sizeof(EncodedFloatTypeHeader) + FindNumFloatTypes(value)*sizeof(EncodedFloatTypeEntry) + FindNumRampTypes(value)*sizeof(EncodedRampEntry);
}

U32 EncodeParam::EncodedTransformSize(TransformType * value)
{
	return sizeof(EncodedTransformTypeHeader) + FindNumTransforms(value)*sizeof(EncodedTransformTypeEntry) + 
		FindNumFloatTypes(value)*sizeof(EncodedFloatTypeEntry) + FindNumRampTypes(value)*sizeof(EncodedRampEntry);
}

SINGLE EncodeParam::FindRampKeyValue(U32 index, SINGLE value, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,Particle * p,IParticleSystem * parentSystem)
{
	EncodedRampEntry * key = &(rampList[index]);
	EncodedRampEntry * prev = NULL;
	while(key->next != -1 && key->key < value)
	{
		prev = key;
		key = &(rampList[key->next]);
	}
	if(key->key < value || prev == NULL)
		return GetFloat(key->value,floatList,rampList,p,parentSystem);
	else
	{
		SINGLE num1 = GetFloat(key->value,floatList,rampList,p,parentSystem);
		SINGLE num2 = GetFloat(prev->value,floatList,rampList,p,parentSystem);
		SINGLE t = (value-prev->key)/(key->key - prev->key);
		return t*(num1-num2)+num2;
	}
	return 0;
}

SINGLE EncodeParam::GetRampValue(U32 index, FloatType::RampType type, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,Particle * p,IParticleSystem * parentSystem,FloatType::Type baseType)
{
	SINGLE value = 0;
	switch(type)
	{
	case FloatType::LIFETIME:
		{
			value = ((SINGLE)(parentSystem->GetCurrentTimeMS()-p->birthTime))/1000.0f;
		}
		break;
	case FloatType::SPEED:
		{
			value = p->vel.magnitude_squared();
		}
		break;
	case FloatType::LIFETIME_PERCENT:
		{
			if(p->lifeTime == 0)
				value = 1;
			else
				value = ((SINGLE)(parentSystem->GetCurrentTimeMS()-p->birthTime))/((SINGLE)(p->lifeTime));
		}
		break;
	case FloatType::EFFECT_LIFETIME:
		{
			value = ((SINGLE)(parentSystem->GetCurrentTimeMS()-parentSystem->StartTime()))/1000.0f;
		}
		break;
	}

	if(baseType == FloatType::LOOP_RAMP)
	{
		bool bHasKey = false;
		EncodedRampEntry * key = &(rampList[index]);
		SINGLE minKey = key->key;
		while(key->next != -1)
		{
			key = &(rampList[key->next]);
		}
		SINGLE maxKey = key->key;
		
		if(value < minKey || value > maxKey)
		{
			SINGLE range = maxKey-minKey;
			if(range)
			{
				value = value - (range*((SINGLE)(floor((value-minKey)/range))));
			}
		}
	}
	else if(baseType == FloatType::OSCILATE_RAMP)
	{
		bool bHasKey = false;
		EncodedRampEntry * key = &(rampList[index]);
		SINGLE minKey = key->key;
		while(key->next != -1)
		{
			key = &(rampList[key->next]);
		}
		SINGLE maxKey = key->key;
		
		if(value < minKey || value > maxKey)
		{
			SINGLE range = maxKey-minKey;
			if(range)
			{
				S32 div = (S32)(floor((value-minKey)/range));
				if(!(div& 0x0001))
					value = value - (range*div);
				else
					value = (range-((value - (range*div))-minKey))+minKey;
			}
		}
	}
	return FindRampKeyValue(index,value,floatList,rampList,p,parentSystem);
};

SINGLE EncodeParam::GetMaxRampValue(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem)
{
	SINGLE maxValue = 0;
	bool bHasValue = false;
	while(index != -1)
	{
		EncodedRampEntry * key = &(rampList[index]);
		SINGLE testVal = GetMaxFloat(key->value,floatList,rampList,parentSystem);
		if(bHasValue)
		{
			if(maxValue < testVal)
			{
				maxValue = testVal;
			}
		}
		else
		{
			maxValue = testVal;
			bHasValue = true;
		}
		index = key->next;
	}

	return maxValue;
};

SINGLE EncodeParam::GetMinRampValue(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem)
{
	SINGLE maxValue = 0;
	bool bHasValue = false;
	while(index != -1)
	{
		EncodedRampEntry * key = &(rampList[index]);
		SINGLE testVal = GetMaxFloat(key->value,floatList,rampList,parentSystem);
		if(bHasValue)
		{
			if(maxValue > testVal)
			{
				maxValue = testVal;
			}
		}
		else
		{
			maxValue = testVal;
			bHasValue = true;
		}
		index = key->next;
	}

	return maxValue;
};

SINGLE EncodeParam::GetFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,Particle * p,IParticleSystem * parentSystem)
{
	switch(floatList[index].type)
	{
	case FloatType::CONSTANT:
		{
			return floatList[index].constant;
		}
		break;
	case FloatType::CONST_RANGE:
		{
			SINGLE minVal = GetFloat(floatList[index].range.min,floatList,rampList,p,parentSystem);
			SINGLE maxVal = GetFloat(floatList[index].range.max,floatList,rampList,p,parentSystem);
			SINGLE t = ((SINGLE)((((U32)(p->birthTime))*324234)%1111))/1111.0f;
			return t*(minVal-maxVal)+maxVal;
		}
		break;
	case FloatType::RANGE:
		{
			return getRandRange(GetFloat(floatList[index].range.min,floatList,rampList,p,parentSystem),GetFloat(floatList[index].range.max,floatList,rampList,p,parentSystem));
		}
		break;
	case FloatType::PARAMETER:
		{
			return parentSystem->GetOwner()->GetCallback()->GetParameter(floatList[index].parameter.name,floatList[index].parameter.targetID,parentSystem->GetContext());
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			return GetRampValue(floatList[index].ramp.firstKey,(FloatType::RampType)(floatList[index].ramp.type),floatList,rampList,p,parentSystem,(FloatType::Type)(floatList[index].type));
		}
		break;
	case FloatType::ADD:
		{
			return GetFloat(floatList[index].add.value1,floatList,rampList,p,parentSystem) + GetFloat(floatList[index].add.value2,floatList,rampList,p,parentSystem);
		}
		break;
	case FloatType::SUBTRACT:
		{
			return GetFloat(floatList[index].subtract.value1,floatList,rampList,p,parentSystem) - GetFloat(floatList[index].subtract.value2,floatList,rampList,p,parentSystem);
		}
		break;
	case FloatType::MULTIPLY:
		{
			return GetFloat(floatList[index].multiply.value1,floatList,rampList,p,parentSystem) * GetFloat(floatList[index].multiply.value2,floatList,rampList,p,parentSystem);
		}
		break;
	case FloatType::DIVIDE:
		{
			return GetFloat(floatList[index].divide.value1,floatList,rampList,p,parentSystem) / GetFloat(floatList[index].divide.value2,floatList,rampList,p,parentSystem);
		}
		break;

	};
	return 0;
}

SINGLE EncodeParam::GetMinFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem)
{
	switch(floatList[index].type)
	{
	case FloatType::CONSTANT:
		{
			return floatList[index].constant;
		}
		break;
	case FloatType::CONST_RANGE:
	case FloatType::RANGE:
		{
			return GetMinFloat(floatList[index].range.min,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::PARAMETER:
		{
			return parentSystem->GetOwner()->GetCallback()->GetParameter(floatList[index].parameter.name,floatList[index].parameter.targetID,parentSystem->GetContext());
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			return GetMinRampValue(floatList[index].ramp.firstKey,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::ADD:
		{
			return GetMinFloat(floatList[index].add.value1,floatList,rampList,parentSystem) + GetMinFloat(floatList[index].add.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::SUBTRACT:
		{
			return GetMinFloat(floatList[index].subtract.value1,floatList,rampList,parentSystem) - GetMaxFloat(floatList[index].subtract.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::MULTIPLY:
		{
			return GetMinFloat(floatList[index].multiply.value1,floatList,rampList,parentSystem) * GetMinFloat(floatList[index].multiply.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::DIVIDE:
		{
			return GetMinFloat(floatList[index].divide.value1,floatList,rampList,parentSystem) / GetMaxFloat(floatList[index].divide.value2,floatList,rampList,parentSystem);
		}
		break;
	};
	return 0;
}

SINGLE EncodeParam::GetMaxFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem)
{
	switch(floatList[index].type)
	{
	case FloatType::CONSTANT:
		{
			return floatList[index].constant;
		}
		break;
	case FloatType::CONST_RANGE:
	case FloatType::RANGE:
		{
			return GetMaxFloat(floatList[index].range.max,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::PARAMETER:
		{
			return parentSystem->GetOwner()->GetCallback()->GetParameter(floatList[index].parameter.name,floatList[index].parameter.targetID,parentSystem->GetContext());
		}
		break;
	case FloatType::LOOP_RAMP:
	case FloatType::OSCILATE_RAMP:
	case FloatType::RAMP:
		{
			return GetMaxRampValue(floatList[index].ramp.firstKey,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::ADD:
		{
			return GetMaxFloat(floatList[index].add.value1,floatList,rampList,parentSystem) + GetMaxFloat(floatList[index].add.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::SUBTRACT:
		{
			return GetMaxFloat(floatList[index].subtract.value1,floatList,rampList,parentSystem) - GetMinFloat(floatList[index].subtract.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::MULTIPLY:
		{
			return GetMaxFloat(floatList[index].multiply.value1,floatList,rampList,parentSystem) * GetMaxFloat(floatList[index].multiply.value2,floatList,rampList,parentSystem);
		}
		break;
	case FloatType::DIVIDE:
		{
			return GetMaxFloat(floatList[index].divide.value1,floatList,rampList,parentSystem) / GetMinFloat(floatList[index].divide.value2,floatList,rampList,parentSystem);
		}
		break;
	};
	return 0;
}

SINGLE EncodeParam::GetFloat(EncodedFloatTypeHeader * header,Particle * p,IParticleSystem * parentSystem)
{
	if (header == 0) return 0;
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	return GetFloat(0,floatList,rampList,p,parentSystem);
}

SINGLE EncodeParam::GetMaxFloat(EncodedFloatTypeHeader * header,IParticleSystem * parentSystem)
{
	if (header == 0) return 0;
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	return GetMaxFloat(0,floatList,rampList,parentSystem);
}

SINGLE EncodeParam::GetMinFloat(EncodedFloatTypeHeader * header,IParticleSystem * parentSystem)
{
	if (header == 0) return 0;
	EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)header) + header->floatEntryOffset);
	EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)header) + header->rampOffset);
	return GetMinFloat(0,floatList,rampList,parentSystem);
}

bool EncodeParam::FindObjectTransform(IParticleSystem * parentSystem,Particle *p, SINGLE t, EncodedTransformTypeEntry * transList,
									  EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList, U32 index,Transform & trans, U32 instance)
{
	switch(TRANS_LIST(transList,index).type)
	{
		case TransformType::TARGET_TRANSFORM:
		{
			return parentSystem->GetOwner()->GetCallback()->GetObjectTransform(TRANS_LIST(transList,index).targetTrans.targetID,TRANS_LIST(transList,index).targetTrans.hpID,parentSystem->GetContext(),trans);
			break;
		}
		case TransformType::TARGET_TRANSFORM_STR:
		{
			return parentSystem->GetOwner()->GetCallback()->GetObjectTransform(TRANS_LIST(transList,index).targetTrans_str.targetID,
				TRANS_LIST(transList,index).targetTrans_str.hpName,parentSystem->GetContext(),trans);
			break;
		}
		case TransformType::FILTER_EFFECT:
		{
			IParticleEffectInstance * targetSource = parentSystem->FindFilter(TRANS_LIST(transList,index).filterName);
			if(targetSource)
			{
				targetSource->GetTransform(t,trans,instance);
				return true;
			}
			break;
		}
		case TransformType::UP:
		{
			trans.set_identity();
			return true;
			break;
		}
		case TransformType::CAMERA:
		{
			trans = parentSystem->GetOwner()->GetCamera()->get_transform();
			return true;
			break;
		}
		case TransformType::CAMERA_LOOK:
		{
			trans = parentSystem->GetOwner()->GetCamera()->get_transform();
			Vector dir = -trans.get_k();
			if(dir.z > 0.001)
			{
				trans.translation = trans.translation+(dir*(trans.translation.z/dir.z)) ;
			}
			else
			{
				trans.translation = Vector(0,0,0);
			}
			return true;
			break;
		}
		case TransformType::OFFSET:
		{
			if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).offset.baseTrans,trans,instance))
			{
				trans.translation.x += GetFloat(TRANS_LIST(transList,index).offset.offX,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				trans.translation.y += GetFloat(TRANS_LIST(transList,index).offset.offY,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				trans.translation.z += GetFloat(TRANS_LIST(transList,index).offset.offZ,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				return true;
			}
			break;
		}
		case TransformType::OFFSET_IJK:
		{
			if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).offsetIJK.baseTrans,trans,instance))
			{
				Transform orth = trans;
				orth.make_orthogonal();
				trans.translation += orth.get_i()*GetFloat(TRANS_LIST(transList,index).offsetIJK.offI,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				trans.translation += orth.get_j()*GetFloat(TRANS_LIST(transList,index).offsetIJK.offJ,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				trans.translation += orth.get_k()*GetFloat(TRANS_LIST(transList,index).offsetIJK.offK,floatList,rampList,p,parentSystem)*FEET_TO_WORLD;
				return true;
			}
			break;
		}
		case TransformType::ROTATE_IJK:
		{
			if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).rotateIJK.baseTrans,trans,instance))
			{
				trans.rotate_around_i(GetFloat(TRANS_LIST(transList,index).rotateIJK.rotI,floatList,rampList,p,parentSystem)*MUL_DEG_TO_RAD);
				trans.rotate_around_j(GetFloat(TRANS_LIST(transList,index).rotateIJK.rotJ,floatList,rampList,p,parentSystem)*MUL_DEG_TO_RAD);
				trans.rotate_around_k(GetFloat(TRANS_LIST(transList,index).rotateIJK.rotK,floatList,rampList,p,parentSystem)*MUL_DEG_TO_RAD);
				return true;
			}
			break;
		}
		case TransformType::LOOK_AT_UP:
		{
			Transform look;
			Transform at;
			Transform up;
			if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.look,look,instance))
			{
				if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.at,at,instance))
				{
					if(FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,TRANS_LIST(transList,index).lookAtUp.up,up,instance))
					{
						Vector j = look.translation-at.translation;
						if(j.x == 0 && j.y == 0 && j.z == 0)
							j = Vector(0,1,0);
						else
							j.fast_normalize();
						Vector k = up.translation-at.translation;
						if(k.x == 0 && k.y == 0 && k.z == 0)
							k = Vector(0,0,1);
						else
							k.fast_normalize();

						Vector i = cross_product(j,k);
						if(i.x == 0 && i.y == 0 && i.z == 0)
							i = Vector(1,0,0);
						else
							i.fast_normalize();

						k = cross_product(i,j);
						k.fast_normalize();

						trans.set_i(i);
						trans.set_j(j);
						trans.set_k(k);
						trans.translation = at.translation;

						return true;
					}
				}
			}
			break;
		}
		case TransformType::INPUT_POINT:
		{
			if(!p->bLive)
				return false;
			Vector k = p->vel;
			if(p->vel == Vector(0,0,0))
			{
				k = Vector(0,1,0);
			}
			k.fast_normalize();
			Vector i;
			if(k == Vector(0,0,1))
				i = Vector(1,0,0);
			else
				i = Vector(0,0,1);
			Vector j = cross_product(k,i);
			j.fast_normalize();
			i = cross_product(j,k);
			i.fast_normalize();
			trans.set_identity();
			trans.set_i(i);
			trans.set_j(j);
			trans.set_k(k);
			trans.translation = p->pos;
			return true;
			break;
		}
		
	}
	return false;
}

bool EncodeParam::FindObjectTransform(IParticleSystem * parentSystem,Particle *p, SINGLE t, EncodedTransformTypeHeader * location, Transform & trans,U32 instance)
{
	if(location)
	{
		EncodedTransformTypeEntry * transList = (EncodedTransformTypeEntry*) (((U8*)location) + location->transformEntryOffset);
		EncodedFloatTypeEntry * floatList = (EncodedFloatTypeEntry *) (((U8*)location) + location->floatEntryOffset);
		EncodedRampEntry * rampList = (EncodedRampEntry *) (((U8*)location) + location->rampOffset);

		return FindObjectTransform(parentSystem,p,t,transList,floatList,rampList,0,trans,instance);
	}
	return false;
}



