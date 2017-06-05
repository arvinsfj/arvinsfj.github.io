
# Brainfuck Programming Language

> *作者：Arvin 日期：2017年6月5日*

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

The semantics of the Brainfuck commands can also be succinctly expressed in terms of C, as follows (assuming that p has been previously defined as a char*):

    >  becomes  ++p;
    <  becomes  --p;
    +  becomes  ++*p;
    -  becomes  --*p;
    .  becomes  putchar(*p);
    ,  becomes  *p = getchar();
    [  becomes  while (*p) {
    ]  becomes  }

>END

[BrainFuck](http://www.muppetlabs.com/~breadbox/bf/) and [Download](documents/BF.zip)
