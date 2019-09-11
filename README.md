# tinyWS
A C++ Tiny Web Server

## 已完成

- 完成基本的Tcp（被动连接）库
- 简易的HTTP服务器，可访问主页HTML和网站favicon图标

## TODO

- 为了获取 TimeType 而暴露了 Timer（内部对象），应调整结构，隐藏 Timer 对象
- http超时处理，并使用时间轮盘管理定时器
- 编写文档，解释核心原理
- 构建
- 压测
- 解决代码中的TODO
- Preactor模式、actor模式和reactor模式等模式的区别

## 参考

- [陈硕的Blog](http://www.cppblog.com/solstice/)
- [开源HTTP解析器](https://www.cnblogs.com/arnoldlu/p/6497837.html)
- [HTTP请求和响应格式](https://www.cnblogs.com/yaozhongxiao/archive/2013/03/02/2940252.html)
- [写一个并发http服务器](https://zhuanlan.zhihu.com/p/23336565)（有代码）
