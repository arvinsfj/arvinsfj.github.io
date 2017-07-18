
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

你可以想想程序中除了关键字、数字常量、变量、操作符号、字符串常量还有什么。

### 三、语法是什么？
---------------------------------

如果你已经对Token有一定的概念，那么语法就是Token的排列（组合）规则。规则都是可以模拟的。为了表达事物的逻辑，编程语言引入了几种逻辑结构，比如分支条件结构、循环结构等。人讲话或者写作（比如英语的从句）大部分其实是递归结构的，编程语言相应的引入了嵌套结构。分支、循环和（递归）嵌套是编程语言的基本结构和规则。具体来讲，if语句后面必须跟关系表达式再跟then，然后是执行语句，再就是else语句；for必须跟next成对使用；let后面必须跟变量再跟=号，后面是表达式；那么多token，有很多组合，而实际的编程语言语法只是其中的很小部分排列组合，关系比较确定。表达一门编程语言的语法可以使用上面的BNF描述，采用递归下降的方式表达，左边是模糊的概念，右边是相对清晰的定义，上面比较模糊，下面比较清晰，中间存在递归结构，但是最终都会用确定的token进行描述。token可以理解成对程序文本的各个字符串或字符的定义，类似单词的概念，程序都是由token组成的。BNF不仅包含语言的语法描述，还可以指导语法的编程实现，是语法实现的整体框架。比如某个结构的*，可以使用循环来实现；多个结构的并列可以使用分支条件实现；左边是函数名称右边是函数实现；右边如果包含左边名称则表示函数调用；越在下面的运算的优先级越高，越早实现和计算，比如乘除运算优先加减运算先进行计算；

按照语法规则，对token进行排列组合，就能得到编译通过（能生成AST的）程序文本（程序表达的逻辑不一定正确）。

什么是factor？就是表达式的基本元素，包含数字（数值）、变量（保存数值的token）、带括号的表达式（递归描述）

什么是term？包含高优先级运算（符号）和基本元素（factor）的token组合，可以只包含factor或者多个操作符号连接的factor

什么是expr？包含低优先级运算（符号）和term的token组合，可以只包含term或者多个操作符号连接的term

什么是relation？包含更低优先级运算（符号）和expr的token组合，可以只包含expr或者多个操作符号连接的expr

还有什么？比如逻辑运算（符号）和relation的token组合。可以自己实现。从上面的分析可以看出，概念是分层和嵌套的，局部可以存在递归结构，最终可以通过嵌套得到最基本的元素。

表达式的递归嵌套结构非常清晰。有了表达式概念，if语句和for语句理解起来就简单的多（非递归结构）。比如if语句，可以认为是根据关系表达式的计算结果进行执行流程跳转。for语句可以理解成if语句的变种，根据关系表达式的结果循环执行（向前跳转）某段程序或者跳出循环执行下一句程序。不同的是for语句需要额外的栈来支持嵌套。非递归结构可以校验token的排列顺序来进行语法校验。递归结构也可以根据token排列结构来校验语法。

变量如何实现？这其实是解释执行阶段需要考虑的问题。简单来实现就是使用26个元素的数组来实现，每个字母映射到数组的下标。

我们先考虑表达式的语法实现。

factor的实现（直接返回factor的数值）：

```
static int factor()
{
    int r;
    switch(tokenizer_token()){
        case TOKENIZER_NUMBER:
            r = tokenizer_num();
            accept(TOKENIZER_NUMBER);
            break;
        case TOKENIZER_VARIABLE:
            r = vbasic_get_var(tokenizer_var_num());//从变量中获取值
            accept(TOKENIZER_VARIABLE);
            break;
        case TOKENIZER_LEFTPAREN:
            accept(TOKENIZER_LEFTPAREN);
            r = expr();
            accept(TOKENIZER_RIGHTPAREN);
            break;
    }
    return r;
}

```

term的实现：

```
static int term()
{
    int f1, f2;
    int op;
    
    //仔细看下面的逻辑结构和BNF中的描述结构，是有多么的相似。
    f1 = factor();
    op = tokenizer_token();
    while(op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD){
        tokenizer_next();//为什么不能使用accept函数
        f2 = factor();
        
        switch(op){
            case TOKENIZER_ASTR:
                f1 *= f2;
                break;
            case TOKENIZER_SLASH:
                f1 /= f2;//除数不能为0，想想怎么校验
                break;
            case TOKENIZER_MOD:
                f1 %= f2;//f2不能为0
                break;
        }
        
        op = tokenizer_token();
    }

    return f1;//计算结果放在f1中
}

```

