// 25 january 2017
#include "barspy.hpp"

// oops, uxtheme.dll stores the strings you pass into SetWindowTheme() (and by extension, the CCM_SETWINDOWTHEME family of messages) in the local atom table
// there does not seem to be a way to inspect the local atom table of another process by...
// ...I hoped I would never have a need to do this but oh well

static const uint8_t call386[] = {
	0x55, 0x8B, 0xEC, 0x51, 0x8B, 0x45, 0x08, 0x89,
	0x45, 0xFC, 0x68, 0x04, 0x01, 0x00, 0x00, 0x8B,
	0x4D, 0xFC, 0x8B, 0x51, 0x0C, 0x52, 0x8B, 0x45,
	0xFC, 0x0F, 0xB7, 0x48, 0x08, 0x51, 0x8B, 0x55,
	0xFC, 0x8B, 0x02, 0xFF, 0xD0, 0x8B, 0x4D, 0xFC,
	0x89, 0x41, 0x10, 0x8B, 0x55, 0xFC, 0x8B, 0x42,
	0x04, 0xFF, 0xD0, 0x8B, 0x4D, 0xFC, 0x89, 0x41,
	0x14, 0x33, 0xC0, 0x8B, 0xE5, 0x5D, 0xC2, 0x04,
	0x00,
};
static const size_t nCall386 = 65;

static const uint8_t callAMD64[] = {
	0x48, 0x89, 0x4C, 0x24, 0x08, 0x48, 0x83, 0xEC,
	0x38, 0x48, 0x8B, 0x44, 0x24, 0x40, 0x48, 0x89,
	0x44, 0x24, 0x20, 0x41, 0xB8, 0x04, 0x01, 0x00,
	0x00, 0x48, 0x8B, 0x44, 0x24, 0x20, 0x48, 0x8B,
	0x50, 0x18, 0x48, 0x8B, 0x44, 0x24, 0x20, 0x0F,
	0xB7, 0x48, 0x10, 0x48, 0x8B, 0x44, 0x24, 0x20,
	0xFF, 0x10, 0x48, 0x8B, 0x4C, 0x24, 0x20, 0x89,
	0x41, 0x20, 0x48, 0x8B, 0x44, 0x24, 0x20, 0xFF,
	0x50, 0x08, 0x48, 0x8B, 0x4C, 0x24, 0x20, 0x89,
	0x41, 0x24, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x38,
	0xC3,
};
static const size_t nCallAMD64 = 81;

struct archInfo {
	const uint8_t *call;
	size_t nCall;
	size_t structSize;
	size_t offGANW;
	size_t sizeGANW;
	size_t offGLE;
	size_t sizeGLE;
	size_t offAtom;
	size_t sizeAtom;
	size_t offBuf;
	size_t sizeBuf;
	size_t offRet;
	size_t sizeRet;
	size_t offLastError;
	size_t sizeLastError;
};

const struct archInfo archInfo[] = {
#define arch386 0
	{
		call386, nCall386,
		24,
		0, 4,
		4, 4,
		8, 2,
		12, 4,
		16, 4,
		20, 4,
	},
#define archAMD64 1
	{
		callAMD64, nCallAMD64,
		40,
		0, 8,
		8, 8,
		16, 2,
		24, 8,
		32, 4,
		36, 4,
	},
};

#define PROP_SUBAPPNAME 0xA911
#define PROP_SUBIDLIST 0xA910
// this is hardcoded into the above binary blobs
#define UXPROPSTRINGSIZE 260

static bool isWOW64Initialized = false;
static BOOL (WINAPI *isWOW64Ptr)(HANDLE, BOOL *) = NULL;

