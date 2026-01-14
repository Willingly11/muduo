# My Muduo 复现计划

这是一个基于 C++11 重写的 Muduo 网络库核心功能项目。

## 📅 项目进度 (Project Progress)

### 第一阶段：基础组件 (Infrastructure)
- [x] **Noncopyable** (`noncopyable.h`) - 对象语义语义
- [ ] **Timestamp** (`Timestamp.h/cc`) - 时间戳类
- [ ] **Logger** (`Logger.h/cc`) - 简单的同步日志（或简易封装）
- [ ] **Thread** (`Thread.h/cc`) - 线程包装
- [ ] **CurrentThread** (`CurrentThread.h/cc`) - 线程局部数据 (TLD)

### 第二阶段：Reactor 模型 (Core)
- [x] **Channel** (`Channel.h/cc`) - 事件分发器 (核心)
- [x] **Poller** (`Poller.h/cc`) - I/O 复用基类
- [x] **EPollPoller** (`EPollPoller.h/cc`) - epoll 的封装
- [x] **EventLoop** (`EventLoop.h/cc`) - 事件循环 (核心)
- [x] **InetAddress** (`InetAddress.h/cc`) - 地址封装

### 测试一
- [x] **test_Channel_Epoll_EventLoop**(`test_Channel_Epoll_EventLoop.cpp`)

### 第三阶段：网络模块 (Network)
- [x] **Socket** (`Socket.h/cc`) - RAII 封装 socket fd
- [ ] **Acceptor** (`Acceptor.h/cc`) - 专门处理新连接
- [ ] **Buffer** (`Buffer.h/cc`) - 应用层缓冲区 (非常重要)
- [ ] **TcpConnection** (`TcpConnection.h/cc`) - 管理一条 TCP 连接

### 第四阶段：服务器与回调 (Server)
- [ ] **TcpServer** (`TcpServer.h/cc`) - 用户使用的服务器类
- [ ] **Callbacks** (`Callbacks.h`) - 回调函数类型定义
- [ ] **EventLoopThread** (`EventLoopThread.h/cc`) - 开启专门的 I/O 线程
- [ ] **EventLoopThreadPool** (`EventLoopThreadPool.h/cc`) - I/O 线程池

### 第五阶段：测试与应用 (Example)
- [ ] **EchoServer** - 回显服务器测试
- [ ] **HttpServer** (可选) - 简单的 HTTP 服务器支持