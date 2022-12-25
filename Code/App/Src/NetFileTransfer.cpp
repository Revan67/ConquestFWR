//--------------------------------------------------------------------------//
//                                                                          //
//                          NetFileTransfer.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/NetFileTransfer.cpp 8     4/07/00 7:57a Jasony $

   FTP implementation over DirectPlay, using non-guaranteed packets

*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "NetFileTransfer.h"
#include "NetPacket.h"
#include "NetBuffer.h"
#include "Startup.h"
#include "CQTrace.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <FileSys.h>
#include <EventSys.h>
#include <MemFile.h>
#include <dplobby.h>

#pragma warning (disable : 4200)


#define PACKET_SIZE		400
#define TIMEOUT_PERIOD	500
#define MAX_THROUGHPUT	50000
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct FTCHANNEL
{
	FTCHANNEL *	pNext;
	C8			fileName[32];
	FTSTATUS	status;
	U32			receiverConn, senderConn;	// connection IDs  (NOT playerID's !!)
	U32			receiverID, senderID;		// playerID's 
	U32			timeout;					// tick when we last received a message from remote side
	U32			fileSize;
	U8 *		buffer;
	union {
	S32			lastPacketSent;
	S32			lastPacketReceived;
	};


	FTCHANNEL (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~FTCHANNEL (void)
	{
		free(buffer);
		memset(this, 0, sizeof(*this));
	}
};
//--------------------------------------------------------------------------//
//
enum FTCODE
{
	FTCODE_CREQUEST,		// connect request
	FTCODE_CREPLY,			// connect reply
	FTCODE_DSEND,			// data send
	FTCODE_DREPLY			// data reply
};

//--------------------------------------------------------------------------//
//
struct CREQUEST_PACKET : BASE_PACKET
{
	FTCODE		code;
	U32			receiverConn;
	C8			fileName[32];

	CREQUEST_PACKET (U32 _receiverConn = 0, const C8 *_fileName=0)
	{
		dwSize = sizeof(*this);
		type = PT_FILE_TRANSFER;
		code  = FTCODE_CREQUEST;
		receiverConn = _receiverConn;
		if (_fileName)
			strncpy(fileName, _fileName, sizeof(fileName));
		else
			fileName[0] = 0;
		fileName[31] = 0;
	}
};
//--------------------------------------------------------------------------//
//
struct CREPLY_PACKET : BASE_PACKET
{
	FTCODE		code;
	U32			receiverConn;
	U32			senderConn;
	U32			fileSize;
	FTSTATUS	reply;
	
	CREPLY_PACKET (void)
	{
		dwSize = sizeof(*this);
		type = PT_FILE_TRANSFER;
		code  = FTCODE_CREPLY;
	}					 
};
//--------------------------------------------------------------------------//
//
struct DSEND_PACKET : BASE_PACKET
{
	FTCODE		code;
	U32			receiverConn;
	U32			senderConn;
	S32			packetNumber;		// packetNumber < 0 signals that this channel is invalid
	U8			data[0];

	DSEND_PACKET (void)
	{
		dwSize = sizeof(*this);
		type = PT_FILE_TRANSFER;
		code = FTCODE_DSEND;
	}
};
//--------------------------------------------------------------------------//
//
struct DREPLY_PACKET : BASE_PACKET
{
	FTCODE		code;
	U32			receiverConn;
	U32			senderConn;
	S32			lastPacketReceived;
	FTSTATUS	reply;

	DREPLY_PACKET (void)
	{
		dwSize = sizeof(*this);
		type = PT_FILE_TRANSFER;
		code = FTCODE_DREPLY;
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE FileTransfer : public IFileTransfer, 
											 ConnectionPointContainer<FileTransfer>,
											 IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(FileTransfer)
	DACOM_INTERFACE_ENTRY(IFileTransfer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	BEGIN_DACOM_MAP_OUTBOUND(FileTransfer)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IFileTransferCallback", point)
	END_DACOM_MAP()

	ConnectionPoint<FileTransfer,IFileTransferCallback> point;

	//--------------------
	// data members
	//--------------------

	U32 startingTick, currentTick;
	U32 sequenceID;
	FTCHANNEL *pChannelList;
	U32 eventHandle;			// handle to event callback

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	FileTransfer (void);

	~FileTransfer (void);
	
	/* IFileTransfer methods */

	DEFMETHOD(Initialize) (void);

	DEFMETHOD_(FTCHANNEL *,GetFile) (DPID fromID, const C8 *fileName);

	DEFMETHOD_(FTSTATUS,CreateFile) (FTCHANNEL * ftChannel, IFileSystem ** file);

	DEFMETHOD_(BOOL32,CloseChannel) (FTCHANNEL * ftChannel);

	DEFMETHOD_(BOOL32,GetTransferProgress) (FTCHANNEL * ftchannel, U32 * bytesReceived, U32 * totalBytes);

	virtual BOOL32 __stdcall EnumerateChannels (IFileTransferEnumerator * enumerator);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* FileTransfer methods */

	void init (void);		// called once at startup

	void update (void);		// return TRUE if handled at least one timeout

	void receiveFTPacket (BASE_PACKET * pPacket);
	
	bool handleTimeout (FTCHANNEL * ftchannel);		// returns true if we have more data to send
	
	void reset (void);
	
	void receiveCREQUEST (CREQUEST_PACKET * packet);
	
	void receiveCREPLY (CREPLY_PACKET * packet);

	void receiveDREPLY (DREPLY_PACKET * packet);

	void receiveDSEND (DSEND_PACKET * packet);

	void fillBuffer (FTCHANNEL * ftchannel);		// should set status on error

	FTCHANNEL * findChannel (U32 receiverConn, U32 receiverID, U32 senderID);

	BOOL32 verifyChannel (FTCHANNEL * ftchannel);

	U32 getBaudRate (void);

	// move a channel to the front of the list, assumes channel is already in the list
	void moveToFront (FTCHANNEL * ftchannel);

	U32 setTick (void)
	{
		return (currentTick = GetTickCount() - startingTick);
	}

	U32 getSequence (void)
	{
		return sequenceID++;
	}

	IDAComponent * getBase (void)
	{
		return (IFileTransfer *) this;
	}
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
FileTransfer::FileTransfer (void) : point(0)
{
	startingTick = GetTickCount() - 1;
	sequenceID = currentTick = 1;
}
//--------------------------------------------------------------------------//
//
FileTransfer::~FileTransfer (void)
{
	COMPTR<IDAConnectionPoint> connection;

	reset();

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//--------------------------------------------------------------------------//
//
GENRESULT FileTransfer::Initialize (void)
{
	reset();
	return GR_OK;
}
//--------------------------------------------------------------------------//
// Close connections initiated by remote machine
// Invalidate connection initiated locally
//
void FileTransfer::reset (void)
{
	FTCHANNEL *pTmp, *pPrev;
	
	while (pChannelList)
	{
		if (PLAYERID==0 || pChannelList->receiverID == PLAYERID)		// we initiated the connection
		{
			pChannelList->status = FTS_INVALIDCHANNEL;
			break;
		}
		// else this is remote connection, ok to delete
		pTmp = pChannelList->pNext;
		delete pChannelList;
		pChannelList = pTmp;
	}

	if ((pPrev = pChannelList) != 0)
	while ((pTmp = pPrev->pNext) != 0)
	{
		if (PLAYERID == 0 || pTmp->receiverID == PLAYERID)		// we initiated the connection
		{
			pTmp->status = FTS_INVALIDCHANNEL;
			pPrev = pTmp;
		}
		else	// else this is remote connection
		{
			pTmp = pTmp->pNext;
			delete pPrev->pNext;
			pPrev->pNext = pTmp;
		}
	}
}
//--------------------------------------------------------------------------//
// ftchannel is already a member of the list
//
void FileTransfer::moveToFront (FTCHANNEL * ftchannel)
{
	FTCHANNEL * node = pChannelList, *pPrev = 0;

	CQASSERT(node);
	while (node && node != ftchannel)
	{
		pPrev = node;
		node = node->pNext;
	}
	CQASSERT(node);

	if (pPrev)		// if not the first element of the list already
	{
		pPrev->pNext = ftchannel->pNext;
		ftchannel->pNext = pChannelList;
		pChannelList = ftchannel;
	}
}
//--------------------------------------------------------------------------//
//
FTCHANNEL * FileTransfer::GetFile (DPID fromID, const C8 *fileName)
{
	FTCHANNEL * ftchannel = new FTCHANNEL;

	setTick();

	ftchannel->pNext = pChannelList;
	pChannelList = ftchannel;

	strncpy(ftchannel->fileName, fileName, sizeof(ftchannel->fileName));
	ftchannel->fileName[sizeof(ftchannel->fileName)-1] = 0;
	ftchannel->status = FTS_INITIALIZING;
	ftchannel->receiverConn = getSequence();
	ftchannel->senderConn = 0;		// unknown
	ftchannel->receiverID = PLAYERID;
	ftchannel->senderID = fromID;
	ftchannel->timeout = 0;
	ftchannel->fileSize = 0;
	ftchannel->buffer = 0;
	ftchannel->lastPacketReceived = -1;		// haven't received a packet yet

	handleTimeout(ftchannel);

	return ftchannel;
}
//--------------------------------------------------------------------------//
//
FTSTATUS FileTransfer::CreateFile (FTCHANNEL * ftchannel, IFileSystem ** file)
{
	FTSTATUS result = ftchannel->status;

	*file = 0;
	if (result == FTS_SUCCESS)
	{
		MEMFILEDESC mdesc = ftchannel->fileName;
		mdesc.lpBuffer = ftchannel->buffer;
		mdesc.dwBufferSize = ftchannel->fileSize;

		if (CreateUTFMemoryFile(mdesc, file) != GR_OK)
		{
			result = FTS_INVALIDFILE;		// could not create the file	
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 FileTransfer::CloseChannel (FTCHANNEL * ftchannel)
{
	BOOL32 result = 0;
	FTCHANNEL *pNode = pChannelList, *pPrev = 0;

	while (pNode)
	{
		if (pNode == ftchannel)
		{
			if (pPrev == 0)	// first entry in the list
				pChannelList = pNode->pNext;
			else
				pPrev->pNext = pNode->pNext;

			delete ftchannel;
			result = 1;
			break;
		}

		pPrev = pNode;
		pNode = pNode->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 FileTransfer::GetTransferProgress (FTCHANNEL * ftchannel, U32 * bytesReceived, U32 * totalBytes)
{
	BOOL32 result = 0;

	switch (ftchannel->status)
	{
	case FTS_SUCCESS:
		result = 1;
		*bytesReceived = *totalBytes = ftchannel->fileSize;
		break;

	case FTS_INITIALIZING:
	case FTS_INPROGRESS:
		*totalBytes = ftchannel->fileSize;
		*bytesReceived = (ftchannel->lastPacketReceived+1) * PACKET_SIZE;
		result = 1;
		break;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 FileTransfer::EnumerateChannels (IFileTransferEnumerator * enumerator)
{
	BOOL32 result = 1;
	FTCHANNEL *pNode = pChannelList;
	FTCHANNELDEF channel;

	while (pNode)
	{
		strncpy(channel.fileName, pNode->fileName, sizeof(channel.fileName));
		channel.status = pNode->status;
		channel.receiverID = pNode->receiverID;
		channel.senderID = pNode->senderID;
		channel.fileSize = pNode->fileSize;
		channel.bytesTransfered = (pNode->lastPacketReceived+1) * PACKET_SIZE;

		if ((result = enumerator->EnumerateFTChannel(channel)) == 0)
			break;

		pNode = pNode->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void FileTransfer::receiveFTPacket (BASE_PACKET * pPacket)
{
	setTick();

	CREPLY_PACKET * packet = (CREPLY_PACKET *) pPacket;

	switch (packet->code)
	{
	case FTCODE_CREQUEST:	// we are a sender
		receiveCREQUEST((CREQUEST_PACKET *)packet);
		break;

	case FTCODE_CREPLY:		// we are a receiver
		receiveCREPLY((CREPLY_PACKET *)packet);
		break;

	case FTCODE_DSEND:		// we are a receiver
		receiveDSEND((DSEND_PACKET *)packet);
		break;

	case FTCODE_DREPLY:		// we are a sender
		receiveDREPLY((DREPLY_PACKET *)packet);
		break;
	} // end switch (packet->code)
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT FileTransfer::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_NETPACKET:
		if (((BASE_PACKET *)param)->type == PT_FILE_TRANSFER)
			receiveFTPacket((BASE_PACKET *)param);
		break;	
	
	case CQE_UPDATE:
	case CQE_DPLAY_MSGWAITING:		// also do work in the background
		update();
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void FileTransfer::update (void)
{
	bool bDataToSend;
	setTick();

	do
	{
		FTCHANNEL *pNode = pChannelList, *pNext;
		bDataToSend = false;

		while (pNode)
		{
			pNext = pNode->pNext;		
		 
			if (pNode->timeout==0 || currentTick - pNode->timeout > TIMEOUT_PERIOD)
				bDataToSend |= handleTimeout(pNode);

			// pNode may be invalid at this point!

			pNode = pNext;		
		}

	} while (bDataToSend);
}
//--------------------------------------------------------------------------//
// return true if there is more data to send
//
bool FileTransfer::handleTimeout (FTCHANNEL * ftchannel)
{
	switch (ftchannel->status)
	{
	case FTS_INITIALIZING:		// waiting to complete connection stage
		if (ftchannel->receiverID == PLAYERID)  // waiting to start receiving the data
		{
			CREQUEST_PACKET packet(ftchannel->receiverConn, ftchannel->fileName);
			ftchannel->timeout = 0;

			if (verifyChannel(ftchannel))
			{
				NETBUFFER->Send(ftchannel->senderID, 0, &packet);
				ftchannel->timeout = currentTick;
			}
		}
		else	// waiting to start sending the data
		{
			CREPLY_PACKET packet;

			packet.receiverConn = ftchannel->receiverConn;
			packet.senderConn = ftchannel->senderConn;
			packet.fileSize   = ftchannel->fileSize;
			packet.reply	  = FTS_SUCCESS;
			ftchannel->timeout = 0;

			if (verifyChannel(ftchannel))
			{
				NETBUFFER->Send(ftchannel->receiverID, 0, &packet);
				ftchannel->timeout = currentTick;
			}
		}
		break;

	case FTS_INPROGRESS:			// waiting to complete data exchange
		if (ftchannel->receiverID == PLAYERID)  // waiting to receive more data
		{
			DREPLY_PACKET packet;

			packet.receiverConn = ftchannel->receiverConn;
			packet.senderConn = ftchannel->senderConn;
			packet.lastPacketReceived = ftchannel->lastPacketReceived;
			packet.reply = FTS_SUCCESS;
			ftchannel->timeout = 0;

			if (verifyChannel(ftchannel))
			{
				NETBUFFER->Send(ftchannel->senderID, 0, &packet);
				ftchannel->timeout = currentTick;
			}
		}
		else  // waiting to send more data
		if (NETBUFFER->TestFTPSend(PACKET_SIZE))
		{
			DSEND_PACKET *packet;
			U32 dataSent;
			S32 maxPacket;	// number of packets - 1
			U32 dataToSend = (ftchannel->fileSize) ? (ftchannel->fileSize-1) : 0;
			BOOL32 bWasSent;

			maxPacket = dataToSend / PACKET_SIZE;

			if (ftchannel->lastPacketSent >= maxPacket)
			{
				ftchannel->lastPacketSent = maxPacket-1;		// resend last packet
			}

			if ((dataSent = PACKET_SIZE * (ftchannel->lastPacketSent + 1)) > ftchannel->fileSize)
				dataSent = ftchannel->fileSize;
			if ((dataToSend = ftchannel->fileSize - dataSent) > PACKET_SIZE)
				dataToSend = PACKET_SIZE;

			packet = (DSEND_PACKET *) malloc(sizeof(DSEND_PACKET) + dataToSend);
			packet->dwSize = sizeof(DSEND_PACKET) + dataToSend;
			packet->type = PT_FILE_TRANSFER;
			packet->code = FTCODE_DSEND;
			packet->receiverConn = ftchannel->receiverConn;
			packet->senderConn = ftchannel->senderConn;
			packet->packetNumber = ftchannel->lastPacketSent + 1;
			memcpy(packet->data, ftchannel->buffer+dataSent, dataToSend);
			ftchannel->timeout = 0;

			if ((bWasSent = verifyChannel(ftchannel)) != 0)
			{
				NETBUFFER->Send(ftchannel->receiverID, 0, packet);	
				ftchannel->lastPacketSent++;
				if (ftchannel->lastPacketSent == maxPacket)
					ftchannel->timeout = currentTick;
			}

			free(packet);
			if (bWasSent && ftchannel->lastPacketSent != maxPacket)  // if we haven't sent the last packet
			{
				return true;
			}
		}
		else // out of bandwidth, start with this guy next time
		{
			moveToFront(ftchannel);
		}
		break;
	}

	return false;
}
//----------------------------------------------------------------------------//
//
FTCHANNEL * FileTransfer::findChannel (U32 receiverConn, U32 receiverID, U32 senderID)
{
	FTCHANNEL * result = pChannelList;

	while (result)
	{
		if (result->receiverConn == receiverConn &&
			result->receiverID == receiverID &&
			result->senderID == senderID)
		{
			switch (result->status)
			{
			case FTS_INVALIDFILE:
			case FTS_INVALIDCHANNEL:
			case FTS_CLOSED:
				result = 0;
				break;
			}
			break;
		}

		result = result->pNext;
	}

	return result;
}
//----------------------------------------------------------------------------//
// We are a sender. Remote player is requesting a file.
//
void FileTransfer::receiveCREQUEST (CREQUEST_PACKET * packet)
{
	FTCHANNEL * ftchannel = findChannel(packet->receiverConn, packet->fromID, PLAYERID);

	if (ftchannel == 0)	// we are starting a new connection
	{
		ftchannel = new FTCHANNEL;

		strncpy(ftchannel->fileName, packet->fileName, sizeof(ftchannel->fileName));
		ftchannel->fileName[sizeof(ftchannel->fileName)-1] = 0;
		ftchannel->status = FTS_INITIALIZING;
		ftchannel->receiverConn = packet->receiverConn;
		ftchannel->senderConn   = getSequence();
		ftchannel->receiverID = packet->fromID;
		ftchannel->senderID = PLAYERID;
		ftchannel->timeout = 0;
		ftchannel->lastPacketSent = -1;		// we haven't sent anything yet
		ftchannel->fileSize = 0;
		ftchannel->buffer = 0;
		
		fillBuffer(ftchannel);		// should set status on error


		if (ftchannel->status == FTS_INITIALIZING)	
		{
			// add channel to the end of the list
			FTCHANNEL * tmp = pChannelList;
			if (tmp)
			{
				while (tmp->pNext)
					tmp = tmp->pNext;
				tmp->pNext = ftchannel;
			}
			else
				pChannelList = ftchannel;

			handleTimeout(ftchannel);
		}
		else  // if we failed to load file
		{
			CREPLY_PACKET packet;

			packet.receiverConn = ftchannel->receiverConn;
			packet.senderConn = ftchannel->senderConn;
			packet.fileSize   = ftchannel->fileSize;
			packet.reply	  = FTS_INVALIDFILE;

			NETBUFFER->Send(ftchannel->receiverID, 0, &packet);

			delete ftchannel;
		}
	}
	else	// got a repeat message
	{
		if (ftchannel->status == FTS_INITIALIZING)	// still waiting
			handleTimeout(ftchannel);

		// else we are already transmitting data, safe to ignore this message
	}
}
//----------------------------------------------------------------------------//
// we are a sender
//
void FileTransfer::receiveDREPLY (DREPLY_PACKET * packet)
{
	FTCHANNEL * ftchannel = findChannel(packet->receiverConn, packet->fromID, PLAYERID);

	if (ftchannel == 0)		// non-existent connection
	{
		if (packet->reply == FTS_SUCCESS)			// everything is OK
		{
			DSEND_PACKET dsend;

			dsend.receiverConn = packet->receiverConn;
			dsend.senderConn = packet->senderConn;
			dsend.packetNumber = -1;		// packetNumber < 0 signals that this channel is invalid

			NETBUFFER->Send(packet->fromID, 0, &dsend);
		}
	}
	else	// receiver would like some data
	{
		if (packet->reply == FTS_SUCCESS)			// everything is OK
		{
			ftchannel->status = FTS_INPROGRESS;		// start the data flow
			ftchannel->lastPacketSent = packet->lastPacketReceived;
			handleTimeout(ftchannel);
		}
		else   // received some error
		{
			ftchannel->status = FTS_CLOSED;		// closed by remote side
		}
	}
}
//----------------------------------------------------------------------------//
// we are a receiver
//
void FileTransfer::receiveCREPLY (CREPLY_PACKET * packet)
{
	FTCHANNEL * ftchannel = findChannel(packet->receiverConn, PLAYERID, packet->fromID);

	if (ftchannel == 0)		// unknown channel
	{
		if (packet->reply == FTS_SUCCESS)
		{
			DREPLY_PACKET rpacket;
		
			rpacket.receiverConn = packet->receiverConn;
			rpacket.senderConn = packet->senderConn;
			rpacket.lastPacketReceived = -1;
			rpacket.reply = FTS_INVALIDCHANNEL;

			NETBUFFER->Send(packet->fromID, 0, &rpacket);
		}
	}
	else
	{
		if (ftchannel->status == FTS_INITIALIZING)
		{
			ftchannel->senderConn = packet->senderConn;
			ftchannel->fileSize = packet->fileSize;
			ftchannel->status = packet->reply;

			if (ftchannel->status == FTS_SUCCESS)
			{
				ftchannel->status = FTS_INPROGRESS;		// start the data flow
				ftchannel->buffer = (U8 *) malloc(ftchannel->fileSize);
				handleTimeout(ftchannel);
			}
		}
	}
}
//----------------------------------------------------------------------------//
// we are a receiver
//
void FileTransfer::receiveDSEND (DSEND_PACKET * packet)
{
	FTCHANNEL *	ftchannel = findChannel(packet->receiverConn, PLAYERID, packet->fromID);

	if (ftchannel == 0)		// unknown channel
	{
		if (packet->packetNumber >= 0)	// is data valid?
		{
			DREPLY_PACKET rpacket;
		
			rpacket.receiverConn = packet->receiverConn;
			rpacket.senderConn = packet->senderConn;
			rpacket.lastPacketReceived = -1;
			rpacket.reply = FTS_INVALIDCHANNEL;
	
			NETBUFFER->Send(packet->fromID, 0, &rpacket);
		}
	}
	else
	{
		if (ftchannel->status != FTS_INPROGRESS)
		{
			// do nothing
		}
		else
		if (packet->packetNumber < 0)		// packetNumber < 0 signals that this channel is invalid
			ftchannel->status = FTS_CLOSED;
		else
		if (packet->packetNumber < ftchannel->lastPacketReceived+1)  // already got this one
		{
			// do nothing
		}
		else
		if (packet->packetNumber > ftchannel->lastPacketReceived+1)   // out of order delivery
		{
			handleTimeout(ftchannel);
		}
		else  // in order delivery, accept the data
		{
			U32 offset = (packet->packetNumber * PACKET_SIZE);
			U32 length = packet->dwSize - sizeof(*packet);

			memcpy(ftchannel->buffer + offset, packet->data, length);
			ftchannel->lastPacketReceived++;
			ftchannel->timeout = currentTick;
			if (offset + length >= ftchannel->fileSize)
			{
				DREPLY_PACKET rpacket;
		
				ftchannel->status = FTS_SUCCESS;
					
				rpacket.receiverConn = packet->receiverConn;
				rpacket.senderConn = packet->senderConn;
				rpacket.lastPacketReceived = -1;
				rpacket.reply = FTS_CLOSED;

				NETBUFFER->Send(packet->fromID, 0, &rpacket);		// notify the other side that we are done
			}
		}
	}
}
//----------------------------------------------------------------------------//
// open file, read it all, set buffer, fileSize, and status
//
void FileTransfer::fillBuffer (FTCHANNEL * ftchannel)
{
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = ftchannel->fileName;
	CONNECTION_NODE<IFileTransferCallback> * pNode = point.pClientList;
	DWORD dwRead;

	while (pNode)
	{
		if (pNode->client->RemoteFileRequest(fdesc.lpFileName, file) == GR_OK)
			break;
		pNode = pNode->pNext;
	}

	fdesc.lpImplementation = "DOS";
	if (file != 0 || DACOM->CreateInstance(&fdesc, file) == GR_OK)
	{
		if ((ftchannel->fileSize = file->GetFileSize()) != 0)
		{
			ftchannel->buffer = (U8 *) malloc(ftchannel->fileSize);
			file->SetFilePointer(0, 0);
			file->ReadFile(0, ftchannel->buffer, ftchannel->fileSize, &dwRead, 0);
		}
	}
	else
	{
		ftchannel->status = FTS_INVALIDFILE;
	}
}
//----------------------------------------------------------------------------//
// channel is suspect. Verify that remote player is valid. If not, delete the channel, and return 0.
//
BOOL32 FileTransfer::verifyChannel (FTCHANNEL * ftchannel)
{
	BOOL32 result = 0;
	DPCAPS dpcaps;

	dpcaps.dwSize = sizeof(dpcaps);
	
	if (PLAYERID)
	{
		if (ftchannel->senderID == PLAYERID)
			result = (DPLAY->GetPlayerCaps(ftchannel->receiverID, &dpcaps, 0) == DP_OK);
		else
			result = (DPLAY->GetPlayerCaps(ftchannel->senderID, &dpcaps, 0) == DP_OK);
	}

	if (result == 0)
	{
		if (PLAYERID==0 || ftchannel->receiverID == PLAYERID)		// we initiated the connection
			ftchannel->status = FTS_CLOSED;		// closed by remote side
		else
			CloseChannel(ftchannel);
	}

	return result;
}
//----------------------------------------------------------------------------//
//
U32 FileTransfer::getBaudRate (void)
{
	U32 result = 0;
	DPCAPS dpcaps;

	dpcaps.dwSize = sizeof(dpcaps);
	
	if (PLAYERID && DPLAY->GetCaps(&dpcaps, 0) == DP_OK)
		result = dpcaps.dwHundredBaud;

	return result;
}
//----------------------------------------------------------------------------//
//
void FileTransfer::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//----------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _netftp : GlobalComponent
{
	FileTransfer * ftp;

	virtual void Startup (void)
	{
		FILETRANSFER = ftp = new DAComponent<FileTransfer>;
		AddToGlobalCleanupList((IDAComponent **) &FILETRANSFER);
	}

	virtual void Initialize (void)
	{
		ftp->init();
	}
};

static _netftp netftp;


//----------------------------------------------------------------------------//
//--------------------------End NetFileTransfer.cpp---------------------------//
//----------------------------------------------------------------------------//
