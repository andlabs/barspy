// 30 january 2017
#include "barspy.hpp"

// C++ is a language where any attempt to be specific (and thus safe) ends in compiler warnings too obtuse to understand, let alone verbose
template<typename T, typename U>
static std::wstring mkstring(T val, const U &gen)
{
	std::wostringstream ss;
	std::vector<std::wstring> names;
	bool first;

	if (val == 0)
		return std::wstring(L"0");
	names.reserve(sizeof (T) * 8);
	gen(&val, names);
	first = true;
	for (auto s : names) {
		if (!first)
			ss << L" | ";
		ss << s;
		first = false;
	}
	if (val != 0) {
		if (!first)
			ss << L" | ";
		ss << L"0x";
		ss.fill(L'0');
		ss.setf(ss.hex | ss.uppercase, ss.basefield | ss.uppercase);
		ss.width(sizeof (T) * 2);
		ss << val;
	}
	return ss.str();
}

#define XTRY(bit, str) if ((*val & bit) != 0) { names.push_back(str); *val &= ~(bit); }
#define TRY(bit) XTRY(bit, L ## #bit)
#define TRYHEAD [](decltype (v) *val, std::vector<std::wstring> &names)

#define TRYCC \
	TRY(CCS_TOP) \
	TRY(CCS_NOMOVEY) \
	TRY(CCS_NORESIZE) \
	TRY(CCS_NOPARENTALIGN) \
	TRY(CCS_ADJUSTABLE) \
	TRY(CCS_NODIVIDER) \
	TRY(CCS_VERT)

std::wstring toolbarStyleString(HWND toolbar)
{
	uint16_t v;

	v = (uint16_t) (GetWindowLongPtrW(toolbar, GWL_STYLE) & 0xFFFF);
	return mkstring(v, TRYHEAD {
		TRYCC
		TRY(TBSTYLE_TOOLTIPS)
		TRY(TBSTYLE_WRAPABLE)
		TRY(TBSTYLE_ALTDRAG)
		TRY(TBSTYLE_FLAT)
		TRY(TBSTYLE_LIST)
		TRY(TBSTYLE_CUSTOMERASE)
		TRY(TBSTYLE_REGISTERDROP)
		TRY(TBSTYLE_TRANSPARENT)
	});
}

std::wstring toolbarExStyleString(HWND toolbar)
{
	DWORD v;

	v = (DWORD) SendMessageW(toolbar, TB_GETEXTENDEDSTYLE, 0, 0);
	return mkstring(v, TRYHEAD {
		TRY(TBSTYLE_EX_DRAWDDARROWS)
		TRY(TBSTYLE_EX_MULTICOLUMN)
		TRY(TBSTYLE_EX_VERTICAL)
		TRY(TBSTYLE_EX_MIXEDBUTTONS)
		TRY(TBSTYLE_EX_HIDECLIPPEDBUTTONS)
		TRY(TBSTYLE_EX_DOUBLEBUFFER)
	});
}

std::wstring toolbarBitmapFlagsString(HWND toolbar)
{
	DWORD v;

	v = (DWORD) SendMessageW(toolbar, TB_GETBITMAPFLAGS, 0, 0);
	return mkstring(v, TRYHEAD {
		TRY(TBBF_LARGE)
	});
}

std::wstring drawTextFlagsString(DWORD v)
{
	return mkstring(v, TRYHEAD {
		TRY(DT_CENTER)
		TRY(DT_RIGHT)
		TRY(DT_VCENTER)
		TRY(DT_BOTTOM)
		TRY(DT_WORDBREAK)
		TRY(DT_SINGLELINE)
		TRY(DT_EXPANDTABS)
		TRY(DT_TABSTOP)
		TRY(DT_NOCLIP)
		TRY(DT_EXTERNALLEADING)
		TRY(DT_CALCRECT)
		TRY(DT_NOPREFIX)
		TRY(DT_INTERNAL)
		TRY(DT_EDITCONTROL)
		TRY(DT_PATH_ELLIPSIS)
		TRY(DT_END_ELLIPSIS)
		TRY(DT_MODIFYSTRING)
		TRY(DT_RTLREADING)
		TRY(DT_WORD_ELLIPSIS)
		TRY(DT_NOFULLWIDTHCHARBREAK)
		TRY(DT_HIDEPREFIX)
		TRY(DT_PREFIXONLY)
	});
}
