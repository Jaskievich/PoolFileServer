// MyFileServer.cpp : Defines the entry point for the console application.
//

#include "MyFileServer.h"


CRITICAL_SECTION gCriticalSection_CONSOLE;

long count_rec = 0;

void _printf_c(const char* mess, int val)
{
	EnterCriticalSection(&gCriticalSection_CONSOLE);
	count_rec++;
	printf("%i - ", count_rec);
	printf(mess, val);
	LeaveCriticalSection(&gCriticalSection_CONSOLE);
}

void _printf_c(const char* mess)
{
	EnterCriticalSection(&gCriticalSection_CONSOLE);
	count_rec++;
	printf("%i - ", count_rec);
	printf(mess);
	LeaveCriticalSection(&gCriticalSection_CONSOLE);
}

void _printf(const char* mess, int val)
{
	EnterCriticalSection(&gCriticalSection_CONSOLE);
	printf(mess, val);
	LeaveCriticalSection(&gCriticalSection_CONSOLE);
}

void _printf(const char* mess)
{
	EnterCriticalSection(&gCriticalSection_CONSOLE);
	printf(mess);
	printf("\n");
	LeaveCriticalSection(&gCriticalSection_CONSOLE);
}

#define BUFLEN 3



int main()
{
	InitializeCriticalSection(&gCriticalSection_CONSOLE);

	CServerTCPThreadpoolExt<CHandler> serverTCPThreadpool(DEFAULT_PORT);

	if (!serverTCPThreadpool.Init())
		serverTCPThreadpool.Run();


	DeleteCriticalSection(&gCriticalSection_CONSOLE);

	getchar();
	getchar();


	return 0;
}

