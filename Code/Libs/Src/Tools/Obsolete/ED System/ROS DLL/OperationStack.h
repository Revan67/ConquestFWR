// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OperationStack_h
#define OperationStack_h
// --------------------------------------------------------------------------
#include <deque>
#include "StringType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AOperation;
class ASceneEntity;
// --------------------------------------------------------------------------
class OperationStack
{
	public:
		// The maximumOperations parameter specifies the maximum number of 
		// operations that the stack will hold. Once the stack has the
		// maximum number of operations and an additional operation is pushed
		// onto the stack, one operation at the bottom of the stack is 
		// removed and deleted.
		OperationStack(unsigned int maximumOperations);

		// Deletes all operations on the stack, starting with the one on the
		// top of the stack
		~OperationStack();

		// Push the operation onto the top of the stack When no longer needed,
		// the operation will be eliminated with a call to delete. Thus, it 
		// is important that the opertation be created with a call to new.
		void PushOperation(AOperation* operation);
		
		// Returns true if the stack is empty; false otherwise.
		bool IsEmpty() const;

		// Returns the name of the top operation. Ensure that the stack is 
		// not empty before invoking this method.
		ROS::ROSString GetTopOperationName() const;

		// Removes the top operation, invokes Perform() on it, and finally
		// deletes the operation. Returns the inverse of this operation. The
		// caller is responsible for deleting the returned instance.
		AOperation* PerformTopOperationAndRemove();

		// Removes operations for the specified entity
		void RemoveOperations(const ROS::ASceneEntity& sceneEntity);

	private:
		typedef std::deque<AOperation*>	Queue;
		
		Queue					mQueue;
		unsigned int			mMaximumOperations;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif