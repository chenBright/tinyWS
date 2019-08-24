# tinyWS
A C++ Tiny Web Server

## 已完成

- 完成基本的Tcp（被动连接）库
- 简易的HTTP服务器，可访问主页HTML和网站favicon图标

## TODO

- Channel tie，用于延长 Channel 的生命周期
- 解决代码中的TODO
- 使用定时器（如超时处理）
- 日志
- ~~HTTP服务器~~，完善HTTP服务器，使其能像Nginx一样，通过简单的配置，作为静态服务器运行
- http超时处理，并使用时间轮盘管理定时器
- BlockingQueue
- 编写文档，解释核心原理
- 单元测试
- 构建
- 压测
