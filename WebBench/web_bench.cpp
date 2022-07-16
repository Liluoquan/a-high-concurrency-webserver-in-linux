#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>
#include <rpc/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <iostream>
using std::cout;
using std::endl;

//http请求方法
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define WEBBENCH_VERSION "v1.5"
//请求大小
#define REQUEST_SIZE 2048  

static int force = 0;             //默认需要等待服务器响应
static int force_reload = 0;      //默认不重新发送请求
static int clients_num = 1;       //默认客户端数量
static int request_time = 30;     //默认模拟请求时间
static int http_version = 2;      //默认http协议版本 0:http/0.9, 1:http/1.0, 2:http/1.1
static char* proxy_host = NULL;   //默认无代理服务器
static int port = 80;             //默认访问80端口 
static int is_keep_alive = 0;     //默认不支持keep alive

static int request_method = METHOD_GET;  //默认请求方法
static int pipeline[2];                  //用于父子进程通信的管道
static char host[MAXHOSTNAMELEN];        //存放目标服务器的网络地址
static char request_buf[REQUEST_SIZE];   //存放请求报文的字节流

static int success_count = 0;    //请求成功的次数
static int failed_count = 0;     //失败的次数
static int total_bytes = 0;     //服务器响应总字节数

volatile bool is_expired = false;    //子进程访问服务器 是否超时

static void Usage() {
	fprintf(stderr,
		"Usage: webbench [option]... URL\n"
		"  -f|--force               不等待服务器响应\n"
		"  -r|--reload              重新请求加载(无缓存)\n"
		"  -9|--http09              使用http0.9协议来构造请求\n" 
		"  -1|--http10              使用http1.0协议来构造请求\n" 
		"  -2|--http11              使用http1.1协议来构造请求\n" 
		"  -k|--keep_alive          客户端是否支持keep alive\n"
		"  -t|--time <sec>          运行时间，单位：秒，默认为30秒\n"
		"  -c|--clients_num <n>     创建多少个客户端，默认为1个\n" 
		"  -p|--proxy <server:port> 使用代理服务器发送请求\n" 
		"  --get                    使用GET请求方法\n" 
		"  --head                   使用HEAD请求方法\n" 
		"  --options                使用OPTIONS请求方法\n" 
		"  --trace                  使用TRACE请求方法\n" 
		"  -?|-h|--help             显示帮助信息\n" 
		"  -V|--version             显示版本信息\n"
	);
}

//构造长选项与短选项的对应
static const struct option OPTIONS[] = {
	{"force",   	no_argument,      &force,               1},
	{"reload",  	no_argument,      &force_reload,        1},
	{"http09",  	no_argument,      NULL,               '9'},
	{"http10",  	no_argument,      NULL,               '1'},
	{"http11",  	no_argument,      NULL,               '2'},
	{"keep_alive",  no_argument,      &is_keep_alive,       1},
	{"time",    	required_argument,NULL,               't'},
	{"clients", 	required_argument,NULL,               'c'},
	{"proxy",   	required_argument,NULL,               'p'},
	{"get",     	no_argument,      &request_method,     METHOD_GET},
	{"head",    	no_argument,      &request_method,    METHOD_HEAD},
	{"options", 	no_argument,      &request_method, METHOD_OPTIONS},
	{"trace",   	no_argument,      &request_method,   METHOD_TRACE},
	{"help",    	no_argument,      NULL,               '?'},
	{"version", 	no_argument,      NULL,               'V'},
	{NULL,      	0,                NULL,                 0}
};


