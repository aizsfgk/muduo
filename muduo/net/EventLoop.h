// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : noncopyable
{
 public:
  typedef std::function<void()> Functor; // 定义一个函数

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  /// 
  /// 必须是相同的线程调用
  ///
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.

  /// poll 返回的时间，通常意味着数据到达
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  /// 迭代器
  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  /// 立即运行回调函数在这个loop线程中
  /// 它唤醒loop并允许cb
  /// 如果在相同的loopthread, cb被运行
  /// 线程安全的
  void runInLoop(Functor cb);
  
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  /// 队列化调用函数， 在poll池结束后执行
  /// 线程安全的
  void queueInLoop(Functor cb);

  /// 队列大小
  size_t queueSize() const;

  // timers

  /// 在指定时间运行
  /// 
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId runAt(Timestamp time, TimerCallback cb);
  
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  /// 延迟delay 运行
  TimerId runAfter(double delay, TimerCallback cb);
  
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  /// 间隔多久运行
  TimerId runEvery(double interval, TimerCallback cb);
  
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  /// 取消定时器
  void cancel(TimerId timerId);

  // internal usage
  // ********** 内部使用 ************** //
  void wakeup();

  
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

  
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return eventHandling_; }


  /**
   * 上下文保存相关数据
   * @param context [description]
   */
  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  
  void handleRead();  // waked up
  
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;   

  bool looping_; /* atomic */  /// 是否正在looping
  std::atomic<bool> quit_;     /// 是否退出

  bool eventHandling_; /* atomic */      /// 是否正在时间处理中
  bool callingPendingFunctors_; /* atomic */  /// 是否正在调用pending函数

  int64_t iteration_;
  const pid_t threadId_;                   // 保存启动该Loop的线程ID
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;         // Poller

  std::unique_ptr<TimerQueue> timerQueue_; // 定时器队列指针


  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeupChannel_; /// 唤醒Channel
  
  boost::any context_;

  // scratch variables
  ChannelList activeChannels_;
  Channel* currentActiveChannel_;

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_); /// 接下来执行的函数
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
