// FrameTracker.h
//
//
//

#ifndef FRAMETRACKER_H
#define FRAMETRACKER_H

#define STRICT
#include <windows.h>

#include "rendpipeline.h"
#include "RPUL.h"

const FT_MAX_SAMPLES = 256;

class CFrameTracker
{
public:
	inline HRESULT Initialize( IRenderPipeline *IRP, float x, float y, float width, float height, float update, U32 num_samples=30 )
	{
		if( IRP ) {
			m_IRenderPipe = IRP;
			IRP->AddRef();
		}

		m_X = x;		m_Y = y;
		m_W = width;	m_H = height;

		m_FrameTime = m_Timer.AddWatch();
		m_UpdateTime = m_Timer.AddWatch();
		m_UpdateTimeS = update;
		
		m_NumSamples = num_samples;
		m_Sample = (m_NumSamples-1);
		
		m_Timer.Start( m_UpdateTime );
		m_Timer.Start( m_FrameTime );
		for( U32 s=0; s<m_NumSamples; s++ ) {
			m_Samples[s] = 0.0;
		}

		Min = 0.0;
		Max = 10.0;
		Avg = 0.0;
		
		m_Font.Initialize( "default_font" );
		m_Font.SetRenderPipeline( IRP );
		m_PB = new PrimitiveBuilder( IRP, num_samples*3 );

		return S_OK;
	}

	inline HRESULT Cleanup()
	{
		if( m_IRenderPipe ) {
			m_IRenderPipe->Release();
			m_IRenderPipe = NULL;
		}

		delete m_PB;
		m_PB = NULL;
		return S_OK;
	}

	inline HRESULT Update()
	{
		if( m_Timer.GetElapsedSeconds(m_UpdateTime) > m_UpdateTimeS ) {
			
			m_Sample = (m_Sample+1) % m_NumSamples;
			m_Samples[m_Sample] = 1 / m_Timer.GetElapsedSeconds( m_FrameTime );

			Avg = 0;
			Min = 1000;
			Max = 0;
			int si = m_Sample;
			for( U32 s=0; s<m_NumSamples; s++ ) {
				Min = min(Min,m_Samples[si]);
				Max = max(Max,m_Samples[si]);
				Avg += m_Samples[si];
				si--;
				if( si < 0 ) {
					si = m_NumSamples-1;
				}
			}
			Avg = Avg / m_NumSamples;
			
			m_Timer.Start( m_UpdateTime );
		}


		m_Timer.Start( m_FrameTime );

		return S_OK;
	}

	inline HRESULT Draw()
	{
//#define ADJUST(f) ((f)-Min)
#define ADJUST(f) ((f))

		if( ViewGraph ) {
			float x=m_X+m_W;
			float y=m_Y+m_H;
			float dx = m_W/m_NumSamples;	// width scale
			float range = ADJUST(Max);
			float sy = m_H/range;			// height scale 

			m_PB->Color3ub( 0, 255, 0 );
			m_PB->Begin( GL_LINES );
				// draw outline
				m_PB->Vertex3f( m_X,	 m_Y+m_H, 0.25 );
				m_PB->Vertex3f( m_X+m_W, m_Y+m_H, 0.25 );
			m_PB->End();

			m_PB->Color3ub( 255, 0, 0 );
			m_PB->Begin( GL_LINES );
				// draw avg
				float yy = y-sy*ADJUST(Avg);
				m_PB->Vertex3f( m_X,	 yy, 0.25 );
				m_PB->Vertex3f( m_X+m_W, yy, 0.25 );
			m_PB->End();

			m_PB->Color3ub( 0, 0, 255 );
			m_PB->Begin( GL_LINE_STRIP );
				int sample = m_Sample;
				for( U32 s=0; s<m_NumSamples; s++ ) {
					m_PB->Vertex3f( x, y-sy*ADJUST(m_Samples[sample]), 0.25 );
					x -= dx;
					sample--;
					if( sample < 0 ) {
						sample = m_NumSamples-1;
					}
				}
			m_PB->End();
		}

		if( ViewText ) {
			m_Font.RenderFormattedString( m_X+m_W+10, m_Y, "%04.2f\n%04.2f\n%04.2f\n%04.2f", m_Samples[m_Sample], Min, Avg, Max );
		}
		return S_OK;
	}

	CFrameTracker()
	{
		m_X = 0;
		m_Y = 0;
		m_W = 320;
		m_H = 100;
		m_NumSamples = 30;
		m_PB = NULL;
		ViewGraph = ViewText = 1;
		m_IRenderPipe = NULL;
	}

	~CFrameTracker()
	{
		Cleanup();
	}

public:
	U32	ViewText;
	U32 ViewGraph;
	float	Min;
	float	Max;
	float	Avg;

protected:

	IRenderPipeline			*m_IRenderPipe;
	RPFont					 m_Font;
	PrimitiveBuilder		*m_PB;
	CStopWatch				 m_Timer;
	SWID					 m_FrameTime;
	SWID					 m_UpdateTime;
	float					 m_UpdateTimeS;
	float					 m_Samples[FT_MAX_SAMPLES];
	U32						 m_NumSamples;
	U32						 m_Sample;
	float					 m_X, 
							 m_Y, 
							 m_W, 
							 m_H;

};

#endif
