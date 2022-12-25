#ifndef PARTICLEPARAMFUNCTIONS_H
#define PARTICLEPARAMFUNCTIONS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ParticleParamFunctions.h                    //
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

inline SINGLE getRandRange(SINGLE minVal, SINGLE maxVal)
{
	return	(
				( 
					(
						((SINGLE)(rand()%1000))/1000.0f
					) * (maxVal-minVal) 
				) + minVal
			);
}

//encoding functions
struct Particle;
struct IParticleSystem;
struct IMeshInstance;
namespace EncodeParam
{
	U32 WriteRampKeyType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,RampKey * value, U32 & floatIndex,U32 & rampIndex);
	U32 WriteFloatType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,FloatType * value, U32 & floatIndex,U32 & rampIndex);
	U32 WriteTransformType(EncodedTransformTypeEntry * transformList,EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,TransformType *value,
					   U32 & transformIndex,U32 & floatIndex,U32 & rampIndex);

	U32 FindNumFloatTypes(RampKey *value);
	U32 FindNumFloatTypes(FloatType *value);
	U32 FindNumFloatTypes(TransformType *value);
	U32 FindNumRampTypes(RampKey *value);
	U32 FindNumRampTypes(FloatType *value);
	U32 FindNumRampTypes(TransformType *value);
	U32 FindNumTransforms(TransformType * value);

	void EncodeFloat(EncodedFloatTypeHeader * header,FloatType * value);
	void EncodeTransform(EncodedTransformTypeHeader * header,TransformType * value);

	RampKey * CreateRampType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index);
	FloatType * CreateFloatType(EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index);
	TransformType * CreateTransformType(EncodedTransformTypeEntry *transList,EncodedFloatTypeEntry *floatList,EncodedRampEntry *rampList,U32 index);

	FloatType * DecodeFloat(EncodedFloatTypeHeader * header);
	TransformType * DecodeTransform(EncodedTransformTypeHeader * header);

	U32 EncodedFloatSize(FloatType * value);
	U32 EncodedTransformSize(TransformType * value);

	SINGLE FindRampKeyValue(U32 index, SINGLE value, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,Particle * p,IParticleSystem * parentSystem);
	SINGLE GetRampValue(U32 index, FloatType::RampType type, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,Particle * p,IParticleSystem * parentSystem,FloatType::Type baseType);
	SINGLE GetMaxRampValue(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem);
	SINGLE GetMinRampValue(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem);
	SINGLE GetMinFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem);
	SINGLE GetMaxFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList,IParticleSystem * parentSystem);
	SINGLE GetMaxFloat(EncodedFloatTypeHeader * header,IParticleSystem * parentSystem);
	SINGLE GetMinFloat(EncodedFloatTypeHeader * header,IParticleSystem * parentSystem);
	SINGLE GetFloat(U32 index, EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList, Particle * p,IParticleSystem * parentSystem);
	SINGLE GetFloat(EncodedFloatTypeHeader * header,Particle * p,IParticleSystem * parentSystem);
	bool FindObjectTransform(IParticleSystem * parentSystem,Particle *p, SINGLE t, EncodedTransformTypeEntry * transList,EncodedFloatTypeEntry * floatList,EncodedRampEntry * rampList, U32 index,Transform & trans,U32 instance);
	bool FindObjectTransform(IParticleSystem * parentSystem,Particle *p, SINGLE t, EncodedTransformTypeHeader * location, Transform & trans,U32 instance);
};
//end encoding functions
#endif
