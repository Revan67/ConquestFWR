#ifndef VOICE_NET_H
#define VOICE_NET_H

#include <afxsock.h>

#include "VoiceIO.h"

//////////////////////////////////////////////////////////////////

class VoiceNet : public CSocket
{
public:
	//
	//	VoiceNet()
	//		Creates a full-duplex TCP/IP connection for voice I/O.
	//		This version is used when a IDirectSound instance already
	//		exists.
	//
	// listenPortBase: The port number for this instance of
	//		the VoiceNet's listen port.  If this port is already
	//		in use, the constructor will increment this number
	//		attempting to find an available port.  You can call
	//		GetLocalPort() to tell if the constructor was successful
	//		and which port number was assigned to the listen port.
	//
	// directSoundInstance: Specifies an existing handle to a
	//		IDirectSound instance.  This would be the instance
	//		handle returned by the IAudioManager class when the
	//		IAudioManager is in use.
	//

	VoiceNet(UINT listenPortBase,
		      IDirectSound& directSoundInstance);

	//
	//	VoiceNet()
	//		Creates a full-duplex TCP/IP connection for voice I/O.
	//		This version is used when the user wants the VoiceNet
	//		class to create a new IDirectSound instance.
	//
	// listenPortBase: See description above
	//
	//	window: Required to properly initialize the IDirectSound
	//		instance.  Used to set the cooperative level of this
	//		application with respect to the sound device.
	//

	VoiceNet(UINT listenPortBase,
				CWnd& window);

	//
	//	Connect()
	//		Connects this VoiceNet instance to another VoiceNet
	//		instance running on the network.
	//
	//	remoteHost: Specifies the name or IP address of the 
	//		other host.
	//
	//	remotePort: Specifies the port number of the remote
	//		host's VoiceNet listen port.
	//

	bool Connect(const char* remoteHost, UINT remotePort);

	//
	// Disconnect()
	//		Disconnects the VoiceNet instance from any other
	//		VoiceNet instance it may be connect to.
	//
	void Disconnect();

	//
	// GetLocalHost()
	//		Returns the host name for the local host.
	//

	CString GetLocalHost() const;

	//
	// GetRemoteHost()
	//		Returns the IP address string for the connected
	//		VoiceNet's host, if any.
	//

	CString GetRemoteHost() const { return remoteHost; }

	//
	// GetLocalPort()
	//		Returns the port number for this instance's listen
	//		port.  Returns zero if the instance failed to initialize.
	//
	
	UINT GetLocalPort() const { return listenPort; }

	//
	// GetRemotePort()
	//		Returns the port number being used on this host to
	//		communicate to the remote VoiceNet instance.  A zero
	//		is returned if there is currently no remote connection.
	//

	UINT GetRemotePort() const { return remotePort; }

	//
	// GetBytesSent()
	//		Returns the number of bytes sent to the remote
	//		VoiceNet instance since the last successful
	//		connection was established.
	//

	UINT GetBytesSent() const { return bytesSent; }

	//
	// GetBytesReceived()
	//		Returns the number of bytes received from the remote
	//		VoiceNet instance since the last successful
	//		connection was established.
	//

	UINT GetBytesReceived() const { return bytesReceived; }

protected:

	//
	// OnAccept()
	//		Called when a new VoiceNet connection is requested.
	//		Note that a new connect request will cause any
	//		current connection to be broken! (May be changed)
	//

	virtual void OnAccept(int nErrorCode);

private:
	friend class VoiceSocket;
	friend class VoiceNetInput;

	void Initialize(UINT listenPortBase,
						 CWnd* window,
						 IDirectSound* directSoundInstance);


	void Speak(void* buffer, int bufferSize);

	void Send(void* buffer, int bufferSize);

	VoiceSocket*   voiceSocket;
	VoiceNetInput* voiceInput;
	VoiceOutput*	voiceOutput;

	CString remoteHost;

	UINT listenPort;
	UINT remotePort;

	UINT bytesSent;
	UINT bytesReceived;
};

//////////////////////////////////////////////////////////////////
// The remaining classes are not publicly usable, the are
// only used internally by the VoiceNet class
//

class VoiceNetInput : public VoiceInput
{
private:
	friend class VoiceNet;

	VoiceNetInput(VoiceNet&);
	~VoiceNetInput();

	virtual void OnInputReady(void* buffer, DWORD bufferSize);

	VoiceNet& voiceNet;
};

//////////////////////////////////////////////////////////////////

class VoiceSocket : public CSocket
{
private:
	friend class VoiceNet;

	VoiceSocket(VoiceNet&);
	virtual ~VoiceSocket();

	bool Connect(const char* hostAddress, UINT hostPort);
	void Disconnect();

	void SetOptions();

	void Send(void* buffer, int bufferSize);

	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode );

	VoiceNet& voiceNet;
};

//////////////////////////////////////////////////////////////////

#endif