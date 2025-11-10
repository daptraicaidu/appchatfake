#pragma once
#include "afxdialogex.h"
#include <winsock2.h>

// CMainChatPage dialog

class CMainChatPage : public CDialogEx
{
	DECLARE_DYNAMIC(CMainChatPage)

public:
	CMainChatPage(SOCKET hSocket, CWnd* pParent = nullptr);
	virtual ~CMainChatPage();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	SOCKET m_hSocket;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
