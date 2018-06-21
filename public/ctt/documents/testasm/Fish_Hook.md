
# 桩、帮助桩、动态符号表、符号表、字符串表和加载命令（macho结构）

> *作者：Arvin 日期：2018年06月21日*

---------------------------------

>BEGIN

Facebook的fishhook库，300行不到的C代码实现了C库函数的动态Hook功能，比较经典，本篇时是阅读fishhook后的一些想法。

### 一、思路
---------------------------------

hook直接修改桩向量的指向即可。动态加载库函数（通过桩的实现）给了C这种静态语言一些动态性质（程序的动态性原理也是一样的）。符号表提供了函数名称到函数入口地址的映射（可以理解成数学里面的一一映射）。符号表的“函数名称”只是字符串表的索引（index），真正的函数名称字符串是保存在字符串表里面的。加载命令可以定位程序各种段节的具体位置（逻辑地址）。

### 二、具体过程
---------------------------------

先看一段C代码：

```
#include <stdio.h>

int main(int argc, const char * argv[]) 
{
    printf("%s\n", "hello world");
    printf("%s\n", "hello desgard");
    return 0;
}
```

使用：gcc hello.c -o hello -g 生成可调式的可执行文件hello。(macho格式)

lldb调试：

![lldb调试断点设置](http://arvinsfj.github.io/public/ctt/documents/testasm/brset.png)

使用：otool -vt main 生成反汇编。

```
hello:
(__TEXT,__text) section
_main:
0000000100000f20  pushq %rbp
0000000100000f21  movq  %rsp, %rbp
0000000100000f24  subq  $0x20, %rsp
0000000100000f28  leaq  0x67(%rip), %rax
0000000100000f2f  leaq  0x64(%rip), %rcx
0000000100000f36  movl  $0x0, -0x4(%rbp)
0000000100000f3d  movl  %edi, -0x8(%rbp)
0000000100000f40  movq  %rsi, -0x10(%rbp)
0000000100000f44  movq  %rax, %rdi
0000000100000f47  movq  %rcx, %rsi
0000000100000f4a  movb  $0x0, %al
0000000100000f4c  callq 0x100000f76
0000000100000f51  leaq  0x3e(%rip), %rdi
0000000100000f58  leaq  0x47(%rip), %rsi
0000000100000f5f  movl  %eax, -0x14(%rbp)
0000000100000f62  movb  $0x0, %al
0000000100000f64  callq 0x100000f76
0000000100000f69  xorl  %edx, %edx
0000000100000f6b  movl  %eax, -0x18(%rbp)
0000000100000f6e  movl  %edx, %eax
0000000100000f70  addq  $0x20, %rsp
0000000100000f74  popq  %rbp
0000000100000f75  retq
```

注意上面的```callq 0x100000f76```对应于C代码的printf库函数调用。地址0x100000f76指向的是什么地方？答案是库函数printf的stubs，即printf函数的桩。桩本质是一条```jmp [(懒加载向量表)表项地址]```指令。实际跳转（jmp）的地址由懒加载向量表表项值（value）决定。换句话来讲，跳转的地址是可以更改的，也就是具有动态性，运行的时候决定跳转的实际地址。懒加载向量表在什么地方呢？通过软件MachOView我们可以看到。Section64(DATA,la_symbol_ptr)节显示如下：

```
100001010 0000000100000F8C
```

![MachOView查看信息](http://arvinsfj.github.io/public/ctt/documents/testasm/lsp.png)

懒加载向量表具体的位置计算可以通过加载命令DATA段的la_symbol_ptr节找到。其实，上面讲的表项地址就是0x100001010，因为我们只用到一个printf库函数。桩的指令实际是：```jmp [0x100001010]```，即是：```jmp 0x100000F8C```。通过lldb的反汇编命令：di -s 0x10000f76，可以清晰的看到一条命令

```
jmpq   *0x94(%rip) ; (void *)0x0000000100000f8c
```

证实了我们的想法。那么地址0x100000F8C又指向什么位置呢？通过lldb反汇编可以看到：

```
0x100000f7c:      leaq   0x85(%rip), %r11          ; (void *)0x0000000000000000
0x100000f83:      pushq  %r11
0x100000f85:      jmpq   *0x75(%rip)               ; (void *)0x00007fff7c034178: dyld_stub_binder
0x100000f8b:      nop    
0x100000f8c:      pushq  $0x0
0x100000f91:      jmp    0x100000f7c
```

![lldb查看stub](http://arvinsfj.github.io/public/ctt/documents/testasm/stubsjmp.png)

我们通过MachOView可以查到上面的代码是stub_helper代码段。那么到这桩第一次运行的思路基本清晰。桩通过查找懒加载向量表，得到跳转地址，第一次是跳转到stub_helper代码段进行动态库的加载绑定和库函数的执行。第二次呢？第二次桩通过查找懒加载向量表得到就不是stub_helper代码段的地址，而是具体的函数地址（库函数调用），在这就是printf的实际地址，不需要进行动态库的加载和绑定过程。也就是说，经过第一次的库函数调用懒加载向量表表项地址为0x100001010的表项值会被修改。通过lldb的```x 0x100001010```命令可以查看位置0x100001010的值。

```
0x100001010: 8c 0f 00 00 01 00 00 00 00 00 00 00 00 00 00 00

修改为

0x100001010: 10 57 0c 7c ff 7f 00 00 00 00 00 00 00 00 00 00
```

![lldb查看懒加载向量表](http://arvinsfj.github.io/public/ctt/documents/testasm/lasymptr.png)

这也证实了我们的想法。我们通过动态修改懒加载向量表（la_symbol_ptr节）就可以实现跳转不同的函数。这也是hook的基本原理。

关于hook还有一个疑问，我们如何找到需要hook的函数呢？最直接的方式是通过函数名称，通过符号表可以对应函数名称的函数地址。macho结构的动态符号表其实跟符号表没什么区别，记录的动态库函数在是符号表中的索引（index）。而动态库函数在在动态符号表中的索引记录在加载命令DATA段的la_symbol_ptr节中（reserved1字段）。

到这hook中的函数名称匹配和修改懒加载向量表都可以实现了。也就是fishhook的实现原理。hook演示代码如下：

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct section_64 section_t;
typedef struct nlist_64 nlist_t;
#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT_64

#ifndef SEG_DATA_CONST
#define SEG_DATA_CONST  "__DATA_CONST"
#endif

//被hook的函数为库函数：strlen，下面是hook函数：new_strlen
static int (*original_strlen)(const char *_s);
int new_strlen(const char *_s)
{
    printf("%s\n", _s);
    return original_strlen(_s);
}

//hook实现函数
static void perform_rebinding_with_section(section_t *section, intptr_t slide, nlist_t *symtab, char *strtab, uint32_t *indirect_symtab)
{
    uint32_t *indirect_symbol_indices = indirect_symtab + section->reserved1;
    void **indirect_symbol_bindings = (void **)((uintptr_t)slide + section->addr);
    for (uint i = 0; i < section->size / sizeof(void *); i++) {
        uint32_t symtab_index = indirect_symbol_indices[i];
        if (symtab_index == INDIRECT_SYMBOL_ABS || symtab_index == INDIRECT_SYMBOL_LOCAL ||
            symtab_index == (INDIRECT_SYMBOL_LOCAL   | INDIRECT_SYMBOL_ABS)) {
            continue;
        }
        uint32_t strtab_offset = symtab[symtab_index].n_un.n_strx;
        char *symbol_name = strtab + strtab_offset;
        if (strnlen(symbol_name, 2) < 2) {
            continue;
        }
        if (strcmp(&symbol_name[1], "strlen") == 0) {
            //交换新旧函数
            *(&original_strlen) = indirect_symbol_bindings[i];
            indirect_symbol_bindings[i] = new_strlen;
            goto symbol_loop;
        }
    symbol_loop:;
    }
}

static void rebind_symbols_for_image(const struct mach_header *header, intptr_t slide)
{
    Dl_info info;
    if (dladdr(header, &info) == 0) {
        return;
    }
    segment_command_t *cur_seg_cmd;
    segment_command_t *linkedit_segment = NULL;
    struct symtab_command* symtab_cmd = NULL;
    struct dysymtab_command* dysymtab_cmd = NULL;
    
    uintptr_t cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_LINKEDIT) == 0) {
                linkedit_segment = cur_seg_cmd;
            }
        } else if (cur_seg_cmd->cmd == LC_SYMTAB) {
            symtab_cmd = (struct symtab_command*)cur_seg_cmd;
        } else if (cur_seg_cmd->cmd == LC_DYSYMTAB) {
            dysymtab_cmd = (struct dysymtab_command*)cur_seg_cmd;
        }
    }
    
    if (!symtab_cmd || !dysymtab_cmd || !linkedit_segment ||
        !dysymtab_cmd->nindirectsyms) {
        return;
    }
    
    // Find base symbol/string table addresses
    uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
    nlist_t *symtab = (nlist_t *)(linkedit_base + symtab_cmd->symoff);
    char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);
    
    // Get indirect symbol table (array of uint32_t indices into symbol table)
    uint32_t *indirect_symtab = (uint32_t *)(linkedit_base + dysymtab_cmd->indirectsymoff);
    
    cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_DATA) != 0 &&
                strcmp(cur_seg_cmd->segname, SEG_DATA_CONST) != 0) {
                continue;
            }
            for (uint j = 0; j < cur_seg_cmd->nsects; j++) {
                section_t *sect =
                (section_t *)(cur + sizeof(segment_command_t)) + j;
                if ((sect->flags & SECTION_TYPE) == S_LAZY_SYMBOL_POINTERS) {
                    perform_rebinding_with_section(sect, slide, symtab, strtab, indirect_symtab);
                }
                if ((sect->flags & SECTION_TYPE) == S_NON_LAZY_SYMBOL_POINTERS) {
                    perform_rebinding_with_section(sect, slide, symtab, strtab, indirect_symtab);
                }
            }
        }
    }
}
//以上是hook实现

int main(int argc, const char * argv[]) 
{
    _dyld_register_func_for_add_image(rebind_symbols_for_image);
    char *str = "hello, hook!";
    printf("%lu\n", strlen(str));
    return 0;
}

```

![Hook演示效果](http://arvinsfj.github.io/public/ctt/documents/testasm/hookxg.png)

当然也可以在lldb中修改懒加载向量表，实现hook。


### 三、随便说点
---------------------------------

1. 一些工具：MachOView、nm、otool、wireshark、lldb、gdb、Hopper、ida64、synalze it等吧
2. 各种表的基址计算方法，请自行参考代码
3. 上面的代码包括fishhook只做了C的修饰符判断（编译出来的函数名称比原有函数名称前面增加了下划线_），只能实现C的Hook
4. OC的Hook可以使用method_exchangeImplementations等函数
5. hook古老的技术，原理并不难，很多地方还是用得到的

>END


