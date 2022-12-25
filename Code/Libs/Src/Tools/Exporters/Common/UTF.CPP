#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression

#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif

#include "sgi_utf.h"
#include "typedefs.h"

void InitHeader(struct UTF_HEADER *header, file_node *root, const int name_len)
{
  int pad;

  sprintf((char*)&header->dwIdentifier,"UTF ");
  header->dwVersion=UTF_VERSION;

  header->dwDirectoryOffset=sizeof(struct UTF_HEADER);
  header->dwDirectorySize=GetFileCount(root)*sizeof(struct UTF_DIR_ENTRY);

  header->dwUnusedEntryOffset=0;
  header->dwDirEntrySize=sizeof(struct UTF_DIR_ENTRY);

  header->dwNamesOffset=header->dwDirectoryOffset+header->dwDirectorySize;

  header->dwNameSpaceUsed = pad = name_len; //(1+GetNameSize(root))*sizeof(char);

  if(pad%4)
  {
    pad += (4-pad%4);
  }
  pad+=4;

  header->dwNameSpaceSize=pad;
  header->dwDataStartOffset=header->dwNamesOffset+header->dwNameSpaceSize;

  //header->dwUnusedSpaceOffset=header->dwDataStartOffset+GetDataSize(root);
  header->dwUnusedSpaceOffset=0;
  header->dwUnusedSpaceSize=0;

  GetFileTime(&(header->LastWriteTime.dwLowDateTime),
              &(header->LastWriteTime.dwHighDateTime));
}

/* count number of files to put into UTF */
int GetFileCount(file_node *node)
{
  int count=0;

  if(node->sibling)
    count+=GetFileCount(node->sibling);
  if(node->child)
    count+=GetFileCount(node->child);

  return (count+1);
}

// calc needed space for names table
/*
int GetNameSize(file_node *node)
{
  int size=0;

  if(node->sibling)
    size+=GetNameSize(node->sibling);
  if(node->child)
    size+=GetNameSize(node->child);

  return (size+strlen(node->name)+1); // +1 for 0 termination
}
*/

/* calc neded space for data */
int GetDataSize(file_node *node)
{
  int size=0;

  if(node->sibling)
    size+=GetDataSize(node->sibling);
  if(node->child)
    size+=GetDataSize(node->child);

  return (size+node->data_size);
}
  
/* asign file_node values */
void InitDirEntries(file_node *node, file_node *previous, int mode)
{
  static int dir_offset;
  //static int name_offset;
  static int data_offset;

  if(mode==RESET)
  {
    dir_offset=0;
    //name_offset=1; // always includes 0
    data_offset=0;
  }

  /* if(previous==NULL){             // check for root node */
  if(!strcmp(node->name,"\\")){      // check for root node
    node->d_entry.dwNext=0;          // / is the only top entry
    //node->d_entry.dwName=name_offset;
	assert(node->d_entry.dwName == 1);
    node->d_entry.dwAttributes=0x00000010;  
    node->d_entry.Sharing.read=0;
    node->d_entry.Sharing.write=0;
    node->d_entry.Sharing.readSharing=0;
    node->d_entry.Sharing.writeSharing=0;
    if(node->child){
      node->d_entry.dwDataOffset=sizeof(struct UTF_DIR_ENTRY);
    }
    else{
      node->d_entry.dwDataOffset=0; // empty tree
    }
    node->d_entry.dwSpaceAllocated=0;
    node->d_entry.dwSpaceUsed=0;
    node->d_entry.dwUncompressedSize=0;
    node->d_entry.DOSCreationTime=
    node->d_entry.DOSLastAccessTime=
    node->d_entry.DOSLastWriteTime=GetDOSTime();
  }
  else{
    /* assign parent's dwDataOffset to myself */
    if((previous->type==D) && (node==previous->child)){
      previous->d_entry.dwDataOffset=dir_offset; /* fixing 0 assignment */
    }

    if(node->sibling){
      node->d_entry.dwNext=dir_offset+sizeof(struct UTF_DIR_ENTRY);
    }
    else{
      node->d_entry.dwNext=0;
    }
    //node->d_entry.dwName=name_offset;

    switch(node->type)
    {
    case D:
      node->d_entry.dwAttributes=0x00000010;  
      /* dwDataOffset is temporarily set to 0.
         child will assign this correctly later when it is placed */
      node->d_entry.dwDataOffset=0;
      node->d_entry.dwSpaceAllocated=0;
      node->d_entry.dwSpaceUsed=0;
      node->d_entry.dwUncompressedSize=0;
      break;
    case F:
      node->d_entry.dwAttributes=0x00000080;  
      node->d_entry.dwDataOffset=data_offset;

      node->d_entry.dwSpaceAllocated = (8 > node->data_size) 
		                             ?  8 : node->data_size;
	  if(node->d_entry.dwSpaceAllocated % 4)
	  {
		node->d_entry.dwSpaceAllocated += (4 - node->d_entry.dwSpaceAllocated%4);
	  }
      node->d_entry.dwSpaceUsed = node->data_size;
      node->d_entry.dwUncompressedSize = node->d_entry.dwSpaceUsed;
      break;
    }
      
    node->d_entry.Sharing.read=0;
    node->d_entry.Sharing.write=0;
    node->d_entry.Sharing.readSharing=0;
    node->d_entry.Sharing.writeSharing=0;
    node->d_entry.DOSCreationTime=
    node->d_entry.DOSLastAccessTime=
    node->d_entry.DOSLastWriteTime=GetDOSTime();  
  }
  
  /* update offsets for next UTF_DIR_ENTRY */
  dir_offset += sizeof(struct UTF_DIR_ENTRY);
  //name_offset += strlen(node->name)+1;
  if(node->type == F)
  {
    data_offset += node->d_entry.dwSpaceAllocated;//node->data_size;
  }

  if(node->sibling)
    InitDirEntries(node->sibling, node, CONTINUE);
  if(node->child)
    InitDirEntries(node->child, node, CONTINUE);
}

