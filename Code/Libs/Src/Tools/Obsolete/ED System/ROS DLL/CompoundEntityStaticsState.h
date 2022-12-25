// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CompoundEntityStaticsState_h
#define CompoundEntityStaticsState_h
// --------------------------------------------------------------------------
//  CompoundEntityStaticsState
// --------------------------------------------------------------------------
#include "ACompoundSceneEntityState.h"
#include "Links.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class DACompoundObject;
// --------------------------------------------------------------------------
class CompoundEntityStaticsState: public ACompoundSceneEntityState
{
    public:
        explicit CompoundEntityStaticsState(const DACompoundObject* dACompoundObject = NULL);
        ~CompoundEntityStaticsState();

		void SetDACompoundObject(const DACompoundObject* dACompoundObject);
		const DACompoundObject* GetDACompoundObject() const;

        virtual Position GetPosition() const;
//       virtual Orientation GetOrientation() const;

        virtual void SetPosition(const Position& kPositionR);
//        virtual void SetOrientation(const Orientation& kPositionR);

        virtual void Write(std::ostream& oStreamR) const;
		virtual void Read(std::istream& iStreamR);

	private:
    	typedef AStaticsState BaseClass;

        void WriteSubObject(std::ostream& oStreamR) const;
		void ReadSubObject(std::istream& iStreamR);

        const DACompoundObject*	mDACompoundObject;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif