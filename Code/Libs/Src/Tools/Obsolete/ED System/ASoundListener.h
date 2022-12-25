// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ASoundListener_h
#define ASoundListener_h
// --------------------------------------------------------------------------
class ASoundListener
{
	public:
		virtual ~ASoundListener()
		{
		}

		virtual ROS::Location		GetLocation() const = 0;
		virtual ROS::Orientation	GetOrientation() const = 0;
};
// --------------------------------------------------------------------------
#endif