//打印消息
static void PrintMessage(const char* url) {
	printf("===================================开始压测===================================\n");
	switch (request_method) {
		case METHOD_GET:
			printf("GET");
			break;
		case METHOD_OPTIONS:
			printf("OPTIONS");
			break;
		case METHOD_HEAD:
			printf("HEAD");
			break;
		case METHOD_TRACE:
			printf("TRACE");
			break;
		default:
			printf("GET");
			break;
	}
	printf(" %s", url);
	switch (http_version) {
		case 0: 
			printf(" (Using HTTP/0.9)");
			break;
		case 1:
			printf(" (Using HTTP/1.0)");
			break;
		case 2: 
			printf(" (Using HTTP/1.1)");
			break;
	}
	printf("\n客户端数量 %d, 每个进程运行 %d秒", clients_num, request_time);

	if (force) {
		printf(", 选择提前关闭连接");
	}
	if (proxy_host != NULL) {
		printf(", 经由代理服务器 %s:%d", proxy_host, port);
	}
	if (force_reload) {
		printf(", 选择无缓存");
	}
	printf("\n");
}

static int ConnectServer(const char* server_host, int server_port) {
	// 协议的类型:IPv4协议 网络数据类型:字节流 网络协议:TCP协议
	int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sockfd < 0) {
		return client_sockfd;
	}
	struct sockaddr_in server_addr;
	struct hostent* host_name;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	//设置服务器ip
	unsigned long ip = inet_addr(server_host);
	if (ip != INADDR_NONE) {
		memcpy(&server_addr.sin_addr, &ip, sizeof(ip));
	} else {
		host_name = gethostbyname(server_host);
		if (host_name == NULL) {
			return -1;
		}
		memcpy(&server_addr.sin_addr, host_name->h_addr, host_name->h_length);
	}
	//设置服务器端口
	server_addr.sin_port = htons(server_port);

	//tcp三次握手建立连接
	if (connect(client_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        //修复套接字泄漏
        close(client_sockfd);
		return -1;
	}

	return client_sockfd;
}


//闹钟信号处理函数
static void AlarmHandler(int signal) {
	is_expired = true;
}	