expr的实现：

```
static int expr()
{
    int t1, t2;
    int op;

    //仔细看下面的逻辑结构和BNF中的描述结构，是有多么的相似。
    t1 = term();
    op = tokenizer_token();
    while(op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR){
        tokenizer_next();//为什么不能使用accept函数
        t2 = term();

        switch(op){
            case TOKENIZER_PLUS:
                t1 += t2;
                break;
            case TOKENIZER_MINUS:
                t1 -= t2;
                break;
            case TOKENIZER_AND:
                t1 &= t2;
                break;
            case TOKENIZER_OR:
                t1 |= t2;
                break;
        }

        op = tokenizer_token();
    }

    return t1;//计算结果放在t1中
}

由于factor函数中会调用expr函数，形成递归。但是factor函数给出了回归条件变量和数字，否则递归不能停止。

```

relation函数的实现：

```
static int relation()
{
    int r1, r2;
    int op;

    //仔细看下面的逻辑结构和BNF中的描述结构，是有多么的相似。
    r1 = expr();
    op = tokenizer_token();
    while(op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ){
        tokenizer_next();//为什么不能使用accept函数
        r2 = expr();

        switch(op){
            case TOKENIZER_LT:
                r1 = r1 < r2;
                break;
            case TOKENIZER_GT:
                r1 = r1 > r2;
                break;
            case TOKENIZER_EQ:
                r1 = r1 == r2;
                break;
        }

        op = tokenizer_token();
    }

    return r1;//计算结果放在r1中
}

```

accept函数的实现：

```
static void accept(int token)
{
    if(token != tokenizer_token()){
        tokenizer_error_print();
        exit(1);
    }

    tokenizer_next();
}

```

仔细观察上面的三个函数，你能领悟到什么？比如运算优先级是如何实现的；代码相似的原因；如何分开语法校验和程序的解释执行等。

接下来考虑非递归结构，比如if语句和for语句等。昨晚写太晚了，今天继续写了。

我们从简单结构入手，比如goto语句。goto语句本质是强制跳转到某一行继续执行。首先找到这一行的数值，然后扫描程序token直到找到该行。

goto函数定义：

```
static void goto_statement()
{
    accept(TOKENIZER_GOTO);//为什么可以使用accept函数
    int line_num = tokenizer_num();
    jump_linenum(line_num);
}

jump_linenum函数是移动token游标直到指向某行，通用函数，gosub、return和next语句都需要使用

static void jump_linenum(int linenum)
{
    tokenizer_init(program_ptr);//从头开始扫描，是否可以改进
    while(tokenizer_num() != linenum){
        do{//注意循环的结束条件
            do {tokenizer_next();} while(tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
            if(tokenizer_token() == TOKENIZER_CR) {tokenizer_next();}
        }while(tokenizer_token() != TOKENIZER_NUMBER);
    }
}

```

简单处理一下print语句：

```
static void print_statement()
{
    accept(TOKENIZER_PRINT);
    do{
        if(tokenizer_token() == TOKENIZER_STRING) {
            tokenizer_str(string, sizeof(string));
            printf("%s", string);
            tokenizer_next();
        } else if(tokenizer_token() == TOKENIZER_COMMA) {//每个逗号代表一个空格
            printf(" ");
            tokenizer_next();
        } else if(tokenizer_token() == TOKENIZER_VARIABLE || tokenizer_token() == TOKENIZER_NUMBER) {//条件是否充分
            printf("%d", expr());//输出表达式，这个地方连接到了表达式的解析和运算（语法和语义分析）
        } else {
            break;
        }
    }while(tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
    printf("\n");
    tokenizer_next();
}

```

注意观察代码实现跟BNF描述之间的关系。依照上面的思路不难写出赋值语句实现。

let函数：

```
static void let_statement()
{
    int var;
    var = tokenizer_var_num();
    
    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_EQ);
    vbasic_set_var(var, expr());//使用表达式给变量赋值
    accept(TOKENIZER_CR);
}

```

基本结构if语句的实现，应该也不难。依照语法顺序读取token和计算表达式，根据表达式计算结果跳转if子句或else子句。

if函数：

