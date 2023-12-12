#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "sendMail.h"

#define IP "127.0.0.1"
#define CRLF "\r\n"

struct Mail
{
	char From[256] = { 0 };
	vector<char*> To;
	vector<char*> Bcc;
	vector<char*> Cc;
	char subject[100] = { 0 };
	char content[1000] = { 0 };
};

char* Input_Content(char* content)
{
	if (content == nullptr) return nullptr;
	int length = 0;
	char ch = 0;
	do {
		scanf("%c", &ch);
		if (length >= 1 && content[length - 1] == '\n' && ch == '\n') break;
		content[length] = ch;
		length++;
	} while (length < 999);
	content[length] = 0;
	return content;
}

void Input_Mail(Mail& mail) // Khi dùng xong phải gọi Free_Mail()
{
	strcpy(mail.From, "lqv@student.hcmus.edu.vn"); // Lấy từ user
	char read[1000] = { 0 };
	char* token = nullptr;
	printf("To: ");
	scanf("%[^\n]s", read);
	token = strtok(read, ",; ");
	while (token)
	{
		mail.To.push_back(_strdup(token)); // Hàm cấp phát cần giải phóng
		token = strtok(nullptr, ", ");
	}
	memset(read, 0, sizeof(read));
	printf("Bcc: ");
	getchar();
	scanf("%[^\n]s", read);
	token = strtok(read, ", ");
	while (token)
	{
		mail.Bcc.push_back(_strdup(token)); // Hàm cấp phát cần giải phóng
		token = strtok(nullptr, ", ");
	}
	memset(read, 0, sizeof(read));
	printf("Cc: ");
	getchar();
	scanf("%[^\n]s", read);
	token = strtok(read, ", ");
	while (token)
	{
		mail.Cc.push_back(_strdup(token)); // Hàm cấp phát cần giải phóng
		token = strtok(nullptr, ", ");
	}
	memset(read, 0, sizeof(read));
	printf("Subject: ");
	getchar();
	scanf("%[^\n]s", mail.subject);
	printf("Content:\n");
	getchar();
	Input_Content(mail.content);
}

void Free_Mail(Mail& mail)
{
	for (char* str : mail.To)
	{
		free(str);
	}
	for (char* str : mail.Bcc)
	{
		free(str);
	}
	for (char* str : mail.Cc)
	{
		free(str);
	}
	mail = { 0 };
}

bool sendMail(const Mail* mail)
{
	if (mail == nullptr) return false;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cerr << "Failed to initialize Winsock.\n";
		return false;
	}

	// Tạo socket máy khách
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		cerr << "Failed to create client socket.\n";
		WSACleanup();
		return false;
	}

	// Thiết lập địa chỉ máy chủ
	sockaddr_in serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(IP);  // Địa chỉ máy chủ
	serverAddress.sin_port = htons(2225);  // Port 2225

	// Kết nối đến máy chủ
	if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		cerr << "Failed to connect to the server.\n";
		closesocket(clientSocket);
		WSACleanup();
		return false;
	}

	//Tin nhắn MIME
	MIME2::CONTENT c;
	auto data = c.SerializeToVector();

	c["Message-ID"] = mail->From;
	time_t cur_time = time(NULL);
	c["Date"] = ctime(&cur_time);
	

	c["MIME-Version"] = "1.0";
	c["User-Agent"] = "Fake Thunderbird";
	c["Content-Language"] = "en-US";

	{
		string temp = "";
		for (char* str : mail->To)
		{
			temp = temp + str + "; ";
		}
		c["To"] = temp.c_str();
	}

	{
		string temp = "";
		for (char* str : mail->Bcc)
		{
			temp = temp + str + "; ";
		}
		c["Bcc"] = temp.c_str();
	}
	{
		string temp = "";
		for (char* str : mail->Cc)
		{
			temp = temp + str + "; ";
		}
		c["Cc"] = temp.c_str();
	}

	c["From"] = mail->From; // sẽ thêm tên phía trước;
	c["Subject"] = mail->subject;
	c["Content-Type"] = "application/octet-stream";
	c["Content-Transfer-Encoding"] = "base64";


	
	
	c["Content-Disposition"] = "attachment; filename=\"hello.txt\"";
	/*string out = MIME2::Char2Base64("Hello", 5);*/
	c.SetData(mail->content);
	data = c.SerializeToVector();

	


	/*
	MIME-Version: 1.0
	Content-Type: application/octet-stream
	Content-Transfer-Encoding: base64
	Content-Disposition: attachment; filename="hello.txt"

	SGVsbG8=
	*/

	//Gửi dữ liệu đến máy chủ

	/*Gửi EHLO*/
	char message[100] = ""; //message: thông điệp gửi đi.
	char buffer[4096] = ""; //buffer: tạm lưu phản hồi Server.
	sprintf(message, "EHLO [%s]\r\n", IP);
	send(clientSocket, message, (int)strlen(message), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);

	/*Gửi from*/
	sprintf(message, "MAIL FROM:<%s>\r\n", mail->From);
	send(clientSocket, message, (int)strlen(message), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);
	/*Gửi to*/
	for (char* to_element : mail->To)
	{
		sprintf(message, "RCPT TO:<%s>\r\n", to_element);
		send(clientSocket, message, (int)strlen(message), 0);
		recv(clientSocket, buffer, sizeof(buffer), 0);
	}
	/*Gửi Data*/
	sprintf(message, "DATA\r\n");
	send(clientSocket, message, (int)strlen(message), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);

	auto msg = c.SerializeToVector(); //chuyển MIME -> vector<char>

	char* willsend = new char[msg.size() + 1]; //Client sẽ gửi willsend - MIME qua chuyển đổi char sang server, +1 để chứa \0
	for (int i = 0; i < msg.size(); i++)
		willsend[i] = msg[i];
	willsend[msg.size()] = '\0';
	//cout << willsend << endl;

	send(clientSocket, willsend, (int)strlen(willsend), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);

	delete[] willsend;


	sprintf(message, "\r\n.\r\n");
	send(clientSocket, message, (int)strlen(message), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);

	sprintf(message, "QUIT\r\n");
	send(clientSocket, message, (int)strlen(message), 0);
	recv(clientSocket, buffer, sizeof(buffer), 0);
	

	// Đóng kết nối
	closesocket(clientSocket);
	WSACleanup();
	return true;
}
int main()
{
	Mail mail = { 0 };
	Input_Mail(mail);
	sendMail(&mail);

	Free_Mail(mail);
}
