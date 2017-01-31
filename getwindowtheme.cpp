// 25 january 2017
#include "barspy.hpp"

// oops, uxtheme.dll stores the strings you pass into SetWindowTheme() (and by extension, the CCM_SETWINDOWTHEME family of messages) in the local atom table
// there does not seem to be a way to inspect the local atom table of another process by...
// ...I hoped I would never have a need to do this but oh well
// oh yeah also UNDOCUMENTED STUFF AHOY
// the nature of uxtheme's storage of the window theme strings is undocumented, SOOOOO

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

static const char *fnGANW = "GetAtomNameW";
static const char *fnGLE = "GetLastError";

static WCHAR *runThread(Process *p, const struct archInfo *ai, void *pCode, void *pData)
{
	HANDLE hThread;
	WCHAR *out;
	DWORD ret;
	DWORD le;

	hThread = p->CreateThread(pCode, pData);
	// TODO switch to MsgWaitForMultipleObjectsEx()? this code assumes it is atomic with regards to the UI
	if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		panic(L"error waiting for process string thread to run: %I32d", GetLastError());
	if (CloseHandle(hThread) == 0)
		panic(L"error closing thread: %I32d", GetLastError());

	p->Read(pData, ai->offRet, &ret, ai->sizeRet);
	p->Read(pData, ai->offLastError, &le, ai->sizeLastError);
	// TODO make sure this logic is right
	if (ret == 0 && le != ERROR_SUCCESS)
		return NULL;
	out = new WCHAR[ret + 1];
	p->Read(pData, ai->structSize, out, (ret + 1) * sizeof (WCHAR));
	return out;
}

void getWindowTheme(HWND hwnd, Process *p, WCHAR **pszSubAppName, WCHAR **pszSubIdList)
{
	HANDLE hSubAppName, hSubIdList;
	ATOM atom;
	int arch;
	const struct archInfo *ai;
	LPVOID pCode, pData;
	void *pkernel32;
	uint32_t off32;
	uint64_t off64;
	void *off;

	*pszSubAppName = NULL;
	*pszSubIdList = NULL;
	hSubAppName = GetPropW(hwnd, MAKEINTATOM(PROP_SUBAPPNAME));
	hSubIdList = GetProp(hwnd, MAKEINTATOM(PROP_SUBIDLIST));
	if (hSubAppName == NULL && hSubIdList == NULL)
		// SetWindowTheme() wasn't called
		return;

	arch = arch386;
	if (p->Is64Bit())
		arch = archAMD64;
	ai = &(archInfo[arch]);

	pCode = p->AllocBlock(ai->nCall);
	p->Write(pCode, 0, ai->call, ai->nCall);
	p->MakeExecutable(pCode, ai->nCall);

	// have some extra padding at the end of the string, just in case
	pData = p->AllocBlock(ai->structSize + UXPROPSTRINGSIZE + 8);

	pkernel32 = p->GetModuleBase(L"kernel32.dll");

	off = p->GetProcAddress(pkernel32, "GetAtomNameW");
	switch (arch) {
	case arch386:
		off32 = (uint32_t) off;
		off = &off32;
		break;
	case archAMD64:
		off64 = (uint64_t) off;
		off = &off64;
		break;
	}
	p->Write(pData, ai->offGANW, off, ai->sizeGANW);
	off = p->GetProcAddress(pkernel32, "GetLastError");
	switch (arch) {
	case arch386:
		off32 = (uint32_t) off;
		off = &off32;
		break;
	case archAMD64:
		off64 = (uint64_t) off;
		off = &off64;
		break;
	}
	p->Write(pData, ai->offGLE, off, ai->sizeGLE);

	switch (arch) {
	case arch386:
		off32 = (uint32_t) (((size_t) pData) + ai->structSize);
		break;
	case archAMD64:
		off64 = (uint64_t) (((size_t) pData) + ai->structSize);
		break;
	}
	p->Write(pData, ai->offBuf, off, ai->sizeBuf);

	if (hSubAppName != NULL) {
		atom = (ATOM) hSubAppName;
		p->Write(pData, ai->offAtom, &atom, ai->sizeAtom);
		*pszSubAppName = runThread(p, ai, pCode, pData);
	}

	if (hSubIdList != NULL) {
		atom = (ATOM) hSubIdList;
		p->Write(pData, ai->offAtom, &atom, ai->sizeAtom);
		*pszSubIdList = runThread(p, ai, pCode, pData);
	}

	p->FreeBlock(pData);
	p->FreeBlock(pCode);
}