```
static void if_statement()
{
    int r;
    
    accept(TOKENIZER_IF);
    r = relation();
    accept(TOKENIZER_THEN);
    if(r){
        statement();//使用递归结构，因为if子句可以执行任何语句
    } else {
        do { tokenizer_next(); } while(tokenizer_token() != TOKENIZER_ELSE && tokenizer_token() != TOKENIZER_CR && tokenizer_token() != TOKENIZER_ENDOFINPUT);
        
        if(tokenizer_token() == TOKENIZER_ELSE) {
            tokenizer_next();
            statement();//else子句
        } else if(tokenizer_token() == TOKENIZER_CR) {
            tokenizer_next();
        }
    }
}

可以看到if结构的实现不是很难，主要是需要区分else子句是否存在。还有一个是if语句嵌套问题，上面是通过递归来处理嵌套，并没有特意去处理嵌套。因为if子句可以执行任何语句，当然包括if语句本身。函数递归调用本身就是一个栈结构，栈结构是用来处理嵌套的。

```

不带参数的函数调用（带参数的自己去研究），跟goto语句类似，仅多了记录返回位置（行号）的操作，被调用函数中需要使用return语句中止函数执行并返回到调用位置继续执行。函数调用当然需要栈结构来暂存返回地址。带参数的函数调用参数也是存储在栈中的。

gosub函数:

```
static void gosub_statement()
{
    int linenum;
    accept(TOKENIZER_GOSUB);
    linenum = tokenizer_num();//行号作为返回地址
    accept(TOKENIZER_NUMBER);
    accept(TOKENIZER_CR);
    if(gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
        gosub_stack[gosub_stack_ptr] = tokenizer_num();//存储gosub语句下一行语句的行号（地址）作为返回地址
        gosub_stack_ptr++;
        jump_linenum(linenum);//跳转
    } else {
        printf("gosub_statement: gosub stack exhausted\n");
    }
}

gosub_stack是一个数组（作为函数调用栈），gosub_stack_ptr是数组下标作为栈顶指针。

```

return函数：

```
static void return_statement()
{
    accept(TOKENIZER_RETURN);
    if(gosub_stack_ptr > 0) {
        gosub_stack_ptr--;
        jump_linenum(gosub_stack[gosub_stack_ptr]);//返回调用位置的下一行语句开始执行
    } else {
        printf("return_statement: non-matching return\n");
    }
}

```

for-next语句，首先要处理循环执行问题（回退式的条件跳转），其次要处理子句嵌套问题。我们可以按照if语句的方式处理跳转和嵌套，但是嵌套的深度不可以控制。换一种思路，我们可以按照函数调用的方式来处理for语句嵌套问题，即自定义调用栈来处理嵌套。相对于函数调用for语句需要多保存一个变量和一个结束值，当然需要记录返回位置（行号或地址）。我们需要一个结构体记录这些数据。并且将该结构实体入栈，实现可控的嵌套。next语句检查条件，并作出回退跳转还是中止循环执行下一行语句。for语句初始化循环环境结构体数据（步进变量初始值、结束值、循环体第一句语句行号（地址））并入栈。

for函数：

```
static void for_statement()
{
    int for_variable, to;

    accept(TOKENIZER_FOR);
    for_variable = tokenizer_var_num();//获取步进变量（下标或索引）
    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_EQ);
    vbasic_set_var(for_variable, expr());//类似let语句，设置步进变量初始数值
    accept(TOKENIZER_TO);
    to = expr();//设置结束数值
    accept(TOKENIZER_CR);

    if(for_stack_ptr < MAX_FOR_STACK_DEPTH) {
        for_stack[for_stack_ptr].line_after_for = tokenizer_num();//记录循环体的第一句行号（地址）
        for_stack[for_stack_ptr].for_variable = for_variable;//记录步进变量（数组下标）
        for_stack[for_stack_ptr].to = to;//记录结束数值
        for_stack_ptr++;//入栈
    } else {
        printf("for_statement: for stack depth exceeded\n");
    }
}

for_stack是数组作为循环嵌套栈使用，里面存储循环环境结构体，for_stack_ptr是循环栈栈顶指针（数组下标）。

```

next函数：

```
static void next_statement()
{
    int var;
    accept(TOKENIZER_NEXT);
    var = tokenizer_var_num();//获取步进变量（下标）
    accept(TOKENIZER_VARIABLE);

    if(for_stack_ptr > 0 && var == for_stack[for_stack_ptr - 1].for_variable) {
        vbasic_set_var(var, vbasic_get_var(var) + 1);//步进变量增加1
        if(vbasic_get_var(var) <= for_stack[for_stack_ptr - 1].to) {//步进变量小于结束数值则继续循环
            jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);//回退跳转继续执行循环
        } else {
            for_stack_ptr--;//出栈
            accept(TOKENIZER_CR);//跳出循环
        }
    } else {
        printf("next_statement: non-matching next (expected %d, found %d)\n", for_stack[for_stack_ptr - 1].for_variable, var);
        accept(TOKENIZER_CR);
    }
}

```

