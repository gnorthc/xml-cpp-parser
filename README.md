# xml-cpp-parser
xml c++ parser

这个项目目前未完善，仅仅做完解析部分的功能，
没有全面支持DTD和命名空间，这两项功能体量太大，以后看情况慢慢做。
对跨平台的依赖，应该只有编码转换这部分没有处理，因为c++17删除了codecvt头文件，没有替代品，我后续也不打算再使用目前暂时没有时间去g++下编写，之后我会慢慢补充。

目前解析3M左右的文档，约90000个节点，我自己测试，需要70ms左右（从读取文件，编码转换，直到解析完成），内存占用约40mb。

速度是xmlhttp的几十倍，内存占用比xmlhttp少20倍。

chrome浏览器里面解析大文档目前有bug，我使用3M的文档给它解析，它一直卡着不动 ie edge浏览器，都能正常解析。
