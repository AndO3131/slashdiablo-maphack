
#include "eventsink.h"
#include "BH.Injector.h"


BOOL CALLBACK EnumWindowsProcAutoInject(HWND hwnd, LPARAM lParam) {
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	wchar_t szClassName[1024];
	GetClassName(hwnd, szClassName, 1024);

	//Check if it is Diablo II
	if (!wcscmp(szClassName, L"Diablo II")) {
		if (lpdwProcessId == lParam) {
			std::cout << std::endl << "Diablo 2 instance found. Injecting..." << std::endl;

			HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, lpdwProcessId);

			DiabloWindow dw = DiabloWindow(hwnd);
			dw.Inject();
			return FALSE;
		}
	}
	return TRUE;
}


HRESULT EventSink::Indicate(long lObjectCount,
	IWbemClassObject **apObjArray) {
	HRESULT hr = S_OK;
	_variant_t vtProp;
	long processID;

	// Take TargetInstance.Handle out of this event and send it to EnumWindowsProcAutoInject as the process id to check.
	for (int i = 0; i < lObjectCount; i++) {
		hr = apObjArray[i]->Get(_bstr_t(L"TargetInstance"), 0, &vtProp, 0, 0);
		if (!FAILED(hr)) {
			IUnknown* str = vtProp;
			hr = str->QueryInterface(IID_IWbemClassObject, reinterpret_cast< void** >(&apObjArray[i]));
			if (SUCCEEDED(hr)) {
				_variant_t cn;
				hr = apObjArray[i]->Get(L"Handle", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr)) {
					if ((cn.vt != VT_NULL) && (cn.vt != VT_EMPTY)) {
						processID = _wtol(cn.bstrVal);
						EnumWindows(EnumWindowsProcAutoInject, processID);
					}
				}
				VariantClear(&cn);
			}
		}
		VariantClear(&vtProp);
	}

	return WBEM_S_NO_ERROR;
}

ULONG EventSink::AddRef() {
	return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release() {
	LONG lRef = InterlockedDecrement(&m_lRef);
	if (lRef == 0)
		delete this;
	return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv) {
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
		*ppv = (IWbemObjectSink *) this;
		AddRef();
		return WBEM_S_NO_ERROR;
	} else return E_NOINTERFACE;
}

HRESULT EventSink::SetStatus(
	/* [in] */ LONG lFlags,
	/* [in] */ HRESULT hResult,
	/* [in] */ BSTR strParam,
	/* [in] */ IWbemClassObject __RPC_FAR *pObjParam
	) {
	return WBEM_S_NO_ERROR;
}