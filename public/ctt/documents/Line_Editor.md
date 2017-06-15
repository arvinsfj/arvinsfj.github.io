
# Line Editor 

> *作者：Arvin 日期：2017年6月15日*

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

        CMD fn[16];//16条命令
        
        char changed;//文本是否改变，暂未用到
        char running;//是否正在运行
        char* filename;//文件名
    } txt;

fn是函数查找表，可以通过命令字符串查找对应的命令函数。同时，需要知道文本的修改状态、是否需要继续运行和文件名。

有上面的结构，我们可以开始实现基本的命令函数了。

"load"命令：

    void load(char* param)
    {  
        char* filename = nextToken(param);
        if (filename&&strlen(filename)) {
            memset(txt.lines, 0, 10000);
            txt.filename = filename;
            FILE *fp = fopen(filename, "r");
            if(fp){
                char line[1000] = {0};
                long ln = 0;
                while(!feof(fp)&&ln<10000){
                    memset(line, 0, 1000);
                    fgets(line, 1000, fp);//fgets获取的字符串除最后包一行外含文件的'\n'字符
                    long llen = strlen(line);
                    if (line[llen-1] != '\n') {//文件的最后一行不包含'\n'，其他行包含'\n'
                        llen+=1;
                    }
                    txt.lines[ln] = malloc(llen);
                    memset(txt.lines[ln], 0, llen);
                    memcpy(txt.lines[ln], line, llen-1);//去除'\n',但确保是完整的字符串,包含'\0'结尾
                    
                    txt.ln = ln;
                    ln++;
                }
                txt.lmax = txt.ln;
                printf("%ld lines read successfully from %s\n", ln, filename);
                
            }else{
                printf("failed to open file: %s\n", filename);
            }
            fclose(fp);
        }else{
            printf("error: must specify filename\n");
        }
    }

"save"命令：

    void save(char* param)
    {
        char* filename = nextToken(param);
        if (filename&&strlen(filename)) {
            txt.filename = filename;
            FILE *fp = fopen(filename, "w");
            if(fp){
                long ln = 0;
                for (ln = 0; ln <= txt.lmax; ln++) {
                    fputs(txt.lines[ln], fp);
                    if (ln != txt.lmax) {//文件最后一行不需要添加'\n'
                        fputc('\n', fp);
                    }
                }
                printf("%ld lines write to file %s successfully\n", ln, filename);
                
            }else{
                printf("failed to open file: %s\n", filename);
            }
            fclose(fp);
        }else{
            printf("error: must specify filename\n");
        }
    }

"quit"命令：

    void quit(char* param)
    {
        txt.running = 0;
    }

"help"命令：

    void help(char* param)
    {
        printf("help info!\n[cmd] [param]\n");
    }

"execute"命令：

    void execute(char* param)
    {//考虑可以跟语言解释器结合起来
        printf("not implement!\n");
    }

"status"命令：

    void status(char* param)
    {
        printf("filename: %s\n", !txt.filename?"<undefined>":txt.filename);//直接创建文本，没有文件名
        printf("current line: %ld\n", txt.ln<=0?0:txt.ln);//txt.ln和txt.lmax最小值都是-1，即没有一行文本
        printf("total lines: %ld\n", txt.lmax+1);//同上解释
    }

"top"命令：

    void top(char* param)
    {
        if (txt.ln>=0) {//ln最小值-1，代表没有一行文本，此时top命令无效
            txt.ln = 0;
        }
    }

"bottom"命令：

    void bottom(char* param)
    {
        txt.ln = txt.lmax;//有可能是-1
    }

"up"命令：

    void up(char* param)
    {
        char* num = nextToken(param);
        long ln = atoi(num);
        if (txt.ln-ln<=-1) {
            txt.ln = -1;//-1代表没有一行文本
            return;
        }else{
            txt.ln = txt.ln-ln;
        }
    }

"down"命令：

    void down(char* param)
    {
        char* num = nextToken(param);
        long ln = atoi(num);
        if (txt.ln+ln>=txt.lmax) {
            txt.ln = txt.lmax;
            return;
        }else{
            txt.ln = txt.ln+ln;
        }
    }

"show"命令：

    void show(char* param)
    {
        if (txt.ln>=0&&txt.ln<=txt.lmax) {//同上
            printf("%04u %s\n", (unsigned int)txt.ln, txt.lines[txt.ln]);
        }
    }

"all"命令：

    void all(char* param)
    {
        for (long i = 0; i <= txt.lmax; i++) {//lmax有可能等于-1，内存中的每行文本不包含'\n'
            printf("%04u %s\n", (unsigned int)i, txt.lines[i]);
        }
    }