bool CheckUTFChar(const file_node *node)
{
	if(node == NULL) return true;

	for(unsigned int i=0; i < strlen(node->name); i++)
	{
		if( node->name[i] == '<' ||
			node->name[i] == '>' ||
			node->name[i] == ':' ||
			node->name[i] == '"' ||
			node->name[i] == '/' ||
			node->name[i] == '\\' ||
			node->name[i] == '|')
		{
			Winprint("Error: %s is an invalid name because of the '%c' character!\n",
				node->name, node->name[i]);
			return false;
		}
	}

	bool result = CheckUTFChar(node->sibling);

	if(result)
	{
		return CheckUTFChar(node->child);
	}

	return false;
}

bool CheckUTFUnique(const file_node *node)
{
	if(node == NULL) return true;

	const file_node *n1 = node;
	while(n1)
	{
		file_node *n2 = n1->sibling;
		while(n2)
		{
			if( !strcmp_icase(n1->name, n2->name) )
			{
				Winprint("Error: UTF can't have both names %s and %s in the same directory!\n"
						 "Not exporting!",
					n1->name, n2->name);
				return false;
			}

			n2 = n2->sibling;
		}

		n1 = n1->sibling;
	}

	n1 = node;
	while(n1)
	{
		if( false == CheckUTFUnique(n1->child) )
		{
			return false;
		}

		n1 = n1->sibling;
	}
	
	return true;
}

/* write out a UTF file version 1.01 */
void WriteUTF(file_node *root, const char *file_name)
{
  if (CheckUTFChar(root->child) &&
	  CheckUTFUnique(root->child))
  {
	  FILE *file_handle;
	  struct UTF_HEADER header;

	  int name_list_len = 0;
	  char *name_list = NULL;
	  BuildNameList(root, &name_list, &name_list_len);
	  
	  InitHeader(&header, root, name_list_len);

	  InitDirEntries(root, NULL, RESET);

	  file_handle = fopen(file_name, "wb");
	  if(!file_handle)
	  {
		Winprint("Error opening %s\n", file_name);
		Free(name_list);
		return;
	  }

	  WriteHeader(&header, file_handle);
	  WriteDirTable(root, file_handle);
	  WriteNameTable(&header, file_handle, name_list);

	  Free(name_list);

	  #ifdef SGI
	  SwapTreeData(root);
	  #endif
	  WriteData(root, file_handle);
	  #ifdef SGI
	  SwapTreeData(root);
	  #endif

	  PadData(&header, file_handle);
	  fflush(file_handle);

	  if(EOF==fclose(file_handle))
	  {
		Winprint("Error closing %s\n", file_name);
		return;
	  }
  }
}

/* write data part of UTF file */
void WriteData(const file_node *node, FILE *file_handle)
{
  if(!node) return;

  // sanity check
  if(node->name == NULL || strlen(node->name) < 1)
  {
    Winprint("Error: node w/o a name!\n");
    exit(1);
  }

  if(verbose_level>=4)
  {
    printf("writting %s size=%d\n",node->name, node->data_size);
  }

  if((node->type == F) && (node->child != NULL)) // sanity check
  {
	  Winprint("Error: File entry %s w/ a child!\n", node->name);
	  exit(1);
  }

  unsigned char pad[8]={0,0,0,0,0,0,0,0};

  if((node->data != NULL) && (node->data_size > 0))
  {
	fwrite(node->data, sizeof(unsigned char), node->data_size, 
		     file_handle);
	if(node->data_size < 8)
	{
		fwrite(pad, sizeof(unsigned char), 8 - node->data_size, 
		     file_handle);
	}else
	if(node->data_size % 4)
	{
		fwrite(pad, sizeof(unsigned char), 4 - node->data_size%4, 
		     file_handle);
	}
  }
  else // sanity check
  if( ((node->data==NULL) && (node->data_size!=0)) || 
	  ((node->data!=NULL) && (node->data_size==0)) )
  {
	  Winprint("Error: node %s is corrupt.\n", node->name);
	  exit(1);
      // some lod groups can have temporarily 0 faces
	  //fwrite(pad, sizeof(unsigned char), 8, file_handle);
  }
  else
  if((node->data == NULL) && (node->data_size == 0) && (node->type == F))
  {
      fprintf(stderr, "Notice: node %s had 0 data to write out!\n", node->name);
	  fwrite(pad, sizeof(unsigned char), 8, file_handle); // so file can be deleted
  }

  if(node->sibling)
    WriteData(node->sibling, file_handle);
  if(node->child)
    WriteData(node->child, file_handle);
}

