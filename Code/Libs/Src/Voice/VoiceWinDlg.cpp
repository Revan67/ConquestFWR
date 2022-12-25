// VoiceWinDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VoiceWin.h"
#include "VoiceWinDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CVoiceWinDlg dialog

CVoiceWinDlg::CVoiceWinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVoiceWinDlg::IDD, pParent),
	  voiceNet(NULL)
{
	//{{AFX_DATA_INIT(CVoiceWinDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVoiceWinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVoiceWinDlg)
	DDX_Control(pDX, IDC_MESSAGEBAR, m_messageBar);
	DDX_Control(pDX, IDC_REMOTEHOST, m_remoteHost);
	DDX_Control(pDX, IDC_REMOTEPORT, m_remotePort);
	DDX_Control(pDX, IDC_LOCALPORT, m_localPort);
	DDX_Control(pDX, IDC_LOCALHOST, m_localHost);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVoiceWinDlg, CDialog)
	//{{AFX_MSG_MAP(CVoiceWinDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BREAK, OnBreak)
	ON_BN_CLICKED(IDC_INITIATE, OnInitiate)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVoiceWinDlg message handlers

BOOL CVoiceWinDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	voiceNet = new VoiceNet(50123, *this);

	if (voiceNet->GetLocalPort())
	{
		m_localHost.SetWindowText(voiceNet->GetLocalHost());

		char buffer[8];
		sprintf(buffer, "%d", voiceNet->GetLocalPort());
		m_localPort.SetWindowText(buffer);

		m_remoteHost.SetWindowText(voiceNet->GetRemoteHost());
		m_remotePort.SetWindowText("50123");

		SetTimer(1, 100, NULL);
	}
	else
	{
		m_localHost.SetWindowText("Error");
		m_localPort.SetWindowText("");
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVoiceWinDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

/////////////////////////////////////////////////////////////////////////////
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.

HCURSOR CVoiceWinDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////

void CVoiceWinDlg::OnBreak() 
{
	voiceNet->Disconnect();
}

/////////////////////////////////////////////////////////////////////////////

void CVoiceWinDlg::OnInitiate() 
{
	CString hostAddr;

	GetWindowText(hostAddr);

	CString remoteHost;
	CString remotePortString;
	UINT	  remotePort;

	m_remoteHost.GetWindowText(remoteHost);
	m_remotePort.GetWindowText(remotePortString);

	remotePort = atoi(remotePortString);

	voiceNet->Connect(remoteHost, remotePort);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CVoiceWinDlg::DestroyWindow() 
{
	delete voiceNet; voiceNet = NULL;

	return CDialog::DestroyWindow();
}

/////////////////////////////////////////////////////////////////////////////

void CVoiceWinDlg::OnTimer(UINT nIDEvent) 
{
	static bool cleared = FALSE;

	if (voiceNet->GetRemotePort())
	{
		m_remoteHost.SetWindowText(voiceNet->GetRemoteHost());

		char buffer[64];
		sprintf(buffer, "%d", voiceNet->GetRemotePort());
		m_remotePort.SetWindowText(buffer);

		sprintf(buffer, "Bytes Sent: %d, Bytes Received: %d",
			     voiceNet->GetBytesSent(),
				  voiceNet->GetBytesReceived());
		m_messageBar.SetWindowText(buffer);

		cleared = FALSE;
	}
	else if (!cleared)
	{
		m_remoteHost.SetWindowText(voiceNet->GetRemoteHost());
		m_remotePort.SetWindowText("50123");
		m_messageBar.SetWindowText("");

		cleared = TRUE;
	}
	
	CDialog::OnTimer(nIDEvent);
}
