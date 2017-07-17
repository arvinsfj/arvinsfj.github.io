
# 一个简单的类BASIC解释器

> *作者：Arvin 日期：2017年7月17日*

---------------------------------

>BEGIN

大学里面学习数据结构（C版）学的比较不错，我的编程人生大概是从这个地方开始的。某个业务适合用什么样的数据结构来表达，某个程序使用了什么数据结构，数据结构确定下来，原理性的东西自然就有了方向。比如文本编辑器，我们可以使用双向链表，每个节点表示文本的一行，有了这样的结构想法，其他的编辑操作，就是对双向链表的操作了。不知道为什么，我个人是比较喜欢基于经验、数据结构的自底向上的软件设计，也就是面向过程的程序设计。面向对象虽然大家都在提倡，但是感觉大多数人写着写着就写成了面向过程的了，有些人认为面向对象增加了程序设计的复杂度，而非减小了复杂度，实践中隐约感觉面向对象是违背人的思维方式的（最起码是我的思维方式），但融入了软件工程的特征，更好的项目组织更高效的开发。回到正题，编程语言是规范化的形式语言，其结构是分层的树形数据结构（树形结构基本都采用递归来实现，就像嵌套结构使用栈来实现一样）。比如表达式的结构，其中可以有数字、变量和带括号的表达式。我们应该了解我们使用的编程语言及其实现原理。

### 一、思路
---------------------------------

下面是语言的BNF语法描述：

```

line ::= number statement CR

statement ::= print plist
              let var = expr
              if relation then statement else statement
              for var = expr to expr
              next var
              goto number
              gosub number
              return
              end        

plist ::= (str | , | expr)*
relation ::= expr ((< | > | =) expr)*
expr ::= term ((+|-|&||) term)*
term ::= factor ((*|/|%) factor)*
factor ::= var | number | (expr)

var ::= a | b | c ...| y | z
number ::= digit digit*
digit ::= 0 | 1 | 2 | 3 | ... | 8 | 9
str ::= " char* "
char :: = a | b | c ...| y | z ... | A | B | C ... | Y | Z ... | ! | @ | # | $ ... | ( | ) ...

```

规范化的语言编译器（解释器）实现，一般分为：词法分析、语法分析、语义分析、程序优化、代码生成和优化指令。解释器稍微有点差别，语法分析生成的AST基本上就可以通过（语言）虚拟机解释执行了。解释器（编译器）开发最有趣的地方就是语法分析部分了。我们这篇也着重于语法分析部分。而且我们不遵从规范的解释器来讲解，完全是从程序设计的角度来分析和设计解释器。不带参函数、最多26个变量（单字符表示）、支持if语句和for语句等基本结构。表达式、基本语句、函数、类，只考虑表达式和基本语句。函数如果有兴趣自己去查资料（我自己也不是非常了解函数和类的实现），一方面涉及到执行环境有点复杂，另一方面我喜欢自由（比如汇编，并不需要代码都必须写在函数里面）。回归本质，程序最终都是在操作（运算）变量，表达式是其在语言中的表现。而语句可以认为是对这种操作的组织（分支和循环）。表达式（算术表达式、关系表达式、逻辑表达式）完全遵从递归，语句（if语句和for语句）结构上并不遵从递归，但可以理解成条件跳转。if是根据关系表达式（条件）计算结果选择向下跳转执行代码段，for语句是根据条件选择向下或者向前跳转执行代码段。我们要实现的“语言”的26个变量构成了程序执行的全局环境（由于不支持函数，我们没有局部执行环境），所有程序操作能用的内存就这26个变量。

    
### 二、我们的程序是由什么组成的？
---------------------------------