仔细观察上面的gosub-return语句和for-next语句的实现，是多么的相似。

最后是程序结束end语句。

end函数：

```
static void end_statement()
{
    accept(TOKENIZER_END);
    ended = 1;//程序结束标志变量
}

```

### 四、将Token作为命令来解释和执行
---------------------------------

上面是全部的表达式和语句的语法分析（校验是否符合语法规则）和语义分析执行的全部核心实现。还需要额外的接口函数，比如statement函数，line函数等。

statement函数：

```
static void statement()
{
    int token;

    token = tokenizer_token();

    switch(token) {
        case TOKENIZER_PRINT:
            print_statement();
            break;
        case TOKENIZER_IF:
            if_statement();
            break;
        case TOKENIZER_GOTO:
            goto_statement();
            break;
        case TOKENIZER_GOSUB:
            gosub_statement();
            break;
        case TOKENIZER_RETURN:
            return_statement();
            break;
        case TOKENIZER_FOR:
            for_statement();
            break;
        case TOKENIZER_NEXT:
            next_statement();
            break;
        case TOKENIZER_END:
            end_statement();
            break;
        case TOKENIZER_LET:
            accept(TOKENIZER_LET);
        /* Fall through. */
        case TOKENIZER_VARIABLE:
            let_statement();
            break;
        default:
            printf("ubasic.c: statement(): not implemented %d\n", token);
            exit(1);
    }
}

```

line函数：

```
static void line_statement()
{
    accept(TOKENIZER_NUMBER);
    statement();
}

```

run函数：

```
void vbasic_run()
{
    if(tokenizer_finished()) {
        printf("vBASIC program finished\n");
        return;
    }

    line_statement();
}

```

finished函数：

```
int vbasic_finished()
{
    return ended || tokenizer_finished();
}

```

init初始化函数：

```
void vbasic_init(const char *program)
{
    program_ptr = program;
    for_stack_ptr = gosub_stack_ptr = 0;
    tokenizer_init(program);
    ended = 0;//程序结束标志，0：未结束 1：已结束
}

program_ptr是程序文本的第一个字符的指针。程序的起始地址。跳转都是从头开始扫描的（有点尴尬）。

```

当然不要忘记变量环境的设置和获取函数。

```
void vbasic_set_var(int varnum, int value)
{
    if(varnum > 0 && varnum <= MAX_VARNUM) {
        variables[varnum] = value;
    }
}

int vbasic_get_var(int varnum)
{
    if(varnum > 0 && varnum <= MAX_VARNUM) {
        return variables[varnum];
    }
    return 0;
}

variables是数组，前面讲到的26个元素的数组，作为26个变量的内存空间。字母会映射到下标。

```


### 五、其他代码
---------------------------------

主要是测试代码。

```
static const char program[] =
"\
10 gosub 100\n\
20 for i = 1 to 10\n\
30 print i\n\
40 next i\n\
50 print \"end\"\n\
60 end\n\
100 print \"subroutine\"\n\
110 return\n\
";

int main(void)
{
    vbasic_init(program);

    do { vbasic_run(); } while(!vbasic_finished());

    return 0;
}

```

### 六、随便说点
---------------------------------

程序基本都是来自大神dunkels的ubasic，修改了一点点东西。主要是分析解释器的写法（非正常实现），正常实现基本都是走AST，然后虚拟机。上面的实现基本将词法分析（分词token）跟其他部分分开，语法分析（检验）和语义分析实现合并在一起。语法校验成功后直接执行语义解释，失败则直接exit。相比较于工业级别的解释器（如python等），有很多不完善的地方，比如不支持完整的函数和类，但是根据dunkels的说明是作为嵌入式领域（硬件资源有限）的设备使用的解释器（或语言），能实现基本的功能，比汇编写起来还是好很多的。

基于这篇你可以对ubasic进行改进和添加新特性，也可以思考更加一般通用的东西，还可以进一步思考虚拟机、模拟器、语言转换、汇编代码生成的问题。

写到这我想到了图灵机，程序文本跟存储单元成了图灵机纸带上的内容，有一个指针（头尾两个指针）在纸带上移动，expr的实现是运算器，语句的实现是控制器。

写这篇花了不少时间，希望对你有用，希望以后你看到的不仅是语法还有原理结构。

>END

[代码下载](Tools/BasicLang/basic_lang.zip)

