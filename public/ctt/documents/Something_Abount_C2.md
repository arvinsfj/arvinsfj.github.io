
# C的线程、网络和SQLite

> *作者：Arvin 日期：2018年6月1日*

---------------------------------

>BEGIN

如题，记录C的一些实用知识。使用类Unix平台。

### 一、思路
---------------------------------

使用gcc进行编译，在类Unix平台进行测试。主要测试C中多线程、HTTP请求和SQLite操作。线程使用tinycthread库，HTTP使用curl库，SQLite使用C的API。学会了这三样，应该就可以使用C做一些有用的功能。

### 二、线程
---------------------------------

代码如下：

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinycthread.h"

//
static char queue = 0;
static char cond = 0;

static thrd_t thrd1;
static thrd_t thrd2;
static thrd_t thrd3;
static mtx_t mtx;
static cnd_t cnd;
static mtx_t mtx_cnd;

//
void print_cond2()
{
	mtx_lock(&mtx_cnd);
	while(cond != 2) {
		cnd_wait(&cnd, &mtx_cnd);
	}
	mtx_unlock(&mtx_cnd);
	printf("%s\n", "hello, cond 2!");
}

//
int worker_thread(void* argv)
{
	mtx_lock(&mtx);
	if (!strcmp("thd1", (char*)argv)){
		queue += 1;
	}
	if (!strcmp("thd2", (char*)argv)){
		queue += 2;
		mtx_lock(&mtx_cnd);
		cond = 2; // 条件成立
        cnd_signal(&cnd); // 发送条件成立信号
        mtx_unlock(&mtx_cnd);
	}
	if (!strcmp("thd3", (char*)argv)){
		queue += 3;
	}
	printf("%s: %d\n", (char*)argv, queue);
	mtx_unlock(&mtx);
	return 0;
}

//
void thrd_init()
{
	queue = 0;
	cond = 0;
	//
	mtx_init(&mtx, mtx_plain);
	//
	mtx_init(&mtx_cnd, mtx_plain);
	cnd_init(&cnd);
	//
	if (thrd_create(&thrd1, worker_thread, "thd1") != thrd_success) {
		perror("thrd1_create");
		exit(1);
	}
	if (thrd_create(&thrd2, worker_thread, "thd2") != thrd_success) {
		perror("thrd2_create");
		exit(1);
	}
	if (thrd_create(&thrd3, worker_thread, "thd3") != thrd_success) {
		perror("thrd3_create");
		exit(1);
	}
	//
	print_cond2(); // 主线程 等待条件达成 cond == 2
}

void thrd_stop()
{
	queue = 0;	
	cond = 0;
}

int main(int argc, char** argv)
{
	thrd_init();
	sleep(1);
	thrd_stop();
	return 0;
}

```

### 四、网络
---------------------------------

socket网络代码如下：

```
#include <netdb.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinycthread.h"

#define QUEUE_SIZE 1048576
#define RECV_SIZE 4096

//
static int sd = 0;
static char *queue = 0;
static int qsize = 0;

static thrd_t recv_thread;
static mtx_t mutex;

// 发送任意长度的字节数据
int sendall(int sd, char* data, int length)
{
	int count = 0, n = 0;
	while (count < length) {
		n = send(sd, data + count, length, 0);
		if (n == -1) return -1;
		count += n;
		length -= n;
	}
	return 0;
}

// 发送任意长度的字符串数据
void sendstr(char* data)
{
	if (sendall(sd, data, strlen(data)) == -1) {
		perror("sendstr");
		exit(1);
	}
}

// 自定义网络协议，测试
void send_version(int version) 
{
    char buffer[1024];
    snprintf(buffer, 1024, "V,%d\n", version);
    sendstr(buffer);
}

// ......

