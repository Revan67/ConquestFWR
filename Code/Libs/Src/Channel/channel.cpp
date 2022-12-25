
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "IChannel2.h"
#include "tcomponent.h"
#include "typedefs.h"
#include "quat.h"
#include <malloc.h>
#include "persistchannel.h"
#include "filesys.h"
#include "da_heap_utility.h"

struct Header
{
	U32 frames;

	//will be < 0.0 if data is not periodic
	SINGLE capture_rate;

	//describes the type of data.  may be an event stream or dof data.
	//types identified in Channeltypes.
	U32 type;
};

struct ChannelArch : public IChannel2
{
	BEGIN_DACOM_MAP_INBOUND(ChannelArch)
	DACOM_INTERFACE_ENTRY(IChannel2)
	DACOM_INTERFACE_ENTRY2(IID_IChannel2,IChannel2)
	END_DACOM_MAP()

	ChannelArch (void);
	~ChannelArch (void);

	virtual GENRESULT COMAPI load (IFileSystem*);

	virtual GENRESULT COMAPI get_duration (SINGLE&) const;
	virtual GENRESULT COMAPI get_data_type (U32&) const;

	//input range [0..get_duration ()]
	virtual GENRESULT COMAPI get_data_pair (SINGLE, const void*&, const void*&, float& ratio) const;

	virtual GENRESULT COMAPI get_data_at_extreme (bool end, const void*&) const;

	virtual GENRESULT COMAPI get_events (SINGLE t0, SINGLE t1, EventIterator::Event[], U32 entries, U32& eventcnt) const;
	virtual GENRESULT COMAPI get_events_at_time (SINGLE, EventIterator::Event[], U32 entries, U32& eventcnt) const;

	GENRESULT init (DACOMDESC*);

protected:

		Header header;

		void* data;

		//including time value for non-periodic data, in bytes
		size_t frame_size;

		//in seconds
		float duration;

		const void* get_frame_data (unsigned int) const;
		const void* get_event_data (unsigned int) const;
		SINGLE get_frame_time (unsigned int i) const;
		unsigned int get_num_frames (void) const;
};

ChannelArch::ChannelArch (void) : data (NULL), duration (0.0f), frame_size (0)
{
}

ChannelArch::~ChannelArch (void) 
{
	if (data)
		free (data);
}

GENRESULT ChannelArch::init (DACOMDESC*)
{
	return GR_OK;
}

inline unsigned int ChannelArch::get_num_frames (void) const
{
	return header.frames;
}

const void* ChannelArch::get_frame_data (unsigned int i) const
{
	ASSERT (i < header.frames);

	return (const char*)data + ((frame_size * i) + ((header.capture_rate < 0.0) ? sizeof (SINGLE) : 0));
}

GENRESULT ChannelArch::get_data_type (U32& out) const
{
	out = header.type;
	return GR_OK;
}

GENRESULT ChannelArch::get_duration (SINGLE& out) const
{
	out = duration;
	return GR_OK;
}

GENRESULT ChannelArch::load (IFileSystem* fs)
{
	if (fs)
	{
		DWORD bytes_read;

		PersistChannelHeader hdr;
		
		DAFILEDESC desc ("Header");
		desc.lpImplementation = "DOS";
	
		HANDLE hndl;
		hndl = fs->OpenChild (&desc);

		if (hndl == INVALID_HANDLE_VALUE)
			return GR_FILE_ERROR;

		BOOL r = fs->ReadFile (hndl, &hdr, sizeof (hdr), &bytes_read);

		fs->CloseHandle (hndl);

		if (!r || (bytes_read != sizeof (hdr)))
			return GR_FILE_ERROR;

		header.frames = hdr.frames;
		header.capture_rate = hdr.capture_rate;
		header.type = hdr.type;

		frame_size = GetFrameSize (header.type);

		//if the capture rate is less than 0.0 then the data is not periodic.
		//in that case each frame consists of a time value of type float and
		//the data.
		if (header.capture_rate < 0.0)
			frame_size += sizeof (SINGLE);

		desc.lpFileName = "Frames";

		hndl = fs->OpenChild (&desc);

		if (INVALID_HANDLE_VALUE == hndl)
			return GR_FILE_ERROR;

		unsigned int malloc_size = fs->GetFileSize (hndl);

		//sanity check on the data size
		if ((header.type != DT_EVENT) &&
			(malloc_size != frame_size * header.frames))
		{
			fs->CloseHandle (hndl);
			return GR_GENERIC;
		}

		data = malloc (malloc_size);

		if (!data)
			return GR_OUT_OF_MEMORY;

		r = fs->ReadFile (hndl, data, malloc_size, &bytes_read);

		fs->CloseHandle (hndl);

		if (!r || (bytes_read != malloc_size))
			return GR_FILE_ERROR;

		if (header.capture_rate < 0.0f ) // using 0.0 causes VS 6.0 to emit bad code -ms
		{
			duration = get_frame_time (get_num_frames () - 1);
		}
		else
		{
			duration = (get_num_frames () - 1) * header.capture_rate;
		}

		return GR_OK;
	}
	else
		return GR_INVALID_PARMS;
}

