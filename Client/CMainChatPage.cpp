// CMainChatPage.cpp : implementation file
//

#include "pch.h"
#include "ChatAppClient.h"
#include "afxdialogex.h"
#include "CMainChatPage.h"
#include <sstream> // Dùng để phân tích chuỗi

// CMainChatPage dialog

IMPLEMENT_DYNAMIC(CMainChatPage, CDialogEx)

CMainChatPage::CMainChatPage(SOCKET hSocket, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent),
	m_hSocket(hSocket),         // Lưu socket
	m_hRecvThread(NULL),      // Khởi tạo Handle luồng
	m_hStopEvent(NULL),       // Khởi tạo Event
	m_sCurrentChatUser(_T("")) // Chưa chọn ai
{

}

CMainChatPage::~CMainChatPage()
{

}

void CMainChatPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listUsers);
	DDX_Control(pDX, IDC_RICHEDIT21, m_chatHistory);
	DDX_Control(pDX, IDC_EDIT1, m_editMessage);
	DDX_Control(pDX, IDC_BUTTON1, m_btnSend);
}


BEGIN_MESSAGE_MAP(CMainChatPage, CDialogEx)
	// --- Map các sự kiện UI ---
	ON_WM_DESTROY() // Bắt sự kiện đóng cửa sổ
	ON_BN_CLICKED(IDC_BUTTON1, &CMainChatPage::OnBnClickedButton1) // Nút Send
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CMainChatPage::OnLvnItemchangedList1) // Click List User

	// --- Map các Message tùy chỉnh từ luồng ---
	ON_MESSAGE(WM_USER_UPDATE_USERS, &CMainChatPage::OnUpdateUsers)
	ON_MESSAGE(WM_USER_RECV_MSG, &CMainChatPage::OnReceiveMessage)
	ON_MESSAGE(WM_USER_HISTORY_START, &CMainChatPage::OnHistoryStart)
	ON_MESSAGE(WM_USER_HISTORY_MSG, &CMainChatPage::OnHistoryMessage)
	ON_MESSAGE(WM_USER_HISTORY_END, &CMainChatPage::OnHistoryEnd)
	ON_MESSAGE(WM_USER_CONNECTION_LOST, &CMainChatPage::OnConnectionLost)
END_MESSAGE_MAP()

#ifndef EM_SETTYPOGRAPHYOPTIONS
#define EM_SETTYPOGRAPHYOPTIONS (WM_USER + 202)
#endif
#ifndef TO_ADVANCEDTYPOGRAPHY
#define TO_ADVANCEDTYPOGRAPHY 1
#endif
// CMainChatPage message handlers

