
// ChatAppClientDlg.h : header file
//

#pragma once
#include "TcpClient.h"

// CChatAppClientDlg dialog
class CChatAppClientDlg : public CDialogEx
{
// Construction
public:
	CChatAppClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATAPPCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit2();
	CEdit m_usernameInput;
	CEdit m_passwordInput;
	CButton m_regButton;
	CButton m_logButton;
	CButton m_changeMode;
	CStatic m_modeStatic;
	CStatic m_errorNetwork;

	// Biến quản lý kết nối
	TcpClient m_client;

	// Biến trạng thái
	bool m_isLoginMode;

	// Hàm public để lấy socket sau khi đăng nhập thành công
	SOCKET GetClientSocket() const { return m_client.GetSocket(); }

protected:
	// Hàm trợ giúp để cập nhật UI
	void UpdateUIMode();

	// Hàm xử lý sự kiện click
	afx_msg void OnBnClickedChangeMode();
	afx_msg void OnBnClickedLogButton();
	afx_msg void OnBnClickedRegButton();
	

};
