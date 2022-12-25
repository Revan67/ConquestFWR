#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* #include "BaseUTF.h" */
#include "sgi_utf.h"
#include "cmp.h"
#include "persistcompound.h"
#include "3db.h"

int verbose_level=1;
int mip_flag=1;
int flag_565=0;

void Usage(char* argv0);

int main(int argc, char *argv[])
{
  int i;
  file_node *root=NULL;
  CompoundObject c_obj;

  if(argc<2)
    Usage(argv[0]); 

  root=ReadUTF(argv[1]);
  LoadCompoundObject(&c_obj, root);
  FreeTree(root);

  for(i=0;i<c_obj.fix_count;i++){
    PrintFix(&(c_obj.fix_list[i]));
  }

  for(i=0;i<c_obj.pris_count;i++){
    PrintPris(&(c_obj.pris_list[i]));
  }

  for(i=0;i<c_obj.rev_count;i++){
    PrintRev(&(c_obj.rev_list[i]));
  }

  for(i=0;i<c_obj.sphere_count;i++){
    PrintSphere(&(c_obj.sphere_list[i]));
  }

  for(i=0; i<c_obj.script_count;i++){
    PrintScript(&(c_obj.script_list[i]), i);
  }

  for(i=0; i<c_obj.n_channel_count; i++){
    PrintNChannel(&c_obj, &(c_obj.n_channel_list[i]), i);
  }

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

  fprintf(stderr,"%s\n",buffer);
  fflush(stderr);

  return 0;
}
