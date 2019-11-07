#include "pch.h"
#include <iostream>
#include <time.h>
#include "xml.hpp"


int main()
{
	
	setlocale(LC_ALL, "");

	// 支持三种编码格式：aqx::xts_utf16 aqx::xts_utf8 aqx::xts_asc
	aqx::xdoc<aqx::xts_utf16> doc;
	auto t = clock();
	int err = doc.load_file(L"G:\\vs2017\\POE64_V1\\生成\\pfile.xml");
	printf("解析文档耗时：%I64d ms\n", clock() - t);
	if (err) {
		printf("%s\n", doc.get_error_info().c_str());
		return 0;
	}
	auto e = doc.get_element(L"CATALOG2");
	printf("%ls\n", e.get_inner_xml().c_str());

	return 0;
}
