// WMIQuerySamples.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <windows.h>
#include <comutil.h>
#include <wbemidl.h>
#include <objbase.h>
#include <atlbase.h>
#include "eventsink.h"
#include <atomic>

std::atomic_bool g_shutdown = false;
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT:
		cout << "Ctrl-C event" << endl;
		g_shutdown = true;
		Sleep(10000);
		return(TRUE);
	default:
		return FALSE;
	}
}
int main(int iArgCnt, char ** argv)
{
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		HRESULT hres;

		// Step 1: --------------------------------------------------
		// Initialize COM. ------------------------------------------

		hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres))
		{
			cout << "Failed to initialize COM library. Error code = 0x"
				<< hex << hres << endl;
			return 1;                  // Program has failed.
		}

		// Step 2: --------------------------------------------------
		// Set general COM security levels --------------------------

		hres = CoInitializeSecurity(
			NULL,
			-1,                          // COM negotiates service
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
		);


		if (FAILED(hres))
		{
			cout << "Failed to initialize security. Error code = 0x"
				<< hex << hres << endl;
			CoUninitialize();
			return 1;                      // Program has failed.
		}

		// Step 3: ---------------------------------------------------
		// Obtain the initial locator IWbemLocator to WMI -------------------------

		IWbemLocator *pLoc = NULL;

		hres = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *)&pLoc);

		if (FAILED(hres))
		{
			cout << "Failed to create IWbemLocator object. "
				<< "Err code = 0x"
				<< hex << hres << endl;
			CoUninitialize();
			return 1;                 // Program has failed.
		}

		// Step 4: ---------------------------------------------------
		// Connect to WMI through the IWbemLocator::ConnectServer method

		IWbemServices *pSvc = NULL;

		// Connect to the local root\cimv2 namespace
		// and obtain pointer pSvc to make IWbemServices calls.
		hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"),
			NULL,
			NULL,
			0,
			NULL,
			0,
			0,
			&pSvc
		);

		if (FAILED(hres))
		{
			cout << "Could not connect. Error code = 0x"
				<< hex << hres << endl;
			pLoc->Release();
			CoUninitialize();
			return 1;                // Program has failed.
		}

		cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;

		// Synchronous Query
		if (SUCCEEDED(hres))
		{
			// execute a query
			CComPtr< IEnumWbemClassObject > enumerator;
			hres = pSvc->ExecQuery(L"WQL", L"SELECT * FROM Win32_Processor",
				WBEM_FLAG_FORWARD_ONLY, NULL, &enumerator);
			if (SUCCEEDED(hres))
			{
				// read the first instance from the enumeration (only one on single processor machines)
				CComPtr< IWbemClassObject > processor = NULL;
				ULONG retcnt;
				hres = enumerator->Next(WBEM_INFINITE, 1L, reinterpret_cast<IWbemClassObject**>(&processor), &retcnt);
				if (SUCCEEDED(hres))
				{
					if (retcnt > 0)
					{
						// extract a property value for installed processor
						_variant_t var_val;
						hres = processor->Get(L"Name", 0, &var_val, NULL, NULL);
						if (SUCCEEDED(hres))
						{
							//_bstr_t str = var_val;
							cout << "Processor name: " << _bstr_t(var_val) << std::endl;
						}
						else
						{
							cout << "IWbemClassObject::Get failed" << std::endl;
							
						}
					}
					else
					{
						cout << "Enumeration empty" << std::endl;
					}
				}
				else
				{
					cout << "Error in iterating through enumeration" << std::endl;
				}
			}
			else
			{
				cout << "Query failed" << std::endl;
			}
		}


		// Step 5: --------------------------------------------------
		// Set security levels on the proxy -------------------------

		hres = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
			NULL,                        // Server principal name 
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities 
		);

		if (FAILED(hres))
		{
			cout << "Could not set proxy blanket. Error code = 0x"
				<< hex << hres << endl;
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return 1;               // Program has failed.
		}

		// Step 6: -------------------------------------------------
		// Receive event notifications -----------------------------

		// Use an unsecured apartment for security
		IUnsecuredApartment* pUnsecApp = NULL;

		hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
			CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
			(void**)&pUnsecApp);

		EventSink* pSink = new EventSink;
		pSink->AddRef();

		IUnknown* pStubUnk = NULL;
		pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

		IWbemObjectSink* pStubSink = NULL;
		pStubUnk->QueryInterface(IID_IWbemObjectSink,
			(void **)&pStubSink);

		// The ExecNotificationQueryAsync method will call
		// The EventQuery::Indicate method when an event occurs
		hres = pSvc->ExecNotificationQueryAsync(
			_bstr_t("WQL"),
			_bstr_t("SELECT * "
				"FROM __InstanceDeletionEvent WITHIN 1 " // "FROM __InstanceCreationEvent WITHIN 1 " // 
				"WHERE TargetInstance ISA 'Win32_Process'"),
			WBEM_FLAG_SEND_STATUS,
			NULL,
			pStubSink);



		// Check for errors.
		if (FAILED(hres))
		{
			printf("ExecNotificationQueryAsync failed "
				"with = 0x%X\n", hres);
			pSvc->Release();
			pLoc->Release();
			pUnsecApp->Release();
			pStubUnk->Release();
			pSink->Release();
			pStubSink->Release();
			CoUninitialize();
			return 1;
		}

		hres = pSvc->ExecNotificationQueryAsync(
			_bstr_t("WQL"),
			_bstr_t("SELECT * "
				"FROM __InstanceCreationEvent WITHIN 1 " // 
				"WHERE TargetInstance ISA 'Win32_Process'"),
			WBEM_FLAG_SEND_STATUS,
			NULL,
			pStubSink);

		// Check for errors.
		if (FAILED(hres))
		{
			printf("ExecNotificationQueryAsync failed "
				"with = 0x%X\n", hres);
			pSvc->Release();
			pLoc->Release();
			pUnsecApp->Release();
			pStubUnk->Release();
			pSink->Release();
			pStubSink->Release();
			CoUninitialize();
			return 1;
		}
		// Wait for the event
		while (!g_shutdown)
			Sleep(10000);

		hres = pSvc->CancelAsyncCall(pStubSink);

		// Cleanup
		// ========

		pSvc->Release();
		pLoc->Release();
		pUnsecApp->Release();
		pStubUnk->Release();
		pSink->Release();
		pStubSink->Release();
		CoUninitialize();
	}

	return 0;   // Program successfully completed.

}