static bool is64BitProcess(HANDLE hProc)
{
	BOOL isWOW;
	SYSTEM_INFO si;

	if (!isWOW64Initialized) {
		HMODULE h;

		// TODO is this correct?
		h = GetModuleHandleW(L"kernel32.dll");
		if (h == NULL)
			panic(L"error opening kernel32.dll for IsWow64Process(): %I32d", GetLastError());
		isWOW64Ptr = (BOOL (WINAPI *)(HANDLE, BOOL *)) GetProcAddress(h, "IsWow64Process");
		isWOW64Initialized = true;
	}
	if (isWOW64Ptr == NULL)
		// no WOW64 on this system; go to the native version check below to be safe
		goto notWOW64;
	if ((*isWOW64Ptr)(hProc, &isWOW) == FALSE)
		panic(L"error calling IsWow64Process(): %I32d", GetLastError());
	// if this is WOW64, it's 32-bit on a 64-bit system
	if (isWOW == TRUE)
		return false;
notWOW64:
	// wait, that's not good enough, because if the function is available on a 32-bit system (as it is with more recent versions of Windows), it will return FALSE unconditionally
	GetNativeSystemInfo(&si);
	return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
}

static void readpm(HANDLE hProc, void *base, size_t off, void *buf, size_t size)
{
	uintptr_t b;
	SIZE_T actual;

	b = (uintptr_t) base;
	b += off;
	base = (void *) b;
	if (ReadProcessMemory(hProc, base,
		buf, size, &actual) == 0)
		panic(L"error reading process memory: %I32d", GetLastError());
	if (size != actual)
		panic(L"ReadProcessMemory() failed to read everything; expected %ju, got %ju", size, actual);
}

static void writepm(HANDLE hProc, void *base, size_t off, const void *data, size_t size)
{
	uintptr_t b;
	SIZE_T actual;

	b = (uintptr_t) base;
	b += off;
	base = (void *) b;
	if (WriteProcessMemory(hProc, base,
		data, size, &actual) == 0)
		panic(L"error writing into process memory: %I32d", GetLastError());
	if (size != actual)
		panic(L"WriteProcessMemory() failed to write everything; expected %ju, got %ju", size, actual);
}

static const char *fnGANW = "GetAtomNameW";
static const char *fnGLE = "GetLastError";

