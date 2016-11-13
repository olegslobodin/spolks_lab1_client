#pragma once

#include <Ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")
#define SocketPort 27015
#define	 WIN32_LEAN_AND_MEAN

using namespace std;

int ConfirmWinSocksDll();

void InitClientSocket(int* clientSocket);

void Work(int* socket);

int Connect(int* socket, string ip);

int Send(string str, int socket);

void PrintLastError();

string ReceiveAnswer(int socket, char* buffer);

void split(const string &s, char delim, vector<string> &elems);

vector<string> split(const string &s, char delim);