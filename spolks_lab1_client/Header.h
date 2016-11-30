#pragma once

#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
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

int Connect(int *socket, string ip);

void SendFile(int socket, string fileName);

int Send(string str, int socket);

void PrintLastError();

string ReceiveAnswer(int socket, char *buffer);

void split(const string &s, char delim, vector<string> &elems);

vector<string> split(const string &s, char delim);