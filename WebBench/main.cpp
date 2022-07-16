#include "web_bench.h"

#include <getopt.h>

int main(int argc, char* argv[]) {
	//解析命令行参数
	ParseArg(argc, argv);

	const char* url = argv[optind];
	//构造http请求报文 
	BuildRequest(url);

	//开始执行压力测试
	WebBench();

	return 0;
}

