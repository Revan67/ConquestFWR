#ifndef BASEPARSER_H
#define BASEPARSER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              BaseParser.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.             //
//                                                                          //
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE IBaseParser : IDAComponent
{
	virtual GENRESULT CorrelateSymbol (const C8 *pSymbolName, void *pOldData, void *pNewData) = 0;

	virtual BOOL32 LoadParseData (IFileSystem * inFile) = 0;

	virtual BOOL32 CreateParser (void) = 0;

	virtual BOOL32 ParseDefFile (void) = 0;
};
#endif