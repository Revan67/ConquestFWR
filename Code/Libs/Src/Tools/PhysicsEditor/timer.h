
class Timer
{
	protected:

		const float initial_result;

		bool initialized;
		DWORD last_tick;
		float timer_frequency;

	public:

		Timer (float _initial_result = (1.0f / 30.0f));

		float compute_elapsed_time (void);
};


inline Timer::Timer (float _initial_result) : 
			initial_result (_initial_result), 
			initialized (false)
{
	LARGE_INTEGER freq;

	BOOL r = QueryPerformanceFrequency (&freq);

	timer_frequency = float (freq.u.LowPart);
}

inline float Timer::compute_elapsed_time (void)
{
	float dt;

	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	DWORD current_tick = count.u.LowPart;

	if (initialized)
		dt = (current_tick - last_tick) / timer_frequency;
	else
	{
		initialized = true;
		dt = initial_result;
	}

	last_tick = current_tick;

	return dt;
}
