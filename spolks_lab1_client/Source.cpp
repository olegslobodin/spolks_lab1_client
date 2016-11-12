#include"Header.h"

void main()
{
	int socket;

	ConfirmWinSocksDll();
	InitClientSocket(&socket);
	Connect(socket, "127.0.0.1");
	Send("time\n", socket);
	Send("echo ooo\n", socket);
	Send("close\n", socket);
	system("pause");
}

int ConfirmWinSocksDll()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		printf("ERROR: WSAStartup failed with error: %d\n", err);
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("ERROR: Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return 1;
	}

	return 0;
}

void PrintLastError()
{
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	wcout << buf;
}

void InitClientSocket(int* clientSocket)
{
	*clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*clientSocket == SOCKET_ERROR)
	{
		PrintLastError();
		return;
	}
}

void Connect(int socket, string ip)
{
	sockaddr_in clientSocketParams;
	clientSocketParams.sin_port = htons(80);
	clientSocketParams.sin_family = AF_INET;
	clientSocketParams.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	connect(socket, (struct sockaddr *)&clientSocketParams, sizeof(clientSocketParams));
}

void Send(string str, int socket)
{
	int bufSize = str.length();
	char* buf = (char*)calloc(bufSize + 1, 1);
	strcpy(buf, str.c_str());
	if (send(socket, buf, bufSize, MSG_DONTROUTE) == -1)
		PrintLastError();
	free(buf);
}