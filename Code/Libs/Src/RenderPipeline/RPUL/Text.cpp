// Text.cpp
//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
#include "FDUMP.h"
#include "RPUL/Text.h"

//

#include <stdarg.h>
#include <stdio.h>

//

typedef struct {
  float x;
  float y;
} CoordRec, *CoordPtr;

typedef struct {
  int num_coords;
  CoordPtr coord;
} StrokeRec, *StrokePtr;

typedef struct {
  int num_strokes;
  StrokePtr stroke;
  float center;
  float right;
} StrokeCharRec, *StrokeCharPtr;

typedef struct {
  char *name;
  int num_chars;
  StrokeCharPtr ch;
  float top;
  float bottom;
} StrokeFontRec, *StrokeFontPtr;

typedef void *GLUTstrokeFont;

#include "glutStrokeRoman.h"

//

RPFont::RPFont( IRenderPipeline *pipe )
{ 
	m_Pipe = NULL; 
	m_Stroker = NULL;
	m_StrokerIsOurs = false;
	SetRenderPipeline( pipe ); 
}

RPFont2D::RPFont2D( IRenderPipeline *pipe ) : RPFont( pipe )
{ 
	SetFontStroker( NULL );
	SetSize( 0.125f, 0.125f );
}

RPFont3D::RPFont3D( IRenderPipeline *pipe ) : RPFont( pipe )
{ 
	SetFontStroker( NULL );
	SetSize( 0.125f, 0.125f );
}

//

RPFont::~RPFont( )
{ 
	Cleanup(); 
}

//

GENRESULT RPFont::SetColor( float r, float g, float b, float a )
{
	m_Stroker->Color4f( r, g, b, a );

	return GR_OK;
}

//

GENRESULT RPFont::SetSize( float scale_y, float scale_x )
{
	m_scale_x = scale_x;
	m_scale_y = scale_y;
	m_kerning_x = m_scale_x+2*(m_scale_x/10.0);
	m_kerning_y = m_scale_y+2*(m_scale_y/10.0);

	return GR_OK;
}

//

GENRESULT RPFont::SetSize( float scale )
{
	return SetSize(scale, scale);
}

//

GENRESULT RPFont::SetRenderPipeline( IRenderPipeline *pipe ) 
{
	DACOM_RELEASE( m_Pipe );
	
	if( (m_Pipe = pipe) != NULL ) {
		m_Pipe->AddRef();
	}

	if( m_Stroker && m_StrokerIsOurs ) {
		m_Stroker->SetPipeline( pipe );
	}

	return GR_OK;	
}

//

GENRESULT RPFont2D::SetFontStroker( RPFontStroker2D *fs )
{
	if( m_StrokerIsOurs ) {
		delete m_Stroker2D;
		m_StrokerIsOurs = false;
	}

	if( (m_Stroker2D = fs) == NULL ) {
		m_Stroker2D = new RPFontStroker2D();
		m_Stroker2D->SetPipeline( m_Pipe );
		m_StrokerIsOurs = true;
	}

	return GR_OK;
}

//

GENRESULT RPFont3D::SetFontStroker( RPFontStroker3D *fs )
{
	if( m_StrokerIsOurs ) {
		delete m_Stroker3D;
		m_StrokerIsOurs = false;
	}

	if( (m_Stroker3D = fs) == NULL ) {
		m_Stroker3D = new RPFontStroker3D();
		m_Stroker3D->SetPipeline( m_Pipe );
		m_StrokerIsOurs = true;
	}

	return GR_OK;
}

//

GENRESULT RPFont::Cleanup(void) 
{
	if( m_StrokerIsOurs ) {
		delete m_Stroker;
		m_StrokerIsOurs = false;
	}

	DACOM_RELEASE( m_Pipe );
	
	return GR_OK;	
}

//

