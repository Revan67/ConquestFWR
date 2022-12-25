#include "malloc.h"
#include "string.h"
#include "fdump.h"
//#include "stdio.h"
//#include "stdlib.h"
#include "hashtable.h"

/////////////////////////////////////////////////////////////
// HashTable 
/////////////////////////////////////////////////////////////

int HashTable::getCount()
{
	return numRecords;
}

unsigned int HashTable::hash( const char *key ) {
	// Returns a full precision hash value for key
	unsigned int last = 0;
	unsigned int sum = 0;
	while( *key ) {
		sum += (*key ^ last);
		last = *key++;
	}
	return sum;
}

int HashTable::set( char **_hashTable, int _hashTableSize, const char *key, char *_recPtr ) {
	int keyLen = strlen( key ) + 1;

	int hashRec = hash( key ) % _hashTableSize;
	int total = _hashTableSize;
	int foundKey = 0;
	char *recPtr;
	int destHashRec = -1;
	while( total-- ) {
		recPtr = _hashTable[hashRec];
		if( recPtr ) {
			// Each hash record is actually a small structure
			// 1st byte is 1 if active, 0 if deleted
			// Starting at the second byte is a null-terminated
			// string which is the key.
			// Following the key is a null-terminated value string.
			if( *recPtr & aaDELETED ) {
				// Found a deleted key. Use this space unless the key is found.
				// NOTE: This will end up using farthest deleted key from the
				// head of the chain first.
				destHashRec = hashRec;
			}
			else if( !memcmp(key, recPtr + 1, keyLen) ) {
				// Found already existing key. Use this slot.
				foundKey = 1;
				destHashRec = hashRec;
				break;
			}
		}
		else {
			// Found an empty slot, marking the end of this chain.
			// Use this slot unless we found a deleted slot above.
			if (destHashRec == -1) {
				destHashRec = hashRec;
			}
			break;
		}
		hashRec = (hashRec+1) % _hashTableSize;
	}
	assert( total );
		// There should always be some room left since we keep
		// the list at 50% free all the time to avoid collisions
	
	// Here, destHashRec contains the index of the found record. The found record value can
	// be either NULL, a pointer to a deleted entry, or a pointer to the found entry.

	if( _recPtr == NULL ) {
		// We are deleting the key
		if( foundKey ) {
			// Only can delete it if it already eixsts
			*_hashTable[destHashRec] |= aaDELETED;
			return 1;
		}
		return 0;
	}
	else {
		// We are either adding a new key or setting the value of an old key.
		// Either way, if the destination record has a value, it must be deleted
		// before the new value is set.
		if( _hashTable[destHashRec] ) {
			free( _hashTable[destHashRec] );
		}
		*_recPtr = aaCHANGED;
		_hashTable[destHashRec] = _recPtr;
	}

	return !foundKey;
}


HashTable::HashTable() {
	hashTable = NULL;
	clear();
}

HashTable::~HashTable() {
	if( hashTable ) {
		free( hashTable );
	}
}

// Retrieve Values
//---------------------------------------------

int HashTable::lookup( const char *key ) {
	int keyLen = strlen( key ) + 1;
	int hashRec = hash(key) % hashTableSize;
	int total = hashTableSize;
	while( total-- ) {
		char *recPtr = hashTable[hashRec];
		if( recPtr ) {
			// Each hash record is actually a small structure
			// 1st byte indicates deleted or changed
			// Starting at the second byte is a null-terminated
			// string which is the key.
			// Following the key is the value buffer.
			if( !(*recPtr & aaDELETED) ) {
				// Non-deleted element, compare
				if( !memcmp(key, recPtr + 1, keyLen) ) {
					return hashRec;
				}
			}
		}
		else {
			// Empty record found before match, terminate search
			return -1;
		}
		hashRec = (hashRec+1) % hashTableSize;
	}
	return -1;
}

int HashTable::get_ith_index (int i)
{
	if (i < 0 || i >= numRecords)
	{
		return -1;
	}

	int here = 0;
	while( here < hashTableSize ) {
		char *recPtr = hashTable[here];
		if( recPtr ) {
			// Each hash record is actually a small structure
			// 1st byte indicates deleted or changed
			// Starting at the second byte is a null-terminated
			// string which is the key.
			// Following the key is the value buffer.
			if( !(*recPtr & aaDELETED) ) {
				// Non-deleted element, return or decrement count
				if (i == 0) {
					return here;
				}
				--i;
			}
		}
		++here;
	}
	return -1;
}

char *HashTable::getKey( int i ) {
	if( 0 <= i && i < hashTableSize ) {
		char *p = hashTable[i];
		if( p && *p ) {
			return p+1;
		}
	}
	return NULL;
}

