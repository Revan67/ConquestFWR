// Copyright (C) 2002-2004 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#ifndef __IRR_RECT_H_INCLUDED__
#define __IRR_RECT_H_INCLUDED__

template <class T> class TPoint
{
public:
	T X;
	T Y;

	TPoint()
		: X(0), Y(0) {};

	TPoint(T x, T y)
		: X(x), Y(y) {};

	TPoint<T> operator + (const TPoint<T>& pos) const
	{
		TPoint<T> ret(*this);
		ret.X += pos.X;
		ret.Y += pos.Y; 
		return ret;
	}

	TPoint<T> operator - (const TPoint<T>& pos) const
	{
		TPoint<T> ret(*this);
		ret.X -= pos.X;
		ret.Y -= pos.Y; 
		return ret;
	}

	const TPoint<T>& operator += (const TPoint<T>& pos)
	{
		X += pos.X;
		Y += pos.Y;
		return *this;
	}

	const TPoint<T>& operator -= (const TPoint<T>& pos)
	{
		X -= pos.X;
		Y -= pos.Y;
		return *this;
	}
};


template <class T> class TRect
{
public:
	TRect()
		: UpperLeftCorner(0,0), LowerRightCorner(0,0) {};
	
	
	TRect(T x, T y, T x2, T y2)
		: UpperLeftCorner(x,y), LowerRightCorner(x2,y2) {};
	
	
	TRect(const TPoint<T>& upperLeft, const TPoint<T>& lowerRight)
		: UpperLeftCorner(upperLeft), LowerRightCorner(lowerRight) {};
	
	TRect(const TRect<T>& other)
		: UpperLeftCorner(other.UpperLeftCorner), LowerRightCorner(other.LowerRightCorner) {};
	
	
	TRect<T> operator+(const TPoint<T>& pos) const
	{
		TRect<T> ret(*this);
		ret.UpperLeftCorner += pos;
		ret.LowerRightCorner += pos;
		return ret;
	}
	
	const TRect<T>& operator+=(const TPoint<T>& pos)
	{
		UpperLeftCorner += pos;
		LowerRightCorner += pos;
		return *this;
	}
	
	bool operator == (const TRect<T>& other) const
	{
		return UpperLeftCorner == other.UpperLeftCorner && LowerRightCorner == other.LowerRightCorner;
	}
	
	
	bool operator != (const TRect<T>& other) const
	{
		return UpperLeftCorner != other.UpperLeftCorner || LowerRightCorner != other.LowerRightCorner;
	}
	
	const TRect<T>& operator = (const TRect<T>& other)
	{
		UpperLeftCorner = other.UpperLeftCorner;
		LowerRightCorner = other.LowerRightCorner;
		return *this;
	}
	
	bool isPointInside(const TPoint<T>& pos) const
	{
		return UpperLeftCorner.X <= pos.X && UpperLeftCorner.Y <= pos.Y &&
			LowerRightCorner.X >= pos.X && LowerRightCorner.Y >= pos.Y;
	}
	
	bool isRectCollided(const TRect<T>& other) const
	{
		return (LowerRightCorner.Y > other.UpperLeftCorner.Y && UpperLeftCorner.Y < other.LowerRightCorner.Y &&
			LowerRightCorner.X > other.UpperLeftCorner.X && UpperLeftCorner.X < other.LowerRightCorner.X);
	}
	
	void clipAgainst(const TRect<T>& other) 
	{
		if (other.LowerRightCorner.X < LowerRightCorner.X)
			LowerRightCorner.X = other.LowerRightCorner.X;
		if (other.LowerRightCorner.Y < LowerRightCorner.Y)
			LowerRightCorner.Y = other.LowerRightCorner.Y;
		
		if (other.UpperLeftCorner.X > UpperLeftCorner.X)
			UpperLeftCorner.X = other.UpperLeftCorner.X;
		if (other.UpperLeftCorner.Y > UpperLeftCorner.Y)
			UpperLeftCorner.Y = other.UpperLeftCorner.Y;
	}
	
	T getWidth() const
	{
		return LowerRightCorner.X - UpperLeftCorner.X;
	}
	
	T getHeight() const
	{
		return LowerRightCorner.Y - UpperLeftCorner.Y;
	}
	
	void repair()
	{
		if (LowerRightCorner.X < UpperLeftCorner.X)
		{
			s32 t = LowerRightCorner.X;
			LowerRightCorner.X = UpperLeftCorner.X;
			UpperLeftCorner.X = t;
		}
		
		if (LowerRightCorner.Y < UpperLeftCorner.Y)
		{
			s32 t = LowerRightCorner.Y;
			LowerRightCorner.Y = UpperLeftCorner.Y;
			UpperLeftCorner.Y = t;
		}
	}
	
	bool isValid()
	{
		return ((LowerRightCorner.X - UpperLeftCorner.X) *
			(LowerRightCorner.Y - UpperLeftCorner.Y) >= 0);
	}
	
	TPoint<T> getCenter() const
	{
		return TPoint<T>((UpperLeftCorner.X + LowerRightCorner.X) / 2,
			(UpperLeftCorner.Y + LowerRightCorner.Y) / 2);
	}
	
	TPoint<T> UpperLeftCorner;
	TPoint<T> LowerRightCorner;
};

#endif
