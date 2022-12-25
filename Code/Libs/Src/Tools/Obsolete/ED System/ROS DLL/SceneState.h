// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneState_h
#define SceneState_h
// --------------------------------------------------------------------------
#include <iostream>

#include "Links.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
//	SceneState
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class APhysicalState;
class State;
// --------------------------------------------------------------------------
class SceneState
{
	public:
		void Write(std::ostream& oStream);
		void Read(std::istream& oStream);
		void Set(Time time);

	private :
		/**# :[Cardinalities = "0..n/"] */
        AggVector<APhysicalState*> mSceneEntityStateV;
        /**#: [Cardinalities = "1..1/"]*/
        AggPointer<State>	mStateP;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif