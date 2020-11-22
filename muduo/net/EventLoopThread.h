// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"


/*

1. 线程和Loop绑定

*/

namespace muduo
{
namespace net
{

class EventLoop;   // 事件循环

class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback; /// 线程初始化回调

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string()); /// 线程回调和线程名字
  ~EventLoopThread();
  
  EventLoop* startLoop();    //// 开始事件循环

 private:
  void threadFunc();    // 线程启动的函数

  EventLoop* loop_ GUARDED_BY(mutex_); /// 事件循环指针
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_ GUARDED_BY(mutex_);
  ThreadInitCallback callback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