/* pad end of file w/ 0's to a 4 byte boundry */
void PadData(struct UTF_HEADER *header, FILE *file_handle)
{
  if(header->dwUnusedSpaceSize > 0)
  {
    unsigned char *tmp=(unsigned char*)Malloc(header->dwUnusedSpaceSize*sizeof(unsigned char));
	memset(tmp, 0, header->dwUnusedSpaceSize);
	/*
    for(int i=0;i<header->dwUnusedSpaceSize;i++)
	{
      tmp[i]=0;
	}
	*/
    fwrite(tmp, sizeof(unsigned char), header->dwUnusedSpaceSize, file_handle);
    Free(tmp);
  }
}

void BuildNameList(file_node *node, char **name_list, int *len)
{
	if(*name_list == NULL)
	{
		assert(*len == 0);
		*len = 1;
		*name_list = (char*)Malloc(sizeof(char));
		(*name_list)[0] = 0;
	}

	int position = 1;
	const char *pt = NULL;
	while( position < *len )
	{
		if( !strcmp(*name_list + position, node->name) )
		{
			pt = *name_list + position;
			break;
		}
		else
		{
			position += strlen(*name_list + position) + 1;
		}
	}

 	if(pt)
	{
		node->d_entry.dwName = pt - *name_list;
	}
	else
	{
		const int old_len = *len;
		*len += strlen(node->name) + 1;
		*name_list = (char*)Realloc(*name_list, *len * sizeof(char));
		memcpy(*name_list + old_len, node->name, strlen(node->name));
		(*name_list)[*len - 1] = 0;

		node->d_entry.dwName = old_len;
	}

  if(node->sibling)
    BuildNameList(node->sibling, name_list, len);
  if(node->child)
    BuildNameList(node->child, name_list, len);
}


/* write NameTable part of UTF file */
//void WriteNameTable(struct UTF_HEADER *header, file_node *node, FILE *file_handle)
void WriteNameTable(struct UTF_HEADER *header, FILE *file_handle, const char *name_list)
{
  /*
  char *name_table;
  unsigned long i;

  name_table=(char*)Malloc(header->dwNameSpaceSize);
  name_table[0]=0;

  BuildNameTable(name_table, node, RESET);

  // pad w/ 0's
  for(i = header->dwNameSpaceUsed; i < header->dwNameSpaceSize; i++)
  {
    name_table[i] = 0;
  }

  fwrite(name_table, sizeof(char), header->dwNameSpaceSize, file_handle);
  Free(name_table);
  */

  fwrite(name_list, sizeof(char), header->dwNameSpaceUsed, file_handle);

  if(header->dwNameSpaceSize > header->dwNameSpaceUsed)
  {
	char *tmp_b = (char*)Malloc(header->dwNameSpaceSize - header->dwNameSpaceUsed);
	memset(tmp_b, 0, header->dwNameSpaceSize - header->dwNameSpaceUsed);
	fwrite(tmp_b, sizeof(char), header->dwNameSpaceSize - header->dwNameSpaceUsed, file_handle);
	Free(tmp_b);
  }
}

/* put all file names in a block of memory */
/*
void BuildNameTable(char *name_table, file_node *node, int mode)
{
  static int index;

  if(mode==RESET)
  {
    index=1;
  }

  memcpy(name_table + index, node->name, strlen(node->name));
  name_table[index+strlen(node->name)] =
  name_table[index+strlen(node->name)+1] = 0;
  index += strlen(node->name) + 1;

  if(node->sibling)
    BuildNameTable(name_table, node->sibling, CONTINUE);
  if(node->child)
    BuildNameTable(name_table, node->child, CONTINUE);
}
*/

/* write out UTF_DIR_ENTRY table of UTF file */
void WriteDirTable(file_node *node, FILE *file_handle)
{
  struct UTF_DIR_ENTRY intel_dir;

#ifdef SGI
  SwapByteOrder((unsigned char*)&(node->d_entry), (unsigned char*)&intel_dir,
                sizeof(struct UTF_DIR_ENTRY), node->name); 
#else
  memcpy((unsigned char*)&intel_dir, (unsigned char*)&(node->d_entry),
         sizeof(struct UTF_DIR_ENTRY));
#endif
  fwrite(&intel_dir, sizeof(char), sizeof(struct UTF_DIR_ENTRY), file_handle);

  if(node->sibling)
    WriteDirTable(node->sibling, file_handle);
  if(node->child)
    WriteDirTable(node->child, file_handle);
}

