#ifndef FILESTRM_H
#define FILESTRM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               FileStrm.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2002 By Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

/*
	FILESTREAM CLASS:
		Meant to provide faster routines for getchar() and peekchar(), 
		by reading the whole file in at one time.


	int open(char *filename, char fatal=1);
	INPUT: 
		filename: name of file to open.
		fatal: Call the Fatal() routine if the file cannot be opened.
			Defaults to 1. (Fatal will be called.)
	OUTPUT:
		Attempts to open the file for READ ONLY.
	RETURNS:
		non-zero if file was opened.
//--------------------------------------------------------------------------//

	void close(void);
	OUTPUT:
		Closes the input file. No further reading is allowed.

//--------------------------------------------------------------------------//
	char get_char(void);
	OUTPUT:
		Retrieves the next character from the input file, and increments
		an internal pointer to the next character. In effect, this "eats"
		the character.
	RETURNS:
		The next character in the input file, or EOF.

//--------------------------------------------------------------------------//
	char peek_char(void);
	OUTPUT:
		Retrieves the next character from the input stream without
		incrementing the internal pointer to the next character. 
	RETURNS:
		The next character in the input file, or EOF.

//--------------------------------------------------------------------------//
	char peek2_char(void);
	RETURNS:
		Returns the character AFTER the next character from the 
		input stream. Can return EOF if either the current character
		or the next character lie after the EOF.

//--------------------------------------------------------------------------//
	static char *get_current_filename(void);
	RETURNS:
		The name of the file that was last opened, and is still open.
		The function returns NULL if no file is currently open.
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

class FileStream
{
public:
	const U8 *buffer;
	int buffer_index;
	bool bOwnMemory;

	FileStream(void);

	~FileStream(void)
	{
		close();
	}

	int open(const char *filename, char fatal=1);

	void close(void);

#ifndef USING_CC
	
	U8 get_char(void)
	{
		if (buffer[buffer_index])	
			return buffer[buffer_index++];
		return 0;
	}

	U8 peek_char(void)
	{
		return buffer[buffer_index];
	}

	U8 peek2_char(void)
	{
		if (buffer[buffer_index])
			return buffer[buffer_index + 1];
		return 0;
	}

#else
	U8 get_char(void);
	U8 peek_char(void);
	U8 peek2_char(void);
	static char *get_current_filename(void);

#endif
	
};

//--------------------------------------------------------------------------//
//------------------------------End FileStrm.h------------------------------//
//--------------------------------------------------------------------------//

#endif
