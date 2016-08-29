# Python语法
-----------------------------
在Python中，代码不是越多越好，而是越少越好。代码不是越复杂越好，而是越简单越好。

#### 一、输入和输出
name = raw_input("please enter your name: ")
print "Hello,", name, 200 + 300
`Hello, Arvin 500`

#### 二、约定
注释：`# 文字描述`
缩进：`tab或4个空格`
命名：`字符大小写敏感`
常量：`全部大写`

#### 三、数据类型
数值：整形和实型（1、0、－1、0xff00、1.0、-1.2、-1.23e9）
布尔：True和False
空值：None
字符："..."或'...'或'''...'''（包含转义字符）
组合：列表、字典、自定义结构

变量可以是任意数据类型。同一变量可以反复赋不同类型的数据。Python是动态语言，弱类型语言。Python变量赋值可以认为是指针操作，对变量赋值就是把数据和变量给关联起来。Python不提供定义常量的机制，全部靠约定。


#### 四、字符串
在计算机内存中，统一使用Unicode编码，当需要保存到硬盘或者需要传输的时候，就转换为UTF-8编码。

Python提供了ord()和chr()函数，可以把字母和对应的数字相互转换。以Unicode表示的字符串用u'...'表示。`u'中文'`和`u'\u4e2d'`。字符串'xxx'虽然是ASCII编码，但也可以看成是UTF-8编码，而u'xxx'则只能是Unicode编码。

把u'xxx'转换为UTF-8编码的'xxx'用encode('utf-8')方法。len()函数可以返回字符串的长度。把UTF-8编码表示的字符串'xxx'转换为Unicode字符串u'xxx'用decode('utf-8')方法。

由于Python源代码也是一个文本文件，所以，当你的源代码中包含中文的时候，在保存源代码时，就需要务必指定保存为UTF-8编码。
   #!/usr/bin/env python
    # -*- coding: utf-8 -*-

在Python中，采用的格式化方式和C语言是一致的，用%实现。
    %d	整数
    %f	浮点数
    %s	字符串
    %x	十六进制整数
其中，格式化整数和浮点数还可以指定是否补0和整数与小数的位数。

#### 五、组合类型
* list
* tuple
* dict
* set

组合类型集合里面的元素的数据类型可以不同，并且可以嵌套自身。

list是可变有序列表。
L = []
classmates = ['Michael', 'Bob', 'Tracy']
print classmates[0]
print classmates[-1]
classmates[0] = "abc"
len(classmates)
classmates.append("asdsf")
classmates.pop()
classmates.insert(1, "afsg")
classmates.pop(1)

另一种有序列表叫元组：tuple。tuple和list非常类似，但是tuple一旦初始化就不能修改（指针）。只有1个元素的tuple定义时必须加一个逗号,，来消除歧义。` t = (1,)`

Python内置了字典：dict的支持，dict全称dictionary，在其他语言中也称为map，使用键-值（key-value）存储，具有极快的查找速度。dict内部存放的顺序和key放入的顺序是没有关系的。dict是用空间来换取时间的一种方法。要保证hash的正确性，作为key的对象就不能变。在Python中，字符串、整数等都是不可变的，因此，可以放心地作为key。而list是可变的，就不能作为key。
d = {'Michael': 95, 'Bob': 75, 'Tracy': 85}
print d['Michael']
d['Adam'] = 67
'Thomas' in d
d.get('Thomas')
d.pop('Bob')

set和dict类似，也是一组key的集合，但不存储value。由于key不能重复，所以，在set中，没有重复的key。要创建一个set，需要提供一个list作为输入集合。重复元素在set中自动被过滤。
s = set([1, 2, 3])
s = set([1, 1, 2, 2, 3, 3])
通过add(key)方法可以添加元素到set中，可以重复添加，但不会有效果。
s.add(4)
通过remove(key)方法可以删除元素。
s.remove(4)
set可以看成数学意义上的无序和无重复元素的集合，因此，两个set可以做数学意义上的交集、并集等操作。
s1 = set([1, 2, 3])
s2 = set([2, 3, 4])
s1 & s2
s1 | s2
set和dict的唯一区别仅在于没有存储对应的value，但是，set的原理和dict一样，所以，同样不可以放入可变对象，因为无法判断两个可变对象是否相等，也就无法保证set内部“不会有重复元素”。

对于不变对象来说，调用对象自身的任意方法，也不会改变该对象自身的内容。相反，这些方法会创建新的对象并返回，这样，就保证了不可变对象本身永远是不可变的。


#### 六、控制流
条件
    age = 20
    if age >= 6:
        print 'teenager'
    elif age >= 18:
        print 'adult'
    else:
        print 'kid'

循环
    names = ['Michael', 'Bob', 'Tracy']
    for name in names:
        print name

    sum = 0
    for x in range(101):
        sum = sum + x
    print sum
    
    sum = 0
    n = 99
    while n > 0:
        sum = sum + n
        n = n - 2
    print sum

从raw_input()读取的内容永远以字符串的形式返回，把字符串和整数比较就不会得到期待的结果，必须先用int()把字符串转换为我们想要的整型。还需要进一步的校验类型。

#### 七、函数
基本上所有的高级语言都支持函数，Python也不例外。Python不但能非常灵活地定义函数，而且本身内置了很多有用的函数，可以直接调用。

借助抽象，我们才能不关心底层的具体计算过程，而直接在更高的层次上思考问题。函数就是最基本的一种代码抽象的方式。

