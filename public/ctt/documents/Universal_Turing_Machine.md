
# Universal Turing Machine

> *作者：Arvin 日期：2017年6月6日*

----------------------------------------

>BEGIN

### 图灵机

-----------------------------------------

图灵机，又称图灵计算、图灵计算机，是由数学家阿兰·麦席森·图灵（1912～1954）提出的一种抽象计算模型，即将人们使用纸笔进行数学运算的过程进行抽象，由一个虚拟的机器替代人们进行数学运算。

所谓的图灵机就是指一个抽象的机器，它有一条无限长的纸带，纸带分成了一个一个的小方格，每个方格有不同的颜色。有一个机器头在纸带上移来移去。机器头有一组内部状态，还有一些固定的程序。在每个时刻，机器头都要从当前纸带上读入一个方格信息，然后结合自己的内部状态查找程序表，根据程序输出信息到纸带方格上，并转换自己的内部状态，然后进行移动。

图灵的基本思想是用机器来模拟人们用纸笔进行数学运算的过程，他把这样的过程看作下列两种简单的动作：

1. 在纸上写上或擦除某个符号；

2. 把注意力从纸的一个位置移动到另一个位置；

3. 而在每个阶段，人要决定下一步的动作，依赖于 (a) 此人当前所关注的纸上某个位置的符号和（b) 此人当前思维的状态。

为了模拟人的这种运算过程，图灵构造出一台假想的机器，该机器由以下几个部分组成：

1. 一条无限长的纸带 TAPE。纸带被划分为一个接一个的小格子，每个格子上包含一个来自有限字母表的符号，字母表中有一个特殊的符号 表示空白。纸带上的格子从左到右依此被编号为 0,1,2,... ，纸带的右端可以无限伸展。

2. 一个读写头 HEAD。该读写头可以在纸带上左右移动，它能读出当前所指的格子上的符号，并能改变当前格子上的符号。

3. 一套控制规则 TABLE。它根据当前机器所处的状态以及当前读写头所指的格子上的符号来确定读写头下一步的动作，并改变状态寄存器的值，令机器进入一个新的状态。

4. 一个状态寄存器。它用来保存图灵机当前所处的状态。图灵机的所有可能状态的数目是有限的，并且有一个特殊的状态，称为停机状态。

### 通用图灵机

-----------------------------------

对于任意一个图灵机，因为它的描述是有限的，因此我们总可以用某种方式将其编码为字符串。我们用 <M> 表示图灵机 M 的编码。我们可以构造出一个特殊的图灵机，它接受任意一个图灵机 M 的编码<M> ，然后模拟 M 的运作，这样的图灵机称为通用图灵机(Universal Turing Machine）。

现代电子计算机其实就是这样一种通用图灵机的模拟，它能接受一段描述其他图灵机的程序，并运行程序实现该程序所描述的算法。但要注意，它只是模拟，因为现实中的计算机的存储都是有限的，所以无法跨越有限状态机的界限。

### 举例

-------------------------------------

1. 现代通用PC计算机，就是一个硬件实现的图灵机

2. 图灵完整的计算机编程语言

3. 计算机软件模拟器

4. 任何使用编程语言编写的特定功能，是一个图灵机，但不是完整的图灵机

5. BrainFuck编程语言（8条指令、1个读写头、1个状态寄存器）

6. x86指令系统，就是一个完整的图灵机

7. 虚拟机

### 如何制造一个图灵机

-------------------------------------

1. 准备一条纸带，也就是内存、数组等
2. 准备读写头，包含内部状态和一组固定的程序，也就是状态寄存器和指令，即CPU
3. 指令应该包含，读写头移动指令、纸带读写指令、纸带数据运算指令
4. 读取纸带数据，结合内部状态，查询指令，并执行指令（数据运算、数据读写、读写头移动）

注意：指令，固化的程序，决定着图灵机的功能。真机还需要一个时钟，使CPU周期性的取指令和执行指令。是否是图灵完整的系统，可以通过指令是否是图灵完整来判断。判断通用图灵机，可以根据是否支持模拟任意图灵机（执行特定图灵机编码）。实际的机器，数据跟程序是混在一起的，也就是“存储程序”思想，你也可以认为程序也是数据。指令执行的环境可以认为是传统意义的数据。


### 思考

-------------------------------------

为什么写一个CPU模拟器，跟开发一门编程语言，逻辑上非常相似？究其根本，因为它们都是在实现一个图灵完整的图灵机。反汇编器跟CPU模拟器，在逻辑上几本没什么区别。BrainFuck语言解释器跟CPU模拟器的实现逻辑也差不多。运行也是获取指令、执行指令并修改内部状态（寄存器）。你可以认为，语言解释器（VM）就是一个图灵机。

现代高级编程语言，基于完整的图灵机，提供了条件结构、循环结构、以及函数子程序块、类等逻辑结构，使编码（特定图灵机的编写）更加符合人类的思维习惯。这也是计算机科学上的一次飞跃，大大加快了软件的开发速度。

指令，决定图灵机的功能。如果能动态更改指令，那么就可以动态更改图灵机的功能。复杂指令集CPU（如：6502、Z80等）具有类似的功能，根据寻址方式的不同，同一指令执行不同的操作。

指令可以看作，人类大脑中的加减乘除运算法则，内部状态（寄存器）可以看作分支的判断条件，左右移动可以看作根据分支条件执行不同的指令（运算），而纸带可以看作记录算式和计算过程以及计算结果的纸。

### 结语

---------------------------------------

从人类的计算过程，抽象出图灵机，从图灵机制造出了电子计算机，从计算机中提炼出了编程语言，最后使用编程语言编写各种功能的软件。这就是计算机科学的进步，但是图灵计算模型从来没变。有时候模仿人类，或许就能得到最佳答案。识别人类行为模式，或许就能得到新的模型，因为人类进化历程中形成很多优秀的行为模式。

### 附件

---------------------------------------

Each turing machine can be specified by the five elements:

    * A finite set of symbols A, also called the alphabet.
    * An initial symbol ainit, which each cell contains on initialization.
    * A finite set of states S.
    * An initial state sinit.
    * A final state sfinal.
    * A function f of state and symbols onto a tupple consisting of a new state, a new symbol, and a movement. The movement can be one of right and left.

Many interesting properties of Turing Machines have been proven so far. For example, that each Turing Machine can be transformed into a Turing Machine with a tape that is cut-off on one side, and endless on the other side. Or that every Turing machine can be transformed into a Turing machine with set of symbols contains just two symbols.

A language is said to be 'Turing-complete', if for each functions that can be calculated with a Turing Machine, it can be shown that there is a program in this language that performs the same function. There are basically three approaches to proof that a language is Turing-complete. These are:

    1. Show there is some mapping from each possible Turing machine to a program in the language.
    2. Show that there is a program in the language that emulates a Universal Turing Machine.
    3. Show that the language is equivalent with (or a superset of) a language that is known to be Turing-complete.


>END
