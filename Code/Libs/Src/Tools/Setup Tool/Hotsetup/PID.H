#ifndef __PID_H__
#define __PID_H__

//
// pid.h
//
//   7/21/95	AjayJ	 Added RandomizePID and PID20LENGTH for PID 2.0
//   1/5/95     WilliamW Corrected the OEM version PID to meet PID 2.0 spec

void GeneratePID(char *pszBuf, UINT cchBuf, DWORD SiteCode, DWORD ProductID, DWORD SerialNum);
BOOL GenerateAndStorePID(LPGETPIDDATA pid);
void RandomizePID(char *szPid, UINT cchBuf, char *szName, BOOL fRandomizeSerial = TRUE);

#define	PID20LENGTH		23

#endif