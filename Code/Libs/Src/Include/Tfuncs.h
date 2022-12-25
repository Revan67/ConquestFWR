// Tfuncs.h
//
//
//

#ifndef Tfuncs_h
#define Tfuncs_h


const double Tpi = 3.1415926535897932384626433832795;

template<typename T>
inline const T & Tmin( const T & a, const T & b )
{
	return (b < a) ? b : a;
}

template<typename T>
inline const T & Tmax( const T & a, const T & b )
{
	return (a < b) ? b : a;
}

template<typename T>
inline const T & Tclamp( const T & mn, const T & mx, const T & in )
{
	return Tmin( Tmax( in, mn ), mx );
}

template<typename T>
inline T Tblend( const T & c0, const T & c1, float alpha )
{
	return (1.0f-alpha) * c0 + (alpha) * c1;	
}

template<typename T>
inline unsigned long Tstep( const T & a, const T & x )
{
	return ((unsigned long)(x < a));
}

template<typename T>
inline T Tpulse( const T & a, const T & b, const T & x )
{
	return Tstep( a, x ) - Tstep( b, x );
}

template<typename T>
inline T Tabs( const T & a )
{
	return (a < (T)0) ? -a : a ;
}

template<typename T>
inline T Tsmoothstep( const T & a, const T & b, const T & x )
{
	if( x < a ) {
		return ((T)0);
	}
	else if( x >= b ) {
		return ((T)1);
	}
	x = (x-a)/(b-a);
	return (x*x * ((T)3-(T)2*x));
}

template<typename T>
inline T Tmod( const T & a, const T & b )
{
	int n = (int)(a/b);
	a -= n*b;
	if( a < (T)0 ) {
		a += b;
	}
	return a;
}

template<typename T>
inline int Tintfloor( const T & a )
{
	return ( ((int)a) - (a<(T)0 && a!=((int)a)) );
}

template<typename T>
inline int Tintceil( const T & a )
{
	return ( ((int)a) + (a>(T)0 && a!=((int)a)) );
}

template<typename T>
inline T Tfloor( const T & a )
{
	return ((T)( ((int)a) - (a<(T)0 && a!=((int)a)) ));
}

template<typename T>
inline T Tceil( const T & a )
{
	return ((T)( ((int)a) + (a>(T)0 && a!=((int)a)) ));
}

template<typename T>
inline T Tgammacorrect( const T & gamma, const T & x )
{
	return ((T) pow( x, (1.0/((double)gamma)) ));
}

template<typename T>
inline T Tbias( T b, T x )
{
	return ((T) pow( x, (log(b)/log(0.5) ) ));
}

template<typename T>
inline T Tgain( const T & b, const T & x )
{
	if( x < 0.5 ) {
		return Tbias( 1-g, 2*x ) / 2;
	}
	return 1 - Tbias( 1-g, 2 - 2*x ) / 2;
}

template<typename T>
inline T Trad2deg( const T & x )
{
	return (x)*(180.0/Tpi);
}

template<typename T>
inline T Tdeg2rad( const T & x )
{
	return (x)*(Tpi/180.0);
}

template<typename T>
inline bool Tnear( const T & a, const T & b, float epsilon )
{
	return (((float)Tabs(a-b)) < epsilon);
}


#endif
