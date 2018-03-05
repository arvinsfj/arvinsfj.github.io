
### C标准库测试

=========================

```
#include <assert.h>

void test_assert(int argc)
{
    assert(argc==1);
    assert(argc==2);
}

int main(int argc, char** argv)
{
    test_assert(argc);
    return 0;
}

```

原型：void assert(int expr);

assert是一个宏而非函数。不返回任何值，在expr为真时不做任何操作程序继续执行，在expr为假（expr等于0）时中断程序执行并在stderr上显示错误信息。用于给C程序添加诊断（断言）。

如果定义了宏NDEBUG，则assert宏失效。assert宏可以使用printf函数和abort函数通过逗号表达式（expr1,expr2）组合定义。如果断言为假的时候，先执行printf函数打印错误信息，后执行abort函数中断程序。

```
#include <ctype.h>

void test_ctype()
{
    //传入int，返回int

    //测试函数
    //isalnum函数、isalpha函数、iscntrl、isdigit、isgraph、islower、isprint、ispunct、isspace、isupper、isxdigit
    if (ispunct('?')) {
        printf("%s\n", "true");
    } else {
        printf("%s\n", "false");
    }

    //转换函数
    printf("%c:%c\n", toupper('a'), tolower('G'));
}

int main(int argc, char** argv)
{
    test_ctype();
    return 0;
}
```

原型：int isalpha(int c);
原型：int tolower(int c);

ctype库包含字符测试函数和字母大小写转换函数。所有函数参数都是int类型（字符），返回类型都是int类型。测试函数如果测试为真则返回非0，如果测试为假则返回0。转换函数返回字母的大写或者小写字母（ascii数值）。

注意字符的各种集合定义（范围）。比如：10进制数字、16进制数字、小写字母、大写字母、字母、字母数字、标点符号、图形字符（字母、数字、标点的并集）、空格（制表符、换行、垂直制表符、换页、回车、空格）、打印（字母数字、标点、空格）、控制（ascii中8进制的000-037和177）、空白（空格、制表符）。

实现的话，无非是将传入的字符判断是否在相应的集合范围中。转换函数的实现是字母加上或者减去固定的数值（大小写字母ascii的差值'a'-'A'=97-65=32）。使用的时候，注意传入的字符。

```
#include <errno.h>

void test_errno()
{
    FILE* fp = fopen("file.txt", "rt");
    if (fp == NULL) {
        fprintf(stderr, "value of errno: %d\n", errno);
        fprintf(stderr, "error opening file: %s\n", strerror(errno));
    } else {
        fclose(fp);
    }
//
    errno = 0;
    double val = sqrt(-10);
    if (errno == EDOM) {
        printf("Invalid argument\n");
    }

    errno = 0;
    val = log(2.000000);
    if (errno == ERANGE) {
        printf("Log(%f) is out of rang\n", 2.00000);
    }
}

int main(int argc, char** argv)
{
    test_errno();
    return 0;
}
```

原型：#define errno (*__error())

errno是一个宏而非函数，__error()是返回int指针的函数。errno定义了__error函数调用并获取int指针所指向的int值。
通过函数调用很好的隐藏了error原始的数值（系统底层是有一个int变量存放错误号的）。错误变量自身是有系统api（函数调用）设置的（也可以自己手工设置，比如：设置为0）。

如果调用系统函数的时候（C库函数），出现错误，则错误变量会被设置成对应的错误号。比如：打开一个不存在的文件，会产生错误2，错误变量会被设置成2，通过errno的宏可以获取到错误号2，通过strerr(errno)函数可以获取错误信息。

```
#include <float.h>

void test_float()
{
    //floating-point = ( S ) p x be

    printf("The maximum value of float = %.10e\n", FLT_MAX);
    printf("The minimum value of float = %.10e\n", FLT_MIN);
    printf("The minimum value of float epsilon = %.10e\n", FLT_EPSILON);
    printf("%d:%d:%d:%d:%d:%d:%d:%d\n", FLT_ROUNDS, FLT_RADIX, FLT_MANT_DIG, FLT_DIG,FLT_MIN_EXP,FLT_MAX_EXP,FLT_MIN_10_EXP,FLT_MAX_10_EXP);
}

int main(int argc, char** argv)
{
    test_float();
    return 0;
}
```

float.h 头文件包含了一组与浮点值相关的依赖于平台的常量。这些常量是由 ANSI C 提出的，这让程序更具有可移植性。

浮点数，即实数（floating-point = (+/-) precision x base^exponent），由符号、基数、指数和精度四部分组成。

FLT 是指类型 float，DBL 是指类型 double，LDBL 是指类型 long double。

