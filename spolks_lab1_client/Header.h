#pragma once

#include <Ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#define SocketPort 27015
#define	 WIN32_LEAN_AND_MEAN

using namespace std;

int ConfirmWinSocksDll();

void PrintLastError();

void InitClientSocket(int* clientSocket);

void Connect(int socket, string ip);

void Send(string str, int socket);