BOOL CMainChatPage::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(_T("Main Chat"));
	// --- 1. Cấu hình List Control ---
	// Thiết lập kiểu xem "Report" và thêm cột "Username"
	m_listUsers.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listUsers.InsertColumn(0, _T("Username"), LVCFMT_LEFT, 200);

	// --- 2. Khởi tạo sự kiện Stop ---
	// CreateEvent (NULL, TRUE, FALSE, NULL)
	//   TRUE: Manual reset (chúng ta phải tự reset)
	//   FALSE: Ban đầu ở trạng thái non-signaled (chưa stop)
	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!m_hStopEvent)
	{
		AfxMessageBox(_T("Lỗi nghiêm trọng: Không thể tạo Stop Event!"), MB_ICONERROR);
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// --- 3. Khởi động Luồng Nhận (Receiver Thread) ---
	// Chúng ta dùng AfxBeginThread an toàn hơn CreateThread
	m_hRecvThread = AfxBeginThread(ReceiverThread, this);
	if (!m_hRecvThread)
	{
		AfxMessageBox(_T("Lỗi nghiêm trọng: Không thể tạo Luồng Nhận!"), MB_ICONERROR);
		EndDialog(IDCANCEL);
		return TRUE;
	}

	// --- 4. Gửi yêu cầu đầu tiên: Lấy danh sách Users ---
	// Luồng nhận sẽ xử lý phản hồi "USERS_LIST ..."
	SendSocketCommand(_T("GET_USERS"));



	// Thiết lập font mặc định cho khung chat
	m_chatHistory.SendMessage(EM_SETTYPOGRAPHYOPTIONS, TO_ADVANCEDTYPOGRAPHY, TO_ADVANCEDTYPOGRAPHY);
	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);
	cf.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;

	_tcscpy_s(cf.szFaceName, _T("Segoe UI Emoji"));
	cf.yHeight = 220;
	cf.bCharSet = DEFAULT_CHARSET;

	m_chatHistory.SetDefaultCharFormat((CHARFORMAT&)cf);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMainChatPage::OnDestroy()
{
	// Đây là hàm dọn dẹp QUAN TRỌNG
	CDialogEx::OnDestroy();

	// --- 1. Báo hiệu cho Luồng Nhận dừng lại ---
	if (m_hStopEvent)
	{
		SetEvent(m_hStopEvent);
	}

	// --- 2. Đóng Socket ---
	// Việc này sẽ làm cho hàm recv() trong luồng nhận bị lỗi (SOCKET_ERROR)
	// và luồng sẽ thoát ra ngay lập tức (thay vì bị block mãi mãi).
	if (m_hSocket != INVALID_SOCKET)
	{
		shutdown(m_hSocket, SD_BOTH);
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	// --- 3. Chờ Luồng Nhận kết thúc ---
	// Chờ tối đa 3 giây, nếu không thì tự thoát
	if (m_hRecvThread)
	{
		WaitForSingleObject(m_hRecvThread->m_hThread, 3000);
		// Không cần CloseHandle(m_hRecvThread) vì AfxBeginThread tự quản lý
	}

	// --- 4. Đóng Handle của Event ---
	if (m_hStopEvent)
	{
		CloseHandle(m_hStopEvent);
	}
}

// =========================================================================
// HÀM GỬI VÀ CÁC HÀM HỖ TRỢ
// =========================================================================

/**
 * @brief Gửi một lệnh qua socket. Thêm \n vào cuối.
 */
bool CMainChatPage::SendSocketCommand(const CString& sCommand)
{
	if (m_hSocket == INVALID_SOCKET) return false;

	std::wstring wCmd(sCommand); // Chắc chắn là wide string

	// Chuyển đổi sang UTF-8 thủ công để đảm bảo nhất hoặc dùng CT2A(sCommand, CP_UTF8)
	// Cách dùng CT2A với CP_UTF8:
	CW2A utf8Cmd(sCommand, CP_UTF8);
	std::string cmd = std::string(utf8Cmd) + "\n";

	int sendResult = send(m_hSocket, cmd.c_str(), (int)cmd.length(), 0);

	if (sendResult == SOCKET_ERROR)
	{
		// Nếu gửi bị lỗi, coi như mất kết nối
		PostMessage(WM_USER_CONNECTION_LOST);
		return false;
	}
	return true;
}

/**
 * @brief Thêm text vào RichEdit với màu sắc.
 */
 /**
  * @brief Thêm text vào RichEdit với màu sắc. (ĐÃ SỬA LỖI)
  */
// File: Client/CMainChatPage.cpp

void CMainChatPage::AppendTextToHistory(const CString& sText, COLORREF color)
{
	long nStart, nEnd;

	// 1. Di chuyển con trỏ xuống cuối
	m_chatHistory.SetSel(-1, -1);
	m_chatHistory.GetSel(nStart, nEnd);

	// 2. Thêm text và xuống dòng
	m_chatHistory.ReplaceSel(sText + _T("\r\n"));

	// 3. Chuẩn bị định dạng
	CHARFORMAT2 cf; // Dùng CHARFORMAT2 để hỗ trợ tốt hơn nếu có
	memset(&cf, 0, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);

	// --- SỬA ĐỔI QUAN TRỌNG ---
	// Thêm cờ CFM_CHARSET
	cf.dwMask = CFM_COLOR | CFM_EFFECTS | CFM_FACE | CFM_CHARSET;

	cf.crTextColor = color;
	cf.dwEffects = 0;
	cf.bCharSet = DEFAULT_CHARSET; // Để Windows tự chọn bảng mã phù hợp (bao gồm tiếng Việt)

	
	_tcscpy_s(cf.szFaceName, _T("Segoe UI Emoji"));

	// 4. Chọn lại đoạn text vừa thêm
	long nNewEnd = nStart + sText.GetLength();
	m_chatHistory.SetSel(nStart, nNewEnd);

	// 5. Áp dụng định dạng
	m_chatHistory.SetSelectionCharFormat((CHARFORMAT&)cf);

	// 6. Cuộn xuống cuối và bỏ chọn
	m_chatHistory.SetSel(-1, -1);
	m_chatHistory.PostMessage(EM_SCROLLCARET, 0, 0);
}

// =========================================================================
// LUỒNG NHẬN (WORKER THREAD)
// =========================================================================

UINT __cdecl CMainChatPage::ReceiverThread(LPVOID pParam)
{
	// Lấy con trỏ đến cửa sổ Dialog
	CMainChatPage* pPage = static_cast<CMainChatPage*>(pParam);
	SOCKET hSocket = pPage->m_hSocket;
	HANDLE hStopEvent = pPage->m_hStopEvent;

	char recvBuf[2048];
	int recvResult;

	// Vòng lặp chính của Luồng Nhận
	while (true)
	{
		// Kiểm tra tín hiệu Stop trước mỗi lần nhận
		if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
		{
			// Nhận được lệnh Stop -> Thoát luồng
			break;
		}

		// Chờ nhận dữ liệu (Blocking)
		// Khi socket bị đóng ở OnDestroy(), hàm này sẽ trả về <= 0
		recvResult = recv(hSocket, recvBuf, sizeof(recvBuf), 0);

		if (recvResult > 0)
		{
			// --- Có dữ liệu ---
			// Thêm dữ liệu vào bộ đệm (m_sReceiveBuffer)
			pPage->m_sReceiveBuffer.append(recvBuf, recvResult);

			// Xử lý bộ đệm, tìm các lệnh hoàn chỉnh (kết thúc bằng \n)
			size_t pos;
			// Vòng lặp
			while ((pos = pPage->m_sReceiveBuffer.find('\n')) != std::string::npos)
			{
				// Lấy ra một lệnh
				std::string cmd = pPage->m_sReceiveBuffer.substr(0, pos);
				// Xóa lệnh đó khỏi bộ đệm
				pPage->m_sReceiveBuffer.erase(0, pos + 1);

				// Xóa \r nếu có
				if (!cmd.empty() && cmd.back() == '\r') {
					cmd.pop_back();
				}

				// Gửi lệnh này về UI Thread để xử lý
				pPage->ProcessServerCommand(cmd);
			}
		}
		else
		{
			// --- Lỗi hoặc Server ngắt kết nối ---
			// recvResult == 0: Server đóng kết nối (Gracefully)
			// recvResult == SOCKET_ERROR: Lỗi (có thể do ta tự đóng socket ở OnDestroy)

			// Kiểm tra xem có phải do ta chủ động Stop không
			if (WaitForSingleObject(hStopEvent, 0) != WAIT_OBJECT_0)
			{
				// Nếu không phải do ta stop -> Đây là lỗi thật
				// Báo về UI Thread
				pPage->PostMessage(WM_USER_CONNECTION_LOST);
			}
			// Thoát luồng
			break;
		}
	}

	// Kết thúc luồng
	return 0;
}

/**
 * @brief (Chạy trên Luồng Nhận) Phân tích lệnh và PostMessage về UI Thread.
 */
void CMainChatPage::ProcessServerCommand(const std::string& sCommand)
{
	if (sCommand.empty()) return;

	// Chuyển sang CString để dễ làm việc với UI
	CA2W unicodeCmd(sCommand.c_str(), CP_UTF8);
	CString sCmdLine(unicodeCmd);

	// Tách lệnh và nội dung
	std::stringstream ss(sCommand);
	std::string command;
	ss >> command;

	// Dùng CString::Tokenize để lấy phần còn lại (an toàn hơn)
	int nTokenPos = sCmdLine.Find(_T(' '));
	CString sData = (nTokenPos == -1) ? _T("") : sCmdLine.Mid(nTokenPos + 1);

	// Phân loại lệnh và PostMessage tương ứng
	// Lưu ý: Tuyệt đối KHÔNG cập nhật UI từ đây (vì đây là Worker Thread)

	if (command == "USERS_LIST")
	{
		// Gửi toàn bộ data (danh sách user) về UI thread
		// Dùng CString::AllocSysString để tạo bản sao
		PostMessage(WM_USER_UPDATE_USERS, 0, (LPARAM)sData.AllocSysString());
	}
	else if (command == "RECV")
	{
		// Định dạng: RECV <sender_username> <Nội dung>
		// sData = "<sender_username> <Nội dung>"
		// Chúng ta sẽ gửi sData về, UI thread sẽ tự phân tích
		PostMessage(WM_USER_RECV_MSG, 0, (LPARAM)sData.AllocSysString());
	}
	else if (command == "HISTORY_START")
	{
		PostMessage(WM_USER_HISTORY_START);
	}
	else if (command == "MSG_ME" || command == "MSG_THEM")
	{
		// Gửi nội dung tin nhắn (sData)
		// và loại tin nhắn (wParam)
		WPARAM type = (command == "MSG_ME") ? 1 : 0; // 1 = ME, 0 = THEM
		PostMessage(WM_USER_HISTORY_MSG, type, (LPARAM)sData.AllocSysString());
	}
	else if (command == "HISTORY_END")
	{
		PostMessage(WM_USER_HISTORY_END);
	}
	// TODO: Xử lý các lệnh lỗi từ server (SEND_OK, ERR_...) nếu cần
}


// =========================================================================
// HANDLERS SỰ KIỆN UI (Chạy trên UI Thread)
// =========================================================================

/**
 * @brief Xử lý sự kiện click nút Send (IDC_BUTTON1).
 */
void CMainChatPage::OnBnClickedButton1()
{
	// --- 1. Kiểm tra đã chọn người nhận chưa ---
	// m_sCurrentChatUser được cập nhật khi click vào List
	if (m_sCurrentChatUser.IsEmpty())
	{
		AfxMessageBox(_T("Vui lòng chọn một người dùng từ danh sách để bắt đầu chat."), MB_ICONWARNING);
		return;
	}

	// --- 2. Lấy nội dung tin nhắn ---
	CString sMessage;
	m_editMessage.GetWindowText(sMessage);
	sMessage.Trim(); // Xóa khoảng trắng thừa

	if (sMessage.IsEmpty())
	{
		return; // Không gửi tin rỗng
	}

	// --- 3. Tạo lệnh SEND ---
	// Định dạng: SEND <recipient_username> <Nội dung>
	CString sCommand;
	sCommand.Format(_T("SEND %s %s"), m_sCurrentChatUser, sMessage);

	// --- 4. Gửi lệnh ---
	if (SendSocketCommand(sCommand))
	{
		// --- 5. Hiển thị "local echo" ---
		// Hiển thị ngay tin nhắn của mình lên cửa sổ chat (màu đỏ)
		// mà không cần chờ phản hồi SEND_OK từ server
		AppendTextToHistory(_T("You: ") + sMessage, RGB(255, 0, 0)); // Màu đỏ

		// --- 6. Xóa nội dung đã gõ ---
		m_editMessage.SetWindowText(_T(""));
	}
	else
	{
		AfxMessageBox(_T("Lỗi gửi tin nhắn. Kết nối có thể đã mất."), MB_ICONERROR);
	}
}

/**
 * @brief Xử lý sự kiện click (hoặc thay đổi) item trên List User (IDC_LIST1).
 */
void CMainChatPage::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	// Chỉ xử lý khi item được "chọn" (selected)
	if ((pNMLV->uNewState & LVIS_SELECTED) && !(pNMLV->uOldState & LVIS_SELECTED))
	{
		// Lấy vị trí item được chọn
		int nItem = pNMLV->iItem;
		if (nItem == -1) return;

		// Lấy tên user từ item đó
		CString sSelectedUser = m_listUsers.GetItemText(nItem, 0);

		// --- 1. Cập nhật user đang chat ---
		m_sCurrentChatUser = sSelectedUser;

		// --- 2. Gửi lệnh yêu cầu lịch sử ---
		CString sCommand;
		sCommand.Format(_T("GET_HISTORY %s"), m_sCurrentChatUser);

		SendSocketCommand(sCommand);

		// Luồng nhận sẽ nhận các gói tin HISTORY_START, MSG_ME, ...
		// và gửi message về UI thread để xử lý (xem OnHistoryStart, OnHistoryMessage)
	}
}


