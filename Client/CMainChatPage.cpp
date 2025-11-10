// CMainChatPage.cpp : implementation file
//

#include "pch.h"
#include "ChatAppClient.h"
#include "afxdialogex.h"
#include "CMainChatPage.h"


// CMainChatPage dialog

IMPLEMENT_DYNAMIC(CMainChatPage, CDialogEx)

CMainChatPage::CMainChatPage(SOCKET hSocket, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent), m_hSocket(hSocket)
{
	
}

CMainChatPage::~CMainChatPage()
{
    if (m_hSocket != INVALID_SOCKET)
    {
        shutdown(m_hSocket, SD_BOTH);
        closesocket(m_hSocket);
        m_hSocket = INVALID_SOCKET;
    }
}

void CMainChatPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMainChatPage, CDialogEx)
END_MESSAGE_MAP()


// CMainChatPage message handlers
