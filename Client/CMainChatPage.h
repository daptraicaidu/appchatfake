// CMainChatPage.h
#pragma once
#include "afxdialogex.h"
#include <winsock2.h>
#include <string> // Cần cho bộ đệm
#include <vector> // Cần cho bộ đệm

// === 1. ĐỊNH NGHĨA CÁC MESSAGE TÙY CHỈNH ===
// Các message này dùng để giao tiếp từ Luồng Nhận (Worker Thread)
// về Luồng Giao diện (UI Thread)
#define WM_USER_UPDATE_USERS    (WM_USER + 101) // Cập nhật danh sách user
#define WM_USER_RECV_MSG        (WM_USER + 102) // Nhận tin nhắn mới (RECV)
#define WM_USER_HISTORY_START   (WM_USER + 103) // Bắt đầu load lịch sử
#define WM_USER_HISTORY_MSG     (WM_USER + 104) // Nhận 1 dòng lịch sử (MSG_ME/MSG_THEM)
#define WM_USER_HISTORY_END     (WM_USER + 105) // Kết thúc load lịch sử
#define WM_USER_CONNECTION_LOST (WM_USER + 106) // Mất kết nối
#define WM_USER_UPDATE_STATUS   (WM_USER + 107) // Online/offline
#define WM_USER_DOWN_COMPLETED (WM_USER + 108) // Download file completed

class CMainChatPage : public CDialogEx
{
	DECLARE_DYNAMIC(CMainChatPage)

public:
	CMainChatPage(SOCKET hSocket, CString sUsername, CWnd* pParent = nullptr);
	virtual ~CMainChatPage();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	// --- BIẾN THÀNH VIÊN ---
	SOCKET m_hSocket;            // Socket kết nối đến server
	CWinThread* m_hRecvThread;   // Con trỏ đến đối tượng luồng nhận
	HANDLE m_hStopEvent;         // Event để báo hiệu luồng dừng lại
	CString m_sMyUsername;		 // Tên user của chính mình
	CString m_sCurrentChatUser;  // Tên của user đang chat cùng
	std::string m_sReceiveBuffer;  // Bộ đệm cho dữ liệu nhận không liên tục
	std::string m_sDownloadBuffer; // Bộ đệm cho dữ liệu file download

	// --- HÀM HỖ TRỢ ---

	/**
	 * @brief Gửi một lệnh (đã có \n) qua socket.
	 */
	bool SendSocketCommand(const CString& sCommand);

	/**
	 * @brief Thêm text vào RichEdit với màu sắc cụ thể.
	 */
	void AppendTextToHistory(const CString& sText, COLORREF color);

	/**
	 * @brief Hàm chính của luồng nhận, chạy nền.
	 */
	static UINT __cdecl ReceiverThread(LPVOID pParam);

	/**
	 * @brief Xử lý một dòng lệnh hoàn chỉnh nhận từ server.
	 */
	void ProcessServerCommand(const std::string& sCommand);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog(); // Cần để khởi tạo luồng

	DECLARE_MESSAGE_MAP()

public:
	// --- CONTROLS ---
	CListCtrl m_listUsers;
	CRichEditCtrl m_chatHistory;
	CEdit m_editMessage;
	CButton m_btnSend;

	// --- HANDLERS SỰ KIỆN UI ---
	afx_msg void OnDestroy(); // Cần để dọn dẹp luồng
	afx_msg void OnBnClickedButton1(); // Click nút Send
	afx_msg void OnBnClickedButtonSave(); // Click nút Save Chat History
	afx_msg void OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult); // Click vào List User

	// --- HANDLERS MESSAGE TÙY CHỈNH (TỪ LUỒNG NHẬN) ---
protected:
	afx_msg LRESULT OnUpdateUsers(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReceiveMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHistoryStart(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHistoryMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHistoryEnd(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnConnectionLost(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserStatusUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadCompleted(WPARAM wParam, LPARAM lParam);
};