// =========================================================================
// HANDLERS MESSAGE TÙY CHỈNH (Chạy trên UI Thread)
// =========================================================================

/**
 * @brief (UI Thread) Xử lý message WM_USER_UPDATE_USERS.
 */
LRESULT CMainChatPage::OnUpdateUsers(WPARAM wParam, LPARAM lParam)
{
	// lparam là con trỏ CString, phải giải phóng
	CString sData = (LPCTSTR)lParam;

	m_listUsers.DeleteAllItems(); // Xóa sạch danh sách cũ

	// sData có dạng: "user1 user2 user3"
	// Tách chuỗi bằng Tokenize
	int nPos = 0;
	CString sUser = sData.Tokenize(_T(" "), nPos);
	int nItem = 0;
	while (!sUser.IsEmpty())
	{
		m_listUsers.InsertItem(nItem++, sUser);
		sUser = sData.Tokenize(_T(" "), nPos);
	}

	// Giải phóng CString đã được cấp phát ở luồng kia
	SysFreeString((BSTR)lParam);
	return 0;
}

/**
 * @brief (UI Thread) Xử lý message WM_USER_RECV_MSG.
 */
LRESULT CMainChatPage::OnReceiveMessage(WPARAM wParam, LPARAM lParam)
{
	CString sData = (LPCTSTR)lParam; // "sender_username <Nội dung>"

	// Tách Tên người gửi và Nội dung
	int nPos = sData.Find(_T(' '));
	if (nPos != -1)
	{
		CString sSender = sData.Left(nPos);
		CString sMessage = sData.Mid(nPos + 1);

		// --- KIỂM TRA QUAN TRỌNG ---
		// Chỉ hiển thị tin nhắn nếu ta đang chat với người đó
		if (sSender == m_sCurrentChatUser)
		{
			// Hiển thị màu xanh
			CString sDisplay;
			sDisplay.Format(_T("%s: %s"), sSender, sMessage);
			AppendTextToHistory(sDisplay, RGB(0, 0, 255));
		}
		else
		{
			// TODO: Báo hiệu có tin nhắn mới từ người khác
			// (Ví dụ: in đậm tên user đó trong m_listUsers)
			// Tạm thời bỏ qua
		}
	}

	SysFreeString((BSTR)lParam);
	return 0;
}

