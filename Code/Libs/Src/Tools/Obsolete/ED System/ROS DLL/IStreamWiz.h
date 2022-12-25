// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef IStreamWiz_h
#define IStreamWiz_h
// --------------------------------------------------------------------------
#include <iostream>
#include <list>
#include <vector>
#include "StreamWiz.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
extern bool	gIsOldFileFormat;
inline IStreamWizSetIsOldFileFormat(bool isOld)
{
	gIsOldFileFormat = isOld;
}
#endif
// --------------------------------------------------------------------------
template<typename TFieldID>
class IStreamWiz
{
	public:
		IStreamWiz(std::istream& iStream)
		: mIStream(&iStream)
		{
			ASSERT(mIStream);

#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
			if(gIsOldFileFormat)
			{
				return;
			}
#endif
			mPutFlagsBlockOffsetPos = mIStream->tellg();

			// Recover the offset information for all the fields
			Offset	flagsBlockOffset;

			ReadOffset(*mIStream, flagsBlockOffset);

			const	std::streamoff	offset = flagsBlockOffset;	// Just to assist the compiler resolve the operator+() in the next statement;

			const std::istream::pos_type	flagsBlockPos = mPutFlagsBlockOffsetPos + offset;

			mIStream->seekg(flagsBlockPos);

			// First read in all the Bit Flags
			BitFlag		bitFlag;
			BitFlagList	bitFlagList;

			do
			{
				*mIStream >> bitFlag;

				bitFlagList.push_back(bitFlag);
			}
			while((bitFlag & (1 << kMaxFlagBits)) != 0);

			// Now read in all the offsets
			BitFlagList::const_iterator			begin = bitFlagList.begin();
			const BitFlagList::const_iterator	end = bitFlagList.end();
			unsigned int						fieldIDBase = 0;

			while(begin != end)
			{
				unsigned int	fieldID = 0;

				bitFlag = *begin;

				bitFlag &= ~(1 << kMaxFlagBits);

				while(bitFlag != 0)
				{
					if((bitFlag & 1) != 0)
					{
						// We have a valid flag! Let's read the corresponding offset
						Offset	offset;

						*mIStream >> offset;

						mFieldDataOffsets.push_back(FieldDataOffset(fieldIDBase + fieldID, offset));
					}

					// Prepare for next bit
					++fieldID;
					bitFlag >>= 1;
				}

				// Prepare for next Bit Flag
				fieldIDBase += kMaxFlagBits;
				++begin;
			}

			mNextDataPos = mIStream->tellg();
		}

		bool Has(TFieldID fieldID) const
		{
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
			if(gIsOldFileFormat)
			{
				return false;
			}
#endif
			FieldDataOffsetList::const_iterator			begin = mFieldDataOffsets.begin();
			const FieldDataOffsetList::const_iterator	end = mFieldDataOffsets.end();

			while(begin != end)
			{
				if(begin->first == fieldID)
				{
					return true;
				}

				++begin;
			}

			return false;
		}

		template<typename TData>
		void Get(TFieldID fieldID, TData& data, const TData& defaultData)
		{
			std::istream*	iStream = GetIStream(fieldID);

			if(iStream)
			{
				*iStream >> data;
			}
			else
			{
				data = defaultData;
			}
		}
		
		template<typename TData>
		void Get(TFieldID fieldID, TData& data)
		{
			std::istream*	iStream = GetIStream(fieldID);
			ASSERT(iStream != NULL);

			if(iStream)
			{
				*iStream >> data;
			}
		}
	
		~IStreamWiz()
		{
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
			if(gIsOldFileFormat)
			{
				return;
			}
#endif
			// Set to end of all data
			mIStream->seekg(mNextDataPos);
		}

	protected:
		std::istream* GetIStream(TFieldID fieldID)
		{
#ifdef SUPPORT_OLD_SCE_FILE_FORMAT
			if(gIsOldFileFormat)
			{
				return mIStream;
			}
#endif
			// First find the offset
			FieldDataOffsetList::const_iterator			begin = mFieldDataOffsets.begin();
			const FieldDataOffsetList::const_iterator	end = mFieldDataOffsets.end();

			while(begin != end)
			{
				if(begin->first == fieldID)
				{
					const std::streamoff	offset = begin->second;	// Just to help the compiler resolve operator+() in the next statement

					mIStream->seekg(mPutFlagsBlockOffsetPos + offset);

					return mIStream;
				}

				++begin;
			}

			return NULL;
		}

	private:
		typedef std::pair<unsigned int, Offset> FieldDataOffset;
		typedef std::list<FieldDataOffset> FieldDataOffsetList;
		typedef std::vector<BitFlag> BitFlagList;

		std::istream*			mIStream;
	    std::istream::pos_type	mPutFlagsBlockOffsetPos;
		FieldDataOffsetList		mFieldDataOffsets;
	    std::istream::pos_type	mNextDataPos;	// This can be computed using the other members. Here for efficiency.
};
// --------------------------------------------------------------------------
#endif