//子进程创建客户端去请求服务器
static void Worker(const char* server_host, const int server_port, const char* request) {
	int client_sockfd = -1;
	int request_size = strlen(request);
	const int response_size = 1500;
	char response_buf[response_size];

	//设置闹钟信号处理函数
	struct sigaction signal_action;
	signal_action.sa_handler = AlarmHandler;
	signal_action.sa_flags = 0;
	if (sigaction(SIGALRM, &signal_action, NULL)) {
		exit(1);
	}
	alarm(request_time);

    if (is_keep_alive) {
        //一直到成功建立连接才退出while
        while (client_sockfd == -1) {
            client_sockfd = ConnectServer(host, port);
        }
        //cout << "1. 建立连接（TCP三次握手）成功!" << endl;
keep_alive:
        while(true) {
            if (is_expired) {
                //到收到闹钟信号会使is_expired为true 此时子进程工作应该结束了(每个子进程都有一个is_expired)
                if (failed_count > 0) {
                    failed_count--;
                }
                return;
            }

            if (client_sockfd < 0) { 
                failed_count++; 
                continue;
            } 

            //向服务器发送请求报文
            if (request_size != write(client_sockfd, request, request_size)) {
                cout << "2. 发送请求报文失败: " << strerror(errno) << endl;
                failed_count++; 
                close(client_sockfd);
                //发送请求失败 要重新创建套接字
                client_sockfd = -1;
                while (client_sockfd == -1) {
                    client_sockfd = ConnectServer(host, port);
                }
                continue;
            }
            //cout << "2. 发送请求报文成功!" << endl;

            //没有设置force时 默认等到服务器的回复
            if (force == 0)  {
                //keep-alive一个进程只创建一个套接字 收发数据都用这个套接字
                //读服务器返回的响应报文到response_buf中
                while (true) {
                    if (is_expired) {
                        break;
                    } 
                    int read_bytes = read(client_sockfd, response_buf, response_size);
                    if (read_bytes < 0) { 
                        //cout << "3. 接收响应报文失败: " << strerror(errno) << endl;
                        failed_count++;
                        close(client_sockfd);
                        //读取响应失败 不用重新创套接字 重新发一次请求即可
                        goto keep_alive;  
                    } else {
                        //cout << "3. 接收响应报文成功！" << endl;
                        total_bytes += read_bytes;
                        break;
                    }
                }
            }
            success_count++;
            //cout << "当前成功：" << success_count << ", 失败：" << failed_count << ", 总字节：" << total_bytes << endl; 
        }
    } else {
not_keep_alive:
        while(true) {
            if (is_expired) {
                //到收到闹钟信号会使is_expired为true 此时子进程工作应该结束了(每个子进程都有一个is_expired)
                if (failed_count > 0) {
                    failed_count--;
                }
                return;
            }

            //与服务器建立连接
            client_sockfd = ConnectServer(host, port);                          
            if (client_sockfd < 0) { 
                //cout << "1. 建立连接失败: " << strerror(errno) << endl; 
                failed_count++; 
                continue;
            } 
            //cout << "1. 建立连接（TCP三次握手）成功!" << endl;

            //向服务器发送请求报文
            if (request_size != write(client_sockfd, request, request_size)) {
                cout << "2. 发送请求报文失败: " << strerror(errno) << endl;
                failed_count++; 
                close(client_sockfd);
                continue;
            }
            //cout << "2. 发送请求报文成功!" << endl;

            //http0.9特殊处理 
            if (http_version == 0) {
                if (shutdown(client_sockfd, 1)) { 
                    failed_count++;
                    close(client_sockfd);
                    continue;
                }
            }

            //没有设置force时 默认等到服务器的回复
            if (force == 0)  {
                //not-keey-alive 每次发送完请求，读取了响应后就关闭套接字
                //下次创建新套接字 发送请求 读取响应
                while (true) {
                    if (is_expired) {
                        break;
                    } 
                    int read_bytes = read(client_sockfd, response_buf, response_size);
                    if (read_bytes < 0) { 
                        //cout << "3. 接收响应报文失败: " << strerror(errno) << endl;
                        failed_count++;
                        close(client_sockfd);
                        //这里读失败想退出来继续创建套接字 去请求服务器,因为有两层循环 所以用goto
                        goto not_keep_alive;  
                    } else {
                        //cout << "3. 接收响应报文成功！" << endl;
                        total_bytes += read_bytes;
                        break;
                    }
                }
            }
            
            //一次发送 接收完成后 关闭套接字
            if (close(client_sockfd)) {
                cout << "4. 关闭套接字失败: " << strerror(errno) << endl;
                failed_count++;
                continue;
            }
            success_count++;
            //cout << "当前成功：" << success_count << ", 失败：" << failed_count << ", 总字节：" << total_bytes << endl; 
        }
    }
}

//命令行解析
void ParseArg(int argc, char* argv[]) {
	int opt = 0;
	int options_index = 0;

	//没有输入选项
	if (argc == 1) {
		Usage();
		exit(1);
	} 

	//一个一个解析输入选项
	while ((opt = getopt_long(argc, argv, "fr912kt:c:p:Vh?",
								OPTIONS, &options_index)) != EOF) {
		switch (opt) {
			case 'f': 
				force = 1;
				break;
			case 'r': 
				force_reload = 1;
				break; 
			case '9': 
				http_version = 0;
				break;
			case '1': 
				http_version = 1;
				break;
			case '2': 
				http_version = 2;
				break;
			case 'k':
				is_keep_alive = 1;
				break;
			case 't': 
				request_time = atoi(optarg);
				break;	
			case 'c': 
				clients_num = atoi(optarg);
				break;
			case 'p': {
				//使用代理服务器 设置其代理网络号和端口号
				char* proxy = strrchr(optarg, ':');
				proxy_host = optarg;
				if (proxy == NULL) {
					break;
				}
				if (proxy == optarg) {
					fprintf(stderr,"选项参数错误, 代理服务器%s: 缺少主机名\n", optarg);
					exit(1);
				}
				if (proxy == optarg + strlen(optarg) - 1) {
					fprintf(stderr,"选项参数错误, 代理服务器%s: 缺少端口号\n", optarg);
					exit(1);
				}
				*proxy = '\0';
				port = atoi(proxy + 1);
				break;
			}
			case '?':
			case 'h': 
				Usage();
				exit(0);
			case 'V': 
				printf("WebBench %s\n", WEBBENCH_VERSION);
				exit(0);
			default:
				Usage();
				exit(1);
		}
	}

	//没有输入URL
	if (optind == argc) {
		fprintf(stderr,"WebBench: 缺少URL参数!\n");
		Usage();
		exit(1);
	}

	//如果没有设置客户端数量和连接时间, 则设置默认
	if (clients_num == 0) {
		clients_num = 1;
	}
	if (request_time == 0) {
		request_time = 30;
	}
}

