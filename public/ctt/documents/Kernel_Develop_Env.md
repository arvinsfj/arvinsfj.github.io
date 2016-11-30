# **Win10下OS内核开发环境搭建**

>*作者：Arvin 日期：2016年11月30日*

-----------------------------

>BEGIN

#### 一、基本思路

我的机器是Win10的操作系统，不想装Linux但编译和调试内核必须在Linux环境下。因此，只能利用虚拟机装Linux，在Linux下编译和运行操作系统内核，源代码编写还是Win10下进行，Win10跟虚拟机系统的源代码管理采用Git，命令操作采用Win10远程登陆Linux的方式。

Win10自带hyper-v虚拟机，只要开启一下就可以。Linux使用Ubuntu发行版，工具全而且方便。在Ubuntu下安装gcc，bochs，git，openssl-server。Ubuntu需要配置网络，使能跟Win10互通。Win10安装Git服务器软件GitStack，源码编辑器，Putty。

#### 二、搭建步骤

1. Win10开启hyper-v虚拟机(在BIOS里面开启虚拟化支持，在Win10组件里面开启hyper-v)
2. hyper-v中安装ubuntu，并配置网络
3. ubuntu安装gcc、bochs、git、openssl-server等，并开启openssl-server
4. 编写shell脚本，make脚本
5. win10安装远程登录客户端Putty
6. win10安装git服务器GitStack

#### 三、使用方式

在Win10上编写源代码，并提交到GitStack服务器上，在Putty里使用Git命令更新源码，并使用gcc编译源码，最后bochs运行系统内核。

#### 四、额外好处

* 对Linux感兴趣但不想真机装Linux的同学，也可以使用该方法搭建Linux练习环境。使用hyper-v装Linux，Linux装openssl-server，使用Putty远程登陆Linux。登陆成功后，就可以使用真实的Linux了。
* 不需要额外装其他虚拟机，而且hyper-v性能还行，可以后台运行。
* 虚拟机安全，不存在文件丢失问题，而且在Linux里面可以安装任何软件扩展性较好。

>END
