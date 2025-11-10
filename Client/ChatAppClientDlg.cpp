
// ChatAppClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatAppClient.h"
#include "ChatAppClientDlg.h"
#include "afxdialogex.h"
#include "resource.h"

#include <string>
#include <atlstr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatAppClientDlg dialog



CChatAppClientDlg::CChatAppClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATAPPCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatAppClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_usernameInput);
    DDX_Control(pDX, IDC_EDIT2, m_passwordInput);
    DDX_Control(pDX, IDC_Regbtn, m_regButton);
    DDX_Control(pDX, IDC_BUTTON1, m_logButton);
    DDX_Control(pDX, IDC_BUTTON2, m_changeMode);
    DDX_Control(pDX, IDC_MODE, m_modeStatic);
    DDX_Control(pDX, IDC_ERROR, m_errorNetwork);
}

BEGIN_MESSAGE_MAP(CChatAppClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT2, &CChatAppClientDlg::OnEnChangeEdit2)
	ON_BN_CLICKED(IDC_BUTTON2, &CChatAppClientDlg::OnBnClickedChangeMode)
	ON_BN_CLICKED(IDC_BUTTON1, &CChatAppClientDlg::OnBnClickedLogButton)
	ON_BN_CLICKED(IDC_Regbtn, &CChatAppClientDlg::OnBnClickedRegButton)
END_MESSAGE_MAP()


// CChatAppClientDlg message handlers

