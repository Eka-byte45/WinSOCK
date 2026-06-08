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
#include<Messages.h>

#define PORT "27015"
#define BUFFER_LENGTH 1500

DWORD WINAPI ReceiveMessages(LPVOID lpParam);

bool g_keepReceiving = true;// Глобальный флаг для управления работой потока приема сообщений

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

	HANDLE hThread = CreateThread(NULL, 0, ReceiveMessages, (LPVOID)connect_socket, 0, NULL);
	if (hThread == NULL)
	{
		closesocket(connect_socket);
		WSACleanup();
		return;
	}

	CHAR sendbuffer[BUFFER_LENGTH] = "Hello Server"; 

	do 
	{
		// Отправляем сообщение на сервер
		iResult = send(connect_socket, sendbuffer, (int)strlen(sendbuffer), 0);
		if (iResult == SOCKET_ERROR)
		{
			cout << "send failed: " << WSAGetLastError() << endl;
			break;
		}
		cout << "Bytes sent: " << iResult << endl;

		// Проверяем, не пришло ли от сервера сообщение об отказе
		if (!g_keepReceiving)
		{
			break;
		}

		// Подготовка к вводу нового сообщения
		ZeroMemory(sendbuffer, BUFFER_LENGTH);
		SetConsoleCP(1251);
		cin.getline(sendbuffer, BUFFER_LENGTH);
		SetConsoleCP(866);

	} while (strcmp(sendbuffer, "exit") != 0 && g_keepReceiving);

	// Завершение работы
	g_keepReceiving = false; // Останавливаем поток приема

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	shutdown(connect_socket, SD_BOTH);
	closesocket(connect_socket);
	WSACleanup();
	////5)Отправка и получение данных:
	//CHAR sendbuffer[BUFFER_LENGTH] = "Hello Server";

	//do
	//{
	//	CHAR recvbuffer[BUFFER_LENGTH] = {};
	//	iResult = send(connect_socket, sendbuffer, strlen(sendbuffer), 0);
	//	if (iResult == SOCKET_ERROR)
	//	{
	//		cout << FormatLastError(WSAGetLastError(), szError) << endl;
	//		cout << "Send failed:\t" << WSAGetLastError() << endl;
	//		closesocket(connect_socket);
	//		freeaddrinfo(result);
	//		WSACleanup();
	//		return;
	//	}
	//	cout << "Bytes sent: " << iResult << endl;
	//	//do
	//	//{
	//		iResult = recv(connect_socket, recvbuffer, BUFFER_LENGTH, 0);
	//		//DWORD dwError = WSAGetLastError();
	//		//CHAR szError[256] = {};
	//		//cout << FormatLastError(dwError, szError) << endl;
	//		if (iResult > 0) cout << recvbuffer << "(" << iResult << " Bytes)" << endl;
	//		else if (result == 0)cout << "Connection closed" << endl;
	//		else cout << FormatLastError(WSAGetLastError(), szError) << endl; //cout << "Recive failed:\t" << WSAGetLastError() << endl;
	//	//} while (iResult > 0);
	//		if (strcmp(recvbuffer, DECLINE_MESSAGE) == 0)
	//		{
	//			system("PAUSE");
	//			break;
	//		}
	//		ZeroMemory(sendbuffer, BUFFER_LENGTH);
	//		SetConsoleCP(1251);
	//	cin.getline(sendbuffer, BUFFER_LENGTH);
	//	SetConsoleCP(866);
	//} while (strcmp(sendbuffer,"exit")!=0);

	//iResult = shutdown(connect_socket, SD_BOTH);
	//if (iResult == SOCKET_ERROR)
	//{
	//	cout << FormatLastError(WSAGetLastError(), szError) << endl;
	//	cout << "Shutdown falied: " << WSAGetLastError() << endl;
	//}
	//closesocket(connect_socket);
	//freeaddrinfo(result);
	//WSACleanup();
}

DWORD WINAPI ReceiveMessages(LPVOID lpParam)
{
	SOCKET client_socket = (SOCKET)lpParam;//
	INT iResult;//переменная для хранения результата функции
	CHAR recvbuffer[BUFFER_LENGTH] = {};//буфер куда ОС будет складывать входящие данные,{} делаем чтобы не было мусора

	while (true)
	{
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);//client_socket - откуда читаем, recvbuffer -  буфер, куда записываем прочитанные данные,BUFFER_LENGTH - максимальное количество байт, которое хотим прочитать
		if (iResult > 0)//данные получены
		{
			// Выводим сообщение сразу, как только получили
			cout << recvbuffer << endl;
			ZeroMemory(recvbuffer, BUFFER_LENGTH); // Очищаем буфер
		}
		else if (iResult == 0)//корректное закрытие со стороны сервера
		{
			cout << "Connection closed by server." << endl;
			break;
		}
		else//ошибка
		{
			cout << "recv failed: " << WSAGetLastError() << endl;
			break;
		}
	}
	return 0;
}
//Эта функция обеспечивает непрерывное получение сообщений от сервера в реальном времени, не
//мешая пользователю вводить свои сообщения
//DWORD WINAPI это стандартный синтаксис для точки входа функции потока в Windows API
//Эта функция возвращает значение типа DWORD
//LPVOID ipParam - это указатель на данные любого типа void*, которые мы передаем в поток при его создании
//в данном случае это дескриптор сокета SOCKET, чтобы функция знала откуда она читает данные