/**
 * @brief (UI Thread) Bắt đầu load lịch sử, xóa chat cũ.
 */
LRESULT CMainChatPage::OnHistoryStart(WPARAM wParam, LPARAM lParam)
{
	m_chatHistory.SetWindowText(_T("")); // Xóa sạch lịch sử cũ
	return 0;
}

/**
 * @brief (UI Thread) Thêm 1 dòng lịch sử.
 */
LRESULT CMainChatPage::OnHistoryMessage(WPARAM wParam, LPARAM lParam)
{
	CString sMessage = (LPCTSTR)lParam;
	bool bIsMe = (wParam == 1); // 1 = ME, 0 = THEM

	if (bIsMe)
	{
		AppendTextToHistory(_T("You: ") + sMessage, RGB(255, 0, 0));
	}
	else
	{
		// Tin nhắn của đối phương (m_sCurrentChatUser)
		CString sDisplay;
		sDisplay.Format(_T("%s: %s"), m_sCurrentChatUser, sMessage);
		AppendTextToHistory(sDisplay, RGB(0, 0, 255));
	}

	SysFreeString((BSTR)lParam);
	return 0;
}

/**
 * @brief (UI Thread) Kết thúc load lịch sử.
 */
LRESULT CMainChatPage::OnHistoryEnd(WPARAM wParam, LPARAM lParam)
{
	// Có thể thêm 1 dòng "Hết lịch sử chat" nếu muốn
	return 0;
}

/**
 * @brief (UI Thread) Mất kết nối.
 */
LRESULT CMainChatPage::OnConnectionLost(WPARAM wParam, LPARAM lParam)
{
	// Vô hiệu hóa UI
	m_editMessage.EnableWindow(FALSE);
	m_btnSend.EnableWindow(FALSE);
	m_listUsers.EnableWindow(FALSE);

	AfxMessageBox(_T("Đã mất kết nối đến server. Vui lòng khởi động lại ứng dụng."), MB_ICONERROR);

	// Tự động đóng cửa sổ chat
	EndDialog(IDCANCEL);

	return 0;
}