#include <stdlib.h>
#include "Header.h"

const int BUFFER_SIZE = 1000;
const int FILE_BUFFER_SIZE = 10 * 1024 * 1024;
const char *END_OF_FILE = "File sent 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *FILE_SEND_ERROR = "File send error 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *FILE_NOT_FOUND = "File is not found 8bb20328-3a19-4db8-b138-073a48f57f4a";
const char *DEFAULT_FILE_PATH_WINDOWS = "../debug/files/";
const int MAX_FLAG_STRING_LENGTH = 60;

int main() {
	int socket;

#if  defined _WIN32 || defined _WIN64
	ConfirmWinSocksDll();
#endif

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
		if (cmd.find("connect") == 0) {
			vector<string> words = split(cmd, ' ');
			if (words.size() > 1) {
				if (Connect(socket, words[1]) == SOCKET_ERROR) {
					cout << "Connection failed\n";
					PrintLastError();
					continue;
				}
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
		else if (SendString(cmd + "\n", *socket) == SOCKET_ERROR)
			continue;
		string answer = ReceiveAnswer(*socket, buffer);
		cout << answer;
	}

	free(buffer);
}

int Connect(int *socket, string ip) {
	InitClientSocket(socket);

	sockaddr_in clientSocketParams;
	clientSocketParams.sin_port = htons(10000);
	clientSocketParams.sin_family = AF_INET;

#if defined _WIN32 || defined _WIN64
	clientSocketParams.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
#elif defined __linux__
	clientSocketParams.sin_addr.s_addr = inet_addr(ip.c_str());
#endif

	return connect(*socket, (struct sockaddr *) &clientSocketParams, sizeof(clientSocketParams));
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

	char *fileBuffer = (char*)calloc(FILE_BUFFER_SIZE, 1);
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
		if (length > 0)
			send(socket, fileBuffer, length, 0);
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
		unsigned long long recievedBytesCount = recv(socket, tempFileBuffer, FILE_BUFFER_SIZE, 0);
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
	int bufSize = str.length();
	char *buf = (char *)calloc(bufSize + 1, 1);
	strcpy(buf, str.c_str());
	if (send(socket, buf, bufSize, MSG_DONTROUTE) == -1) {
		PrintLastError();
		return -1;
	}
	free(buf);
	return 0;
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

		int bytesRecieved = recv(socket, currentPos, BUFFER_SIZE - (currentPos - buffer), 0);
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