//---------------------------------------------------------------------------
#ifndef TimeTagH
#define TimeTagH
//---------------------------------------------------------------------------
//#include <list>
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
class TimeTag
{
    public:
        explicit TimeTag(float time = -1, int tag = -1)
        : mTime(time), mTag(tag)
        {
        }

        float GetTime() const
        {
            return mTime;
        }

        int GetTag() const
        {
            return mTag;
        }

    private:
        float   mTime;
        int     mTag;
};
//---------------------------------------------------------------------------
//typedef std::list<TimeTag>    TimeTagList;
}
//---------------------------------------------------------------------------
#endif
