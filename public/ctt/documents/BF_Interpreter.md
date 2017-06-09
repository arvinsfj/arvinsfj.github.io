
# Brainfuck Language Interpreter

> *作者：Arvin 日期：2017年6月9日*

----------------------------------------

>BEGIN

A Brainfuck program has an implicit byte pointer, called "the pointer", which is free to move around within an array of 30000 bytes, initially all set to zero. The pointer itself is initialized to point to the beginning of this array.

The Brainfuck programming language consists of eight commands, each of which is represented as a single character.

    >  Increment the pointer.
    <  Decrement the pointer.
    +  Increment the byte at the pointer.
    -  Decrement the byte at the pointer.
    .  Output the byte at the pointer.
    ,  Input a byte and store it in the byte at the pointer.
    [  Jump forward past the matching ] if the byte at the pointer is zero.
    ]  Jump backward to the matching [ unless the byte at the pointer is zero.

-------------------------------------------

根据上面的信息如何写一个BF的解释器呢？解释器是解释执行程序的，那么BF的程序是怎么样子的呢？如："+++[-]."，是由上面8个命令组成的。这些程序代码字符串，当然需要存储在内存中，以便解释器获取单个命令并执行对应操作。以BF程序的样式，我们可以用一个字符数组保存整段程序，同时需要一个当前位置标志记住当前命令。根据传统，我们用cs作为数组名称，保存char类型命令，大小30000字节。当前位置使用ip作为名称。

    struct {
        char cs[30000];
        long ip;
    } vm;

程序是有地方保存了，但是程序的产生的中间数据和最终结果保存在哪呢？其实就是上面提到的“the pointer”指向的数组。BF程序是操作这个数组中的数据，比如：读写数据、移动指针和自加和自减数据。我们需要一个大小是30000的数组，保存类型暂定为char类型，名称ds。一样的道理需要一个当前位置的标志，名称dp。

    struct {
        char cs[30000];
        long ip;
        
        char ds[30000];//可以不为char类型
        long dp;
    } vm;

如何将命令跟数据操作结合起来？命令必须有对应的函数，该函数（方法）操作ds中的数据。我们可以保存函数的指针。

    typedef void (*Callback)(void);
    
    struct {
        char cs[30000];
        long ip;
        
        char ds[30000];
        long dp;

        Callback fn[128];//128,因为单个字符的ascii值不会超过128
    } vm;

fn是函数查找表，可以通过8种命令（BF中的命令都是单个字符，ascii总共128个字符，命令作为下标最大也是128）查找对应的命令函数。

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
        vm.sp--;
        vm.ip = vm.ss[vm.sp];
    }

"["命令：

    void vm_while_enter(){
        if (vm.ds[vm.dp]){
            vm.ss[vm.sp] = vm.ip - 1;//因为在run函数中ip自动加1的缘故，当前ip已经指向"["后面一条命令了，所以要减1
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
            freopen(argv[1], "r", stdin);  
        }  
    
        setup();  
        run();  
    
        return 0;  
    }

OK，整个解释器的C代码就这些，保存为“bf.c”文件，命令行执行“gcc bf.c -o bf”命令编译成可执行程序"bf"。执行“./bf hello.bf”就会得到"hello.bf"程序的结果了。

下面是“hello.bf”程序：

    ++++++++[>++++++++<-]>+.>++++++++++[>+++++++++++<-]>++++.++++.-------------.+++++.>++++++++++.

>END
