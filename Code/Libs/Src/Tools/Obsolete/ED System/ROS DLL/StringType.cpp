// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "StringType.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kEncodedString
};
// --------------------------------------------------------------------------
namespace ROS
{
const int	kMaxChars = 255;
// --------------------------------------------------------------------------
class EncodedStringWriter
{
	public:
		explicit EncodedStringWriter(const ROSString& string)
		: mString(string)
		{
		}

		void Write(std::ostream& oStream) const
		{
			unsigned int	stringSize = mString.length();

			ASSERT(stringSize <= kMaxChars);

			oStream << stringSize;
			oStream.write(mString.c_str(), stringSize);
			oStream << std::endl;
		}

	private:
		const ROSString&	mString;

};
// --------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& oStream, const EncodedStringWriter& stringWriter)
{
	stringWriter.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
class EncodedStringReader
{
	public:
		explicit EncodedStringReader(ROSString& string)
		: mString(string)
		{
		}

		void Read(std::istream& iStream)
		{
			char			stringBuff[kMaxChars + 1];
			unsigned int	stringSize;

			iStream >> stringSize;

			iStream.read(stringBuff, stringSize);
			stringBuff[stringSize] = 0;

			mString = ROSString(stringBuff);
		}

	private:
		ROSString&	mString;

};
// --------------------------------------------------------------------------
std::istream& operator>>(std::istream& iStream, EncodedStringReader& stringReader)
{
	stringReader.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
ROSString TrimLeadingWhiteSpace(const ROSString& string)
{
	ROSString::size_type	idx = string.find_first_not_of(' ');

	if(idx != ROSString::npos)
	{	return string.substr(idx);
	}
	else
	{	return ROSString("");
	}
}
// --------------------------------------------------------------------------
ROSString TrimTrailingWhiteSpace(const ROSString& string)
{
	ROSString::size_type	idx = string.find_last_not_of(' ');

	if(idx != ROSString::npos)
	{	return string.substr(0, idx + 1);
	}
	else
	{	return ROSString("");
	}
}
// --------------------------------------------------------------------------
void Write(std::ostream& oStream, const ROSString& string)
{
	EdOStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kEncodedString, EncodedStringWriter(string));
}
// --------------------------------------------------------------------------
void Read(std::istream& iStream, ROSString& string)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kEncodedString, EncodedStringReader(string));
}
// --------------------------------------------------------------------------
void ReadLine(std::istream& iStream, ROSString& string)
{
    char    stringBuff[kMaxChars + 1];

    iStream.getline(stringBuff, kMaxChars);

    string = ROSString(stringBuff);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