// 管道读数据：从多线程共享内存中获取数据（涉及内存区域的线程互斥）
char* recvqueue()
{
	char* result = 0;
	mtx_lock(&mutex);
	//互斥区域
	char* p = queue + qsize - 1;
	while (p >= queue && *p != '\n') p--; // 在queue中从后向前扫描字符'\n'
	if (p >= queue) {
		int length = p - queue + 1; // 字符串长度
		result = malloc(length + 1); // 多加1个字符存储字符串结束符'\0'
		memcpy(result, queue, length);
		result[length] = '\0';
		int remain = qsize - length;
		memmove(queue, p + 1, remain);
		qsize -= length;
	}
	mtx_unlock(&mutex);
	return result;
}

// 管道写数据：socket读数据线程向多线程共享内存中写数据（涉及内存区域的线程互斥）
int sendqueue(void* argv)
{
	char* data = malloc(RECV_SIZE);
	int length = 0;
	while (1) {
		if ((length = recv(sd, data, RECV_SIZE - 1, 0)) > 0) {
			data[length] = '\0';
			while (1) {
				int done = 0;
				mtx_lock(&mutex);
				//互斥区域
				if (qsize + length < QUEUE_SIZE) {
					memcpy(queue + qsize, data, length + 1);
					qsize += length;
					done = 1;
				}
				mtx_unlock(&mutex);
				if (done) {
					break;
				}
				sleep(0);
			}
		}
	}
	free(data);
	return 0;
}

// socket接受数据子线程客户端
void recvnet()
{
	queue = malloc(QUEUE_SIZE); // 分配线程共享区域内存
	qsize = 0;
	mtx_init(&mutex, mtx_plain); // 初始化互斥变量

	// 创建socket接受数据子线程
	if (thrd_create(&recv_thread, sendqueue, 0) != thrd_success) {
		perror("recvnet");
		exit(1);
	}
}

// socket创建和连接客户端
void connectnet(char* hostname, int port)
{
	struct hostent* host;
	struct sockaddr_in address;
	if ((host = gethostbyname(hostname)) == 0) {
		perror("gethostbyname");
		exit(1);
	}
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ((struct in_addr *)(host->h_addr_list[0]))->s_addr;
	address.sin_port = htons(port);
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (connect(sd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		perror("connect");
        exit(1);
	}
}

// 客户端启动
void client_start(char* hostname, int port)
{
	connectnet(hostname, port);
	recvnet();
}

// 客户端关闭
void client_close()
{
	close(sd);
	qsize = 0;
	free(queue);
}

int main(int argc, char** argv)
{
	if (argc != 3) {
		return 0;
	}
	client_start(argv[1], atoi(argv[2]));
	// 操作区域 repl
	char cmd[128] = {0}, *str = 0;
	int done = 1, length = 0;
	while (done) {
		str = gets(cmd);
		if (str && !strcmp("quit", cmd)){
			done = 0;
		} else {
			strncat(str, "\r\n\r\n", 4);
			printf("%c", '\n');
			sendstr(str);
			sleep(3);
			printf("%s\n", recvqueue());
		}
	} 
	client_close();
	return 0;
}

```

http网络代码如下：

```
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSE_LENGTH 1024*1024

size_t write_function(char *data, size_t size, size_t count, void *resp) {
    size_t length = size * count;
    char *dst = (char *)resp;
    char *src = malloc(length + 1);
    memcpy(src, data, length);
    src[length] = '\0';
    strncat(dst, src, MAX_RESPONSE_LENGTH - strlen(dst) - 1);
    free(src);
    return length;
}

int main(int argc, char** argv)
{
	static char* url = "https://www.baidu.com/";
	CURL* curl = curl_easy_init();
	if (curl){
		long http_code = 0;
		char response[MAX_RESPONSE_LENGTH] = {0};
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
		CURLcode code = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);
        if (code == CURLE_OK && http_code == 200) {
            printf("status: %s\n\n", "OK!");
            printf("%s\n", response);
        } else {
        	printf("status: %s\n\n", "Fail!");
        }
	}

	return 0;
}


```

### 五、SQLite数据库
---------------------------------

代码如下：

```
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "sqlite3.h"
#include "tinycthread.h"

//

static sqlite3 *db;
static sqlite3_stmt *get_key_stmt;
static sqlite3_stmt *set_key_stmt;

static thrd_t thrd;
static mtx_t mtx;
static cnd_t cnd;
static char cond = -1;

