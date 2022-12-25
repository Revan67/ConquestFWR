#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* #include "BaseUTF.h" */
#include "sgi_utf.h"

void Usage(char* argv0);
int verbose_level=1;

int main(int argc, char *argv[])
{
  file_node *root=NULL;

  if(argc<2)
    Usage(argv[0]); 

  root=ReadUTF(argv[1]);
  PrintTree(root);
  FreeTree(root);

  return 0;
}

void Usage(char *argv0)
{
  fprintf(stderr,"Usage: %s <infile.utf>\n",argv0);
  exit(1);  
}

int Winprint(const char *format, ...)
{
  va_list args;
  char buffer[256];

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  fprintf(stdout,"%s\n",buffer);
  fflush(stdout);

  return 0;
}

