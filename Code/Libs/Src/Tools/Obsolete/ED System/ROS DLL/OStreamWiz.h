// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OStreamWiz_h
#define OStreamWiz_h
// --------------------------------------------------------------------------
#include <iostream>
#include <list>
#include "StreamWiz.h"
#include "CodeMsg.h"

// --------------------------------------------------------------------------
extern std::ostream * gKludgyThornConversionStep1Stream;
// -------------------------------------------------------------------------

// --------------------------------------------------------------------------
template<typename TFieldID>
class OStreamWiz
{
	public:
		OStreamWiz(std::ostream& oStream)
		: mOStream(&oStream)
		{
			ASSERT(mOStream);

			mFlagsBlockOffsetPos = mOStream->tellp();	// Remember the position so that we can update the offset later

			WriteOffset(*mOStream, 0);	// The 0 is a placeholder. We will comeback and put in the correct value in ~OstreamWiz().

//			*mOStream << std::endl;
		}

		template<typename TData>
		void Put(TFieldID fieldID, const TData& data, const char * extraData = NULL)
		{
			Add(fieldID);

			*mOStream << data << std::endl;
			if (extraData)
			{
				if (gKludgyThornConversionStep1Stream)
				{
					//std::ostream *extraDataStream = reinterpret_cast<std::ostream *> (gKludgyThornConversionStep1Stream);
					//*extraDataStream << extraData;
					*gKludgyThornConversionStep1Stream << extraData;
				}
			}
		}

		~OStreamWiz()
		{
			// Update the put flag
			const std::ostream::pos_type	currentPos = mOStream->tellp();
			const Offset					flagsBlockOffset = currentPos - mFlagsBlockOffsetPos;

			mOStream->seekp(mFlagsBlockOffsetPos);

			WriteOffset(*mOStream, flagsBlockOffset);

			// Restore to current location
			mOStream->seekp(currentPos);

			if(mFieldDataOffsets.empty())
			{
				// No fields were written!
				const BitFlag	bitFlag = 0;

				*mOStream << bitFlag << std::endl;
			}
			else
			{
				// Now put in the flags and offsets
				mFieldDataOffsets.sort();

				// First the flags
				FieldDataOffsetList::const_iterator			begin = mFieldDataOffsets.begin();
				const FieldDataOffsetList::const_iterator	end = mFieldDataOffsets.end();

				BitFlag			bitFlag = 0;
				unsigned int	fieldIDBase = 0;

				while(begin != end)
				{
					unsigned int	fieldID = begin->first - fieldIDBase;

					if(fieldID < kMaxFlagBits)
					{
						// This flag can be written to the current BitFlag
						bitFlag |= (1 << fieldID);

						++begin;

						if(begin == end)
						{
							// Loop is about to end! Write out the current flag
							*mOStream << bitFlag << std::endl;
						}						
					}
					else
					{
						// This flag cannot be accomodated in the current BitFlag
						// Write out the current BitFlag and setup for the next one
						bitFlag |= (1 << kMaxFlagBits);	// To indicate that more flag bits follow

						*mOStream << bitFlag << std::endl;

						bitFlag = 0;
						fieldIDBase += kMaxFlagBits;
					}
				}

				// Now write the offsets
				begin = mFieldDataOffsets.begin();

				while(begin != end)
				{
					Offset	streamOff = begin->second;

					*mOStream << streamOff << std::endl;

					++begin;
				}
			}
		}

	protected:
		void Add(TFieldID fieldID)
		{
			ASSERT(fieldID >= 0);

			const std::ostream::pos_type	currentPos = mOStream->tellp();
			
			mFieldDataOffsets.push_back(FieldDataOffset(fieldID, currentPos - mFlagsBlockOffsetPos));
		}

	private:
		typedef std::pair<TFieldID, Offset> FieldDataOffset;
		typedef std::list<FieldDataOffset> FieldDataOffsetList;

