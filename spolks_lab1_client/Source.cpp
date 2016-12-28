#include <stdlib.h>
#include "Header.h"

const int BUFFER_SIZE = 1000;
const int PACKAGE_NUMBER_SIZE = 11;
const int FILE_BUFFER_SIZE = 6 * 1024 - PACKAGE_NUMBER_SIZE;
const int TIMEOUT_MS = 10;
unsigned long outPackageNumber = 0;
char inPackageNumberStr[PACKAGE_NUMBER_SIZE];
char outPackageNumberStr[PACKAGE_NUMBER_SIZE];
const char *END_OF_FILE = "File sent 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *FILE_SEND_ERROR = "File send error 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *FILE_NOT_FOUND = "File is not found 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *DEFAULT_FILE_PATH_WINDOWS = "../debug/files/";
const int MAX_FLAG_STRING_LENGTH = 60;
const bool UDP = true;
string defaultIp = "127.0.0.1";

int main() {
	int socket;

#if  defined _WIN32 || defined _WIN64
	ConfirmWinSocksDll();
#endif

	InitClientSocket(&socket);
	Work(&socket);

#if  defined _WIN32 || defined _WIN64
	system("pause");
#endif
	return 0;
}

#if  defined _WIN32 || defined _WIN64
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
#endif

void InitClientSocket(int *clientSocket) {
	if (UDP)
		*clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	else
		*clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*clientSocket == SOCKET_ERROR) {
		PrintLastError();
		return;
	}
}

void Work(int *socket) {
	string cmd;
	char *buffer = (char *)calloc(BUFFER_SIZE, 1);

	while (true) {
		getline(cin, cmd);
		if ((UDP && cmd.find("default") == 0) || (!UDP && cmd.find("connect") == 0)) {
			vector<string> words = split(cmd, ' ');
			if (words.size() > 1) {
				if (ConnectOrSetIp(socket, words[1]) == SOCKET_ERROR) {
					cout << "Connection failed\n";
					PrintLastError();
					continue;
				}
				if (cmd.find("default") == 0)
					continue;
			}
		}
		else if (cmd.find("send") == 0) {
			vector<string> words = split(cmd, ' ');
			if (words.size() > 1) {
				SendFile(*socket, words[1]);
				continue;
			}
			else {
				cout << "Please specify a file name";
			}
		}
		else if (cmd.find("receive") == 0) {
			vector<string> words = split(cmd, ' ');
			if (words.size() > 2) {
				ReceiveFile(*socket, words[1], words[2]);
				continue;
			}
			else if (words.size() > 1) {
				ReceiveFile(*socket, words[1], DEFAULT_FILE_PATH_WINDOWS);
				continue;
			}
			else {
				cout << "Please specify a file name";
			}
		}
		else if (cmd.length() == 0)
			continue;
		else if (SendString(cmd + "\n", *socket) == SOCKET_ERROR)
			continue;
		string answer = ReceiveAnswer(*socket, buffer);
		cout << answer;
	}

	free(buffer);
}

sockaddr_in GetSocketParamsByIp(string ip)
{
	sockaddr_in socketParams;
	socketParams.sin_port = htons(10000);
	socketParams.sin_family = AF_INET;

#if defined _WIN32 || defined _WIN64
	socketParams.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
#elif defined __linux__
	clientSocketParams.sin_addr.s_addr = inet_addr(ip.c_str());
#endif

	return socketParams;
}

int ConnectOrSetIp(int *socket, string ip) {
	if (UDP) {
		defaultIp = ip;
		return 0;
	}
	else {
		sockaddr_in clientSocketParams = GetSocketParamsByIp(ip);
		return connect(*socket, (struct sockaddr *) &clientSocketParams, sizeof(clientSocketParams));
	}
}

