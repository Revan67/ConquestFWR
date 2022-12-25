// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AOperation_h
#define AOperation_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ASceneEntity;
// --------------------------------------------------------------------------
// AOperation
// --------------------------------------------------------------------------
// To create an operation, implement a descendant of the AOperation class.
// The method descriptions below provide the details for a well -behaved
// descendant.
class CPP_DECL AOperation
{
	public:
		// Define a destructor for the descendant if there is any cleanup to
		// be performed
		virtual ~AOperation();

		// Returns the name of the operation. By default returns the
		// name supplied in the constructor. Override only if you need to
		// compute the description dynamically.
		virtual ROSString GetName() const;

		// Override to actually perform the operation. Return an instance of 
		// a descendant of AOperation that knows how to restore the state 
		// before the execution of Perform(). In other words, it returns
		// an inverse of itself. The caller is responsible for deleting the
		// returned instance.
		virtual AOperation* Perform() = 0;

		// Returns the entity upon which this object operates. Returns NULL
		// if this operation does not work on a ASceneEntity.
		const ASceneEntity* GetEntity() const;

	protected:
		// The name argument describes the operation. Word it as a command
		// and keep it short. The entity argument specifies the entity
		// upon which this operation operates.
		AOperation(const ROSString& name, ASceneEntity* entity);

		// Set the description
		void SetName(const ROSString& name);

		// Returns the entity upon which this object operates. Returns NULL
		// if this operation does not work on a ASceneEntity.
		ASceneEntity* GetEntity();

	private:
		ROSString		mName;
		ASceneEntity*	mEntity;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
