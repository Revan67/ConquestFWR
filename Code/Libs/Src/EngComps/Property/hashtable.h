#ifndef HASHTABLE_H
#define HASHTABLE_H

class HashTable {
  protected:
	// This implementes a hash-keyed associative array
	// in the same vain as a perl assoc. array

	int hashTableSize;
		// Number of pointers in the hash table
	int numRecords;
		// Actual active records in the table
	char **hashTable;
		// Array of key pointers, which are of the form:
		// 1 byte flags:
	enum {
		aaDELETED = 0x01,
		aaCHANGED = 0x02
	};
//		#define aaDELETED (0x01)
//		#define aaCHANGED (0x02)
		// followed by 0-terminated keyt string
		// followed by data

	unsigned int hash( const char *key );
		// hashing function

	int set( char **_hashTable, int _hashTableSize, const char *key, char *_recPtr );
		// Internal method to functionalize the during rebuild of tables
		// Returns 1 if the key is new, 0 if it is replacing an existing

  public:
	HashTable();
	~HashTable();

	int lookup( const char *key );
		// Returns the index of key, or -1 if it wasn't found

	char *getKey( int i );
	char *getVal( int i );
		// Gets the i_th key/val in table. 
		// Useful when dumping the table contents

	int getCount();
		// Returns the number of records in the table.

	int get_ith_index (int i);
		// Returns the table index (i.e. same kind of output as lookup) for ith value in the table.

	char *get( const char *key, int retEmptyString=0 );
		// Get the value associated with key.
		// if retEmptyString is true then a search that
		// fails will return an empty string ("") 
		// instead of NULL

	virtual void set( const char *key, const char *value, int len=-1 );
		// Sets the hash record key with value, overwriting
		// any existing value if there is one.  len is
		// the length of the value, or -1 if it is 
		// a null terminated string whose len should
		// be calculated.
		//
		// The buffer pointed to by value is always
		// copied into this buffer of len.
		//
		// Setting a key to NULL means you want to kill
		// the key.  Internally, it is marked as deleted.
		//
		// Returns the index of the array, although this
		// is typically ignored.

	void clear();
		// kill everything, start fresh

	int size() { return hashTableSize; }

	void copyFrom( HashTable &table );
		// Copies all the items from table.
		// Does *NOT* clear first, if you want to
		// start off fresh, call clear() first

	// The hash table  keeps track of what has changed with a 
	// flag for each object.  This can be useful for loading and saving
	// differences, for example in the config system.
	int hasChanged( int i );
	int hasChanged( const char *key );
	void setChanged( int i );
	void setChanged( const char *key );
	void clearChanged( int i );
	void clearChanged( const char *key );
	void clearChangedAll();
};

struct IntTable : public HashTable {
	int getInt( int index ) {
		char *ptr = getVal( index );
		if( ptr ) {
			return *((int *)ptr);
		}
		return 0;
	}

	int getInt( const char *key ) {
		char *ptr = get( key );
		if( ptr ) {
			return *((int *)ptr);
		}
		return 0;
	}

	void set( const char *key, int value ) {
		HashTable::set( key, (char *)&value, sizeof(int) );
	}
};

struct BufferTable : public IntTable {
	// This is used to hold memory buffers, indexed by name.
	// If the btOWN_MEMORY is set, then when you kill or
	// change a key's value, it will deallocate the memory

	int flags;
	enum { btOWN_MEMORY = 0x01 };
//		#define btOWN_MEMORY (0x01)

	char *get( const char *key, int retEmptyString=0 ) {
		// Returns the pointer to the buffer assigned as 
		// opposed to a pointer to the pointer to the buffer 
		// as would be the case if this we not overloaded
		// Ignore the retEmptyString, not used.
		return (char *)getInt( key );
	}

	virtual void set( const char *key, char *value, int len=-1 ) {
		if( flags & btOWN_MEMORY ) {
			char *old = (char *)getInt( key );
			if( old ) {
				delete old;
			}
		}
		IntTable::set( key, (int)value );
	}

	BufferTable( int _flags = btOWN_MEMORY ) {
		flags = _flags;
	}
};

#endif
