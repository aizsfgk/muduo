// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "muduo/net/TimerQueue.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Timer.h"
#include "muduo/net/TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace muduo
{
namespace net
{
namespace detail
{


/**
 * 4个方法
 *   1. 创建timerfd
 *   2. 和当前相差多久
 *   3. 读取timerfd; 用read函数读取计时器的超时次数，改值是一个8字节无符号的长整型
 *   4. 重置timerfd
 *
 * 来源：https://www.cnblogs.com/wenqiang/p/6698371.html
 */
int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
  {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

/**
 * 用read函数读取计时器的超时次数，改值是一个8字节无符号的长整型
 */
void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany)
  {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  memZero(&newValue, sizeof newValue);
  memZero(&oldValue, sizeof oldValue);


  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret)
  {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}  // namespace detail
}  // namespace net
}  // namespace muduo

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

/**
 * 10个TimerQueue相关函数
 *
 * 
 */
TimerQueue::TimerQueue(EventLoop* loop)
  : loop_(loop),
    timerfd_(createTimerfd()),    /// 一个TimerFd可以维护多个Timer
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{

  // 注册可读函数
  timerfdChannel_.setReadCallback(
      std::bind(&TimerQueue::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  // 
  // 激活可读关注
  timerfdChannel_.enableReading(); // 
}

TimerQueue::~TimerQueue()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);    // 析构的时候销毁
  // do not remove channel, since we're in EventLoop::dtor();
  for (const Entry& timer : timers_)
  {
    delete timer.second;
  }
}


/*
  添加定时器

 */
TimerId TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
  // 1. 新建一个Timer
  Timer* timer = new Timer(std::move(cb), when, interval);

  // 运行一次
  loop_->runInLoop(
      std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
  loop_->runInLoop(
      std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
  loop_->assertInLoopThread();
  bool earliestChanged = insert(timer);// 先插入

  // 如果是最早的，重置定时器
  if (earliestChanged)
  {
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());

  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  if (it != activeTimers_.end())
  {
    // 删除操作
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1); (void)n;
    delete it->first; // FIXME: no delete please
    activeTimers_.erase(it);
  }
  else if (callingExpiredTimers_)
  { 
    // 加入到取消定时器
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}


/*
   如果定时器FD可读的时候，调用该函数处理

 */
void TimerQueue::handleRead()
{
  loop_->assertInLoopThread();

  // 当前时间 
  Timestamp now(Timestamp::now());
  // 读取一下定时器的超时次数； 必须是0次; 否则报错
  readTimerfd(timerfd_, now);

  // 获取过期时间的定时器
  // 
  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (const Entry& it : expired)
  {
    it.second->run();
  }
  callingExpiredTimers_ = false;   // 不再调用过期函数

  reset(expired, now);     // 重置
}


// 获取已经过期的 定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
  assert(timers_.size() == activeTimers_.size());

  // typedef std::pair<Timestamp, Timer*> Entry;
  std::vector<Entry> expired;

  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));   // 指针的最大值

  TimerList::iterator end = timers_.lower_bound(sentry);      // 返回不小于某个值的迭代器

  assert(end == timers_.end() || now < end->first);

  std::copy(timers_.begin(), end, back_inserter(expired));    // 要复制的元素范围； 目标范围的起始地址：back_inserter(expired) // 能用于添加元素到容器 c 尾端的 std::back_insert_iterator
  timers_.erase(timers_.begin(), end);   // 删除元素

  for (const Entry& it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = activeTimers_.erase(timer);  // 删除该激活的定时器
    assert(n == 1); (void)n;
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}


// 重置
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
  Timestamp nextExpire;

  for (const Entry& it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repeat()
        && cancelingTimers_.find(timer) == cancelingTimers_.end())
    {
      // 重新启动
      it.second->restart(now);
      // 并插入 timers_
      insert(it.second);
    }
    else
    {
      // FIXME move to a free list
      delete it.second; // FIXME: no delete please
    }
  }


  /// typedef std::set<Entry> TimerList;                /// 定时器列表;   红黑树，所以是有序的
  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();   /// 最近的过期时间
  }

  if (nextExpire.valid())     /// 下次过期时间 > 0
  {
    resetTimerfd(timerfd_, nextExpire);    /// 重置定时器
  }
}

/*
   
 */
bool TimerQueue::insert(Timer* timer)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());

  bool earliestChanged = false;
  Timestamp when = timer->expiration();   /// 当前定时器的过期时间
  TimerList::iterator it = timers_.begin();


  if (it == timers_.end() || when < it->first)
  {
    earliestChanged = true;
  }
  {
    std::pair<TimerList::iterator, bool> result
      = timers_.insert(Entry(when, timer));
    assert(result.second); (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result
      = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second); (void)result;
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}

