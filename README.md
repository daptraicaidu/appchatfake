Đây là 2 phần code, những file code quan trọng mà tôi có và cần thiết để bạn có thể hiểu, có một số chỗ code tôi có note lại // là tôi tạm thời không sử dụng và nó không gây lỗi cho nên bạn có thể skip nếu nó không lỗi, còn nếu đoạn nào mà gây lỗi thì cứ nhắc tôi nhé, bạn chỉ cần tập trung vào yêu cầu của tôi, hướng dẫn sửa chi tiết hoặc tạo code mới một cách cẩn thận là được, không sửa code cũ lung tung, sửa thì phải hướng dẫn chi tiết và sao lại sửa chỗ đó



Cau truc Database
CREATE TABLE IF NOT EXISTS Users (

&nbsp;   UserId        INTEGER PRIMARY KEY AUTOINCREMENT,

&nbsp;   Username      TEXT UNIQUE NOT NULL,

&nbsp;   PasswordHash  TEXT NOT NULL,

&nbsp;   CreateDate    DATETIME NOT NULL DEFAULT (CURRENT\_TIMESTAMP),

);



CREATE TABLE IF NOT EXISTS Messages (

&nbsp;   MessageId     INTEGER PRIMARY KEY AUTOINCREMENT,

&nbsp;   SenderId      INTEGER NOT NULL,

&nbsp;   ReceiverId    INTEGER NOT NULL,

&nbsp;   Content       TEXT NOT NULL,

&nbsp;   SentDate      DATETIME NOT NULL DEFAULT (CURRENT\_TIMESTAMP),

&nbsp;   IsDelivered   INTEGER NOT NULL DEFAULT 0,

&nbsp;   DeliveredDate DATETIME,

&nbsp;   FOREIGN KEY (SenderId)  REFERENCES Users(UserId),

&nbsp;   FOREIGN KEY (ReceiverId) REFERENCES Users(UserId)

);

CREATE INDEX IF NOT EXISTS IX\_Messages\_Recv\_Deliv ON Messages(ReceiverId, IsDelivered);

