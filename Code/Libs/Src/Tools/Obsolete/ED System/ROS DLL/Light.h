// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Light_h
#define Light_h
// --------------------------------------------------------------------------
#include <iostream>

#include "Color.h"
#include "StringType.h"
#include "DynamicSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Position;
class Scene;
// --------------------------------------------------------------------------
//  Light
// --------------------------------------------------------------------------
class Light : public DynamicSceneEntity 
{
    public:
		friend class LightStateAccessor;

		Light(const ROSString& kNameR, bool makeNameUnique, Scene& scene);
        Light(Scene& scene);
        
        virtual ROSString GetArchetypeName() const;
        static ROSString GetLightArchetypeName();

        void SetAmbient(const Color kColorR);
        /**# :[Description = "The argurment can range from 0 through 1. This method tries to match the balance of the red, green and blue components. The alpha component remains unaffected."] */
        void SetAmbientIntensity(float intensity);
        void SetDiffuse(const Color kColorR);
        /**# :[Description = "The argurment can range from 0 through 1. This method tries to match the balance of the red, green and blue components. The alpha component remains unaffected."] */
        void SetDiffuseIntensity(float intensity);
        void SetSpecular(const Color kColorR);
        /**# :[Description = "The argurment can range from 0 through 1. This method tries to match the balance of the red, green and blue components. The alpha component remains unaffected."] */
        void SetSpecularIntensity(float intensity);
        void SetPosition(const Position& kPositionR);

        bool IsPositionFixed();
        bool IsColorFixed();

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    protected:
		void WriteSubObject(std::ostream& oStreamR) const;
		void ReadSubObject(std::istream& iStreamR);

        virtual void Render(const ROS::DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

    private:
        typedef DynamicSceneEntity BaseClass;

        Color mAmbient;
        Color mDiffuse;
        Color mSpecular;
        bool mIsPositionFixed;
        bool mIsColorFixed;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