/* write out UTF main header */
void WriteHeader(struct UTF_HEADER *header, FILE *file_handle)
{
  struct UTF_HEADER intel_hdr;

  /* swap header byte order (except "UTF ") */
  intel_hdr.dwIdentifier=header->dwIdentifier;
#ifdef SGI
  SwapByteOrder(((unsigned char*)header)+4, ((unsigned char*)&intel_hdr)+4,
                sizeof(struct UTF_HEADER)-4, "WriteHeader");
#else
  memcpy(((unsigned char*)&intel_hdr)+4, ((unsigned char*)header)+4,
         sizeof(struct UTF_HEADER)-4);
#endif

  fwrite(&intel_hdr, sizeof(char), sizeof(struct UTF_HEADER), file_handle);
}

/* convert UNIX time to DOS time */
DWORD GetDOSTime(void)
{
  DWORD day, month, year, second, minute, hour;
  time_t today;
  char *str, *month_str;

  // str=(char*)Malloc(26*sizeof(char));
  time(&today);
  today+=8*3600; /* adjust for CST (Winter) */
  str=ctime(&today);

  strtok(str, " "); // skip day of week 
  month_str=strtok(NULL, " ");
  if(!strcmp(month_str, "Jan"))
    month=1;
  else 
  if(!strcmp(month_str, "Feb"))
    month=2;
  else 
  if(!strcmp(month_str, "Mar"))
    month=3;
  else 
  if(!strcmp(month_str, "Apr"))
    month=4;
  else 
  if(!strcmp(month_str, "May"))
    month=5;
  else 
  if(!strcmp(month_str, "Jun"))
    month=6;
  else 
  if(!strcmp(month_str, "Jul"))
    month=7;
  else 
  if(!strcmp(month_str, "Aug"))
    month=8;
  else 
  if(!strcmp(month_str, "Sep"))
    month=9;
  else 
  if(!strcmp(month_str, "Oct"))
    month=10;
  else 
  if(!strcmp(month_str, "Nov"))
    month=11;
  else 
  if(!strcmp(month_str, "Dec"))
    month=12;
  else
  {
	month = 0;
	Winprint("Error: bad month %s!\n", month_str);
  }
   
  day=atoi(strtok(NULL, " "));
  hour=atoi(strtok(NULL, ":"));
  minute=atoi(strtok(NULL, ":"));
  second=atoi(strtok(NULL, " "));
  year=atoi(strtok(NULL, " "));
  year-=1980;

  month<<=5;
  year<<=9;
  // second>>=1;
  // second<<=16;
  second=0; // round to nearest minute
  minute<<=21;
  hour<<=27; 

  // Free(str);
  return(day | month | year | second | minute | hour);
}

/* convert UNIX time to WindowsFileTime */
void GetFileTime(DWORD *low, DWORD *high)
{
  /* unsigned long long base=0x01BBF776BB190000; // 1/1/1997 0:0:0 */
  U64 base=0x019db1ded53e8000; // 1/1/1970 0:0:0 in 100ns
  U64 today;

  today=base+10*(time(0)*1000000);

  *low=(DWORD)((today<<32)>>32);
  *low-=(*low%(10*60*1000000)); // strip seconds
  *high=(DWORD)(today>>32); 
}

void GetStringTime(char *buffer)
{
  DWORD day, month = 0, year, minute, hour;
  time_t today;
  char *str, *month_str;

  // str=(char*)Malloc(26*sizeof(char));
  time(&today);
#ifdef SGI
  today+=7*3600; /* adjust for CST (Winter 8x; Summer 7x) */
#endif
  str=ctime(&today);

  strtok(str, " "); // skip day of week 
  month_str=strtok(NULL, " ");
  if(!strcmp(month_str, "Jan"))
    month=1;
  else 
  if(!strcmp(month_str, "Feb"))
    month=2;
  else 
  if(!strcmp(month_str, "Mar"))
    month=3;
  else 
  if(!strcmp(month_str, "Apr"))
    month=4;
  else 
  if(!strcmp(month_str, "May"))
    month=5;
  else 
  if(!strcmp(month_str, "Jun"))
    month=6;
  else 
  if(!strcmp(month_str, "Jul"))
    month=7;
  else 
  if(!strcmp(month_str, "Aug"))
    month=8;
  else 
  if(!strcmp(month_str, "Sep"))
    month=9;
  else 
  if(!strcmp(month_str, "Oct"))
    month=10;
  else 
  if(!strcmp(month_str, "Nov"))
    month=11;
  else 
  if(!strcmp(month_str, "Dec"))
    month=12;
   
  day=atoi(strtok(NULL, " "));
  hour=atoi(strtok(NULL, ":"));
  minute=atoi(strtok(NULL, ":"));
  strtok(NULL, " "); //second=atoi(strtok(NULL, " "));
  year=atoi(strtok(NULL, " "));
  year-=1900;

  sprintf(buffer,"%.2d%.2d%.2d%.2d%.2d", year, month, day, hour, minute);
}

