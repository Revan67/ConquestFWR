// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef StringListH
#define StringListH
//---------------------------------------------------------------------------
#include <vector>
#include "CodeMsg.h"
#include "StringType.h"
//---------------------------------------------------------------------------
#if 1
// New implementation
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
class StringList
{
	public:
		StringList()
        {
        }

		ROSString GetString(int index) const
        {
            ASSERT(index < mStrings.size());

            return mStrings[index];
        }

		int GetStringCount() const
		{
        	return mStrings.size();
		}

		void Add(const ROSString& string)
		{
			mStrings.push_back(string);
		}

		void Replace(unsigned int index, const ROSString& string)
		{
            ASSERT(index < mStrings.size());

			mStrings[index] = string;
		}

		void Replace(const StringList& stringList)
		{
            Resize(stringList.GetStringCount());

			*this = stringList;
		}

		void Clear()
		{	mStrings.clear();
		}

        void Read(std::istream& iStream);

        void ReadStringsTillEndOfStream(std::istream& iStream)
        {
			while(!iStream.eof())
			{	ROSString	stringEntry;

	    		ROS::ReadLine(iStream, stringEntry);
				Add(stringEntry);
			}
		}

		int Find(const ROSString& string)
		{
			StringCollection::const_iterator		begin = mStrings.begin();
			const StringCollection::const_iterator	end = mStrings.end();
			unsigned int	idx = 0;

			while(begin != end)
			{	if((*begin) == string)
				{	return idx;
				}

    			++begin;
				++idx;
			}

			// String was not found!
			return -1;
		}

		int FindSubString(const ROSString& string)
		{
			const unsigned int	numEntities = mStrings.size();

			StringCollection::const_iterator		begin = mStrings.begin();
			const StringCollection::const_iterator	end = mStrings.end();
			unsigned int	idx = 0;

			while(begin != end)
			{	if((*begin).find(string) != ROSString::npos)
				{	return idx;
				}

    			++begin;
				++idx;
			}

			// String was not found!
			return -1;
		}

		void Write(std::ostream& oStream) const;

		void ReadStrings(std::istream& iStream)
        {
            const int	kMaxChars = 200;
            char	buff[kMaxChars];

            iStream.getline(buff, kMaxChars);
            
			while(strlen(buff) != 0)
            {	
				mStrings.push_back(ROSString(buff));
	            iStream.getline(buff, kMaxChars);
            }
        }

		void Resize(unsigned int stringCount)
		{
			mStrings.resize(stringCount);
		}

		StringList& operator=(const StringList& stringList)
		{
			if(this != &stringList)
			{
				ASSERT(GetStringCount() == stringList.GetStringCount());

				const unsigned int	count = GetStringCount();

				for(unsigned int idx = 0; idx < count; ++idx)
				{
					Replace(idx, stringList.GetString(idx));
				}
			}

			return *this;
		}

	private:
		typedef std::vector<ROSString> StringCollection;

		friend std::ostream& operator<<(std::ostream& oStream, const ROS::StringList::StringCollection& strings);
		friend std::istream& operator>>(std::istream& iStream, ROS::StringList::StringCollection& strings);

		enum FieldID
		{
			kStringCollection
		};
		
		StringCollection	mStrings;
};
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::StringList& list)
{
	list.Write(oStream);

	return oStream;
}
//---------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::StringList& list)
{
	list.Read(iStream);

	return iStream;
}
//---------------------------------------------------------------------------
#else
// Old implementation
#ifdef __STD_IOSTREAM__
#undef __STD_IOSTREAM__
#include <vcl/classes.hpp>
#define __STD_IOSTREAM__
#else
#include <vcl/classes.hpp>
#endif

#include "StringType.h"
//---------------------------------------------------------------------------
template <class TClass>
class StringList: public TStringList
{	public:
		typedef TClass ObjectType;
        typedef TObject StoredObjectType;

						__fastcall StringList()
                    	:TStringList()
                    	{
                    	}
		ROS::ROSString __fastcall	GetString(int index) const
        				{
                        	ASSERT(index < Count);

                        	AnsiString astr = this->Strings[index];
        					ROS::ROSString str = astr.c_str();

                            return str;
        				}

        ObjectType __fastcall	GetObject(int index) const
        				{
                        	ASSERT(index < Count && dynamic_cast<ObjectType>(reinterpret_cast<ObjectType>(Objects[index])));

							ObjectType obj = reinterpret_cast<ObjectType>(Objects[index]);

                            return dynamic_cast<ObjectType>(obj);
        				}

		int __fastcall	GetStringCount()
						{
        					return this->Count;
						}
        void __fastcall	Read(std::istream& iStreamR)
        				{
                        	const int	kMaxChars = 200;
                            char	buff[kMaxChars];

                        	iStreamR.getline(buff, kMaxChars);
                            while(strlen(buff) != 0)
                            {	Add(buff);
	                        	iStreamR.getline(buff, kMaxChars);
                            }
                        }
};

#endif

#endif	// StringListH