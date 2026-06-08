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
#include<Messages.h>

#define PORT "27015"
#define BUFFER_LENGTH 1500
#define MAX_CONNECTIONS 3

SOCKET sockets[MAX_CONNECTIONS] = {};//массив для хранения дескрипторов сокетов клиента
DWORD dwThredIDs[MAX_CONNECTIONS] = {};//массив для хранения системных ай ди для каждого клиента
HANDLE hThreads[MAX_CONNECTIONS] = {};//массив дескрипторов потоков для управления их жизненным циклом
INT g_ActiveClients = 0;//Счетчик клиентов

//struct ClentParametrs
//{
//	SOCKET client_socket;
//	sockaddr_in client_address;
//};

VOID ClientHandle(SOCKET client_socket);
VOID ShowActiveClients();
VOID Broadcast(CHAR sz_message[], DWORD dwID);
//VOID Realease(SOCKET client_socket);

void main()
{
	setlocale(LC_ALL, "");
	cout << "SERVER" << endl;
	DWORD dwError = 0;//для хранения кода ошибки
	CHAR szError[256] = {};//буфер для текстового описания ошибки
	//1)Init WinSOCK
	WSADATA wsaData;//специальная структура, которую требует WSAStartup
	int iResult;
	//WSAStartup это функция-загрузчик, она загружает в апямять процессора библиотеку WS2_32.dll
	//MAKEWORD(2,2) - старший и младший номер версии, вместе означает версия 2.2
	//&wsaData - указатель на структуру, система ее заполнит реальными данными о реальной версии Winsock
	//функция WSAStartup возвращает 0, если все прошло успешно
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	dwError = WSAGetLastError();
	if (iResult != 0)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "WSAStartup failed: " << iResult << endl;
		return;
	}
	//2)параметры подключения:
	addrinfo hints;//структура, опреедляющая какой именно сокет мы хотим создать
	addrinfo* result;
	ZeroMemory(&hints, sizeof(hints));//обнуляем структуру перед заполнением
	hints.ai_family = AF_INET;//используем IPv4
	hints.ai_socktype = SOCK_STREAM;//потоковый сокет, данные будут передаваться непрерывно
	hints.ai_protocol = IPPROTO_TCP;//протокол TCP
	hints.ai_flags = AI_PASSIVE;//это  флаг говорит функции getaddrinfo заполни IP-адрес так, чтобы сокет мог слушать
	//входящие соеднения на всех сетевых интерфейсах этого компьютера
	iResult = getaddrinfo(NULL, PORT, &hints, &result);//функция переводит порт в формат понятный для ОС
	//первый флаг NULL+AI_PASSIVE означает слушать на всех доступных адресах, PORT - передаем порт как строку
	//&hints - указатель на шаблон с требованиями к сокету, &result -  указатель на указатель это входной
	//параметр, функция выделит память и создаст связный список из структур addtinfo,которые подходят под наши требования
	//getaddrinfo - берет шаблон и порт, формирует готовую структуру данных result? эту структуру можно предать
	//в фунции socket(), bind()
	//Если функция выполнилась успешно, то она возвращает 0
	//getaddrinfo работает как аллокатор
	dwError = WSAGetLastError();
	if (iResult != 0)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "getaddrinfo() failed: " << iResult << endl;
		WSACleanup();//корректоно выгружаем библиотеку Winsock и освобождаем ресурсы
		return;
	}
	//Создаем сокет для сервера, который он будет постоянно слушать "LISTENING"
	//функция socket() обращается к операционной системе с просьбой создать новый сокет и дать его дескриптор
	//в функцию мы передаем парметры, которые получили от getaddrinfo 
	SOCKET listen_socket =
		socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	dwError = WSAGetLastError();
	if (listen_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Listen socket error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);//освобождаем память, которую выделила функция getaddrinfo для структуры result
		WSACleanup();//выключаем сетевую подсистему
		return;
	}
	//4) Bind socket;
	iResult = bind(listen_socket, result->ai_addr, result->ai_addrlen);//команда ОС, которая связывает созданный сокет с
	//конкретным сетевым адресом (IP + порт)
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Bind falied with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);//освобождение блока памяти, функция принимает указатель на начало блока памяти (result)
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
	do
	{
		ShowActiveClients();
		sockaddr_in client_address;//структура, хранящая адрес подключившегося клиента
		int client_addrlen = sizeof(client_address);//сохраняем размер структуры
		client_address.sin_family = AF_INET;//адрес типа IPv4
		//accept - это блокирующая функция, это означает что выполнение программы остановится и будет ждать, пока не
		//придет новый клиент
		//функция берет слушающий сокет и создает новый сокет client_soket
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_addrlen);
		dwError = WSAGetLastError();
		if (client_socket == INVALID_SOCKET)
		{
			cout << FormatLastError(dwError, szError) << endl;
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
		}
		cout << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;
		//ClientHandle(client_socket);
		if (g_ActiveClients < MAX_CONNECTIONS)//количество подключений меньше лимита
		{
			sockets[g_ActiveClients] = client_socket;//сохраняем дескриптор нового сокета в массив для дальнейшего управления
			hThreads[g_ActiveClients] = CreateThread//создаем поток
			(
				NULL,//Security attributes
				0,//Stack size
				(LPTHREAD_START_ROUTINE)ClientHandle,//Указатель на функцию, которая будет выполняться в потоке
				(LPVOID)sockets[g_ActiveClients],
				0,
				&dwThredIDs[g_ActiveClients]
			);
			g_ActiveClients++;//увеличиваем счетчик клиентов
		}
		else
		{
			CHAR recv_buffer[BUFFER_LENGTH] = {};
			iResult = recv(client_socket, recv_buffer, BUFFER_LENGTH, NULL);//читаем данные, то что успел отправить клиент
			/*if (iResult != 0)
			{
				FormatLastError(WSAGetLastError(), szError);
				cout << szError << endl;
			}
			else*/ cout << recv_buffer << endl;
			//CHAR szDeclainMessage[];
			iResult = send(client_socket, DECLINE_MESSAGE, strlen(DECLINE_MESSAGE),NULL);//отрпавляем сообщение об отказе
			shutdown(client_socket, SD_BOTH);//закрываем соединение
			closesocket(client_socket);
		}
		//6.1) Получаем информацию о сокете клиента:
		//sockaddr_in* client_address_in = &client_address;
		//CHAR* clientIP = inet_ntoa(client_address.sa_data+2);
	} while (true);
	WaitForMultipleObjects(MAX_CONNECTIONS, hThreads, TRUE, INFINITE);

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