//构造请求
void BuildRequest(const char* url) {
	bzero(host, MAXHOSTNAMELEN);
	bzero(request_buf, REQUEST_SIZE);

	//无缓存和代理都要在http1.0以上才能使用
	if (force_reload && proxy_host != NULL && http_version < 1) {
		http_version = 1;
	}
	if (request_method == METHOD_HEAD && http_version < 1) {
		http_version = 1;
	}
	//OPTIONS和TRACE要1.1
	if (request_method == METHOD_OPTIONS && http_version < 2) {
		http_version = 2;
	}
	if (request_method == METHOD_TRACE && http_version < 2) {
		http_version = 2;
	}

	//http请求行 的请求方法
	switch (request_method) {
		case METHOD_GET: 
			strcpy(request_buf, "GET");
			break;
		case METHOD_HEAD: 
			strcpy(request_buf, "HEAD");
			break;
		case METHOD_OPTIONS: 
			strcpy(request_buf, "OPTIONS");
			break;
		case METHOD_TRACE: 
			strcpy(request_buf, "TRACE");
			break;
		default:
			strcpy(request_buf, "GET");
			break;
	}
	strcat(request_buf, " ");

	//判断url是否有效
	if (NULL == strstr(url, "://")) {
		fprintf(stderr, "\n%s: 无效的URL\n", url);
		exit(2);
	}
	if (strlen(url) > 1500) {
		fprintf(stderr,"URL长度过长\n");
		exit(2);
	}
	if (proxy_host == NULL) {
		//忽略大小写比较前7位
		if (0 != strncasecmp("http://", url, 7)) {
			fprintf(stderr,"\n无法解析URL(只支持HTTP协议, 暂不支持HTTPS协议)\n");
			exit(2);
		}
	}
	//定位url中主机名开始的位置
	int i = strstr(url, "://") - url + 3;
	//在主机名开始的位置是否有/ 没有则无效
	if (strchr(url + i, '/') == NULL) {
		fprintf(stderr,"\n无效的URL,主机名没有以'/'结尾\n");
		exit(2);
	}

	//填写请求url 无代理时
	if (proxy_host == NULL) {
		//有端口号 填写端口号
		if (index(url + i, ':') != NULL 
			&& index(url + i, ':') < index(url + i, '/')) {
			//设置域名或IP
			strncpy(host, url + i, strchr(url + i, ':') - url - i);
			char port_str[10];
			bzero(port_str, 10);
			strncpy(port_str, index(url + i, ':') + 1, strchr(url + i, '/') - index(url + i, ':') -1);
			//设置端口
			port = atoi(port_str);
			//避免写了: 却没写端口号
			if (port == 0) {
				port = 80;
			}
		} else {
			strncpy(host, url + i, strcspn(url + i, "/"));
		}
		strcat(request_buf + strlen(request_buf), url + i + strcspn(url + i, "/"));
	} else {
		//有代理服务器 直接填就行
		strcat(request_buf, url);
	}

	//填写http协议版本
	if (http_version == 0) {
		strcat(request_buf, " HTTP/0.9");
	} else if (http_version == 1) {
		strcat(request_buf, " HTTP/1.0");
	} else if (http_version == 2) {
		strcat(request_buf, " HTTP/1.1");
	}
	strcat(request_buf, "\r\n");

	//请求头部
	if (http_version > 0) {
		strcat(request_buf, "User-Agent: WebBench " WEBBENCH_VERSION "\r\n");
	}
	//域名或IP字段
	if (proxy_host == NULL && http_version > 0) {
		strcat(request_buf, "Host: ");
		strcat(request_buf, host);
		strcat(request_buf, "\r\n");
	}
	//强制重新加载, 无缓存字段
	if (force_reload && proxy_host != NULL) {
		strcat(request_buf, "Pragma: no-cache\r\n");
	}
	//keep alive
	if (http_version > 1) {
		strcat(request_buf, "Connection: ");
		if (is_keep_alive) {
			strcat(request_buf, "keep-alive\r\n");
		} else {
			strcat(request_buf, "close\r\n");
		}
	}
	//添加空行
	if (http_version > 0) {
		strcat(request_buf, "\r\n"); 
	}

	printf("Requset: %s\n", request_buf);
	PrintMessage(url);
}

