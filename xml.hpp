#pragma once
#include <setjmp.h>
#include <set>
#include <list>
#include <map>
#include <algorithm>
#include <fstream>
#include <string>
#include "tcvt.h"
#include "encode_adaptive.h"

namespace aqx {

	namespace aqx_internal {

#ifndef __AQX_UTF8_CHAR_LEN
#define __AQX_UTF8_CHAR_LEN
		static unsigned char utf8_char_len[] = {
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
			4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
		};
#endif

		//单字节的字符状态值，对应语法常量
		static unsigned short xml_char_syntax[] = {
			0,0,0,0,0,0,0,0,0,8,8,0,0,8,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			8,64,4,0,0,0,0,4,0,0,0,0,0,8208,16,128,
			1552,1552,1552,1552,1552,1552,1552,1552,1552,1552,0,256,1,2048,2,0,
			0,1072,1072,1072,1072,1072,1072,48,48,48,48,48,48,48,48,48,
			48,48,48,48,48,48,48,48,48,48,48,4096,0,0,0,48,
			0,1072,1072,1072,1072,1072,1072,48,48,48,48,48,48,48,48,48,
			48,48,48,48,48,48,48,48,48,48,48,0,0,0,0,0,
		};


		namespace XML_SYNTAX {
			//语法常量定义，渣英语，名称定义凑合看吧。

			static constexpr auto _X_LT{ static_cast<unsigned short>(0x01) };			// <
			static constexpr auto _X_GT{ static_cast<unsigned short>(0x02) };			// >
			static constexpr auto _X_STRING{ static_cast<unsigned short>(0x04) };		// ' "
			static constexpr auto _X_SPACE{ static_cast<unsigned short>(0x08) };		// \r\n\r空格
			static constexpr auto _X_NAME{ static_cast<unsigned short>(0x10) };		// A-Z a-z 0-9 _ - .
			static constexpr auto _X_BEGINNAME{ static_cast<unsigned short>(0x20) };	// A-Z a-z _
			static constexpr auto _X_EXCLAM{ static_cast<unsigned short>(0x40) };		// !
			static constexpr auto _X_TAGEND{ static_cast<unsigned short>(0x80) };		// /
			static constexpr auto _X_ESCAPEEND{ static_cast<unsigned short>(0x100) };	// ;
			static constexpr auto _X_NUMBER{ static_cast<unsigned short>(0x200) };	// 数字0-9
			static constexpr auto _X_HEX{ static_cast<unsigned short>(0x400) };		// 16进制0-9 A-F a-f
			static constexpr auto _X_EQUAL{ static_cast<unsigned short>(0x800) };		// =
			static constexpr auto _X_LB{ static_cast<unsigned short>(0x1000) };		// [
			static constexpr auto _X_NEGATIVE{ static_cast<unsigned short>(0x2000) };	// -
			static constexpr auto _X_MULTIBYTE{ static_cast<unsigned short>(0x4000) };	// 多字节字符
		}

		//保险起见，为了未来考虑，定义一下xml文档的最大长度，时代发展太迅猛，万一我有生之年能用上128bit，到时候也许处理64bit长度的文档就跟我们现在解析小文档一样。
		using xml_size_t = unsigned int;
		static constexpr auto _xnf{ static_cast<xml_size_t>(-1) };

		//这个结构，用来储存转义符位置，以便于快速替换，备用，暂不实现，因为这关乎性能。
		struct xml_escape_pos { xml_size_t pos, len; };

		template<typename _XtsTy>
		class xparser_t;

		//xml文本迭代器的基本模板类
		template<typename _Ty>
		class xts_t {
		public:
			using Basetype = _Ty;

		protected:
			const _Ty *text;
			xml_size_t size;
			xml_size_t index;
			_Ty c;
			unsigned char cl;
			unsigned short s;
			unsigned short flags;
		};

		//解析错误信息结构
		//解析时不处理行，列问题，有错误发生时后处理，因为，行，列处理，会使解析速度慢差不多一倍。
		struct xerrorpos {
			xml_size_t pos;
			int number;
			std::string information;
			xml_size_t line;
			xml_size_t column;
		};

		//这两个结构用来储存一些字符串常量，实现两种字符串格式的快速引用，这两个结构绑定到三种xts类中
		struct xmultybyte_constvalue {
			static constexpr const char *br_tag = "<br/>";
			static constexpr const char *crlf = "\r\n";
			static constexpr const char *end_tag_syntax = "</";
			static constexpr const char *autoend_tag_syntax = "/>";
			static constexpr const char *comment_end = "--";
			static constexpr const char *cdata_end = "]]>";
		};

		struct xwidechar_constvalue {
			static constexpr const wchar_t *br_tag = L"<br/>";
			static constexpr const wchar_t *crlf = L"\r\n";
			static constexpr const wchar_t *end_tag_syntax = L"</";
			static constexpr const wchar_t *autoend_tag_syntax = L"/>";
			static constexpr const wchar_t *comment_end = L"--";
			static constexpr const wchar_t *cdata_end = L"]]>";
		};

		//utf8的文本迭代器，先基于这个来实现
		class xts_utf8 : public xts_t<char>
		{
		public:
			using strtype = std::string;
			static constexpr int _encoding{ 2 };
			using constval = xmultybyte_constvalue;

