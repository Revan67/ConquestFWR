// Text.h
//
//
//


#ifndef RPTEXT_H
#define RPTEXT_H

//

#include "rendpipeline.h"
#include "results.h"
#include "RPUL/PrimitiveBuilder.h"

//

#pragma warning( disable : 4200 )

//

struct RPFontVertex {
	Vector pos;			// **screen** position (sx,sy,sz) in 2D case
	float  rhw;			// reciprical homogenous W
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	float			u, v;
};

#define D3DFVF_RPFONTVERTEX_2D ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
//#define D3DFVF_RPFONTVERTEX_3D ( D3DFVF_XYZ | D3DFVF_RESERVED1 | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
#define D3DFVF_RPFONTVERTEX_3D ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 )

//

// RPFontStroker is defined just so generic TPrimitiveBuilder code can be
// written only once for both 2D & 3D
typedef TPrimitiveBuilder<RPFontVertex, 0> RPFontStroker;
typedef TPrimitiveBuilder<RPFontVertex, D3DFVF_RPFONTVERTEX_2D> RPFontStroker2D;
typedef TPrimitiveBuilder<RPFontVertex, D3DFVF_RPFONTVERTEX_3D> RPFontStroker3D;

// not to be used directly. use one of RPFont2D or RPFont3D
class RPFont
{
public:
	
	GENRESULT SetRenderPipeline( IRenderPipeline *pipe );
	GENRESULT SetSize( float scale_y, float scale_x );
	GENRESULT SetSize( float scale );
	GENRESULT SetColor( float r, float g, float b, float a=1.0f );
	GENRESULT Cleanup(void);

	~RPFont();

protected:
	RPFont( IRenderPipeline *pipe = NULL );

	IRenderPipeline *m_Pipe;
	
	union
	{
		RPFontStroker	*m_Stroker;
		RPFontStroker2D *m_Stroker2D;
		RPFontStroker3D *m_Stroker3D;
	};

	bool m_StrokerIsOurs;

	float m_scale_x;
	float m_scale_y;
	float m_kerning_x;
	float m_kerning_y;
};

//

class RPFont2D : public RPFont
{
public:
	RPFont2D( IRenderPipeline *pipe = NULL );

	GENRESULT SetFontStroker( RPFontStroker2D *fs );
	GENRESULT RenderFormattedString( float sx, float sy, const char *fmt, ...);
	GENRESULT RenderString( float sx, float sy, const char *text );
	GENRESULT SetOrtho(void);
};

//

class RPFont3D : public RPFont
{
public:
	RPFont3D( IRenderPipeline *pipe = NULL );

	GENRESULT SetFontStroker( RPFontStroker3D *fs );
	GENRESULT RenderFormattedString( float sx, float sy, const char *fmt, ...);
	GENRESULT RenderString( float sx, float sy, const char *text );
	GENRESULT SetOrtho(void);
};

#endif