// see also https://www.codeproject.com/tips/139349/getting-the-address-of-a-function-in-a-dll-loaded
// TODO switch to psapi?
static void findProcs(HANDLE hProc, DWORD pid, void **pGetAtomNameW, void **pGetLastError)
{
	HANDLE th;
	MODULEENTRY32W me;
	BOOL gotGANW = FALSE, gotGLE = FALSE;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_FILE_HEADER fileHeader;
	size_t nextPos;
	IMAGE_DATA_DIRECTORY exportPos;
	IMAGE_EXPORT_DIRECTORY exports;
	DWORD *funcs;
	DWORD *names;
	WORD *ords;
	DWORD i;
	DWORD le;

	th = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (th == INVALID_HANDLE_VALUE)
		panic(L"error opening toolhelp32 info to get proc addresses in process: %I32d", GetLastError());

	ZeroMemory(&me, sizeof (MODULEENTRY32W));
	me.dwSize = sizeof (MODULEENTRY32W);
	if (Module32FirstW(th, &me) == 0) {
		le = GetLastError();
		if (le == ERROR_NO_MORE_FILES)
			goto done;
		panic(L"error getting first module in process: %I32d", le);
	}
	for (;;) {
		if (_wcsicmp(me.szModule, L"kernel32.dll") == 0)
			goto found;
		ZeroMemory(&me, sizeof (MODULEENTRY32W));
		me.dwSize = sizeof (MODULEENTRY32W);
		if (Module32NextW(th, &me) == 0) {
			le = GetLastError();
			if (le == ERROR_NO_MORE_FILES)
				goto done;
			panic(L"error getting next module in process: %I32d", le);
		}
	}

found:
	nextPos = 0;
	readpm(hProc, me.modBaseAddr, nextPos, &dosHeader, sizeof (IMAGE_DOS_HEADER));
	nextPos += sizeof (IMAGE_DOS_HEADER) + dosHeader.e_lfanew + 4;
	readpm(hProc, me.modBaseAddr, nextPos, &fileHeader, sizeof (IMAGE_FILE_HEADER));
	nextPos += sizeof (IMAGE_FILE_HEADER);
	if (fileHeader.SizeOfOptionalHeader == sizeof (IMAGE_OPTIONAL_HEADER64)) {
		IMAGE_OPTIONAL_HEADER64 oh;

		readpm(hProc, me.modBaseAddr, nextPos, &oh, sizeof (IMAGE_OPTIONAL_HEADER64));
		exportPos = oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	} else {
		IMAGE_OPTIONAL_HEADER32 oh;

		readpm(hProc, me.modBaseAddr, nextPos, &oh, sizeof (IMAGE_OPTIONAL_HEADER32));
		exportPos = oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	}
	nextPos = exportPos.VirtualAddress;
	readpm(hProc, me.modBaseAddr, nextPos, &exports, sizeof (IMAGE_EXPORT_DIRECTORY));

	funcs = new DWORD[exports.NumberOfFunctions];
	nextPos = exports.AddressOfFunctions;
	readpm(hProc, me.modBaseAddr, nextPos, funcs, exports.NumberOfFunctions * sizeof (DWORD));
	names = new DWORD[exports.NumberOfNames];
	nextPos = exports.AddressOfNames;
	readpm(hProc, me.modBaseAddr, nextPos, names, exports.NumberOfNames * sizeof (DWORD));
	ords = new WORD[exports.NumberOfNames];
	nextPos = exports.AddressOfNameOrdinals;
	readpm(hProc, me.modBaseAddr, nextPos, ords, exports.NumberOfNames * sizeof (WORD));
	for (i = 0; i < exports.NumberOfNames; i++) {
		// "GetAtomNameW\0" == 13
		// "GetLastError\0" also == 13
		char name[13];

		readpm(hProc, me.modBaseAddr, names[i], name, 13 * sizeof (char));
		// TODO handle forwards
		if (!gotGANW)
			if (memcmp(name, fnGANW, 13 * sizeof (char)) == 0) {
				*pGetAtomNameW = me.modBaseAddr + funcs[ords[i]];
				gotGANW = TRUE;
			}
		if (!gotGLE)
			if (memcmp(name, fnGLE, 13 * sizeof (char)) == 0) {
				*pGetLastError = me.modBaseAddr + funcs[ords[i]];
				gotGLE = TRUE;
			}
		// did we finish?
		if (gotGANW && gotGLE)
			break;
	}
	delete[] ords;
	delete[] names;
	delete[] funcs;

done:
	if (!gotGANW)
		panic(L"couldn't find GetAtomNameW() in process");
	if (!gotGLE)
		panic(L"couldn't find GetLastError() in process");
	if (CloseHandle(th) == 0)
		panic(L"error closing toolhelp32 info for getting proc addresses in process: %I32d", GetLastError());
}

static WCHAR *runThread(HANDLE hProc, const struct archInfo *ai, void *pCode, void *pData)
{
	HANDLE hThread;
	WCHAR *out;
	DWORD ret;
	DWORD le;

	hThread = CreateRemoteThread(hProc, NULL, 0,
		(LPTHREAD_START_ROUTINE) pCode, pData, 0, NULL);
	if (hThread == NULL)
		panic(L"error creating thread to get string from process: %I32d", GetLastError());
	// TODO switch to MsgWaitForMultipleObjectsEx()? this code assumes it is atomic with regards to the UI
	if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		panic(L"error waiting for process string thread to run: %I32d", GetLastError());
	if (CloseHandle(hThread) == 0)
		panic(L"error closing thread: %I32d", GetLastError());

	readpm(hProc, pData, ai->offRet, &ret, ai->sizeRet);
	readpm(hProc, pData, ai->offLastError, &le, ai->sizeLastError);
	// TODO make sure this logic is right
	if (ret == 0 && le != ERROR_SUCCESS)
		return NULL;
	out = new WCHAR[ret + 1];
	readpm(hProc, pData, ai->structSize, out, (ret + 1) * sizeof (WCHAR));
	return out;
}

