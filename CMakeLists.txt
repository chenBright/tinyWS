cmake_minimum_required(VERSION 3.12)
project(tinyWS)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(tinyWS main.cpp Epoll.cpp Epoll.h EventLoop.cpp EventLoop.h Channel.cpp Channel.h base/noncopyable.h base/Thread.cpp base/Thread.h base/ThreadPool.cpp base/ThreadPool.h base/MutexLock.h base/Condition.h Timer.cpp Timer.h TimerQueue.cpp TimerQueue.h EventLoopThread.cpp EventLoopThread.h EventLoopThreadPool.cpp EventLoopThreadPool.h base/Singleton.h TimerId.h Acceptor.cpp Acceptor.h InternetAddress.cpp InternetAddress.h Socket.cpp Socket.h TcpServer.cpp TcpServer.h TcpConnection.cpp TcpConnection.h Buffer.cpp Buffer.h CallBack.h)

target_link_libraries(tinyWS ${CMAKE_THREAD_LIBS_INIT})
