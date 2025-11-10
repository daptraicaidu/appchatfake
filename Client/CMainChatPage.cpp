// CMainChatPage.cpp : implementation file
//

#include "pch.h"
#include "ChatAppClient.h"
#include "afxdialogex.h"
#include "CMainChatPage.h"


// CMainChatPage dialog

IMPLEMENT_DYNAMIC(CMainChatPage, CDialogEx)

CMainChatPage::CMainChatPage(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{

}

CMainChatPage::~CMainChatPage()
{
}

void CMainChatPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMainChatPage, CDialogEx)
END_MESSAGE_MAP()


// CMainChatPage message handlers
