//---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <afx.h>

#include "timer.h"
//---------------------------------------------------------------------------
Timer::Timer(void)
{
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);
	frequency = freq.u.LowPart;

	reset();
}
//---------------------------------------------------------------------------
void Timer::reset(void)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	start_tick = li.u.LowPart;
}
//---------------------------------------------------------------------------
unsigned long Timer::get_elapsed_ticks(void) const
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	unsigned long tick = li.u.LowPart;
	return tick - start_tick;
}
//---------------------------------------------------------------------------
float Timer::get_elapsed_time(void) const
{
	unsigned long ticks = get_elapsed_ticks();
	return float(ticks) / float(frequency);
}
//---------------------------------------------------------------------------

