#ifndef SYMTABLE_H
#define SYMTABLE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                SymTable.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//
/*
	SymbleTable class - used to store symbols in an efficient (O(log n) 
	insertion and retrieval) way.

	//--------------------------------------------------------------------------//
	void free(void);
	OUTPUT:
		clears all symbols from all levels. Returns class to initial state.

	//--------------------------------------------------------------------------//
	void set_reserved(void);
	OUTPUT:
		Inserts all of the predefined Pascal symbols into the list at level 0.
		Further insertions into the symbol list will be done at level 1.

	//--------------------------------------------------------------------------//
	SYMBOL get_symbol(char *_name, unsigned hashval, char create = 0, int *level=0);
	INPUT:
		_name: ASCIIZ for identifying the symbol. Must be unique for
			each level.
		hashval: 32 bit value used for searching the table
		create: 0 if function should return NULL on failure,
				1 if function should create a new symbol if not already in list
		level: pointer to an integer.
				If non-null, returns the level that the variable was found in.
	RETURNS:
		Returns a pointer to symbol matching both the name and hashval input vraiables.
	OUTPUT:
		May add a new symbol to the table if 'create'==1 and symbol	was not
		already found.

	//--------------------------------------------------------------------------//
	SYMBOL create_symbol(char *_name, unsigned hashval, int level);
	INPUT:
		_name: ASCIIZ for identifying the symbol. Must be unique for
			each level.
		hashval: 32 bit value used for searching the table
		level: which level to insert the symbol
	RETURNS:
		Returns a pointer to symbol matching both the name and hashval input vraiables.
	OUTPUT:
		Calls the error handling routine if the symbol was already 
		present in the table at that level.

	//--------------------------------------------------------------------------//
	int set_current_level(int new_level);   // return new current level
	INPUT:
		new_level: sets new 'current_level'. All furthur insertions
			into the table will be at the new level. 
	OUTPUT:
		All levels of the table above the new level will be erased.

	//--------------------------------------------------------------------------//
	unsigned get_hash(char *_name);
	INPUT:
		_name: ASCIIZ name for a symbol
	RETURNS:
		32 bit integer which is the result of the hash function
		performed on the '_name' string. This result should be
		used when calling other symbol table functions which 
		require a hashval.

	//--------------------------------------------------------------------------//
	SYMBOL insertbt(char *_name, int basictp, int siz);
	INPUT:
		_name: ASCIIZ name for a symbol
		basictp: which kind of basic type this is
		siz: number of bytes type uses
	RETURNS:
		Symbol table entry for type.
	OUTPUT:
		Creates a TYPESYM, and inserts it into the table.

	//--------------------------------------------------------------------------//
	SYMBOL insertfn(char *_name, SYMBOL resulttp, SYMBOL argtp);
	INPUT:
		_name: ASCIIZ name for a symbol
		resulttp: Pointer to a type symbol of the result type for the function.
		argtp: Pointer to a type symbol of the first argument for the function.
	RETURNS:
		Symbol table entry for the function.
	OUTPUT:
		Creates a FUNCTIONSYM, for a function with one argument.

	//--------------------------------------------------------------------------//
	void print_table(void);
	OUTPUT:	
		Prints all of the symbols for every level to stdout.

	//--------------------------------------------------------------------------//
	void print_node(SYMBOL node);
	OUTPUT:
		Prints the symbol to stdout.

//--------------------------------------------------------------------------//
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct Symbol;
typedef Symbol *SYMBOL;

/* Define kinds of symbols.  The kind field should be one of these. */
enum SYMKIND {
	ARGSYM,
	BASICTYPE,
	CONSTSYM,
	VARSYM,
	ENUMSYM,
	FUNCTIONSYM,
	ARRAYSYM,
	RECORDSYM,
	TYPESYM,
	POINTERSYM,
	NAMESYM
};


struct Symbol
{
	unsigned long dwRefs;
	char 	*name;
	unsigned hash;
  	SYMBOL link;
  	SYMBOL left;
  	SYMBOL right;

	SYMKIND kind;                /* kind of symbol -- see defines below. */
	int    basicdt;             /* type code for basic data types       */
	SYMBOL datatype;    /* pointer for more complex data types  */
	int    blocklevel;
	int    size;
	int    offset;
	int    alignment;
	union  { 
	 char  *stringconst;
	 long  intnum;
	 float realnum; 
	} constval;
	int    arrayval;		/* number of elements in array (in ARRAYSYM) */
	int    modifiers;		/* type modifiers e.g. unsigned, volatile, etc. */
	unsigned long internal_refs;	/* number of times elememts of this struct self reference (RECORDSYM) */
	unsigned bit_offset:8;
	unsigned bit_count:8;

	Symbol(void)
	{
	 	memset(this, 0, sizeof(*this));
	}

	void * operator new (size_t size);

	unsigned long Release (void);

	unsigned long AddRef (void);
};


//----------------------------
// modifier flags
//----------------------------

#define MODIFIER_UNSIGNED		0x00000001
#define MODIFIER_READONLY		0x00000002
#define MODIFIER_HEXVIEW		0x00000004
#define MODIFIER_SPELLCHECK		0x00000008
#define MODIFIER_FILENAME		0x00000010
#define MODIFIER_COLOR  		0x00000020


#define MAX_LEVELS 	16

class SymbolManager
{
	SYMBOL SymbolArray[MAX_LEVELS];	
	int current_level;
	static char *search_name;
	static unsigned search_hashval;

	void remove_tree(int level);
	static void remove_node(SYMBOL sym);
	static SYMBOL find_node(SYMBOL sym);		
	void add_node (SYMBOL sym);

	SYMBOL CreateBOOL (void);

public:

	SymbolManager(void);
	
	~SymbolManager(void)
	{
		free();
	}

	void free(void);

	SYMBOL set_reserved(void);

	SYMBOL get_symbol(char *_name, char create=0)
	{	
		return get_symbol(_name, get_hash(_name), create);
	}

	SYMBOL get_symbol(char *_name, unsigned hashval, char create = 0);

	SYMBOL create_symbol(char *_name, unsigned hashval, int level);

	SYMBOL create_symbol(char *_name)
	{
		return create_symbol(_name, get_hash(_name), current_level);
	}

	SYMBOL insertbt(char *_name, int basictp, int siz);

	SYMBOL insertfn(char *_name, SYMBOL resulttp, SYMBOL argtp);

	int set_current_level(int new_level);   // return new current level
	
	int inc_current_level(void)
	{
		return ++current_level;
	}

	int dec_current_level(void)
	{
		return set_current_level(current_level-1);
	}

	int get_current_level(void)
	{
		return current_level;
	}
	
	void push_namespace (SYMBOL name);

	void pop_namespace (SYMBOL name)
	{
		dec_current_level();
	}

	void print_table(void);

	static void print_node(SYMBOL node);

	static unsigned get_hash(char *_name);
};

//--------------------------------------------------------------------------//
//---------------------------End SymTable.h---------------------------------//
//--------------------------------------------------------------------------//



#endif