		std::ostream*			mOStream;
	    std::ostream::pos_type	mFlagsBlockOffsetPos;
		FieldDataOffsetList		mFieldDataOffsets;
};
// --------------------------------------------------------------------------
template<typename TFieldID>
class EdOStreamWiz
{
	public:
		EdOStreamWiz(std::ostream& oStream)
		: mOStream(&oStream)
		{
			ASSERT(mOStream);

			mFlagsBlockOffsetPos = mOStream->tellp();	// Remember the position so that we can update the offset later

			WriteOffset(*mOStream, 0);	// The 0 is a placeholder. We will comeback and put in the correct value in ~OstreamWiz().

//			*mOStream << std::endl;
		}

		template<typename TData>
		void Put(TFieldID fieldID, const TData& data, const char * extraData = NULL)
		{
			Add(fieldID);

			*mOStream << data << std::endl;
		}

		~EdOStreamWiz()
		{
			// Update the put flag
			const std::ostream::pos_type	currentPos = mOStream->tellp();
			const Offset					flagsBlockOffset = currentPos - mFlagsBlockOffsetPos;

			mOStream->seekp(mFlagsBlockOffsetPos);

			WriteOffset(*mOStream, flagsBlockOffset);

			// Restore to current location
			mOStream->seekp(currentPos);

			if(mFieldDataOffsets.empty())
			{
				// No fields were written!
				const BitFlag	bitFlag = 0;

				*mOStream << bitFlag << std::endl;
			}
			else
			{
				// Now put in the flags and offsets
				mFieldDataOffsets.sort();

				// First the flags
				FieldDataOffsetList::const_iterator			begin = mFieldDataOffsets.begin();
				const FieldDataOffsetList::const_iterator	end = mFieldDataOffsets.end();

				BitFlag			bitFlag = 0;
				unsigned int	fieldIDBase = 0;

				while(begin != end)
				{
					unsigned int	fieldID = begin->first - fieldIDBase;

					if(fieldID < kMaxFlagBits)
					{
						// This flag can be written to the current BitFlag
						bitFlag |= (1 << fieldID);

						++begin;

						if(begin == end)
						{
							// Loop is about to end! Write out the current flag
							*mOStream << bitFlag << std::endl;
						}						
					}
					else
					{
						// This flag cannot be accomodated in the current BitFlag
						// Write out the current BitFlag and setup for the next one
						bitFlag |= (1 << kMaxFlagBits);	// To indicate that more flag bits follow

						*mOStream << bitFlag << std::endl;

						bitFlag = 0;
						fieldIDBase += kMaxFlagBits;
					}
				}

				// Now write the offsets
				begin = mFieldDataOffsets.begin();

				while(begin != end)
				{
					Offset	streamOff = begin->second;

					*mOStream << streamOff << std::endl;

					++begin;
				}
			}
		}

	protected:
		void Add(TFieldID fieldID)
		{
			ASSERT(fieldID >= 0);

			const std::ostream::pos_type	currentPos = mOStream->tellp();
			
			mFieldDataOffsets.push_back(FieldDataOffset(fieldID, currentPos - mFlagsBlockOffsetPos));
		}

	private:
		typedef std::pair<TFieldID, Offset> FieldDataOffset;
		typedef std::list<FieldDataOffset> FieldDataOffsetList;

		std::ostream*			mOStream;
	    std::ostream::pos_type	mFlagsBlockOffsetPos;
		FieldDataOffsetList		mFieldDataOffsets;
};
// --------------------------------------------------------------------------
template<class TFieldID>
inline bool operator<(const std::pair<TFieldID, Offset>& fieldData1, const std::pair<TFieldID, Offset>& fieldData2)
{
	ASSERT(fieldData1.first != fieldData2.first);	// No two should have the same field

	return fieldData1.first < fieldData2.first;
}
// --------------------------------------------------------------------------
#endif

