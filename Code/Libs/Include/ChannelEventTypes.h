#ifndef _CH_EVENT_TYPES
#define _CH_EVENT_TYPES

typedef enum {SOUND_START=1, SOUND_STOP, EVENT_TAG, NAMED_EVENT, CHANNEL_END, CHANNEL_BEGIN} channel_event;

// EVENT DATA FOLLOWING EACH OF ABOVE TYPES
// struct sound_start{ char *wave_file_name;  // NULL terminated };
// struct sound_stop{ };

#endif