/* read a UTF file into a link memory data structure of file_node */
file_node* ReadUTF(char *name)
{
  file_node *root=NULL;
  char *names;
  FILE *file_handle;
  struct UTF_HEADER header;

  file_handle=fopen(name,"rb");
  if(!file_handle){
    if(verbose_level>2){
      fprintf(stderr,"Warning: could not open %s\n",name);
    }
    return NULL;
  }

  ReadHeader(&header, file_handle);
  names=ReadNames(&header, file_handle);
  root=ReadTree(&header, NULL, names, file_handle);
  Free(names);
  fclose(file_handle);
  #ifdef SGI
  SwapTreeData(root);
  #endif

  return root;
}

/* read a UTF header */
void ReadHeader(struct UTF_HEADER *header, FILE *file_handle)
{
  struct UTF_HEADER intel;

  fseek(file_handle, 0, 0);
  fread(&intel, sizeof(unsigned char), sizeof(struct UTF_HEADER), file_handle);

  header->dwIdentifier=intel.dwIdentifier;
  if(memcmp((unsigned char*)&header->dwIdentifier, "UTF ", 4)){
    Winprint("Not a UTF file\n");
    exit(1);
  }
#ifdef SGI
  SwapByteOrder(((unsigned char*)&intel)+4, ((unsigned char*)header)+4,
                  sizeof(struct UTF_HEADER)-4, "ReadHeader");
#else
  memcpy(((unsigned char*)header)+4, ((unsigned char*)&intel)+4,
         sizeof(struct UTF_HEADER)-4);
#endif
  if(header->dwVersion>UTF_VERSION){
    fprintf(stderr,"Warning: version %.2f is too new.\n",
            (float)header->dwVersion/100.0);
  }
}

/* reat UTF file tree */
file_node* ReadTree(struct UTF_HEADER *header, struct UTF_DIR_ENTRY *entry,
                    char *names, FILE *file_handle)
{
  file_node *node=NULL;
  struct UTF_DIR_ENTRY next_entry;
  struct UTF_DIR_ENTRY intel_entry;

  node=(file_node*)Malloc(sizeof(file_node));

  // process root node
  if(entry==NULL){
    node->type=D;
    node->mem=M;
    node->name=(char*)Malloc(2*sizeof(char));
    strcpy(node->name, "\\");
    // node->data="";
    node->data=NULL;
    node->data_size=0;
    node->sibling=NULL;

    fseek(file_handle, (long)header->dwDirectoryOffset, 0);
    fread(&intel_entry, sizeof(unsigned char), sizeof(struct UTF_DIR_ENTRY),
          file_handle); 
#ifdef SGI
    SwapByteOrder((unsigned char*)&intel_entry, (unsigned char*)&node->d_entry,
                  sizeof(struct UTF_DIR_ENTRY), "Root\\");
#else
    memcpy((unsigned char*)&node->d_entry, (unsigned char*)&intel_entry, 
               sizeof(struct UTF_DIR_ENTRY));
#endif

    // read dir entry & process child
    if((node->d_entry.dwAttributes & 0x00000010) &&
       (node->d_entry.dwDataOffset)){
      fseek(file_handle, (long)(header->dwDirectoryOffset+
                                node->d_entry.dwDataOffset), 0);
      fread(&intel_entry, sizeof(unsigned char), sizeof(struct UTF_DIR_ENTRY),
            file_handle);
#ifdef SGI
      SwapByteOrder((unsigned char*)&intel_entry, (unsigned char*)&next_entry,
                    sizeof(struct UTF_DIR_ENTRY), "Root dir entry");
#else
      memcpy((unsigned char*)&next_entry, (unsigned char*)&intel_entry, 
             sizeof(struct UTF_DIR_ENTRY));
#endif
      node->child=ReadTree(header, &next_entry, names, file_handle);
    }
    else{
      node->child=NULL;
      fprintf(stderr,"Warning: no tree in UTF\n");
    }
  }

  // process regular (non root) node
  else{
    node->name=(char*)Malloc((strlen(names+entry->dwName)+1)*sizeof(char));
    strcpy(node->name, names+entry->dwName);
    node->d_entry=*entry;

    if(entry->dwAttributes & 0x00000010){
      node->type=D;
      node->mem=M;
      // node->data="";
      node->data=NULL;
      node->data_size=0;
    }
    else{
      node->type=F;
      node->mem=M;

	  if( entry->dwSpaceUsed > 0 ){
        node->data=(unsigned char*)
                   Malloc((entry->dwSpaceUsed+1)*sizeof(unsigned char));
        fseek(file_handle, (long)(header->dwDataStartOffset+entry->dwDataOffset), 0);
        fread(node->data, sizeof(unsigned char), entry->dwSpaceUsed, file_handle);
        node->data[entry->dwSpaceUsed]=0;
        node->data_size=(int)entry->dwSpaceUsed;
	  }
	  else{
		node->data=NULL;
        node->data_size=0;
	  }
	}

    // read dir entry and process sibling
    if(entry->dwNext){
      fseek(file_handle, (long)(header->dwDirectoryOffset+entry->dwNext), 0);
      fread(&intel_entry, sizeof(unsigned char), sizeof(struct UTF_DIR_ENTRY),
            file_handle);
#ifdef SGI
      SwapByteOrder((unsigned char*)&intel_entry, (unsigned char*)&next_entry,
                    sizeof(struct UTF_DIR_ENTRY), node->name);
#else
      memcpy((unsigned char*)&next_entry, (unsigned char*)&intel_entry, 
             sizeof(struct UTF_DIR_ENTRY));
#endif
      node->sibling=ReadTree(header, &next_entry, names, file_handle);
    }
    else
      node->sibling=NULL;

    // read dir entry and process child
    if((entry->dwAttributes & 0x00000010) && (entry->dwDataOffset)){
      fseek(file_handle, (long)(header->dwDirectoryOffset+
                                entry->dwDataOffset), 0);
      fread(&intel_entry, sizeof(unsigned char), sizeof(struct UTF_DIR_ENTRY),
            file_handle);
#ifdef SGI
      SwapByteOrder((unsigned char*)&intel_entry, (unsigned char*)&next_entry,
                    sizeof(struct UTF_DIR_ENTRY), node->name);
#else
      memcpy((unsigned char*)&next_entry, (unsigned char*)&intel_entry,
             sizeof(struct UTF_DIR_ENTRY));
#endif
      node->child=ReadTree(header, &next_entry, names, file_handle);
    }
    else
      node->child=NULL;
  }

  return node;
}

