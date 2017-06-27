
# Tiny Basic Language

> *作者：Arvin 日期：2017年6月27日*

----------------------------------------

>BEGIN

CPU的控制器划分了指令和硬件，同理语言编译器（或者解释器）划分了语言和指令。机器语言和汇编语言，是基于指令的语言，更多的是通过指令来控制机器，指令本身没有逻辑结构，各种指令相互作用和组合才实现了某种逻辑结构（条件和循环）。这一阶段的编程思维更多的是机器执行指令的思维方式。通过先辈的努力，实现了从汇编到现代高级语言的转变，也将编程的思维方式从机器执行指令的方式转变为软件逻辑结构的表达。根本上是在总结和归纳汇编编程的逻辑结构，从语言层面提供逻辑结构命令，而非机器指令，比如if、while、for等逻辑结构的关键字（命令）。通过编译器（解释器）将这些通用的逻辑结构翻译（替代）成多条机器指令的实现。那编译器自身怎么实现的呢？最早的当然是汇编或者机器指令实现的，有了第一个编译器，就可以通过这种语言来实现该语言的编译器（自编译）。我们可以将高级逻辑结构翻译成汇编代码（或机器指令）的工具叫做编译器，将逻辑结构翻译成其他语言命令的工具叫做解释器。一般语言的运行除了命令还需要运行环境，这构成了某种语言的虚拟机。另外，语言的标准库（比如C语言）本质是对系统（操作系统）的系统调用的封装（总结归纳）。系统调用代表着操作系统提供给上层软件的全部功能（机器功能的子集）。系统调用原始使用形式是汇编形式（ABI），标准库提供给上层的使用形式是函数（API）,标准库中系统调用的实现也是汇编形式。操作系统除了提供系统调用外，还具有机器功能（资源）的管理职能。通过系统调用可能得到也可能得不到机器的资源。现代软件系统的运行都必须经过操作系统。软件不经过操作系统裸机也是可以运行的，比如操作系统自己。需不需要操作系统对于某些人是值得思考的事情。回到正题，下面是Tiny Basic语言的BNF语法描述。

## Tiny BASIC Grammar

Tiny BASIC is tiny, so it's grammar is also tiny. The grammar, if you were
wondering, is copied directly off of wikipedia. It is listed in Backus-Naur
form(BNF). Hopefully, you are familiar with it. `CR` stands for carriage return,
<kbd>Enter</kbd>.

    line ::= statement CR

    statement ::= PRINT expr-list
                  IF expression relop expression THEN statement
                  GOTO expression
                  INPUT var-list
                  LET var = expression
                  GOSUB expression
                  RETURN
                  CLEAR
                  LIST
                  RUN
                  END

    expr-list ::= (string|expression) (, (string|expression) )*

    var-list ::= var (, var)*

    expression ::= (+|-|ε) term ((+|-) term)*

    term ::= factor ((*|/) factor)*

    factor ::= var | number | (expression)

    var ::= A | B | C ... | Y | Z

    number ::= digit digit*

    digit ::= 0 | 1 | 2 | 3 | ... | 8 | 9

    relop ::= < (>|=|ε) | > (<|=|ε) | =

任何语言程序文本基本都由`关键字`、`运算符`、`数字数值`、`变量`等几种字符串组成。这几种字符串串行组合连接可以构成表达式、语句等复杂字符串，同时也更加符合语言的表达习惯。比如GOTO是关键字，后面连接表达式就可以构成GOTO语句了，而表达式本身由运算符、变量、数字数值构成。Tiny Basic语言，由一行一行的语句构成，而语句由更加低层的元素构成。上面的BNF描述，就是对Tiny Basic语言的这种构成的描述。理解BNF的关键是递归，而递归的关键是重复结构和退出条件。

--------------------------------------------

*enjoy programming yourself!*


>END