```
#include <limits.h>

void test_limits()
{
    printf("The number of bits in a byte: %d\n", CHAR_BIT);

    printf("The maximum value of SIGNED CHAR = %d\n", SCHAR_MAX);
    printf("The maximum value of UNSIGNED CHAR = %d\n", UCHAR_MAX);

    printf("The minimum value of CHAR = %d\n", CHAR_MIN);
    printf("The maximum value of CHAR = %d\n", CHAR_MAX);

    printf("The minimum value of SHORT INT = %d\n", SHRT_MIN);
    printf("The maximum value of SHORT INT = %d\n", SHRT_MAX);

    printf("The minimum value of INT = %d\n", INT_MIN);
    printf("The maximum value of INT = %d\n", INT_MAX);

    printf("The minimum value of LONG = %ld\n", LONG_MIN);
    printf("The maximum value of LONG = %ld\n", LONG_MAX);
}

int main(int argc, char** argv)
{
    test_limits();
    return 0;
}
```

limits.h头文件中的宏限制了各种变量类型（比如 char、short、int 和 long）的值。这些限制指定了变量不能存储任何超出这些限制的值。即，定义了各种类型变量的最大和最小值（整数）。浮点数定义在float.h头文件中。

这些数值跟平台相关，有些平台short跟int所占用的字节数量是相同的，即最大和最小值范围相同。其次，整数在机器中都是以补码的形式存在的（存储和运算），其表示（存储）方式跟浮点数完全不同。如果是有符号整数，则最高位表示符号正负，剩下的字节位决定了取值范围。如果是无符号的，则字节的所有位都表示正数的取值范围。整数所有字节在机器（主机）内存中以小端序存储（数值的低位字节存储在内存的低地址）。

```
#include <locale.h>

void test_locale()
{
    //LC_ALL、LC_COLLATE、LC_CTYPE、LC_MONETARY、LC_NUMERIC、LC_TIME
    //char *setlocale(int category, const char *locale)
    //struct lconv *localeconv(void)

    time_t currtime;
    struct tm* timer;
    char buffer[80];

    time( &currtime );
    timer = localtime( &currtime );

    printf("Locale is: %s\n", setlocale(LC_ALL, "en_GB"));
    strftime(buffer,80,"%c", timer );
    printf("Date is: %s\n", buffer);

    printf("Locale is: %s\n", setlocale(LC_ALL, "de_DE"));
    strftime(buffer,80,"%c", timer );
    printf("Date is: %s\n", buffer);

    struct lconv* lc;
    setlocale(LC_MONETARY, "en_US");
    lc = localeconv();
    printf("Local Currency Symbol: %s\n",lc->currency_symbol);
    printf("International Currency Symbol: %s\n",lc->int_curr_symbol);

    setlocale(LC_MONETARY, "it_IT");
    lc = localeconv();
    printf("Local Currency Symbol: %s\n",lc->currency_symbol);
    printf("International Currency Symbol: %s\n",lc->int_curr_symbol);
}

int main(int argc, char** argv)
{
    test_locale();
    return 0;
}
```

原型：char *setlocale(int category, const char *locale)
原型：struct lconv *localeconv(void)

locale.h 头文件定义了本地化设置，比如日期格式和货币符号。6个宏，1个lconv结构体，2个函数。
通过设置函数可以影响系统lconv结构体变量的全部字段或者部分字段（由分类宏决定），通过获取函数得到系统的结构体变量指针。category是6个宏中的任意一个，locale是本地化字符串（比如：en_US等）。

通过设置本地化参数，先是修改系统的lconv结构体变量中的字段，之后回影响到其他的库函数。比如：设置LC_CTYPE的本地化参数，会影响到所有的字符库函数（换句话讲，其他的库函数回使用系统lconv结构体变量中的相应字段的值）。系统lconv结构体变量是全局的。

```
#include <math.h>

void test_math()
{
    //传入double类型参数，返回double类型返回值

    //HUGE_VAL宏，当函数的结果不可以表示为浮点数时。
    //如果是因为结果的幅度太大以致于无法表示，则函数会设置 errno 为 ERANGE 来表示范围错误，并返回一个由宏 HUGE_VAL 或者它的否定（- HUGE_VAL）命名的一个特定的很大的值。
    //如果结果的幅度太小，则会返回零值。在这种情况下，error 可能会被设置为 ERANGE，也有可能不会被设置为 ERANGE。

    //数学函数
    printf("sin(30) = %lf\n", sin(30.0/180*M_PI));
    printf("cos(60) = %lf\n", cos(60.0/180*M_PI));
    printf("tan(45) = %lf\n", tan(45.0/180*M_PI));
    //
    printf("exp(3) = %lf\n", exp(3));
    printf("log(3) = %lf\n", log(3));
    printf("pow(2,10) = %lf\n", pow(2,10));
    printf("sqrt(16) = %lf\n", sqrt(16));
}

int main(int argc, char** argv)
{
    test_math();
    return 0;
}
```

