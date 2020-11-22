// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include "muduo/base/Atomic.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"

namespace muduo
{
namespace net
{

///
/// Internal class for timer event.
///
class Timer : noncopyable
{
 public:
  Timer(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when), // 过期时间
      interval_(interval), // 间隔
      repeat_(interval > 0.0), // 是否重复
      sequence_(s_numCreated_.incrementAndGet())
  { }

  // 直接调用回调函数
  void run() const
  {
    callback_();
  }

  // 过期时间
  Timestamp expiration() const  { return expiration_; }
  // 是否重复
  bool repeat() const { return repeat_; }
  // 序列号
  int64_t sequence() const { return sequence_; }

  /*
    重新启动
    1. 如果是重复的
         now时间 + 间隔
    2. 否则
         返回一个 0 时间
   */
  void restart(Timestamp now);

  // 创建了几个定时器
  static int64_t numCreated() { return s_numCreated_.get(); }

 private:
  const TimerCallback callback_;  // 回调函数
  Timestamp expiration_;          // 过期时间
  const double interval_;         // 间隔时间
  const bool repeat_;             // 是否重复
  const int64_t sequence_;        // 序列号

  static AtomicInt64 s_numCreated_;   // static  分布在静态存储区
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_TIMER_H
