# Markdown语法
-------------------------

### 段落和换行
-------------------------
`<enter>键`


### 标题（共6级）（HEADERS）
-------------------------
`# 标题H1`

`## 标题H2`

`### 标题H3`

### This is an H3 ######


### 引用（嵌套headers, lists, code blocks）（BLOCKQUOTES）
-------------------------
`>引用文本1`

`>##### 引用文本2`

`>1. 引用文本3`

`> 引用文本3`

> ## This is a header.
> 
> 1. This is the first list item.
> 2. This is the second list item.
> 
> Here's some example code:
> 
> return shell_exec("echo $input | $markdown_script");


### 列表（LISTS）
-------------------------
`* Red`

`* Green`

`* Blue`

`1. Bird`

`2. McHale`

`3. Parish`

* Red
* Green
* Blue


1. Bird
2. McHale
3. Parish



### 代码块（至少4个空格或1个tab）（CODE BLOCKS）
-------------------------
`    This is a code block.`

    This is a code block.


### 横线（HORIZONTAL RULES）
-------------------------
`* * *`

`***`

`*****`

`- - -`

`---------------------------------------`

--------


### 链接（LINKS）
-------------------------
`This is [an example](http://example.com/ "Title") inline link.`

This is [an example](http://example.com/ "Title") inline link.

`This is [an example][id] reference-style link.`

`[id]: http://example.com/ "Optional Title Here"`

This is [an example][id] reference-style link.
[id]: http://example.com/ "Optional Title Here"

`![Alt text](/path/to/img.jpg)`

`![Alt text](/path/to/img.jpg "Optional title")`

![Alt text](/path/to/img.jpg)

![Alt text](/path/to/img.jpg "Optional title")

`![Alt text][id]`

`[id]: url/to/image "Optional title attribute"`

![Alt text][id]
[id]: url/to/image "Optional title attribute"

`<http://example.com/>`

<http://example.com/>

`<address@example.com>`

<address@example.com>


### 强调（EMPHASIS）
-------------------------
`*single asterisks*`

`_single underscores_`

`**double asterisks**`

`__double underscores__`

*single asterisks*

_single underscores_

**double asterisks**

__double underscores__

un*frigging*believable

\*this text is surrounded by literal asterisks\*


### 代码（CODE）
-------------------------
    Use the `printf()` function.

Use the `printf()` function.

``There is a literal backtick (`) here.``


### 字符转义（BACKSLASH ESCAPES）
-------------------------
`\*literal asterisks\*`

\*literal asterisks\*

    \ backslash
    ` backtick
    * asterisk
    _ underscore
    {} curly braces
    [] square brackets
    () parentheses
    # hash mark
    + plus sign
    - minus sign (hyphen)
    . dot
    ! exclamation mark`
	