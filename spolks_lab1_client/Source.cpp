#include <stdlib.h>
#include"Header.h"

const int BUFFER_SIZE = 1000;

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
    char *buffer = (char *) calloc(BUFFER_SIZE, 1);

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
        } else if (Send(cmd + "\n", *socket) == -1)
            continue;
        string answer = ReceiveAnswer(*socket, buffer);
        cout << answer << endl;
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

int Send(string str, int socket) {
    int bufSize = str.length();
    char *buf = (char *) calloc(bufSize + 1, 1);
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
    //find position of line ending
    char *currentPos = strchr(buffer, '\0');
    if (currentPos == NULL)
        currentPos = buffer;

    int bytesRecieved = recv(socket, currentPos, BUFFER_SIZE - (currentPos - buffer), 0);
    if (bytesRecieved == SOCKET_ERROR) {
        buffer[0] = '\0';
        PrintLastError();
        return "";
    }
    currentPos += bytesRecieved;
    *currentPos = '\0';

    int responseLength = currentPos - buffer;
    string result;
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