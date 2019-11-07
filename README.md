# xml-cpp-parser
xml c++ parser

这个项目目前未完善，仅仅做完解析部分的功能，
没有全面支持DTD和命名空间，这两项功能体量太大，以后看情况慢慢做。
目前可以测试过可以在linux (g++ --version = 9.1.1) 与windows中使用，
windows中支持3种编码格式 aqx::xdoc 是模板类，aqx::xts_utf8, aqx::xts_utf16, xts_asc 是模板参数
linux 中 aqx::xdoc 是直接定义好采用 aqx::xts_utf8 的类。

目前解析3M左右的文档，约90000个节点，我自己测试，需要70ms左右（从读取文件，编码转换，直到解析完成），内存占用约40Mb。
速度是xmlhttp的几十倍，内存占用比xmlhttp少20倍(xmlhttp 需要800mb)。

chrome浏览器里面解析大文档目前有bug，我使用3M的文档给它解析，它一直卡着不动 ie edge浏览器，都能正常解析。