简单来讲，就是由一行一行的字符串组成的。进一步讲，是由特定字符（字符串）遵从逻辑结构编写的字符串组成。我们将这些字符分类会得到所谓的Token，比如单字符变量和操作符号、数字的数值（字符串）、字符串常量、语言关键字（字符串）。逻辑上，我们可以先判断数字字符串，其次判断单字符操作符号，然后判断字符串常量（"开头和结尾），判断语言关键字，最后判断单字符变量（a ~ z）。逻辑结构就是let语句、print语句、if语句和for语句，最重要的还是if语句和for语句。写解释器的第一步就是根据输入的程序字符串划分各种token（字符串片段）。具体来讲，首先需要一个初始化方法tokenizer_init初始化分词程序，其次需要一个获取当前token的方法tokenizer_token，然后是获取当前tokne的值方法（字符串、变量、数值），还需要一个获取下一个token的方法tokenizer_next，最后还需要分词结束判断方法tokenizer_finished和分词错误中断方法tokenizer_error_print。另一种思路是将token类别和token的值封装在一个结构体中形成完整的token实体，对外提供初始化方法、获取下一个token方法、分词结束判断方法、分词错误方法。我们按照第一种方法实现（C语言风格），并且是边分词边执行。

初始化方法：

```
void tokenizer_init(const char *program)
{
    ptr = program;
    current_token = get_next_token();
}

我们需要记录程序的当前扫描位置和当前token。

```

获取当前token方法：

```
int tokenizer_token()
{
    return current_token;
}

```

接下来根据当前token类型，来获取当前token的值：

```
获取数字数值常量，atoi函数自动识别数字结束位置，也可以自己尝试一下写个数字字符串转数值的函数

int tokenizer_num()
{
    return atoi(ptr);
}

获取字符串常量，需要自己识别字符串结束位置（"结尾）

void tokenizer_str(char* dest, int len)
{
    char* str_end;
    int str_len;

    if(tokenizer_token() != TOKENIZER_STRING){
        return;
    }

    str_end = strchr(ptr+1, '"');
    if(str_end == NULL){
        return;
    }
    
    str_len = str_end - ptr - 1;
    if(len < str_len){
        str_len = len;
    }

    memcpy(dest, ptr+1, str_len);
    dest[str_len] = 0;
}

获取变量（下标）方法：

int tokenizer_var_num()
{
    return *ptr - 'a';
}

因为我们只有26个变量，每个字母是一个变量。可以使用一个26个元素的数组存储变量，下标是变量访问地址，可以将0~25映射到'a'~'z'字符。

```

获取下一个token的方法（也就是向前移动游标，分词器的核心）：

```
对外开放的接口

void tokenizer_next()
{
    if(tokenizer_finished()){
        return;
    }

    ptr = nextptr;//一个token是有开始位置和结束位置的，将开始位置ptr移动到结束位置nextptr继续寻找下一个token
    while(*ptr == ' ')++ptr;//跳过空格
    
    current_token = get_next_token();
    
    return;
}

get_next_token()函数是内部分词的核心函数，返回token类型并设置token的开始和结束指针，内部函数习惯上（C语言）加上static关键字

static int get_next_token()
{
    int i;
    
    if(*ptr == 0){
        //程序字符串结尾标志，代表分词结束，也表示程序执行结束（我们的解释器是边分词边执行的）
        return TOKENIZER_ENDOFINPUT;
    }

    if(isdigit(*ptr)){//第一个字符为数字字符
        for(i = 0; i < MAX_NUMLEN; i++){
            if(!isdigit(ptr[i])){
                if(i > 0){
                    nextptr = ptr + i;//设置token结束位置（指针）
                    return TOKENIZER_NUMBER;
                } else {
                    return TOKENIZER_ERROR;//数字太短
                }
            }
            if(!isdigit(ptr[i])){
                return TOKENIZER_ERROR;//数字错误
            }
        }
        return TOKENIZER_ERROR;//数字太长
    } else if(op_char()){
        nextptr = ptr + 1;//我们定义的操作都是1个字符，也可以自己扩展成多字符的操作符，比如&&、||、!=等。
        return op_char();//字符到token类型的映射函数
    } else if(*ptr == '"'){
        nextptr = ptr;//nextptr指向字符串开始字符'"'
        do{nextptr++}while(*nextptr != '"');//找字符串结束字符'"'所在位置（指针）
        nextptr++;//指向下一个字符
        return TOKENIZER_STRING;
    } else {
        //查找关键字，keywords是关键字结构体查找列表（数组）
        for(kt = keywords; kt->kw != NULL; kt++){
            if(strncmp(ptr, kt->kw, strlen(kt->kw)) == 0){
                //匹配成功
                nextptr = ptr + strlen(kt->kw);
                return kt->token;
            }
        }
    }

    //上面都没有返回（没有分词成功），则尝试作为变量分词
    if(*ptr >= 'a' && *ptr <= 'z'){
        nextptr = ptr + 1;//变量都是单个字母，自己可以扩展成字符串
        return TOKENIZER_VARIABLE;
    }

    return TOKENIZER_ERROR;//不在token类别中，则返回错误token类
}

上面除了正常的token类型，新增分词错误TOKENIZER_ERROR和程序结束TOKENIZER_ENDOFINPUT类型

```