INT GetSlotIndex(DWORD dwID)
{
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
	{
		if (dwThredIDs[i] == dwID)return i;
	}
}

VOID Shift(INT start)
{
	for (INT i = 0; i < MAX_CONNECTIONS; ++i)
	{
		sockets[i] = sockets[i + 1];
		dwThredIDs[i] = dwThredIDs[i + 1];
		hThreads[i] = hThreads[i + 1];
	}
	sockets[MAX_CONNECTIONS-1] = NULL;
	dwThredIDs[MAX_CONNECTIONS - 1] = NULL;
	hThreads[MAX_CONNECTIONS - 1] = NULL;
	g_ActiveClients--;
}

VOID ClientHandle(SOCKET client_socket)
{
	sockaddr_in client_address;//sockaddr_in - структура для хранения информации об адресе клиента по протоколу IPv4
	client_address.sin_family = AF_INET;//sin_family указывает,что это за адрес,для IPv4 это AF_INET
	INT namelen = sizeof(client_address);//измерили размер client_address структуры
	getpeername(client_socket, (sockaddr*)&client_address, &namelen);//функция позволяет узнать серверу кто к нему подключился, она извлекает инфу о клиенте и помещает в структуру client_address
	CHAR sz_client_address[32] = {};//создаем буффер
	//inet_ntoa - эта функция принимает IP адрес из структруры и преобразует его в строку
	//client_address.sin_port - содержит номер порта,но в сетевом порядке байт
	//ntohs(Network-To-Host Short) -  конвертирует число из сетевого порядка в порядок, понятный процессору(host byte order)
	CHAR sz_client_connected[32] = {};
	sprintf(sz_client_address, "%s:%d - ", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	sprintf(sz_client_connected, "%s CONNECTED", sz_client_address);
	//Broadcast(sz_client_connected, GetCurrentThreadId());
	cout << "Client connected:\t "<<sz_client_address<<"\tSOCKET:\t"<<client_socket << endl;
	INT iResult = 0;//сколько байт получено по результатам функции recv
	DWORD dwError = 0;//для хранения кода ошибки
	CHAR szError[256] = {};//буфер для текстового описания ошибки
	INT iSendResult = 0;//сколько байт отправлено по результатам функции send
	//цикл будет выполняться до тех пор пока результат получения данных больше нуля
	do                                
	{
		CHAR sendBuffer[BUFFER_LENGTH] = {};//буфер для отправки данных
		CHAR recvbuffer[BUFFER_LENGTH] = {};//буфер для входящих данных
		//recv - это функция пытается прочитать данные из сокета, она кладет полученные данные в recvbuffer
		//эта функция может прочитать максимум BUFFER_LENGTH байт
		//возвращает >0, то количество успешно прочитанных байт
		//0 - соединение корректон закрыто клиентом
		//SOCKET_ERROR(-1) произошла ошибка
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);
		dwError = WSAGetLastError();//функция запрашивает у ОС код последней ошибки,которая произошла в контексте сетевых операций для данного потока
		if (iResult > 0)//данные успешно получены
		{
			//выводим на консоль адрес клиента и полученные от него данные
			//strlen(recvbuffer) отправляем только полезные данные
			cout << sz_client_address << recvbuffer << "(" << strlen(recvbuffer) << " Bytes)" << endl;
			sprintf(sendBuffer, "%s%s", sz_client_address, recvbuffer);
			Broadcast(sendBuffer, GetCurrentThreadId());
			//эхо-логика сервер отправляет те же самые данные, которые получил
			//iSendResult = send(client_socket, recvbuffer, strlen(recvbuffer), 0);
			dwError = WSAGetLastError();
			if (iSendResult == SOCKET_ERROR)//проверяем успешно ли прошла отправка,если нет, то  выводим ошибку и закрываем сокет
			{
				cout << FormatLastError(dwError, szError) << endl;
				cout << "Send failed with error: " << WSAGetLastError() << endl;
				closesocket(client_socket);
			}
			else cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)cout << "Connection closing..." << endl;//клиент отключился 
		else//произошла ошибка при получении данных, выводим описание ошибки и закрываем сокет
		{
			cout << FormatLastError(dwError, szError) << endl;
			cout << "Recive failed with error: " << WSAGetLastError() << endl;
			closesocket(client_socket);
		}
	} while (iResult > 0);
	DWORD dwID = GetCurrentThreadId();
	Shift(GetSlotIndex(dwID));
	cout << sz_client_address << "left" << endl;
	//по завершенеию цикла (отключение клиента или же ошибка) нужно закрыть соединение
	iResult = shutdown(client_socket, SD_BOTH);//shutdown - функция, закрывающая соединение, флаг
	//SD_BOTH означает запретить отправку и получение
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shutdown failed with " << FormatLastError(dwError, szError) << endl;
	closesocket(client_socket);
	//Realease(client_socket);
	ShowActiveClients();
	ExitThread(0);
}
//VOID Realease(SOCKET client_socket)
//{
//	for (int i = 0; i < MAX_CONNECTIONS; ++i)
//	{
//		if (client_socket == sockets[i])
//		{
//			sockets[i] = NULL;
//			//dwThredIDs[i] = NULL;
//			//hThreads[i] = NULL;
//			for (int j = i; sockets[j] || j < MAX_CONNECTIONS - 1; ++j)
//			{
//				sockets[j] = sockets[j + 1];
//				dwThredIDs[j] = dwThredIDs[j + 1];
//				hThreads[j] = hThreads[j + 1];
//			}
//		}
//	}
//	g_ActiveClients--;
//	ShowActiveClients();
//}
//
VOID ShowActiveClients()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hConsole, &info);
	COORD cursor = { 25,1 };
	SetConsoleCursorPosition(hConsole, cursor);
	cout << "Количество подключений: " << g_ActiveClients;
	SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
}
VOID Broadcast(CHAR sz_message[],DWORD dwID)
{
	for (INT i = 0; i < g_ActiveClients; ++i)
	{
		if (dwThredIDs[i] != dwID)send(sockets[i], sz_message, strlen(sz_message), 0);
	}
}
