// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AStateVariable_h
#define AStateVariable_h

#include "StringType.h"

// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  AStateVariable
// --------------------------------------------------------------------------
class AStateVariable
{
    public:
        virtual ~AStateVariable() = 0;

        const ROSString& GetName() const;

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    private :
        ROSString mName;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
