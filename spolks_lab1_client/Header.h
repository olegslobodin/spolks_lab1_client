#pragma once

#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <vector>

#if  defined _WIN32 || defined _WIN64

#include <Ws2tcpip.h>
#include <winsock2.h>
#include <direct.h>
#define SocketPort 27015
#define WIN32_LEAN_AND_MEAN

#elif defined __linux__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <errno.h>

#define SOCKET_ERROR -1

#endif

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#if  defined _WIN32 || defined _WIN64
int ConfirmWinSocksDll();
#endif

void InitClientSocket(int *clientSocket);

void Work(int *socket);

sockaddr_in GetSocketParamsByIp(string ip);

int ConnectOrSetIp(int *socket, string ip);

void SendFile(int socket, string path);

void ReceiveFile(int socket, string fileName, string savePath);

bool Contains(char *buffer, int bufferLength, const char *substring);

int Pos(char *buffer, int bufferLength, const char *substring);

int SendString(string str, int socket);

void PrintLastError();

string ReceiveAnswer(int socket, char *buffer);

void split(const string &s, char delim, vector<string> &elems);

vector<string> split(const string &s, char delim);

string GetFileNameFromPath(string s);

void MyStrcpy(char* dest, char* source, int length);

bool AreEqual(char* first, char* second, int length);

int MySelect(int socket);

int SendUDP(int s, char* buf, int len, sockaddr_in* to);

int ReceiveUDP(int s, char* buf, int len, sockaddr_in* from, socklen_t* fromlen);