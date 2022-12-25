// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef _h_SequenceGenerator
#define _h_SequenceGenerator
// --------------------------------------------------------------------------
#include <iostream>
// --------------------------------------------------------------------------
template<typename TElement>
class SequenceGenerator
{
	public:
    	SequenceGenerator(const TElement& initialValue, const TElement& increment);

        TElement GetNextValue();

        void SetNextValue(const TElement& newNextValue);
        void SetIncrement(const TElement& newIncrement);
        void Set(const TElement& newNextValue, const TElement& newIncrement);

        void Write(std::ostream& ostream) const;
        void Read(std::istream& istream);

	private:
		enum FieldID
		{	
			kNextValue,
			kIncrement
		};

    	TElement	mNextValue;
        TElement	mIncrement;
};
// --------------------------------------------------------------------------
template<typename TElement>
inline SequenceGenerator<TElement>::SequenceGenerator(const TElement& initialValue, const TElement& increment)
: mNextValue(initialValue), mIncrement(increment)
{
}
// --------------------------------------------------------------------------
template<typename TElement>
inline TElement SequenceGenerator<TElement>::GetNextValue()
{
    TElement nextValue = mNextValue;
    mNextValue += mIncrement;

    return nextValue;
}
// --------------------------------------------------------------------------
template<typename TElement>
inline void SequenceGenerator<TElement>::SetNextValue(const TElement& newNextValue)
{
    mNextValue = newNextValue;
}
// --------------------------------------------------------------------------
template<typename TElement>
inline void SequenceGenerator<TElement>::SetIncrement(const TElement& newIncrement)
{
    mIncrement = newIncrement;
}
// --------------------------------------------------------------------------
template<typename TElement>
inline void SequenceGenerator<TElement>::Set(const TElement& newNextValue, const TElement& newIncrement)
{
    SetNextValue(newNextValue);
    SetIncrement(newIncrement);
}
// --------------------------------------------------------------------------
template<typename TElement>
inline void SequenceGenerator<TElement>::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    oWiz.Put(kNextValue, mNextValue);
    oWiz.Put(kIncrement, mIncrement);
}
// --------------------------------------------------------------------------
template<typename TElement>
inline void SequenceGenerator<TElement>::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    iWiz.Get(kNextValue, mNextValue);
    iWiz.Get(kIncrement, mIncrement);
}
// --------------------------------------------------------------------------
template<typename TElement>
std::ostream& operator<<(std::ostream& oStream, const SequenceGenerator<TElement>& sGen)
{
	sGen.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
template<typename TElement>
std::istream& operator>>(std::istream& iStream, SequenceGenerator<TElement>& sGen)
{
	sGen.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