//
static int pg = 0, qg = 0, keyg = 0;

// 初始化数据库
int db_init(char *path) 
{
    static const char *create_query =
        "create table if not exists key ("
        "    p int not null,"
        "    q int not null,"
        "    key int not null"
        ");";
    static const char *get_key_query =
        "select key from key where p = ? and q = ?;";
    static const char *set_key_query =
        "insert or replace into key (p, q, key) "
        "values (?, ?, ?);";
    int rc;
    rc = sqlite3_open(path, &db);
    if (rc) return rc;
    rc = sqlite3_exec(db, create_query, NULL, NULL, NULL);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, get_key_query, -1, &get_key_stmt, NULL);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, set_key_query, -1, &set_key_stmt, NULL);
    if (rc) return rc;
    sqlite3_exec(db, "begin;", NULL, NULL, NULL);
    return 0;
}

void db_close() 
{
    sqlite3_exec(db, "commit;", NULL, NULL, NULL);
    sqlite3_finalize(get_key_stmt);
    sqlite3_finalize(set_key_stmt);
    sqlite3_close(db);
}

void db_commit() 
{
    mtx_lock(&mtx);
    cond = 2;
    cnd_signal(&cnd);
    mtx_unlock(&mtx);
}

void _db_commit() {
    sqlite3_exec(db, "commit; begin;", NULL, NULL, NULL);
}

int db_get_key(int p, int q) 
{
    sqlite3_reset(get_key_stmt);
    sqlite3_bind_int(get_key_stmt, 1, p);
    sqlite3_bind_int(get_key_stmt, 2, q);
    if (sqlite3_step(get_key_stmt) == SQLITE_ROW) {
        return sqlite3_column_int(get_key_stmt, 0);
    }
    return 0;
}

void db_set_key(int p, int q, int key) 
{
    mtx_lock(&mtx);
    pg = p; qg = q; keyg = key;
    cond = 1;
    cnd_signal(&cnd);
    mtx_unlock(&mtx);
}

void _db_set_key(int p, int q, int key) 
{
    sqlite3_reset(set_key_stmt);
    sqlite3_bind_int(set_key_stmt, 1, p);
    sqlite3_bind_int(set_key_stmt, 2, q);
    sqlite3_bind_int(set_key_stmt, 3, key);
    sqlite3_step(set_key_stmt);
}

int db_worker_run(void *arg) 
{
    int running = 1;
    while (running) {
        mtx_lock(&mtx);
        while (cond == -1) {
            cnd_wait(&cnd, &mtx);
        }
        mtx_unlock(&mtx);
        switch (cond) {
            case 1:
                _db_set_key(pg, qg, keyg);
                break;
            case 2:
                _db_commit();
                break;
            case 3:
                running = 0;
                break;
        }
    }
    return 0;
}

void db_worker_start() 
{
	cond = -1;
    cnd_init(&cnd);
    mtx_init(&mtx, mtx_plain);
    if(thrd_create(&thrd, db_worker_run, NULL) != thrd_success) {
    	perror("thrd_create");
    	exit(1);
    }
}

void db_worker_stop() 
{
    mtx_lock(&mtx);
    cond = 3;
    cnd_signal(&cnd);
    mtx_unlock(&mtx);
    thrd_join(thrd, NULL);
    cnd_destroy(&cnd);
    mtx_destroy(&mtx);
}

int main(int argc, char** argv)
{
	db_init("./test.sqlite");
	db_worker_start();
	db_set_key(12,13,14);
	db_commit();
	sleep(3);
	printf("%d\n", db_get_key(12,13));
	db_worker_stop();
	db_close();
	return 0;
}


```


### 六、随便说点
---------------------------------

1. C代码的解释有时间再做，上面的代码已经够复杂了
2. 开发惯例，数据的读写（i/o）放到后台线程操作
3. 上面的代码使用到了：tinycthread、socket、curl、sqlite3等知识，相应的C库请自行下载
4. 测试平台用的是macOS High Sierra平台，全部可以执行
5. 后面添加lodepng库的使用....


>END

