#include "examples/simple/echo/echo.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"

#include <unistd.h>

// using namespace muduo;
// using namespace muduo::net;

int main()
{
  // 记录一条日志	
  LOG_INFO << "pid = " << getpid();

  // 新建一个事件循环
  muduo::net::EventLoop loop;

  // 构建一个网络地址
  muduo::net::InetAddress listenAddr(2007);
  // 构建一个服务器
  EchoServer server(&loop, listenAddr);

  // 启动服务器
  server.start();

  // 开启事件循环
  loop.loop();
}

