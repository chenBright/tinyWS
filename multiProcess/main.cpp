#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>   // struct stat
#include <sys/mman.h>   // mmap()、munmap()

#include <functional>

#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"
#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

using namespace std::placeholders;
using namespace tinyWS_process;

void httpCallback(const HttpRequest& request, HttpResponse& response);
void set404NotFound(HttpResponse& response);

int main(int argc, char* argv[]) {
//     debug() << "pid = " << ::getpid() << ", tid = " << Thread::gettid() << std::endl;

    int processNum = 0;
    if (argc > 1) {
        processNum = ::atoi(argv[1]);
    }

    EventLoop loop;
    InternetAddress listenAddress(12315);
//    InternetAddress listenAddress(std::string("127.0.0.1"), 12315); // for pressure test
    HttpServer server(&loop, listenAddress, "tinyWS");

    server.setProcessNum(processNum);
    server.setHttpCallback(std::bind(&httpCallback, _1, _2));
    // 在 server::start() 函数调用之前，必须配置好跟子进程相关的设置。
    // 因为直到程序结束，子进程都不会从 start() 函数返回。
    server.start();
    loop.loop();

    return 0;
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
//    close(fd);
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