常用的数学函数，比如：三角函数、指数函数、对数函数、平方根函数、上下限函数、绝对值函数、余数函数等。分为：double、float、long double三种参数类型。unix扩展部分，增加了一些函数和常量宏定义。

基本覆盖了能用到的基本数学函数。这些函数的实现，按照数学定义即可。

```
#include <setjmp.h>

static jmp_buf buf;

void second(void) {
    printf("second\n");         // 打印
    longjmp(buf,1);             // 跳回setjmp的调用处 - 使得setjmp返回值为1
}

void first(void) {
    second();
    printf("first\n");          // 不可能执行到此行
}

void test_setjmp()
{
    if ( !setjmp(buf) ) {
        first();                // 进入此行前，setjmp返回0
    } else {                    // 当longjmp跳转回，setjmp返回1，因此进入此行
        printf("main\n");       // 打印
    }
}

int main(int argc, char** argv)
{
    test_setjmp();
    return 0;
}
```

原型：int setjmp(jmp_buf environment)
原型：void longjmp(jmp_buf environment, int value)

C语言给出的长跳转功能。1个存储调用环境变量数组，2个定义跳转的函数。部分平台扩展到了signal的长跳转。

setjmp函数把当前环境保存在变量 environment 中，以便函数 longjmp() 后续使用。如果这个宏直接从宏调用中返回，则它会返回零，但是如果它从 longjmp() 函数调用中返回，则它会返回一个非零值。

longjmp函数恢复最近一次调用 setjmp() 宏时保存的环境，jmp_buf 参数的设置是由之前调用 setjmp() 生成的。value是返回给setjmp函数作为其返回值。

setjmp函数回返回两次，第一次是保存buf后返回（返回值为0），第二次是longjmp函数调用之后返回到setjmp函数处，并且setjmp函数返回值为longjmp函数的实参value的值。

上面的例子中first函数的语句printf("first\n");永远不会被执行。

```
#include <signal.h>

void sighandler(int signum) {
    printf("捕获信号 %d，跳出...\n", signum);
    exit(1);
}

void test_signal()
{
    //信号处理：SIG_DFL、SIG_ERR、SIG_IGN
    //信号码：SIGABRT、SIGFPE、SIGILL、SIGINT、SIGSEGV、SIGTERM
    //函数：void (*signal(int sig, void (*func)(int)))(int)
    //函数：int raise(int sig)

    signal(SIGINT, sighandler);
    while(1) {
        printf("开始休眠一秒钟...\n");
        sleep(1);
        //raise(SIGINT);
    }
}

int main(int argc, char** argv)
{
    test_signal();
    return 0;
}
```

C给出的系统信号处理方法。可以忽略某些信号，可以默认处理信号，可以报错某些信号，还可以自定义信号处理程序。信号由硬件或者人工产生，需要特定的处理。这种机制，给了程序处理信号的方法，并且回打断程序的执行流程。最常见的莫过于中断（Ctrl+C）信号的处理。信号提供了程序的生命周期、异常处理、特定交互等相关处理入口。

上面的例子，正常是一个死循环程序，当接收到SIGINT信号时，会挂起死循环执行sighandler函数。即，信号的执行优先级高于普通的程序优先级。

```
#include <stdarg.h>

void test_stdarg(int num, ...)
{
    //处理可变参数函数的参数获取，一个变量类型，三个处理宏
    int val = 0;
    va_list ap;
    va_start(ap, num);
    for(int i = 0; i < num; i++) {
        val += va_arg(ap, int);
    }
    va_end(ap);
    printf("%d\n", val);
}

int main(int argc, char** argv)
{
    test_stdarg(3,10,20,30);
    test_stdarg(4,10,20,30,40);
    return 0;
}
```

C给出的一种可变形参的函数定义方法。一般这类函数的最后一个确定参数显式或者隐式表示可变参数的个数。（比如：printf函数就是隐式的表示了可变参数的个数）

va_list类型存储三个宏的参数信息。va_start宏通过最后一个确定型参数初始化va_list类型变量，va_arg宏通过va_list变量和参数类型获取可变参数的值，最后va_end宏清理va_list变量。

注意上面的三个宏，并不能拿到可变参数的个数，需要显式传入或者判断结束，来确定循环的终止。

实现上，基本是拿到最后一个确定参数的结尾地址，作为可变参数的起始地址，然后通过可变参数的类型确定某参数所占字节个数，并且作为一个数值返回给应用程序。重复上面的步骤，直到返回全部的可变参数数值，最后清理一下临时va_list变量。这其中需要临时变量（游标）记录当前可变参数。

