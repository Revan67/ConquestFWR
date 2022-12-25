#include <stdlib.h>
#include <string.h>
#include "misc.h"

#ifndef SGI
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#endif

// size_t is unsigned int
void* Malloc(const size_t size)
{
  if(size == 0)
  {
    Winprint("Error: malloc 0 bytes.\n");
    return (void*)0;
  }

  if(size > 200 * 1024 * 1024)
  {
    Winprint("Error: malloc %d bytes!\n", size);
	Dbreak();
    exit(1);
  }

  void *ptr = malloc(size);
  if(ptr == (void*)0)
  {
    Winprint("Error: malloc of %d bytes failed!\n", size);
	Dbreak();
    exit(1);
  }
  return ptr;
}

void* Realloc(void *ptr, const size_t size)
{
  if(size == 0)
  {
    Winprint("Error: realloc 0 bytes.\n");
    Free(ptr);
    return (void*)0;
  }
  
  if(size > 200 * 1024 * 1024)
  {
    Winprint("Error: realloc %d bytes!\n", size);
	Dbreak();
    exit(1);
  }

  ptr = realloc(ptr, size);
  if(ptr == (void*)0)
  {
    Winprint("Error: realloc of %d bytes failed!\n", size);
	Dbreak();
    exit(1);
  }
  return ptr;
}

void MemSwap(void *a, void *b, const int size)
{	
	if(a != b)
	{
		int words = (int)(size / sizeof(unsigned int));
		
		for(int i = 0; i < words; i++)
		{
			*(unsigned int*)b ^= *(unsigned int*)a;
			*(unsigned int*)a ^= *(unsigned int*)b;
			*(unsigned int*)b ^= *(unsigned int*)a;

			a = (void*)((unsigned int*)a + 1);
			b = (void*)((unsigned int*)b + 1);
		}

		// remainder
		words = size - words * sizeof(unsigned int);
		for(i = 0; i < words; i++)
		{
			*(unsigned char*)b ^= *(unsigned char*)a;
			*(unsigned char*)a ^= *(unsigned char*)b;
			*(unsigned char*)b ^= *(unsigned char*)a;

			a = (void*)((unsigned char*)a + 1);
			b = (void*)((unsigned char*)b + 1);
		}
	}
}

void StripPath(char *name)
{
  char *previous=NULL;
  char *current;

  // strip unix path
  current=strtok(name, "/");
  while(current)
  {
    previous=current;
    current=strtok(NULL, "/");
  } 
  memmove(name,previous,strlen(previous)+1);
  //strcpy(name,previous);

  // strip dos path
  current=strtok(name, "\\");
  while(current){
    previous=current;
    current=strtok(NULL, "\\");
  }
  memmove(name,previous,strlen(previous)+1);
  //strcpy(name,previous);
}

void StripExtension(char *name)
{
	int i = strlen(name) - 1;
	while(i--)
	{
		if(name[i] == '.')
		{
			name[i] = 0;
			return;
		}
	}
}

#ifdef _DEBUG
#include <float.h>
void TrapFpu(bool on)
{
#ifdef WIN32
	_clear87();

	if(on)
	{
		// prepare to trap floating point exceptions
		unsigned int control_word = _control87 (0, 0);

		//control_word &= ~(EM_INVALID | EM_UNDERFLOW | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
		control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
		//control_word &= ~(EM_OVERFLOW | EM_ZERODIVIDE);
		_control87 (control_word, MCW_EM);
	}
	else
	{
		_control87(_CW_DEFAULT, 0xfffff);
	}
	
	// set precision
	//_control87( _PC_24, MCW_PC );
	//_control87( _PC_64, MCW_PC );

	// set rounding mode /QIfist
	//_control87( _RC_CHOP, MCW_RC); // correct
	//_control87( _RC_NEAR, MCW_RC); // fast CRT default

#endif
}
#else
void TrapFpu(bool){}
#endif