void SendFile(int socket, string path)
{
	ifstream file;
	file.open(path, ios::binary);
	if (file.is_open()) {
		string fileName = GetFileNameFromPath(path);
		SendString("send " + fileName + "\n", socket);
	}
	else {
		file.open(DEFAULT_FILE_PATH_WINDOWS + path, ios::binary);
		if (file.is_open()) {
			string fileName = GetFileNameFromPath(DEFAULT_FILE_PATH_WINDOWS + path);
			SendString("send " + fileName + "\n", socket);
		}
		else {
			cout << "Can't open the file " << path << ". Error #" << errno << endl;
			return;
		}
	}

	char *cmdBuffer = (char*)calloc(1000, 1);
	string answer = ReceiveAnswer(socket, cmdBuffer);
	if (answer.find("ready") == string::npos) {
		cout << answer;
		free(cmdBuffer);
		return;
	}

	char *fileBuffer = (char*)calloc(FILE_BUFFER_SIZE + PACKAGE_NUMBER_SIZE, 1);
	unsigned long long pos = 0, length = 0;
	file.seekg(0, ios::end);
	unsigned long long fileSize = file.tellg();
	file.seekg(0, ios::beg);

	do {
		if (fileSize - pos < FILE_BUFFER_SIZE)
			length = fileSize - pos;
		else
			length = FILE_BUFFER_SIZE;
		file.read(fileBuffer, length);
		cout << "\r" << file.tellg() << " bytes read";
		pos = file.tellg();
		int result = 0;
		if (length > 0) {
			if (UDP) {
				length += PACKAGE_NUMBER_SIZE;
				sockaddr* destAddr = (sockaddr*)&GetSocketParamsByIp(defaultIp);
				result = SendUDP(socket, fileBuffer, length, destAddr);
			}
			else
				result = send(socket, fileBuffer, length, 0);
		}
		if (result == -1) {
			PrintLastError();
			break;
		}
	} while (length > 0);
	file.close();

	SendString(END_OF_FILE, socket);

	answer = ReceiveAnswer(socket, cmdBuffer);
	free(cmdBuffer);
	free(fileBuffer);
	cout << endl << answer;
}

void ReceiveFile(int socket, string fileName, string savePath) {
	ofstream file;
	file.open(savePath + fileName, ios::binary);

	if (file.is_open()) {
		SendString("receive " + fileName + "\n", socket);
	}
	else {
		cout << "Can't open the file for writing" << savePath << ". Error #" << errno << endl;
		return;
	}

	char *fileBuffer = new char[FILE_BUFFER_SIZE + 100];
	char *tempFileBuffer = new char[FILE_BUFFER_SIZE + 100];
	char *currentPos = fileBuffer;
	unsigned long long totalBytesReceived = 0;
	bool sendingComplete = false;


	while (!sendingComplete)
	{
		unsigned long long recievedBytesCount = 0;
		if (UDP) {
			sockaddr clientAddr;
			int fromLen = sizeof clientAddr;
			recievedBytesCount = ReceiveUDP(socket, tempFileBuffer, FILE_BUFFER_SIZE, &clientAddr, &fromLen);
		}
		else
			recievedBytesCount = recv(socket, tempFileBuffer, FILE_BUFFER_SIZE, 0);
		MyStrcpy(currentPos, tempFileBuffer, recievedBytesCount);
		currentPos += recievedBytesCount;
		if (Contains(fileBuffer, FILE_BUFFER_SIZE, END_OF_FILE)) {
			sendingComplete = true;
			recievedBytesCount -= strlen(END_OF_FILE);
			currentPos = fileBuffer + Pos(fileBuffer, FILE_BUFFER_SIZE, END_OF_FILE);
		}
		if (Contains(fileBuffer, FILE_BUFFER_SIZE, FILE_SEND_ERROR)) {
			cout << "File sending was aborted\n";
			break;
		}
		if (Contains(fileBuffer, FILE_BUFFER_SIZE, FILE_NOT_FOUND)) {
			cout << "File is not found\n";
			break;
		}
		else if (recievedBytesCount == SOCKET_ERROR) {
			cout << "Can't receive file on server side. Error #" + errno + '\n';
			PrintLastError();
			break;
		}
		if (sendingComplete)
		{
			file.write(fileBuffer, currentPos - fileBuffer);
		}
		else
			//buffer should store last ~ 60 bytes to find flags such as EOF
			if (currentPos - fileBuffer >= MAX_FLAG_STRING_LENGTH) {
				int bytesToWriteCount = currentPos - fileBuffer - MAX_FLAG_STRING_LENGTH;
				file.write(fileBuffer, bytesToWriteCount);
				for (int i = 0; i < MAX_FLAG_STRING_LENGTH; i++)
					fileBuffer[i] = currentPos[i - MAX_FLAG_STRING_LENGTH];
				currentPos = fileBuffer + MAX_FLAG_STRING_LENGTH;
			}
		totalBytesReceived += recievedBytesCount;
		cout << "\r" << totalBytesReceived << " bytes received";
		if (sendingComplete)
			cout << endl << "Receiving complete" << endl;
	}
	delete fileBuffer;
	delete tempFileBuffer;
	file.close();
}

bool Contains(char *buffer, int bufferLength, const char *substring)
{
	return Pos(buffer, bufferLength, substring) > -1;
}

int Pos(char *buffer, int bufferLength, const char *substring)
{
	int bufferPos = 0;
	while (bufferPos < bufferLength)
	{
		int subStringPos = 0;
		int tempBufferPos = bufferPos;
		while (bufferPos < bufferLength && buffer[tempBufferPos] == substring[subStringPos])
		{
			tempBufferPos++;
			if (++subStringPos == strlen(substring))
				return bufferPos;
		}
		bufferPos++;
	}
	return -1;
}

