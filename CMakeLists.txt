cmake_minimum_required(VERSION 3.10)
project(tinyWS)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(tinyWS_thread multiThread/main.cpp multiThread/net/Epoll.cpp multiThread/net/Epoll.h multiThread/net/EventLoop.cpp multiThread/net/EventLoop.h multiThread/net/Channel.cpp multiThread/net/Channel.h multiThread/base/noncopyable.h multiThread/base/Thread.cpp multiThread/base/Thread.h multiThread/base/ThreadPool.cpp multiThread/base/ThreadPool.h multiThread/base/MutexLock.h multiThread/base/Condition.h multiThread/net/Timer.cpp multiThread/net/Timer.h multiThread/net/TimerQueue.cpp multiThread/net/TimerQueue.h multiThread/net/EventLoopThread.cpp multiThread/net/EventLoopThread.h multiThread/net/EventLoopThreadPool.cpp multiThread/net/EventLoopThreadPool.h multiThread/base/Singleton.h multiThread/net/TimerId.h multiThread/net/Acceptor.cpp multiThread/net/Acceptor.h multiThread/net/InternetAddress.cpp multiThread/net/InternetAddress.h multiThread/net/Socket.cpp multiThread/net/Socket.h multiThread/net/TcpServer.cpp multiThread/net/TcpServer.h multiThread/net/TcpConnection.cpp multiThread/net/TcpConnection.h multiThread/net/Buffer.cpp multiThread/net/Buffer.h multiThread/net/CallBack.h multiThread/http/HttpServer.cpp multiThread/http/HttpServer.h multiThread/http/HttpRequest.cpp multiThread/http/HttpRequest.h multiThread/http/HttpResponse.cpp multiThread/http/HttpResponse.h multiThread/http/HttpContext.cpp multiThread/http/HttpContext.h multiThread/base/BlockingQueue.h multiThread/base/BoundedBlockingQueue.h multiThread/base/Atomic.h multiThread/base/Logger.cpp multiThread/base/Logger.h)

add_executable(tinyWS_process multiProcess/main.cpp)

target_link_libraries(tinyWS_thread ${CMAKE_THREAD_LIBS_INIT})
