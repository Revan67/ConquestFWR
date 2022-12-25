// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Color_h
#define Color_h
// --------------------------------------------------------------------------
#include <iostream>

#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	Color
// --------------------------------------------------------------------------
class CPP_DECL Color
{
	public:
		typedef float ColorComponent;

		Color();
		Color(ColorComponent red, ColorComponent green, ColorComponent blue, ColorComponent alpha);

		void SetRed(ColorComponent red);
		void SetGreen(ColorComponent green);
		void SetBlue(ColorComponent blue);
		void SetAlpha(ColorComponent alpha);
		void Set(ColorComponent red, ColorComponent green, ColorComponent blue, ColorComponent alpha);

		ColorComponent GetRed() const;
		ColorComponent GetGreen() const;
		ColorComponent GetBlue() const;
		ColorComponent GetAlpha() const;

		Color Interpolate(const Color& nextColor, float t) const;

    	void Write(std::ostream& oStream) const;
    	void Read(std::istream& iStream);

	private :
    	void WriteSubObject(std::ostream& oStream) const;
    	void ReadSubObject(std::istream& iStream);

        ColorComponent mRed;
        ColorComponent mGreen;
        ColorComponent mBlue;
        ColorComponent mAlpha;
};
// --------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Color& color)
{
	color.Write(oStream);

	return oStream;
}
//---------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Color& color)
{
	color.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif