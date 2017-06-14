
# Line Editor 

> *作者：Arvin 日期：2017年6月14日*

----------------------------------------

>BEGIN

In computing, a line editor is a text editor computer program in which each editing command applies to one or more complete lines of text designated by the user. Line editors precede screen-based text editors and originated in an era when a computer operator typically interacted with a teleprinter (essentially a printer with a keyboard), with no video display, and no ability to navigate a cursor interactively in a document. Line editors were also a feature of many home computers, avoiding the need for a more memory-intensive full-screen editor. `ed` is a line editor for the Unix operating system. It was one of the first parts of the Unix operating system that were developed, in August 1969. It remains part of the POSIX and Open Group standards for Unix-based operating systems, alongside more sophisticated full-screen editors such as `vi`.

The line editor consists of some elements, such as the memory structure, the edit commands, the cursor move up and down commands, the load and save commands, and other info commands.

    buffer: the memory buffer of text lines 
    cursor: the current line
    edit: replace the current line with a new text line
    delete: delete the current line
    insert: insert a new line after the current line
    append: append a new line after the last line
    show: show the current line text
    all: show all text lines
    up[num]: navigate up [num] lines
    down[num]: navigate down [num] lines
    top: move to very top
    bottom: move to very bottom
    save: write the text buffer to a file
    load: load a file content to the buffer
    status: output file status statistics
    help: output help info
    execute: execute the buffer text as a program
    quit: quit the editor

一般基于终端的编辑器都会有多种模式，比如：编辑模式、命令模式、浏览模式等，在这里我们不考虑模式问题，全部使用命令+内容的形式操作。

-------------------------------------------

行编辑器是基于行来操作的，对行的操作都是原子操作。如何表示一行呢？当然是字符串了。有了行如何表示多行文本呢？简单考虑可以使用足够大的字符串数组。编辑操作针对具体的行，所以当前行需要一个标志变量。数据结构是操作的起点，是操作对象的内存表示。

    struct {
        char* lines[10000];
        long ln;
    } txt;

上面`lines`表示文本内容，存储字符串指针；`ln`表示当前行在`lines`中的下标，文本最多10000行。后面可以是使用双向链表表示文本。
如何将命令跟数据操作结合起来？命令必须有对应的函数，该函数（方法）操作lines中的数据。我们可以保存函数的指针。

    typedef void (*Callback)(char*);
    
    struct CMD {
       char* cmd_name;
       Callback cmd_fn;
    };

    struct {
        char* lines[10000];
        long ln;

        CMD fn[16];//15条命令
        
        char changed;//文本是否改变
        char running;//是否正在运行
        char* filename;//文件名
    } txt;

fn是函数查找表，可以通过命令字符串查找对应的命令函数。同时，需要知道文本的修改状态、是否需要继续运行和文件名。

有上面的结构，我们可以开始实现基本的命令函数了。

">"命令：

    void vm_forward(){
        vm.dp++;//没有考虑数组下标越界的情况，可以使用%运算限制
    }

"<"命令：

    void vm_backward(){
        vm.dp--;
    }

"+"命令：

    void vm_increment(){
        vm.ds[vm.dp]++;
    }

"-"命令：

    void vm_decrement(){
        vm.ds[vm.dp]--;
    }

"."命令：

    void vm_input(){
        vm.ds[vm.dp] = getchar();
    }

","命令：

    void vm_output(){
        putchar(vm.ds[vm.dp]);
    }

