// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;

/*
  这是客户端使用的连接器


  
 */
class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:

  // 类型：新连接到来回调函数
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  /**
   * 构造函数
   */
  Connector(EventLoop* loop, const InetAddress& serverAddr);
  /**
   * 析构函数
   */
  ~Connector();

  /**
   * 设置新连接到来回调函数
   */
  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  /**
   * 启动
   */
  void start();    // can be called in any thread
  /**
   * 重新启动
   */
  void restart();  // must be called in loop thread
  /**
   * 停止
   */
  void stop();     // can be called in any thread

  /**
   * 返回服务器地址
   * @return [description]
   */
  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  //3种装： 已经关闭连结，连接中，已完成连接
  enum States { kDisconnected, kConnecting, kConnected };

  // 最大重试
  static const int kMaxRetryDelayMs = 30*1000;
  // 初始化重试秒数
  static const int kInitRetryDelayMs = 500;

  /**
   * 设置状态
   */
  void setState(States s) { state_ = s; }

  /**
   * 开始事件循环
   */
  void startInLoop();
  /**
   * 停止事件循环
   */
  void stopInLoop();

  /**
   * 去连接
   */
  void connect();
  /**
   * 连接中
   */
  void connecting(int sockfd);
  /**
   * 处理写
   */
  void handleWrite();
  /**
   * 处理错误
   */
  void handleError();
  /*
    重试
   */
  void retry(int sockfd);
  /*
    删除并重置Channel
  */
  int removeAndResetChannel();
  /*
    重置Channel
   */
  void resetChannel();

  // 事件循环
  EventLoop* loop_;
  // 服务器地址
  InetAddress serverAddr_;
  // 是否已经连接
  bool connect_; // atomic
  // 状态
  States state_;  // FIXME: use atomic variable
  // channle
  std::unique_ptr<Channel> channel_;

  // 新连接到来-回调函数
  NewConnectionCallback newConnectionCallback_;
  // 重试微妙
  int retryDelayMs_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CONNECTOR_H
