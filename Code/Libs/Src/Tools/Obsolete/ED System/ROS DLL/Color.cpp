// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Color.h"
// --------------------------------------------------------------------------
/**# implementation Color:: id(C_0886795857)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kRed,
	kGreen,
	kBlue,
	kAlpha
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN Color::Color()
: mRed(0), mGreen(0), mBlue(0), mAlpha(0)
{
}
// --------------------------------------------------------------------------
CPP_DEFN Color::Color(ColorComponent red, ColorComponent green, ColorComponent blue, ColorComponent alpha)
: mRed(red), mGreen(green), mBlue(blue), mAlpha(alpha)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::SetRed(ColorComponent red)
{
	mRed = red;
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::SetGreen(ColorComponent green)
{
	mGreen = green;
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::SetBlue(ColorComponent blue)
{
	mBlue = blue;
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::SetAlpha(ColorComponent alpha)
{
	mAlpha = alpha;
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::Set(ColorComponent red, ColorComponent green, ColorComponent blue, ColorComponent alpha)
{
	SetRed(red);
	SetGreen(green);
	SetBlue(blue);
	SetAlpha(alpha);
}
// --------------------------------------------------------------------------
CPP_DEFN Color::ColorComponent Color::GetRed() const
{
	return mRed;
}
// --------------------------------------------------------------------------
CPP_DEFN Color::ColorComponent Color::GetGreen() const
{
	return mGreen;
}
// --------------------------------------------------------------------------
CPP_DEFN Color::ColorComponent Color::GetBlue() const
{
	return mBlue;
}
// --------------------------------------------------------------------------
CPP_DEFN Color::ColorComponent Color::GetAlpha() const
{
	return mAlpha;
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    oWiz.Put(kRed, mRed);
    oWiz.Put(kGreen, mGreen);
    oWiz.Put(kBlue, mBlue);
    oWiz.Put(kAlpha, mAlpha);
}
// --------------------------------------------------------------------------
CPP_DEFN Color Color::Interpolate(const Color& nextColor, float t) const
{
	float diff = 1 - t;

	return Color(	GetRed()	* diff + nextColor.GetRed()		* t,
    				GetGreen()	* diff + nextColor.GetGreen()	* t,
    				GetBlue()	* diff + nextColor.GetBlue()	* t,
    				GetAlpha()	* diff + nextColor.GetAlpha()	* t
				);
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Color::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    iWiz.Get(kRed, mRed);
    iWiz.Get(kGreen, mGreen);
    iWiz.Get(kBlue, mBlue);
    iWiz.Get(kAlpha, mAlpha);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------