// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StreamWiz_h
#define StreamWiz_h
// --------------------------------------------------------------------------
#include "Typedefs.h"
// --------------------------------------------------------------------------
typedef unsigned int BitFlag;
typedef U32 Offset;
// --------------------------------------------------------------------------
const unsigned int	kMaxFlagBits = (8 * sizeof(BitFlag)) - 1;
const Offset		kMaxOffset = 0xFFFFFFFF;
// --------------------------------------------------------------------------
template<unsigned int num>
class NumDecimalPlacesInConst
{
	public:
		operator const unsigned int() const
		{
			return kNumDecimals;
		}

		enum
		{
			kNumDecimals = 1 + NumDecimalPlacesInConst<num / 10>::kNumDecimals
		};
};
// --------------------------------------------------------------------------
class NumDecimalPlacesInConst<0>
{
	public:
		operator const unsigned int() const
		{
			return kNumDecimals;
		}

		enum
		{
			kNumDecimals = 0
		};
};
// --------------------------------------------------------------------------
inline unsigned int NumDecimalPlaces(unsigned int num)
{
	if(num != 0)
	{
		return 1 + NumDecimalPlaces(num / 10);
	}
	else
	{
		return 0;
	}
}
// --------------------------------------------------------------------------
inline void WriteOffset(std::ostream& oStream, Offset offset)
{
	const unsigned int	numLeading0s = NumDecimalPlacesInConst<kMaxOffset>() - NumDecimalPlaces(offset);

	for(unsigned int idx = 0; idx < numLeading0s; ++idx)
	{
		oStream << '0';
	}

	oStream << offset << std::endl;
}
// --------------------------------------------------------------------------
inline void ReadOffset(std::istream& iStream, Offset& offset)
{
	iStream >> offset;
}
// --------------------------------------------------------------------------
#endif