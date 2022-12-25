#ifndef TIMER_H
#define TIMER_H
//
// Timer.h - A useful class for measuring performance
//

//
// Include files
//

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

//
// Macros
//

// Using Microsoft specific stuff here. __int64 makes this stuff easy. Redefine
// for whatever your compiler needs.

#define LITOINT64(a)  (*((__int64 *) &(a)))
#define INT64TOLI(a)  (*((LARGE_INTEGER *) &(a)))
#define LITODOUBLE(a) ((double) LITOINT64(a))
#define LIADD(a,b)    ((LARGE_INTEGER) (LITOINT64(a) + LITOINT64(b)))
#define LISUB(a,b)    ((LARGE_INTEGER) (LITOINT64(a) - LITOINT64(b)))
#define LIMUL(a,b)    ((LARGE_INTEGER) (LITOINT64(a) * LITOINT64(b)))
#define LIDIV(a,b)    ((LARGE_INTEGER) (LITOINT64(a) / LITOINT64(b)))

inline LARGE_INTEGER operator + (LARGE_INTEGER a, LARGE_INTEGER b)
{
	__int64 result = (LITOINT64(a) + LITOINT64(b));
	return INT64TOLI(result);
}

inline LARGE_INTEGER operator - (LARGE_INTEGER a, LARGE_INTEGER b)
{
	__int64 result = (LITOINT64(a) - LITOINT64(b));
	return INT64TOLI(result);
}

inline LARGE_INTEGER operator * (LARGE_INTEGER a, LARGE_INTEGER b)
{
	__int64 result = (LITOINT64(a) * LITOINT64(b));
	return INT64TOLI(result);
}

inline LARGE_INTEGER operator / (LARGE_INTEGER a, LARGE_INTEGER b)
{
	__int64 result = (LITOINT64(a) / LITOINT64(b));
	return INT64TOLI(result);
}

//
// Class and structure definitions
//

struct Timer
{
	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	LARGE_INTEGER accumTime;
	LARGE_INTEGER freq;

	Timer() { reset(); }

	void reset (void)
	{
		startTime.LowPart = 0;
		startTime.HighPart = 0;
		endTime.LowPart = 0;
		endTime.HighPart = 0;
		accumTime.LowPart = 0;
		accumTime.HighPart = 0;
		QueryPerformanceFrequency(&freq);
	}

	void begin(void)
	{
		QueryPerformanceCounter(&startTime);
	}

	void end(void)
	{
		QueryPerformanceCounter(&endTime);
		accumTime = accumTime + (endTime - startTime);
	}

	// Return the accumulation of time between begin() and end() pairs since the timer was
	// last reset, in seconds.
	double accumSecs(void)
	{
		return LITODOUBLE(accumTime) / LITODOUBLE(freq);
	}

	// Return the accumulation of time between the last begin() and end() pair, in seconds.
	// WARNING: The return value is undefined if called between a begin() and end() pair.
	double deltaSecs(void)
	{
		LARGE_INTEGER delta = endTime - startTime;
		return LITODOUBLE(delta) / LITODOUBLE(freq);
	}

	// Return the elapsed time since the last begin() in seconds.
	double elapsedSecs(void)
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		currentTime = currentTime - startTime;
		return LITODOUBLE(currentTime) / LITODOUBLE(freq);
	}
};

#endif
