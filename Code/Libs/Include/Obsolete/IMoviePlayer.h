#ifndef IMOVIEPLAYER_H
#define IMOVIEPLAYER_H
//
// IMoviePlayer.h - IMoviePlayer interface definition
//

//
// Design Notes:
//      This interface is expected to be implemented by the same component that implements the
// IRenderPipeline interface. This interface is used to manipulate movie playing onto drawing surfaces.
// The chosen drawing surfaces can be either textures, the back surface, or the front surface. More than one
// movie can play on the same surface.
//      The movie must always be bound to some surface; playing a movie without a surface doesn't make any sense.
// The required order of operations is:
// 1) Create a texture with only one mip level.
// 2) Open a movie on that texture using OpenTextureMovie
// 3) Use that texture to render some triangles
// 4) Update the movie periodically.
// If playback on the back or primary surface is desired, the steps are the same, except that you don't need to
// create a texture, and you call OpenScreenMovie instead.
//      It is important to note that if a movie's audio is played, it will not be positional, i.e. it will
// play at its full volume. A future system might allow the audio stream to be positional.
//
//      You must create an IFileSystem from which the movie data will be read. The system will figure out the best
// way to play the movie.
//

//
// Include files
//

#include <dacom.h>
#include <filesys.h>

//
// Constants
//

const U32 SURF_FRONT = 0;
const U32 SURF_BACK = 1;

//
// Interfaces
//

#undef	DA_METHOD
#define DA_METHOD(name,params) virtual GENRESULT COMAPI name params = 0;

#define IID_IMoviePlayer MAKE_IID("IMoviePlayer", 1)

struct IMoviePlayer : public IDAComponent
{
	// Opening and closing methods
	DA_METHOD( open_texture_movie, (U32 &out_hmovie, IFileSystem *fs, U32 htexture, RECT *r=NULL, bool audio=false));
	DA_METHOD( open_screen_movie, (U32 &out_hmovie, IFileSystem *fs, U32 surface, RECT *r=NULL, bool audio=false));

	DA_METHOD( close_movie (U32 hmovie));

	// Playback control methods
	DA_METHOD( start (U32 hmovie));
	DA_METHOD( stop (U32 hmovie));
	DA_METHOD( set_time  (U32 hmovie, float time));         // time is relative to the start of the movie
	DA_METHOD( update (U32 hmovie, float deltaTime));   // time is relative to the last time

	DA_METHOD( update_all (float deltaTime));            // updates all playing movies

	// Querying methods
	DA_METHOD( is_done (U32 hmovie));    // returns GR_OK if done, GR_GENERIC if not
	DA_METHOD( get_texture (U32 hmovie, U32 &htexture)); // returns GR_OK if is a texture, GR_GENERIC otherwise
	DA_METHOD( get_movie_info (U32 hmovie, U32 *width, U32 *height, float *duration));
};

#endif