void getWindowClass(HWND hwnd, WCHAR **pszSubAppName, WCHAR **pszSubIdList)
{
	DWORD pid;
	HANDLE hProc;
	HANDLE hSubAppName, hSubIdList;
	ATOM atom;
	int arch;
	const struct archInfo *ai;
	LPVOID pCode, pData;
	DWORD unused;		// TODO
	uint32_t off32;
	uint64_t off64;
	void *off;
	void *pGetAtomNameW, *pGetLastError;

	*pszSubAppName = NULL;
	*pszSubIdList = NULL;
	hSubAppName = GetPropW(hwnd, MAKEINTATOM(PROP_SUBAPPNAME));
	hSubIdList = GetProp(hwnd, MAKEINTATOM(PROP_SUBIDLIST));
	if (hSubAppName == NULL && hSubIdList == NULL)
		// SetWindowTheme() wasn't called
		return;

	GetWindowThreadProcessId(hwnd, &pid);
	hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
	if (hProc == NULL)
		panic(L"failed to open process for getting window theme: %I32d", GetLastError());

	arch = arch386;
	if (is64BitProcess(hProc))
		arch = archAMD64;
	ai = &(archInfo[arch]);

	pCode = VirtualAllocEx(hProc,
		NULL, ai->nCall,
		MEM_COMMIT, PAGE_READWRITE);
	if (pCode == NULL)
		panic(L"error allocating room for code in process: %I32d", GetLastError());
	writepm(hProc, pCode, 0, ai->call, ai->nCall);
	if (VirtualProtectEx(hProc, pCode, ai->nCall, PAGE_EXECUTE_READ, &unused) == 0)
		panic(L"error marking code in process as executable: %I32d", GetLastError());

	// TODO fill with address of GetAtomNameW() and GetLastError()

	pData = VirtualAllocEx(hProc,
		// have some extra padding at the end of the string, just in case
		NULL, ai->structSize + UXPROPSTRINGSIZE + 8,
		MEM_COMMIT, PAGE_READWRITE);
	if (pData == NULL)
		panic(L"error allocating room for data in process: %I32d", GetLastError());

	findProcs(hProc, pid, &pGetAtomNameW, &pGetLastError);
	switch (arch) {
	case arch386:
		off32 = (uint32_t) pGetAtomNameW;
		off = &off32;
		break;
	case archAMD64:
		off64 = (uint64_t) pGetAtomNameW;
		off = &off64;
		break;
	}
	writepm(hProc, pData, ai->offGANW, off, ai->sizeGANW);
	switch (arch) {
	case arch386:
		off32 = (uint32_t) pGetLastError;
		break;
	case archAMD64:
		off64 = (uint64_t) pGetLastError;
		break;
	}
	writepm(hProc, pData, ai->offGLE, off, ai->sizeGLE);

	switch (arch) {
	case arch386:
		off32 = (uint32_t) (((size_t) pData) + ai->structSize);
		break;
	case archAMD64:
		off64 = (uint64_t) (((size_t) pData) + ai->structSize);
		break;
	}
	writepm(hProc, pData, ai->offBuf, off, ai->sizeBuf);

	if (hSubAppName != NULL) {
		atom = (ATOM) hSubAppName;
		writepm(hProc, pData, ai->offAtom, &atom, ai->sizeAtom);
		*pszSubAppName = runThread(hProc, ai, pCode, pData);
	}

	if (hSubIdList != NULL) {
		atom = (ATOM) hSubIdList;
		writepm(hProc, pData, ai->offAtom, &atom, ai->sizeAtom);
		*pszSubIdList = runThread(hProc, ai, pCode, pData);
	}

	if (VirtualFreeEx(hProc, pData, 0, MEM_RELEASE) == 0)
		panic(L"error removing data from process: %I32d", GetLastError());
	if (VirtualFreeEx(hProc, pCode, 0, MEM_RELEASE) == 0)
		panic(L"error removing code from process: %I32d", GetLastError());
	if (CloseHandle(hProc) != 0)
		panic(L"failed to close process: %I32d", GetLastError());
}
