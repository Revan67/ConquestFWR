// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OrientationKeyPointStaticsState_h
#define OrientationKeyPointStaticsState_h
// --------------------------------------------------------------------------
#include "AStaticsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticSceneEntity;
// --------------------------------------------------------------------------
//  OrientationKeyPointStaticsState
// --------------------------------------------------------------------------
class CPP_DECL OrientationKeyPointStaticsState: public AStaticsState
{
    public:
        OrientationKeyPointStaticsState(AStaticSceneEntity& entity, unsigned int keyPointIndex);

		const AStaticSceneEntity& GetSceneEntity() const;
		AStaticSceneEntity& GetSceneEntity();

		unsigned int GetKeyPointIndex() const;
		void SetKeyPointIndex(unsigned int keyPointIndex);

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