/* read UTF file name block */
char* ReadNames(struct UTF_HEADER *header, FILE *file_handle)
{
  char *names;

  names=(char*)Malloc(header->dwNameSpaceUsed*sizeof(char));
  fseek(file_handle, (long)header->dwNamesOffset, 0);
  fread(names, sizeof(unsigned char), header->dwNameSpaceUsed, file_handle);

  return names;
}

/* print a UTF file from memory */
void PrintTree(file_node *node)
{
  static int indent=0;

  Indent(indent);
  printf("%s [%d]\n",node->name,node->data_size);

  if(node->child){
    indent++;
    PrintTree(node->child);
    indent--;
  }
  if(node->sibling){
    PrintTree(node->sibling);
  }
}

void Indent(int indent)
{
  if(indent==0){
    printf("\n");
  }
  else{
    while(indent--){
      printf("|---");
    }
  }
}

file_node* CreateExporterVersion(void)
{
	file_node *node = CreateNode("Exporter Version", F);
	node->mem = M;
	node->data_size = strlen(__DATE__) + strlen(__TIME__) + 2;
	node->data = (unsigned char*)Malloc(node->data_size);
	sprintf((char*)(node->data), "%s %s", __DATE__, __TIME__);

	return node;
}

/* free all mallocked mem */
void FreeTree(file_node *node)
{
  if(node)
  {
    FreeTree(node->child);
    FreeTree(node->sibling);

    if(verbose_level>=4)
	{
      printf("freeing %s\n",node->name);
    }
    Free(node->name);
    if(node->mem==M)
	{
      Free(node->data);
    }
    Free(node);
  }
}

void FreeTreeData(file_node *node)
{

  FreeTree(node);
#if 0
  if(!node)
    return;

  if(node->child){
    FreeTree(node->child);
  }
  if(node->sibling){
    FreeTree(node->sibling);
  }

  if(verbose_level>=4){
    printf("freeing %s\n",node->name);
  }
  Free(node->name);
  if(node->mem==M){
    Free(node->data);
  }
  Free(node);
#endif
}

file_node* CreateNode(const char *name, int type)
{
  file_node *node;

  node=(file_node*)Malloc(sizeof(file_node));
  node->name=(char*)Malloc((strlen(name)+1)*sizeof(char));
 
  strcpy(node->name, name);
  node->type=type;
  node->mem=A;
  node->data_size=0;
  node->data=NULL;
  node->sibling=NULL;
  node->child=NULL;

  return node;
}

void ReplaceNode(file_node *root, file_node *old_node, file_node *new_node)
{
	file_node *parent = GetPreviousNode(root, old_node->name);

	//new_node->child = old_node->child;
	new_node->sibling = old_node->sibling;

	if(parent->child == old_node)
	{
		parent->child = new_node;
	}
	else
	if(parent->sibling == old_node)
	{
		parent->sibling = new_node;
	}
	else
	{
		Winprint("Error: can't replace node %s w/ node %s.\n",old_node->name, new_node->name);
		exit(1);
	}

	old_node->sibling = NULL;
	//Free(old_node->name);
	//Free(old_node);
}

