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

#define PROP_SUBAPPNAME 0xA911
#define PROP_SUBIDLIST 0xA910
// this is hardcoded into the above binary blobs
#define UXPROPSTRINGSIZE 260

static ProcessHelper *mkProcessHelper(Process *p)
{
	ProcessHelper *ph;

	ph = new ProcessHelper(p);
	ph->SetCode(call386, nCall386, callAMD64, nCallAMD64);
	ph->AddField("GetAtomNameWPtr", fieldPointer, 0, 4, 0, 8);
	ph->AddField("GetLastErrorPtr", fieldPointer, 4, 4, 8, 8);
	ph->AddField("atom", fieldATOM, 8, 2, 16, 2);
	ph->AddField("buf", fieldPointer, 12, 4, 24, 8);
	ph->AddField("ret", fieldUINT, 16, 4, 32, 4);
	ph->AddField("lastError", fieldDWORD, 20, 4, 36, 4);
	// extra just to be safe
	ph->SetExtraDataSize(UXPROPSTRINGSIZE + 8);
	return ph;
}

static WCHAR *runThread(ProcessHelper *ph, ATOM atom)
{
	UINT ret;
	DWORD lastError;

	ph->WriteField("atom", atom);
	ph->Run();

	ph->ReadField("ret", &ret);
	ph->ReadField("lastError", &lastError);
	// TODO make sure this logic is right
	if (ret == 0 && lastError != ERROR_SUCCESS)
		return NULL;
	return (WCHAR *) ph->ReadExtraData();
}

void getWindowTheme(HWND hwnd, Process *p, WCHAR **pszSubAppName, WCHAR **pszSubIdList)
{
	HANDLE hSubAppName, hSubIdList;
	ATOM atom;
	ProcessHelper *ph;
	void *pkernel32;

	*pszSubAppName = NULL;
	*pszSubIdList = NULL;
	hSubAppName = GetPropW(hwnd, MAKEINTATOM(PROP_SUBAPPNAME));
	hSubIdList = GetProp(hwnd, MAKEINTATOM(PROP_SUBIDLIST));
	if (hSubAppName == NULL && hSubIdList == NULL)
		// SetWindowTheme() wasn't called
		return;

	ph = mkProcessHelper(p);

	pkernel32 = p->GetModuleBase(L"kernel32.dll");

	ph->WriteFieldProcAddress("GetAtomNameWPtr", pkernel32, "GetAtomNameW");
	ph->WriteFieldProcAddress("GetLastErrorPtr", pkernel32, "GetLastError");
	ph->WriteFieldPointer("buf", ph->ExtraDataPtr());

	if (hSubAppName != NULL) {
		atom = (ATOM) hSubAppName;
		*pszSubAppName = runThread(ph, atom);
	}

	if (hSubIdList != NULL) {
		atom = (ATOM) hSubIdList;
		*pszSubIdList = runThread(ph, atom);
	}

	delete ph;
}