SINGLE ChannelArch::get_frame_time (unsigned int i) const
{
	ASSERT (i < get_num_frames ());

	SINGLE result;

	if (header.capture_rate < 0.0)
		result = *(SINGLE*)((const char*)data + (frame_size * i));
	else
		result = (i * header.capture_rate);

	return result;
}

GENRESULT ChannelArch::get_data_pair (SINGLE time, 
									const void*& baseval, 
									const void*& nextval, 
									float& ratio) const
{
	GENRESULT result = GR_INVALID_PARMS;

	SINGLE dur;
	get_duration (dur);

	if ((time >= 0.0) && (time <= dur))
	{
		unsigned int base = 0;

		float ft = get_frame_time (base);
		bool past_end = false;

		while ((ft <= time) && !past_end)
		{
			base++;

			if (base < get_num_frames ())
				ft = get_frame_time (base);
			else
				past_end = true;
		}

		//base points to a frame that has time > frame_time

		if (past_end)
		{
			baseval = nextval = get_frame_data (base - 1);
			ratio = 0.0f;
		}
		else
		{
			float ft0 = get_frame_time(base - 1);
			float ft1 = get_frame_time(base);

			ratio = (time - ft0) / (ft1 - ft0);

			baseval = get_frame_data(base - 1);
			nextval = get_frame_data(base);
		}

		result = GR_OK;
	}

	return result;
}

GENRESULT ChannelArch::get_data_at_extreme (bool end, const void*& val) const
{
	GENRESULT result;

	int pos = 0;

	if (end)
		pos = get_num_frames () - 1;

	if (pos < 0)
		result = GR_GENERIC;
	else
	{
		val = get_frame_data (pos);
		result = GR_OK;
	}

	return result;
}

GENRESULT ChannelArch::get_events_at_time (SINGLE t, EventIterator::Event out[], U32 entries, U32& eventcnt) const
{
	GENRESULT result = GR_GENERIC;

	eventcnt = 0;
		
	if (header.type == DT_EVENT)
	{
		result = GR_OK;

		for (unsigned int pos = 0; pos < get_num_frames (); pos++)
		{
			//EMAURER report all events generated for this time
			SINGLE valid_time = get_frame_time (pos);

			if (t == valid_time)
			{
				if (eventcnt < entries)
				{
					out[eventcnt].time = t;
					out[eventcnt].data = get_event_data (pos);
				}
				else
					result = GR_GENERIC;

				eventcnt++;
			}
		}
	}

	return result;
}

const void* ChannelArch::get_event_data (unsigned int frame) const
{
	ASSERT (frame < get_num_frames ());

	int offset = *(int*)get_frame_data (frame);

	return (const char*)data + (frame_size * header.frames) + offset;
}

//EMAURER events defined at t0 or t1 are not reported.

GENRESULT ChannelArch::get_events (SINGLE t0, 
									SINGLE t1, 
									EventIterator::Event out[], 
									U32 entries, 
									U32& eventcnt) const
{
	GENRESULT result = GR_GENERIC;

	eventcnt = 0;
		
	if (header.type == DT_EVENT)
	{
		result = GR_INVALID_PARMS;

		SINGLE dur;
		get_duration (dur);

		//EMAURER are the times different and in range
		if ((t0 >= 0.0f) && (t0 <= dur) &&
			(t1 >= 0.0f) && (t1 <= dur) &&
			(t0 != t1))
		{
			int base;
			int next;
			int add;

			if (t0 < t1)
			{
				add = 1;
				int cur = 0;

				while (true)
				{
					if (cur >= (int)get_num_frames ())
						break;
					else
					{
						float ft = get_frame_time (cur);

						if (ft <= t0)
							base = cur;
						
						if (ft < t1)
							next = cur;
						else
							break;

						cur++;
					}
				}

				//EMAURER base is greatest frame that is <= t0
				//EMAURER next is greatest frame that is < t1

				base++;
				next++;
			}
			else
			{
				add = -1;
				int cur = get_num_frames () - 1;

				while (true)
				{
					if (cur < 0)
						break;
					else
					{
						float ft = get_frame_time (cur);

						if (ft >= t0)
							base = cur;
						
						if (ft > t1)
							next = cur;
						else
							break;

						cur--;
					}
				}

				base--;
				next--;
			}

			ASSERT (base >= 0);
			ASSERT (base < (int)get_num_frames ());
			ASSERT (next >= -1);
			ASSERT (next <= (int)get_num_frames ());

			for (int i = base; i != next; i += add, eventcnt++)
			{
				if (eventcnt < entries)
				{
					out[eventcnt].time = get_frame_time (i);
					out[eventcnt].data = get_event_data (i);
				}
			}

			result = GR_OK;
		}
	}

	return result;
}

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:

			DA_HEAP_ACQUIRE_HEAP( HEAP );
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			IComponentFactory* server = new DAComponentFactory<DAComponent<ChannelArch>, DACOMDESC> (IID_IChannel2);

			if (server)
			{
				DACOM_Acquire ()->RegisterComponent(server, IID_IChannel2, DACOM_NORMAL_PRIORITY);
			}

			server->Release();	// DACOM has added a reference to the server, 
								// we call Release() since we don't save the pointer
			break;
	}

	return TRUE;
}