file_node* GetPreviousNode(file_node *node, const char *target)
{
  file_node *tmp=NULL;
  static int counter=0;

  if((!node) || 
     ((node->sibling) && (!strcmp(node->sibling->name, target))) ||
     ((node->child) && (!strcmp(node->child->name, target)))){
    return node;
  }
  else{
    counter++;
    tmp=GetPreviousNode(node->child, target);
    counter--;
    if(!tmp && counter)
      tmp=GetPreviousNode(node->sibling, target);
  }
    
  return tmp;
}

file_node* GetDeepNode(file_node *node, const char *target)
{
  file_node *tmp;
  static int counter=0;

  if(node==NULL) return NULL;

  if(!strcmp(node->name, target)){
    return node;
  }
  else{
    counter++;
    tmp=GetDeepNode(node->child, target);
    counter--;
    if(!tmp && counter)
      tmp=GetDeepNode(node->sibling, target);
  }
     
  return tmp;
}

file_node* GetDeepNodeG(file_node *node, const char *target)
{
  file_node *tmp=NULL;

  if(node){ 
    tmp=GetDeepNode(node, target);
    if(!tmp){
      fprintf(stderr, "Error: node %s not found from %s",target, node->name);
      exit(1);
    }
  }
  return tmp;
}


int strcmp_icase(const char * const str1, const char * const str2)
{
	const int delta = 'A' - 'a';
	const int length = _MIN( strlen(str1), strlen(str2) );

	for(int i = 0; i < length; i++)
	{
		const int v1 = (str1[i] >= 'A' && str1[i] <= 'Z') ? str1[i] - delta : str1[i];
		const int v2 = (str2[i] >= 'A' && str2[i] <= 'Z') ? str2[i] - delta : str2[i];

		if(v1 > v2)
		{
			return 1;
		}
		else
		if(v1 < v2)
		{
			return -1;
		}
	}

	if( strlen(str1) > strlen(str2) )
	{
		return 1;
	}
	else
	if(strlen(str1) < strlen(str2) )
	{
		return -1;
	}

	return 0;
}


file_node* GetWideNode(file_node *node, const char *target, bool ignore_case/*=false*/)
{
  file_node *tmp = NULL;
  static int counter = 0;

  /*
  if(!node)
  {
	  return node;
  }
  
  if(!ignore_case && !strcmp(node->name, target))
  {
	  return node;
  }

  if(ignore_case && )
  {
	  return node;
  }
  */

  if(!node || !strcmp(node->name, target) || (ignore_case && !strcmp_icase(node->name, target)))
  {
    return node;
  }
  else
  {
    if(counter)
	{
      tmp=GetWideNode(node->sibling, target, ignore_case);
    }
    if(!tmp || !counter){
      counter++;
      tmp=GetWideNode(node->child, target, ignore_case);
      counter--;
    }
  }
     
  return tmp;
}

file_node* GetWideNodeG(file_node *node, const char *target)
{
  file_node *tmp=NULL;

  if(node){
    tmp=GetWideNode(node, target);
    if(!tmp){
      fprintf(stderr, "Error: node %s not found from %s",target, node->name);
      exit(1);
    }
  }
  return tmp;
}

int ChildCount(file_node *node)
{
  int count=0;

  if(!node)
    return -1;

  if(node->child){
    count++;
    node=node->child;
  } 
  while(node->sibling){
    count++;
    node=node->sibling;
  } 
  return count;
}

int ChildFCount(file_node *node)
{
  int count=0;

  if(!node)
    return -1;
 
  if(node->child){
    if(node->type == F){
      count++;
    }
    node=node->child;
  } 
  while(node->sibling){
    if(node->type == F){
      count++;
    }
    node=node->sibling;
  } 
  return count;
}

int SiblingCount(file_node *node)
{
  int count=0;
 
  if(!node)
    return -1;

  while(node->sibling){
    count++;
    node=node->sibling;
  } 
  return count;
}

int SiblingFCount(file_node *node)
{
  int count=0;
 
  if(!node)
    return -1;

  while(node->sibling){
    if(node->type == F){
      count++;
    }
    node=node->sibling;
  } 
  return count;
}



file_node* GetLastSib(file_node *node)
{
  if(node == NULL) return NULL;
  

  while(node->sibling)
  {
    node = node->sibling;
  }

  return node;
}

file_node* GetLastChild(file_node *node)
{
  if(node==NULL){
    return NULL;
  }

  while(node->child){
    node=node->child;
  }
  return node;
}


#ifdef SGI // used for byteorder swapping
#include "names.h"
#include "persistcompound.h"
#include "persisthardpoint.h"