char *HashTable::getVal( int i ) {
	if( 0 <= i && i < hashTableSize ) {
		char *p = hashTable[i];
		if( p && *p ) {
			return p + strlen(p+1) + 2;
		}
	}
	return NULL;
}

char *HashTable::get( const char *key, int retEmptyString ) {
	int keyLen = strlen( key ) + 1;
	int hashRec = hash(key) % hashTableSize;
	int total = hashTableSize;
	while( total-- ) {
		char *recPtr = hashTable[hashRec];
		if( recPtr ) {
			// Each hash record is actually a small structure
			// 1st byte indicates deleted or changed
			// Starting at the second byte is a null-terminated
			// string which is the key.
			// Following the key is the value data
			if( !(*recPtr & aaDELETED) ) {
				// Non-deleted element, compare
				if( !memcmp(key, recPtr + 1, keyLen) ) {
					// Found it
					return recPtr + 1 + keyLen;
				}
			}
		}
		else {
			// Empty record found before match, terminate search
			if( retEmptyString ) {
				return "";
			}
			return NULL;
		}
		hashRec = (hashRec+1) % hashTableSize;
	}
	if( retEmptyString ) {
		return "";
	}
	return NULL;
}

// Set Values
//---------------------------------------------

void HashTable::set( const char *key, const char *value, int len ) {
	if( value && numRecords > hashTableSize / 2 ) {
		// If the table is 50% utilized, then go double its size
		// and rebuild the whole hash table
		int _numRecords = 0;
		int _hashTableSize = hashTableSize * 2;
		char **_hashTable = (char **)malloc( sizeof(char*) * _hashTableSize );
		memset( _hashTable, 0, sizeof(char*) * _hashTableSize );
		for( int i=0; i<hashTableSize; i++ ) {
			if( hashTable[i] && !(hashTable[i][0] & aaDELETED) ) {
				// a non-deleted record to add to the new table
				char *key = &hashTable[i][1];
				set( _hashTable, _hashTableSize, key, hashTable[i] );
				_numRecords++;
			}
		}
		free( hashTable );
		hashTable = _hashTable;
		hashTableSize = _hashTableSize;
		numRecords = _numRecords;
	}
	if( value ) {
		int keyLen = strlen( key ) + 1;
		int valLen = (len == -1) ? strlen(value)+1 : len;
		char *recPtr = (char *)malloc( 1 + keyLen + valLen );
		*recPtr = 0;
		memcpy( recPtr+1, key, keyLen );
		memcpy( recPtr+1+keyLen, value, valLen );
		numRecords += set( hashTable, hashTableSize, key, recPtr );
	}
	else {
		// Deleting record
		numRecords -= set( hashTable, hashTableSize, key, NULL );
	}
}

void HashTable::clear() {
	if( hashTable ) {
		free( hashTable );
	}
	hashTableSize = 16;
	numRecords = 0;
	hashTable = (char **)malloc( sizeof(char*) * hashTableSize );
	memset( hashTable, 0, sizeof(char*) * hashTableSize );
}

void HashTable::copyFrom( HashTable &table ) {
	for( int i=0; i<table.size(); i++ ) {
		char *key = table.getKey(i);
		char *val = table.getVal(i);
		if( key ) {
			set( key, val );
		}
	}
}

// Change Tracking
//---------------------------------------------

int HashTable::hasChanged( int i ) {
	if( 0 <= i && i < hashTableSize ) {
		char *c = hashTable[i];
		if( c && !(*c & aaDELETED) ) {
			return (*c & aaCHANGED);
		}
	}
	return 0;
}

int HashTable::hasChanged( const char *key ) {
	int i = lookup( key );
	if( i >= 0 ) {
		return hasChanged( i );
	}
	return 0;
}

void HashTable::setChanged( int i ) {
	if( 0 <= i && i < hashTableSize ) {
		char *c = hashTable[i];
		if( c ) {
			*c |= aaCHANGED;
		}
	}
}

void HashTable::setChanged( const char *key ) {
	int i = lookup( key );
	if( i >= 0 ) {
		setChanged( i );
	}
}

void HashTable::clearChanged( int i ) {
	if( 0 <= i && i < hashTableSize ) {
		char *c = hashTable[i];
		if( c ) {
			*c &= ~aaCHANGED;
		}
	}
}

void HashTable::clearChanged( const char *key ) {
	int i = lookup( key );
	if( i >= 0 ) {
		clearChanged( i );
	}
}

void HashTable::clearChangedAll() {
	for( int i=0; i<hashTableSize; i++ ) {
		clearChanged( i );
	}
}