上面的命令基本都是操作数据或者当前数据位置的，没什么逻辑结构非常基本。剩下的“[”和“]”涉及到当前程序指针的跳转，也就是代码段的重复执行，即循环结构，并且循环本身涉及到条件跳转。那么条件本身存放在哪个地方呢？按照BF的尿性，当然是“the pointer”了，也就是当前数据指针所指向的位置。判定跳转的逻辑是什么呢？如果“the pointer”位置数据为0，则离开循环继续执行“]”后面的命令，否则回溯到"["命令执行。因为需要回溯到“[”命令，所以在遇到"["命令的时候，需要记录该命令的位置。还需要考虑一个情况，那就是“[-[+]]”，即循环的嵌套结构。嵌套结构，基本可以使用栈结构来表示。也就是遇到“[”我们会将该命令的位置保存（记录）在栈中的一个元素，遇到"]"会将栈顶元素出栈，即回溯位置出栈。同时要兼顾进出栈的判定条件，即跳转条件。这里我们需要一个栈结构，足够大的数组+当前栈顶位置，就可以简单的构成一个栈结构。

    typedef void (*Callback)(void);
    
    struct {
        char cs[30000];
        long ip;
        
        char ds[30000];
        long dp;

        long ss[1000];//存放cs数组下标，所以是long类型
        long sp;

        Callback fn[128];
    } vm;


“]”命令：

    void vm_while_exit(){
        vm.sp--;//栈顶元素出栈
        if (vm.ds[vm.bp]) {//如果不为0，则回溯到"["位置的前一个位置，run函数会自动对ip加1，也就是"["的位置
            vm.ip = vm.ss[vm.sp];
        }
    }

"["命令：

    void vm_while_enter(){
        if (vm.ds[vm.dp]){
            vm.ss[vm.sp] = vm.ip - 1;//"["的前一个命令位置入栈，因为run函数自动对ip增加1
            vm.sp++;
        }else{//跳过所有的[...]
            int c = 1;
            for (vm.ip++; vm.cs[vm.ip] && c; vm.ip++) {
                if (vm.cs[vm.ip] == '[') {
                    c++;
                } else if (vm.cs[vm.ip] == ']') {
                    c--;
                }
            }
        }
    }

如果一开始“the pointer”位置的值就是0，就不需要进入到循环中，直接跳过"[...]"。这点就是条件跳转，类似高级语言的"if"结构。

命令函数就这么多，还需要一些辅助函数，比如：命令函数查询表（fn）的构建、BF程序文件读入内存（cs）函数、数据的初始化以及取指和执指循环函数。

    void setup() {
        int c;
        int i;
    
        memset(&vm, 0, sizeof(vm));
        vm.fn['>'] = vm_forward;//单个字符ascii值不超过128，字符作为数组下标
        vm.fn['<'] = vm_backward;
        vm.fn['+'] = vm_increment;
        vm.fn['-'] = vm_decrement;
        vm.fn['.'] = vm_output;
        vm.fn[','] = vm_input;
        vm.fn['['] = vm_while_entry;
        vm.fn[']'] = vm_while_exit;
    
        for (i = 0; (c = getchar()) != EOF; ) {
            if (strchr("<>.,+-[]", c)) {//判定有效命令
                vm.cs[i] = c;
                i++;
            }
        }
    }

    void run() {
        while (vm.cs[vm.ip]) {
            vm.fn[vm.cs[vm.ip]]();
            vm.ip++;
        }
    }

最后是解释器的入口函数：main函数

    int main(int argc, char* argv[]) {
        if (argc > 1) {  
            freopen(argv[1], "r", stdin);//将打开的文件重定向到标准输入(stdin)，程序后面直接从stdin获取数据
        }  
    
        setup();  
        run();  
    
        return 0;  
    }

OK，整个解释器的C代码就这些，保存为“bf.c”文件，命令行执行“gcc bf.c -o bf”命令编译成可执行程序"bf"。执行“./bf hello.bf”就会得到"hello.bf"程序的结果了。

下面是“hello.bf”程序：

    ++++++++[>++++++++<-]>+.>++++++++++[>+++++++++++<-]>++++.++++.-------------.+++++.>++++++++++.


--------------------------------

### 结语

1. 编写解释器没有想象中的难
2. ascii字符作为数组下标，可以直接将数组大小设置成128即可
3. 利用数组可以简化条件的判断，可以直接通过下标映射到函数
4. 利用求余运算可以限制数组越界，不需要条件判断
5. 利用数组+位置变量的方式可以避免指针的使用
6. 嵌套结构可以使用栈来表示，栈可以使用数组+栈顶位置的组合构成
7. 循环结构和条件结构可以使用跳转来实现（条件跳转和强制跳转），循环结构包含了条件跳转
8. 数据和当前数据的指针可以同时修改，程序的当前指针可以修改，但是一般程序本身数据指令是不能修改的。（ps：特殊方法还是可以修改程序本身的，毕竟程序也是数据）
9. 使用freopen函数可以简化文件的读取
10. 使用strchr函数可以判定字符串是否包含某个字符

 
*enjoy programming yourself!*


>END
