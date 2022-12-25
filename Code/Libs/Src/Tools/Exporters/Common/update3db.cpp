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

  if(argc!=3)
    Usage(argv[0]); 

  root=ReadUTF(argv[1]);
  Load3DBObject(&obj, root); // sets new_format_flag
  FreeTree(root);

  Old2New(&obj);
  calcRigidBody(&obj);
  // LOD ??

  new_format_flag=1;
  root=CreateNode("\\",D);
  root->child=CreateOF3D(&obj);
  root->child->sibling=CreateRigidBody(&(obj.extents));
  WriteUTF(root, argv[2]);
  FreeObject(&obj);
  FreeTree(root);
  if(verbose_level>=2){
    printf("done w/ Write3DB\n");
  }

  return 0;
}

void Usage(char *argv0)
{
  fprintf(stderr,"Usage: %s <old.3db> <new.3db>\n",argv0);
  exit(1);  
}
