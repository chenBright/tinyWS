#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>   // struct stat
#include <sys/mman.h>   // mmap()、munmap()

#include <functional>
#include <iostream>

#include "net/EventLoop.h"
#include "base/Thread.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"
#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "base/Logger.h"
#include "net/TimerId.h"

using namespace std::placeholders;
using namespace tinyWS_thread;

void test_runEvery();
void httpCallback(const HttpRequest &request, HttpResponse &response);
void set404NotFound(HttpResponse &response);

int main(int argc, char* argv[]) {
//     debug() << "pid = " << ::getpid() << ", tid = " << Thread::gettid() << std::endl;

    int threadNums = 0;
    int port = 19123;
    if (argc > 1) {
        threadNums = ::atoi(argv[1]);
    }
    if (argc > 2) {
        port = ::atoi(argv[2]);
    }

    EventLoop loop;
    InternetAddress listenAddress(port);
//    InternetAddress listenAddress(std::string("127.0.0.1"),8888); // for pressure test
    HttpServer server(&loop, listenAddress, "tinyWS");

    loop.runEvery(2 * 1000 * 1000, std::bind(&test_runEvery));

    server.setThreadNum(threadNums);
    server.start();
    server.setHttpCallback(std::bind(&httpCallback, _1, _2));
    loop.loop();

    return 0;
}

void test_runEvery() {
    std::cout << "test Timer" << std::endl;
}

void httpCallback(const HttpRequest& request, HttpResponse& response) {
//    debug() << "httpCallback() " << std::endl;
//
//    const std::string &path = request.path();
//    const std::string prefix = "/tmp/tmp.epZ6PWHYhj/web";
//    std::string filename;
//    if (path  == "/") {
//        filename = prefix + "/index.html";
//        response.setContentType("text/html");
//    } else if (path == "/favicon.ico") {
//        filename = prefix + "/favicon.ico";
//        response.setContentType("image/x-icon");
//    }
//
//    // 使用 mmap 读取文件
//    struct stat fileBuffer{};
//    int fd;
//    if (filename.empty() || stat(filename.c_str(), &fileBuffer) < 0
//        || (fd = ::open(filename.c_str(), O_RDONLY, 0)) < 0) {
//
//        set404NotFound(response);
//        return;
//    }
//
//    void *mmapResult = ::mmap(nullptr,
//            static_cast<size_t>(fileBuffer.st_size),
//            PROT_READ, MAP_PRIVATE, fd, 0);
//    close(fd); // 已经映射完成，可以关闭文件描述符。
//
//    if (mmapResult == (void*)-1) {
//        munmap(mmapResult, static_cast<size_t>(fileBuffer.st_size));
//        set404NotFound(response);
//        return;
//    }
//
//    char *srcAddress = static_cast<char*>(mmapResult);
//    // 添加数据到 Response Body
//    response.setBody(std::string(srcAddress, srcAddress + fileBuffer.st_size));
//    munmap(mmapResult, static_cast<size_t>(fileBuffer.st_size));

    response.setBody("Hello World!"); // for pressure test

//    std::cout << "Hello World!" << std::endl;

    response.setStatusCode(HttpResponse::k200OK);
    response.setStatusMessage("OK");
    response.addHeader("server", "tinyWS");
    response.addHeader("User-Agent", request.getHeader("User-Agent"));
}

void set404NotFound(HttpResponse& response) {
    response.setStatusCode(HttpResponse::k404NotFound);
    response.setStatusMessage("Not Found");
    response.setBody("Not Found");
    response.setCloseConnection(true);
}