接下来是程序分词结束判断方法：

```
int tokenizer_finished()
{
    return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}

```

分词错误方法：

```
void tokenizer_error_print()
{
    printf("tokenizer_error_print: '%s'\n", ptr);
}

```

单字符（操作符号）Token查找方法和关键字Token查找表定义如下：

```
static int op_char()
{
    if(*ptr == '\n') {
        return TOKENIZER_CR;
    } else if(*ptr == ',') {
        return TOKENIZER_COMMA;
    } else if(*ptr == ';') {
        return TOKENIZER_SEMICOLON;
    } else if(*ptr == '+') {
        return TOKENIZER_PLUS;
    } else if(*ptr == '-') {
        return TOKENIZER_MINUS;
    } else if(*ptr == '&') {
        return TOKENIZER_AND;
    } else if(*ptr == '|') {
        return TOKENIZER_OR;
    } else if(*ptr == '*') {
        return TOKENIZER_ASTR;
    } else if(*ptr == '/') {
        return TOKENIZER_SLASH;
    } else if(*ptr == '%') {
        return TOKENIZER_MOD;
    } else if(*ptr == '(') {
        return TOKENIZER_LEFTPAREN;
    } else if(*ptr == ')') {
        return TOKENIZER_RIGHTPAREN;
    } else if(*ptr == '<') {
        return TOKENIZER_LT;
    } else if(*ptr == '>') {
        return TOKENIZER_GT;
    } else if(*ptr == '=') {
        return TOKENIZER_EQ;
    }
    return 0;
}

static const struct {
    char *keyword;
    int token;
} keywords[] = {
    {"let", TOKENIZER_LET},
    {"print", TOKENIZER_PRINT},
    {"if", TOKENIZER_IF},
    {"then", TOKENIZER_THEN},
    {"else", TOKENIZER_ELSE},
    {"for", TOKENIZER_FOR},
    {"to", TOKENIZER_TO},
    {"next", TOKENIZER_NEXT},
    {"goto", TOKENIZER_GOTO},
    {"gosub", TOKENIZER_GOSUB},
    {"return", TOKENIZER_RETURN},
    {"call", TOKENIZER_CALL},
    {"end", TOKENIZER_END},
    {NULL, TOKENIZER_ERROR}
}, *kt;

```

其他杂项定义如下：

```
static char const *ptr, *nextptr;//当前token的开始和结束指针

#define MAX_NUMLEN 5//最大的数字长度

static int current_token = TOKENIZER_ERROR;//当前token默认为ERROR类型

enum {
TOKENIZER_ERROR,
TOKENIZER_ENDOFINPUT,
TOKENIZER_NUMBER,
TOKENIZER_STRING,
TOKENIZER_VARIABLE,
TOKENIZER_LET,
TOKENIZER_PRINT,
TOKENIZER_IF,
TOKENIZER_THEN,
TOKENIZER_ELSE,
TOKENIZER_FOR,
TOKENIZER_TO,
TOKENIZER_NEXT,
TOKENIZER_GOTO,
TOKENIZER_GOSUB,
TOKENIZER_RETURN,
TOKENIZER_CALL,
TOKENIZER_END,
TOKENIZER_COMMA,
TOKENIZER_PLUS,
TOKENIZER_MINUS,
TOKENIZER_AND,
TOKENIZER_OR,
TOKENIZER_ASTR,
TOKENIZER_SLASH,
TOKENIZER_MOD,
TOKENIZER_LEFTPAREN,
TOKENIZER_RIGHTPAREN,
TOKENIZER_LT,
TOKENIZER_GT,
TOKENIZER_EQ,
TOKENIZER_CR,
};

```

### 三、语法是什么？
---------------------------------



### 四、将Token作为命令来解释和执行
---------------------------------



### 五、其他代码
---------------------------------



>END

[代码下载](Tools/BasicLang/basic_lang.zip)

