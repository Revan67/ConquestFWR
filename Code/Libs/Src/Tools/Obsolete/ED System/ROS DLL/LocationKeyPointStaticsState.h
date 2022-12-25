// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LocationKeyPointStaticsState_h
#define LocationKeyPointStaticsState_h
// --------------------------------------------------------------------------
#include "AStaticsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticSceneEntity;
// --------------------------------------------------------------------------
//  LocationKeyPointStaticsState
// --------------------------------------------------------------------------
class CPP_DECL LocationKeyPointStaticsState: public AStaticsState
{
    public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

        LocationKeyPointStaticsState(AStaticSceneEntity& entity, unsigned int keyPointIndex);

		const AStaticSceneEntity& GetSceneEntity() const;
		AStaticSceneEntity& GetSceneEntity();

		unsigned int GetKeyPointIndex() const;
		void SetKeyPointIndex(unsigned int keyPointIndex);

		InterpolationType GetInterpolationType() const; 

        virtual Position GetPosition() const;
        virtual void SetPosition(const Position& position);

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    private:
        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

        typedef AStaticsState BaseClass;

        AStaticSceneEntity&	mStaticSceneEntity;
		unsigned int		mKeyPointIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif