#pragma once


#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#include <deque>
#include <vector>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

/*
Вариант серврера - accep в одном потоке, очередь сокетов, пул потоков для обработки клиентов
*/

typedef char TCHAR_ADDRESS[16];
//#define MAX_BUF 1024

#define DEFAULT_BUFLEN 64*1024
#define DEFAULT_PORT "5001"

/*
Прототип функции вывода
Необходимр реализовать
*/
void _printf(const char* mess, int val);


static int ReadAll(SOCKET ClientSocket, char *p_buff, int size)
{
	int iResult = 0, size_total = size;
	while (size) {
		iResult = recv(ClientSocket, p_buff, size, 0);
		if (iResult < 1) {
			if (iResult == 0) break;
			return iResult;
		}
		p_buff += iResult;
		size -= iResult;
	}
	return size_total - size;
}

//static 	int _SendAll(SOCKET ClientSocket, char *p_buff, int size)
//{
//	int iResult = 0;
//	while (size) {
//		iResult = send(ClientSocket, p_buff, size, 0);
//		if (iResult < 0) break;
//		size -= iResult;
//		p_buff += iResult;
//	}
//	return iResult;
//}
//
//#define MAX_BUF 1024
//
//static int SendAll(SOCKET ClientSocket, char *p_buff, int size)
//{
//	int iResult = 0;
//	while (MAX_BUF < size) {
//		iResult = _SendAll(ClientSocket, p_buff, MAX_BUF);
//		if (iResult < 0) break;
//		p_buff += MAX_BUF;
//		size -= MAX_BUF;
//	}
//	iResult = _SendAll(ClientSocket, p_buff, size);
//	return iResult;
//}


struct TBuffHeader
{
	int cmd;				// Код команды

	unsigned long size;		// Размер дополнительного буфера

	DWORD param;			// Дополнительный параметр

};

struct TSocketData
{
	SOCKET ClientSocket;

	timeval tv;

	TBuffHeader header;

	char *p_buff;

	TSocketData() :p_buff(NULL), ClientSocket(NULL)
	{
		ClearHeaderBuffNull();
		tv.tv_sec = 10;
		tv.tv_usec = 0;
	}

	void ClearHeaderBuffNull()
	{
		ZeroMemory(&header, sizeof(TBuffHeader));
		p_buff = NULL;
	}

	~TSocketData()
	{
		if (ClientSocket) {
			shutdown(ClientSocket, SD_BOTH);
			closesocket(ClientSocket);
		}
	}

	int ReadHeader()
	{
		return ReadAll( (char *)&header, sizeof(header));
	}

	long ReadBuff(long size)
	{
		return ReadAll( p_buff, size);
	}

	int SendHeader()
	{
		return _SendAll( (char *)&header, sizeof(header));
	}

	int SendBuff()
	{
		return SendAll( p_buff, header.size);
	}

	int SendBuff(long size)
	{
		return SendAll(p_buff, size);
	}

	int ReadAll( int size )
	{
		int iResult = 0;
		while (DEFAULT_BUFLEN < size) {
			iResult = ReadAll(p_buff, DEFAULT_BUFLEN);
			if (iResult <= 0) break;
			p_buff += DEFAULT_BUFLEN;
			size -= DEFAULT_BUFLEN;
		}
		return  ReadAll(p_buff, size);
	}

private:

	int TrySelectRecive(char *p_buff, int size)
	{
		const int  kol_pop = 3;
		fd_set st;
		int RetCod = 0;
		int i = 0;
		while (1)
		{
			st.fd_count = 1;
			st.fd_array[0] = ClientSocket;
			RetCod = select(FD_SETSIZE, &st, NULL, NULL, &tv);
			if (RetCod != 0) break;
			if (i == kol_pop) {
				return RetCod;
			}
			++i;
		}
		return  recv(ClientSocket, p_buff, size, 0);
	}

	int ReadAll( char *p_buff, int size)
	{
		int iResult = 0, size_total = size;
		while (size) {
			iResult = TrySelectRecive(p_buff, size);	
			if (iResult < 1) {
				if (iResult == 0) break;
				return iResult;
			}
			p_buff += iResult;
			size -= iResult;
		}
		return size_total - size;
	}

	int _SendAll( char *p_buff, int size)
	{
		int iResult = 0;
		while (size) {
			iResult = send(ClientSocket, p_buff, size, 0);
			if (iResult <= 0) break;
			size -= iResult;
			p_buff += iResult;
		}
		return iResult;
	}


	int SendAll( char *p_buff, int size)
	{
		int iResult = 0;
		while (DEFAULT_BUFLEN < size) {
			iResult = _SendAll( p_buff, DEFAULT_BUFLEN);
			if (iResult <= 0) break;
			p_buff += DEFAULT_BUFLEN;
			size -= DEFAULT_BUFLEN;
		}
		iResult = _SendAll( p_buff, size);
		return iResult;
	}

};