BOOL CChatAppClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	try
	{
		if (!m_client.Connect("127.0.0.1", 9999))
		{
			AfxMessageBox(_T("Không thể kết nối đến Service. Vui lòng thử lại sau."), MB_ICONERROR);
			EndDialog(IDCANCEL); // Đóng dialog nếu không kết nối được
			return TRUE;
		}
	}
	catch (const std::exception& e)
	{
		// Xử lý lỗi nếu WSAStartup thất bại
		CString errorMsg(e.what());
		AfxMessageBox(_T("Lỗi Winsock nghiêm trọng: ") + errorMsg, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// Đặt trạng thái ban đầu là Đăng nhập
	m_isLoginMode = true;
	UpdateUIMode();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatAppClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatAppClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChatAppClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CChatAppClientDlg::OnEnChangeEdit2()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


// === HÀM TRỢ GIÚP CẬP NHẬT UI ===
void CChatAppClientDlg::UpdateUIMode()
{
    if (m_isLoginMode)
    {
        // Chế độ Đăng nhập
        m_logButton.ShowWindow(SW_SHOW);
        m_regButton.ShowWindow(SW_HIDE);
        m_modeStatic.SetWindowText(_T("Đăng nhập"));
        m_changeMode.SetWindowText(_T("Chuyển sang Đăng ký"));
        SetWindowText(_T("Đăng nhập")); // Đổi tiêu đề cửa sổ
    }
    else
    {
        // Chế độ Đăng ký
        m_logButton.ShowWindow(SW_HIDE);
        m_regButton.ShowWindow(SW_SHOW);
        m_modeStatic.SetWindowText(_T("Đăng ký"));
        m_changeMode.SetWindowText(_T("Chuyển sang Đăng nhập"));
        SetWindowText(_T("Đăng ký")); // Đổi tiêu đề cửa sổ
    }
}

// === HÀM XỬ LÝ SỰ KIỆN CLICK ===

// Click vào dòng chữ "Chuyển chế độ"
void CChatAppClientDlg::OnBnClickedChangeMode()
{
    m_isLoginMode = !m_isLoginMode; // Lật trạng thái
    UpdateUIMode(); // Cập nhật lại UI
}

// Click nút "Đăng nhập"
// Click nút "Đăng nhập"
void CChatAppClientDlg::OnBnClickedLogButton()
{
    // BẮT BUỘC PHẢI CÓ DÒNG NÀY ĐẦU TIÊN
    USES_CONVERSION;

    CString cstrUser, cstrPass;
    m_usernameInput.GetWindowText(cstrUser);
    m_passwordInput.GetWindowText(cstrPass);

    if (cstrUser.IsEmpty() || cstrPass.IsEmpty()) {
        AfxMessageBox(_T("Vui lòng nhập đầy đủ Username và Password."), MB_ICONWARNING);
        return;
    }

    std::string user = std::string(CT2A(cstrUser)); // hoặc CW2A(cstrUser, CP_UTF8)
    std::string pass = std::string(CT2A(cstrPass)); // hoặc CW2A(cstrPass, CP_UTF8)

    std::string command = std::string("LOGIN ") + user + " " + pass + "\n";


    // Gửi lệnh
    if (!m_client.SendRequest(command)) {
        AfxMessageBox(_T("Mất kết nối tới service."), MB_ICONERROR);
        EndDialog(IDCANCEL);
        return;
    }

    // Chờ nhận phản hồi
    std::string response = m_client.ReceiveResponse();

    // Xử lý phản hồi
    if (response == "LOGIN_OK")
    {
        AfxMessageBox(_T("Đăng nhập thành công!"), MB_OK);
        m_client.DetachSocket();
        EndDialog(IDOK);
    }
    else if (response == "ERR_WRONG_PASS")
    {
        AfxMessageBox(_T("Sai mật khẩu. Vui lòng thử lại."), MB_ICONWARNING);
    }
    else if (response == "ERR_NOT_FOUND")
    {
        AfxMessageBox(_T("Tên đăng nhập không tồn tại."), MB_ICONWARNING);
    }
    else if (response == "ERR_CONNECTION_LOST")
    {
        AfxMessageBox(_T("Mất kết nối tới service."), MB_ICONERROR);
        EndDialog(IDCANCEL);
    }
    else
    {
        CString errorMsg(response.c_str());
        AfxMessageBox(_T("Lỗi không xác định: ") + errorMsg, MB_ICONERROR);
    }
}

// Click nút "Đăng ký"
void CChatAppClientDlg::OnBnClickedRegButton()
{
    // BẮT BUỘC PHẢI CÓ DÒNG NÀY ĐẦU TIÊN
    USES_CONVERSION;

    CString cstrUser, cstrPass;
    m_usernameInput.GetWindowText(cstrUser);
    m_passwordInput.GetWindowText(cstrPass);

    if (cstrUser.IsEmpty() || cstrPass.IsEmpty()) {
        AfxMessageBox(_T("Vui lòng nhập đầy đủ Username và Password."), MB_ICONWARNING);
        return;
    }

    std::string user = std::string(CT2A(cstrUser)); // hoặc CW2A(cstrUser, CP_UTF8)
    std::string pass = std::string(CT2A(cstrPass)); // hoặc CW2A(cstrPass, CP_UTF8)

    std::string command = std::string("REG ") + user + " " + pass + "\n";


    // Gửi lệnh
    if (!m_client.SendRequest(command)) {
        AfxMessageBox(_T("Mất kết nối tới service."), MB_ICONERROR);
        EndDialog(IDCANCEL);
        return;
    }

    // Chờ nhận phản hồi
    std::string response = m_client.ReceiveResponse();

    // Xử lý phản hồi
    if (response == "REG_OK")
    {
        AfxMessageBox(_T("Đăng ký thành công! Vui lòng đăng nhập."), MB_OK);
        m_isLoginMode = true;
        UpdateUIMode();
    }
    else if (response == "ERR_USER_EXISTS")
    {
        AfxMessageBox(_T("Tên đăng nhập này đã tồn tại."), MB_ICONWARNING);
    }
    else if (response == "ERR_CONNECTION_LOST")
    {
        AfxMessageBox(_T("Mất kết nối tới service."), MB_ICONERROR);
        EndDialog(IDCANCEL);
    }
    else
    {
        CString errorMsg(response.c_str());
        AfxMessageBox(_T("Lỗi không xác định: ") + errorMsg, MB_ICONERROR);
    }
}