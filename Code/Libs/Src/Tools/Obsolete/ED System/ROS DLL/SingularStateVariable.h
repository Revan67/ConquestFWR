// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SingularStateVariable_h
#define SingularStateVariable_h

#include "AStateVariable.h"
#include "Links.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	SingularStateVariable
// --------------------------------------------------------------------------
template <class TVariableType>
class SingularStateVariable : public AStateVariable
{
	public:
    	SingularStateVariable(TVariableType& value);

        void SetValue(const TVariableType& value);
        TVariableType GetValue() const;

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& oStream);

   private:
   		typedef AStateVariable BaseClass;

		enum FieldID
		{
		};

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& oStream);

	   	AssPointer<TVariableType>	mValue;
};
// --------------------------------------------------------------------------
template <class TVariableType>
SingularStateVariable<TVariableType>::SingularStateVariable(TVariableType& value)
: mValue(&value)
{
}
// --------------------------------------------------------------------------
template <class TVariableType>
void SingularStateVariable<TVariableType>::SetValue(const TVariableType& value)
{
	*mValue = value;
}
// --------------------------------------------------------------------------
template <class TVariableType>
TVariableType SingularStateVariable<TVariableType>::GetValue() const
{
	return *mValue;
}
// --------------------------------------------------------------------------
template <class TVariableType>
void SingularStateVariable<TVariableType>::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
template <class TVariableType>
void SingularStateVariable<TVariableType>::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
template <class TVariableType>
void SingularStateVariable<TVariableType>::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
template <class TVariableType>
void SingularStateVariable<TVariableType>::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif