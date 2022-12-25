#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include "BaseUTF.h" */
#include "sgi_utf.h"
#include "3db.h"

int verbose_level=1;
int mip_flag=1;
int scale_flag=0;
float scale_factor=1.0;
int new_format_flag;

void Usage(char* argv0);

int main(int argc, char *argv[])
{
  file_node *root=NULL;
  object obj;

  if(argc<3)
    Usage(argv[0]); 

  new_format_flag=0;
  root=ReadUTF(argv[1]);
  WriteUTF(root, argv[2]);
  FreeTree(root);

  return 0;
}

void Usage(char *argv0)
{
  fprintf(stderr,"Attempts to fix/reset a UTF file.\n");
  fprintf(stderr,"Usage: %s <old.utf> <new.utf>\n",argv0);
  exit(1);  
}
