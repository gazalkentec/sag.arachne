//**********************************************************//
//															//
//	sag.arachne.boost										//
//															//
//**********************************************************//

#include <Windows.h>
#include <tchar.h>

#include "stdafx.h"
#include "configurator.h"
#include "logger.h"

using namespace std;

Configurator config;

SERVICE_STATUS			g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE	g_StatusHandle = NULL;
HANDLE					g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);

VOID WINAPI ServiceCtrlHandler(DWORD);

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD WINAPI PLCExchangeWorker(LPVOID lpParam);
DWORD WINAPI MAINDBExchangeWorker(LPVOID lpParam);

int _tmain(int argc, TCHAR *argv[], TCHAR *env[])
{
	BOOST_LOG_NAMED_SCOPE("i/o service");

	/*
	//for (int i = 0; i < 10; i++)
	//{
	//	std::cout << "\a" << std::endl;

	//	Sleep(1000);
	//}

	//std::cout << "Parameters count: " << argc << std::endl;

	//std::cout << "Parameters: " << std::endl;

	//for (int i = 0; argv[i]; i++)
	//{

	//	std::wstring buff = argv[i];
	//
	//	std::cout << std::string(buff.begin(), buff.end()) << std::endl;

	//}

	//std::cout << "Environment: " << std::endl;

	//for (int i = 0; env[i]; i++)
	//{

	//	std::wstring buff = env[i];

	//	std::cout << std::string(buff.begin(), buff.end()) << std::endl;

	//std::cout << char(*env[i]);
	//
	//for (; *env[i]++; )
	//{
	//	std::cout << char(*env[i]);
	//}

	//std::cout << std::endl;
	//}

	//system("PAUSE >> VOID");

	//return(EXIT_SUCCESS); */

	config.LoadConfig(argc, argv, env);

	//loginfo << "start";

	if (!config.IsLoaded())
	{
		//_ERROR << "Config is not loaded! Server is unable to start...";
		return -1;
	} else _INFO << "Config is loaded successfully. Server is try to start...";

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ LPWSTR(config.ServiceName().c_str()), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		_ERROR << "Server start error! Shutdown!";
		return GetLastError();
	}

	_INFO << "Server is stopped...";

	return ERROR_SUCCESS;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;

	g_StatusHandle = RegisterServiceCtrlHandler(LPWSTR(config.ServiceName().c_str()), ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		_ERROR << "Server start error! Is will be stopped!";
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	/*
	* Perform tasks neccesary to start the service here
	*/
	//OutputDebugString(_T("My Sample Service: ServiceMain: Performing Service Start Operations"));

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		//OutputDebugString(_T("My Sample Service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
		}
		goto EXIT;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	// Start the thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);


	/*
	* Perform any cleanup tasks
	*/

	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

EXIT:

	return;
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		_INFO << "Server stop request. Service will be stopped...";

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks neccesary to stop the service here
		*/

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	//OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Exit"));
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{

	_INFO << "main worker is running...";

	// Start the thread that will perform the main task of the service
	CreateThread(NULL, 0, MAINDBExchangeWorker, NULL, 0, NULL);
	// Start the thread that will perform the main task of the service
	CreateThread(NULL, 0, PLCExchangeWorker, NULL, 0, NULL);
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{

		_INFO << "main worker is working...";

		Sleep(50);
	}

	_INFO << "main worker exit...";

	return ERROR_SUCCESS;
}


DWORD WINAPI PLCExchangeWorker(LPVOID lpParam)
{

	_INFO << "PLC exchange worker is running...";

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{

		_INFO << "PLC exchanger is working...";

		Sleep(config.PLCPollPeriodMSec());
	}

	_INFO << "PLC exchange worker exit...";

	return ERROR_SUCCESS;
}

DWORD WINAPI MAINDBExchangeWorker(LPVOID lpParam)
{

	_INFO << "MainDB worker is running...";

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{

		_INFO << "MainDB exchanger is working...";

		Sleep(config.MainDBPollPeriodMSec());
	}

	_INFO << "MainDB worker exit...";

	return ERROR_SUCCESS;
}