//父进程的作用: 创建子进程 读子进程测试到的数据 然后处理
void WebBench() {
	pid_t pid = 0;
	FILE* pipe_fd = NULL;
	int req_succ, req_fail, resp_bytes;

	//尝试建立连接一次
	int sockfd = ConnectServer(proxy_host == NULL ? host : proxy_host, port);
	if (sockfd < 0) { 
		fprintf(stderr,"\n连接服务器失败, 中断压力测试\n");
		exit(1);
	}
	close(sockfd);
	//创建父子进程通信的管道
	if (pipe(pipeline)) {
		perror("通信管道建立失败");
		exit(1);
	}

	int proc_count;
	//让子进程取测试 建立子进程数量由clients_num决定
	for (int proc_count = 0; proc_count < clients_num; proc_count++) {
		//创建子进程
		pid = fork();
		//<0是失败, =0是子进程, 这两种情况都结束循环 =0时子进程可能继续fork
		if (pid <= 0) {
			sleep(1); 
			break;
		}
	}

	//fork失败
	if (pid < 0) {
		fprintf(stderr, "第%d个子进程创建失败\n", proc_count);
		perror("创建子进程失败");
		exit(1);
	}

	//子进程处理
	if (pid == 0) {
		//子进程 创建客户端连接服务端 并发出请求报文
		Worker(proxy_host == NULL ? host : proxy_host, port, request_buf);
		pipe_fd = fdopen(pipeline[1], "w");
		if (pipe_fd == NULL) {
			perror("管道写端打开失败");
			exit(1);
		}
		//进程运行完后，向管道写端写入该子进程在这段时间内请求成功的次数 失败的次数 读取到的服务器响应总字节数
		fprintf(pipe_fd, "%d %d %d\n", success_count, failed_count, total_bytes);
		//关闭管道写端
		fclose(pipe_fd);

		exit(1);
	} else {
		// 父进程处理
		pipe_fd = fdopen(pipeline[0], "r");
		if (pipe_fd == NULL) {
			perror("管道读端打开失败");
			exit(1);
		}
		//因为我们要求数据要及时 所以没有缓冲区是最合适的 
		setvbuf(pipe_fd, NULL, _IONBF, 0);
		success_count = 0;
		failed_count = 0;
		total_bytes = 0;

		while (true) {
			//从管道读端读取数据
			pid = fscanf(pipe_fd, "%d %d %d", &req_succ, &req_fail, &resp_bytes);
			if (pid < 2) {
				fprintf(stderr, "某个子进程异常\n");
				break;
			}
			success_count += req_succ;
			failed_count += req_fail;
			total_bytes += resp_bytes;
			//我们创建了clients_num个子进程 所以要读clients_num多次数据
			if (--clients_num == 0) {
				break;
			}
		}
		fclose(pipe_fd);

		printf("\n速度: %d 请求/秒, %d 字节/秒.\n请求: %d 成功, %d 失败\n",
				(int)((success_count + failed_count) / (float)request_time),
				(int)(total_bytes / (float)request_time),
				success_count, failed_count);
	}
}

