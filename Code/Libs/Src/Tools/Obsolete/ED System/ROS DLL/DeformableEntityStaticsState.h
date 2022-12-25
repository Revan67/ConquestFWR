// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DeformableEntityStaticsState_h
#define DeformableEntityStaticsState_h
// --------------------------------------------------------------------------
//  DeformableEntityStaticsState
// --------------------------------------------------------------------------
#include "ACompoundSceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class DADeformableObject;
// --------------------------------------------------------------------------
class DeformableEntityStaticsState : public ACompoundSceneEntityState
{
    public:
        DeformableEntityStaticsState(const DADeformableObject* dADeformableObject);
        virtual ~DeformableEntityStaticsState();
        
		void SetDADeformableObject(const DADeformableObject* dADeformableObject);
        const DADeformableObject* GetDADeformableObject() const;

        virtual Position GetPosition() const;

        virtual void SetPosition(const Position& position);

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
    	typedef ACompoundSceneEntityState BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

	    const DADeformableObject* mDADeformableObject;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif