// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/TcpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
  : loop_(CHECK_NOTNULL(loop)),                                      // 事件循环
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),                                                  // 服务器名字
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), // 接收器
    threadPool_(new EventLoopThreadPool(loop, name_)),               // 线程池

    connectionCallback_(defaultConnectionCallback),                  // 已连接回调函数;  连接已经建立后；在新的ioLoop里执行

    messageCallback_(defaultMessageCallback),                        // 消息到达回调函数
    
    nextConnId_(1)                                                   // 下一个连接ID
{
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto& item : connections_)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();  //  shared_ptr的reset( )函数的作用是将引用计数减1，停止对指针的共享，除非引用计数为0，否则不会发生删除操作。；  https://blog.csdn.net/boiled_water123/article/details/101194033
    conn->getLoop()->runInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn)); /// 依次销毁
  }
}

/**
 * 设置线程数
 * @param numThreads [description]
 */
void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

/**
 * 启动服务器
 */
void TcpServer::start()
{
  if (started_.getAndSet(1) == 0)
  {
    /// 线程池启动
    threadPool_->start(threadInitCallback_);

    assert(!acceptor_->listening());
    loop_->runInLoop(
        std::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

/**
 * 新建连接
 * 当accept 新的已完成连接时候调用
 * @param sockfd   [description]
 * @param peerAddr [description]
 */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  // 是MainLoop
  loop_->assertInLoopThread();

  // 获取下一个ioLoop
  EventLoop* ioLoop = threadPool_->getNextLoop();

  
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  // 连接名字
  string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toIpPort();

  // 获取本地地址
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  // sockfd 是已连接套接字
  // peerAddr 是对端地址

  // 构建新连接
  // 
  // 这个新连接的loop是ioLoop
  // 
  // 
  TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
  // 服务器TcpServer 保存所有的连接
  connections_[connName] = conn;

  // 为连接设置回调函数
  
  //***********  sever 和 conn 绑定 ************** //
  //
  // 连接完成
  /**
   *
   *  connectionCallback_ 这个函数会在 connection的established里边调用
   *
   * 
   */
  conn->setConnectionCallback(connectionCallback_);



  // 接收消息
  /**
   * 当数据可读； 处理
   */
  conn->setMessageCallback(messageCallback_);

  // 写完成
  /**
   * 当输出缓冲区的数据，完全写完后，会回调
   */
  conn->setWriteCompleteCallback(writeCompleteCallback_);


  // 关闭， 内部使用
  // 
  // 
  // 连接关闭
  // 
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe


  // io循环里执行 connectEstablished	  
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}


/**
 * 删除连接
 * @param conn [description]
 */
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}


/**
 * 删除连接，线程安全
 * @param conn [description]
 */
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name()); // map里边剔除这个连接

  (void)n;
  assert(n == 1);

  /// 获取当前Loop
  EventLoop* ioLoop = conn->getLoop();

  /// 在Loop里
  ioLoop->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}

