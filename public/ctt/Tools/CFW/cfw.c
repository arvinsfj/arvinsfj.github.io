#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>

#define SIZE_OF(arr) (int)(sizeof(arr)/sizeof(arr[0])) //数组size

//命令协议实现
//hello 命令
void cmd_hello(char* cmdstr, int length)
{
	printf("%s\n", "hello, command!");
}

typedef void (*func)(char*, int);
struct cmd_action {
	char* name;
	func action;
} cmdarr[] = {
	{"hello", cmd_hello},
};

////////////////////////////////以上是命令的实现//////////////////////////

//命令分发函数
void cmd_dispatch(char* cmdstr, int length)
{
	//命令分发
	for (int i = 0; i < SIZE_OF(cmdarr); ++i) {
		struct cmd_action cmd = cmdarr[i];
		if (!strncmp(cmdstr, cmd.name, strlen(cmd.name))) {
			cmd.action(cmdstr, length);
			return;
		}
	}
	printf("%s\n", "command not found!");
}

void subprocess(int read_fd)
{
	//子进程
	char read_buf[512] = {0};
	int num = -1;
	int done = 1;
	do {
		memset(read_buf, 0, sizeof(read_buf));
		num = read(read_fd, read_buf, sizeof(read_buf));
		if (!strcmp(read_buf, "quit")) {
			done = 0;
		} else {
			//执行命令
			cmd_dispatch(read_buf, num);
		}

	} while(done);

	close(read_fd);//关闭读取通道

	exit(0);//关闭子进程
}

int main(int argc, char** argv)
{
	//

	//pipe
	int fd[2];
	int result = pipe(fd);
	if (result == -1) {
		perror("pipe error!");
		exit(1);
	}
	//fork
	pid_t pid = fork();
	if (pid == 0) {
		//子进程
		close(fd[1]);//关闭写通道
		subprocess(fd[0]);//监督读取通道
	} else if (pid > 0){
		//主进程
		close(fd[0]);//关闭读通道，使用写通道
	} else {
		//fork错误
		char* errinfo = strerror(errno);
		int length = strlen(errinfo)+7;
		char* err = malloc(length);
		snprintf(err, length, "FORK: %s", errinfo);
		perror(err);
		free(err);
		exit(1);
	}

	char str[512];
	int done = 1;
	do {
		scanf("%s", str);
		if (!strcmp(str, "quit")) {
			done = 0;
			write(fd[1], str, strlen(str)+1);
			wait(NULL);
		} else {
			//向子进程写入命令
			write(fd[1], str, strlen(str)+1);
		}
	} while (done);
	
	close(fd[1]);//关闭写通道

	return 0;
}