#pragma once
#if defined(_WIN32) || defined(_WIN64)
#ifndef _WINDOWS_
#include <windows.h>
#endif
#endif

namespace aqx {

	static size_t _mbs2wcs(int _Cp, const std::string &_Mbs, std::wstring &_Wcs) {
		int n = MultiByteToWideChar(_Cp, 0, _Mbs.c_str(), (int)_Mbs.length(), nullptr, 0);
		_Wcs.resize(n);
		return MultiByteToWideChar(_Cp, 0, _Mbs.c_str(), (int)_Mbs.length(), (wchar_t*)_Wcs.data(), (int)_Wcs.capacity());
	}

	static size_t _wcs2mbs(int _Cp, const std::wstring &_Wcs, std::string &_Mbs) {
		int n = WideCharToMultiByte(_Cp, 0, _Wcs.c_str(), (int)_Wcs.length(), nullptr, 0, NULL, FALSE);
		_Mbs.resize(n);
		return WideCharToMultiByte(_Cp, 0, _Wcs.c_str(), (int)_Wcs.length(), (char*)_Mbs.data(), (int)_Mbs.capacity(), NULL, FALSE);
	}

	
	static size_t utf8_from_asc(std::string &_Result, const std::string &_Asc) {
		std::wstring _Tmp;
		_mbs2wcs(CP_ACP, _Asc, _Tmp);
		return _wcs2mbs(CP_UTF8, _Tmp, _Result);
	}

	static size_t utf16_from_asc(std::wstring &_Result, const std::string &_Asc) {
		return _mbs2wcs(CP_ACP, _Asc, _Result);
	}

	static size_t asc_from_utf8(std::string &_Result, const std::string &_U8s) {
		std::wstring _Tmp;
		_mbs2wcs(CP_UTF8, _U8s, _Tmp);
		return _wcs2mbs(CP_ACP, _Tmp, _Result);
	}

	static size_t utf16_from_utf8(std::wstring &_Result, const std::string &_U8s) {
		return _mbs2wcs(CP_UTF8, _U8s, _Result);
	}

	static size_t utf8_from_utf16(std::string &_Result, const std::wstring &_Wcs) {
		return _wcs2mbs(CP_UTF8, _Wcs, _Result);
	}

	static size_t asc_from_utf16(std::string &_Result, const std::wstring &_Wcs) {
		return _wcs2mbs(CP_ACP, _Wcs, _Result);
	}

}
