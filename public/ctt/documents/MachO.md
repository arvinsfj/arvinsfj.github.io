
# Mach-O可执行格式

> *作者：Arvin 日期：2018年1月13日*

---------------------------------

>BEGIN

### Mach-O格式
------------------------------

Mach-O是Mac和iOS上的可执行文件格式。可执行文件加载进内存后得到可以运行的进程。系统通过解析文件，建立依赖，初始化运行时环境，才能真正开始执行该进程。可执行文件是一种文件，并且是进程在磁盘的存储文件。


### Mach-O文件分析
-----------------------------------

这里介绍一个MH文件分析工具：MachOView。请自行下载和使用。

我们使用下面的C代码进行分析：

```
#include <stdio.h>

int main(int argc, char** argv){
    printf("Hello, World!\n");
    return 0;
}

```
使用命令：gcc main.c -o main 生成main可执行文件。通过MachOView打开浏览可知道Mach-O可执行文件包含下面几个部分：

1. 文件头（mach64 Header）
2. 加载命令（Load Commands）
3. 文本段（__TEXT）
4. 数据段(__DATA)
5. 动态加载器信息（Dynamic Loader Info）
6. 入口函数（Function Starts）
7. 符号表（Symbol Table）
8. 动态库符号表（Dynamic Symbol Table）
9. 字符串表（String Table）

文件头主要标识了该文件的支持的位数、CPU类型、文件类型、加载命令的条数和大小、文件标识等。系统最先解释文件头获得以上信息。

加载命令紧跟在文件头后面。非常详细清晰的指示可执行文件加载器如何设置和加载二进制数据到内存。一个段可以包含几个区域（节）。
特别需要注意段__TEXT的区域__text，因为里面标识了程序的二进制代码如何加载到内存的。其他__TEXT中的节包括：__stubs、__stub_helper、__cstring、__unwind_info等。__DATA段包含：__nl_symbol_ptr、__la_symbol_ptr等。其他加载内存的段包括：__PAGEZERO、__LINKEDIT等。以上都是使用加载内存命令：LC_SEGMENT_64。

其他的加载命令还包括：LC_DYLD_INFO_ONLY、LC_DYSYMTAB、LC_LOAD_DYLINKER、LC_SYMTAB、LC_VERSION_MIN_MACOSX、LC_SOURCE_VERSION、LC_MAIN、LC_LOAD_DYLIB、LC_FUNCTION_STARTS、LC_DATA_IN_CODE、LC_UUID等。

还有区域命令：__text、__stubs、__stub_helper、__cstring、__unwind_info。

这些段命令和区命令（加载命令）都有其具体含义和用途，自行弄清楚。

其中涉及到动态库加载命令有很多。涉及动态加载器地址信息、动态库地址信息、动态库绑定、地址重定向等主要信息。

动态加载动态库，关键在于理解桩的概念。获取动态库函数的过程十分复杂，自己调试可理解其详细过程。


### Mach-O格式和加载启动过程使用举例
-----------------------------------

1. category冲突分析
2. 非OC函数switch
3. bitcode分析
4. 包支持架构分析
5. 常量字符串分析
6. crash符号化
7. 符号模块查找
8. 学习经典的数据格式，有助于设计数据协议、网络协议等

二进制包越大，文件解析越慢，进程启动越慢。

由于动态库的函数调用都要通过数据段进行中转，而数据段用户态是可以进行修改的。可以利用该特性进行动态库函数调用的修改（代码注入或者函数hook）。

>END

