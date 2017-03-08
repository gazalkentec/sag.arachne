//**********************************************************//
//															//
//	sag.arachne.core										//
//															//
//**********************************************************//

#include "stdafx.h"
#include "configurator.h"
#include "logger.h"

using namespace std;

Configurator config;

SERVICE_STATUS			g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE	g_StatusHandle = nullptr;
HANDLE					g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD WINAPI PLCExchangeWorker(LPVOID lpParam);
DWORD WINAPI MAINDBExchangeWorker(LPVOID lpParam);

int InstallService();
int RemoveService();
int StartService();

wchar_t* SERVICE_PATH;

int _tmain(int argc, wchar_t* argv[], wchar_t* env[])
{
	SERVICE_PATH = static_cast<wchar_t*>(argv[0]);

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

	if (config.IsLoaded())
	_INFO << "Config is loaded successfully.";
	else return -1;

	if (argc - 1 == 0)
	{
		_SERVICE_TABLE_ENTRYW ServiceTable[] =
		{
			{LPWSTR((config.ServiceName()).c_str()), static_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain)},
			{nullptr, nullptr}
		};

		if (StartServiceCtrlDispatcherW(ServiceTable) == FALSE)
		{
			_ERROR << "Server start error! Shutdown!";
			return GetLastError();
		}

		_INFO << "Server is stopped...";
	}
	else if ((wcscmp(argv[argc - 1], _T("install")) == 0) || (wcscmp(argv[argc - 1], _T("i")) == 0))
	{
		InstallService();
	}
	else if ((wcscmp(argv[argc - 1], _T("remove")) == 0) || (wcscmp(argv[argc - 1], _T("r")) == 0))
	{
		RemoveService();
	}
	else if ((wcscmp(argv[argc - 1], _T("start")) == 0) || (wcscmp(argv[argc - 1], _T("s")) == 0))
	{
		StartService();
	}

	return ERROR_SUCCESS;
}

	int InstallService()
	{
		auto hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);

		if (!hSCManager)
		{
			_ERROR << "Can't open Service Control Manager";
			return -1;
		}

		//TCHAR buf[] = TEXT("sag.arachne.core");
		//TCHAR buf[] = TEXT(config.ServiceName().begin(), config.ServiceName().end());
		//TCHAR buf[] = _T("sag.arachne.core");

		auto hService = CreateService(
			hSCManager,
			config.ServiceName_C(),
			config.ServiceName_C(),
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			SERVICE_PATH,
			nullptr, nullptr, nullptr, nullptr, nullptr);

		if (!hService)
		{
			auto errCode = GetLastError();
			char *errText;

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				                              nullptr,
				                              errCode,
				                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				                              reinterpret_cast<LPTSTR>(&errText),
				                              0,
				                              nullptr);

			_ERROR << "Create service error!";
			_ERROR << errText;

			CloseServiceHandle(hSCManager);

			return -1;
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);

		_INFO << "Create service success.";

		return ERROR_SUCCESS;
	}

	int RemoveService()
	{
		auto hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
		if (!hSCManager)
		{
			_ERROR << "Can't open Service Control Manager";
			return -1;
		}

		auto hService = OpenService(hSCManager, _T("sag.arachne.core"), SERVICE_STOP | DELETE);

		if (!hService)
		{
			_ERROR << "Can't remove service!";
			CloseServiceHandle(hSCManager);
			return -1;
		}

		DeleteService(hService);
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);

		_INFO << "Service remove success.";
		return ERROR_SUCCESS;
	}

	int StartService()
	{
		auto hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
		if (!hSCManager)
		{
			_ERROR << "Can't open Service Control Manager";
			return -1;
		}

		auto hService = OpenService(hSCManager, LPWSTR((config.ServiceName()).c_str()), SERVICE_START);

		if (!hService)
		{
			CloseServiceHandle(hSCManager);

			_ERROR << "Can't start service!";
			return -1;
		}

		if (!StartService(hService, 0, nullptr))
		{
			CloseServiceHandle(hService);
			CloseServiceHandle(hSCManager);

			_ERROR << "Error starting service!";
			return -1;
		}

		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);

		return ERROR_SUCCESS;
	}

	VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
	{
		DWORD Status = E_FAIL;

		g_StatusHandle = RegisterServiceCtrlHandler(LPWSTR(config.ServiceName().c_str()), ServiceCtrlHandler);

		if (g_StatusHandle == nullptr)
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
		g_ServiceStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (g_ServiceStopEvent == nullptr)
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
		HANDLE hThread = CreateThread(nullptr, 0, ServiceWorkerThread, nullptr, 0, nullptr);
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
		CreateThread(nullptr, 0, MAINDBExchangeWorker, nullptr, 0, nullptr);
		// Start the thread that will perform the main task of the service
		CreateThread(nullptr, 0, PLCExchangeWorker, nullptr, 0, nullptr);
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