```
#include <stddef.h>

void test_stddef()
{
    //定义了各种变量类型和宏。这些定义中的大部分也出现在其它头文件中。

    //变量类型：ptrdiff_t、size_t、wchar_t、NULL
    printf("%lu:%lu:%lu:%lu\n", sizeof(ptrdiff_t), sizeof(size_t), sizeof(wchar_t), sizeof(NULL));

    //偏移宏：offsetof(type, member-designator)
    struct off {
        int k;
        char c;
        short s;
    };
    printf("%lu\n", offsetof(struct off, s));
}

int main(int argc, char** argv)
{
    test_stddef();
    return 0;
}
```

stddef .h 头文件定义了各种变量类型和宏。这些定义中的大部分也出现在其它头文件中。NULL是在该头文件中作为宏定义，并且是void*类型，值是0。offsetof宏计算结构体成员偏移字节的。其他含定义了ptrdiff_t、size_t、wchar_t等变量类型。注意成员偏移字节的计算，需要考虑字节对齐。

```
#include <stdio.h>

void test_stdio()
{
    //输入输出相关的函数库
    //库变量类型：size_t、FILE、fpos_t
    //库宏：NULL、BUFSIZ、EOF、SEEK_CUR、SEEK_END、SEEK_SET、stdin、stdout、stderr等
    //库函数：文件流等，有很多非常巧妙的使用方法，比如：sscanf函数，同时注意格式化字符串的使用
    //文件流、标准流、字符串

    char buffer[512];
    FILE* fp = fopen("ctest.md", "rt");

    fgets(buffer, 511, fp);
    fgets(buffer, 511, fp);

    printf("%s\n", buffer);

    fclose(fp);
}

int main(int argc, char** argv)
{
    test_stdio();
    return 0;
}
```

输入输出流，是非常实用的库，使用非常多。使用的时候，要注意格式化字符串的使用方法，以及各个函数的边界处理方法。这些函数的底层实现基本都是调用操作系统的系统调用来实现的。使用的时候，遵循：打开-读写-关闭的模式。

```
#include <stdlib.h>

void test_stdlib()
{
    //库变量类型：size_t、wchar_t、div_t、ldiv_t
    //库宏：NULL、EXIT_FAILURE、EXIT_SUCCESS、RAND_MAX、MB_CUR_MAX

    //库函数：字符串转数值、内存分配、重新分配和回收、程序终止相关、环境变量、system命令、绝对值、除法、数组查找和排序、随机数、字符串和字符数组转换

    printf("%d\n", atoi("123"));
    char* k = malloc(2);
    k[0] = 1;k[1]=2;
    int v = *((short*)k);
    printf("%d\n", v);
    free(k);
}

int main(int argc, char** argv)
{
    test_stdlib();
    return 0;
}
```

主要是字符串转数值、内存管理函数和随机数生成函数。有时间可以研究一下这些函数的底层实现。

```
#include <string.h>

void test_string()
{
    //库变量类型：size_t
    //库宏:NULL
    //库函数：字符串操作函数、内存复制函数

    char buffer[512];
    char* src = "hello,world!";

    memcpy(buffer, src, 13);
    printf("%s\n", buffer);
    printf("%s\n", strncat(buffer, src, 5));
}

int main(int argc, char** argv)
{
    test_string();
    return 0;
}
```

主要是内存复制函（搜索、设置、复制、比较、移除）和字符串处理函数（搜索、连接、比较、复制、长度、分解成token等）。可以探索底层的实现。

```
#include <time.h>

void test_time()
{
    //库变量类型：size_t、clock_t、time_t、struct tm
    //库宏：NULL、CLOCKS_PER_SEC（每秒的处理器时钟周期个数）

    //库函数：时间日历 处理函数
    struct tm t;
    t.tm_sec    = 10;
    t.tm_min    = 10;
    t.tm_hour   = 6;
    t.tm_mday   = 25;
    t.tm_mon    = 2;
    t.tm_year   = 89;
    t.tm_wday   = 6;
    puts(asctime(&t));

    time_t curtime;
    time(&curtime);
    printf("当前时间 = %s", ctime(&curtime));

    clock_t start_t;
    start_t = clock();
    printf("程序启动，start_t = %ld\n", start_t);

    time_t rawtime;
    struct tm *info;
    char buffer[80];
    time( &rawtime );
    info = localtime( &rawtime );
    strftime(buffer,80,"%x - %I:%M%p", info);
    printf("格式化的日期 & 时间 : |%s|\n", buffer );
    
    printf("%d\n", CLOCKS_PER_SEC);
}

int main(int argc, char** argv)
{
    test_time();
    return 0;
}
```

时间日期处理函数。注意tm是结构体，time_t和clock_t都是整数。

------------------------

>END

[代码下载](documents/ctest.zip)

