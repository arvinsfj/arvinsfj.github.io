# mac电脑搭建git服务器
-----------------------------------------

#### 一、基本思路

1. 使用ssh协议进行git的pull和push。

2. 配置"[receive] denyCurrentBranch = ignore”使git接受push操作。

3. 配置.gitignore文件，忽略不需要管理的文件。

4. 使用SourceTree进行日常操作。也可以使用xcode自带的Source Control功能进行日常操作。

#### 二、基本步骤

1. mac电脑自带git。

2. 打开remote login共享，其实是打开ssh。

3. 打开/etc/sshd_config文件，并修改"#UseDNS yes"成"UseDNS no"。优化ssh加速ssh的访问。

4. ssh使用rsa方式认证，加速ssh的访问。将客户端的公钥拷贝给服务器。

5. 如果局域网限制了各个mac的访问权限，则可以开启mac的internet share，自己组建局域网。服务器ip是192.168.2.1。

6. 安装SourceTree免费软件。该软件是git的图形操作界面。

7. 使用git init --bare命令初始化一个git项目。

8. 打开.git/config文件，在尾部添加"[receive] denyCurrentBranch = ignore"

9. 添加.gitignore文件到git服务器。git add和git commit命令完成。

10. 客户端使用SourceTree或者其他工具进行项目管理。git clone ssh://username@192.168.2.1/Users/username/project_path