int SendString(string str, int socket) {
	int bufSize = str.length() + PACKAGE_NUMBER_SIZE;
	char *buf = (char *)calloc(bufSize, 1);
	strcpy(buf, str.c_str());
	int result = 0;
	if (UDP) {
		sockaddr* destAddr = (sockaddr*)&GetSocketParamsByIp(defaultIp);
		result = SendUDP(socket, buf, bufSize, destAddr);
	}
	else
		result = send(socket, buf, bufSize, MSG_DONTROUTE);

	if (result == -1)
		PrintLastError();
	free(buf);
	return result;
}

void PrintLastError() {
#if  defined _WIN32 || defined _WIN64
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
	wcout << buf;
#elif defined __linux__
	int error = errno;
	cout << strerror(error);
#endif
}

string ReceiveAnswer(int socket, char *buffer) {
	char *currentPos = strchr(buffer, '\0');

	do {
		//find position of line ending
		if (currentPos == NULL)
			currentPos = buffer;

		int bytesRecieved = 0;
		if (UDP) {
			sockaddr clientAddr;
			int fromLen = sizeof clientAddr;
			bytesRecieved = ReceiveUDP(socket, currentPos, BUFFER_SIZE - (currentPos - buffer), &clientAddr, &fromLen);
		}
		else
			bytesRecieved = recv(socket, currentPos, BUFFER_SIZE - (currentPos - buffer), 0);
		if (bytesRecieved == SOCKET_ERROR) {
			buffer[0] = '\0';
			PrintLastError();
			return "";
		}
		currentPos += bytesRecieved;
	} while (strchr(buffer, '\n') == NULL);
	*currentPos = '\0';
	string result;
	int responseLength = currentPos - buffer;
	result.assign(buffer, responseLength);
	for (int i = 0; i < BUFFER_SIZE - responseLength + 1; i++)
		buffer[i] = buffer[i + responseLength + 1];

	return result;
}

void split(const string &s, char delim, vector<string> &elems) {
	stringstream ss;
	ss.str(s);
	string item;
	while (getline(ss, item, delim)) {
		if (item.length())
			elems.push_back(item);
	}
}

vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

string GetFileNameFromPath(string path) {
	string temp = path;
	replace(path.begin(), path.end(), '\\', '/');
	string fileName = split(temp, '/').back();
	return fileName;
}

void MyStrcpy(char* dest, char* source, int length) {
	for (int i = 0; i < length; i++)
		dest[i] = source[i];
}

bool AreEqual(char* first, char* second, int length) {
	for (int i = 0; i < length && (first[i] != 0 || second[i] != 0); i++)
		if (first[i] != second[i])
			return false;
	return true;
}

int MySelect(int socket) {
	fd_set fds;
	int n;
	struct timeval tv;

	// Set up the file descriptor set.
	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	// Set up the struct timeval for the timeout.
	tv.tv_sec = TIMEOUT_MS / 1000;
	tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;

	// Wait until timeout or data received.
	return select(socket, &fds, NULL, NULL, &tv);
}

//buffer must have free PACKAGE_NUMBER_SIZE bytes at the end to insert package number
int SendUDP(SOCKET s, char* buf, int len, sockaddr* to) {
	_ultoa(outPackageNumber, buf + len - PACKAGE_NUMBER_SIZE, 10);
	int fromLen = sizeof *to;
	while (true) {
		if (sendto(s, buf, len, 0, to, sizeof sockaddr_in) == -1)
			return -1;
		switch (MySelect(s))
		{
		case -1:
			return -1;
		case 0:
			continue;
		default:
			break;
		}
		while (recvfrom(s, outPackageNumberStr, PACKAGE_NUMBER_SIZE, 0, to, &fromLen) == -1);
		if (AreEqual(outPackageNumberStr, buf + len - PACKAGE_NUMBER_SIZE, PACKAGE_NUMBER_SIZE))
			break;
	}
	outPackageNumber++;
	return 0;
}

int ReceiveUDP(SOCKET s, char* buf, int len, sockaddr* from, int* fromlen) {
	int receivedBytesCount = 0;
	bool accepted = false;
	while (!accepted) {
		receivedBytesCount = recvfrom(s, buf, len, 0, from, fromlen);
		if (receivedBytesCount == -1)
			return -1;
		receivedBytesCount -= PACKAGE_NUMBER_SIZE;
		if (sendto(s, buf + receivedBytesCount, PACKAGE_NUMBER_SIZE, 0, from, *fromlen) == -1)
			return -1;
		if (!AreEqual(inPackageNumberStr, buf + receivedBytesCount, PACKAGE_NUMBER_SIZE))
			accepted = true;
		MyStrcpy(inPackageNumberStr, buf + receivedBytesCount, PACKAGE_NUMBER_SIZE);
	}
	return receivedBytesCount;
}