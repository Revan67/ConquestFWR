#ifndef __CACHEMAP_H
#define __CACHEMAP_H

#include <map>

//WARNING this template uses operator == on the Key type to check for a cache hit.
//std::map doesn't use or require operator ==. Make sure that operator == is consistent
//with the Pred type used.

template<class Key, class T, class Pred = std::less<Key>, class A = std::allocator<T> > class cachemap : public std::map<Key, T, Pred, A>
{
public:
    explicit cachemap(const Pred& comp = Pred(), const A& al = A()) : MAP (comp, al)
	{ 
		cache = end (); 
	}

    cachemap(const cachemap& x) : MAP (x)
	{ 
		cache = end (); 
	}

	cachemap(const value_type *first, const value_type *last, const Pred& comp = Pred(), const A& al = A()) : MAP (first, last, comp, al)
	{ 
		cache = end (); 
	}

    std::pair<iterator, bool> insert(const value_type& x)
	{ 
		std::pair<iterator, bool> result = MAP::insert (x);
		cache = result.first;
		return result;
	}

    iterator insert(iterator it, const value_type& x)
	{ 
		return cache = MAP::insert (it, x);
	}

    void insert(const value_type *first, const value_type *last)
	{ 
		MAP::insert (first, last); 
		cache = end (); 
	}

    iterator erase(iterator it)
	{ 
		iterator result;

		if (!(it == cache))
			result = MAP::erase (it);
		else
		{
			result = MAP::erase (it);
			cache = end ();
		}

		return result;
	}
		
    iterator erase(iterator first, iterator last)
	{
		iterator result = MAP::erase (first, last);
		cache = end ();
		return result;
	}

    size_type erase(const Key& key)
	{
		size_type result = MAP::erase (key);
		cache = end ();
		return result;
	}

	void clear()
	{
		MAP::clear ();
		cache = end ();
	}

	void swap(cachemap x)
	{
		MAP::swap (x);
		cache = end ();
	}

    iterator find(const Key& key)
	{
		iterator result;

		if (check_cache (key))
			result = cache;
		else
		{
			result = MAP::find (key);
			cache = result;
		}

		return result;
	}

	const_iterator find(const Key& key) const
	{
		iterator result;

		if (check_cache (key))
			result = cache;
		else
		{
			result = (const_cast<CACHEMAP&> (*this)).MAP::find (key);
			cache = result;
		}

		return result;
	}

protected:

	mutable iterator cache;

	bool check_cache (const Key& key) const
	{
		return (!(end () == cache) && (*cache).first == key);
	}

private:
	typedef std::map<Key, T, Pred, A> MAP;
	typedef cachemap<Key, T, Pred, A> CACHEMAP;
};

#endif
