// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "muduo/base/Mutex.h"

#include <pthread.h>

namespace muduo
{

/*
  1. 线程在准备检查共享状态时，锁定互斥量
  2. 检查共享变量的状态
  3. 如果共享变量未处于预期状态，线程应在等待条件变量并进入休眠前，解锁互斥量
  4. 当线程因为条件变量的通知而被再度唤醒时，必须对互斥量进行再次加锁，因为典型的情况下，线程
     会立即访问共享变量。


     
 */
class Condition : noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)
    : mutex_(mutex)
  {
    MCHECK(pthread_cond_init(&pcond_, NULL)); // 条件变量初始化
  }

  ~Condition()
  {
    MCHECK(pthread_cond_destroy(&pcond_));   // 条件变量销毁
  }

  void wait()
  {
    MutexLock::UnassignGuard ug(mutex_);
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));      /// 阻塞等待在该条件变量上
  }

  // returns true if time out, false otherwise.
  bool waitForSeconds(double seconds);

  void notify()
  {
    MCHECK(pthread_cond_signal(&pcond_));       // 至少唤醒一个
  }

  void notifyAll()
  {
    MCHECK(pthread_cond_broadcast(&pcond_));    // 唤醒全部
  }

 private:
  MutexLock& mutex_;     /// 互斥量的引用
  pthread_cond_t pcond_; /// 条件变量(通知状态的改变)
};

}  // namespace muduo

#endif  // MUDUO_BASE_CONDITION_H