GENRESULT RPFont2D::RenderString( float sx, float sy, const char *text ) 
{
	if( m_Pipe ) {
		m_Pipe->set_texture_stage_texture( 0, 0 );
		m_Pipe->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
		m_Pipe->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		m_Pipe->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Pipe->set_render_state( D3DRS_ZENABLE, FALSE );
	}

	StrokeCharPtr ch;
	StrokePtr stroke;
	CoordPtr coord;
	int i;
	const char *p;
	
	float dx = sx;
	float dy = sy; 
	float y_jump = m_scale_y * (rp_glutStrokeRoman.top - rp_glutStrokeRoman.bottom);

	if( dy < 0.0f ) {
		return GR_GENERIC; // clipped
	}

	for( p = text; *p; p++ ) {

		if( *p == '\n' ) {
			dy += y_jump;
			dx = sx;
			continue;
		}

		int c = *p;
	
		if( c < 0 || c >= rp_glutStrokeRoman.num_chars ) {
			c = '#';
			ASSERT( c < 0 || c >= rp_glutStrokeRoman.num_chars );
		}
		
		ch = &(rp_glutStrokeRoman.ch[c]);
		ASSERT( ch != NULL );

		if( dx >= 0.0f ) { 	// basic clipping
			for( i=ch->num_strokes, stroke=ch->stroke; i>0; i--, stroke++ ) {

				const CoordPtr end = stroke->coord + stroke->num_coords;
				m_Stroker2D->Begin( PB_LINE_STRIP );
				for( coord=stroke->coord; coord < end; coord++ ) {

					m_Stroker2D->Vertex3f( dx + m_scale_x * coord->x,  dy - m_scale_y * coord->y, 0 );
				}
				m_Stroker2D->End();	
			}
		}

		dx += m_scale_x * ch->right;
	}

	return GR_OK;	
}

//

// this is different from 2D in that
// 1) call a different m_Stroker
// 2) leave zenable on
// 3) don't do 2D clipping
GENRESULT RPFont3D::RenderString( float sx, float sy, const char *text ) 
{
	if( m_Pipe ) {
		m_Pipe->set_texture_stage_texture( 0, 0 );
		m_Pipe->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
		m_Pipe->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	}

	StrokeCharPtr ch;
	StrokePtr stroke;
	CoordPtr coord;
	int i;
	const char *p;
	
	float dx = sx;
	float dy = sy;
	float y_jump = m_scale_y * (rp_glutStrokeRoman.top - rp_glutStrokeRoman.bottom);

	for( p = text; *p; p++ ) {

		if( *p == '\n' ) {
			dy += y_jump;
			dx = sx;
			continue;
		}

		int c = *p;
	
		if( c < 0 || c >= rp_glutStrokeRoman.num_chars ) {
			c = '#';
			ASSERT( c < 0 || c >= rp_glutStrokeRoman.num_chars );
		}
		
		ch = &(rp_glutStrokeRoman.ch[c]);
		ASSERT( ch != NULL );

		for( i=ch->num_strokes, stroke=ch->stroke; i>0; i--, stroke++ ) {

			const CoordPtr end = stroke->coord + stroke->num_coords;
			m_Stroker3D->Begin( PB_LINE_STRIP );
			for( coord=stroke->coord; coord < end; coord++ ) {

				m_Stroker3D->Vertex3f( dx + m_scale_x * coord->x,  dy - m_scale_y * coord->y, 0 );
			}
			m_Stroker3D->End();	
		}
	
		dx += m_scale_x * ch->right;
	}

	return GR_OK;	
}

//

GENRESULT RPFont3D::RenderFormattedString( float sx, float sy, const char *fmt, ...)
{
	static char szOut[2048+1];

	va_list args;

	va_start(args, fmt);
	vsprintf( szOut, fmt, args );
	va_end(args);

	return RenderString( sx, sy, szOut );
}

//

GENRESULT RPFont2D::RenderFormattedString( float sx, float sy, const char *fmt, ...)
{
	static char szOut[2048+1];

	va_list args;

	va_start(args, fmt);
	vsprintf( szOut, fmt, args );
	va_end(args);

	return RenderString( sx, sy, szOut );
}

//

GENRESULT RPFont2D::SetOrtho(void) 
{
	return GR_OK;	
}

//

GENRESULT RPFont3D::SetOrtho(void) 
{
/*
	Transform tmp(false);
	tmp.set_identity();
	m_Pipe->set_modelview( tmp );
*/

	int vp[4];
	m_Pipe->get_viewport( &vp[0], &vp[1], &vp[2], &vp[3] );
	// left, right, bottom, top, near, far
	m_Pipe->set_ortho( vp[0], vp[2], vp[3], vp[1], 0, 1000 );

	return GR_OK;	
}

// EOF