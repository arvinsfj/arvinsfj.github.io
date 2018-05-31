# Sublime配置单个C文件的编译和运行
-----------------------------------------

Sublime目前算是Mac上的能满足自己做C语言实验需求的编辑器了。（ps：还用不到vim和emacs啦）

#### 一、基本思路

1. 安装Sublime和购买输入LICENSE

2. 配置C的编译和运行脚本

3. shift+command+b组合键，选择编译或运行

#### 二、步骤

1. 正确安装Sublime并打开
2. 选择```Tools -> Build System -> New Build System```
3. 输入配置脚本（c.sublime-build）
4. 保存配置脚本文件到"/Users/当前用户名/Library/Application Support/Sublime Text 3/Packages/User"目录下
5. shift+command+b组合键，选择编译或运行（编译和运行结果会在Sublime底部弹出显示，若错误还会在代码下面提示）

配置脚本如下：

```
{
    //"shell_cmd": "make"
    "working_dir": "$file_path",
    "shell_cmd": "gcc -Wall \"$file_name\" -o \"$file_base_name\" && gcc -S \"$file_name\"",
    "file_regex": "^(..[^:]*):([0-9]+):?([0-9]+)?:? (.*)$",
    "selector": "source.c",

    "variants": 
    [
        {   
            "name": "Run",
            "shell_cmd": "gcc -Wall \"$file\" -o \"$file_base_name\" && \"${file_path}/${file_base_name}\""
        }
    ]
}
```

#### 三、随便说点

记录一下，虽然很简单。


