//---------------------------------------------------------------------------
// GameTimer
//---------------------------------------------------------------------------

struct GameTimer
{
	unsigned		ticks;				// ticks per second

	LARGE_INTEGER	last,now;
	unsigned		elapsed;			// ticks per frame

	unsigned		frame_count;

	double			fps;				// frames per second
	double			time_per_frame;		// seconds per frame

	void init (void)
	{
		LARGE_INTEGER  freq;
		QueryPerformanceFrequency(&freq);
		ticks = freq.u.LowPart;			// clock ticks per second

		QueryPerformanceCounter(&last);

		fps = 30.0;						// default fps 
		time_per_frame = 1.0 / fps;

		frame_count = 0;
	}

	void update (void)
	{
		//
		// Calculate ELAPSED time between frames in ticks
		//
		// ignore the upper 32-bits since it should only tick over once
		// which will not effect the UNSIGNED 
		//
		QueryPerformanceCounter(&now);
		elapsed = now.u.LowPart - last.u.LowPart;
		last = now;

		if (++frame_count >= 2) // wait until we have 2 real time values
		{
			fps = double(ticks) / double(elapsed);
		}

		time_per_frame = 1.0 / fps;
	}
};

//---------------------------------------------------------------------------
