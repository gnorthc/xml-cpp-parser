#pragma once
#include <string>
#include "tcvt.h"

#ifndef _nf
#define _nf ((size_t)-1)
#endif
namespace aqx {

	namespace encode_adaptive {


		static constexpr auto unknow{ static_cast<int>(-1) };
		static constexpr auto sys{ static_cast<int>(0) };
		static constexpr auto utf16{ static_cast<int>(1) };
		static constexpr auto utf8{ static_cast<int>(2) };
		static int profile_predict(unsigned char *_Text, size_t _Size, int &_Off, int _Def = 0) {

			if (_Size >= 3) {
				if (_Text[0] == 0xEF &&
					_Text[1] == 0xBB &&
					_Text[2] == 0xBF) {
					_Off = 3;
					return 2;
				}
			}
			if (_Size >= 2) {
				if (_Text[0] == 0xFF && _Text[1] == 0xFE) {
					_Off = 2;
					return 1;
				}
			}

			_Off = 0;
			size_t s = _Size;
			if (s > 0x10)
				s = 0x10;
			int x = 0;
			for (size_t i = 0; i < s; i++) {
				if (_Text[i] == 0)
					x++;
			}

			if (_Size == s && x == 1)
				return _Def;
			if (!x)
				return _Def;
			return 1;
		}

		template<typename _Ty>
		static int profile_adaptive(char *_Text, size_t _Size, _Ty &_Result, int _Def = 0) {
			int _StartOff = 0;
			int _SrcCode = encode_adaptive::profile_predict((unsigned char*)_Text, _Size, _StartOff, _Def);
			size_t _TargetCode = 0;
			if (sizeof(decltype(*_Result.c_str())) == 2)
				_TargetCode = 1;
			std::wstring _utf16;
			if (_SrcCode == 2) 
				aqx::utf16_from_utf8(_utf16, _Text + _StartOff);
			else if (_SrcCode == 1) 
				_utf16 = (wchar_t*)(_Text + _StartOff);
			else
				aqx::utf16_from_asc(_utf16, _Text + _StartOff);
			auto _proc0 = [](void *_Res, std::wstring &_wstr) { asc_from_utf16(*(std::string*)_Res, _wstr); };
			auto _proc1 = [](void *_Res, std::wstring &_wstr) { *(std::wstring*)(_Res) = _wstr; };
			auto _proc2 = [](void *_Res, std::wstring &_wstr) { aqx::utf8_from_utf16(*(std::string*)(_Res), _wstr); };

			if (_TargetCode == 0) 
				_proc0(&_Result, _utf16);
			else 
				_proc1(&_Result, _utf16);
			return _SrcCode;
		}

		template<typename _Ty>
		static size_t specifiy(char *_Text, int _Srcec, int _Targetec, _Ty &_Result) {
			if (sizeof(_Ty::_Mybase::_Alty::value_type) == 1 && _Targetec == 1) 
				return _nf;
			if (sizeof(_Ty::_Mybase::_Alty::value_type) == 2 && _Targetec != 1)
				return _nf;
			if (_Srcec == 2) {
				
				if (_Targetec == 2)
				{
					*(std::string*)&_Result = (_Text);
					return _Result.length();
				}
				else if (_Targetec == 1) 
					return utf16_from_utf8(*(std::wstring*)&_Result, _Text);
				else
					return asc_from_utf8(*(std::string*)&_Result, _Text);

			}
			else if (_Srcec == 1)
			{
				if (_Targetec == 2)
					return utf8_from_utf16(*(std::string*)&_Result, (wchar_t*)_Text);
				else if (_Targetec == 1) {
					*(std::wstring*)&_Result = (wchar_t*)(_Text);
					return _Result.length();
				}
				else
					return asc_from_utf16(*(std::string*)&_Result, (wchar_t*)_Text);
			}
			else
			{
				if(_Targetec == 2)
					return utf8_from_asc(*(std::string*)&_Result, _Text);
				else if(_Targetec == 1)
					return utf16_from_asc(*(std::wstring*)&_Result, _Text);
				else{
					*(std::string*)&_Result = (_Text);
					return _Result.length();
				}
			}
			return _nf;
		}


