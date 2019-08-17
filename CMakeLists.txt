cmake_minimum_required(VERSION 3.12)
project(tinyWS)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(tinyWS main.cpp net/Epoll.cpp net/Epoll.h net/EventLoop.cpp net/EventLoop.h net/Channel.cpp net/Channel.h base/noncopyable.h base/Thread.cpp base/Thread.h base/ThreadPool.cpp base/ThreadPool.h base/MutexLock.h base/Condition.h net/Timer.cpp net/Timer.h net/TimerQueue.cpp net/TimerQueue.h net/EventLoopThread.cpp net/EventLoopThread.h net/EventLoopThreadPool.cpp net/EventLoopThreadPool.h base/Singleton.h net/TimerId.h net/Acceptor.cpp net/Acceptor.h net/InternetAddress.cpp net/InternetAddress.h net/Socket.cpp net/Socket.h net/TcpServer.cpp net/TcpServer.h net/TcpConnection.cpp net/TcpConnection.h net/Buffer.cpp net/Buffer.h net/CallBack.h http/HttpServer.cpp http/HttpServer.h http/HttpRequest.cpp http/HttpRequest.h http/HttpResponse.cpp http/HttpResponse.h http/HttpContext.cpp http/HttpContext.h)

target_link_libraries(tinyWS ${CMAKE_THREAD_LIBS_INIT})