"append"命令：

    void append(char* param)
    {//文本最后一行新建一行文本
        char* line = nextToken(param);
        long llen = strlen(line)+1;//+1是为了把文本结束符'\0'也算在内
        if (llen>1&&txt.lmax<9999) {
            bottom(param);
            txt.ln++;
            txt.lines[txt.ln] = malloc(llen);
            memcpy(txt.lines[txt.ln], line, llen);
            txt.lmax++;
        }else{
            printf("not enough memory or have not line text!\n");
        }
    }

"insert"命令：

    void insert(char* param)
    {//在当前行后面插入一行新文本
        char* line = nextToken(param);
        long llen = strlen(line)+1;
        if (llen>1&&txt.lmax<9999) {
            for (long i = txt.lmax; i >= txt.ln+1; i--) {//数组后移动,空出位置
                txt.lines[i+1] = txt.lines[i];
            }
            txt.ln++;
            txt.lines[txt.ln] = malloc(llen);
            memcpy(txt.lines[txt.ln], line, llen);
            txt.lmax++;
        }else{
            printf("not enough memory or have not line text!\n");
        }
    }

"delete"命令：

    void delete(char* param)
    {
        if (txt.ln>=0) {//ln==-1,代表没有一行文本
            free(txt.lines[txt.ln]);
            for (long i = txt.ln; i <= txt.lmax; i++) {//数组前移,删除当前行文本
                txt.lines[i] = txt.lines[i+1];
            }
            txt.lines[txt.lmax] = 0;
            txt.lmax--;
            if (txt.ln>=txt.lmax) {//当前行下标不能超过最大行下标
                txt.ln = txt.lmax;
            }
        }else{
            printf("have no line\n");
        }
    }

"edit"命令：

    void edit(char* param)
    {
        char* line = nextToken(param);
        long llen = strlen(line)+1;//+1位了'\0'计算在内
        if (llen>1) {
            free(txt.lines[txt.ln]);
            txt.lines[txt.ln] = malloc(llen);
            memcpy(txt.lines[txt.ln], line, llen);
        }else{
            printf("have not line text!\n");
        }
    }

命令函数就这么多，还需要一些辅助函数，比如：命令函数查询表（fn）的构建、命令参数读取函数、数据的初始化以及命令匹配函数。

    //去掉左边的空格
    void strltrim(char *pStr)
    {
        char *pTmp = pStr;
        while (*pTmp == ' '){
            pTmp++;
        }
        while(*pTmp != '\0'){
            *pStr = *pTmp;
            pStr++;
            pTmp++;
        }
        *pStr = '\0';
    }

    //去掉右边的空格
    void strrtrim(char *pStr)
    {
        char *pTmp = pStr+strlen(pStr)-1;
        while (*pTmp == ' '){
            *pTmp = '\0';
            pTmp--;
        }
    }

    //获取命令参数字符串函数
    char* nextToken(char* line)
    {
        char* pp = strchr(line, ' ');
        if (pp) {
            long plen = strlen(pp)+1;
            char* tmpParam = malloc(plen);
            memcpy(tmpParam, pp, plen);
            strltrim(tmpParam);
            strrtrim(tmpParam);
            if (strlen(tmpParam)) {//排除参数全部是空格的情况
                return tmpParam;
            }
        }
        
        return NULL;
    }

    //设置和初始化函数
    void setup()
    {
        memset(txt.lines, 0, 10000);
        txt.ln = -1;
        txt.lmax = -1;
        txt.changed = 0;
        txt.running = 1;
        txt.filename = 0;
    
        //下面的代码是构建命令函数映射表，难看
        struct CMD cmd_load = {"load", load};
        struct CMD cmd_save = {"save", save};
        struct CMD cmd_quit = {"quit", quit};
        struct CMD cmd_help = {"help", help};
        struct CMD cmd_execute = {"execute", execute};
        struct CMD cmd_status = {"status", status};
        struct CMD cmd_top = {"top", top};
        struct CMD cmd_bottom = {"bottom", bottom};
        struct CMD cmd_up = {"up", up};
        struct CMD cmd_down = {"down", down};
        struct CMD cmd_show = {"show", show};
        struct CMD cmd_all = {"all", all};
        struct CMD cmd_append = {"append", append};
        struct CMD cmd_insert = {"insert", insert};
        struct CMD cmd_delete = {"delete", delete};
        struct CMD cmd_edit = {"edit", edit};
        txt.fn[0] = cmd_load;
        txt.fn[1] = cmd_save;
        txt.fn[2] = cmd_quit;
        txt.fn[3] = cmd_help;
        txt.fn[4] = cmd_execute;
        txt.fn[5] = cmd_status;
        txt.fn[6] = cmd_top;
        txt.fn[7] = cmd_bottom;
        txt.fn[8] = cmd_up;
        txt.fn[9] = cmd_down;
        txt.fn[10] = cmd_show;
        txt.fn[11] = cmd_all;
        txt.fn[12] = cmd_append;
        txt.fn[13] = cmd_insert;
        txt.fn[14] = cmd_delete;
        txt.fn[15] = cmd_edit;
    }

    //命令和操作匹配函数，由于命令使用了字符串（而非单个字符），所以需要CMD结构体来封装命令
    void handleInput(char* line)
    {
        line[strlen(line)-1] = '\0';//除掉fgets函数获得的字符串结尾处的'\n'字符
        //printf("%s\n", line);
        int lcmd = 0;
        char* endp = strchr(line, ' ');
        if (endp) {
            lcmd = endp - line + 1;//指针（地址）的差值即是字符串的长度
        }else{//有可能命令后面没有参数
            lcmd = strlen(line) + 1;
        }
        
        char* cmd = malloc(lcmd);
        memset(cmd, 0, lcmd);
        memcpy(cmd, line, lcmd-1);
        //printf("%s\n", cmd);
        char isvalid = 0;
        for (int i = 0; i < 16; i++) {//命令与命令函数匹配，循环搜索效率肯定没有数组单字符下标映射高
            struct CMD scmd = txt.fn[i];
            if (!strncmp(scmd.cmd_name, cmd, lcmd)) {
                scmd.cmd_fn(line);
                isvalid = 1;
                break;
            }
        }
        if (!isvalid) {
            printf("invalid command: %s\n", cmd);
        }
    }