[Python标准库](https://docs.python.org/2/library/functions.html)也可以在交互式命令行通过help(abs)查看abs函数的帮助信息。

函数名其实就是指向一个函数对象的引用，完全可以把函数名赋给一个变量，相当于给这个函数起了一个“别名”。

在Python中，定义一个函数要使用def语句，依次写出函数名、括号、括号中的参数和冒号:，然后，在缩进块中编写函数体，函数的返回值用return语句返回。

    def my_abs(x):
        if x >= 0:
            return x
        else:
            return -x

请注意，函数体内部的语句在执行时，一旦执行到return时，函数就执行完毕，并将结果返回。因此，函数内部通过条件判断和循环可以实现非常复杂的逻辑。
如果没有return语句，函数执行完毕后也会返回结果，只是结果为None。
return None可以简写为return。

    def nop():
        pass
        
    if age >= 18:
        pass

缺少了pass，代码运行就会有语法错误。

调用函数时，如果参数个数不对，Python解释器会自动检查出来，并抛出TypeError。但是如果参数类型不对，Python解释器就无法帮我们检查。

    def my_abs(x):
        if not isinstance(x, (int, float)):
            raise TypeError('bad operand type')
        if x >= 0:
            return x
        else:
            return -x

返回多个值（返回值是一个tuple）
    import math
    def move(x, y, step, angle=0):
        nx = x + step * math.cos(angle)
        ny = y - step * math.sin(angle)
        return nx, ny
在语法上，返回一个tuple可以省略括号，而多个变量可以同时接收一个tuple，按位置赋给对应的值，所以，Python的函数返回多值其实就是返回一个tuple。

Python的函数定义非常简单，但灵活度却非常大。除了正常定义的必选参数外，还可以使用默认参数、可变参数和关键字参数，使得函数定义出来的接口，不但能处理复杂的参数，还可以简化调用者的代码。

默认参数（必选参数在前，默认参数在后）
    def power(x, n=2):
        s = 1
        while n > 0:
            n = n - 1
            s = s * x
        return s

可变参数
在Python函数中，还可以定义可变参数。顾名思义，可变参数就是传入的参数个数是可变的，可以是1个、2个到任意个，还可以是0个。

    def calc(*numbers):
        sum = 0
        for n in numbers:
            sum = sum + n * n
        return sum
    
    nums = [1, 2, 3]
    calc(*nums)

关键字参数
可变参数允许你传入0个或任意个参数，这些可变参数在函数调用时自动组装为一个tuple。而关键字参数允许你传入0个或任意个含参数名的参数，这些关键字参数在函数内部自动组装为一个dict。

    def person(name, age, **kw):
        print 'name:', name, 'age:', age, 'other:', kw
    
    person('Michael', 30)
    person('Adam', 45, gender='M', job='Engineer')
    kw = {'city': 'Beijing', 'job': 'Engineer'}
    person('Jack', 24, **kw)

在Python中定义函数，可以用必选参数、默认参数、可变参数和关键字参数，这4种参数都可以一起使用，或者只用其中某些，但是请注意，参数定义的顺序必须是：必选参数、默认参数、可变参数和关键字参数。

    def func(a, b, c=0, *args, **kw):
        print 'a =', a, 'b =', b, 'c =', c, 'args =', args, 'kw =', kw
      
      args = (1, 2, 3, 4)
      kw = {'x': 99}
      func(*args, **kw)

对于任意函数，都可以通过类似func(*args, **kw)的形式调用它，无论它的参数是如何定义的。

在函数内部，可以调用其他函数。如果一个函数在内部调用自身本身，这个函数就是递归函数。递归函数的优点是定义简单，逻辑清晰。理论上，所有的递归函数都可以写成循环的方式，但循环的逻辑不如递归清晰。使用递归函数需要注意防止栈溢出。

    def fact(n):
        if n==1:
            return 1
        return n * fact(n - 1)

使用递归函数需要注意防止栈溢出。在计算机中，函数调用是通过栈（stack）这种数据结构实现的，每当进入一个函数调用，栈就会加一层栈帧，每当函数返回，栈就会减一层栈帧。由于栈的大小不是无限的，所以，递归调用的次数过多，会导致栈溢出。

解决递归调用栈溢出的方法是通过尾递归优化，事实上尾递归和循环的效果是一样的，所以，把循环看成是一种特殊的尾递归函数也是可以的。

**尾递归是指，在函数返回的时候，调用自身本身，并且，return语句不能包含表达式。**
这样，编译器或者解释器就可以把尾递归做优化，使递归本身无论调用多少次，都只占用一个栈帧，不会出现栈溢出的情况。

    def fact(n):
        return fact_iter(n, 1)
    
    def fact_iter(num, product):
        if num == 1:
            return product
        return fact_iter(num - 1, num * product)

大多数编程语言没有针对尾递归做优化，Python解释器也没有做优化，所以，即使把上面的fact(n)函数改成尾递归方式，也会导致栈溢出。

#### 八、Python特性
* 切片
* 迭代
* 列表生成表达式
* 生成器
* 高阶函数
* 返回函数
* 匿名函数
* 装饰器
* 偏函数

对经常取指定索引范围的操作，用循环十分繁琐，因此，Python提供了切片（Slice）操作符，能大大简化这种操作。

L = range(100)
print L[0:3]
print L[:3]
print L[1:3]
print L[-2:]
print L[:10:2]
print L[::5]
print L[:] #复制

L[0:3]表示，从索引0开始取，直到索引3为止，但不包括索引3。即索引0，1，2，正好是3个元素。

倒数第一个元素的索引是-1。
tuple也是一种list，唯一区别是tuple不可变。因此，tuple也可以用切片操作，只是操作的结果仍是tuple。
字符串'xxx'或Unicode字符串u'xxx'也可以看成是一种list，每个元素就是一个字符。因此，字符串也可以用切片操作，只是操作结果仍是字符串。

如果给定一个list或tuple，我们可以通过for循环来遍历这个list或tuple，这种遍历我们称为迭代（Iteration）。
在Python中，迭代是通过forin来完成的，而很多语言比如C或者Java，迭代list是通过下标完成的。Python的for循环抽象程度要高于Java的for循环，因为Python的for循环不仅可以用在list或tuple上，还可以作用在其他可迭代对象上。比如dict就可以迭代。

    d = {'a': 1, 'b': 2, 'c': 3}
    for key in d:
        print key

因为dict的存储不是按照list的方式顺序排列，所以，迭代出的结果顺序很可能不一样。

默认情况下，dict迭代的是key。如果要迭代value，可以用for value in d.itervalues()，如果要同时迭代key和value，可以用for k, v in d.iteritems()。

字符串也是可迭代对象。

如何判断一个对象是可迭代对象呢？方法是通过collections模块的Iterable类型判断。

    from collections import Iterable
    isinstance('abc', Iterable)
    isinstance([1,2,3], Iterable)
    isinstance(123, Iterable)

Python内置的enumerate函数可以把一个list变成索引-元素对，这样就可以在for循环中同时迭代索引和元素本身。

    for i, value in enumerate(['A', 'B', 'C']):
        print i, value


列表生成式即List Comprehensions，是Python内置的非常简单却强大的可以用来创建list的生成式。举个例子，要生成list [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]可以用range(1, 11)。

print [x * x for x in range(1, 11)]
print [x * x for x in range(1, 11) if x % 2 == 0]
print [m + n for m in 'ABC' for n in 'XYZ']

    import os
    [d for d in os.listdir('.')]
    
    d = {'x': 'A', 'y': 'B', 'z': 'C' }
    for k, v in d.iteritems():
        print k, '=', v
    
    [k + '=' + v for k, v in d.iteritems()]
    
    L = ['Hello', 'World', 'IBM', 'Apple']
    [s.lower() for s in L if isinstance(s, str)]

使用内建的isinstance函数可以判断一个变量是不是字符串。


如果列表元素可以按照某种算法推算出来，那我们是否可以在循环的过程中不断推算出后续的元素呢？这样就不必创建完整的list，从而节省大量的空间。在Python中，这种一边循环一边计算的机制，称为生成器（Generator）。

    L = [x * x for x in range(10)]
    print L
    g = (x * x for x in range(10))
    print g
    for n in g:
        print n

generator保存的是算法，每次调用next()，就计算出下一个元素的值，直到计算到最后一个元素，没有更多的元素时，抛出StopIteration的错误。上面这种不断调用next()方法实在是太变态了，正确的方法是使用for循环，因为generator也是可迭代对象。

nerator非常强大。如果推算的算法比较复杂，用类似列表生成式的for循环无法实现的时候，还可以用函数来实现。

    def fib(max):
        n, a, b = 0, 0, 1
        while n < max:
            yield b
            a, b = b, a + b
            n = n + 1
            
    print fib(6)

最难理解的就是generator和函数的执行流程不一样。函数是顺序执行，遇到return语句或者最后一行函数语句就返回。而变成generator的函数，在每次调用next()的时候执行，遇到yield语句返回，再次执行时从上次返回的yield语句处继续执行。


在计算机的层次上，CPU执行的是加减乘除的指令代码，以及各种条件判断和跳转指令，所以，汇编语言是最贴近计算机的语言。
而计算则指数学意义上的计算，越是抽象的计算，离计算机硬件越远。
对应到编程语言，就是越低级的语言，越贴近计算机，抽象程度低，执行效率高，比如C语言；越高级的语言，越贴近计算，抽象程度高，执行效率低，比如Lisp语言。
函数式编程就是一种抽象程度很高的编程范式，纯粹的函数式编程语言编写的函数没有变量，因此，任意一个函数，只要输入是确定的，输出就是确定的，这种纯函数我们称之为没有副作用。而允许使用变量的程序设计语言，由于函数内部的变量状态不确定，同样的输入，可能得到不同的输出，因此，这种函数是有副作用的。
**函数式编程的一个特点就是，允许把函数本身作为参数传入另一个函数，还允许返回一个函数！**
Python对函数式编程提供部分支持。由于Python允许使用变量，因此，Python不是纯函数式编程语言。

变量可以指向函数。函数名也是变量。既然变量可以指向函数，函数的参数能接收变量，那么一个函数就可以接收另一个函数作为参数，这种函数就称之为高阶函数。编写高阶函数，就是让函数的参数能够接收别的函数。

    def add(x, y, f):
        return f(x) + f(y)
    
    print add(-5, 6, abs)

Python内建了map()和reduce()函数。
map()函数接收两个参数，一个是函数，一个是序列，map将传入的函数依次作用到序列的每个元素，并把结果作为新的list返回。

    def f(x):
        return x * x
    
    print map(f, [1, 2, 3, 4, 5, 6, 7, 8, 9])
    print map(str, [1, 2, 3, 4, 5, 6, 7, 8, 9])

map()作为高阶函数，事实上它把运算规则抽象了。

reduce()函数把一个函数作用在一个序列[x1, x2, x3...]上，这个函数必须接收两个参数，reduce把结果继续和序列的下一个元素做累积计算。
    reduce(f, [x1, x2, x3, x4]) = f(f(f(x1, x2), x3), x4)
    
    def add(x, y):
        return x + y
    
    reduce(add, [1, 3, 5, 7, 9])
    
    def fn(x, y):
        return x * 10 + y
    
    reduce(fn, [1, 3, 5, 7, 9])
    
    def char2num(s):
        return {'0': 0, '1': 1, '2': 2, '3': 3, '4': 4, '5': 5, '6': 6, '7': 7, '8': 8, '9': 9}[s]
        
    reduce(fn, map(char2num, '13579'))
    
    def str2int(s):
        def fn(x, y):
            return x * 10 + y
        def char2num(s):
            return {'0': 0, '1': 1, '2': 2, '3': 3, '4': 4, '5': 5, '6': 6, '7': 7, '8': 8, '9': 9}[s]
            
    return reduce(fn, map(char2num, s))
    
    def char2num(s):
        return {'0': 0, '1': 1, '2': 2, '3': 3, '4': 4, '5': 5, '6': 6, '7': 7, '8': 8, '9': 9}[s]
        
    def str2int(s):
        return reduce(lambda x,y: x*10+y, map(char2num, s))
    
    print str2int('1234')

Python内建的filter()函数用于过滤序列。
filter()也接收一个函数和一个序列。和map()不同的时，filter()把传入的函数依次作用于每个元素，然后根据返回值是True还是False决定保留还是丢弃该元素。

    def is_odd(n):
        return n % 2 == 1
    
    filter(is_odd, [1, 2, 4, 5, 6, 9, 10, 15])
    
    def not_empty(s):
        return s and s.strip()
    
    filter(not_empty, ['A', '', 'B', None, 'C', '  '])

Python内置的sorted()函数就可以对list进行排序。
排序也是在程序中经常用到的算法。无论使用冒泡排序还是快速排序，排序的核心是比较两个元素的大小。如果是数字，我们可以直接比较，但如果是字符串或者两个dict呢？直接比较数学上的大小是没有意义的，因此，比较的过程必须通过函数抽象出来。通常规定，对于两个元素x和y，如果认为x < y，则返回-1，如果认为x == y，则返回0，如果认为x > y，则返回1，这样，排序算法就不用关心具体的比较过程，而是根据比较结果直接排序。

    print sorted([36, 5, 12, 9, 21])

sorted()函数也是一个高阶函数，它还可以接收一个比较函数来实现自定义的排序。

    def reversed_cmp(x, y):
        if x > y:
            return -1
        if x < y:
            return 1
        return 0
    
    print sorted([36, 5, 12, 9, 21], reversed_cmp)
    
    sorted(['bob', 'about', 'Zoo', 'Credit'])
    
    def cmp_ignore_case(s1, s2):
        u1 = s1.upper()
        u2 = s2.upper()
        if u1 < u2:
            return -1
        if u1 > u2:
            return 1
        return 0
        
    print sorted(['bob', 'about', 'Zoo', 'Credit'], cmp_ignore_case)

高阶函数除了可以接受函数作为参数外，还可以把函数作为结果值返回。

    def lazy_sum(*args):
        def sum():
            ax = 0
            for n in args:
                ax = ax + n
            return ax
        return sum
    
    f1 = lazy_sum(1, 3, 5, 7, 9)
    print f1()
    f2 = lazy_sum(1, 3, 5, 7, 9)
    print f1 == f2
    # f1()和f2()的调用结果互不影响

在这个例子中，我们在函数lazy_sum中又定义了函数sum，并且，内部函数sum可以引用外部函数lazy_sum的参数和局部变量，当lazy_sum返回函数sum时，相关参数和变量都保存在返回的函数中，这种称为“闭包（Closure）”的程序结构拥有极大的威力。

当我们调用lazy_sum()时，每次调用都会返回一个新的函数，即使传入相同的参数。

    def count():
        fs = []
        for i in range(1, 4):
            def f():
                return i*i
            fs.append(f)
        return fs

    f1, f2, f3 = count()
    print f1(), f2(), f3() #9 9 9

返回闭包时牢记的一点就是：返回函数不要引用任何循环变量，或者后续会发生变化的变量。
返回函数

阅读: 81205
函数作为返回值

高阶函数除了可以接受函数作为参数外，还可以把函数作为结果值返回。

我们来实现一个可变参数的求和。通常情况下，求和的函数是这样定义的：

def calc_sum(*args):
    ax = 0
    for n in args:
        ax = ax + n
    return ax
但是，如果不需要立刻求和，而是在后面的代码中，根据需要再计算怎么办？可以不返回求和的结果，而是返回求和的函数！

def lazy_sum(*args):
    def sum():
        ax = 0
        for n in args:
            ax = ax + n
        return ax
    return sum
当我们调用lazy_sum()时，返回的并不是求和结果，而是求和函数：

>>> f = lazy_sum(1, 3, 5, 7, 9)
>>> f
<function sum at 0x10452f668>
调用函数f时，才真正计算求和的结果：

>>> f()
25
在这个例子中，我们在函数lazy_sum中又定义了函数sum，并且，内部函数sum可以引用外部函数lazy_sum的参数和局部变量，当lazy_sum返回函数sum时，相关参数和变量都保存在返回的函数中，这种称为“闭包（Closure）”的程序结构拥有极大的威力。

请再注意一点，当我们调用lazy_sum()时，每次调用都会返回一个新的函数，即使传入相同的参数：

>>> f1 = lazy_sum(1, 3, 5, 7, 9)
>>> f2 = lazy_sum(1, 3, 5, 7, 9)
>>> f1==f2
False
f1()和f2()的调用结果互不影响。

闭包

注意到返回的函数在其定义内部引用了局部变量args，所以，当一个函数返回了一个函数后，其内部的局部变量还被新函数引用，所以，闭包用起来简单，实现起来可不容易。

另一个需要注意的问题是，返回的函数并没有立刻执行，而是直到调用了f()才执行。我们来看一个例子：

def count():
    fs = []
    for i in range(1, 4):
        def f():
             return i*i
        fs.append(f)
    return fs

f1, f2, f3 = count()
在上面的例子中，每次循环，都创建了一个新的函数，然后，把创建的3个函数都返回了。

你可能认为调用f1()，f2()和f3()结果应该是1，4，9，但实际结果是：

>>> f1()
9
>>> f2()
9
>>> f3()
9
全部都是9！原因就在于返回的函数引用了变量i，但它并非立刻执行。等到3个函数都返回时，它们所引用的变量i已经变成了3，因此最终结果为9。

返回闭包时牢记的一点就是：返回函数不要引用任何循环变量，或者后续会发生变化的变量。

如果一定要引用循环变量怎么办？方法是再创建一个函数，用该函数的参数绑定循环变量当前的值，无论该循环变量后续如何更改，已绑定到函数参数的值不变。

当我们在传入函数时，有些时候，不需要显式地定义函数，直接传入匿名函数更方便。匿名函数有个限制，就是只能有一个表达式，不用写return，返回值就是该表达式的结果。匿名函数也是一个函数对象，也可以把匿名函数赋值给一个变量，再利用变量来调用该函数。也可以把匿名函数作为返回值返回。

    map(lambda x: x * x, [1, 2, 3, 4, 5, 6, 7, 8, 9])
    f = lambda x: x * x
    print f(3)

由于函数也是一个对象，而且函数对象可以被赋值给变量，所以，通过变量也能调用该函数。
函数对象有一个__name__属性，可以拿到函数的名字。

    def now():
        print '2013-12-25'
        
    print now.__name__

假设我们要增强now()函数的功能，比如，在函数调用前后自动打印日志，但又不希望修改now()函数的定义，这种在代码运行期间动态增加功能的方式，称之为“装饰器”（Decorator）。本质上，decorator就是一个返回函数的高阶函数。

    import functools
    def log(func):
        @functools.wraps(func)
        def wrapper(*args, **kw):
            print 'call %s():' % func.__name__
            return func(*args, **kw)
        return wrapper
    
    @log
    def now():
        print '2013-12-25'
      
    print now()

调用now()函数，不仅会运行now()函数本身，还会在运行now()函数前打印一行日志。`now = log(now)`

如果decorator本身需要传入参数，那就需要编写一个返回decorator的高阶函数，写出来会更复杂。`now = log('execute')(now)`

    import functools
    def log(text):
        def decorator(func):
            @functools.wraps(func)
            def wrapper(*args, **kw):
                print '%s %s():' % (text, func.__name__)
                return func(*args, **kw)
            return wrapper
        return decorator
    
    @log('execute')
    def now():
        print '2013-12-25'
        
    print now()

Python的functools模块提供了很多有用的功能，其中一个就是偏函数（Partial function）。要注意，这里的偏函数和数学意义上的偏函数不一样。functools.partial就是帮助我们创建一个偏函数的。

    import functools
    int2 = functools.partial(int, base=2)
    int2('1000000')
    int2('1010101')

简单总结functools.partial的作用就是，把一个函数的某些参数给固定住（也就是设置默认值），返回一个新的函数，调用这个新函数会更简单。


#### 九、Python模块
在Python中，一个.py文件就称之为一个模块（Module）。
使用模块还可以避免函数名和变量名冲突。相同名字的函数和变量完全可以分别存在不同的模块中，因此，我们自己在编写模块时，不必考虑名字会与其他模块冲突。但是也要注意，尽量不要与内置函数名字冲突。
为了避免模块名冲突，Python又引入了按目录来组织模块的方法，称为包（Package）。引入了包以后，只要顶层的包名不与别人冲突，那所有模块都不会与别人冲突。

每一个包目录下面都会有一个__init__.py的文件，这个文件是必须存在的，否则，Python就把这个目录当成普通目录，而不是一个包。__init__.py可以是空文件，也可以有Python代码，因为__init__.py本身就是一个模块，而它的模块名就是包名。

任何模块代码的第一个字符串都被视为模块的文档注释。

使用__author__变量把作者写进去，这样当你公开源代码后别人就可以瞻仰你的大名。

导入sys模块后，我们就有了变量sys指向该模块，利用sys这个变量，就可以访问sys模块的所有功能。

导入模块时，还可以使用别名，这样，可以在运行时根据当前环境选择最合适的模块。比如Python标准库一般会提供StringIO和cStringIO两个库，这两个库的接口和功能是一样的，但是cStringIO是C写的，速度更快。

    try:
        import cStringIO as StringIO
    except ImportError: # 导入失败会捕获到ImportError
        import StringIO

当我们在命令行运行hello模块文件时，Python解释器把一个特殊变量__name__置为__main__，而如果在其他地方导入该hello模块时，if判断将失败，因此，这种if测试可以让一个模块通过命令行运行时执行一些额外的代码，最常见的就是运行测试。

在一个模块中，我们可能会定义很多函数和变量，但有的函数和变量我们希望给别人使用，有的函数和变量我们希望仅仅在模块内部使用。在Python中，是通过_前缀来实现的。

正常的函数和变量名是公开的（public），可以被直接引用。
类似__xxx__这样的变量是特殊变量，可以被直接引用，但是有特殊用途，比如上面的__author__，__name__就是特殊变量，hello模块定义的文档注释也可以用特殊变量__doc__访问，我们自己的变量一般不要用这种变量名。
类似_xxx和__xxx这样的函数或变量就是非公开的（private），不应该被直接引用。
private函数和变量“不应该”被直接引用，而不是“不能”被直接引用，是因为Python并没有一种方法可以完全限制访问private函数或变量，但是，从编程习惯上不应该引用private函数或变量。

在Python中，安装第三方模块，是通过setuptools这个工具完成的。Python有两个封装了setuptools的包管理工具：easy_install和pip。目前官方推荐使用pip。

现在，让我们来安装一个第三方库——Python Imaging Library，这是Python下非常强大的处理图像的工具库。一般来说，第三方库都会在Python官方的pypi.python.org网站注册，要安装一个第三方库，必须先知道该库的名称，可以在官网或者pypi上搜索，比如Python Imaging Library的名称叫PIL，因此，安装Python Imaging Library的命令就是：`pip install PIL`

* PIL
* MySQL-python
* numpy
* Jinja2

当我们试图加载一个模块时，Python会在指定的路径下搜索对应的.py文件，如果找不到，就会报错。默认情况下，Python解释器会搜索当前目录、所有已安装的内置模块和第三方模块，搜索路径存放在sys模块的path变量中。

    import sys
    sys.path
    sys.path.append('/Users/michael/my_py_scripts')

添加自己的搜索目录，设置环境变量PYTHONPATH。

Python提供了__future__模块，把下一个新版本的特性导入到当前版本，于是我们就可以在当前版本中测试一些新版本的特性。

    # still running on Python 2.7
    from __future__ import unicode_literals
    print '\'xxx\' is unicode?', isinstance('xxx', unicode)
    print 'u\'xxx\' is unicode?', isinstance(u'xxx', unicode)
    print '\'xxx\' is str?', isinstance('xxx', str)
    print 'b\'xxx\' is str?', isinstance(b'xxx', str)
    
    from __future__ import division
    print '10 / 3 =', 10 / 3
    print '10.0 / 3 =', 10.0 / 3
    print '10 // 3 =', 10 // 3


#### 十、Python面向对象
面向对象最重要的概念就是类（Class）和实例（Instance），必须牢记类是抽象的模板，比如Student类，而实例是根据类创建出来的一个个具体的“对象”，每个对象都拥有相同的方法，但各自的数据可能不同。

数据封装、继承和多态只是面向对象程序设计中最基础的3个概念。还有多重继承、定制类、元类等概念。

    class Student(object):
        pass
    
    bart = Student()
    print bart

class后面紧接着是类名，即Student，类名通常是大写开头的单词，紧接着是(object)，表示该类是从哪个类继承下来的，继承的概念我们后面再讲，通常，如果没有合适的继承类，就使用object类，这是所有类最终都会继承的类。

可以自由地给一个实例变量绑定属性。

    bart.name = 'Bart Simpson'
    print bart.name

由于类可以起到模板的作用，因此，可以在创建实例的时候，把一些我们认为必须绑定的属性强制填写进去。通过定义一个特殊的__init__方法，在创建实例的时候，就把name，score等属性绑上去。

    class Student(object):
        def __init__(self, name, score):
            self.name = name
            self.score = score

注意到__init__方法的第一个参数永远是self，表示创建的实例本身，因此，在__init__方法内部，就可以把各种属性绑定到self，因为self就指向创建的实例本身。

有了__init__方法，在创建实例的时候，就不能传入空的参数了，必须传入与__init__方法匹配的参数，但self不需要传，Python解释器自己会把实例变量传进去。

      bart = Student('Bart Simpson', 59)
      print bart.name

和普通的函数相比，在类中定义的函数只有一点不同，就是第一个参数永远是实例变量self，并且，调用时，不用传递该参数。除此之外，类的方法和普通函数没有什么区别，所以，你仍然可以用默认参数、可变参数和关键字参数。

面向对象编程的一个重要特点就是数据封装。

既然Student实例本身就拥有这些数据，要访问这些数据，就没有必要从外面的函数去访问，可以直接在Student类的内部定义访问数据的函数，这样，就把“数据”给封装起来了。这些封装数据的函数是和Student类本身是关联起来的，我们称之为类的方法。

要定义一个方法，除了第一个参数是self外，其他和普通函数一样。要调用一个方法，只需要在实例变量上直接调用，除了self不用传递，其他参数正常传入。

封装的另一个好处是可以给Student类增加新的方法。

如果要让内部属性不被外部访问，可以把属性的名称前加上两个下划线__，在Python中，实例的变量名如果以__开头，就变成了一个私有变量（private），只有内部可以访问，外部不能访问。

    class Student(object):
        def __init__(self, name, score):
            self.__name = name
            self.__score = score
        
        def print_score(self):
            print '%s: %s' % (self.__name, self.__score)

这样就确保了外部代码不能随意修改对象内部的状态，这样通过访问限制的保护，代码更加健壮。
但是如果外部代码要获取name和score怎么办？可以给Student类增加get_name和get_score这样的方法。
如果又要允许外部代码修改score怎么办？可以给Student类增加set_score方法。
在方法中，可以对参数做检查，避免传入无效的参数。

需要注意的是，在Python中，变量名类似__xxx__的，也就是以双下划线开头，并且以双下划线结尾的，是特殊变量，特殊变量是可以直接访问的，不是private变量，所以，不能用__name__、__score__这样的变量名。

有些时候，你会看到以一个下划线开头的实例变量名，比如_name，这样的实例变量外部是可以访问的，但是，按照约定俗成的规定，当你看到这样的变量时，意思就是，“虽然我可以被访问，但是，请把我视为私有变量，不要随意访问”。

双下划线开头的实例变量是不是一定不能从外部访问呢？其实也不是。不能直接访问__name是因为Python解释器对外把__name变量改成了_Student__name，所以，仍然可以通过_Student__name来访问__name变量。

继承的最大好处是子类获得了父类的全部功能。可以对子类增加一些方法。继承的第二个好处需要我们对原代码做一点改进。

当子类和父类都存在相同的run()方法时，我们说，子类的run()覆盖了父类的run()，在代码运行的时候，总是会调用子类的run()。这样，我们就获得了继承的另一个好处：多态。

判断一个变量是否是某个类型可以用isinstance()判断。

在继承关系中，如果一个实例的数据类型是某个子类，那它的数据类型也可以被看做是父类。但是，反过来就不行。

对于一个变量，我们只需要知道它是Animal类型，无需确切地知道它的子类型，就可以放心地调用run()方法，而具体调用的run()方法是作用在Animal、Dog、Cat还是Tortoise对象上，由运行时该对象的确切类型决定，这就是多态真正的威力：调用方只管调用，不管细节，而当我们新增一种Animal的子类时，只要确保run()方法编写正确，不用管原来的代码是如何调用的。

对扩展开放：允许新增Animal子类；
对修改封闭：不需要修改依赖Animal类型的run_twice()等函数。

判断对象类型，使用type()函数。基本类型都可以用type()判断。如果一个变量指向函数或者类，也可以用type()判断。type()函数返回type类型。Python把每种type类型都定义好了常量，放在types模块里，使用之前，需要先导入。类型本身的类型就是TypeType。

      import types
      type('abc')==types.StringType
      type(u'abc')==types.UnicodeType
      type([])==types.ListType
      type(str)==types.TypeType

对于class的继承关系来说，使用type()就很不方便。我们要判断class的类型，可以使用isinstance()函数。isinstance()判断的是一个对象是否是该类型本身，或者位于该类型的父继承链上。能用type()判断的基本类型也可以用isinstance()判断。并且还可以判断一个变量是否是某些类型中的一种。

    isinstance('a', (str, unicode))
    isinstance(u'a', (str, unicode))
    isinstance(u'a', basestring)
    isinstance(h, Animal)

如果要获得一个对象的所有属性和方法，可以使用dir()函数，它返回一个包含字符串的list。

类似__xxx__的属性和方法在Python中都是有特殊用途的，比如__len__方法返回长度。在Python中，如果你调用len()函数试图获取一个对象的长度，实际上，在len()函数内部，它自动去调用该对象的__len__()方法。剩下的都是普通属性或方法。

仅仅把属性和方法列出来是不够的，配合getattr()、setattr()以及hasattr()，我们可以直接操作一个对象的状态。如果试图获取不存在的属性，会抛出AttributeError的错误。

正常情况下，当我们定义了一个class，创建了一个class的实例后，我们可以给该实例绑定任何属性和方法，这就是动态语言的灵活性。但是，给一个实例绑定的方法，对另一个实例是不起作用的。为了给所有实例都绑定方法，可以给class绑定方法。给class绑定方法后，所有实例均可调用。通常情况下，上面的set_score方法可以直接定义在class中，但动态绑定允许我们在程序运行的过程中动态给class加上功能，这在静态语言中很难实现。

      class Student(object):
          pass
      
      s = Student()
      s.name = 'Michael'
      def set_age(self, age): 
          self.age = age
      from types import MethodType
      s.set_age = MethodType(set_age, s, Student)
      s.set_age(25)
      print s.age
      Student.set_score = MethodType(set_score, None, Student)
      

为了达到限制的目的，Python允许在定义class的时候，定义一个特殊的__slots__变量，来限制该class能添加的属性。

      class Student(object):
          __slots__ = ('name', 'age') # 用tuple定义允许绑定的属性名称

由于'score'没有被放到__slots__中，所以不能绑定score属性，试图绑定score将得到AttributeError的错误。
使用__slots__要注意，__slots__定义的属性仅对当前类起作用，对继承的子类是不起作用的。除非在子类中也定义__slots__，这样，子类允许定义的属性就是自身的__slots__加上父类的__slots__。


Python内置的@property装饰器就是负责把一个方法变成属性调用的。

      class Student(object):
          @property
          def score(self):
              return self._score
          @score.setter
          def score(self, value):
              if not isinstance(value, int):
                  raise ValueError('score must be an integer!')
              if value < 0 or value > 100:
                  raise ValueError('score must between 0 ~ 100!')
              self._score = value

@property的实现比较复杂，我们先考察如何使用。把一个getter方法变成属性，只需要加上@property就可以了，此时，@property本身又创建了另一个装饰器@score.setter，负责把一个setter方法变成属性赋值，于是，我们就拥有一个可控的属性操作。

Python允许使用多重继承，因此，Mixin就是一种常见的设计。

通过多重继承，一个子类就可以同时获得多个父类的所有功能。
在设计类的继承关系时，通常，主线都是单一继承下来的，例如，Ostrich继承自Bird。但是，如果需要“混入”额外的功能，通过多重继承就可以实现，比如，让Ostrich除了继承自Bird外，再同时继承Runnable。这种设计通常称之为Mixin。

Mixin的目的就是给一个类增加多个功能，这样，在设计类的时候，我们优先考虑通过多重继承来组合多个Mixin的功能，而不是设计多层次的复杂的继承关系。

我们不需要复杂而庞大的继承链，只要选择组合不同的类的功能，就可以快速构造出所需的子类。


Python的class中还有许多这样有特殊用途的函数，可以帮助我们定制类。
* __len__
* __str__
* __repr__
* __iter__
* __getitem__
* __setitem__
* __delitem__
* __getattr__
* __call__

通过callable()函数，我们就可以判断一个对象是否是“可调用”对象。

动态语言和静态语言最大的不同，就是函数和类的定义，不是编译时定义的，而是运行时动态创建的。

type()函数既可以返回一个对象的类型，又可以创建出新的类型，比如，我们可以通过type()函数创建出Hello类，而无需通过class Hello(object)...的定义。

要创建一个class对象，type()函数依次传入3个参数：
* class的名称；
* 继承的父类集合，注意Python支持多重继承，如果只有一个父类，别忘了tuple的单元素写法；
* class的方法名称与函数绑定，这里我们把函数fn绑定到方法名hello上。

通过type()函数创建的类和直接写class是完全一样的，因为Python解释器遇到class定义时，仅仅是扫描一下class定义的语法，然后调用type()函数创建出class。

type()函数允许我们动态创建出类来，也就是说，动态语言本身支持运行期动态创建类，这和静态语言有非常大的不同，要在静态语言运行期创建类，必须构造源代码字符串再调用编译器，或者借助一些工具生成字节码实现，本质上都是动态编译，会非常复杂。

除了使用type()动态创建类以外，要控制类的创建行为，还可以使用metaclass。

先定义metaclass，就可以创建类，最后创建实例。

metaclass允许你创建类或者修改类。换句话说，你可以把类看成是metaclass创建出来的“实例”。

      # metaclass是创建类，所以必须从`type`类型派生：
      class ListMetaclass(type):
          def __new__(cls, name, bases, attrs):
              attrs['add'] = lambda self, value:       self.append(value)
              return type.__new__(cls, name, bases, attrs)
      class MyList(list):
          __metaclass__ = ListMetaclass # 指示使用ListMetaclass来定制类


__new__()方法接收到的参数依次是：
* 当前准备创建的类的对象；
* 类的名字；
* 类继承的父类集合；
* 类的方法集合


#### 十一、Python异常处理
有的错误是程序编写有问题造成的，比如本来应该输出整数结果输出了字符串，这种错误我们通常称之为bug，bug是必须修复的。
有的错误是用户输入造成的，比如让用户输入email地址，结果得到一个空字符串，这种错误可以通过检查用户输入来做相应的处理。
还有一类错误是完全无法在程序运行过程中预测的，比如写入文件的时候，磁盘满了，写不进去了，或者从网络抓取数据，网络突然断掉了。这类错误也称为异常，在程序中通常是必须处理的，否则，程序会因为各种问题终止并退出。

Python内置了一套异常处理机制，来帮助我们进行错误处理。
Python的pdb可以让我们以单步方式执行代码。

在程序运行的过程中，如果发生了错误，可以事先约定返回一个错误代码，这样，就可以知道是否有错，以及出错的原因。在操作系统提供的调用中，返回错误码非常常见。比如打开文件的函数open()，成功时返回文件描述符（就是一个整数），出错时返回-1。
用错误码来表示是否出错十分不便，因为函数本身应该返回的正常结果和错误码混在一起，造成调用者必须用大量的代码来判断是否出错。

所以高级语言通常都内置了一套try...except...finally...的错误处理机制，Python也不例外。

      try:
          print 'try...'
          r = 10 / 0
          print 'result:', r
      except ZeroDivisionError, e:
          print 'except:', e
      finally:
          print 'finally...'
      print 'END'

如果发生了不同类型的错误，应该由不同的except语句块处理。没错，可以有多个except来捕获不同类型的错误。如果没有错误发生，可以在except语句块后面加一个else，当没有错误发生时，会自动执行else语句。

Python的错误其实也是class，所有的错误类型都继承自BaseException，所以在使用except时需要注意的是，它不但捕获该类型的错误，还把其子类也“一网打尽”。

Python所有的错误都是从BaseException类派生的，常见的错误类型和继承关系看这里：[错误类](https://docs.python.org/2/library/exceptions.html#exception-hierarchy)。

不需要在每个可能出错的地方去捕获错误，只要在合适的层次去捕获错误就可以了。

如果错误没有被捕获，它就会一直往上抛，最后被Python解释器捕获，打印一个错误信息，然后程序退出。

因为错误是class，捕获一个错误就是捕获到该class的一个实例。因此，错误并不是凭空产生的，而是有意创建并抛出的。Python的内置函数会抛出很多类型的错误，我们自己编写的函数也可以抛出错误。

如果要抛出错误，首先根据需要，可以定义一个错误的class，选择好继承关系，然后，用raise语句抛出一个错误的实例。

启动Python解释器时可以用-O参数来关闭assert。

      assert n != 0, 'n is zero!'

如果断言失败，assert语句本身就会抛出AssertionError。

和assert比，logging不会抛出错误，而且可以输出到文件。

      import logging
      logging.basicConfig(level=logging.INFO)
      logging.info('n = %d' % n)

这就是logging的好处，它允许你指定记录信息的级别，有debug，info，warning，error等几个级别，当我们指定level=INFO时，logging.debug就不起作用了。同理，指定level=WARNING后，debug和info就不起作用了。这样一来，你可以放心地输出不同级别的信息，也不用删除，最后统一控制输出哪个级别的信息。
logging的另一个好处是通过简单的配置，一条语句可以同时输出到不同的地方，比如console和文件。

启动Python的调试器pdb，让程序以单步方式运行，可以随时查看运行状态。

      python -m pdb err.py
      输入命令l来查看代码
      输入命令n可以单步执行代码
      任何时候都可以输入命令p 变量名来查看变量
      输入命令q结束调试，退出程序
      需要import pdb，然后，在可能出错的地方放一个pdb.set_trace()，就可以设置一个断点
      用命令c继续运行

单元测试是用来对一个模块、一个函数或者一个类来进行正确性检验的测试工作。

为了编写单元测试，我们需要引入Python自带的unittest模块。
编写单元测试时，我们需要编写一个测试类，从unittest.TestCase继承。
以test开头的方法就是测试方法，不以test开头的方法不被认为是测试方法，测试的时候不会被执行。
对每一类测试都需要编写一个test_xxx()方法。由于unittest.TestCase提供了很多内置的条件判断，我们只需要调用这些方法就可以断言输出是否是我们所期望的。最常用的断言就是assertEquals()。另一种重要的断言就是期待抛出指定类型的Error，比如通过d['empty']访问不存在的key时，断言会抛出KeyError。而通过d.empty访问不存在的key时，我们期待抛出AttributeError。

      self.assertEquals(abs(-1), 1) # 断言函数返回的结果与1相等
      with self.assertRaises(KeyError):
          value = d['empty']
      with self.assertRaises(AttributeError):
          value = d.empty
      
      if __name__ == '__main__':
          unittest.main()
      
      python mydict_test.py
      python -m unittest mydict_test

可以在单元测试中编写两个特殊的setUp()和tearDown()方法。这两个方法会分别在每调用一个测试方法的前后分别被执行。

* 单元测试可以有效地测试某个程序模块的行为，是未来重构代码的信心保证。
* 单元测试的测试用例要覆盖常用的输入组合、边界条件和异常。
* 单元测试代码要非常简单，如果测试代码太复杂，那么测试代码本身就可能有bug。
* 单元测试通过了并不意味着程序就没有bug了，但是不通过程序肯定有bug。

可以把代码与其他说明可以写在注释中，然后，由一些工具来自动生成文档。这些代码可以自动执行。

      def abs(n):
          '''
          Function to get absolute value of number.
          Example:
          >>> abs(1)
          1
          >>> abs(-1)
          1
          >>> abs(0)
          0
          '''
          return n if n >= 0 else (-n)
      
      if __name__=='__main__':
          import doctest
          doctest.testmod()

Python内置的“文档测试”（doctest）模块可以直接提取注释中的代码并执行测试。
doctest严格按照Python交互式命令行的输入和输出来判断测试结果是否正确。只有测试异常的时候，可以用...表示中间一大段烦人的输出。

