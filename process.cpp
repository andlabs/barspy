// 26 january 2017
#include "barspy.hpp"

// Normally I would only have needed a way to allocate, read, and write process memory to get structures out of controls.
// The other stuff — the code injection stuff — ... well, just check gettheme.cpp for info.

// might not be available on all systems
static BOOL (WINAPI *pIsWow64Process)(HANDLE, BOOL *) = NULL;
static BOOL (WINAPI *pEnumProcessModulesEx)(HANDLE hProcess, HMODULE *, DWORD, LPDWORD, DWORD) = NULL;

// do not check errors; instead treat any errors as "not found" (this is the safest, cleanest way)
void initProcess(void)
{
	HMODULE kernel32;
	HMODULE psapi;

	kernel32 = LoadLibraryW(L"kernel32.dll");
	if (kernel32 != NULL)
		pIsWow64Process = (BOOL (WINAPI *)(HANDLE, BOOL *)) GetProcAddress(kernel32, "IsWow64Process");

	psapi = LoadLibraryW(L"psapi.dll");
	if (psapi != NULL)
		pEnumProcessModulesEx = (BOOL (WINAPI *)(HANDLE hProcess, HMODULE *, DWORD, LPDWORD, DWORD)) GetProcAddress(psapi, "EnumProcessModulesEx");
}

Process::Process(DWORD pid)
{
	this->pid = pid;
	this->hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, this->pid);
	if (this->hProc == NULL)
		panic(L"failed to open process: %I32d", GetLastError());
}

Process::~Process(void)
{
	if (CloseHandle(this->hProc) == 0)
		panic(L"failed to close process: %I32d", GetLastError());
}

// note: I did come up with this algorithm before reading what I'm about to link, but here's confirmation that this is indeed the right way to go http://stackoverflow.com/a/14186661/3408572
bool Process::Is64Bit(void)
{
	SYSTEM_INFO si;

	// first find out if we're a 32-bit process on a 64-bit system
	if (pIsWow64Process != NULL) {
		BOOL is;

		if ((*pIsWow64Process)(this->hProc, &is) == FALSE)
			// TODO should we pretend is is FALSE here?
			panic(L"error calling IsWow64Process(): %I32d", GetLastError());
		if (is != FALSE)
			// yep, we're 32-bit on 64-bit
			return false;
	}

	// that only tells us if the process was 32-bit on a 64-bit system
	// for both 32-bit native and 64-bit native we need to ask the OS what it is
	// TODO what if PROCESSOR_ARCHITECTURE_UNKNOWN?
	GetNativeSystemInfo(&si);
	return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
}

void *Process::AllocBlock(size_t size)
{
	void *ret;

	ret = VirtualAllocEx(this->hProc,
		NULL, size,
		MEM_COMMIT, PAGE_READWRITE);
	if (ret == NULL)
		panic(L"error allocating block in process: %I32d", GetLastError());
	return ret;
}

void Process::FreeBlock(void *block)
{
	if (VirtualFreeEx(this->hProc, block, 0, MEM_RELEASE) == 0)
		panic(L"error freeing block from process: %I32d", GetLastError());
}

void Process::MakeExecutable(void *block, size_t size)
{
	// TODO I had a TODO here but I don't remember what for...
	DWORD unused;

	if (VirtualProtectEx(this->hProc, block, size,
		PAGE_EXECUTE_READ, &unused) == 0)
		panic(L"error marking block in process as executable: %I32d", GetLastError());
	if (FlushInstructionCache(this->hProc, block, size) == 0)
		panic(L"error flushing instruction cache in process: %I32d", GetLastError());
}

void Process::Read(void *base, size_t off, void *buf, size_t len)
{
	uintptr_t b;
	SIZE_T actual;

	b = (uintptr_t) base;
	b += off;
	base = (void *) b;
	if (ReadProcessMemory(this->hProc, base,
		buf, len, &actual) == 0)
		panic(L"error reading process memory: %I32d", GetLastError());
	if (len != actual)
		panic(L"ReadProcessMemory() failed to read everything; expected %ju, got %ju", len, actual);
}

void Process::Write(void *base, size_t off, const void *buf, size_t len)
{
	uintptr_t b;
	SIZE_T actual;

	b = (uintptr_t) base;
	b += off;
	base = (void *) b;
	if (WriteProcessMemory(this->hProc, base,
		buf, len, &actual) == 0)
		panic(L"error writing into process memory: %I32d", GetLastError());
	if (len != actual)
		panic(L"WriteProcessMemory() failed to write everything; expected %ju, got %ju", len, actual);
}

#define ourLIST_MODULES_ALL 0x03

static void callEnumProcessModules(HANDLE hProc, HMODULE *mods, DWORD cb, LPDWORD needed)
{
	if (pEnumProcessModulesEx != NULL) {
		if ((*pEnumProcessModulesEx)(hProc, mods, cb, needed, ourLIST_MODULES_ALL) == 0)
			panic(L"EnumProcessModulesEx() failed: %I32d", GetLastError());
		return;
	}
	if (EnumProcessModules(hProc, mods, cb, needed) == 0)
		panic(L"EnumProcessModules() failed: %I32d", GetLastError());
}

