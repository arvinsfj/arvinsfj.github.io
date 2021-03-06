# shell脚本简单学习和总结

> *作者：Arvin 日期：2018年12月09日*

------------------------

>BEGIN

先扯一些其他的东西，今天提笔的时候想到之前写技术博客的时候方法好像有点问题。效率不够高，思路不够流畅，使用的工具不对。理想的情况是像在unix命令行里那样表达想法。主要问题如下：

1. 在GUI下重复进入一个很深的博客内容目录，很浪费时间，并且不能立即写下文字；
2. 每次写文章的时候都是先拷贝一份之前的文档，去掉内容留下模版格式，偷懒导致格式单一；
3. 写完的时候需要微调文章样式，先在本地运行博客系统并在浏览器中看样式，需要在多个软件中切换，很不方便；
4. 涉及到文件上传或者图片上传的时候，需要不断重复（源码需要不断修改）拷贝文件到博客相应目录，重复工作太多；
5. 在中文内容与英文代码之间不断切换；
6. 需要眼睛看键盘，在键盘和显示器之间不断切换；

能想到的部分解决方法是，1) 统一的工作台shell，可以自由的切换软件和路径； 2) 记住Markdown语法和几套不同的样式模版； 3) vi代替GUI编辑器；4) 英语代替中文; 5) 键盘盲打；

目前英文水平有限，除了4之外其他的项目可以有意去训练了。

这里复习一下markdown语法：

1. 标题分为1-6个级别，模版使用1级和3级标题，注意井字之后需要一个空格；
2. 分割线：```--------------```，还有其他形式，模版中就是它了，分割段落用，最好它的上下各空一行；
3. 段落，文字前后各自空一行即可；
4. 字体有3种效果：斜体、加粗和删除线，对应```*斜体*、**加粗**、~~删除线~~```，符号和文本之间没有空格，模版中会灵活使用；
5. 图片和链接：```[链接](url绝对地址)、![图片](图片绝对地址)```，显示图文超链接，模版中灵活使用；
6. 引用文本：```> 引用别人的文字```，注意空格和可以嵌套使用，模版中一般作为特殊标记文本使用；
7. 无序列表和有序列表：```* 无序项目、1. 有序项目```，模版中经常用，注意空格和嵌套；
8. 代码：```两个反引号括起来或者三个反引号```，区别就是一行和多行的区别，代码中的文字保持源样式即使是md中的特殊符号；

Markdown还有一些其他的语法格式，这里不详细讲，常用的就上面这些。注意有些标记是可以相互嵌套使用的。

文章一般分为几部分：1) 文章标题； 2) 段落； 3) 分级小标题； 4) 修改历史和目录 5) 作者和日期 6) 关键源码等引用 7) 配图和链接等 8) 小结和总结等。这里指的是技术文章，当然像日常的文学作品等是比这个要简洁的，考虑的主要是内容上的。论文是这个要复杂的，讲究的是科学事实和实验过程结果等。

写文章是门大学问，文字能力有限这里不表。

关于写作技术上的问题，先写这么多。下面进入正题：shell脚本。

shell作为CLI软件是一种人机交互界面，是人使用计算机的一种方式（很强大、通用和高效，当然在图形下的应用基本就无能为力了，比如：3D游戏等吧）。shell另一种含义是指shell脚本，可以认为是一种脚本编程语言，一种更高级使用计算机的方式，除了常用的命令、命令组合，shell脚本可以更好的控制组合这些命令（软件），比如可以循环、可以使用变量、可以分支等。在实际开发的时候，有些工作可以使用shell脚本进行自动化，比通用的编程语言简单，比单个命令又要强大一些。

一般shell脚本是以文件的形式存在，在运行之前要用```chmod 777 脚本文件路径```加上可执行权限（777是最大权限）并且在脚本首行加上下面的一句话：

```
#!/bin/bash
```

### 一、数据部分
--------------------

数据主要以变量和数组的形式存在。如下：

```
name="arvin"
name=(1 2 3 "4" 5 6)
name=1
name=1.2

```

使用变量如下：

```
echo $name
echo ${name}
echo ${name[0]}
echo ${name[*]}
echo ${#name}
echo ${#name[*]}
echo ${#name[0]}

$name[0]="4"

```

变量的类型是弱类型的，而且随着的值不同而不同。简单来说，你可以认为它是一块内存，可以存放任何类型的数据。

shell中变量可以是：局部变量、环境变量和shell变量。用的比较多的是局部变量和环境变量。

字符串单独来说一下：

```
# 单引号是原样输出的
# 双引号才能进行连接，插入变量等

echo "hello, "$name
echo "hello, ${name}"
echo $hello $name
echo "legth: ${#name}"
echo ${name:1:2}

# 连接操作中间是没有空格的，主要因为shell中空格是用于区分参数的

```

