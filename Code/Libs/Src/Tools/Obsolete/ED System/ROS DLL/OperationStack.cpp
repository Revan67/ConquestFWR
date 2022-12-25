// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "OperationStack.h"
#include "AOperation.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
OperationStack::OperationStack(unsigned int maximumOperations)
: mMaximumOperations(maximumOperations)
{
	ASSERT(maximumOperations > 0);
}
// --------------------------------------------------------------------------
OperationStack::~OperationStack()
{
	while(!mQueue.empty())
	{
		AOperation*	operation = mQueue.front();

		mQueue.pop_front();

		delete operation;
	}
}
// --------------------------------------------------------------------------
void OperationStack::PushOperation(AOperation* operation)
{
	ASSERT(operation != NULL);
	ASSERT(mQueue.size() <= mMaximumOperations);

	if(mQueue.size() == mMaximumOperations)
	{
		AOperation*	operation = mQueue.back();

		mQueue.pop_back();

		delete operation;
	}

	mQueue.push_front(operation);
}
// --------------------------------------------------------------------------
bool OperationStack::IsEmpty() const
{
	return mQueue.empty();
}
// --------------------------------------------------------------------------
ROS::ROSString OperationStack::GetTopOperationName() const
{
	ASSERT(!IsEmpty());

	return mQueue.front()->GetName();
}
// --------------------------------------------------------------------------
AOperation* OperationStack::PerformTopOperationAndRemove()
{
	ASSERT(!IsEmpty());

	AOperation*	operation = mQueue.front();

	mQueue.pop_front();

	AOperation* inverse = operation->Perform();

	delete operation;

	return inverse;
}
// --------------------------------------------------------------------------
void OperationStack::RemoveOperations(const ROS::ASceneEntity& sceneEntity)
{
	// We will iterate the collection from the back so that the removal of
	// operations does not invalidate the iterator.

	Queue::iterator			end = mQueue.end();
	const Queue::iterator	begin = mQueue.begin();

	while(end != begin)
	{
		--end;

		const AOperation*	operation = *end;

		if(operation->GetEntity() == &sceneEntity)
		{
			mQueue.erase(end);

			delete operation;
		}
	}
}
// --------------------------------------------------------------------------
}