void *Process::GetModuleBase(const WCHAR *modname)
{
	HMODULE *mods;
	DWORD count, needed;
	DWORD i;
	WCHAR basename[MAX_PATH];		// from MSDN samples, meh, this will do
	MODULEINFO mi;

	count = 24;
	for (;;) {
		mods = new HMODULE[count];
		callEnumProcessModules(this->hProc, mods,
			count * sizeof (HMODULE), &needed);
		if ((count * sizeof (HMODULE)) == needed)
			break;
		delete[] mods;
		count = needed / sizeof (HMODULE);
	}
	for (i = 0; i < count; i++) {
		if (GetModuleBaseNameW(this->hProc, mods[i], basename, MAX_PATH) == 0)
			panic(L"GetModuleBaseNameW() failed: %I32d", GetLastError());
		if (_wcsicmp(basename, modname) == 0)
			break;
	}
	if (i == count)			// not found
		return NULL;
	// TODO is this step even necessary? or is (void *) (mods[i]) enough, given MSDN and other sources?
	if (GetModuleInformation(this->hProc, mods[i], &mi, sizeof (MODULEINFO)) == 0)
		panic(L"GetModuleInformation() failed: %I32d", GetLastError());
	return mi.lpBaseOfDll;
}

// see also https://www.codeproject.com/tips/139349/getting-the-address-of-a-function-in-a-dll-loaded
// TODO this is clumsy
void *Process::GetProcAddress(void *modbase, const char *procname)
{
	size_t nName;
	char *namebuf;
	size_t nextPos;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_FILE_HEADER fileHeader;
	IMAGE_DATA_DIRECTORY exportPos;
	IMAGE_EXPORT_DIRECTORY exports;
	DWORD *funcs;
	DWORD *names;
	WORD *ords;
	DWORD i;

	nName = strlen(procname);
	namebuf = new char[nName + 1];

	nextPos = 0;
	this->Read(modbase, nextPos, &dosHeader, sizeof (IMAGE_DOS_HEADER));
	nextPos += dosHeader.e_lfanew + sizeof (DWORD);
	this->Read(modbase, nextPos, &fileHeader, sizeof (IMAGE_FILE_HEADER));
	nextPos += sizeof (IMAGE_FILE_HEADER);
	if (fileHeader.SizeOfOptionalHeader == sizeof (IMAGE_OPTIONAL_HEADER64)) {
		IMAGE_OPTIONAL_HEADER64 oh;

		this->Read(modbase, nextPos, &oh, sizeof (IMAGE_OPTIONAL_HEADER64));
		exportPos = oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	} else {
		IMAGE_OPTIONAL_HEADER32 oh;

		this->Read(modbase, nextPos, &oh, sizeof (IMAGE_OPTIONAL_HEADER32));
		exportPos = oh.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	}
	nextPos = exportPos.VirtualAddress;
	this->Read(modbase, nextPos, &exports, sizeof (IMAGE_EXPORT_DIRECTORY));

	funcs = new DWORD[exports.NumberOfFunctions];
	nextPos = exports.AddressOfFunctions;
	this->Read(modbase, nextPos, funcs, exports.NumberOfFunctions * sizeof (DWORD));
	names = new DWORD[exports.NumberOfNames];
	nextPos = exports.AddressOfNames;
	this->Read(modbase, nextPos, names, exports.NumberOfNames * sizeof (DWORD));
	ords = new WORD[exports.NumberOfNames];
	nextPos = exports.AddressOfNameOrdinals;
	this->Read(modbase, nextPos, ords, exports.NumberOfNames * sizeof (WORD));
	for (i = 0; i < exports.NumberOfNames; i++) {
		this->Read(modbase, names[i], namebuf, (nName + 1) * sizeof (char));
		// TODO handle forwards
		if (memcmp(namebuf, procname, (nName + 1) * sizeof (char)) == 0)
			break;
	}

	delete[] ords;
	delete[] names;
	delete[] funcs;
	delete[] namebuf;
	if (i == exports.NumberOfNames)
		return NULL;
	return (void *) (((uintptr_t) modbase) + funcs[ords[i]]);
}

HANDLE Process::CreateThread(void *threadProc, void *param)
{
	HANDLE hThread;

	hThread = CreateRemoteThread(this->hProc, NULL, 0,
		(LPTHREAD_START_ROUTINE) threadProc, param, 0, NULL);
	if (hThread == NULL)
		panic(L"error creating thread in process: %I32d", GetLastError());
	return hThread;
}

Process *processFromHWND(HWND hwnd)
{
	DWORD pid;

	GetWindowThreadProcessId(hwnd, &pid);
	return new Process(pid);
}
