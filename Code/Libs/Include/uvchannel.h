//
//
//

#ifndef UVCHANNEL_H
#define UVCHANNEL_H

//



//

struct UVChannel
{

	char *		name;			// channel name

	int			vertex_count;	// number of (reference) vertices that animate

	int			*vertex_lookup;	// a list for converting UVChanel index into a texture_vertex_chain index

	float		fps;			// frames per second

	float		time_length;	// length of animation (frame_count / fps)

	int			frame_count;	// number of animation frames

	int			*uv_chain;		// list of indices for each animated vertex for each frame into 
								// mesh->texture_vertex_list; length is vertex_count * frame_count
	int			interpolate;	// specifies whether frames should be interpolated

	UVChannel(void);

	~UVChannel(void);
	
	UVChannel & operator = (const UVChannel & chnl)
	{
		name = strdup(chnl.name);
		vertex_count = chnl.vertex_count;
		vertex_lookup = new int[vertex_count];
		memcpy(vertex_lookup, chnl.vertex_lookup, vertex_count*sizeof(int));
		fps = chnl.fps;
		time_length = chnl.time_length;
		frame_count = chnl.frame_count;
		uv_chain = new int[vertex_count * frame_count];
		memcpy(uv_chain, chnl.uv_chain, vertex_count * frame_count * sizeof(int));
		interpolate = chnl.interpolate;

		return *this;
	}
};

//

inline UVChannel::UVChannel (void)
{
	name = NULL;
	vertex_count = 0;
	vertex_lookup = NULL;
	fps = 0.0f;
	time_length = 0.0f;
	frame_count = 0;
	uv_chain = NULL;
	interpolate = 1;
}

inline UVChannel::~UVChannel (void)
{
	if(name)
		free(name);

	if(vertex_lookup)
		free(vertex_lookup);

	if(uv_chain)
		free(uv_chain);
}

#endif