		static void unknow_append(void *_Res, std::string _Str) { *(std::string*)(_Res) += _Str; }
		static void unknow_wappend(void *_Res, std::wstring _Str) { *(std::wstring*)(_Res) += _Str; }





		static int xmlec_nbom_wchar(wchar_t *_Text, size_t _Size) {
			if (_Size < 7) return -1;//小于7字节的xml文档是不成立的
			auto p = wcschr(_Text, L'<');
			if (!p) return -1;
			if (p[1] == L'?') {
				if (p != _Text) return -3;//xml声明没有位于xml文件头部
				p = wcsstr(_Text + 2, L"?>");
				if (!p) return -4;//没有找到xml声明结尾
			}
			return 1;
		}

		static int xmlec_nbom_char(char *_Text, size_t _Size) {
			auto p = strchr(_Text, '<');
			if (!p) return -1;

			if (!p[1]) //找到第一个<，如果他它之后一个字符是0，则考虑它是不是utf16
			{
				if (p - _Text == _Size - 1) return -2;//如果它已经是字符串最后一个有效字符，直接报错。
				if (_Size % 2) return -2; //长度不是偶数，说明绝对不可能是utf16
				return xmlec_nbom_wchar((wchar_t*)_Text, (_Size >> 1));
			}

			if (p[1] == '?') {
				if (p != _Text) return -3;//xml声明没有位于xml文件头部
				p = strstr(_Text + 2, "?>");
				if (!p) return -4;//没有找到xml声明结尾
				auto s = (p - _Text) + 2;
				std::string str(_Text, p - _Text + 2);
				std::transform(str.begin(), str.end(), str.begin(), toupper);
				if (str.find("UTF-8") != _nf) return 2;
				if (str.find("GBK") != _nf) return 0;
				if (str.find("GB2312") != _nf) return 0;
			}

			return 2;
		}

		static int xmlec_predict(char *_Text, size_t _Size, int *err_number, size_t *_Off = NULL, int _Default = 2) {
			*err_number = 0;
			if (_Size < 7) {
				//小于7字节的xml文档是不成立的
				*err_number = 29;
				return -1;
			}

			//先基于bom判断
			if ((unsigned char)(_Text[0]) == 0xEF && (unsigned char)(_Text[1]) == 0xBB && (unsigned char)(_Text[2]) == 0xBF) {
				if (_Off) *_Off = 3;
				auto p = strchr(_Text + 3, '<');
				if (!p) {
					*err_number = 29;
					return -1;
				}

				if (p[1] == '?') 
				{
					if (p != _Text + 3) {
						
						*err_number = 31;
						return -1;
					}
					p = strstr(_Text + 5, "?>");
					if (!p) {
						*err_number = 32;
						return -1;
					}
				}

				return 2;
			}
			else if ((unsigned char)(_Text[0]) == 0xFF && (unsigned char)(_Text)[1] == 0xFE) {
				if (_Off) *_Off = 2;
				auto p = wcschr((wchar_t*)_Text + 1, L'<');
				if (!p) {
					*err_number = 29;
					return -1;
				}

				if (p[1] == L'?')
				{
					if (p != (wchar_t*)_Text + 1) {
						*err_number = 31;
						return -1;
					}
					p = wcsstr((wchar_t*)_Text + 3, L"?>");
					if (!p) {
						*err_number = 32;
						return -1;
					}
						
				}
				return 1;
			}

			if (_Off) *_Off = 0;
			int n = xmlec_nbom_char(_Text, _Size);
			if (n < -1) {

				if (n == -2)
					*err_number = 29;
				else if (n == -3)
					*err_number = 31;
				else if (n == -4)
					*err_number = 32;
				
				return -1;
			}
			else if (n >= 0) return n;
			if (!(_Size % 2))
				n = xmlec_nbom_wchar((wchar_t*)_Text, (_Size >> 1));

			if (n < -1) {

				if (n == -2)
					*err_number = 29;
				else if (n == -3)
					*err_number = 31;
				else if (n == -4)
					*err_number = 32;

				return -1;
			}

			return _Default;
		}


	};


}