最后是编辑器的入口函数：main函数

    int main(int argc, char** argv)
    {
        setup();
        
        printf("editor version: %s\n", "0.1");
        char line[1000] = {0};
        do {
            printf(">> ");
            memset(line, 0, 1000);
            fgets(line, 1000, stdin);//char '\n' included at the end of the string
            handleInput(line);
        } while (txt.running);//操作循环,使用命令“quit”结束循环
        
        return 0;
    }

OK，整个行编辑器的C代码就这些，保存为“ledit.c”文件，命令行执行“gcc ledit.c -o ledit”命令编译成可执行程序"ledit"。执行“./ledit”就可以运行ledit程序了，在">> "后执行"load s.bas"命令加载s.bas文件内容，执行"all"命令查看所加载的内容。

下面是“s.bas”程序：

    LET I = 0
    PRINT I * I
    LET I = I + 1
    IF I < 10 THEN GOTO 1
    PRINT I


--------------------------------

### 结语

1. 上面是简单的行编辑器实现，`ed`是真实的行编辑器，可以分析它的源代码，原理都是差不多的
2. 命令字符串跟命令函数的匹配，需要额外的结构体封装和循环搜索，降低了代码耦合性，增加了扩展性
3. C程序设计，都是从内存数据结构开始，然后基于该结构实现特定功能，因此写代码之前要看清目标软件的内存数据结构，继而是操作原理
4. 循环不仅用于批量处理数据，还可以跟输入函数（阻塞函数）一起用于用户交互处理
5. 二维文本表示，这里是使用字符串数组结构表示，更好的方法是字符串双向链表结构，插入和删除操作就不需要移动数据了
6. fgets函数获取一行字符串，其中很大可能是包含换行'\n'字符的，文本的最后一行不包含'\n'，原因是最后一行的结尾是EOF标志
7. stdin、stdout、stderr是三个标准输入和输出，可以认为是内置的文件描述符FD，当然操作也跟文件操作差不多
8. C可以通过memcpy函数、strcpy函数来获取子串，memcpy注意字符串长度问题，要将'\0'一起拷贝，也可以利用一个指针来“创建”（模拟）子串，比如一个指针指向一个字符串的中间，那么该指针指向的位置到字符串结尾就是一个子串
9. 抽象是一个暧昧的概念，我觉得不仅仅用于面向对象的语言，也可以是面向过程的语言，比如C中的函数、结构体、变量、指针等
10. 上面的编辑器只是起点，比如可以简化命令字符串、增强删除命令支持多行删除、插入多行、打印多行、clear命令、复制粘贴命令、绑定语言解释器等
11. 现在只考虑行的操作，如果考虑每行字符串的列操作，就是一个screen-full类似vi的编辑器，如果编辑器基于GUI，就是比较现代的编辑器了，但是编辑器的内存数据结构基本没变
12. 再一个就是考虑编辑器模式问题，比如支持编辑模式和命令模式
13. 逻辑和定义混乱肯定会导致代码混乱，这里混乱指逻辑上的重叠、不全面、无效等


*enjoy programming yourself!*


>END

[DOWNLOAD](documents/ledit.zip)
