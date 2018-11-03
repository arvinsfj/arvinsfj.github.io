
# UDP NAT Hole Punching (UDP打洞)

> *作者：Arvin 日期：2018年11月03日*

---------------------------------

>BEGIN

UDP NAT 局域网穿透，俗称“打洞”。Libjingle是谷歌的一个知名的P2P库，部分呢实现了Jingle协议，通过它可以建立一个直通的连接，实现点对点数据传输。“打洞”技术基本也是为了实现点对点传输而存在的。
像即时通信类应用，比如：QQ、Google Talk等，一般都会用到这种技术。

### 一、思路
---------------------------------

技术思路不难，就是客户端a和b，同时连接公网的中间服务器c，该服务器负责交换a和b的公网ip地址，之后a和b之间就可以使用对方的公网ip进行直接通信了。
技术上一般选用UDP协议实现（由于需要端口复用），而且第一次传输一般是被阻止的（"打洞"），后面就正常了。最后，这种穿透技术跟NAT（路由器等网络设备）的类型相关，不是一定成功的（特别是国内网络环境）。

### 二、POC代码分析
---------------------------------

服务器端：

```
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	printf("UDP NAT Hole Punching PoC Server.\n");
	if(argc != 2) {
		printf("Usage: ./hpserver <port>\n");
		exit(1);
	}

	int server_port = atoi(argv[1]);

	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);//服务器socket

	struct sockaddr_in server_addr, peers_addr[2];//服务器地址和客户端端点地址数组

	server_addr.sin_family = AF_INET;//创建服务器socket地址，ip+port
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero), 8);
	
	bind(server_socket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));//socket绑定地址
	

	// Wait for UDP datagrams from peers
	printf("Phase 1. Waiting for peers on port %d.\n", server_port);

	int addr_len = sizeof(struct sockaddr);
	
	int current_peer = 0;
	char recv_data[1024];

	while (current_peer < 2)
	{
		recvfrom(server_socket, &recv_data, 1024, 0, 
			(struct sockaddr*) &peers_addr[current_peer], &addr_len);//这里只关心客户端地址，不关心客户端传过来的数据

		printf("\tNew peer found: (%s, %d)\n", 
			inet_ntoa(peers_addr[current_peer].sin_addr), 
			ntohs(peers_addr[current_peer].sin_port)); 
		current_peer ++;
	}
	printf("End of Phase 1.\n\n");
	
	// Send NATed IP and Ports of peers to each other
	printf("Phase 2. Sending holes to peers.\n");

	printf("\tSending IP and Port of second peer to the first peer.\n");	
	sendto(server_socket, &peers_addr[0], sizeof(struct sockaddr_in), 0,
		(struct sockaddr*) &peers_addr[1], sizeof(struct sockaddr));//向客户端0发送客户端1的socket地址

	printf("\tSending IP and Port of first peer to the second peer.\n");
	sendto(server_socket, &peers_addr[1], sizeof(struct sockaddr_in), 0,
		(struct sockaddr*)&peers_addr[0], sizeof(struct sockaddr));//向客户端1发送客户端0的socket地址

	//上面的2句发送代码就是交换客户端的公网ip地址
	
	printf("End of Phase 2.\n\n");
	printf("Exiting.\n\n");
	
	return 0;
}

```

客户端代码：

```
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	printf("UDP NAT Hole Punching PoC Client.\n");
	
	if(argc != 4) {
		printf("Usage: ./hpclient <client_port> <rendezvous_server_ip> <rendezvous_server_port>\n");
		exit(1);
	}

	int client_port = atoi(argv[1]);
	char* rendezvous_server_ip = argv[2];
	int rendezvous_server_port = atoi(argv[3]);

	int sock = socket(AF_INET, SOCK_DGRAM, 0);//创建客户端socket

	struct sockaddr_in client_addr, rendezvous_server_addr;

	client_addr.sin_family = AF_INET;//创建客户端socket地址
	client_addr.sin_port = htons(client_port);
	client_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(client_addr.sin_zero), 8);

	bind(sock, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

	rendezvous_server_addr.sin_family = AF_INET;//创建中间服务器socket地址
	rendezvous_server_addr.sin_port = htons(rendezvous_server_port);
	inet_aton(rendezvous_server_ip, &rendezvous_server_addr.sin_addr.s_addr);
	bzero(&(rendezvous_server_addr.sin_zero), 8);
		

	// Send an empty UDP datagram to Rendezvous Server
	printf("Phase 1. Sending empty UDP to rendezvous server.\n");

	char* data = "";
	sendto(sock, data, strlen(data), 0,
		(struct sockaddr *)&rendezvous_server_addr, sizeof(struct sockaddr));//向中间服务器发送空字符串，本质是告诉服务器自己的ip地址
	printf("End of Phase 1.\n\n");


	// Receiving peer address from server
	printf("Phase 2. Receiving peer address from server.\n");
	
	int sockaddr_in_len = sizeof(struct sockaddr_in);
	char peer_bytes[sockaddr_in_len];

	int sockaddr_len = sizeof(struct sockaddr);
	recvfrom(sock, peer_bytes, sockaddr_in_len, 0,
		(struct sockaddr *)&rendezvous_server_addr, &sockaddr_len);//从服务器读取数据，其实等到的是另外一个客户端的ip地址

	struct sockaddr_in peer_addr;
	memcpy(&peer_addr, &peer_bytes, 16);//创建对面客户端的socket地址

	printf("\tReceived peer address: (%s, %d)\n", inet_ntoa(peer_addr.sin_addr),
		ntohs(peer_addr.sin_port));
	printf("End of Phase 2.\n\n");


	// Simple UDP chat between peers
	printf("Phase 3. Establishing communication with peer");

	while(1) {
		printf("\tType a message to send to another peer: ");
		char buffer[1024];
		fgets(buffer, 1024, stdin);//从终端命令行（标准输入）读取字符串
		
		sendto(sock, buffer, strlen(buffer) + 1, 0,
			(struct sockaddr* )&peer_addr, sizeof(struct sockaddr));//向对面客户端发送字符数据

		recvfrom(sock, buffer, 1024, 0,
			(struct sockaddr* )&peer_addr, &sockaddr_len);//等待并接受对面客户端数据

		printf("\tReceived from peer: %s\n", buffer);
	}
	
	//上面的while循环其实就是两个客户端之间通信了，与中间服务器无关
	//这之间涉及客户端的端口复用，因为客户端最开始是跟中间服务器通信的，后面是客户端之间进行通信的。虽然ip地址改变了，但是通信的端口是不变的。
	//对于比较安全的NAT设备，对通信的端口也进行严格控制，这种UDP穿透技术是失效的。
	//最后说一下为什么第一次数据传输会失败。因为一般的NAT设备是会丢弃没有源地址记录的数据包。什么意思？就是会丢弃来源不明的数据包。
	//NAT设备虽然丢弃这种来源不明的数据包，但是会在NAT设备记录该次通信，也就是会记录该数据包的ip地址。下一次如果数据包是该ip地址的NAT设备则会认为是有效数据包。

	return 0;
}


```

上面的代码在国外朋友的机器上是穿透成功的，在国内网络环境下还没成功。大家知道原理就可以了，有条件的可以试一下。


### 三、随便说点
---------------------------------

1、UDP穿透应该是利用的网络协议和NAT设备的设计缺陷实现的P2P技术
2、有了这种技术，很多平常公网不能实现的应用现在可以做起来，比如：视频聊天，远程桌面等
3、socket通信了解一下


>END

[代码下载](documents/NAT.zip)

