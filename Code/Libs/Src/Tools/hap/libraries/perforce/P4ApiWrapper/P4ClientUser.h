#ifndef __P4CLIENTUSER_H__
#define __P4CLIENTUSER_H__

#include "P4ApiWrapper.h"



class P4ClientUserWrap : public ClientUser 
{
public:			
	P4ClientUserWrap();

	virtual void	InputData( StrBuf *strbuf, Error *e );
	virtual void 	HandleError( Error *err );
	virtual void 	OutputError( const_char *errBuf );
	virtual void	OutputInfo( char level, const_char *data );
	virtual void 	OutputBinary( const_char *data, int length );
	virtual void 	OutputText( const_char *data, int length );

	virtual void	OutputStat( StrDict *varList );

	virtual void	Prompt( const StrPtr &msg, StrBuf &rsp, 
				int noEcho, Error *e );
	virtual void	ErrorPause( char *errBuf, Error *e );

	virtual void	Edit( FileSys *f1, Error *e );

	virtual void	Diff( FileSys *f1, FileSys *f2, int doPage, 
				char *diffFlags, Error *e );

	virtual void	Merge( FileSys *base, FileSys *leg1, FileSys *leg2, 
				FileSys *result, Error *e );

	virtual void	Help( const_char *const *help );

	virtual FileSys	*File( FileSysType type );

	virtual void	Finished() {}

public:
	SScFileStatus m_fs;

	HWND m_hwndNotify;



}; // P4ClientUserWrap



#endif //__P4CLIENTUSER_H__