enum E_Result
{
	e_OK,		// данные полностью получены
	e_Next,		// Есть еще порция данных
	e_Err		// Ошибка при обработке данных
};


E_Result HandleCmd(TSocketData &buff_cmd, TCHAR_ADDRESS address);



VOID CALLBACK MyWorkCallbackHandleClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);

VOID CALLBACK MyWorkCallbackHandleClientExt(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);


class CCriticalSection
{
private:
	CRITICAL_SECTION &criticalSection;
public:
	CCriticalSection(CRITICAL_SECTION &_criticalSection) :criticalSection(_criticalSection)
	{
		Enter();
	}
	void Enter()
	{
		EnterCriticalSection(&criticalSection);
	}

	void Leave()
	{
		LeaveCriticalSection(&criticalSection);
	}

	~CCriticalSection()
	{
		Leave();
	}
};





/*
Класс отвечающий за TCP - сервер на основе пула потоков
*/
class CServerTCPThreadpool
{
protected:

	WSADATA wsaData;
	SOCKET ListenSocket;
	PTP_WORK work;

	int InitWinSock()
	{
		// Initialize Winsock
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}

		struct addrinfo *result = NULL;
		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, port, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		return 0;
	}

public:

	std::deque<pair<SOCKET, TCHAR_ADDRESS> > deqSocket;
	CRITICAL_SECTION mCriticalSection;

	const char		*port;

public:

	CServerTCPThreadpool(const char	*_port) :port(_port)
	{
		ListenSocket = INVALID_SOCKET;
		InitializeCriticalSection(&mCriticalSection);
	}

	bool GetSocket_Address(SOCKET &sock, TCHAR_ADDRESS adress)
	{
		CCriticalSection criticalSection(mCriticalSection);
		if (deqSocket.empty()) return false;
		sock = deqSocket.front().first;
		strncpy_s(adress, sizeof(TCHAR_ADDRESS), deqSocket.front().second, sizeof(TCHAR_ADDRESS));
		deqSocket.pop_front();
		adress[sizeof(TCHAR_ADDRESS) - 1] = 0;
		return true;
	}

	/*
	Инициализация для работы с простой передачей данных по сети  (не предполагает передачу больших объемов)
	*/
	int Init()
	{
		work = CreateThreadpoolWork(MyWorkCallbackHandleClient, this, NULL);
		if (NULL == work) {
			_printf("CreateThreadpoolWork failed. LastError: %u\n", GetLastError());
		}

		//	SetThreadpoolThreadMaximum(pool_dat->pool, 256);
		return InitWinSock();
	}

	void Run()
	{
		
		SOCKET socket = INVALID_SOCKET;
		sockaddr_in client;
		int len_client = sizeof(sockaddr);
		pair<SOCKET, TCHAR_ADDRESS> p;
		for (;; ) {

			socket = accept(ListenSocket, (sockaddr *)&client, &len_client);

			EnterCriticalSection(&mCriticalSection);
			p.first = socket;
		//	strncpy_s(p.second, inet_ntoa(client.sin_addr), 15);
			inet_ntop(AF_INET, (void *)&client.sin_addr, p.second, 15);
			deqSocket.push_back(p);
			LeaveCriticalSection(&mCriticalSection);
			if (socket == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				continue;
			}
			// Receive until the peer shuts down the connection
			SubmitThreadpoolWork(work);
		}

		WaitForThreadpoolWorkCallbacks(work, false);

		closesocket(ListenSocket);
	}

	~CServerTCPThreadpool()
	{
		DeleteCriticalSection(&mCriticalSection);
		closesocket(ListenSocket);
		WSACleanup();
	}

};


VOID CALLBACK MyWorkCallbackHandleClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
	// Instance, Parameter, and Work not used in this example.
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(Work);
	CServerTCPThreadpool *serverTCPThreadpool = (CServerTCPThreadpool *)Parameter;
	TCHAR_ADDRESS ip_address_client;
	TSocketData buff_cmd;
	if (!serverTCPThreadpool->GetSocket_Address(buff_cmd.ClientSocket, ip_address_client)) return;
	int iResult = 0;
	const char *mess_send = "ok";
	E_Result res = e_OK;
	do
	{
		buff_cmd.ClearHeaderBuffNull();
		iResult = ReadAll(buff_cmd.ClientSocket, (char *)&buff_cmd.header, sizeof(buff_cmd.header));
		if (iResult < 1) {
			_printf("recv headr failed with error: %d\n", WSAGetLastError());
			return;
		}
		// Обработка строки-команды
		res = HandleCmd(buff_cmd, ip_address_client);
		/*	if( buff_cmd.header.size && res != e_Err ){
		iResult = ReadAll(buff_cmd.ClientSocket, buff_cmd.p_buff, buff_cmd.header.size);
		if (iResult < 1) {
		_printf("recv buff failed with error: %d\n", WSAGetLastError());
		return;
		}
		}*/

	} while (res == e_Next);
	// Ошибка 
	if (res == e_Err)	mess_send = "er";
	iResult = send(buff_cmd.ClientSocket, mess_send, 2, 0);
	if (iResult == SOCKET_ERROR) {
		_printf("send failed with error: %d\n", WSAGetLastError());
		return;
	}
	iResult = shutdown(buff_cmd.ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) _printf("shutdown failed with error: %d\n", WSAGetLastError());
	//	Sleep(1000);
}



