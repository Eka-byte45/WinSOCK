#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_MEAN
#define  WIN32_LEAN_MEAN
#endif // !WIN32_LEAN_MEAN
#include<iostream>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include<Windows.h>

#include<FormatLastError.h>
using namespace std;

#pragma comment(lib,"WS2_32.lib")
#pragma comment(lib,"FormatLastError.lib")

#define PORT "27015"
#define BUFFER_LENGTH 1500
#define MAX_CONNECTIONS 5

SOCKET sockets[MAX_CONNECTIONS] = {};
DWORD dwThredIDs[MAX_CONNECTIONS] = {};
HANDLE hThreads[MAX_CONNECTIONS] = {};

//struct ClentParametrs
//{
//	SOCKET client_socket;
//	sockaddr_in client_address;
//};

VOID ClientHandle(SOCKET client_socket);

void main()
{
	setlocale(LC_ALL, "");
	cout << "SERVER" << endl;
	DWORD dwError = 0;
	CHAR szError[256] = {};
	//1)Init WinSOCK
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	dwError = WSAGetLastError();
	if (iResult != 0)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "WSAStartup failed: " << iResult << endl;
		return;
	}
	//2)параметры подключения:
	addrinfo hints;
	addrinfo* result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	dwError = WSAGetLastError();
	if (iResult != 0)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "getaddrinfo() failed: " << iResult << endl;
		WSACleanup();
		return;
	}
	//Создаем сокет для сервера, который он будет постоянно слушать "LISTENING"
	SOCKET listen_socket =
		socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	dwError = WSAGetLastError();
	if (listen_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Listen socket error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//4) Bind socket;
	iResult = bind(listen_socket, result->ai_addr, result->ai_addrlen);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Bind falied with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	freeaddrinfo(result);
	//5) Запускаем прослушивание сокета:
	if (listen(listen_socket,MAX_CONNECTIONS) == SOCKET_ERROR)
	{
		dwError = WSAGetLastError();
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//6)Обработка соединений от клиентов:
	INT i = 0;//Счетчик клиентов
	do
	{
		sockaddr_in client_address;
		int client_addrlen = sizeof(client_address);
		client_address.sin_family = AF_INET;
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_addrlen);
		dwError = WSAGetLastError();
		if (client_socket == INVALID_SOCKET)
		{
			cout << FormatLastError(dwError, szError) << endl;
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
		}
		if (i < MAX_CONNECTIONS)
		{
			sockets[i] = client_socket;
			hThreads[i] = CreateThread
			(
				NULL,//Security attributes
				0,//Stack size
				(LPTHREAD_START_ROUTINE)ClientHandle,//Указатель на функцию, которая будет выполняться в потоке
				(LPVOID)sockets[i],
				0,
				&dwThredIDs[i]
			);
			i++;
		}
		//6.1) Получаем информацию о сокете клиента:
		//sockaddr_in* client_address_in = &client_address;
		//CHAR* clientIP = inet_ntoa(client_address.sa_data+2);
		cout << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;
		//ClientHandle(client_socket);
	} while (true);
	

	//7)Получение и отправка данных:
	/*INT iSendResult = 0;
	do
	{
		CHAR sendBuffer[BUFFER_LENGTH] = {};
		CHAR recvbuffer[BUFFER_LENGTH] = {};
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);
		dwError = WSAGetLastError();
		if (iResult > 0)
		{
			cout << recvbuffer << "(" << strlen(recvbuffer) << " Bytes)" << endl;
			iSendResult = send(client_socket, recvbuffer,strlen(recvbuffer), 0);
			dwError = WSAGetLastError();
			if (iSendResult == SOCKET_ERROR)
			{
				cout << FormatLastError(dwError, szError) << endl;
				cout << "Send failed with error: " << WSAGetLastError() << endl;
				closesocket(client_socket);
			}
			else cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)cout << "Connection closing..." << endl;
		else
		{
			cout << FormatLastError(dwError, szError) << endl;
			cout << "Recive failed with error: " << WSAGetLastError() << endl;
			closesocket(client_socket);
		}
	} while (iResult > 0);*/

	/*iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shutdown failed with " << FormatLastError(dwError, szError) << endl;*/

	/*iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Server shutdown failed with " << FormatLastError(dwError, szError) << endl;*/

	
	closesocket(listen_socket);
	WSACleanup();
}

VOID ClientHandle(SOCKET client_socket)
{
	sockaddr_in client_address;
	client_address.sin_family = AF_INET;
	INT namelen = sizeof(client_address);
	getpeername(client_socket, (sockaddr*)&client_address, &namelen);
	CHAR szName[32] = {};
	sprintf(szName, "%s:%d\t", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

	cout << "Client connected:\t "<<szName<<"\tSOCKET:\t"<<client_socket << endl;
	INT iResult = 0;
	DWORD dwError = 0;
	CHAR szError[256] = {};
	INT iSendResult = 0;
	do
	{
		CHAR sendBuffer[BUFFER_LENGTH] = {};
		CHAR recvbuffer[BUFFER_LENGTH] = {};
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);
		dwError = WSAGetLastError();
		if (iResult > 0)
		{
			cout << recvbuffer << "(" << strlen(recvbuffer) << " Bytes)" << endl;
			iSendResult = send(client_socket, recvbuffer, strlen(recvbuffer), 0);
			dwError = WSAGetLastError();
			if (iSendResult == SOCKET_ERROR)
			{
				cout << FormatLastError(dwError, szError) << endl;
				cout << "Send failed with error: " << WSAGetLastError() << endl;
				closesocket(client_socket);
			}
			else cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)cout << "Connection closing..." << endl;
		else
		{
			cout << FormatLastError(dwError, szError) << endl;
			cout << "Recive failed with error: " << WSAGetLastError() << endl;
			closesocket(client_socket);
		}
	} while (iResult > 0);

	iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shutdown failed with " << FormatLastError(dwError, szError) << endl;
	closesocket(client_socket);
}
