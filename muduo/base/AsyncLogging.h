// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

// 无界阻塞队列
#include "muduo/base/BlockingQueue.h"
// 有界阻塞队列
#include "muduo/base/BoundedBlockingQueue.h"
// 倒计时门闩 同步
#include "muduo/base/CountDownLatch.h"
// 互斥锁
#include "muduo/base/Mutex.h"
// 线程
#include "muduo/base/Thread.h"
// 日志流
#include "muduo/base/LogStream.h"

// cpp 原子类
#include <atomic>
// 动态的连续数组
#include <vector>

namespace muduo
{

class AsyncLogging : noncopyable
{
 public:

  /**
   * basename 日志路径名字
   * rollSize 多大切割一次
   * flushInterval 多久冲刷一次
   */
  AsyncLogging(const string& basename,
               off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop(); // 停止运行
    }
  }

  /**
   * 追加日志
   * @param logline [日志文本]
   * @param len     [长度]
   */
  void append(const char* logline, int len);

  void start()
  {
    running_ = true;

    thread_.start(); // 线程启动
    latch_.wait();   // ???
  }

  void stop() NO_THREAD_SAFETY_ANALYSIS
  {
    running_ = false;

    cond_.notify();  // 条件变量通知

    thread_.join();  // 回收线程
  }

 private:

  // 线程函数
  void threadFunc();

  /**
   * 定义几个类型
   */
  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;
  std::atomic<bool> running_;
  const string basename_;
  const off_t rollSize_;


  muduo::Thread thread_;
  muduo::CountDownLatch latch_;
  muduo::MutexLock mutex_;

  // 条件变量
  muduo::Condition cond_ GUARDED_BY(mutex_);

  // 当前的Buffer
  BufferPtr currentBuffer_ GUARDED_BY(mutex_);
  // 下一个Buffer
  BufferPtr nextBuffer_ GUARDED_BY(mutex_);
  
  BufferVector buffers_ GUARDED_BY(mutex_);
};

}  // namespace muduo

#endif  // MUDUO_BASE_ASYNCLOGGING_H
