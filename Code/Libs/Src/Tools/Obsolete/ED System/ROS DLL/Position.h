// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Position_h
#define Position_h
// --------------------------------------------------------------------------
#include "Orientation.h"
#include "Location.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  Position
// --------------------------------------------------------------------------
class CPP_DECL Position
{
    public:
    	Position()
        {
        	mOrientation.SetIdentity();
        }

        Position(const Location& location, const Orientation& orientation)
        :mLocation(location), mOrientation(orientation)
        {
        }

        void SetLocation(const Location& kLocationR);
        void SetOrientation(const Orientation& kOrientationR);

        const Location& GetLocation() const;
        const Orientation& GetOrientation() const;

		Position Interpolate(const Position& nextPosition, float t) const;

        void Write(std::ostream& oStream) const;
		void Read(std::istream& iStream);

    private :
        void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

        Orientation mOrientation;
        Location mLocation;
};
// --------------------------------------------------------------------------
inline Position Position::Interpolate(const Position& nextPosition, float t) const
{
	return Position(GetLocation().Interpolate(nextPosition.GetLocation(), t),
			ROS::Interpolate(GetOrientation(), nextPosition.GetOrientation(), t));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::Position& position)
{
	position.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::Position& position)
{
	position.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif
