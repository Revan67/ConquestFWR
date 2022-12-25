#ifndef TOKENDEF_H
#define TOKENDEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                TokenDef.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//


struct Symbol;

                                 /* token data structure */
typedef struct tokn {
  int    tokentype;  /* OPERATOR, DELIMITER, RESERVED, etc */
  int    datatype;   /* INTEGER, REAL, STRINGTYPE, or BOOLETYPE */
  struct Symbol * symtype;
  struct Symbol * symentry;
  struct tokn * operands;
  struct tokn * link;
  union { char  *tokenstring;   /* values of different types, overlapped */
          int   which;
          long  intnum;
          double realnum; } tokenval;
  } *TOKEN;

/* The following alternative kinds of values share storage in the token
   record.  Only one of the following can be present in a given token.  */
#define whichval  tokenval.which
#define intval    tokenval.intnum
#define realval   tokenval.realnum
#define stringval tokenval.tokenstring

#define OPERATOR       0         /* token types */
#define DELIMITER      1
#define RESERVED       2
#define IDENTIFIERTOK  3
#define STRINGTOK      4
#define NUMBERTOK      5

#define PLUSOP         1         /* operator numbers */
#define MINUSOP        2
#define TIMESOP        3
#define DIVIDEOP       4
#define ASSIGNOP       5
#define EQOP           6
#define NEOP           7
#define LTOP           8
#define LEOP           9
#define GEOP          10
#define GTOP          11
#define POINTEROP     12
#define DOTOP         13
#define ANDOP         14
#define OROP          15
#define NOTOP         16
#define DIVOP         17
#define MODOP         18
#define XOROP         19
#define LOGANDOP	  20
#define LOGOROP	      21
#define SCOPEOP		  22


#define INTEGER    0             /* number types */
#define REAL       1
#define STRINGTYPE 2
#define BOOLETYPE  3

#define IDENTIFIER 257          /* token types for use with YACC */
#define STRING 258
#define NUMBER 259

   /* subtract OPERATOR_BIAS from the following to get operator numbers */
#define PLUS 260
#define OPERATOR_BIAS  (PLUS - 1)    /* added to Operators */
#define MINUS 261
#define TIMES 262
#define DIVIDE 263
#define ASSIGN 264
#define EQ 265
#define NE 266
#define LT 267
#define LE 268
#define GE 269
#define GT 270
#define POINT 271
#define DOT 272
#define AND 273
#define OR 274
#define NOT 275
#define DIV 276
#define MOD 277
#define XOR 278
#define LOGAND 279
#define LOGOR 280
#define SCOPE 281


   /* subtract DELIMITER_BIAS from the following to get delimiter numbers */
#define COMMA 282
#define DELIMITER_BIAS (COMMA - 1)   /* added to Delimiters */
#define SEMICOLON 283
#define COLON 284
#define LPAREN 285
#define RPAREN 286
#define LBRACKET 287
#define RBRACKET 288
#define DOTDOT 289
#define LBRACE 290
#define RBRACE 291
#define LCONTINUE 292

   /* subtract RESERVED_BIAS from the following to get reserved word numbers */
#define _STRUCT 293
#define RESERVED_BIAS  (_STRUCT - 1)   /* added to reserved words */
#define _CLASS 294
#define _PUBLIC 295
#define _PRIVATE 296
#define _PROTECTED 297
#define _TYPEDEF 298
#define _SIGNED 299
#define _UNSIGNED 300
#define _ENUM 301
#define _CONST 302
#define _UNION 303
#define _NAMESPACE 304
#define _READONLY 305
#define _HEXVIEW 306
#define _SPELLCHECK 307
#define _FILENAMECHECK 308
#define _COLORCHECK 309



#ifdef __cplusplus
extern "C" {
#endif

TOKEN talloc();           /* allocate a new token record */

#ifdef __cplusplus
}
#endif

#endif

//--------------------------------------------------------------------------//
//----------------------------End TokenDef.h--------------------------------//
//--------------------------------------------------------------------------//
