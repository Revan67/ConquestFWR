#ifndef NETFILETRANSFER_H
#define NETFILETRANSFER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                           NetFileTransfer.h                              //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/NetFileTransfer.h 2     2/12/99 12:25p Jasony $
	
   FTP implementation over DirectPlay, using non-guaranteed packets

*/
//--------------------------------------------------------------------------//
//							IFileTransfer Documentation
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

typedef DWORD DPID, FAR *LPDPID;
struct FTCHANNEL;
typedef FTCHANNEL * FTPCHANNEL;
struct IFileSystem;
struct BASE_PACKET;

//--------------------------------------------------------------------------//
/*
//------------------------------------------------------
//
GENRESULT IFileTransferCallback::RemoteFileRequest (const C8 *fileName, IFileSystem ** file);
	INPUT:
		fileName: ASCIIZ name of the file being requested by the remote machine
		file: Address of IFileSystem pointer that will be set by this method.
	RETURNS:
		GR_OK if the file system instance was created to satisfy the request.
	OUTPUT:
		If the filename is recognized, the routine creates an instance of IFileSystem that can be
		used to read that file.
	NOTES:
		A remote machine may request a file by a special "codename" rather than an actual filename.
		This allows files to be identified by an internal name.
		This method is called when a file tranfer request is received from a remote machine.

//------------------------------------------------------
//
GENRESULT IFileTransfer::Initialize (void);
	RETURNS:
		GR_OK on success.
	OUTPUT:
		Re-initializes internal data structures.
		This method should be called when the network connection has been established.

//------------------------------------------------------
//
FTPCHANNEL IFileTransfer::GetFile (DPID fromID, const C8 *fileName);
	INPUT:
		fromID:  DirectPlay ID of the remote player to get the file from.
		fileName: The ASCIIZ name of the file to retrieve.
	RETURNS:
		A pointer to a FTCHANNEL structure.
	OUTPUT:
		Initializes a file transfer from a remote machine.
	NOTES:
		Use the returned FTCHANNEL pointer in calls to CreateFile() and GetTransferProgress(). 
		When you are done with the connection, you should call CloseChannel to free the internal
		resources used by the connection.

//------------------------------------------------------
//
FTSTATUS IFileTransfer::CreateFile (FTPCHANNEL ftchannel, IFileSystem ** file);
	INPUT:
		ftchannel: Handle to a file transfer connection started by a call to GetFile.
		file: Address of a IFileSystem interface pointer that will receive the instance address.
	RETURNS:
		FTS_SUCCESS:		Transfer complete. *file points to the transfered file.	 [DONE]
		FTS_INITIALIZING:	Waiting to complete connection stage		[WAIT]
		FTS_INPROGRESS:		Waiting to complete data exchange			[WAIT]
		FTS_INVALIDFILE:	Transfer failed, remote file was not found. [FAILURE]
		FTS_CLOSED:			Remote side closed the connection early.	[FAILURE]
		FTS_INVALIDCHANNEL:	'ftchannel' does not correspond to a valid transfer connection. [FAILURE]
	OUTPUT:
		Creates an instance of IFileSytem if the file transfer was completed. If the transfer
		failed, or is still waiting, no file is created and *file is set to NULL.
	NOTES:
		Be sure to close the connection when you are done with it. (See CloseChannel(), below)

//------------------------------------------------------
//
BOOL32 IFileTransfer::CloseChannel (FTPCHANNEL ftchannel);
	INPUT:
		ftchannel: Handle to a file transfer connection started by a call to GetFile.
	RETURNS:
		TRUE if the 'ftchannel' was closed properly.
	OUTPUT:
		Closes the file transfer channel and closes any resources associated with it. This method
		does not affect any files that were created using CreateFile. You should call Release on any 
		created files when you are done with them.
	NOTES:
		If the file transfer was still in the [WAIT] stage, the connection is closed immediately. Any data
		that may have been received is discarded.
		You must still call CloseChannel() to release a connection that has failed.

//------------------------------------------------------
//
BOOL32 IFileTransfer::GetTransferProgress (FTPCHANNEL ftchannel, U32 * bytesReceived, U32 * totalBytes);
	INPUT:
		ftchannel: Handle to a file transfer connection started by a call to GetFile.
		bytesReceived: Address of value that will be set to the number of bytes received so far.
		totalBytes: Address of value that will be set to the total number of bytes expected in the file.
	RETURNS:
		TRUE if the channel specified in 'ftchannel' is valid, and correct values were retured.
	OUTPUT:
		Checks the transfer progress for a connection.

//------------------------------------------------------
//
void IFileTransfer::ReceiveFTPacket (BASE_PACKET * pPacket);	// does not free the packet
	INPUT:
		pPacket: Address of a network packet received from a remote machine. The 'type' of the packet
			should be PT_FILE_TRANSFER.
	OUTPUT:
		Processes the packet received from a remote machine. The packet may be incoming data, or a reply to one
		of our packets, or a request to start a file transfer.
	NOTES:
		Call this method when a PT_FILE_TRANSFER packet is received. The method does not free any resources
		associated with the packet, and makes no assumptions about how it was received.

*/
//--------------------------------------------------------------------------//
//
enum FTSTATUS
{
	FTS_SUCCESS,

	FTS_INITIALIZING,		// waiting to complete connection stage
	FTS_INPROGRESS,			// waiting to complete data exchange

	FTS_INVALIDFILE,		// file not found
	FTS_INVALIDCHANNEL,
	FTS_CLOSED				// remote side closed the connection
};
//--------------------------------------------------------------------------//
// structure used for enumeration
//
struct FTCHANNELDEF
{
	C8			fileName[32];
	FTSTATUS	status;
	U32			receiverID, senderID;		// playerID's 
	U32			fileSize;
	U32			bytesTransfered;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IFileTransferCallback : public IDAComponent
{
	virtual GENRESULT __stdcall RemoteFileRequest (const C8 *fileName, IFileSystem ** file) = 0;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IFileTransferEnumerator
{
	virtual BOOL32 __stdcall EnumerateFTChannel (const struct FTCHANNELDEF & channel) = 0;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IFileTransfer : public IDAComponent
{
	virtual GENRESULT __stdcall Initialize (void) = 0;

	virtual FTPCHANNEL __stdcall GetFile (DPID fromID, const C8 *fileName) = 0;

	virtual FTSTATUS __stdcall CreateFile (FTPCHANNEL ftchannel, IFileSystem ** file) = 0;

	virtual BOOL32 __stdcall CloseChannel (FTPCHANNEL ftchannel) = 0;

	virtual BOOL32 __stdcall GetTransferProgress (FTPCHANNEL ftchannel, U32 * bytesReceived, U32 * totalBytes) = 0;

	virtual BOOL32 __stdcall EnumerateChannels (IFileTransferEnumerator * enumerator) = 0;
};





//---------------------------------------------------------------------------------//
//-------------------------------End NetFileTransfer.h-----------------------------//
//---------------------------------------------------------------------------------//
#endif