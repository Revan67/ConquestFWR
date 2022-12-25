#include "stdafx.h"
#include "P4ClientUser.h"

P4ClientUserWrap::P4ClientUserWrap()
{
	m_hwndNotify = 0;
}


void P4ClientUserWrap::InputData( StrBuf *strbuf, Error *e )
{
	ClientUser::InputData(strbuf, e);
}

void P4ClientUserWrap::HandleError( Error *err )
{
	ClientUser::HandleError(err);
}
void P4ClientUserWrap::OutputError( const_char *errBuf )
{
	if (m_hwndNotify)
	{
		::SendMessage(m_hwndNotify, NM_SENDERROR, (WPARAM)errBuf, 0);
	}
	ClientUser::OutputError(errBuf);
}
void P4ClientUserWrap::OutputInfo( char level, const_char *data )
{
	if (m_hwndNotify)
	{
		::SendMessage(m_hwndNotify, NM_SENDINFO, (WPARAM)data, (LPARAM)level);
	}

	ClientUser::OutputInfo(  level, data );
}
void P4ClientUserWrap::OutputBinary( const_char *data, int length )
{
	ClientUser::OutputBinary(data, length );
}
void P4ClientUserWrap::OutputText( const_char *data, int length )
{
	if (m_hwndNotify)
	{
		::SendMessage(m_hwndNotify, NM_SENDINFO, (WPARAM)data, 0);
	}
	
	ClientUser::OutputText(data, length);
}

void P4ClientUserWrap::OutputStat( StrDict *varList )
{
/*	clientFile      -- local path
	depotFile       -- name in depot
	headAction      -- action at head rev, if in depot
	headChange      -- head rev changelist#, if in depot
	headRev         -- head rev #, if in depot
	headType        -- head rev type, if in depot
	headTime        -- head rev mod time, if in depot
	haveRev         -- rev had on client, if on client
	action          -- open action, if opened
	change          -- open changelist#, if opened
	unresolved      -- unresolved integration records
	otherOpen       -- set if someone else has it open
	otherLock       -- set if someone else has it locked
	ourLock         -- set if this user/client has it locked*/

	//StrPtr * pClientPath = varList->GetVar("clientFile");

	m_fs.bControlled = false;
	m_fs.bOpenForEdit = false;
	m_fs.bOpenByOther = false;
	m_fs.bNewerInSc = false;

	StrPtr * pHeadRev = varList->GetVar("headRev");
	if (pHeadRev)
	{
		m_fs.bControlled = true;
		StrPtr* pHaveRev = varList->GetVar("haveRev");
		if (pHaveRev)
		{
			m_fs.bNewerInSc = (strtoul(pHaveRev->Text(), 0, 10) < strtoul(pHeadRev->Text(), 0, 10));

			StrPtr * pAction = varList->GetVar("action");
			if (pAction)
			{
				m_fs.bOpenForEdit = true;	
			}

			StrPtr * pOther = varList->GetVar("otherOpen");
			if (pOther)
				m_fs.bOpenByOther = true;
		}
		else
			m_fs.bNewerInSc = true;
	}
}

void P4ClientUserWrap::Prompt( const StrPtr &msg, StrBuf &rsp, 
			int noEcho, Error *e )
{
	ClientUser::Prompt(msg, rsp, noEcho, e );
}


void P4ClientUserWrap::ErrorPause( char *errBuf, Error *e )
{
	ClientUser::ErrorPause(errBuf, e);
}

void P4ClientUserWrap::Edit( FileSys *f1, Error *e )
{
	ClientUser::Edit(f1, e);
}

void P4ClientUserWrap::Diff( FileSys *f1, FileSys *f2, int doPage, 
			char *diffFlags, Error *e )
{
	const char* strDiffApp = "p4diff.exe";
	string strDiffPath = strDiffApp;
	bool bNeedP4Path = false;

	HKEY P4Key;
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Perforce\\P4win\\Options", 0,                             // reserved
										KEY_READ, &P4Key) )
	{
		vector<unsigned char> P4Buff;
		P4Buff.resize(256);
		DWORD dwSize = P4Buff.size()-1;
		DWORD dwType;
		
		//assume that perforce is configured to use its own diff utility
		//if so we'll need its path
		bNeedP4Path = true;

		//check to see if perforce is configured to use its own diff util
		if (ERROR_SUCCESS == RegQueryValueEx(P4Key, "DiffAppInternal", 0, &dwType, &(P4Buff[0]), &dwSize))
		{
			if (REG_SZ == dwType)
			{
				char *cszDiffApInternal = (char*) &(P4Buff[0]);
				if (cszDiffApInternal[0] == '0')
				{
					//perforce is configured to use an external diff utility - get the name (and hopefully path)
					dwSize = P4Buff.size()-1;
					if (ERROR_SUCCESS == RegQueryValueEx(P4Key, "DiffApp", 0, &dwType, &(P4Buff[0]), &dwSize))
					{
						strDiffPath = (char*)&(P4Buff[0]);
						bNeedP4Path = false;
					}
				}
			}
		}
		RegCloseKey(P4Key);
	}

	if (bNeedP4Path)
	{
		//perforce is set to use its internal diff - since it probably isn't on the path, 
		//get the location of the perforce dir from the registry

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Perforce\\environment", 0,                             // reserved
											KEY_READ, &P4Key) )
		{
			vector<unsigned char> P4Dir;
			P4Dir.resize(256);
			DWORD dwSize = P4Dir.size()-1;
			DWORD dwType;
			if (ERROR_SUCCESS == RegQueryValueEx(P4Key, "P4INSTROOT", 0, &dwType, &(P4Dir[0]), &dwSize))
			{
				strDiffPath = (char*) &(P4Dir[0]);
				if (strDiffPath.size() > 0)
				{
					if (strDiffPath[strDiffPath.size()-1] != '\\')
						strDiffPath += '\\';
				}
				strDiffPath += strDiffApp;
			}
			RegCloseKey(P4Key);
		}
	}


	
	string app = strDiffPath;
	string cmd = "\"";
	cmd += strDiffPath;
	cmd += "\"";
	cmd += " ";
	cmd += "\"";
	cmd += f1->Name();
	cmd += "\"";

	cmd += " ";
	cmd += "\"";
	cmd += f2->Name();
	cmd += "\"";

	
	STARTUPINFO si = {0};
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = {0};
	if (CreateProcess(app.c_str(), (char*)cmd.c_str(), 0, 0, 0, 0, 0, 0, &si, &pi))
	{
		//allow the process to startup before continuing - the temp file will be
		//deleted when we return.
		WaitForInputIdle(pi.hProcess, 2000);
	}
	else
	{
		
	}

	//ClientUser::Diff(f1, f2, doPage, diffFlags, e );
}

void P4ClientUserWrap::Merge( FileSys *base, FileSys *leg1, FileSys *leg2, 
			FileSys *result, Error *e )
{
	ClientUser::Merge(base, leg1, leg2, result, e );
}

void P4ClientUserWrap::Help( const_char *const *help )
{
	ClientUser::Help(help);
}

FileSys	*P4ClientUserWrap::File( FileSysType type )
{
	return ClientUser::File(type);
}