			//初始化
			void init(const char *_Text, xml_size_t _Size) {
				text = _Text;
				size = _Size;
				index = 0;
				c = text[0];
				cl = utf8_char_len[(unsigned char)c];
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//处理下一个字符
			void next() {
				index += cl;
				c = text[index];
				cl = utf8_char_len[(unsigned char)c];
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//向前回退n个字符，目前，只有在根节点之前的处理，有用到这个
			void back(xml_size_t len) {
				index -= len;
				c = text[index];
				cl = utf8_char_len[(unsigned char)c];
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//next，并判断语法
			bool next_is_flags() {
				next();
				return (flags & s) != 0;
			}

			//next, 并判断下一个字符的值
			bool next_is_char(char _Chr) {
				next();
				return _Chr == c;
			}

			//解析错误时，用于获取行，列。
			void next_donot_syntax() {
				index += cl;
				c = text[index];
				cl = utf8_char_len[(unsigned char)c];
			}

			//设置允许的语法
			void set_flags(unsigned short _Flags) {
				flags = _Flags;
			}

		private:
			friend class xparser_t<xts_utf8>;
		};

		//asc的文本迭代器
		class xts_asc : public xts_t<char>
		{
		public:
			using strtype = std::string;
			static constexpr int _encoding{ 0 };
			using constval = xmultybyte_constvalue;

			//初始化
			void init(const char *_Text, xml_size_t _Size) {
				text = _Text;
				size = _Size;
				index = 0;
				c = text[0];
				cl = ((unsigned short)c >= 0x80) ? 2 : 1;
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//处理下一个字符
			void next() {
				index += cl;
				c = text[index];
				cl = ((unsigned short)c >= 0x80) ? 2 : 1;
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//向前回退n个字符，目前，只有在根节点之前的处理，有用到这个
			void back(xml_size_t len) {
				index -= len;
				c = text[index];
				cl = ((unsigned short)c >= 0x80) ? 2 : 1;
				s = (cl != 1) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//next，并判断语法
			bool next_is_flags() {
				next();
				return (flags & s) != 0;
			}

			//next, 并判断下一个字符的值
			bool next_is_char(char _Chr) {
				next();
				return _Chr == c;
			}

			void next_donot_syntax() {
				index += cl;
				c = text[index];
				cl = ((unsigned short)c >= 0x80) ? 2 : 1;
			}

			//设置允许的语法
			void set_flags(unsigned short _Flags) {
				flags = _Flags;
			}

		private:
			friend class xparser_t<xts_asc>;
		};


		class xts_utf16 : public xts_t<wchar_t> {
		public:
			using strtype = std::wstring;
			static constexpr int _encoding{ 1 };
			using constval = xwidechar_constvalue;
			xts_utf16() {
				//utf16的字符不是变长的，固定为1
				//虽然有4字节的utf16字符，但影响不到最终解析逻辑。
				cl = 1;
			}

		private:
			void init(const wchar_t *_Text, xml_size_t _Size) {
				text = _Text;
				size = _Size;
				index = 0;
				c = text[0];
				cl = 1;
				s = ((unsigned short)c >= 0x80) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//处理下一个字符
			void next() {
				c = text[++index];
				s = ((unsigned short)c >= 0x80) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			void back(xml_size_t len) {
				index -= len;
				c = text[index];
				s = ((unsigned short)c >= 0x80) ?
					XML_SYNTAX::_X_MULTIBYTE | XML_SYNTAX::_X_BEGINNAME | XML_SYNTAX::_X_NAME :
					xml_char_syntax[(unsigned char)c];
			}

			//next，并判断语法
			bool next_is_flags() {
				next();
				return (flags & s) != 0;
			}

			//next, 并判断下一个字符的值
			bool next_is_char(char _Chr) {
				next();
				return _Chr == c;
			}

			void next_donot_syntax() {
				c = text[++index];
			}

			//设置允许的语法
			void set_flags(unsigned short _Flags) {
				flags = _Flags;
			}
		private:
			friend class xparser_t<xts_utf16>;
		};


		template<typename _XtsTy>
		class xdocument_t;

		template<typename _Ty>
		class xelement_t;

		template<typename _Ty>
		class xresource_t {
		public:

			//xml节点数据结构，因为结构相互依赖的原因，所以嵌套在一起
			using Basetype = typename _Ty::_Mybase::_Alty::value_type;
			class xnode;
			using xtagindex_t = std::list<xnode*>;
			using xtagindex_ref = typename xtagindex_t::iterator;
			using xdoctext_t = std::list<_Ty>;

			using xattrname_t = std::set<_Ty>;
			using xattrvalue_t = std::map<_Ty, xml_size_t>;
			using xtagtext_t = std::map<_Ty, xtagindex_t>;
			using xdoctext_ref = typename xdoctext_t::iterator;
			using xtagtext_ref = typename xtagtext_t::iterator;
			using xattrname_ref = typename xattrname_t::iterator;
			using xattrvalue_ref = typename xattrvalue_t::iterator;

			class xnode {
			public:
				using _Self_Reftype = typename std::list<xnode>::iterator;
				xnode() {
					parent = nullptr;
				}

				xnode(xnode *_Parent, xresource_t *_Resource) {
					parent = _Parent;
					ti.doc_body_ref = inner.end = inner.begin = _Resource->docs.end();
				}

			private:
				void refactor_tag_body(int _Style, xml_size_t _PreSize, xresource_t *_Resource)
				{
					_Ty &_Tmp = _Resource->refactor_buffer;
					_Tmp.clear();
					_Tmp.reserve(_PreSize);
					_Tmp += (Basetype)'<';
					_Tmp += ti.name->first;

					for (auto it = attrs.begin(); it != attrs.end(); ++it) {
						_Tmp += (Basetype)' ';
						_Tmp += *it->name;
						_Tmp += (Basetype)'=';
						_Tmp += (Basetype)it->st;
						_Tmp += it->value->first;
						_Tmp += (Basetype)it->st;
					}
					if (_Style == 2)
						_Tmp += (Basetype)'/';
					_Tmp += (Basetype)'>';
					if (ti.doc_body_ref == _Resource->docs.end()) {
						_Resource->docs.push_back(_Tmp);
						ti.doc_body_ref = --(_Resource->docs.end());
						parent->inner.end = ti.doc_body_ref;
						if (parent->inner.begin == _Resource->docs.end())
							parent->inner.begin = ti.doc_body_ref;
					}
					else
					{

						*ti.doc_body_ref = _Tmp;
					}
				}

			private:
				friend class xresource_t;
				friend class xparser_t<xts_utf8>;
				friend class xparser_t<xts_utf16>;
				friend class xparser_t<xts_asc>;
				friend class xdocument_t<xts_utf8>;
				friend class xdocument_t<xts_utf16>;
				friend class xdocument_t<xts_asc>;
				friend class xelement_t<xts_utf8>;
				friend class xelement_t<xts_utf16>;
				friend class xelement_t<xts_asc>;
				struct xattr { xattrname_ref name; xattrvalue_ref value; char st; };

				struct tag_info {
					xtagtext_ref name;//标签名称
					xtagindex_ref name_index_ref;//在标签名称索引中的引用，本质上其实就是个指针
					xdoctext_ref doc_body_ref;//整个标签信息，包含属性 <t ...> 在文档中的实体
				}ti;

				std::list<xattr> attrs;
				struct xinner { xdoctext_ref begin, end; }inner;
				std::list<xnode> child;
				xnode *parent;
				_Self_Reftype self;
			};

			xresource_t() {
				//预定义的几个转义符实体：lt gt amp quot apos
				escape_bodys[{ (Basetype)'l', (Basetype)'t'}] = { (Basetype)'<' };
				escape_bodys[{ (Basetype)'g', (Basetype)'t' }] = { (Basetype)'>' };
				escape_bodys[{ (Basetype)'a', (Basetype)'m', (Basetype)'p' }] = { (Basetype)'&' };
				escape_bodys[{ (Basetype)'q', (Basetype)'u', (Basetype)'o', (Basetype)'t' }] = { (Basetype)'"' };
				escape_bodys[{ (Basetype)'a', (Basetype)'p', (Basetype)'o', (Basetype)'s' }] = { (Basetype)'\'' };
			}

			void clear() {
				root.child.clear();
				docs.clear();
				tags.clear();
				attr_names.clear();
				attr_values.clear();
				root.parent = nullptr;
				root.ti.doc_body_ref = root.inner.end = root.inner.begin = docs.end();
			}

			xnode root;
			xdoctext_t docs;
			xtagtext_t tags;
			xattrname_t attr_names;
			xattrvalue_t attr_values;
			std::map<_Ty, _Ty> escape_bodys;
			_Ty refactor_buffer;
		};

		template<typename _XtsTy>
		class xparser_t {
		public:
			using _StringTy = typename _XtsTy::strtype;
			using Basetype = typename _XtsTy::Basetype;
			xparser_t() {
				// 这里忽悠一下编译器，自动根据类型选择：strstr 或 wcsstr 
				typedef const char *(__cdecl*STRSTRFUNC)(const char *, const char *);
				typedef const wchar_t *(__cdecl*WSTRSTRFUNC)(const wchar_t *, const wchar_t *);
				typedef const Basetype *(__cdecl*MYSTRSTRFUNC)(const void *, const void *);
				__multiec_strstr = ((sizeof(Basetype) == 1) ? 
					((MYSTRSTRFUNC)((STRSTRFUNC)strstr)) : 
					((MYSTRSTRFUNC)((WSTRSTRFUNC)wcsstr)));
			}

		private:

			void x_escape_number()
			{
				xml_size_t ebgn = xts.index - 1;
				long long x = 0;
				if (!xts.next_is_char('x')) {
					xts.index -= xts.cl;
					xts.set_flags(XML_SYNTAX::_X_NUMBER);
					if (!xts.next_is_flags()) err(xts.index, 22);
					xts.set_flags(XML_SYNTAX::_X_NUMBER | XML_SYNTAX::_X_ESCAPEEND);
					for (;;) {
						x = (x * 10) + (xts.c - '0');
						if (!xts.next_is_flags()) err(xts.index, 22);
						if (xts.c == ';')
							break;
					}
				}
				else
				{
					xts.set_flags(XML_SYNTAX::_X_HEX);
					if (!xts.next_is_flags()) err(xts.index, 23);
					xts.set_flags(XML_SYNTAX::_X_HEX | XML_SYNTAX::_X_ESCAPEEND);
					for (int i = 0;; i++) {
						long long _Tmp;
						switch (xts.c) {
						case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
							_Tmp = xts.c - '0';
							break;
						case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
							_Tmp = xts.c - 'a' + 10;
							break;
						case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
							_Tmp = xts.c - 'A' + 10;
							break;
						}

						x += (_Tmp << (i << 2));
						if (!xts.next_is_flags()) err(xts.index, 23);
						if (xts.c == ';')
							break;
					}
				}
				if (x < 0x20) {
					switch (x) {
					case '\t':case '\r':case '\n':
						break;
					default:
						err(ebgn, 24);
					}
				}
				else if (x > 0xD800 && x < 0xDFFF)
					err(ebgn, 25);
				else if (x > 0x10FFFF)
					err(ebgn, 26);

			}

			void x_escape_body()
			{
				xml_size_t nbgn = xts.index;
				for (;;) {
					xts.next();
					if (xts.c == ';') {
						break;
					}
					else {
						if (!(xts.s & XML_SYNTAX::_X_BEGINNAME)) err(xts.index, 19);
					}
				}

				_StringTy &_Tmp = _strtmp[4];
				_Tmp.assign(xts.text + nbgn, xts.index - nbgn);
				auto it = res->escape_bodys.find(_Tmp);
				if (it == res->escape_bodys.end()) {
					errinfobuffer.reserve(_Tmp.length() * 3);
					int n = sprintf_s((char*)errinfobuffer.data(), errinfobuffer.capacity(),
						((sizeof(Basetype) == 2) ? "%ls" : "%s"),
						_Tmp.c_str());
					err(nbgn, 20, errinfobuffer.c_str());
				}
			}

			void x_escape() {
				xts.next();
				if (xts.c == '#') {
					x_escape_number();
				}
				else
				{
					if (!(xts.s & XML_SYNTAX::_X_BEGINNAME)) err(xts.index, 18);
					x_escape_body();
				}
			}

			void x_cdata() {
				xml_size_t cbgn = xts.index - 2;
				const char *pcdata = "CDATA[";
				for (int i = 0; i < 6; i++) {
					if (!xts.next_is_char(pcdata[i]))
						err(xts.index, 16);
				}

				// CDATA的结束符比注释标签还要省事，直接向后搜索]]>
				const Basetype *p = __multiec_strstr(xts.text + xts.index + 1, _XtsTy::constval::cdata_end);
				if (p) {
					xts.index = (xml_size_t)(p - xts.text + 3);
					res->docs.push_back(_StringTy(xts.text + cbgn, xts.index - cbgn));
					cur->inner.end = --(res->docs.end());
					if (cur->inner.begin == res->docs.end()) cur->inner.begin = cur->inner.end;
				}
				else
				{
					err(cbgn, 17);
				}
			}

			void x_comment() {
				xml_size_t cbgn = xts.index - 2;
				if (!xts.next_is_char('-')) err(xts.index, 13);
				/*
					不太清楚为什么xml注释中不允许存在--，我反正照做了。
					从代码此处看，实际上是可以允许的，就像是CDATA的结束符那样。
					utf8的情况下，无法双字搜索。
					utf16的情况下，也无法4字节搜索。
					例如这种情况：
					<--a-->，如果双字搜索，从a开始，有一个-就被忽略掉了，如果要判断这个问题，那实际上和单字节搜索一样的性能。
				*/

				const Basetype *p = __multiec_strstr(xts.text + xts.index + 1, _XtsTy::constval::comment_end);
				if (p) {
					if (p[2] == '>') {
						xts.index = (xml_size_t)(p - xts.text + 3);
						res->docs.push_back(_StringTy(xts.text + cbgn, xts.index - cbgn));
						cur->inner.end = --(res->docs.end());
						if (cur->inner.begin == res->docs.end()) cur->inner.begin = cur->inner.end;
					}
					else
					{
						err((xml_size_t)(p - xts.text), 15);
					}
				}
				else
				{
					err(cbgn, 14);
				}
			}

			void x_specifics_tag() {
				//特殊标签，共有两个分支，注释和CDATA，DTD在根节点之前处理，不会进入这里
				xts.set_flags(XML_SYNTAX::_X_LB | XML_SYNTAX::_X_NEGATIVE);
				if (!xts.next_is_flags()) err(xts.index, 2);
				if (xts.c == '-')
					x_comment();
				else
					x_cdata();
			}

			void x_end_node() {

				//结束标签处理

				xts.set_flags(XML_SYNTAX::_X_BEGINNAME);
				if (!xts.next_is_flags()) err(xts.index, 10);

				xml_size_t nbgn = xts.index;
				xml_size_t nend;
				xts.set_flags(XML_SYNTAX::_X_NAME | XML_SYNTAX::_X_SPACE | XML_SYNTAX::_X_GT);
				bool _BackSpace = false;

				for (;;) {
					if (!xts.next_is_flags()) err(xts.index, 10);
					if (!(xts.s & XML_SYNTAX::_X_NAME)) {
						nend = xts.index;
						if (xts.s & XML_SYNTAX::_X_SPACE)
							_BackSpace = true;
						break;
					}
				}

				if (_BackSpace) {
					//后面还有空格
					xts.set_flags(XML_SYNTAX::_X_SPACE | XML_SYNTAX::_X_GT);
					for (;;) {
						if (!xts.next_is_flags()) err(xts.index, 11);
						if (xts.c == '>')
							break;
					}
				}

				_StringTy &tmp = _strtmp[0];

				tmp.reserve(nend - nbgn + 0x10);
				tmp.assign(xts.text + nbgn, nend - nbgn);

				if (tmp != cur->ti.name->first) {
					errinfobuffer.reserve((tmp.length() + cur->ti.name->first.length()) * 3 + 0x20);
					int n = sprintf_s((char*)errinfobuffer.data(), errinfobuffer.capacity(),
						((sizeof(Basetype) == 2) ? "%ls 与 %ls 不一致" : "%s 与 %s 不一致"),
						tmp.c_str(), cur->ti.name->first.c_str());
					err(nbgn, 12, errinfobuffer.c_str());
				}

				if (cur->inner.begin == res->docs.end()) {
					//如果这个节点的内容为空，说明，它跟一个自结束的节点没有区别
					//直接在父节点中将它修改一个自结束节点即可。
					cur->parent->inner.end->erase(cur->parent->inner.end->length() - 1);
					cur->parent->inner.end->append(_XtsTy::constval::autoend_tag_syntax);
				}
				else
				{
					res->docs.push_back(_XtsTy::constval::end_tag_syntax);
					auto it = --(res->docs.end());
					it->append(tmp);
					(*it) += (Basetype)'>';
					cur->inner.end = it;
				}

				cur = cur->parent;
			}

			int x_tag_name() {

				auto new_name = [this](xml_size_t left, xml_size_t right) {
					_StringTy &tmp = _strtmp[0];
					tmp.assign(xts.text + left, right - left);

					auto it = res->tags.find(tmp);
					if (it == res->tags.end())
						it = res->tags.insert({ tmp, std::list<_Nodetype*>() }).first;
					it->second.push_back(cur);
					cur->ti.name = it;
					cur->ti.name_index_ref = (--(it->second.end()));
				};

				xts.set_flags(
					XML_SYNTAX::_X_NAME |		//符合名称规范的字符
					XML_SYNTAX::_X_GT |		// >
					XML_SYNTAX::_X_TAGEND |	// /自结束标签
					XML_SYNTAX::_X_SPACE		// 空白字符
				);

				xml_size_t name_begin = xts.index;
				for (;;) {

					if (!xts.next_is_flags()) err(xts.index, 1);

					switch (xts.c) {
					case '>':
						new_name(name_begin, xts.index);
						return 1;
					case '/':
						if (!xts.next_is_char('>')) err(xts.index, 4);
						new_name(name_begin, xts.index - 1);
						return 2;
					default:
						if (xts.s & XML_SYNTAX::_X_SPACE) {
							new_name(name_begin, xts.index);
							return 0;
						}
						break;
					}
				}
			}

			bool x_attr_name(_StringTy &_Name) {
				xts.set_flags(
					XML_SYNTAX::_X_NAME |		//符合名称规范的字符
					XML_SYNTAX::_X_EQUAL |	//等于号
					XML_SYNTAX::_X_SPACE		// 空白字符
				);
				xml_size_t name_begin = xts.index;
				for (;;) {
					if (!xts.next_is_flags()) err(xts.index, 5);
					if (xts.s & (XML_SYNTAX::_X_EQUAL | XML_SYNTAX::_X_SPACE)) {
						_Name.assign(xts.text + name_begin, xts.index - name_begin);
						return xts.c == '=';
					}
				}
				return false;
			}

			char x_attr_value(_StringTy &_Value) {
				xts.set_flags(
					XML_SYNTAX::_X_STRING | //字符串 " '
					XML_SYNTAX::_X_SPACE // 空白字符
				);

				char _Style;

				for (;;) {
					if (!xts.next_is_flags()) err(xts.index, 7);
					if (xts.s & XML_SYNTAX::_X_STRING) {
						_Style = (char)xts.c;
						break;
					}
				}

				xml_size_t value_begin = xts.index + 1;

				if (_Style == '"') {
					for (;;) {
						xts.next();
						switch (xts.c) {
						case 0:
							err(xts.index, 8);
						case '<':
							err(xts.index, 9);
						case '&':
							//处理转义符
							x_escape();
							break;

						case '"':
							//字符串结束
							_Value.assign(xts.text + value_begin, xts.index - value_begin);
							return _Style;
						default:
							break;
						}
					}
				}
				else
				{
					for (;;) {
						xts.next();
						switch (xts.c) {
						case 0:
							err(xts.index, 8);
						case '<':
							err(xts.index, 9);
						case '&':
							//处理转义符
							x_escape();
							break;

						case '\'':
							//字符串结束
							_Value.assign(xts.text + value_begin, xts.index - value_begin);
							return _Style;
						default:
							break;
						}
					}

				}

				return _Style;
			}

			void x_attr(xml_size_t &_Presize) {
				_StringTy &name = _strtmp[0];
				_StringTy &value = _strtmp[1];

				if (!x_attr_name(name)) {
					//x_attr_name中没有找到等于号，对应这种: <a x  =...
					xts.set_flags(
						XML_SYNTAX::_X_EQUAL | //等号
						XML_SYNTAX::_X_SPACE // 空白字符
					);

					for (;;) {
						if (!xts.next_is_flags()) err(xts.index, 7);
						if (xts.c == '=') break;
					}
				}


				char _Style = x_attr_value(value);
				_Presize += (xml_size_t)(name.length() + value.length() + 6);

				auto itn = res->attr_names.find(name);
				if (itn == res->attr_names.end())
					itn = res->attr_names.insert(name).first;

				auto itv = res->attr_values.find(value);
				if (itv == res->attr_values.end())
					itv = res->attr_values.insert({ value, 1 }).first;

				cur->attrs.push_back({ itn, itv, _Style });
			}

			int x_preattr(xml_size_t &_Presize) {

				/*
					x_tag_name里没有找到 > 的情况下，在标签属性解析开始之前，
					对应下面这几种情况：
					<a >
					<a />
					<a x=...
				*/

				xts.set_flags(
					XML_SYNTAX::_X_SPACE |		//空白字符
					XML_SYNTAX::_X_GT |			// >
					XML_SYNTAX::_X_BEGINNAME |		//名称首字符 
					XML_SYNTAX::_X_TAGEND			// /自结束标签
				);

				for (;;) {
					if (!xts.next_is_flags()) err(xts.index, 5);


					switch (xts.c) {
					case '>':
						return 1;
					case '/':
						if (!xts.next_is_char('>')) err(xts.index, 4);
						return 2;
					default:

						if (xts.s & XML_SYNTAX::_X_BEGINNAME) {
							x_attr(_Presize);
							xts.set_flags(
								XML_SYNTAX::_X_SPACE |		// 空白字符
								XML_SYNTAX::_X_GT |			// >
								XML_SYNTAX::_X_BEGINNAME |		// 名称首字符 
								XML_SYNTAX::_X_TAGEND			// /自结束标签
							);
						}
						break;
					}
				}


			}

			int node_size = 0;

			void x_new_node() {

				node_size++;
				cur->child.push_back(_Nodetype(cur, res));
				auto it = (--cur->child.end());
				cur = &(*it);
				cur->self = it;

				int n = x_tag_name();

				xml_size_t _PreSize = (xml_size_t)(cur->ti.name->first.length() + 3);
				if (!n) n = x_preattr(_PreSize);

				cur->refactor_tag_body(n, _PreSize, res);

				if (n == 2)
					cur = cur->parent;
			}

			void x_tag() {
				//标签开始后，下一个字符只能是 符合名称规范的第一个字符，感叹号 !，结束标签 /
				xts.next();
				switch (xts.c) {
				case '!':
					x_specifics_tag();
					break;
				case '/':
					x_end_node();
					break;
				default:
					if (!(xts.s & XML_SYNTAX::_X_BEGINNAME)) err(xts.index, 1);
					x_new_node();
					break;
				}
			}

			void x_text() {

				//标签之外的有效文本处理

				xml_size_t tbegin = _xnf;
				_StringTy &tmp = _strtmp[3];
				tmp.clear();

				for (;;) {
					xts.next();
					switch (xts.c) {
					case 0:
						return;
					case '&':
						if (tbegin == _xnf)
							tbegin = xts.index;
						x_escape();
						break;
					case '<':
						//处理标签之前，先处理有效文本
						if (tbegin != _xnf) {
							if (tmp.length()) tmp += ' ';
							tmp.append(xts.text + tbegin, xts.index - tbegin);
							tbegin = _xnf;
						}

						if (tmp.length()) {
							res->docs.push_back(tmp);
							cur->inner.end = --(res->docs.end());
							if (cur->inner.begin == res->docs.end()) cur->inner.begin = cur->inner.end;
							tmp.clear();
						}

						x_tag();
						break;
					default:
						if (!(xts.s & XML_SYNTAX::_X_SPACE)) {
							if (tbegin == _xnf)
								tbegin = xts.index;
						}
						else
						{
							//遇到空白字符时，如果有效文本开始位置已经记录过了，则将这一段有效的东西添加到有效文本结
							if (tbegin != _xnf) {
								if (tmp.length()) tmp += ' ';
								tmp.append(xts.text + tbegin, xts.index - tbegin);
								tbegin = _xnf;
							}
						}
					}
				}
			}

			void x_dtd() {
				xml_size_t pos = xts.index - 1;
				const char *p = "OCTYPE";
				for (int i = 0; i < 6; i++) {
					if (!xts.next_is_char(p[i])) err(pos, 2);
				}

				int n = 1;
				int _StrType = 0;
				for (;;) {
					xts.next();
					switch (xts.c) {
					case '"':
					case '\'':
						if (!_StrType)
							_StrType = xts.c;
						else if (_StrType == xts.c)
							_StrType = 0;
						break;
					case '<':
						n++;
						break;
					case '>':
						if (!_StrType) {
							if (!(--n)) {

								res->docs.push_back(_StringTy(xts.text + pos, xts.index - pos + 1));
								cur->inner.end = --(res->docs.end());
								if (cur->inner.begin == res->docs.end()) cur->inner.begin = cur->inner.end;
								//wprintf(L"%s\n", cur->inner.end->c_str());
								return;
							}
						}
						break;
					default:
						break;
					}
				}
			}

			void x_declare() {
				xml_size_t pos = xts.index - 1;
				int _StrType = 0;
				for (;;) {
					xts.next();
					switch (xts.c) {
					case '"':
					case '\'':
						if (!_StrType)
							_StrType = xts.c;
						else if (_StrType == xts.c)
							_StrType = 0;
						break;
					case '?':
						if (!_StrType) {
							if (!xts.next_is_char('>')) err(xts.index, 30);
							res->docs.push_back(_StringTy(xts.text + pos, xts.index - pos + 1));
							cur->inner.end = --(res->docs.end());
							if (cur->inner.begin == res->docs.end()) cur->inner.begin = cur->inner.end;
							return;
						}
						break;
					default:
						break;
					}
				}
			}

			int x_root() {
				if (setjmp(_Rem)) return -1;
				xts.set_flags(XML_SYNTAX::_X_LT | XML_SYNTAX::_X_SPACE);
				bool root_break = false;
				for (;;) {
					if (xts.c == '<') {
						xts.next();
						switch (xts.c) {
						case '!':
							xts.next();
							if (xts.c == '-')
								x_comment();
							else if (xts.c == 'D')
								x_dtd();
							else
								err(xts.index, 2);
							xts.set_flags(XML_SYNTAX::_X_LT | XML_SYNTAX::_X_SPACE);
							break;
						case '?':
							if (xts.index != 1)
								err(xts.index, 31);
							x_declare();
							xts.set_flags(XML_SYNTAX::_X_LT | XML_SYNTAX::_X_SPACE);
							break;
						default:
							if (xts.s & XML_SYNTAX::_X_BEGINNAME) {
								xts.back(1);
								x_tag();
								x_text();
								root_break = true;
							}
							else
							{
								err(xts.index, 2);
							}
							break;
						}

						if (root_break)
							break;
					}

					if (!xts.next_is_flags())
						err(xts.index, 3);

				}

				if (cur != &(res->root)) {
					//解析完字符串之后，如果当前标签不为null，则属于错误。
					err(xts.size, 28);
				}

				return 0;
			}

			void err(xml_size_t _Pos, int _Number, const char *_Info = "") {
				errp = { _Pos, _Number, _Info };
				longjmp(_Rem, 1);
			}

		public:
			int load(const Basetype *_Text, int _Size, xresource_t<_StringTy> *pres) {
				xts.init((const Basetype*)_Text, _Size);
				errp.number = 0;
				errp.pos = 0;
				res = pres;
				cur = &(res->root);
				return x_root();
			}

			void get_errp(xerrorpos &e) {
				e = errp;
			}

			void get_err_pos(xerrorpos &e) {
				e.line = 0;
				e.column = 0;
				if (!e.pos || !e.number)
					return;
				auto pos = xts.index - xts.cl;
				xts.index = 0;
				xts.c = xts.text[0];
				e.line = 1;
				e.column = 1;
				for (; xts.index < e.pos; xts.next_donot_syntax())
				{
					if (xts.c == '\n') {
						e.line++;
						e.column = 1;
					}
					else
					{
						e.column++;
					}
				}
				xts.index = pos;
				xts.next();
			}

		private:
			friend class xelement_t<_XtsTy>;
			jmp_buf _Rem;
			_XtsTy xts;
			xerrorpos errp;
			using _Nodetype = typename xresource_t<_StringTy>::xnode;
			_Nodetype *cur;
			xresource_t<_StringTy> *res;
			_StringTy _strtmp[8];//由于使用了jmp_buf来进行错误直接远跳，为了避免内存泄漏，所以将栈中需要的字符串对象也储存在这里
			const Basetype*(*__multiec_strstr)(const void*, const void*);
			std::string errinfobuffer;
		};


		static const char *xml_error_information[] = {
			"",
			"开始标签:无效的元素名称",		//1
			"根节点之前的无效的特殊标签",		//2
			"根节点之前的无效的字符",			//3
			"自结束标签：此处应为 >",			//4
			"标签属性:无效的标签属性名称",		//5
			"标签属性:此处应为 =",			//6
			"标签属性:此处应为 \" 或 '",		//7
			"标签属性:未找到对应的属性结束符(\" 或 ')",	//8
			"标签属性:< 不允许出现在属性值中",	//9
			"结束标签:无效的元素名称",		//10
			"结束标签:此处应为 >",			//11
			"结束标签:开始标签与结束标签不匹配，参考信息:%s",	//12
			"注释标签:无效的注释标签，此处或许应为 -",//13
			"注释标签:未找到注释标签结束符(-->)",//14
			"注释标签:-- 不允许单独出现在注释标签中",//15
			"CDATA:无效的CDATA标签",//16
			"CDATA:未找到CDATA结束符(]]>)",//17
			"转义符:无效的转义符名称首字符",//18
			"转义符:无效的转义符字符",//19
			"转义符:%s 是未定义的实体",//20
			"转义符:无效的转义符字符",//21
			"字符数值转义:无效的10进制数字字符",//22
			"字符数值转义:无效的16进制数字字符",//23
			"字符数值转义:小于32(0x20)的字符仅允许\\t\\r\\n出现在xml中",//24
			"字符数值转义:0xD800-0xDFFF为UNICODE代理字符，不允许单独出现在xml中",//25
			"字符数值转义:字符值溢出，参考最大值（0x10FFFF）",//26
			"转义符:无效的转义符",//27
			"根节点未封闭",//28
			"无效的文档：%s",//29
			"XML声明种错误的符号，此处应为 >",//30
			"XML声明前不允许存在其他字符",//31
			"未找到XML声明结束符(?>)",//32
		};

		static xml_size_t XGI_FORMAT = 1;
		static xml_size_t XGI_COMMENT = 2;
		static xml_size_t XGI_CDATAONLY = 4;
		static xml_size_t XGI_TEXTONLY = 8;

		template<typename _XtsTy>
		class xelement_t {
		public:

			using _StringTy = typename _XtsTy::strtype;
			using _Nodetype = typename xresource_t<_StringTy>::xnode;
			using Basetype = typename xresource_t<_StringTy>::Basetype;
			bool eof() {
				return _Node == nullptr;
			}

			xelement_t(_Nodetype *_Val) {

				_Node = _Val;
			}

			_StringTy get_name() {
				return _Node->ti.name->first;
			}

			_StringTy get_attr(const _StringTy &_AttrName) {
				for (auto it = _Node->attrs.begin(); it != _Node->attrs.end(); ++it) {
					if (*(it->name) == _AttrName)
						return it->value->first;
				}
				return "";
			}

			_StringTy get_inner_xml() {
				_StringTy _Tmp;
				for (auto it = _Node->inner.begin; it != _Node->inner.end; ++it) {
					if (it->length() > 3 && 
						it->at(0) == '<' &&
						it->at(1) == '!' &&
						it->at(2) == '-')
						continue;

					if (it->length() > 4 && *it == _XtsTy::constval::br_tag)
					{
						_Tmp += _XtsTy::constval::crlf;
						continue;
					}

					if (it->length() > 1 && it->at(0) == '<' && it->at(1) != '/')
						_Tmp += _XtsTy::constval::crlf;
					_Tmp += it->c_str();
				}
				return _Tmp;
			}

		private:
			_Nodetype *_Node;
		};


		template<typename _XtsTy>
		class xdocument_t {
		public:
			xdocument_t() {

			}

			~xdocument_t() {
				res.clear();
			}

			using _StringTy = typename _XtsTy::strtype;
			using _ParserTy = typename xparser_t<_XtsTy>;
			using Basetype = typename _XtsTy::Basetype;

			using element = xelement_t<_XtsTy>;

			int load_file(const _StringTy &_Filename) {
				errp.pos = 0;
				errp.line = 0;
				errp.column = 0;
				res.clear();
				std::ifstream fs(_Filename.c_str(), std::ios::binary);
				fs.seekg(0, std::ios::end);
				size_t s = (size_t)fs.tellg();
				fs.seekg(0, std::ios::beg);

				if (!s) {
					errp.information.reserve(_Filename.length() * 3);
					sprintf_s((char*)errp.information.data(), errp.information.capacity(),
						(sizeof(Basetype) == 2) ? "%ls" : "%s", _Filename.c_str());

					errp.number = 29;
					errp.pos = 0;
					return -1;
				}

				char *p = new char[s + 2];
				p[s] = 0; p[s + 1] = 0;
				fs.read(p, s);
				fs.close();
				size_t _Off = 0;

				//预测文档编码，并不一定准确，只能说想到的判断都做了。
				/*返回值有4种：
					0 多字节编码非utf-8
					1 utf-16
					2 utf-8
					-1 错误
				*/

				
				_SrcEncode = encode_adaptive::xmlec_predict(p, s, &(errp.number), &_Off);
				if (_SrcEncode < 0) {
					delete p;
					errp.information.reserve(_Filename.length() * 3);
					sprintf_s((char*)errp.information.data(), errp.information.capacity(),
						(sizeof(Basetype) == 2) ? "%ls" : "%s", _Filename.c_str());
					return -1;
				}

				
				//很遗憾的事情是，c++17删除了编码转换库，所以，只能使用操作系统的函数来完成了。
				//虽然这个类库并不依赖c++17，但为了以后和新标准对接，所以只能自己实现跨平台的转换策略。
				_StringTy _Text;
				if (encode_adaptive::specifiy(p + _Off, _SrcEncode, _XtsTy::_encoding, _Text) == _nf) {
					delete p;
					errp.information.reserve(_Filename.length() * 3);
					sprintf_s((char*)errp.information.data(), errp.information.capacity(),
						(sizeof(Basetype) == 2) ? "%ls" : "%s", _Filename.c_str());
					errp.number = 29;
					return -1;
				}

				delete p;

				

				_ParserTy xp;
				int _Result = xp.load(_Text.c_str(), (xml_size_t)s, &res);

				//res.root.inner.begin = ++res.docs.begin();
				res.root.inner.end = res.docs.end();
				xp.get_errp(errp);
				if (errp.number) xp.get_err_pos(errp);


				return _Result;
			}

			element get_element(const _StringTy &_TagName) {
				auto it = res.tags.find(_TagName);
				if (it != res.tags.end())
					return *(it->second.begin());
				return nullptr;
			}

			std::string get_error_info() {
				char buf[256];
				std::string _Result;
				if (errp.pos != 0) {
					sprintf_s(buf, "XML错误位于 行(%d), 列(%d):", errp.line, errp.column);
					_Result += buf;
				}
				sprintf_s(buf, xml_error_information[errp.number], errp.information.c_str());
				_Result += buf;
				return _Result;
			}

			element root() {
				return &(res.root);
			}

		private:
			xresource_t<_StringTy> res;
			xerrorpos errp;
			int _SrcEncode;
		};
	}

	template<typename _Ty>
	using xdoc = aqx_internal::xdocument_t<_Ty>;
	using xts_utf8 = aqx_internal::xts_utf8;
	using xts_utf16 = aqx_internal::xts_utf16;
	using xts_asc = aqx_internal::xts_asc;


}