/* reorder 3db data to little-endian */
void SwapTreeData(file_node *node)
{
  int i;

  if(!node)
    return;

  if(verbose_level>=4)
  {
    printf("changing byte order of %s\n",node->name);
  }
  if(node->data_size>0)
  {
    /* don't byte reorder character data to little-endian */
    if((!strcmp(node->name, Image_linear_bitmap)) || 
       (!strcmp(node->name, Image_RGB_palette))||
       (!strcmp(node->name, ROOT_OBJ_NAME))||
       (!strcmp(node->name, OBJECT_NAME))||
       (!strcmp(node->name, FILE_NAME))||
       // (strstr(node->name, "Part_"))||
       (strstr(node->name, "Sc_"))||
       (!strcmp(node->name, Mesh_name))|| // deformable mesh
       (!strcmp(node->name, Alpha_8_bit))||
       (!strcmp(node->name, Image_indices))||
       (!strcmp(node->name, Palette_RGB_888))||
       (!strcmp(node->name, Texture_name))||
       (!strcmp(node->name, "Parent name"))||
       (!strcmp(node->name, "Child name"))||
       (!strcmp(node->name, "Channel name"))||
       (!strcmp(node->name, "Skeleton"))||
       (!strcmp(node->name, "Name")) ||
       (!strcmp(node->name, "Exporter Version"))
      )
	{
      ;
    }else
    if(!strcmp(node->name,JT_FIXED_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(Fix))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(Fix)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name,JT_PRISMATIC_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(Pris))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(Pris)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name,JT_REVOLUTE_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(Rev))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(Rev)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name,JT_SPHERICAL_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(PersistSphere))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(PersistSphere)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name,JT_LOOSE_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(Loose))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(Loose)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name,JT_TRANSLATIONAL_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(Trans))
	  {
         SwapByteOrder(node->data+i+2*PARTNAME_MAX, 
                       node->data+i+2*PARTNAME_MAX, 
                       sizeof(Trans)-2*PARTNAME_MAX, node->name);
      } 
    }else
    if(!strcmp(node->name, HP_FIXED_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(PersistHPFixed))
	  {
         SwapByteOrder(node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       sizeof(PersistHPFixed)-PERSIST_MAX_HARDPOINT_NAME,
                       node->name);
      } 
    }else
    if(!strcmp(node->name, HP_PRISMATIC_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(PersistHPPrismatic))
	  {
         SwapByteOrder(node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       sizeof(PersistHPPrismatic)-PERSIST_MAX_HARDPOINT_NAME,
                       node->name);
      } 
    }else
    if(!strcmp(node->name, HP_REVOLUTE_NAME))
	{
      for(i=0; i<node->data_size; i+=sizeof(PersistHPRevolute))
	  {
         SwapByteOrder(node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       node->data+i+PERSIST_MAX_HARDPOINT_NAME, 
                       sizeof(PersistHPRevolute)-PERSIST_MAX_HARDPOINT_NAME,
                       node->name);
      } 
    }else
    if(!strcmp(node->name, Image_colors))
	{
      SwapByteOrder16(node->data, node->data, node->data_size, node->name);
    } 
    else
	{
      SwapByteOrder(node->data, node->data, node->data_size, node->name);
    }
  }

  if(node->sibling)
    SwapTreeData(node->sibling);

  if(node->child)
    SwapTreeData(node->child);
}

/* change byte order from SGI to Intel; in and out can be the same*/
int SwapByteOrder(const unsigned char* in, unsigned char* out, int bytes,
                  const char *name)
{
  // unsigned long *word_list;
  int words;
  int i;
  unsigned char *tmp;

  if(bytes%4)
  {
    Winprint("%s data not 32-bit !\n",name);
    exit(1);
  }

  tmp=(unsigned char*)Malloc(bytes);
  memcpy(tmp,in,bytes);
  words=(int)(bytes/4);
  for(i=0; i<words; i++)
  {
    out[i*4]=tmp[i*4+3];
    out[i*4+1]=tmp[i*4+2];
    out[i*4+2]=tmp[i*4+1];
    out[i*4+3]=tmp[i*4];
  }
  Free(tmp);

  return 0;

#if 0 // this works 99% of the time ???
  words=(int)(bytes/4);
  word_list=(unsigned long *)out;
  for(i=0; i<words; i++)
  {
    word_list[i]=(in[3]<<24)+(in[2]<<16)+(in[1]<<8)+in[0];
    in+=4;
  }
#endif
}

/* change byte order from SGI to Intel 16 bit */
void SwapByteOrder16(const unsigned char* in, unsigned char* out, int bytes, const char *name)
{
  int words;
  int i;
  unsigned char *tmp;

  if(bytes%2)
  {
    Winprint("%s data not 16-bit\n", name);
    exit(1);
  }

  tmp=(unsigned char*)Malloc(bytes);
  memcpy(tmp,in,bytes);
  words=(int)(bytes/2);
  for(i=0; i<words; i++)
  {
    out[i*2]=tmp[i*2+1];
    out[i*2+1]=tmp[i*2];
  }
  Free(tmp);
}

#endif
