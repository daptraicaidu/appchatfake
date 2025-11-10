#pragma once
#include "afxdialogex.h"


// CMainChatPage dialog

class CMainChatPage : public CDialogEx
{
	DECLARE_DYNAMIC(CMainChatPage)

public:
	CMainChatPage(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CMainChatPage();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
