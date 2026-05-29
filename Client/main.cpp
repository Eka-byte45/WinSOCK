#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>

#include<FormatLastError.h>


using namespace std;
#pragma comment(lib,"WS2_32.lib")
#pragma comment(lib,"FormatLastError.lib")

#define PORT "27015"
#define BUFFER_LENGTH 1500

void main()
{
	setlocale(LC_ALL, "");
	cout << "CLIENT" << endl;
	CHAR szError[256] = {};
	//1)Init WinSOCK:
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult!=0)
	{
		cout << "WSAStartup failed: " << iResult << endl;
		return;
	}
	//2)Задаем параметры подключения: IP-адрес сервера и порт
	struct addrinfo hints;
	struct addrinfo* result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	iResult = getaddrinfo("127.0.0.1", PORT, &hints, &result);
	if (iResult != 0)
	{
		cout << "getaddrinfo() failed: " << iResult << endl;
		WSACleanup();
		return;
	}
	//3)создаем клиентский сокет:
	SOCKET connect_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (connect_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(WSAGetLastError(), szError) << endl;
		cout << "Socket creation error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//4)Подключение к Серверу:
	iResult = connect(connect_socket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		
		//CHAR szBuffer[256] = {};
		
		cout << "Unable to connect to Server." << endl;
		//cout << "Error " << dwError << ":\t" << lpBuffer << endl;
		cout << FormatLastError(dwError,szError) << endl;

		closesocket(connect_socket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	//5)Отправка и получение данных:
	CHAR sendbuffer[BUFFER_LENGTH] = "Hello Server";
	CHAR recvbuffer[BUFFER_LENGTH] = {};

	do
	{
		iResult = send(connect_socket, sendbuffer, strlen(sendbuffer), 0);
		if (iResult == SOCKET_ERROR)
		{
			cout << FormatLastError(WSAGetLastError(), szError) << endl;
			cout << "Send failed:\t" << WSAGetLastError() << endl;
			closesocket(connect_socket);
			freeaddrinfo(result);
			WSACleanup();
			return;
		}
		cout << "Bytes sent: " << iResult << endl;
		//do
		//{
			iResult = recv(connect_socket, recvbuffer, BUFFER_LENGTH, 0);
			if (iResult > 0) cout << recvbuffer << "(" << iResult << " Bytes)" << endl;
			else if (result == 0)cout << "Connection closed" << endl;
			else cout << FormatLastError(WSAGetLastError(), szError) << endl; //cout << "Recive failed:\t" << WSAGetLastError() << endl;
		//} while (iResult > 0);
		cin.getline(sendbuffer, BUFFER_LENGTH);
	} while (strcmp(sendbuffer,"exit")!=0);

	iResult = shutdown(connect_socket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		cout << FormatLastError(WSAGetLastError(), szError) << endl;
		cout << "Shutdown falied: " << WSAGetLastError() << endl;
	}
	closesocket(connect_socket);
	freeaddrinfo(result);
	WSACleanup();
}