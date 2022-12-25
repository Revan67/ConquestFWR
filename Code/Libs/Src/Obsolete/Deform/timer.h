//
//
//

#ifndef TIMER_H
#define TIMER_H

//

struct Timer
{
	unsigned long frequency;
	unsigned long start_tick;

	Timer(void);
	~Timer(void) {}

	void reset(void);
	unsigned long get_elapsed_ticks(void) const;
	float get_elapsed_time(void) const;
};

//

#endif