这里说一下shell中的注释：

```
单行用 #

多行用 

:<<EOF
注释内容
EOF

```

在shell脚本中获取命令行中的参数使用```$0、$1、$2....$n```的形式，函数传递参数也是这样。

最后看一句：

```
for file in `ls ./`; do echo file; done;

```

shell脚本考虑到了在命令后中单行写出全部脚本的内容的方式，$变量永远是获取变量的值形式，`#`永远是获取长度，反引号永远是在引用执行代码。


### 二、数据操作
-------------------------

基本运算：

```
# 采用expr命令计算表达式，+\-\\*\/\%\=\==\!=
echo `expr 2 + 2 * $x` # expr要求数值或变量跟运算符之间要有空格

if [ $a != 1 ]; then echo "hello"; fi;

# 关系运算, -eq\-ne\-gt\-lt\-ge\-le
if [ $a -eq $b ]; then echo "hello"; fi;

# 布尔运算，!\-a\-o
if [ $a != 1 ]; then echo "hello"; fi;
if [ $a -a $b ]; then echo "hello"; fi;

# 逻辑运算，&&、||
if [[ $a -lt 100 && $b -gt 100 ]]; then echo "hello"; fi;

# 字符串运算符，=\!=\-z\-n\str
if [ $a = "hello" ]; then echo "hello"; fi;

# 文件测试运算，-b\-c\-d\-f\-g\-k\-p\-u\-r\-w\-x\-s\-e
if [ -f $file ]; then echo "hello"; fi;

```

信息输出显示：

```
echo "It is a test"
echo "\"it is a test\""
read name
echo "my name is ${name}\n"
echo 'my name is arvin!'
echo 'hello, world!' > file.txt
echo `date`

# printf format-str [arg0 arg1 arg2 ... argn]
# printf命令是模拟C等的输出函数
printf "%-10s %-8s %-4s\n" 姓名 性别 体重kg  
printf "%-10s %-8s %-4.2f\n" 郭靖 男 66.1234 
printf "%-10s %-8s %-4.2f\n" 杨过 男 48.6543 
printf "%-10s %-8s %-4.2f\n" 郭芙 女 47.9876 

```

test命令：

```
# test命令在shell中主要用于某个条件是否成立，可以对数值、字符串和文件条件进行测试
# -eq\-ne\-gt\-ge\-lt\-le
if test $[num] -eq $[num2]; then echo "hello"; fi;

# 字符串测试，=\!=\-z\-n
if test $str1 = $str2; then echo "hello"; else echo "world"; fi;

# 文件测试，-e\-r\-w\-x\-s\-d\-f\-c\-d
if test -e ./bash; then echo "存在"'; else echo "不存在"; fi;

```

流程控制：

```
# 条件分支
if [ $a -eq $b ]; then echo "hello"; elif [ $a -gt $b ]; then echo "hello1"; else echo "hello2"; fi;

read num
case $num in; 1) echo "1";;; 2) echo "2";;; 3) echo "3";;; esac;

# 循环操作
for file in Ada Java Coffe; do echo $file; done;

while(( $int<=5 )); do echo $int; let "int++"; done;

while :; do echo "hello"; done;
while true; do echo "hello"; done;

unit [ ! $a -lt 10 ]; do echo "hello"; done;

# 循环的2个跳出方式
break
continue

```

函数：

```
[function] name [()]
{
	action;
	[return int;]
}

demofunc(){
	echo "hello'
}

func1(){
	return $(($1+$2))
}
func1
echo "hello, $?"

```

shell脚本可以包含其他shell脚本的

```
. ./test.sh

或者

source ./test.sh

```

### 三、文件重定向
----------------------------

重定向功能是类unix系统独有的，类似的还有管道的概念。

```

输出重定向：>
输入重定向：<
输出以追加的方式重定向：>>

文件描述符n重定向到file：n > file
类似的如下：

n >> file
n >& m    # 将输出文件m和n合并
n <& m    

<<tag
内容
tag

# 注意
标准输入（stdin）：0
标准输出（stdout）：1
标准错误（stderr）：2


# 特殊文件
/dev/null
/dev/zero

```

输入输出重定向需要认真学好。管道也是。

最后看两句：

```
svn st | awk '{if($1=="?"){print $2}}' | xargs svn add
svn st | awk '{if($1=="!"){print $2}}' | xargs svn delete

```

[AWK](http://www.ruanyifeng.com/blog/2018/11/awk.html)


### 四、总结
---------------------

1. shell脚本很简单，主要还是学习控制流、输出、条件和重定向
2. 单个命令（软件、程序等）和命令组合使用是用好shell接口的关键
3. 开发过程中需要留下心眼，如果能用shell脚本自动化尽量写成shell脚本，避免重复浪费时间（生命）

------------------------

>END
