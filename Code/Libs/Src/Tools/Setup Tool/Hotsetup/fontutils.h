#ifndef     FONTUTILS_H
#define     FONTUTILS_H


LPSTR    ReadLongName(PSTR pszFile);


// don't know if we need all this junque...will fix later --mel

// Menu defines.
#define IDM_ABOUT               500
#define IDM_EXIT                510
#define IDM_INSTALLRASTERVECTOR 520
#define IDM_INSTALLTRUETYPE     530

// String Table Defines
#define IDS_PROGNAME       1
#define IDS_MAINMENUNAME   2
#define IDS_MAINCLASSNAME  3

#define LIST_BORDER        5 
#define WINDOW_THRESHOLD   30

// Font information structure
typedef struct
{        
    char FontName[80];
    char FontType[20];
    char File[80];
    char FOTFile[80];
    BOOL Scalable;
    char Pitch[20];
} FONTINF;


// Global variables defined in this header.  One .C file (INIT.C) should have
// IN_INIT #define'd prior to including this header -- it's in that
// C module where these variables will reside.

#ifdef IN_INIT
#define EXTERN
#endif

#ifndef IN_INIT
#define EXTERN extern
#endif

EXTERN HANDLE ghInst;                     // Handle to this instance
EXTERN HWND   ghWnd;                      // Handle to main window
EXTERN HWND   ghWndList;                  // Handle to listbox  
EXTERN HFONT  ghCurrFont;                 // Handle of current font to be displayed 
EXTERN FONTINF ghFontInfo;                // Font information for current font;

// Type definitions and macros

#define tag_NamingTable         0x656d616e        /* 'name' */

// Executable file header information
typedef struct
{
    WORD    wFileSignature;         // 0x5A4D
    WORD    wLengthMod512;          // bytes on last page
    WORD    wLength;                // 512 byte pages
    WORD    wRelocationTableItems;  
    WORD    wHeaderSize;            // Paragraphs
    WORD    wMinAbove;              // Paragraphs
    WORD    wDesiredAbove;          // Paragraphs
    WORD    wStackDisplacement;     // Paragraphs
    WORD    wSP;                    // On entry
    WORD    wCheckSum;
    WORD    wIP;                    // On entry
    WORD    wCodeDisplacement;      // Paragraphs
    WORD    wFirstRelocationItem;   // Offset from beginning
    WORD    wOverlayNumber;
    WORD    wReserved[ 16 ];
    LONG    lNewExeOffset;          
} OLDEXE;

// FontInfo structure located at beginning of a .fnt file
typedef struct 
{
        WORD  fontOrdinal;
        WORD  dfVersion;
        DWORD dfSize;
        char  dfCopyright[60];
        WORD  dfType;
        WORD  dfPoints;
        WORD  dfVertRes;
        WORD  dfHorizRes;
        WORD  dfAscent;
        WORD  dfInternalLeading;
        WORD  dfExternalLeading;
        BYTE  dfItalic;
        BYTE  dfUnderline;
        BYTE  dfStrikeOut;
        WORD  dfWeight;
        BYTE  dfCharSet;
        WORD  dfPixWidth;
        WORD  dfPixHeight;
        BYTE  dfPitchAndFamily;
        WORD  dfAvgWidth;
        WORD  dfMaxWidth;
        BYTE  dfFirstChar;
        BYTE  dfLastChar;
        BYTE  dfDefaultChar;
        BYTE  dfBreakChar;
        WORD  dfWidthBytes;
        DWORD dfDevice;
        DWORD dfFace;
        DWORD dfReserved;
        char  dfCharTable[100];        
} FONTDIRENTRY;

// New executable file header
typedef struct
{
    WORD  wNewSignature;    // 0x454e
    char  cLinkerVer;       // Version number 
    char  cLinkerRev;       // Revision number 
    WORD  wEntryOffset;     // Offset to Entry Table
    WORD  wEntrySize;       // Number of bytes in Entry Table
    long  lChecksum;        // 32 bit check sum for the file
    WORD  wFlags;           // Flag word 
    WORD  wAutoDataSegment; // Seg number for automatic data seg
    WORD  wHeapInit;        // Initial heap allocation; 0 for no heap
    WORD  wStackInit;       // Initial stack allocation; 0 for libraries
    WORD  wIPInit;          // Initial IP setting 
    WORD  wCSInit;          // Initial CS segment number
    WORD  wSPInit;          // Initial SP setting 
    WORD  wSSInit;          // Initial SS segment number
    WORD  wSegEntries;      // Count of segment table entries
    WORD  wModEntries;      // Entries in Module Reference Table 
    WORD  wNonResSize;      // Size of non-resident name table (bytes)
    WORD  wSegOffset;       // Offset of Segment Table 
    WORD  wResourceOffset;  // Offset of Resource Table 
    WORD  wResOffset;       // Offset of resident name table 
    WORD  wModOffset;       // Offset of Module Reference Table 
    WORD  wImportOffset;    // Offset of Imported Names Table 
    long  lNonResOffset;    // Offset of Non-resident Names Table
                            // THIS FIELD IS FROM THE BEGINNING OF THE FILE
                            // NOT THE BEGINNING OF THE NEW EXE HEADER
    WORD  wMoveableEntry;   // Count of movable entries in entry table
    WORD  wAlign;           // Segment alignment shift count
    WORD  wResourceSegs;    // Count of resource segments
    BYTE  bExeType;         // Operating System flags  
    BYTE  bAdditionalFlags; // Additional exe flags 
    WORD  wFastOffset;      // offset to FastLoad area 
    WORD  wFastSize;        // length of FastLoad area 
    WORD  wReserved;
    WORD  wExpVersion;      // Expected Windows version number 
} NEWEXE, *PNEWEXE;

// Macros for TrueType portability
#define FS_2BYTE(p)  ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
#define FS_4BYTE(p)  ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )
#define SWAPW(a)        ((short) FS_2BYTE( (unsigned char FAR*)(&a) ))
#define SWAPL(a)        ((long) FS_4BYTE( (unsigned char FAR*)(&a) ))


typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;
typedef long sfnt_TableTag;

typedef struct {
    uint16 platformID;
    uint16 specificID;
    uint16 languageID;
    uint16 nameID;
    uint16 length;
    uint16 offset;
} sfnt_NameRecord;

typedef struct {
    uint16 format;
    uint16 count;
    uint16 stringOffset;
} sfnt_NamingTable;

typedef struct {
    sfnt_TableTag   tag;
    uint32          checkSum;
    uint32          offset;
    uint32          length;
} sfnt_DirectoryEntry;



typedef struct {
    int32 version;                  /* 0x10000 (1.0) */
    uint16 numOffsets;              /* number of tables */
    uint16 searchRange;             /* (max2 <= numOffsets)*16 */
    uint16 entrySelector;           /* log2 (max2 <= numOffsets) */
    uint16 rangeShift;              /* numOffsets*16-searchRange*/
    sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
} sfnt_OffsetTable;
#define OFFSETTABLESIZE     12  /* not including any entries */


#endif      // FONTUTILS_H