// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/StringPiece.h"
#include "muduo/base/Types.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"

#include <memory>

#include <boost/any.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info; // 各个字段解释说明: https://blog.csdn.net/dyingfair/article/details/95855952

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> // https://blog.csdn.net/caoshangpa/article/details/79392878; 为了共享对象
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,            /// 属于哪个事件循环
                const string& name,         /// 名字
                int sockfd,                 /// socket FD
                const InetAddress& localAddr,   /// 本地地址
                const InetAddress& peerAddr);   /// 远端地址
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }              // 是否已经连接
  bool disconnected() const { return state_ == kDisconnected; }        // 是否已经关闭连接


  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  string getTcpInfoString() const;


  /// ******** 发送数据 ******* ///
  // void send(string&& message); // C++11
  void send(const void* message, int len);
  void send(const StringPiece& message);

  // void send(Buffer&& message); // C++11
  void send(Buffer* message);  // this one will swap data


  /// ******** 关闭 ********** ///
  void shutdown(); // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
  // 
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);


  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop


  /// *********** 设置上下文 *********** ///
  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }


  /// ************** 4 个回调函数 ***************** ///
  /// 设置回调
  void setConnectionCallback(const ConnectionCallback& cb)  /// 连接建立
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)   /// 消息到达
  { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)   /// 写完成
  { writeCompleteCallback_ = cb; }

  /// 设置高水位标记回调
  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }



  /// Advanced interface
  Buffer* inputBuffer()
  { return &inputBuffer_; }

  Buffer* outputBuffer()
  { return &outputBuffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  /// Server Conn 4 种状态
  /// 
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop* loop_;
  const string name_;
  
  StateE state_;  // FIXME: use atomic variable
  bool reading_;

  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;

  CloseCallback closeCallback_;  /// 关闭回调

  size_t highWaterMark_;
  Buffer inputBuffer_;
  Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.
  boost::any context_;
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;     /// TcpConnection 指针

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_TCPCONNECTION_H
