
# 如何理解C语言的函数功能

> *作者：Arvin 日期：2019年05月17日*

---------------------------------

>BEGIN

在大规模使用C或者阅读C代码的时候，关键函数一般比较难懂。有没有好的方法阅读和理解某个函数呢？下面将一些我的方法。

### 一、理解C函数功能
---------------------------------

例子如下：

```
// Error reporting

// Finds a line (pointed by a given pointer) from the input file
// to print it out.
static void print_line(char *buf, char *path, char *pos) {
  char *start = buf;
  int line = 0;
  int col = 0;

  for (char *p = buf; p; p++) {
    if (*p == '\n') {
      start = p + 1;
      line++;
      col = 0;
      continue;
    }

    if (p != pos) {
      col++;
      continue;
    }

    fprintf(stderr, "error at %s:%d:%d\n\n", path, line + 1, col + 1);

    // Print out the line containing the error location.
    int linelen = (int)(strchr(p, '\n') - start);
    fprintf(stderr, "%.*s\n", linelen, start);

    // Show tabs for tabs and spaces for other characters
    // so that the column matches.
    for (int i = 0; i < col; i++)
      fprintf(stderr, (start[i] == '\t') ? "\t" : " ");

    fprintf(stderr, "↑\n\n");
    return;
  }
}

```

第一步看注释。一般核心函数前面都有一些函数说明（很多用的是英语），参数、返回值和功能简介。个人最看重的是功能简介，做到心里有个初步影响。比如下面的：

```
// Error reporting

// Finds a line (pointed by a given pointer) from the input file
// to print it out.

翻译如下：

// 错误报告

// 从输入文件中找到一行（被一个指针指着），并打印该行。

```

就字面意思，这个print_line函数作用是打印错误信息，打印一行文本（并带一个指针）。根据经验，这是在命令行中打印信息，输出的全部是字符。

第二步看函数名称。```print_line```，打印行。根据经验的话就是打印一行文本。

第三步看函数参数和局部变量。函数参数跟局部变量相同的地方是都是分配的栈内存，不同的地方是函数参数是在调用函数的时候赋值的（局部变量不能从函数外部赋值）。

这里我们主要分析局部变量的读写属性和作用范围。参数buf、path和pos是只读属性，并没有进行写入。start、line和col基本是写属性。p、linelen和i基本是专用变量，功能单一不需要太多的关注。

只读的变量只要搞清楚保存的是什么数据即可，比如：buf保存的是字符串头指针；path是路径字符串指针；pos是指向buf某个位置的字符指针。

写变量是代码操作的主要变量对象。我们需要重点关注。比如：start、line和col。

第四步看代码块划分。上面的函数基本是for块中包含两个if块和一个顺序块。for块是遍历buf中的单个字符。第一个if块是特殊处理'\n'字符，即行标记处理。第二个if块处理特殊位置pos，即列标记处理。最后的顺序块是输出信息。

第五步是看块的输入和输出（跳转）。


### 五、随便说点
---------------------------------

1. 注意自动增长的C实现方式，可能也是任何高级语言实现可变数组的方式。
2. 这三种数据结构还是比较好用的，实用也很简单。
3. 指针还是C的精髓所在，提供了基本的抽象功能，不过也引进了模糊复杂性。
4. 向下取整怎么做呢？大概如右：```x & ~(align - 1)```

>END


