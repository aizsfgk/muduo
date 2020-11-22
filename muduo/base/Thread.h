// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "muduo/base/Atomic.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>     // 智能指针
#include <pthread.h>

namespace muduo
{

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;  // 

  explicit Thread(ThreadFunc, const string& name = string());
  // FIXME: make it movable in C++11
  ~Thread();

  // 启动
  void start();
  // 连接
  int join(); // return pthread_join()

  // 返回是否启动
  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  // 
  // 返回线程ID
  pid_t tid() const { return tid_; }
  // 返回线程名
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

 private:
  void setDefaultName();

  bool       started_;   // 是否启动
  bool       joined_;    // 是否连接
  pthread_t  pthreadId_; // 
  pid_t      tid_;
  ThreadFunc func_;      // 启动函数
  string     name_;
  CountDownLatch latch_; // 倒计时门闩

  static AtomicInt32 numCreated_;
};

}  // namespace muduo
#endif  // MUDUO_BASE_THREAD_H