/*
Класс для работы с расширенной передачей данных по сети (предполагает передачу больших объемов)
*/
template<class T>
class CServerTCPThreadpoolExt :public CServerTCPThreadpool
{
public:

	CServerTCPThreadpoolExt(const char	*_port) : CServerTCPThreadpool(_port)
	{
	}

	int Init()
	{
		work = CreateThreadpoolWork(MyWorkCallbackHandleClientExt<T>, this, NULL);
		if (NULL == work) {
			_printf("CreateThreadpoolWork failed. LastError: %u\n", GetLastError());
		}
		return InitWinSock();
	}
};


template<class T>
VOID CALLBACK MyWorkCallbackHandleClientExt(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
	// Instance, Parameter, and Work not used in this example.
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(Work);
	CServerTCPThreadpoolExt<T> *serverTCPThreadpool = (CServerTCPThreadpoolExt<T> *)Parameter;
	TCHAR_ADDRESS ip_address_client;
	TSocketData buff_cmd_in;
	if (!serverTCPThreadpool->GetSocket_Address(buff_cmd_in.ClientSocket, ip_address_client)) return;
//	_printf("open socket %i\n", buff_cmd_in.ClientSocket);
	T objectClient;
	int iResult = 1;
	while (iResult > 0) {
		int iResult = buff_cmd_in.ReadHeader();
		if (iResult > 0)
			iResult = objectClient.HandleCmd(buff_cmd_in, ip_address_client);
		else break;
	}	
	if (iResult < 0) {
		_printf("recv or send headr failed with error: %d\n", WSAGetLastError());
	}
	iResult = shutdown(buff_cmd_in.ClientSocket, SD_BOTH);
	closesocket(buff_cmd_in.ClientSocket);
//	_printf("close socket %i\n", buff_cmd_in.ClientSocket);
	buff_cmd_in.ClientSocket = 0;
	if (iResult == SOCKET_ERROR) _printf("shutdown failed with error: %d\n", WSAGetLastError());
	//	Sleep(1000);
}

//template<class T>
//VOID CALLBACK MyWorkCallbackHandleClientExt(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
//{
//	// Instance, Parameter, and Work not used in this example.
//	UNREFERENCED_PARAMETER(Instance);
//	UNREFERENCED_PARAMETER(Work);
//	CServerTCPThreadpoolExt<T> *serverTCPThreadpool = (CServerTCPThreadpoolExt<T> *)Parameter;
//	TCHAR_ADDRESS ip_address_client;
//	TSocketData buff_cmd_in;
//	if (!serverTCPThreadpool->GetSocket_Address(buff_cmd_in.ClientSocket, ip_address_client)) return;
//	E_Result res = e_OK;
//	T objectClient;
//	int iResult = ReadAll(buff_cmd_in.ClientSocket, (char *)&buff_cmd_in.header, sizeof(buff_cmd_in.header));
//	if (iResult < 1) {
//		_printf("recv headr failed with error: %d\n", WSAGetLastError());
//		return;
//	}
//	res = objectClient.ReciveHandleCmd(buff_cmd_in, ip_address_client);
//	while (res == e_Next && buff_cmd_in.header.size)
//	{
//		// Обработка строки-команды
//		iResult = ReadAll(buff_cmd_in.ClientSocket, buff_cmd_in.p_buff, buff_cmd_in.header.size);
//		if (iResult < 1) {
//			_printf("recv buff failed with error: %d\n", WSAGetLastError());
//			return;
//		}
//		res = objectClient.ReciveHandleCmd(buff_cmd_in, ip_address_client);
//	}
//	// Ошибка 
//	if (res != e_Err) {
//		if (objectClient.SendHandleCmd(buff_cmd_in)) {
//			iResult = SendAll(buff_cmd_in.ClientSocket, (char *)&buff_cmd_in.header, sizeof(TBuffHeader));
//			iResult = SendAll(buff_cmd_in.ClientSocket, buff_cmd_in.p_buff, buff_cmd_in.header.size);
//		}
//	}
//	else iResult = send(buff_cmd_in.ClientSocket, "er", 2, 0);
//
//	if (iResult == SOCKET_ERROR) {
//		_printf("send failed with error: %d\n", WSAGetLastError());
//		return;
//	}
//	iResult = shutdown(buff_cmd_in.ClientSocket, SD_SEND);
//	if (iResult == SOCKET_ERROR) _printf("shutdown failed with error: %d\n", WSAGetLastError());
//	//	Sleep(1000);
//}
