// handlemap


#ifndef _HANDLEMAP_
#define _HANDLEMAP_

//

#pragma warning( disable : 4786 )

#include <map>

#pragma warning( disable : 4786 )

//

template< class _Hnd, class _Ty, class _Pr = std::less<_Hnd>, class _A = std::allocator<_Ty> >
class handlemap 
{
public:	// Interface

	typedef typename std::map< _Hnd, _Ty, _Pr, _A >	database_type;
	typedef _Ty								value_type;
	typedef std::pair<_Hnd,_Ty>				pair_type;
	typedef typename database_type::iterator			iterator;
	typedef typename database_type::const_iterator	const_iterator;
	typedef typename database_type::size_type		size_type;
	
	// NOTE: the iterator returned by handlemap methods will
	// always be consistent with a handlemap::pair_type
	//
	// i.e. typeof( find( foo )->first ) == _Hnd
	// and  typeof( find( foo )->secont ) == _Ty

	//

	handlemap( _Hnd _InitHnd = 0 ) 
	{
		next_handle = _InitHnd;

		cache_checks = 0;
		cache_hits = 0;
		cache_valid = false;
	}
	
	// finds and returns the next available free handle
	// to an item in the database
	//
	_Hnd allocate_handle( void )
	{
		return next_handle++;
	}

	// find an item in the database given a handle
	//
	iterator find( const _Hnd &_H ) 
	{
		iterator it;
		
		cache_checks++;

		if( cache_valid && cache->first == _H ) {
			cache_hits++;
			return cache;
		}
		else if( (it = database.find( _H )) != end() ) {
			cache = it;
			cache_valid = true;
		}

		return it;
	}

	// find an item in the database given a handle
	//
	const_iterator find( const _Hnd &_H ) const
	{
		iterator it;
		
		cache_checks++;

		if( cache_valid && cache->first == _H ) {
			cache_hits++;
			return cache;
		}
		else if( (it = (const_cast<HANDLEMAP&> (*this)).database.find( _H )) != end() ) {
			cache = it;
			cache_valid = true;
		}

		return it;
	}

	// insert a key value pair into the database
	// returns an iterator to the inserted pair
	//
	iterator insert( const _Hnd &_H, const value_type &_V = value_type() )
	{
		iterator it;

		if( (it = database.insert( pair_type( _H, _V ) ).first) != end() ) {
			cache = it;
			cache_valid = true;
		}

		return it;
	}

	// remove the element attached to _H from the database.
	// does not affect outstanding handles.
	// 
	// TODO: does this affect outstanding iterators?  in a map, no,
	// TODO: but in a vector, yes?
	//
	size_type erase( const _Hnd &_H )
	{
		if( cache_valid && cache->first == _H ) {
			cache_valid = false;
		}
		return database.erase( _H );
	}

	// remove an element from the database.
	// does not affect outstanding handles.
	// returns the element immediately after _It or end() if none.
	//
	// TODO: does this affect outstanding iterators?  in a map, no,
	// TODO: but in a vector, yes?
	//
	iterator erase( iterator _It )
	{
		if( cache_valid && cache->first == _It->first ) {
			cache_valid = false;
		}
		return database.erase( _It );
	}

	// returns the number of elements currently in the database
	//
	//
	size_type size() const
	{
		return database.size();
	}

	// clear all elements in the database
	//
	//
	void clear() 
	{
		cache_valid = false;

		database.clear();
	}

	// return the first element in the database
	//
	const_iterator begin() const
	{
		return database.begin();
	}

	// return the first element in the database
	//
	iterator begin() 
	{
		return database.begin();
	}

	// return an iterator to the element after the last element
	//
	const_iterator end() const
	{
		return database.end();
	}

	// return an iterator to the element after the last element
	//
	iterator end() 
	{
		return database.end();
	}

	// these really should be read only
	//
	mutable int cache_checks;
	mutable int cache_hits;

protected:	// Data
	database_type database;
	_Hnd next_handle;

	mutable iterator cache;
	mutable bool cache_valid;

private:
	typedef handlemap<_Hnd, _Ty, _Pr, _A> HANDLEMAP;
};

//

#ifdef ENGINE_H

//

template< typename _Ty >
class arch_handlemap : public handlemap< ARCHETYPE_INDEX, _Ty >
{
};

//

template< typename _Ty >
class inst_handlemap : public handlemap< INSTANCE_INDEX, _Ty >
{
};

#endif

//

#ifdef __IRENDERCOMPONENT_H

template< typename _Ty >
class rarch_handlemap : public handlemap< RENDER_ARCHETYPE, _Ty >
{
};

#endif

//

#endif 
