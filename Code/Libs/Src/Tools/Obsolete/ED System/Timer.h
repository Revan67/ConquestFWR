//---------------------------------------------------------------------------
#ifndef TIMER_H
#define TIMER_H
//---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <afx.h>

#if 1
#include <stdio.h>
#endif

//---------------------------------------------------------------------------
class Timer
{
	public:
        explicit	inline			Timer(bool start = false);

                    inline	void 	Run();
                    inline	void	Stop();
                    inline	void	Reset();
                    inline	bool	IsRunning() const;

//                    inline	void	SetElapedTime(float elapsedTime);
                    inline	float 	GetElapsedTime() const;

	private:
    	bool					mIsRunning;
        		LARGE_INTEGER 	mFrequency;
        mutable	LARGE_INTEGER 	mStartCount;
        mutable	float			mElapsedTime;
};
//---------------------------------------------------------------------------
inline Timer::Timer(bool start)
:mIsRunning(false)
{
	QueryPerformanceFrequency(&mFrequency);

	Reset();

    if(start)
    {	Run();
    }
  }
//---------------------------------------------------------------------------
inline void Timer::Run()
{
    if(!IsRunning())
    {	QueryPerformanceCounter(&mStartCount);
    	mIsRunning = true;
    }
}
//---------------------------------------------------------------------------
inline void Timer::Reset()
{
	mElapsedTime = 0.0;

    if(IsRunning())
    {	QueryPerformanceCounter(&mStartCount);
    }
}
//---------------------------------------------------------------------------
inline void	Timer::Stop()
{
	if(IsRunning())
    {	LARGE_INTEGER 	currCount;

  		QueryPerformanceCounter(&currCount);

        mElapsedTime += (currCount.QuadPart - mStartCount.QuadPart) / mFrequency.QuadPart;

        mIsRunning = false;
    }
}
//---------------------------------------------------------------------------
inline bool	Timer::IsRunning() const
{
	return mIsRunning;
}
//---------------------------------------------------------------------------
inline float Timer::GetElapsedTime() const
{
	if(IsRunning())
    {	LARGE_INTEGER 	currCount;

  		QueryPerformanceCounter(&currCount);

        mElapsedTime += float(currCount.QuadPart - mStartCount.QuadPart) / float(mFrequency.QuadPart);

        mStartCount = currCount;
	}

#if 0
    {	char	str[100];
   		sprintf(str, "Elapsed Time: %f\n", mElapsedTime);
		OutputDebugString(str);
	}
#endif

    return mElapsedTime;
}
//---------------------------------------------------------------------------
#endif


