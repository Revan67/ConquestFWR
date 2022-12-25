#include "VoiceNet.h"

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceNet::VoiceNet(UINT listenPortBase, CWnd& window)
{
	Initialize(listenPortBase, &window, NULL);
}

///////////////////////////////////////////////////////////////////////

VoiceNet::VoiceNet(UINT listenPortBase, IDirectSound& directSoundInstance)
{
	Initialize(listenPortBase, NULL, &directSoundInstance);
}

///////////////////////////////////////////////////////////////////////

void VoiceNet::Initialize(UINT listenPortBase,
							     CWnd* window,
							     IDirectSound* directSoundInstance)
{
	voiceSocket = NULL;
	voiceInput = NULL;
	voiceOutput = NULL;
	listenPort = 0;
	remotePort = 0;
	bytesSent = 0;
	bytesReceived = 0;

	for (UINT port = listenPortBase;
	     port < listenPortBase + 16;
		  port++)
	{
		if (Create(port))
		{
			listenPort = port;
			break;
		}
	}
	if (listenPort)
	{
		if (!Listen())
		{
			Close();
			listenPort = 0;
			return;
		}

		voiceInput = new VoiceNetInput(*this);

		if (window)
		{
			voiceOutput = new VoiceOutput(*window);
		}
		else
		{
			voiceOutput = new VoiceOutput(*directSoundInstance);
		}
	}
}

///////////////////////////////////////////////////////////////////////

bool VoiceNet::Connect(const char* hostName, UINT hostPort)
{
	Disconnect();

	voiceSocket = new VoiceSocket(*this);

	if (voiceSocket->Connect(hostName, hostPort))
	{
		remoteHost = hostName;
		remotePort = hostPort;
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////

void VoiceNet::Disconnect()
{
	if (voiceSocket)
	{
		delete voiceSocket; voiceSocket = NULL;

		remoteHost = "";
		remotePort = 0;
		bytesSent = 0;
		bytesReceived = 0;
	}
}

///////////////////////////////////////////////////////////////////////

void VoiceNet::OnAccept(int nErrorCode)
{
	Disconnect();

	CSocket::OnAccept(nErrorCode);

	voiceSocket = new VoiceSocket(*this);

	SOCKADDR_IN addr;
	int addrSize = sizeof addr;

   Accept(*voiceSocket, (SOCKADDR*)&addr, &addrSize);

	remotePort = addr.sin_port;
	remoteHost = inet_ntoa(addr.sin_addr);

	voiceSocket->SetOptions();
}

///////////////////////////////////////////////////////////////////////

void VoiceNet::Speak(void* buffer, int bufferSize)
{
	voiceOutput->Output(buffer, bufferSize);

	bytesReceived += bufferSize;
}

///////////////////////////////////////////////////////////////////////

void VoiceNet::Send(void* buffer, int bufferSize)
{
	if (voiceSocket)
	{
		voiceSocket->Send(buffer, bufferSize);
		bytesSent += bufferSize;
	}
}

///////////////////////////////////////////////////////////////////////

CString VoiceNet::GetLocalHost() const
{
	char buffer[32];
	gethostname(buffer, sizeof buffer);
	return CString(buffer);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceSocket::VoiceSocket(VoiceNet& voiceNetIn) :
	voiceNet(voiceNetIn)
{
}

///////////////////////////////////////////////////////////////////////

VoiceSocket::~VoiceSocket()
{
	Close();
}

///////////////////////////////////////////////////////////////////////

void VoiceSocket::SetOptions()
{
	BOOL noDelay = TRUE;
	SetSockOpt(TCP_NODELAY, &noDelay, sizeof noDelay);
}

///////////////////////////////////////////////////////////////////////

bool VoiceSocket::Connect(const char* hostAddress, UINT hostPort)
{
	Disconnect();

	Create();
	
	if (!CSocket::Connect(hostAddress, hostPort)) return FALSE;

	SetOptions();

	return TRUE;
}

///////////////////////////////////////////////////////////////////////

void VoiceSocket::Disconnect()
{
	Close();
}

///////////////////////////////////////////////////////////////////////

void VoiceSocket::OnClose(int nErrorCode)
{
	voiceNet.Disconnect();
}

///////////////////////////////////////////////////////////////////////

void VoiceSocket::OnReceive(int nErrorCode)
{
	int messageSize;

	int bufferSize;
	char* buffer;

	messageSize = Receive(&bufferSize, sizeof bufferSize);

	if (messageSize < 0)
	{
		voiceNet.Disconnect();
		return;
	}
	else if (messageSize == 0)
	{
		voiceNet.Disconnect();
		return;
	}

	buffer = new char[bufferSize];

	int remaining = bufferSize;

	while (remaining)
	{
		messageSize = Receive(buffer + (bufferSize - remaining),
									 remaining);

		if (messageSize < 0)
		{
			delete[] buffer;
			voiceNet.Disconnect();
			return;
		}
		else if (messageSize == 0)
		{
			delete[] buffer;
			voiceNet.Disconnect();
			return;
		}
		remaining -= messageSize;
	}

	voiceNet.Speak(buffer, bufferSize);

	delete[] buffer;
}

///////////////////////////////////////////////////////////////////////

void VoiceSocket::Send(void* buffer, int bufferSize)
{
	CSocket::Send(&bufferSize, sizeof bufferSize);
	CSocket::Send(buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceNetInput::VoiceNetInput(VoiceNet& voiceNetIn) :
	voiceNet(voiceNetIn)
{
	Start();
}

///////////////////////////////////////////////////////////////////////

VoiceNetInput::~VoiceNetInput()
{
	Stop();
}

///////////////////////////////////////////////////////////////////////

void VoiceNetInput::OnInputReady(void* buffer, DWORD bufferSize)
{
	voiceNet.Send(buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
