# 类Unix系统开发环境
----------------

> *作者：Arvin 日期：2017年3月14日*

>BEGIN

类Unix系统，最常见的就是Linux和MacOS了，MacOS有其自己的一套开发环境，比如Xcode、clang等，但是Linux上也有其开发环境。如果想比较高效并且系统统一的进行开发，必须有一套通用的开发环境，这里建议使用Linux的开发方式。类Unix环境，三大必须的环境是Shell、Editor、Compiler。Shell是人机接口，并且提供简单的Shell脚本编程语言，灵活使用可以大大的提高工作效率。Editor是编辑器，这里建议神器Vim编辑器，大家都懂的。Compiler当属GCC了，支持很多语言的编译，并且系统统一。总结下来就是：Shell、Vim和GCC了。这里还要说一句，GCC只负责编译工作，编译整个工程，还需要make工具，所以真正的开发项目还需要知道make语法。项目版本管理使用git工具。

Note：
* Shell:    bash
* Editor:   vim
* Compiler: gcc
* Builder:  make
* Version:  git

>END

