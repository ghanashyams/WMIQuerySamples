// EventSink.cpp
#include "stdafx.h"
#include "eventsink.h"

ULONG EventSink::AddRef()
{
	return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
	LONG lRef = InterlockedDecrement(&m_lRef);
	if (lRef == 0)
		delete this;
	return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
	{
		*ppv = (IWbemObjectSink *) this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else return E_NOINTERFACE;
}


HRESULT EventSink::Indicate(long lObjectCount,
	IWbemClassObject **apObjArray)
{
	HRESULT hres = S_OK;

	HRESULT hr = S_OK;
	_variant_t vtProp;

	for (int i = 0; i < lObjectCount; i++)
	{

		VARIANT v; 
		BSTR strClassProp = SysAllocString(L"__CLASS");//(L"__CLASS");
		HRESULT hr;

		hr = apObjArray[i]->Get(strClassProp, 0, &v, 0, 0);
		SysFreeString(strClassProp);
		_bstr_t ParentClassType(v.bstrVal);
		VariantClear(&v);

		hr = apObjArray[i]->Get(_bstr_t(L"TargetInstance"), 0, &vtProp, 0, 0);
		if (!FAILED(hr))
		{
			IUnknown* str = vtProp; bool isProcessQuery = false;
			hr = str->QueryInterface(IID_IWbemClassObject, reinterpret_cast< void** >(&apObjArray[i]));
			if (SUCCEEDED(hr))
			{
				_variant_t cn;
				hr = apObjArray[i]->Get(L"__CLASS", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
						wcout << "__CLASS : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << '\t';
					else
						if (_bstr_t(cn.bstrVal) == _bstr_t(L"Win32_Process"))
							isProcessQuery = true;
				}
				VariantClear(&cn);

				if (isProcessQuery)
				{
					if (ParentClassType == _bstr_t(L"__InstanceDeletionEvent"))
						wcout << "PROCEND : " << '\t';
					else if (ParentClassType == _bstr_t(L"__InstanceCreationEvent"))
						wcout << "PROCSTART : " << '\t';

					hr = apObjArray[i]->Get(L"Name", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "Name : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << '\t';
						else
							wcout << "Name : " << cn.bstrVal << '\t';
					}
					VariantClear(&cn);

					hr = apObjArray[i]->Get(L"ProcessId", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "ProcessId : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << '\t';
						else
							wcout << "ProcessId : " << cn.intVal << '\t';
					}
					VariantClear(&cn);

					hr = apObjArray[i]->Get(L"ParentProcessId", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "ParentPId : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << '\t';
						else
							wcout << "ParentPId : " << cn.intVal << '\t';
					}
					VariantClear(&cn);
#ifdef one
					hr = apObjArray[i]->Get(L"ExecutionState", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "ExecutionState : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << '\t';
						else
							wcout << "ExecutionState : " << cn.iVal << '\t';
					}
					VariantClear(&cn);

					hr = apObjArray[i]->Get(L"Status", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "Status : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
						else
							wcout << "Status : " << cn.bstrVal << endl << endl;
					}
					VariantClear(&cn);
#endif
					hr = apObjArray[i]->Get(L"CommandLine", 0, &cn, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
							wcout << "CommandLine : " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl << endl;
						else
							wcout << "CommandLine : " << cn.bstrVal << endl << endl;
					}
					VariantClear(&cn);

					str->Release();
				}
			}

		}
		VariantClear(&vtProp);

	}

/*	for (int i = 0; i < lObjectCount; i++)
	{
		VARIANT v;
		BSTR strClassProp = SysAllocString(L"TargetInstance");//(L"__CLASS");
		HRESULT hr;

		hr = apObjArray[i]->Get(strClassProp, 0, &v, 0, 0);
		SysFreeString(strClassProp);
		
		// check the HRESULT to see if the action succeeded.
		if (SUCCEEDED(hr) && (V_VT(&v) == VT_BSTR))
		{
			wprintf(L"The class name is %s\n.", V_BSTR(&v));
		}
		else
		{
			wprintf(L"Error in getting specified object\n");
		}
		VariantClear(&v);
		printf("Event occurred\n");
	}
*/
	return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
	/* [in] */ LONG lFlags,
	/* [in] */ HRESULT hResult,
	/* [in] */ BSTR strParam,
	/* [in] */ IWbemClassObject __RPC_FAR *pObjParam
)
{
	if (lFlags == WBEM_STATUS_COMPLETE)
	{
		printf("Call complete. hResult = 0x%X\n", hResult);
	}
	else if (lFlags == WBEM_STATUS_PROGRESS)
	{
		printf("Call in progress.\n");
	}

	return WBEM_S_NO_ERROR;
}    // end of EventSink.cpp