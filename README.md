Đây là 2 phần code, những file code quan trọng mà tôi có và cần thiết để bạn có thể hiểu, có một số chỗ code tôi có note lại // là tôi tạm thời không sử dụng và nó không gây lỗi cho nên bạn có thể skip nếu nó không lỗi, còn nếu đoạn nào mà gây lỗi thì cứ nhắc tôi nhé, bạn chỉ cần tập trung vào yêu cầu của tôi, hướng dẫn sửa chi tiết hoặc tạo code mới một cách cẩn thận là được, không sửa code cũ lung tung, sửa thì phải hướng dẫn chi tiết và sao lại sửa chỗ đó

Lưu ý: Cần code theo thư viện của winapi, tránh dùng các thư viện quá hiện đại(trừ một số trường hợp dùng thư viện của winapi mà phức tạp quá thì thôi, có thể thảo luận với tôi trước nếu bạn định dùng thư viện hiện đại).


Cấu trúc Database

CREATE TABLE IF NOT EXISTS Users (

    UserId        INTEGER PRIMARY KEY AUTOINCREMENT,

    Username      TEXT UNIQUE NOT NULL,

    PasswordHash  TEXT NOT NULL,

    CreateDate    DATETIME NOT NULL DEFAULT (CURRENT_TIMESTAMP),

);

CREATE TABLE IF NOT EXISTS Messages (

    MessageId     INTEGER PRIMARY KEY AUTOINCREMENT,

    SenderId      INTEGER NOT NULL,

    ReceiverId    INTEGER NOT NULL,

    Content       TEXT NOT NULL,

    SentDate      DATETIME NOT NULL DEFAULT (CURRENT_TIMESTAMP),

    FOREIGN KEY (SenderId)  REFERENCES Users(UserId),

    FOREIGN KEY (ReceiverId) REFERENCES Users(UserId)

);

CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username

ON Users(Username);

CREATE INDEX IF NOT EXISTS idx_messages_pair

ON Messages(SenderId, ReceiverId, SentDate);


