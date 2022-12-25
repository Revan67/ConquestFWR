#ifndef UTF_H
#define UTF_H
#include <stdio.h>
#include <assert.h>

#include "misc.h"

#define UTF_VERSION 0x101
#define DWORD unsigned long

enum {UNUSED=0, F, D, M, A};
enum {CONTINUE, RESET};

extern int verbose_level;

typedef struct _MY_FILETIME
  {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
  }MY_FILETIME;

struct UTF_HEADER /* from BaseUTF.h */
{
        DWORD dwIdentifier;         // 'UTF ' identifier
        DWORD dwVersion;            // File version number (0x100 = 1.00)

        DWORD dwDirectoryOffset;    // Offset to beginning of directory area
        DWORD dwDirectorySize;      // Size of directory area in bytes (used area)

        DWORD dwUnusedEntryOffset;  // offset to first unused entry, 0 == nothing unused
        DWORD dwDirEntrySize;       // size of a directory entry, in bytes

        DWORD dwNamesOffset;        // offset to buffer containing all of the ASCIIZ filenames
        DWORD dwNameSpaceSize;      // size of the names buffer
        DWORD dwNameSpaceUsed;      // number of bytes actually used in the names buffer

        DWORD dwDataStartOffset;    // offset to beginning of file data

        DWORD dwUnusedSpaceOffset;  // offset to first block of unused space
        DWORD dwUnusedSpaceSize;    // Cumulative size of unused resources

        MY_FILETIME   LastWriteTime;  // last time directory structure was changed
        //
        // Reserved expansion area for future header versions
        //
};

#define byte unsigned char
struct UTF_SHARING
{
        byte    read;                // count of files open with read access
        byte    write;               // count of files open with write access 
        byte    readSharing;         // count of files open with read sharing on
        byte    writeSharing;   
};

//
// UTF directory entry representation
//

struct UTF_DIR_ENTRY
{
    DWORD           dwNext;             // offset to next entry within directory
    DWORD           dwName;             // offset into name buffer of an ASCIIZ string

    DWORD           dwAttributes;       // directory / file / read-only, etc
    struct UTF_SHARING Sharing;         // dynamic sharing state 
    DWORD           dwDataOffset;       // file data (for file), or directory offset to child directory entry
    DWORD           dwSpaceAllocated;   // disk space allocated for file (may be larger than used)
    DWORD           dwSpaceUsed;        // disk space actually used by the file
    DWORD           dwUncompressedSize; // uncompressed file size

    DWORD           DOSCreationTime;
    DWORD           DOSLastAccessTime;
    DWORD           DOSLastWriteTime;
};

/* node for in memory file structure */
typedef struct _file_node
{
        int type;  // F file, D directory
        int mem; // M malloc, A auto
        char *name;
        unsigned char *data;
        int data_size;
        struct _file_node *sibling;
        struct _file_node *child;
        struct UTF_DIR_ENTRY d_entry;
}file_node;

/* write */
void BuildNameList(file_node *node, char **name_list, int *len);
void InitHeader(struct UTF_HEADER *header, file_node *root);
void InitDirEntries(file_node *node, file_node *previous, int mode);
void WriteUTF(file_node *root, const char *file_name);
bool CheckUTFChar(const file_node *node);
bool CheckUTFUnique(const file_node *node);
void WriteHeader(struct UTF_HEADER *header, FILE *file_handle);
void WriteDirTable(file_node *root, FILE *file_handle);
//void WriteNameTable(struct UTF_HEADER *header, file_node *root, 
                    //FILE *file_handle);
void WriteNameTable(struct UTF_HEADER *header, FILE *file_handle, const char *name_list);
//void BuildNameTable(char *name_table, file_node *node, int mode);
void WriteData(const file_node *root, FILE *file_handle);
void PadData(struct UTF_HEADER *header, FILE *file_handle);
int GetFileCount(file_node *node);
//int GetNameSize(file_node *node);
int GetDataSize(file_node *node);

#ifdef SGI
void SwapTreeData(file_node *node);
int SwapByteOrder(const unsigned char* in, unsigned char* out, int bytes,
                  const char *name);
void SwapByteOrder16(const unsigned char* in, unsigned char* out, int bytes,
                     const char *name);
#endif

DWORD GetDOSTime(void);
void GetFileTime(DWORD *low, DWORD *high);
void GetStringTime(char *buffer);

/* read */
file_node* ReadUTF(char *name);
void ReadHeader(struct UTF_HEADER *header, FILE *file_handle);
char* ReadNames(struct UTF_HEADER *header, FILE *file_handle);
file_node* ReadTree(struct UTF_HEADER *header,
                    struct UTF_DIR_ENTRY *next_entry, char *names,
                    FILE *file_handle);
void PrintTree(file_node *root);
void ProcessTree(file_node *node, file_node *parent);
void Indent(int indent);
void FreeTree(file_node *node);
void FreeTreeData(file_node *node);

file_node* CreateExporterVersion(void);
file_node* CreateNode(const char *name, int type);

file_node* GetLastSib(file_node *node);
file_node* GetLastChild(file_node *node);
file_node* GetDeepNode(file_node *node, const char *target);
file_node* GetDeepNodeG(file_node *node, const char *target);
file_node* GetWideNode(file_node *node, const char *target, bool ignore_case=false);
file_node* GetWideNodeG(file_node *node, const char *target);
file_node* GetPreviousNode(file_node *node, const char *target);
void ReplaceNode(file_node *root, file_node *old_node, file_node *new_node);

int ChildCount(file_node *node);
int SiblingCount(file_node *node);
int ChildFCount(file_node *node);
int SiblingFCount(file_node *node);

int strcmp_icase(const char * const str1, const char * const str2);

#endif // UTF_H
